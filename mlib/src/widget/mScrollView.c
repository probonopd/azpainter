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

/*************************************************
 * mScrollView/mScrollViewArea : スクロールビュー
 *************************************************/

#include "mDef.h"

#include "mScrollView.h"
#include "mScrollViewArea.h"

#include "mWidget.h"
#include "mScrollBar.h"
#include "mPixbuf.h"
#include "mEvent.h"
#include "mSysCol.h"
#include "mWidget_pv.h"


//------------------------

static void _onsize_handle(mWidget *);
static void _layout_handle(mWidget *wg);
static void _draw_handle(mWidget *wg,mPixbuf *pixbuf);
static mBool _showScrollBar(mScrollView *p);

//------------------------


/***************//**

@defgroup scrollview mScrollView
@brief スクロールビュー

表示エリアは mScrollViewArea の派生として作成し、mScrollView の子にする。\n
mScrollViewData::area に表示エリアウィジェットのポインタをセットする。\n
mScrollViewArea の onSize() ハンドラ内でスクロールのステータスを変更すること。

<h3>継承</h3>
mWidget \> mScrollView

@ingroup group_widget
@{

@file mScrollView.h
@def M_SCROLLVIEW(p)
@struct mScrollView
@struct mScrollViewData
@enum MSCROLLVIEW_STYLE

@var MSCROLLVIEW_STYLE::MSCROLLVIEW_S_SCROLL_NOTIFY_SELF
スクロールバーの通知を mScrollViewArea に送らずに、自身へ送る

*********************/


//=============================


/** レイアウト左上位置とサイズ取得 */

static void _getLayoutBox(mScrollView *p,mBox *box)
{
	if(p->sv.style & MSCROLLVIEW_S_FRAME)
	{
		box->x = box->y = 1;
		box->w = p->wg.w - 2;
		box->h = p->wg.h - 2;
	}
	else
	{
		box->x = box->y = 0;
		box->w = p->wg.w;
		box->h = p->wg.h;
	}
}

/** スクロールバー操作時
 * 
 * area にイベントを送る */

static void _scrollbar_handle(mScrollBar *scrb,int pos,int type)
{
	mScrollView *p = M_SCROLLVIEW(scrb->wg.param);

	mWidgetAppendEvent_notify(M_WIDGET(p->sv.area), M_WIDGET(p),
		(scrb->sb.style & MSCROLLBAR_S_VERT)?
			MSCROLLVIEWAREA_N_SCROLL_VERT: MSCROLLVIEWAREA_N_SCROLL_HORZ,
		pos, type);
}

/** スクロールバー作成
 *
 * wg.param に mScrollView のポインタ */

static mScrollBar *_create_scrollbar(mScrollView *p,int vert)
{
	mScrollBar *scr;

	scr = mScrollBarNew(0, M_WIDGET(p), (vert)? MSCROLLBAR_S_VERT: MSCROLLBAR_S_HORZ);

	scr->wg.notifyTarget = MWIDGET_NOTIFYTARGET_PARENT;
	scr->wg.param = (intptr_t)p;
	
	if(!(p->sv.style & MSCROLLVIEW_S_SCROLL_NOTIFY_SELF))
		scr->sb.handle = _scrollbar_handle;
	
	return scr;
}


//=============================
// メイン
//=============================


/** 作成 */

mScrollView *mScrollViewNew(int size,mWidget *parent,uint32_t style)
{
	mScrollView *p;
	
	if(size < sizeof(mScrollView)) size = sizeof(mScrollView);
	
	p = (mScrollView *)mWidgetNew(size, parent);
	if(!p) return NULL;
	
	p->wg.draw = _draw_handle;
	p->wg.layout = _layout_handle;
	p->wg.onSize = _onsize_handle;
	
	p->wg.hintW = (style & MSCROLLVIEW_S_VERT)? MSCROLLBAR_WIDTH: 1;
	p->wg.hintH = (style & MSCROLLVIEW_S_HORZ)? MSCROLLBAR_WIDTH: 1;
	
	p->sv.style = style;
	
	//スクロールバー

	if(style & MSCROLLVIEW_S_HORZ)
		p->sv.scrh = _create_scrollbar(p, FALSE);

	if(style & MSCROLLVIEW_S_VERT)
		p->sv.scrv = _create_scrollbar(p, TRUE);
	
	return p;
}

/** 構成を行う
 *
 * 中身の表示サイズが変わった時など。\n
 * スクロールバーの有無を再判定し、レイアウトし直す。 */

void mScrollViewConstruct(mScrollView *p)
{
	if(_showScrollBar(p))
		_layout_handle(M_WIDGET(p));
}


//========================
// ハンドラ
//========================


/** 現在のサイズから、スクロールバーの有無をセット
 * 
 * @return TRUE でスクロールバーの有無の変更あり */

mBool _showScrollBar(mScrollView *p)
{
	mScrollViewArea *area;
	mBox box;
	int horz,vert,flag = 0;
	
	area = p->sv.area;
	if(!area) return FALSE;
	
	_getLayoutBox(p, &box);
	
	//------ 指定サイズ時にスクロールを表示するかどうか
	/* 0 bit : 水平、スクロールバー無し時
	 * 1 bit : 水平、スクロールバーあり時
	 * 2 bit : 垂直、スクロールバー無し時
	 * 3 bit : 垂直、スクロールバーあり時 */
	
	//水平
	
	if(p->sv.style & MSCROLLVIEW_S_FIX_HORZ)
		flag |= 3;
	else
	{
		if((area->sva.isBarVisible)(area, box.w - MSCROLLBAR_WIDTH, TRUE)) flag |= 1;
		if((area->sva.isBarVisible)(area, box.w, TRUE)) flag |= 2;
	}

	//垂直
	
	if(p->sv.style & MSCROLLVIEW_S_FIX_VERT)
		flag |= 4 | 8;
	else
	{
		if((area->sva.isBarVisible)(area, box.h - MSCROLLBAR_WIDTH, FALSE)) flag |= 4;
		if((area->sva.isBarVisible)(area, box.h, FALSE)) flag |= 8;
	}
	
	//--------- 判定
	
	horz = vert = 0;
	
	if(p->sv.scrh && !p->sv.scrv)
		//水平のみ
		horz = ((flag & 2) != 0);
	else if(!p->sv.scrh && p->sv.scrv)
		//垂直のみ
		vert = ((flag & 8) != 0);
	else
	{
		//----- 両方あり
		
		//水平

		if((flag & 3) == 3)
			horz = TRUE;
		else if((flag & 3) == 0)
			horz = FALSE;
		else
		{
			if((flag & 12) == 12)
				horz = ((flag & 1) != 0);  //垂直バーあり
			else
				horz = ((flag & 2) != 0);  //垂直バーなし
		}

		//垂直

		if((flag & 12) == 12)
			vert = TRUE;
		else if((flag & 12) == 0)
			vert = FALSE;
		else
		{
			if((flag & 3) == 3)
				vert = ((flag & 4) != 0);
			else
				vert = ((flag & 8) != 0);
		}
	}
	
	//------- バーの有無切り替え
	
	flag = 0;
	
	if(p->sv.scrh && mWidgetIsVisible(M_WIDGET(p->sv.scrh)) != horz)
	{
		mWidgetShow(M_WIDGET(p->sv.scrh), horz);
		flag = 1;
	}

	if(p->sv.scrv && mWidgetIsVisible(M_WIDGET(p->sv.scrv)) != vert)
	{
		mWidgetShow(M_WIDGET(p->sv.scrv), vert);
		flag = 1;
	}
	
	return flag;
}

/** サイズ変更時 */

void _onsize_handle(mWidget *wg)
{
	_showScrollBar(M_SCROLLVIEW(wg));
}

/** レイアウト */

void _layout_handle(mWidget *wg)
{
	mScrollView *p = M_SCROLLVIEW(wg);
	mBox box;
	int edge,scrb = 0;
	
	_getLayoutBox(p, &box);
	
	if(mWidgetIsVisible(M_WIDGET(p->sv.scrh))) scrb |= 1;
	if(mWidgetIsVisible(M_WIDGET(p->sv.scrv))) scrb |= 2;
	
	//area
	
	if(p->sv.area)
	{
		mWidgetMoveResize(M_WIDGET(p->sv.area),
			box.x, box.y,
			box.w - ((scrb & 2)? MSCROLLBAR_WIDTH: 0),
			box.h - ((scrb & 1)? MSCROLLBAR_WIDTH: 0));
	}
	
	//スクロールバー
	
	edge = (scrb == 3)? MSCROLLBAR_WIDTH: 0;
	
	if(p->sv.scrh && (scrb & 1))
	{
		mWidgetMoveResize(M_WIDGET(p->sv.scrh),
			box.x, box.y + box.h - MSCROLLBAR_WIDTH,
			box.w - edge, MSCROLLBAR_WIDTH); 
	}

	if(p->sv.scrv && (scrb & 2))
	{
		mWidgetMoveResize(M_WIDGET(p->sv.scrv),
			box.x + box.w - MSCROLLBAR_WIDTH, box.y,
			MSCROLLBAR_WIDTH, box.h - edge); 
	}
}

/** 描画 */

void _draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	mScrollView *p = M_SCROLLVIEW(wg);
	int frame;
	
	//枠

	if(p->sv.style & MSCROLLVIEW_S_FRAME)
		mPixbufBox(pixbuf, 0, 0, wg->w, wg->h, MSYSCOL(FRAME));
	
	//右下の余白
	
	frame = ((p->sv.style & MSCROLLVIEW_S_FRAME) != 0);
	
	if(mWidgetIsVisible(M_WIDGET(p->sv.scrh))
		&& mWidgetIsVisible(M_WIDGET(p->sv.scrv)))
	{
		mPixbufFillBox(pixbuf,
			wg->w - frame - MSCROLLBAR_WIDTH,
			wg->h - frame - MSCROLLBAR_WIDTH,
			MSCROLLBAR_WIDTH, MSCROLLBAR_WIDTH,
			MSYSCOL(FACE));
	}
}

/* @} */


//*********************************
// mScrollViewArea
//*********************************


/**********//**

@defgroup scrollviewarea mScrollViewArea
@brief スクロールビューの表示エリア

スクロールビューのスクロールバーが操作された時は、デフォルトで mScrollViewArea に通知される。

@ingroup group_widget
@{

@file mScrollViewArea.h
@def M_SCROLLVIEWAREA(p)
@struct _mScrollViewArea
@struct mScrollViewAreaData
@enum MSCROLLVIEWAREA_NOTIFY

***********/


static mBool _svarea_isBarVisible(mScrollViewArea *p,int size,mBool horz)
{
	return TRUE;
}

/** 作成 */

mScrollViewArea *mScrollViewAreaNew(int size,mWidget *parent)
{
	mScrollViewArea *p;
	
	if(size < sizeof(mScrollViewArea)) size = sizeof(mScrollViewArea);
	
	p = (mScrollViewArea *)mWidgetNew(size, parent);
	if(!p) return NULL;
	
	p->sva.isBarVisible = _svarea_isBarVisible;
	
	return p;
}

/** スクロール位置取得 */

void mScrollViewAreaGetScrollPos(mScrollViewArea *p,mPoint *pt)
{
	mScrollView *sv = M_SCROLLVIEW(p->wg.parent);
	
	pt->x = (sv->sv.scrh)? (sv->sv.scrh)->sb.pos: 0;
	pt->y = (sv->sv.scrv)? (sv->sv.scrv)->sb.pos: 0;
}

/** 水平スクロール位置取得 */

int mScrollViewAreaGetHorzScrollPos(mScrollViewArea *p)
{
	mScrollBar *sb = M_SCROLLVIEW(p->wg.parent)->sv.scrh;

	return (sb)? sb->sb.pos: 0;
}

/** 垂直スクロール位置取得 */

int mScrollViewAreaGetVertScrollPos(mScrollViewArea *p)
{
	mScrollBar *sb = M_SCROLLVIEW(p->wg.parent)->sv.scrv;

	return (sb)? sb->sb.pos: 0;
}

/** スクロールバー取得 */

mScrollBar *mScrollViewAreaGetScrollBar(mScrollViewArea *p,mBool vert)
{
	if(vert)
		return M_SCROLLVIEW(p->wg.parent)->sv.scrv;
	else
		return M_SCROLLVIEW(p->wg.parent)->sv.scrh;
}

/** @} */
