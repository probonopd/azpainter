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
 * mListHeader : リストのヘッダ部分
 *****************************************/

#include "mDef.h"
#include "mWidget.h"
#include "mPixbuf.h"
#include "mFont.h"
#include "mEvent.h"
#include "mSysCol.h"
#include "mList.h"
#include "mCursor.h"

#include "mListHeader.h"


//----------------------

#define _ITEM_TOP(p)  ((mListHeaderItem *)p->lh.list.top)

#define _PADDING_H  2

#define _PRESSF_RESIZE  1
#define _PRESSF_CLICK   2

static void _onsize_handle(mWidget *wg);
static void _draw_handle(mWidget *p,mPixbuf *pixbuf);

//----------------------


/******************//**

@defgroup listheader mListHeader
@brief リストビューのヘッダ部分

- 幅拡張のアイテムは最初の一つのみ有効。

<h3>継承</h3>
mWidget \> mListHeader

@ingroup group_widget

@{

@file mListHeader.h
@def M_LISTHEADER(p)
@def M_LISTHEADER_ITEM(p)
@struct mListHeader
@struct mListHeaderData
@struct mListHeaderItem
@enum MLISTHEADER_STYLE
@enum MLISTHEADER_NOTIFY
@enum MLISTHEADER_ITEM_FLAGS

@var MLISTHEADER_STYLE::MLISTHEADER_STYLE_SORT
ヘッダに矢印を付けて、クリックでソートのタイプを変更できるようにする。

@var MLISTHEADER_NOTIFY::MLISTHEADER_N_CHANGE_WIDTH
項目の幅サイズが変わった時 (ドラッグ中も)\n
param1 : アイテムのポインタ。\n
param2 : 幅

@var MLISTHEADER_NOTIFY::MLISTHEADER_N_CHANGE_SORT
ソートの情報が変わった時。\n
param1 : アイテムのポインタ。\n
param2 : 1 で昇順、0 で降順

***********************/


//===========================


/** アイテム破棄ハンドラ */

static void _destroy_headeritem(mListItem *p)
{
	mFree(M_LISTHEADER_ITEM(p)->text);
}

/** カーソル位置からアイテム取得 */

static mListHeaderItem *_getItemByPos(mListHeader *p,int x,int *topx)
{
	mListHeaderItem *pi;
	int xx;

	xx = -(p->lh.scrx);

	for(pi = _ITEM_TOP(p); pi; pi = M_LISTHEADER_ITEM(pi->i.next))
	{
		if(xx <= x && x < xx + pi->width)
		{
			if(topx) *topx = xx;
			return pi;
		}

		xx += pi->width;
	}

	return NULL;
}

/** アイテムの幅変更 */

static void _change_item_width(mListHeader *p,mListHeaderItem *pi,int w)
{
	if(w < 4) w = 4;

	if(pi->width != w)
	{
		pi->width = w;

		mWidgetAppendEvent_notify(NULL, M_WIDGET(p),
			MLISTHEADER_N_CHANGE_WIDTH, (intptr_t)pi, w);

		mWidgetUpdate(M_WIDGET(p));
	}
}


//===========================


/** 解放処理 */

void mListHeaderDestroyHandle(mWidget *p)
{
	mListDeleteAll(&M_LISTHEADER(p)->lh.list);
}

/** サイズ計算 */

void mListHeaderCalcHintHandle(mWidget *p)
{
	p->hintH = mWidgetGetFontHeight(p) + _PADDING_H * 2;
}

/** 作成 */

mListHeader *mListHeaderNew(int size,mWidget *parent,uint32_t style)
{
	mListHeader *p;
	
	if(size < sizeof(mListHeader)) size = sizeof(mListHeader);
	
	p = (mListHeader *)mWidgetNew(size, parent);
	if(!p) return NULL;
	
	p->wg.destroy = mListHeaderDestroyHandle;
	p->wg.calcHint = mListHeaderCalcHintHandle;
	p->wg.event = mListHeaderEventHandle;
	p->wg.draw = _draw_handle;
	p->wg.onSize = _onsize_handle;
	p->wg.fEventFilter |= MWIDGET_EVENTFILTER_POINTER;

	p->lh.style = style;
	
	return p;
}

/** 先頭アイテム取得 */

mListHeaderItem *mListHeaderGetTopItem(mListHeader *p)
{
	return M_LISTHEADER_ITEM(p->lh.list.top);
}

/** アイテム追加 */

void mListHeaderAddItem(mListHeader *p,
	const char *text,int width,uint32_t flags,intptr_t param)
{
	mListHeaderItem *pi;

	pi = (mListHeaderItem *)mListAppendNew(&p->lh.list,
			sizeof(mListHeaderItem), _destroy_headeritem);

	if(pi)
	{
		pi->text = mStrdup(text);
		pi->width = width;
		pi->flags = flags;
		pi->param = param;
	}
}

/** すべてのアイテムの幅取得 */

int mListHeaderGetFullWidth(mListHeader *p)
{
	mListHeaderItem *pi;
	int w = 0;

	for(pi = _ITEM_TOP(p); pi; pi = M_LISTHEADER_ITEM(pi->i.next))
		w += pi->width;

	return w;
}

/** 水平スクロール位置セット */

void mListHeaderSetScrollPos(mListHeader *p,int scrx)
{
	if(p->lh.scrx != scrx)
	{
		p->lh.scrx = scrx;
		mWidgetUpdate(M_WIDGET(p));
	}
}

/** ソートのパラメータを設定 */

void mListHeaderSetSortParam(mListHeader *p,int index,mBool up)
{
	p->lh.sortitem = (mListHeaderItem *)mListGetItemByIndex(&p->lh.list, index);
	p->lh.sortup = (up != 0);

	mWidgetUpdate(M_WIDGET(p));
}


//========================
// ハンドラ
//========================


/** ボタン押し時 */

static void _event_press(mListHeader *p,mEvent *ev)
{
	mListHeaderItem *pi;
	int x;

	pi = _getItemByPos(p, ev->pt.x - 4, &x);

	if(pi)
	{
		if(!(pi->flags & MLISTHEADER_ITEM_F_FIX)
			&& ev->pt.x > x + pi->width - 4)
		{
			//境界上ドラッグでリサイズ
			
			p->lh.fpress = _PRESSF_RESIZE;
			p->lh.dragitem = pi;
			p->lh.dragLeft = x;

			mWidgetGrabPointer(M_WIDGET(p));
		}
		else if(p->lh.style & MLISTHEADER_STYLE_SORT)
		{
			//ソートありの場合、クリックでソート項目と昇降順変更

			if(p->lh.sortitem == pi)
				p->lh.sortup ^= 1;
			else
				p->lh.sortitem = pi;

			p->lh.fpress = _PRESSF_CLICK;

			mWidgetGrabPointer(M_WIDGET(p));
			mWidgetUpdate(M_WIDGET(p));
		}
	}
}

/** カーソル移動時 */

static void _event_motion(mListHeader *p,mEvent *ev)
{
	mListHeaderItem *pi;
	mCursor cur;
	int x;

	switch(p->lh.fpress)
	{
		//ボタンが押されていない時
		//(境界の上に乗ったらカーソル変更)
		case 0:
			pi = _getItemByPos(p, ev->pt.x - 4, &x);

			if(pi && !(pi->flags & MLISTHEADER_ITEM_F_FIX)
				&& ev->pt.x > x + pi->width - 4)
				cur = mCursorGetDefault(MCURSOR_DEF_HSPLIT);
			else
				cur = 0;

			mWidgetSetCursor(M_WIDGET(p), cur);
			break;

		//リサイズ中
		case _PRESSF_RESIZE:
			_change_item_width(p, p->lh.dragitem, ev->pt.x - p->lh.dragLeft);
			break;

	}
}

/** グラブ解放 */

static void _release_grab(mListHeader *p)
{
	if(p->lh.fpress)
	{
		if(p->lh.fpress == _PRESSF_CLICK)
		{
			//ソート変更通知
			
			mWidgetAppendEvent_notify(NULL, M_WIDGET(p),
				MLISTHEADER_N_CHANGE_SORT, (intptr_t)p->lh.sortitem, p->lh.sortup);

			mWidgetUpdate(M_WIDGET(p));
		}
	
		p->lh.fpress = 0;
		mWidgetUngrabPointer(M_WIDGET(p));
	}
}

/** イベント */

int mListHeaderEventHandle(mWidget *wg,mEvent *ev)
{
	mListHeader *p = M_LISTHEADER(wg);

	switch(ev->type)
	{
		case MEVENT_POINTER:
			if(ev->pt.type == MEVENT_POINTER_TYPE_MOTION)
				_event_motion(p, ev);
			else if(ev->pt.type == MEVENT_POINTER_TYPE_PRESS
				|| ev->pt.type == MEVENT_POINTER_TYPE_DBLCLK)
			{
				if(ev->pt.btt == M_BTT_LEFT && !p->lh.fpress)
					_event_press(p, ev);
			}
			else if(ev->pt.type == MEVENT_POINTER_TYPE_RELEASE)
			{
				if(ev->pt.btt == M_BTT_LEFT)
					_release_grab(p);
			}
			break;
		case MEVENT_LEAVE:
			mWidgetSetCursor(wg, 0);
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

/** サイズ変更時 */

void _onsize_handle(mWidget *wg)
{
	mListHeader *p = M_LISTHEADER(wg);
	mListHeaderItem *pi,*pi_expand = NULL;
	int w = 0;

	//EXPAND アイテムと、EXPAND 以外のすべての幅取得

	for(pi = _ITEM_TOP(p); pi; pi = M_LISTHEADER_ITEM(pi->i.next))
	{
		if(!pi_expand && (pi->flags & MLISTHEADER_ITEM_F_EXPAND))
			pi_expand = pi;
		else
			w += pi->width;
	}

	//EXPAND アイテムの幅セット

	if(pi_expand)
		_change_item_width(p, pi_expand, wg->w - w);
}

/** 描画 */

void _draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	mListHeader *p = M_LISTHEADER(wg);
	mFont *font;
	mListHeaderItem *pi;
	int x,w,wh,n;
	mBool have_arrow;
	
	font = mWidgetGetFont(wg);
	wh = wg->h;
	have_arrow = ((p->lh.style & MLISTHEADER_STYLE_SORT) != 0);

	//背景

	mPixbufFillBox(pixbuf, 0, 0, wg->w, wh - 1, MSYSCOL(FACE_DARK));

	//下枠線

	mPixbufLineH(pixbuf, 0, wh - 1, wg->w, MSYSCOL(FRAME));

	//項目

	x = -(p->lh.scrx);

	for(pi = _ITEM_TOP(p); pi; x += pi->width, pi = M_LISTHEADER_ITEM(pi->i.next))
	{
		w = pi->width;
	
		if(x + w <= 0) continue;
		if(x >= wg->w) break;

		//クリック時の背景

		if(p->lh.fpress == _PRESSF_CLICK && p->lh.sortitem == pi)
			mPixbufFillBox(pixbuf, x, 0, w - 1, wh - 1, MSYSCOL(FACE_DARKER));

		//右境界線

		mPixbufLineV(pixbuf, x + w - 1, 0, wh, MSYSCOL(FRAME));

		//----

		mPixbufSetClipBox_d(pixbuf, x + 2, 0, w - 4, wh);

		//テキスト

		n = x + 2;

		if(pi->flags & MLISTHEADER_ITEM_F_RIGHT)
		{
			n += w - 4 - mFontGetTextWidth(font, pi->text, -1);

			if(have_arrow && pi == p->lh.sortitem) n -= 8;
		}

		mFontDrawText(font, pixbuf, n, _PADDING_H, pi->text, -1, MSYSCOL_RGB(TEXT));

		//矢印

		if(have_arrow && pi == p->lh.sortitem)
		{
			n = x + w - 6;
		
			if(p->lh.sortup)
				mPixbufDrawArrowUp(pixbuf, n, wh >> 1, MSYSCOL(TEXT));
			else
				mPixbufDrawArrowDown(pixbuf, n, wh >> 1, MSYSCOL(TEXT));
		}

		mPixbufClipNone(pixbuf);
	}
}

/** @} */
