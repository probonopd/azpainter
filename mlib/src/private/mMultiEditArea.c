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
 * mMultiEditArea
 *****************************************/

#include "mDef.h"
#include "mWidget.h"
#include "mScrollBar.h"
#include "mPixbuf.h"
#include "mFont.h"
#include "mEvent.h"
#include "mSysCol.h"

#include "mMultiEdit.h"
#include "mMultiEditArea.h"
#include "mEditTextBuf.h"


//--------------------------

#define _PADDING_X  2
#define _PADDING_Y  1

#define _IS_STYLE(p,f)  (M_MULTIEDIT((p)->wg.parent)->me.style & (f))

static int _event_handle(mWidget *wg,mEvent *ev);
static void _draw_handle(mWidget *wg,mPixbuf *pixbuf);

//--------------------------


//========================
// sub
//========================


/** フォント、行の高さを取得 */

static int _get_line_height(mMultiEditArea *p)
{
	mFont *font = mWidgetGetFont(M_WIDGET(p));

	return font->lineheight;
}

/** ポインタイベント時、文字位置を取得 */

static int _getcharpos_ptevent(mMultiEditArea *p,mEvent *ev)
{
	return mEditTextBuf_getPosAtPixel_forMulti(p->mea.buf,
		ev->pt.x - _PADDING_X + mScrollViewAreaGetHorzScrollPos(M_SCROLLVIEWAREA(p)),
		ev->pt.y - _PADDING_Y + mScrollViewAreaGetVertScrollPos(M_SCROLLVIEWAREA(p)),
		_get_line_height(p));
}

/** スクロール調整 */

static void _adjustScroll(mMultiEditArea *p)
{
	int scrx,scry,curx,cury,w,h,lineh;
	mEditTextBuf *buf;

	buf = p->mea.buf;
	lineh = _get_line_height(p);

	scrx = mScrollViewAreaGetHorzScrollPos(M_SCROLLVIEWAREA(p));
	scry = mScrollViewAreaGetVertScrollPos(M_SCROLLVIEWAREA(p));
	curx = buf->multi_curx;
	cury = buf->multi_curline * lineh;
	w = p->wg.w - _PADDING_X * 2;
	h = p->wg.h - _PADDING_Y * 2 - lineh;

	if(scrx >= buf->maxwidth)
		scrx = 0;
	else if(curx - scrx < 0)
		scrx = curx;
	else if(curx - scrx >= w)
		scrx = curx - w + 1;

	if(scry >= buf->maxlines * lineh)
		scry = 0;
	else if(cury - scry < 0)
		scry = cury;
	else if(cury - scry >= h)
		scry = cury - h;

	mScrollBarSetPos(mScrollViewAreaGetScrollBar(M_SCROLLVIEWAREA(p), FALSE), scrx);
	mScrollBarSetPos(mScrollViewAreaGetScrollBar(M_SCROLLVIEWAREA(p), TRUE), scry);
}

/** 入力などによるテキスト変更時 */

static void _onChangeText(mMultiEditArea *p)
{
	mMultiEditArea_setScrollInfo(p);

	mMultiEditArea_onChangeCursorPos(p);
	
	//スクロールバー再構成

	mWidgetAppendEvent_only(p->wg.parent, MEVENT_CONSTRUCT);

	//CHANGE 通知

	if(_IS_STYLE(p, MMULTIEDIT_S_NOTIFY_CHANGE))
	{
		mWidgetAppendEvent_notify(NULL,
			M_WIDGET(p->wg.parent), MMULTIEDIT_N_CHANGE, 0, 0);
	}
}


//========================
// main
//========================


/** スクロールバー表示するかのハンドラ */

static mBool _is_bar_visible(mScrollViewArea *area,int size,mBool horz)
{
	mMultiEditArea *p = M_MULTIEDITAREA(area);

	if(horz)
		return (size < p->mea.buf->maxwidth + _PADDING_X * 2);
	else
		return (size < p->mea.buf->maxlines * _get_line_height(p) + _PADDING_Y * 2);
}

/** サイズ変更時 */

static void _onsize_handle(mWidget *wg)
{
	mMultiEditArea_setScrollInfo(M_MULTIEDITAREA(wg));
}

/** 作成 */

mMultiEditArea *mMultiEditAreaNew(int size,mWidget *parent,mEditTextBuf *textbuf)
{
	mMultiEditArea *p;
	
	if(size < sizeof(mMultiEditArea)) size = sizeof(mMultiEditArea);
	
	p = (mMultiEditArea *)mScrollViewAreaNew(size, parent);
	if(!p) return NULL;
	
	p->wg.draw = _draw_handle;
	p->wg.event = _event_handle;
	p->wg.onSize = _onsize_handle;
	p->wg.fEventFilter |= MWIDGET_EVENTFILTER_POINTER | MWIDGET_EVENTFILTER_SCROLL;

	p->sva.isBarVisible = _is_bar_visible;

	p->mea.buf = textbuf;
	
	return p;
}

/** スクロール情報セット */

void mMultiEditArea_setScrollInfo(mMultiEditArea *p)
{
	mScrollBar *horz,*vert;

	horz = mScrollViewAreaGetScrollBar(M_SCROLLVIEWAREA(p), FALSE);
	vert = mScrollViewAreaGetScrollBar(M_SCROLLVIEWAREA(p), TRUE);

	//+1 はカーソルの分
	mScrollBarSetStatus(horz, 0, p->mea.buf->maxwidth + 1, p->wg.w - _PADDING_X * 2);

	mScrollBarSetStatus(vert, 0,
		p->mea.buf->maxlines * _get_line_height(p), p->wg.h - _PADDING_Y * 2);
}

/** カーソル位置変更時 */

void mMultiEditArea_onChangeCursorPos(mMultiEditArea *p)
{
	mWidget *lv = p->wg.parent;
	mPoint pt;

	//スクロール調整

	_adjustScroll(p);

	//IM 位置 (位置は mMultiEdit 上での座標)

	pt.x = p->mea.buf->multi_curx
		- mScrollViewAreaGetHorzScrollPos(M_SCROLLVIEWAREA(p)) + _PADDING_X;

	pt.y = p->mea.buf->multi_curline * _get_line_height(p)
		- mScrollViewAreaGetVertScrollPos(M_SCROLLVIEWAREA(p)) + _PADDING_Y;

	if(pt.x < _PADDING_X) pt.x = _PADDING_X;
	else if(pt.x > p->wg.w) pt.x = p->wg.w;

	if(pt.y < _PADDING_Y) pt.y = _PADDING_Y;
	else if(pt.y > p->wg.h) pt.y = p->wg.h;

	mWidgetMapPoint(M_WIDGET(p), p->wg.parent, &pt);

	lv->imX = pt.x;
	lv->imY = pt.y;
}


//========================
// ハンドラ
//========================


/** キー押し時 */

static void _event_keydown(mMultiEditArea *p,mEvent *ev)
{
	int ret;

	//処理
	
	ret = mEditTextBuf_keyProc(p->mea.buf, ev->key.code, ev->key.state,
		!_IS_STYLE(p, MMULTIEDIT_S_READONLY));

	//
	
	if(ret == MEDITTEXTBUF_KEYPROC_TEXT)
		_onChangeText(p);
	else if(ret == MEDITTEXTBUF_KEYPROC_CURSOR)
		mMultiEditArea_onChangeCursorPos(p);

	//更新
	
	if(ret)
		mWidgetUpdate(M_WIDGET(p));
}

/** ボタン押し */

static void _event_press(mMultiEditArea *p,mEvent *ev)
{
	int pos;

	//文字位置

	pos = _getcharpos_ptevent(p, ev);

	//親にフォーカスセット

	mWidgetSetFocus(p->wg.parent);

	//カーソル位置移動 (+Shift で選択範囲拡張)

	mEditTextBuf_moveCursorPos(p->mea.buf, pos, ev->pt.state & M_MODS_SHIFT);

	//

	p->mea.fpress = TRUE;
	p->mea.drag_pos = pos;

	mWidgetGrabPointer(M_WIDGET(p));
	mWidgetUpdate(M_WIDGET(p));
}

/** ポインタ移動 */

static void _event_motion(mMultiEditArea *p,mEvent *ev)
{
	int pos;

	pos = _getcharpos_ptevent(p, ev);

	if(pos != p->mea.drag_pos)
	{
		mEditTextBuf_dragExpandSelect(p->mea.buf, pos);

		p->mea.drag_pos = pos;

		_adjustScroll(p);
		
		mWidgetUpdate(M_WIDGET(p));
	}
}

/** グラブ解放 */

static void _release_grab(mMultiEditArea *p)
{
	if(p->mea.fpress)
	{
		p->mea.fpress = 0;
		mWidgetUngrabPointer(M_WIDGET(p));

		mMultiEditArea_onChangeCursorPos(p);
	}
}

/** イベント */

int _event_handle(mWidget *wg,mEvent *ev)
{
	mMultiEditArea *p = M_MULTIEDITAREA(wg);

	switch(ev->type)
	{
		case MEVENT_KEYDOWN:
			if(!p->mea.fpress)
				_event_keydown(p, ev);
			break;

		//文字
		case MEVENT_CHAR:
		case MEVENT_STRING:
			if(!p->mea.fpress && !_IS_STYLE(p, MMULTIEDIT_S_READONLY))
			{
				if(mEditTextBuf_insertText(p->mea.buf,
					(ev->type == MEVENT_CHAR)? NULL: (char *)ev->data,
					(ev->type == MEVENT_CHAR)? ev->ch.ch: 0))
				{
					_onChangeText(p);
				
					mWidgetUpdate(wg);
				}
			}
			break;

		case MEVENT_POINTER:
			if(ev->pt.type == MEVENT_POINTER_TYPE_MOTION)
			{
				if(p->mea.fpress)
					_event_motion(p, ev);
			}
			else if(ev->pt.type == MEVENT_POINTER_TYPE_PRESS)
			{
				if(!p->mea.fpress && ev->pt.btt == M_BTT_LEFT)
					_event_press(p, ev);
			}
			else if(ev->pt.type == MEVENT_POINTER_TYPE_RELEASE)
			{
				if(ev->pt.btt == M_BTT_LEFT)
					_release_grab(p);
			}
			break;

		//通知
		case MEVENT_NOTIFY:
			if((ev->notify.type == MSCROLLVIEWAREA_N_SCROLL_HORZ
				|| ev->notify.type == MSCROLLVIEWAREA_N_SCROLL_VERT)
				&& (ev->notify.param2 & MSCROLLBAR_N_HANDLE_F_CHANGE))
			{
				//スクロールバー位置変更

				mWidgetUpdate(wg);
			}
			break;

		//ホイール (上下で垂直スクロール)
		case MEVENT_SCROLL:
			if(!p->mea.fpress)
			{
				if( mScrollBarMovePos(mScrollViewAreaGetScrollBar(M_SCROLLVIEWAREA(p), TRUE),
					((ev->scr.dir == MEVENT_SCROLL_DIR_UP)? -3: 3) * _get_line_height(p)) )
					mWidgetUpdate(wg);
			}
			break;

		case MEVENT_FOCUS:
			if(ev->focus.bOut)
				_release_grab(p);
			break;
		
		default:
			return FALSE;
	}

	return TRUE;
}

/** 描画 */

void _draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	mMultiEditArea *p = M_MULTIEDITAREA(wg);
	mEditTextBuf *editbuf;
	mFont *font;
	uint32_t *pc;
	uint16_t *pw;
	int x,y,xtop,lineh,areaw,areah,pos,cw,scrx,scry;
	mBool bsel;

	editbuf = p->mea.buf;
	
	font = mWidgetGetFont(wg);
	lineh = font->lineheight;

	pc = editbuf->text;
	pw = editbuf->widthbuf;

	scrx = mScrollViewAreaGetHorzScrollPos(M_SCROLLVIEWAREA(p));
	scry = mScrollViewAreaGetVertScrollPos(M_SCROLLVIEWAREA(p));

	areaw = wg->w - _PADDING_X * 2;
	areah = wg->h - _PADDING_Y * 2;

	//背景

	mPixbufFillBox(pixbuf, 0, 0, wg->w, wg->h,
		((wg->fState & MWIDGET_STATE_ENABLED)
			&& !(_IS_STYLE(p, MMULTIEDIT_S_READONLY)))? MSYSCOL(FACE_LIGHTEST): MSYSCOL(FACE));

	//テキスト領域クリッピング

	if(!mPixbufSetClipBox_d(pixbuf, _PADDING_X, _PADDING_Y, areaw, areah))
		return;

	//テキスト描画

	xtop = x = -scrx;
	y = -scry;

	for(pos = 0; *pc && y < areah; )
	{
		if(y <= -lineh)
		{
			//----- Y がスクロールで隠れている場合、次の行へ

			for(; *pc && *pc != '\n'; pc++, pw++, pos++);
			
			y += lineh;
		}
		else if(*pc == '\n')
		{
			//------ 改行

			//選択範囲内の場合は、わかりやすいように 4px の幅を付加

			if(mEditTextBuf_isSelectAtPos(editbuf, pos))
			{
				mPixbufFillBox(pixbuf,
					x + _PADDING_X, y + _PADDING_Y, 4, lineh, MSYSCOL(FACE_SELECT));
			}

			x = xtop;
			y += lineh;
		}
		else if(x >= areaw)
		{
			//----- X が表示領域を超えた場合、次の行へ

			for(; *pc && *pc != '\n'; pc++, pw++, pos++);

			x = xtop;
			y += lineh;
		}
		else
		{
			//----- 文字

			cw = *pw;

			if(-cw < x)
			{
				bsel = mEditTextBuf_isSelectAtPos(editbuf, pos);

				//選択範囲背景

				if(bsel)
				{
					mPixbufFillBox(pixbuf,
						x + _PADDING_X, y + _PADDING_Y, cw, lineh, MSYSCOL(FACE_SELECT));
				}

				//文字
					 
				mFontDrawTextUCS4(font, pixbuf, x + _PADDING_X, y + _PADDING_Y,
					pc, 1, (bsel)? MSYSCOL_RGB(TEXT_SELECT): MSYSCOL_RGB(TEXT));
			}

			x += cw;
		}

		if(!(*pc)) break;

		pc++, pw++, pos++;
	}

	//カーソル

	if(wg->parent->fState & MWIDGET_STATE_FOCUSED)
	{
		mPixbufLineV(pixbuf,
			editbuf->multi_curx - scrx + _PADDING_X,
			editbuf->multi_curline * lineh - scry + _PADDING_Y,
			lineh, MPIXBUF_COL_XOR);
	}
}

/* @} */
