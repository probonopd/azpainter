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
 * mLineEdit [一行テキスト入力]
 *****************************************/

#include <stdlib.h>
#include <stdio.h>

#include "mDef.h"

#include "mLineEdit.h"

#include "mWidget.h"
#include "mPixbuf.h"
#include "mFont.h"
#include "mKeyDef.h"
#include "mStr.h"
#include "mEvent.h"
#include "mSysCol.h"
#include "mUtilStr.h"
#include "mUtilCharCode.h"

#include "mEditTextBuf.h"


//-------------------------

#define _SPIN_W    13

#define _PRESS_F_DRAG       1
#define _PRESS_F_SPINUP     2
#define _PRESS_F_SPINDOWN   3

#define _TIMERID_SPIN_START   0
#define _TIMERID_SPIN_REPEAT  1

//--------------------------


/*************************//**

@defgroup lineedit mLineEdit
@brief 一行テキスト入力

<h3>継承</h3>
mWidget \> mLineEdit

@ingroup group_widget

@{

@file mLineEdit.h
@def M_LINEEDIT(p)
@struct mLineEditData
@struct mLineEdit
@enum MLINEEDIT_STYLE
@enum MLINEEDIT_NOTIFY

@var MLINEEDIT_STYLE::MLINEEDIT_S_NOTIFY_CHANGE
内容が変化した時に通知する

@var MLINEEDIT_STYLE::MLINEEDIT_S_NOTIFY_ENTER
ENTER キーが押された時に通知する

@var MLINEEDIT_STYLE::MLINEEDIT_S_READONLY
読み込み専用

@var MLINEEDIT_STYLE::MLINEEDIT_S_SPIN
数値入力用のスピンを付ける

@var MLINEEDIT_STYLE::MLINEEDIT_S_NO_FRAME
枠を付けない


@var MLINEEDIT_NOTIFY::MLINEEDIT_N_CHANGE
内容が変化した時

@var MLINEEDIT_NOTIFY::MLINEEDIT_N_ENTER
ENTER キーが押された時

********************************/



//=====================================
// sub
//=====================================


/** テキスト部分の右端位置取得 */

static int _getTextRight(mLineEdit *p)
{
	if(p->le.style & MLINEEDIT_S_SPIN)
		return p->wg.w - 1 - _SPIN_W - 2;
	else
		return p->wg.w - p->le.padding;
}

/** スクロール位置調整
 *
 * カーソル位置がウィジェット範囲外の場合、調整 */

static void _adjustScroll(mLineEdit *p)
{
	int x,w;
	
	x = p->le.curX;
	w = _getTextRight(p) - p->le.padding;
	
	if(x - p->le.scrX < 0)		//カーソルが左に隠れている
		p->le.scrX = x;
	else if(x - p->le.scrX >= w)	//カーソルが右に隠れている
		p->le.scrX = x - w + 1;
}

/** スクロール最大位置調整
 *
 * 現在のスクロール位置と最大位置を比較 */

static void _adjustScroll_max(mLineEdit *p)
{
	int maxw,w;
	
	maxw = mFontGetTextWidthUCS4(
		mWidgetGetFont(M_WIDGET(p)),
		p->le.buf->text, p->le.buf->textLen);

	w = _getTextRight(p) - p->le.padding;

	if(maxw <= w)
		//ウィジェット幅の方が大きい
		p->le.scrX = 0;
	else if(p->le.scrX > maxw + 1 - w)
		//テキスト最大位置が右端になるように
		p->le.scrX = maxw + 1 - w;
}

/** カーソル位置変更時
 *
 * @param adjust_scrmax スクロール最大値調整
 * (カーソル位置を任意の位置に移動した時など) */

static void _changeCurPos(mLineEdit *p,mBool adjust_scrmax)
{
	p->le.curX = mFontGetTextWidthUCS4(
		mWidgetGetFont(M_WIDGET(p)),
		p->le.buf->text, p->le.buf->curPos);

	if(adjust_scrmax)
		_adjustScroll_max(p);

	_adjustScroll(p);
	
	//IM 位置

	p->wg.imX = p->le.curX - p->le.scrX + p->le.padding;
}

/** 文字列変更時 */

static void _changeText(mLineEdit *p)
{
	_changeCurPos(p, TRUE);

	//CHANGE 通知

	if(p->le.style & MLINEEDIT_S_NOTIFY_CHANGE)
	{
		mWidgetAppendEvent_notify(NULL,
			M_WIDGET(p), MLINEEDIT_N_CHANGE, 0, 0);
	}
}

/** マウス位置から文字列位置取得 */

static int _getPtToTextPos(mLineEdit *p,int ptx)
{
	mFont *font;
	uint32_t *pt;
	int x = 0,w,i;
	
	ptx = ptx - p->le.padding + p->le.scrX;
	
	if(ptx <= 0) return 0;
	
	font = mWidgetGetFont(M_WIDGET(p));
	
	for(pt = p->le.buf->text, i = 0; *pt; pt++, i++)
	{
		w = mFontGetCharWidthUCS4(font, *pt);
		
		//文字の半分以上なら次の文字
		
		if(x <= ptx && ptx < x + w)
			return i + (ptx - x >= w / 2);
		
		x += w;
	}
	
	return i;
}

/** スピン操作による値上下 */

static void _spinUpDown(mLineEdit *p,mBool up)
{
	int n;
	
	n = mLineEditGetNum(p);
	
	if(up)
	{
		if(n < p->le.numMax) n++;
	}
	else
	{
		if(n > p->le.numMin) n--;
	}
	
	mLineEditSetNum(p, n);
	
	_changeText(p);
}


//=====================================
// メイン
//=====================================


/** 解放処理 */

void mLineEditDestroyHandle(mWidget *p)
{
	mEditTextBuf_free(M_LINEEDIT(p)->le.buf);
}

/** サイズ計算 */

void mLineEditCalcHintHandle(mWidget *p)
{
	p->hintW = M_LINEEDIT(p)->le.padding * 2 + 2;
	p->hintH = M_LINEEDIT(p)->le.padding * 2 + mWidgetGetFontHeight(p);
	
	if(M_LINEEDIT(p)->le.style & MLINEEDIT_S_SPIN)
	{
		p->hintW += _SPIN_W + 1;
		if(p->hintH < 15) p->hintH = 15;
	}
}

/** 作成 */

mLineEdit *mLineEditCreate(mWidget *parent,int id,uint32_t style,
	uint32_t fLayout,uint32_t marginB4)
{
	mLineEdit *p;

	p = mLineEditNew(0, parent, style);
	if(!p) return NULL;

	p->wg.id = id;
	p->wg.fLayout = fLayout;

	mWidgetSetMargin_b4(M_WIDGET(p), marginB4);

	return p;
}

/** 作成 */

mLineEdit *mLineEditNew(int size,mWidget *parent,uint32_t style)
{
	mLineEdit *p;
	
	if(size < sizeof(mLineEdit)) size = sizeof(mLineEdit);
	
	p = (mLineEdit *)mWidgetNew(size, parent);
	if(!p) return NULL;
	
	p->wg.destroy = mLineEditDestroyHandle;
	p->wg.calcHint = mLineEditCalcHintHandle;
	p->wg.draw = mLineEditDrawHandle;
	p->wg.event = mLineEditEventHandle;
	
	p->wg.fState |= MWIDGET_STATE_TAKE_FOCUS;
	p->wg.fEventFilter |= MWIDGET_EVENTFILTER_POINTER | MWIDGET_EVENTFILTER_KEY
		| MWIDGET_EVENTFILTER_CHAR | MWIDGET_EVENTFILTER_STRING | MWIDGET_EVENTFILTER_SCROLL;

	if(style & MLINEEDIT_S_NOTIFY_ENTER)
		p->wg.fAcceptKeys |= MWIDGET_ACCEPTKEY_ENTER;

	p->le.style = style;
	p->le.padding = (style & MLINEEDIT_S_NO_FRAME)? 0: 3;

	p->wg.imX = p->wg.imY = p->le.padding;
	
	//mEditTextBuf
	
	p->le.buf = mEditTextBuf_new(M_WIDGET(p), FALSE);
	if(!p->le.buf)
	{
		mWidgetDestroy(M_WIDGET(p));
		return NULL;
	}
	
	return p;
}

/** 数値情報セット */

void mLineEditSetNumStatus(mLineEdit *p,int min,int max,int dig)
{
	p->le.numMin = min;
	p->le.numMax = max;
	p->le.numDig = dig;
}

/** 文字長さから幅をセット */

void mLineEditSetWidthByLen(mLineEdit *p,int len)
{
	int w;

	w = mFontGetTextWidth(mWidgetGetFont(M_WIDGET(p)), "9", 1);

	p->wg.hintOverW = w * len + p->le.padding * 2 + 1;

	if(p->le.style & MLINEEDIT_S_SPIN)
		p->wg.hintOverW += _SPIN_W;
}

/** テキストセット */

void mLineEditSetText(mLineEdit *p,const char *text)
{
	mEditTextBuf_setText(p->le.buf, text);
	
	p->le.curX = 0;
	p->le.scrX = 0;
	p->wg.imX = p->le.padding;
	
	mWidgetUpdate(M_WIDGET(p));
}

/** UCS-4 テキストセット */

void mLineEditSetText_ucs4(mLineEdit *p,const uint32_t *text)
{
	char *utf8;

	utf8 = mUCS4ToUTF8_alloc(text, -1, NULL);

	mLineEditSetText(p, utf8);

	mFree(utf8);
}

/** int 値を文字列にしてセット */

void mLineEditSetInt(mLineEdit *p,int n)
{
	char m[32];
	
	mIntToStr(m, n);
	
	mLineEditSetText(p, m);
}

/** 数値セット */

void mLineEditSetNum(mLineEdit *p,int num)
{
	char m[32];

	if(num < p->le.numMin)
		num = p->le.numMin;
	else if(num > p->le.numMax)
		num = p->le.numMax;
	
	mFloatIntToStr(m, num, p->le.numDig);
	
	mLineEditSetText(p, m);
}

/** double 値セット
 *
 * 小数点以下が 0 なら通常の数値扱いとする。
 * 
 * @param dig 小数点以下桁数 */

void mLineEditSetDouble(mLineEdit *p,double d,int dig)
{
	double f;
	char m[64];

	f = d - (int)d;

	if(f == 0)
		mLineEditSetInt(p, (int)d);
	else
	{
		snprintf(m, 64, "%.*f", dig, d);
		mLineEditSetText(p, m);
	}
}


//====================


/** 空かどうか */

mBool mLineEditIsEmpty(mLineEdit *p)
{
	return mEditTextBuf_isEmpty(p->le.buf);
}

/** テキスト取得 */

void mLineEditGetTextStr(mLineEdit *p,mStr *str)
{
	mStrSetTextUCS4(str, p->le.buf->text, p->le.buf->textLen);
}

/** int 値に変換して取得 */

int mLineEditGetInt(mLineEdit *p)
{
	return mUCS4StrToFloatInt(p->le.buf->text, 0);
}

/** double 値に変換して取得 */

double mLineEditGetDouble(mLineEdit *p)
{
	char *buf;
	double d;
	
	buf = mUCS4ToUTF8_alloc(p->le.buf->text, p->le.buf->textLen, NULL);
	if(!buf) return 0;

	d = strtod(buf, NULL);
	
	mFree(buf);
	
	return d;
}

/** 数値取得 */

int mLineEditGetNum(mLineEdit *p)
{
	int n;

	n = mUCS4StrToFloatInt(p->le.buf->text, p->le.numDig);
	
	if(n < p->le.numMin)
		n = p->le.numMin;
	else if(n > p->le.numMax)
		n = p->le.numMax;
	
	return n;
}

/** すべて選択 (フォーカスがあることが前提) */

void mLineEditSelectAll(mLineEdit *p)
{
	if((p->wg.fState & MWIDGET_STATE_FOCUSED)
			&& mEditTextBuf_selectAll(p->le.buf))
		mWidgetUpdate(M_WIDGET(p));
}

/** カーソルを右端へ移動 */

void mLineEditCursorToRight(mLineEdit *p)
{
	mEditTextBuf_cursorToBottom(p->le.buf);

	_changeCurPos(p, TRUE);

	mWidgetUpdate(M_WIDGET(p));
}


//========================
// イベント
//========================


/** キー押し時 */

static void _event_key(mLineEdit *p,mEvent *ev)
{
	int ret;

	if(p->le.fpress) return;

	//ENTER

	if(ev->key.code == MKEY_ENTER)
	{
		mWidgetAppendEvent_notify(NULL, M_WIDGET(p), MLINEEDIT_N_ENTER, 0, 0);
		return;
	}
	
	//処理
	
	ret = mEditTextBuf_keyProc(p->le.buf, ev->key.code, ev->key.state,
		!(p->le.style & MLINEEDIT_S_READONLY));

	//
	
	if(ret == MEDITTEXTBUF_KEYPROC_TEXT)
		_changeText(p);
	else if(ret == MEDITTEXTBUF_KEYPROC_CURSOR)
		_changeCurPos(p, FALSE);

	//更新
	
	if(ret)
		mWidgetUpdate(M_WIDGET(p));
}

/** 左ボタン押し */

static void _event_btt_left_press(mLineEdit *p,mEvent *ev)
{
	int pos,y;

	if((p->le.style & MLINEEDIT_S_SPIN)
		&& ev->pt.x > _getTextRight(p) + 1)
	{
		//スピン部分
		
		y = ev->pt.y;

		if(!(p->le.style & MLINEEDIT_S_READONLY)
			&& ev->pt.x < p->wg.w - 1
			&& y >= 1 && y < p->wg.h - 1)
		{
			y--;
		
			if(y < (p->wg.h - 2) / 2)
				p->le.fpress = _PRESS_F_SPINUP;
			else
				p->le.fpress = _PRESS_F_SPINDOWN;
			
			_spinUpDown(p, (p->le.fpress == _PRESS_F_SPINUP));
			
			mWidgetSetFocus_update(M_WIDGET(p), TRUE);
			mWidgetGrabPointer(M_WIDGET(p));
			
			mWidgetTimerAdd(M_WIDGET(p), _TIMERID_SPIN_START, 500, 0);
		}
	}
	else
	{
		//テキスト部分
		
		pos = _getPtToTextPos(p, ev->pt.x);
		
		mEditTextBuf_moveCursorPos(p->le.buf,
			pos, ev->pt.state & M_MODS_SHIFT);
		
		_changeCurPos(p, FALSE);

		p->le.fpress = _PRESS_F_DRAG;
		p->le.posLast = pos;
		
		mWidgetSetFocus_update(M_WIDGET(p), TRUE);
		mWidgetGrabPointer(M_WIDGET(p));
	}
}

/** イベント */

int mLineEditEventHandle(mWidget *wg,mEvent *ev)
{
	mLineEdit *p = M_LINEEDIT(wg);
	int pos;

	switch(ev->type)
	{
		case MEVENT_POINTER:
			if(ev->pt.type == MEVENT_POINTER_TYPE_MOTION)
			{
				//移動 (ドラッグで選択範囲拡張)
				
				if(p->le.fpress == _PRESS_F_DRAG)
				{
					pos = _getPtToTextPos(p, ev->pt.x);
				
					if(pos != p->le.posLast)
					{
						mEditTextBuf_dragExpandSelect(p->le.buf, pos);
						
						p->le.posLast = pos;
						
						_changeCurPos(p, FALSE);
						
						mWidgetUpdate(wg);
					}
				}
			}
			else if(ev->pt.type == MEVENT_POINTER_TYPE_PRESS
				|| ev->pt.type == MEVENT_POINTER_TYPE_DBLCLK)
			{
				//押し
			
				if(!p->le.fpress && ev->pt.btt == M_BTT_LEFT)
					_event_btt_left_press(p, ev);
			}
			else if(ev->pt.type == MEVENT_POINTER_TYPE_RELEASE)
			{
				//離し
			
				if(p->le.fpress && ev->pt.btt == M_BTT_LEFT)
				{
					if(p->le.fpress == _PRESS_F_SPINUP
						|| p->le.fpress == _PRESS_F_SPINDOWN)
					{
						mWidgetUpdate(wg);
						mWidgetTimerDeleteAll(wg);
					}
				
					p->le.fpress = 0;
					
					mWidgetUngrabPointer(wg);
				}
			}
			break;

		//キー
		case MEVENT_KEYDOWN:
			_event_key(p, ev);
			break;
		//文字
		case MEVENT_CHAR:
		case MEVENT_STRING:
			if(p->le.fpress || (p->le.style & MLINEEDIT_S_READONLY)) break;

			if(mEditTextBuf_insertText(p->le.buf,
				(ev->type == MEVENT_CHAR)? NULL: (char *)ev->data,
				(ev->type == MEVENT_CHAR)? ev->ch.ch: 0))
			{
				_changeText(p);
			
				mWidgetUpdate(wg);
			}
			break;

		//ホイール
		case MEVENT_SCROLL:
			if((p->le.style & MLINEEDIT_S_SPIN)
				&& (ev->scr.dir == MEVENT_SCROLL_DIR_UP || ev->scr.dir == MEVENT_SCROLL_DIR_DOWN))
				_spinUpDown(p, (ev->scr.dir == MEVENT_SCROLL_DIR_UP));
			break;
		
		//タイマー
		case MEVENT_TIMER:
			if(ev->timer.id == _TIMERID_SPIN_START)
			{
				//スピン開始
				mWidgetTimerDelete(wg, _TIMERID_SPIN_START);
				mWidgetTimerAdd(wg, _TIMERID_SPIN_REPEAT, 120, 0);
			}
			else
				//リピート
				_spinUpDown(p, (p->le.fpress == _PRESS_F_SPINUP));
			break;
		
		/* [IN]タブでの移動時は全選択 [OUT]選択解除 */
		case MEVENT_FOCUS:
			if(ev->focus.bOut)
			{
				p->le.buf->selTop = -1;
				
				if(p->le.fpress)
				{
					p->le.fpress = 0;
					mWidgetUngrabPointer(wg);
				}
			}
			else
			{
				if(ev->focus.by == MEVENT_FOCUS_BY_TABMOVE)
					mEditTextBuf_selectAll(p->le.buf);
			}
		
			mWidgetUpdate(wg);
			break;
		default:
			return FALSE;
	}

	return TRUE;
}


//====================
// 描画
//====================


/** スピン描画 */

static void _draw_spin(mLineEdit *p,mPixbuf *pixbuf)
{
	int x,x2;
	uint32_t colDown,colFg;
	
	x = p->wg.w - 1 - _SPIN_W;
	x2 = x + _SPIN_W / 2;

	colFg = MSYSCOL(TEXT_REVERSE);
	colDown = MSYSCOL(TEXT);
	
	//背景

	mPixbufFillBox(pixbuf, x, 1, _SPIN_W, p->wg.h - 2, MSYSCOL(FACE_DARKER));
	
	//上
	
	mPixbufDrawArrowUp(pixbuf, x2, 5,
		(p->le.fpress == _PRESS_F_SPINUP)? colDown: colFg);

	//下
	
	mPixbufDrawArrowDown(pixbuf, x2, p->wg.h - 6,
		(p->le.fpress == _PRESS_F_SPINDOWN)? colDown: colFg);
}

/** テキスト描画 */

static void _draw_text(mLineEdit *p,mPixbuf *pixbuf)
{
	mFont *font;
	mEditTextBuf *tbuf;
	uint32_t *ptext,*ptext_top,col;
	int tpos,right,left,bsel,w,len;
	
	tbuf = p->le.buf;
	
	tpos = 0;
	ptext = tbuf->text;
	ptext_top = ptext;
	
	right = _getTextRight(p);
	left  = p->le.padding - p->le.scrX;

	bsel = mEditTextBuf_isSelectAtPos(tbuf, tpos);
	col = (p->wg.fState & MWIDGET_STATE_ENABLED)?
			MSYSCOL_RGB(TEXT): MSYSCOL_RGB(TEXT_DISABLE);
	
	font = mWidgetGetFont(M_WIDGET(p));

	//クリッピング
	
	mPixbufSetClipBox_d(pixbuf,
		p->le.padding, 0, right - p->le.padding, p->wg.h);

	//-------- テキスト
	
	for(; *ptext; ptext++, tpos++)
	{
		if((!bsel && tpos == tbuf->selTop) ||
			(bsel && tpos == tbuf->selEnd))
		{
			//選択と通常文字の境
			
			len = ptext - ptext_top;
			
			w = mFontGetTextWidthUCS4(font, ptext_top, len);
			
			if(bsel)
			{
				mPixbufFillBox(pixbuf,
					left, p->le.padding, w, font->height, MSYSCOL(FACE_SELECT));
			}
			
			mFontDrawTextUCS4(font, pixbuf,
				left, p->le.padding, ptext_top, len,
				(bsel)? MSYSCOL_RGB(TEXT_SELECT): col);
			
			ptext_top = ptext;
			left += w;
			bsel ^= 1;
		}
	}
	
	//残り
	
	if(left < right)
	{
		len = ptext - ptext_top;
		
		if(bsel)
		{
			w = mFontGetTextWidthUCS4(font, ptext_top, len);

			mPixbufFillBox(pixbuf,
				left, p->le.padding, w, font->height, MSYSCOL(FACE_SELECT));
		}
		
		mFontDrawTextUCS4(font, pixbuf,
			left, p->le.padding, ptext_top, len,
			(bsel)? MSYSCOL_RGB(TEXT_SELECT): col);
	}

	//--------- カーソル

	if(p->wg.fState & MWIDGET_STATE_FOCUSED)
	{
		left = p->le.curX - p->le.scrX;
		
		if(left >= 0 && left < right - p->le.padding)
		{
			mPixbufLineV(pixbuf, p->le.padding + left, p->le.padding,
				p->wg.h - p->le.padding * 2,
				MPIXBUF_COL_XOR);
		}
	}
}

/** 描画 */

void mLineEditDrawHandle(mWidget *wg,mPixbuf *pixbuf)
{
	mLineEdit *p = M_LINEEDIT(wg);
	int frame;
	uint32_t col;
	
	frame = !(p->le.style & MLINEEDIT_S_NO_FRAME);
	
	//枠

	if(frame)
	{
		if(!(wg->fState & MWIDGET_STATE_ENABLED))
			col = MSYSCOL(FRAME_LIGHT);
		else if(wg->fState & MWIDGET_STATE_FOCUSED)
			col = MSYSCOL(FRAME_FOCUS);
		else
			col = MSYSCOL(FRAME_DARK);
	
		mPixbufBox(pixbuf, 0, 0, wg->w, wg->h, col);
	}
	
	//背景
	
	mPixbufFillBox(pixbuf, frame, frame,
		wg->w - frame * 2, wg->h - frame * 2,
		((wg->fState & MWIDGET_STATE_ENABLED) && !(p->le.style & MLINEEDIT_S_READONLY))
			? MSYSCOL(FACE_LIGHTEST): MSYSCOL(FACE));
	
	//スピン
	
	if(p->le.style & MLINEEDIT_S_SPIN)
		_draw_spin(p, pixbuf);
	
	//テキスト・カーソル
	
	_draw_text(p, pixbuf);
}

/** @} */
