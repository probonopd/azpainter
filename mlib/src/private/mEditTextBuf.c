/*$
Copyright (c) 2017, Azel
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERSAND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
$*/

/*****************************************
 * mEditTextBuf
 *
 * テキスト入力のバッファと処理
 *****************************************/

#include "mDef.h"
#include "mKeyDef.h"
#include "mUtilCharCode.h"
#include "mClipboard.h"
#include "mFont.h"
#include "mWidget.h"

#include "mEditTextBuf.h"


/*
 * allocLen : 確保されているバッファの長さ (NULL 含む)
 *
 * curPos : カーソルの文字位置
 * selTop : 選択範囲の先頭位置 (-1 でなし)
 * selEnd : 選択範囲の終端位置 +1
 *
 * maxlines, maxwidth : 複数行時の最大行数と最大幅 (内容が変更されると常に再計算される)
 */


//===========================
// sub
//===========================


/** 文字列の長さから確保長さ取得 */

static int _calcAllocLen(int len)
{
	int i;
	
	for(i = 32; i < (1<<28) && i <= len; i <<= 1);
	
	return i;
}

/** UTF-8 -> UCS-4
 * 
 * @param dst NULL で文字数のみ計算 */

static int _utf8_to_ucs(const char *src,uint32_t *dst,mBool bMulti)
{
	const char *ps;
	uint32_t ucs;
	int ret,len = 0,last_cr = 0;
	
	for(ps = src; *ps; )
	{
		ret = mUTF8ToUCS4Char(ps, -1, &ucs, &ps);
		
		if(ret < 0) break;
		if(ret > 0) continue;
		
		if(ucs == '\r' || ucs == '\n')
		{
			if(!bMulti) continue;
			
			//改行は '\n' で統一
			
			if(ucs == '\r')
			{
				ucs = '\n';
				last_cr = 1;
			}
			else if(ucs == '\n')
			{
				if(last_cr) continue;
			}
		}
		else
		{
			last_cr = 0;
		
			if(ucs < 128 && !(ucs >= 32 && ucs < 127)) continue;
		}
		
		if(dst) *(dst++) = ucs;
		len++;
	}
	
	return len;
}

/** 文字幅バッファに全文字列の幅をセット */

static void _setwidthbuf_full(mEditTextBuf *p)
{
	mFont *font;
	uint32_t *pc;
	uint16_t *pw;

	font = mWidgetGetFont(p->widget);

	pc = p->text;
	pw = p->widthbuf;

	for(; *pc; pc++, pw++)
	{
		if(*pc == '\n')
			*pw = 0;
		else
			*pw = mFontGetCharWidthUCS4(font, *pc);
	}
}

/** 文字カーソル位置から複数行用のカーソル位置を計算 [MULTI] */

static void _calcCursorPos_forMulti(mEditTextBuf *p)
{
	uint32_t *pc;
	uint16_t *pw;
	int i,x,line;
	
	if(!p->bMulti) return;

	pc = p->text;
	pw = p->widthbuf;

	x = line = 0;

	for(i = p->curPos; i > 0; i--, pc++, pw++)
	{
		if(*pc == '\n')
		{
			line++;
			x = 0;
		}
		else
			x += *pw;
	}

	p->multi_curx = x;
	p->multi_curline = line;
}

/** 最大幅と最大行数を計算 [MULTI] */

static void _calcMaxWidthAndLines_forMulti(mEditTextBuf *p)
{
	uint32_t *pc;
	uint16_t *pw;
	int maxw,lines,linew;

	if(!p->bMulti) return;

	pc = p->text;
	pw = p->widthbuf;

	lines = 1;
	linew = maxw = 0;

	for(; *pc; pc++, pw++)
	{
		if(*pc == '\n')
		{
			if(maxw < linew) maxw = linew;
			linew = 0;
			lines++;
		}
		else
			linew += *pw;
	}

	if(maxw < linew) maxw = linew;

	p->maxlines = lines;
	p->maxwidth = maxw;
}

/** カーソル上下移動時、カーソル位置から移動後の文字位置を取得 [MULTI] */

static int _getUpDownCursorCharPos_forMulti(mEditTextBuf *p,mBool up)
{
	uint32_t *pc;
	uint16_t *pw;
	int i,x,line,curx,newline;

	pc = p->text;
	pw = p->widthbuf;
	curx = p->multi_curx;

	newline = p->multi_curline + (up? -1: 1);

	for(i = x = line = 0; *pc; pc++, pw++, i++)
	{
		//移動後の行で、現在カーソルのX位置より右ならその位置
		
		if(line == newline)
		{
			if(up)
			{
				if(x + *pw >= curx) break;
			}
			else
			{
				if(x >= curx) break;
			}
		}

		//改行

		if(*pc == '\n')
		{
			if(line == newline) break;

			line++;
			x = 0;
		}
		else
			x += *pw;
	}

	return i;
}


//===========================
// main
//===========================


/** 解放 */

void mEditTextBuf_free(mEditTextBuf *p)
{
	if(p)
	{
		mFree(p->text);
		mFree(p->widthbuf);
		mFree(p);
	}
}

/** 作成 */

mEditTextBuf *mEditTextBuf_new(mWidget *widget,mBool multiline)
{
	mEditTextBuf *p;
	
	p = (mEditTextBuf *)mMalloc(sizeof(mEditTextBuf), TRUE);
	if(!p) return NULL;
		
	//テキストバッファ (初期長さ = 32)
	
	p->text = (uint32_t *)mMalloc(sizeof(uint32_t) * 32, FALSE);
	if(!p->text)
	{
		mFree(p);
		return NULL;
	}

	//文字幅バッファ

	if(multiline)
	{
		p->widthbuf = (uint16_t *)mMalloc(2 * 32, FALSE);
		if(!p->widthbuf)
		{
			mEditTextBuf_free(p);
			return NULL;
		}
	}

	//
	
	p->text[0] = 0;
	p->allocLen = 32;
	
	p->widget = widget;
	p->selTop = -1;
	p->bMulti = multiline;

	return p;
}

/** テキストバッファのリサイズ */

mBool mEditTextBuf_resize(mEditTextBuf *p,int len)
{
	int newlen;
	void *pnew;

	//確保されている

	if(len < p->allocLen) return TRUE;

	//確保サイズ
	
	newlen = _calcAllocLen(len);

	//テキストバッファ
	
	pnew = mRealloc(p->text, sizeof(uint32_t) * newlen);
	if(!pnew) return FALSE;
	
	p->text = (uint32_t *)pnew;

	//文字幅バッファ

	if(p->bMulti)
	{
		pnew = mRealloc(p->widthbuf, 2 * newlen);
		if(!pnew) return FALSE;

		p->widthbuf = (uint16_t *)pnew;
	}
	
	p->allocLen = newlen;
	
	return TRUE;
}

/** 空にする */

void mEditTextBuf_empty(mEditTextBuf *p)
{
	p->text[0] = 0;
	p->textLen = 0;
	p->curPos = 0;
	p->selTop = -1;

	p->maxlines = 1;
	p->maxwidth = 0;
	p->multi_curx = 0;
	p->multi_curline = 0;
}

/** 空かどうか */

mBool mEditTextBuf_isEmpty(mEditTextBuf *p)
{
	return (p->textLen == 0);
}

/** 指定文字位置が選択範囲内か */

mBool mEditTextBuf_isSelectAtPos(mEditTextBuf *p,int pos)
{
	return (p->selTop != -1 && p->selTop <= pos && pos < p->selEnd);
}

/** カーソルを終端へ移動 */

void mEditTextBuf_cursorToBottom(mEditTextBuf *p)
{
	p->curPos = p->textLen;

	_calcCursorPos_forMulti(p);
}

/** ピクセル位置から文字位置を取得 [MULTI] */

int mEditTextBuf_getPosAtPixel_forMulti(mEditTextBuf *p,int px,int py,int lineh)
{
	uint32_t *pc;
	uint16_t *pw;
	int x,y,pos;

	if(px < 0) px = 0;
	if(py < 0) py = 0;

	pc = p->text;
	pw = p->widthbuf;

	x = y = pos = 0;

	for(; *pc; pc++, pw++, pos++)
	{
		//文字の範囲内
		
		if(x <= px && px < x + *pw && y <= py && py < y + lineh) break;

		//

		if(*pc == '\n')
		{
			if(py < y + lineh) break;
		
			x = 0;
			y += lineh;
		}
		else
			x += *pw;
	}

	return pos;
}


//================================
// テキストセット
//================================


/** テキストセット
 * 
 * カーソル位置は先頭へ。選択範囲はなしとなる。 */

void mEditTextBuf_setText(mEditTextBuf *p,const char *text)
{
	int len;

	//空にする

	mEditTextBuf_empty(p);

	if(!text) return;
	
	//文字数
	
	len = _utf8_to_ucs(text, NULL, p->bMulti);
	if(len == 0) return;
	
	if(!mEditTextBuf_resize(p, len)) return;
	
	//テキストセット
	
	_utf8_to_ucs(text, p->text, p->bMulti);
	
	p->text[len] = 0;
	p->textLen = len;

	//複数行時

	if(p->bMulti)
	{
		_setwidthbuf_full(p);
		_calcMaxWidthAndLines_forMulti(p);
	}
}

/** テキスト/文字挿入
 * 
 * @param ch 0 以外で文字を挿入、0 で text の文字列を挿入 */

mBool mEditTextBuf_insertText(mEditTextBuf *p,const char *text,char ch)
{
	int len;
	uint32_t *ts,*td;
	uint16_t *ws,*wd;
	mFont *font;
	char m[2];
	int i;
	
	if(ch)
	{
		//文字挿入
		
		m[0] = ch;
		m[1] = 0;
		
		text = m;
	}
	else
	{
		if(!text) return FALSE;
	}
	
	//文字数
	
	len = _utf8_to_ucs(text, NULL, p->bMulti);
	if(len == 0) return FALSE;
	
	//選択範囲の文字列を削除
	
	mEditTextBuf_deleteSelText(p);
	
	if(!mEditTextBuf_resize(p, p->textLen + len))
		return FALSE;

	//挿入位置を空ける (テキスト)
	
	ts = p->text + p->textLen;
	td = ts + len;
	
	for(i = p->textLen - p->curPos; i >= 0; i--)
		*(td--) = *(ts--);
	
	//テキストセット
	
	_utf8_to_ucs(text, p->text + p->curPos, p->bMulti);

	//複数行時

	if(p->bMulti)
	{
		//挿入位置を空ける (文字幅)
		
		ws = p->widthbuf + p->textLen;
		wd = ws + len;
		
		for(i = p->textLen - p->curPos; i >= 0; i--)
			*(wd--) = *(ws--);

		//文字幅セット

		font = mWidgetGetFont(p->widget);

		ts = p->text + p->curPos;
		ws = p->widthbuf + p->curPos;

		for(i = len; i > 0; i--, ts++)
			*(ws++) = mFontGetCharWidthUCS4(font, *ts);
	}
	
	//
	
	p->textLen += len;
	p->curPos += len;

	//複数行時
	
	_calcMaxWidthAndLines_forMulti(p);
	_calcCursorPos_forMulti(p);

	return TRUE;
}


//================================
// 処理
//================================


/** すべて選択 */

mBool mEditTextBuf_selectAll(mEditTextBuf *p)
{
	if(p->textLen == 0)
		return FALSE;
	else
	{
		p->selTop = 0;
		p->selEnd = p->textLen;
		return TRUE;
	}
}

/** 選択範囲の文字列削除 */

mBool mEditTextBuf_deleteSelText(mEditTextBuf *p)
{
	if(p->selTop < 0)
		return FALSE;
	else
	{
		mEditTextBuf_deleteText(p, p->selTop, p->selEnd - p->selTop);
	
		p->selTop = -1;
	
		return TRUE;
	}
}

/** 指定範囲の文字列削除 */

void mEditTextBuf_deleteText(mEditTextBuf *p,int pos,int len)
{
	uint32_t *ts,*td;
	uint16_t *ws,*wd;
	int i;
	
	//詰める (テキスト)
	
	td = p->text + pos;
	ts = td + len;
		
	for(i = p->textLen - (pos + len); i > 0; i--)
		*(td++) = *(ts++);
	
	*td = 0;

	//詰める (文字幅)

	if(p->bMulti)
	{
		wd = p->widthbuf + pos;
		ws = wd + len;
			
		for(i = p->textLen - (pos + len); i > 0; i--)
			*(wd++) = *(ws++);
	}

	//
	
	p->textLen -= len;
	p->curPos = pos;

	//複数行時
	
	_calcMaxWidthAndLines_forMulti(p);
	_calcCursorPos_forMulti(p);
}

/** カーソルを指定文字位置に移動
 *
 * @param select 0 以外で選択範囲の移動 */

void mEditTextBuf_moveCursorPos(mEditTextBuf *p,int pos,int select)
{
	if(select)
		mEditTextBuf_expandSelect(p, pos);
	else
		p->selTop = -1;
	
	p->curPos = pos;

	_calcCursorPos_forMulti(p);
}

/** 選択範囲拡張 (カーソル位置から指定位置まで) */

void mEditTextBuf_expandSelect(mEditTextBuf *p,int pos)
{
	if(p->selTop < 0)
	{
		//選択なし -> 選択開始
		
		if(pos < p->curPos)
			p->selTop = pos, p->selEnd = p->curPos;
		else
			p->selTop = p->curPos, p->selEnd = pos;
	}
	else
	{
		//選択あり -> 拡張
		
		if(pos < p->curPos)
		{
			if(pos < p->selTop)
				p->selTop = pos;
			else
				p->selEnd = pos;
		}
		else if(pos > p->curPos)
		{
			if(pos > p->selEnd)
				p->selEnd = pos;
			else
				p->selTop = pos;
		}
		
		if(p->selTop == p->selEnd)
			p->selTop = -1;
	}
}

/** ドラッグによる選択範囲拡張時 */

void mEditTextBuf_dragExpandSelect(mEditTextBuf *p,int pos)
{
	mEditTextBuf_expandSelect(p, pos);
	
	p->curPos = pos;
	
	_calcCursorPos_forMulti(p);
}

/** 貼り付け */

mBool mEditTextBuf_paste(mEditTextBuf *p)
{
	char *text;
	
	text = mClipboardGetText(p->widget);
	if(!text)
		return FALSE;
	else
	{
		mEditTextBuf_insertText(p, text, 0);
	
		mFree(text);
	
		return TRUE;
	}
}

/** コピー */

void mEditTextBuf_copy(mEditTextBuf *p)
{
	char *utf8;
	int len;

	if(p->selTop != -1)
	{
		utf8 = mUCS4ToUTF8_alloc(p->text + p->selTop,
			p->selEnd - p->selTop, &len);
	
		if(utf8)
		{
			mClipboardSetText(p->widget, utf8, len + 1);
			
			mFree(utf8);
		}
	}
}

/** 切り取り */

mBool mEditTextBuf_cut(mEditTextBuf *p)
{
	if(p->selTop == -1)
		return FALSE;
	else
	{
		mEditTextBuf_copy(p);
		mEditTextBuf_deleteSelText(p);
	
		return TRUE;
	}
}


//================================
// 操作処理
//================================


/** カーソルを上下左右に移動
 * 
 * @param select 選択範囲の移動
 * @return 移動したか */

static int _move_cur_dir(mEditTextBuf *p,uint32_t key,int select)
{
	int pos = 0;
	
	switch(key)
	{
		//左
		case MKEY_LEFT:
			if(!select && p->selTop != -1)
				pos = p->selTop;  //選択解除で、選択先頭へ
			else if(p->curPos)
				pos = p->curPos - 1;
			else
				return FALSE;
			break;
		//右
		case MKEY_RIGHT:
			if(!select && p->selTop != -1)
				pos = p->selEnd;  //選択解除で、選択終端へ
			else if(p->curPos < p->textLen)
				pos = p->curPos + 1;
			else
				return FALSE;
			break;
		//上
		case MKEY_UP:
			if(!p->bMulti || p->multi_curline == 0)
				return FALSE;
			else
				pos = _getUpDownCursorCharPos_forMulti(p, TRUE);
			break;
		//下
		case MKEY_DOWN:
			if(!p->bMulti || p->multi_curline == p->maxlines - 1)
				return FALSE;
			else
				pos = _getUpDownCursorCharPos_forMulti(p, FALSE);
			break;
	}
	
	mEditTextBuf_moveCursorPos(p, pos, select);

	return TRUE;
}

/** 先頭へ移動 */

static void _move_cur_home(mEditTextBuf *p)
{
	if(!p->bMulti)
		p->curPos = 0;
	else
	{
		//複数行時は行の先頭

		uint32_t *pc = p->text + p->curPos;
		int pos = p->curPos;

		for(; pos > 0; pos--, pc--)
		{
			if(pc[-1] == '\n') break;
		}

		p->curPos = pos;

		_calcCursorPos_forMulti(p);
	}
}

/** 終端へ移動 */

static void _move_cur_end(mEditTextBuf *p)
{
	if(!p->bMulti)
		p->curPos = p->textLen;
	else
	{
		//複数行時は行の終端

		uint32_t *pc = p->text + p->curPos;

		for(; *pc && *pc != '\n'; pc++);
		
		p->curPos = pc - p->text;

		_calcCursorPos_forMulti(p);
	}
}

/** バックスペース/削除 */

static int _key_delete(mEditTextBuf *p,int backspace)
{
	if(p->selTop >= 0)
	{
		//選択がある場合は、選択文字列を削除
	
		if(mEditTextBuf_deleteSelText(p))
			return MEDITTEXTBUF_KEYPROC_TEXT;
	}
	else if(backspace)
	{
		//バックスペース : カーソルの一つ前の文字を削除
		
		if(p->curPos > 0)
		{
			mEditTextBuf_deleteText(p, p->curPos - 1, 1);
			return MEDITTEXTBUF_KEYPROC_TEXT;
		}
	}
	else
	{
		//DELETE
		
		if(p->curPos < p->textLen)
		{
			mEditTextBuf_deleteText(p, p->curPos, 1);
			return MEDITTEXTBUF_KEYPROC_TEXT;
		}
	}
	
	return 0;
}

/** キー処理
 *
 * @param editok 編集可能 */

int mEditTextBuf_keyProc(mEditTextBuf *p,uint32_t key,uint32_t state,int editok)
{
	int ret,ctrl;
	char m[2];
	
	ret = 0;
	ctrl = state & M_MODS_CTRL;
	
	switch(key)
	{
		//ENTER
		case MKEY_ENTER:
			if(p->bMulti && editok)
			{
				m[0] = '\n';
				m[1] = 0;

				if(mEditTextBuf_insertText(p, m, 0))
					ret = MEDITTEXTBUF_KEYPROC_TEXT;
			}
			break;
		//上下左右
		case MKEY_LEFT:
		case MKEY_RIGHT:
		case MKEY_UP:
		case MKEY_DOWN:
			if(_move_cur_dir(p, key, state & M_MODS_SHIFT))
				ret = MEDITTEXTBUF_KEYPROC_CURSOR;
			break;
		//後退
		case MKEY_BACKSPACE:
			if(editok)
				ret = _key_delete(p, TRUE);
			break;
		//削除
		case MKEY_DELETE:
			if(editok)
				ret = _key_delete(p, FALSE);
			break;
		//HOME
		case MKEY_HOME:
			_move_cur_home(p);
			ret = MEDITTEXTBUF_KEYPROC_CURSOR;
			break;
		//END
		case MKEY_END:
			_move_cur_end(p);
			ret = MEDITTEXTBUF_KEYPROC_CURSOR;
			break;
		//Ctrl+V
		case 'V':
			if(ctrl && editok)
			{
				if(mEditTextBuf_paste(p))
					ret = MEDITTEXTBUF_KEYPROC_TEXT;
			}
			break;
		//Ctrl+C
		case 'C':
			if(ctrl)
				mEditTextBuf_copy(p);
			break;
		//Ctrl+X
		case 'X':
			if(ctrl && editok)
			{
				if(mEditTextBuf_cut(p))
					ret = MEDITTEXTBUF_KEYPROC_TEXT;
			}
			break;
		//Ctrl+A
		case 'A':
			if(ctrl)
			{
				if(mEditTextBuf_selectAll(p))
					ret = MEDITTEXTBUF_KEYPROC_UPDATE;
			}
			break;
	}
	
	return ret;
}
