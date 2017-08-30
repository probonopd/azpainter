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
 * mSplitter
 *****************************************/

#include "mDef.h"

#include "mSplitter.h"

#include "mWidget.h"
#include "mWindowDef.h"
#include "mWindow.h"
#include "mPixbuf.h"
#include "mEvent.h"
#include "mSysCol.h"
#include "mCursor.h"
#include "mGui.h"


/****************//**

@defgroup splitter mSplitter
@brief スプリッター

このウィジェットを操作すると、ウィジェットツリー上における前または次のウィジェットのサイズが変更される。\n
サイズを変更するウィジェットでは、レイアウトで幅を拡張するものは EXPAND_W/H を指定。それ以外は FIX_W/H で幅を固定する。

<h3>継承</h3>
mWidget \> mSplitter

@ingroup group_widget
@{

@file mSplitter.h
@def M_SPLITTER(p)
@struct mSplitter
@struct mSplitterData
@enum MSPLITTER_STYLE
@enum MSPLITTER_NOTIFY

@var MSPLITTER_NOTIFY::MSPLITTER_N_MOVED
サイズが変更された時。\n
param1 : 前のウィジェットの増減サイズ。

*********************/

/*
 * presspos : 押し時のカーソル位置 (x or y)
 * dragdiff : ドラッグ中の変化幅 (+で prev が増加)
 */

//--------------

#define _SPL_WIDTH  6
#define _SPL_MINW   7
#define _IS_VERT(p)  (p->spl.style & MSPLITTER_S_VERT)

//--------------



/** コールバック関数デフォルト */

static int _callback_gettarget_default(mSplitter *p,mSplitterTargetInfo *info)
{
	mWidget *prev,*next;

	prev = p->wg.prev;
	next = p->wg.next;

	//前と次のいずれかがない
	
	if(!prev || !next) return 0;

	//

	info->wgprev = prev;
	info->wgnext = next;

	if(_IS_VERT(p))
	{
		info->prev_cur = prev->h;
		info->next_cur = next->h;
	}
	else
	{
		info->prev_cur = prev->w;
		info->next_cur = next->w;
	}

	info->prev_min = info->prev_max = 0;
	info->prev_max = info->next_max = info->prev_cur + info->next_cur;

	return 1;
}


//=========================
// main
//=========================


/** 作成
 *
 * レイアウトフラグは自動で MLF_EXPAND_W/H に設定される。 */

mSplitter *mSplitterNew(int size,mWidget *parent,uint32_t style)
{
	mSplitter *p;
	
	if(size < sizeof(mSplitter)) size = sizeof(mSplitter);
	
	p = (mSplitter *)mWidgetNew(size, parent);
	if(!p) return NULL;
	
	p->wg.draw = mSplitterDrawHandle;
	p->wg.event = mSplitterEventHandle;
	p->wg.fEventFilter |= MWIDGET_EVENTFILTER_POINTER;

	p->spl.style = style;
	p->spl.func_target = _callback_gettarget_default;

	//

	if(style & MSPLITTER_S_VERT)
	{
		p->wg.fLayout = MLF_EXPAND_W;
		p->wg.hintW = _SPL_MINW;
		p->wg.hintH = _SPL_WIDTH;

		mWidgetSetCursor(M_WIDGET(p), mCursorGetDefault(MCURSOR_DEF_VSPLIT));
	}
	else
	{
		p->wg.fLayout = MLF_EXPAND_H;
		p->wg.hintW = _SPL_WIDTH;
		p->wg.hintH = _SPL_MINW;

		mWidgetSetCursor(M_WIDGET(p), mCursorGetDefault(MCURSOR_DEF_HSPLIT));
	}
	
	return p;
}

/** コールバック関数セット (対象の情報取得)
 *
 * @param func  NULL でデフォルト */

void mSplitterSetCallback_getTarget(mSplitter *p,mSplitterCallbackGetTarget func,intptr_t param)
{
	p->spl.func_target = (func)? func: _callback_gettarget_default;
	p->spl.info.param = param;
}


//========================
// sub
//========================


/** ドラッグ中のバー描画 */

static void _draw_dragbar(mSplitter *p)
{
	mRect rc;

	//絶対値で直接ウィンドウ用イメージに描画

	rc.x1 = p->wg.parent->absX + p->wg.x;
	rc.y1 = p->wg.parent->absY + p->wg.y;

	if(_IS_VERT(p))
		rc.y1 += p->spl.dragdiff;
	else
		rc.x1 += p->spl.dragdiff;

	rc.x2 = rc.x1 + p->wg.w - 1;
	rc.y2 = rc.y1 + p->wg.h - 1;

	mPixbufFillBox_pat2x2(p->wg.toplevel->win.pixbuf,
		rc.x1, rc.y1, p->wg.w, p->wg.h, MPIXBUF_COL_XOR);

	//更新

	mWindowUpdateRect(p->wg.toplevel, &rc);
}

/** ドラッグ時 */

static void _event_motion(mSplitter *p,mEvent *ev)
{
	int diff,pw,nw;
	mSplitterTargetInfo *info = &p->spl.info;

	//開始時からの変化幅

	diff = ((_IS_VERT(p))? ev->pt.y: ev->pt.x) - p->spl.presspos;

	//prev のサイズ

	pw = info->prev_cur + diff;

	if(pw < info->prev_min)
	{
		pw = info->prev_min;
		diff = pw - info->prev_cur;
	}
	else if(pw > info->prev_max)
	{
		pw = info->prev_max;
		diff = pw - info->prev_cur;
	}

	//next のサイズ

	nw = info->next_cur - diff;

	if(nw < info->next_min)
	{
		nw = info->next_min;
		diff = info->next_cur - nw;
	}
	else if(nw > info->next_max)
	{
		nw = info->next_max;
		diff = info->next_cur - nw;
	}

	//バー描画

	if(diff != p->spl.dragdiff)
	{
		_draw_dragbar(p);

		p->spl.dragdiff = diff;

		_draw_dragbar(p);
	}
}

/** 適用 */

static void _set_split(mSplitter *p)
{
	mSplitterTargetInfo *info = &p->spl.info;
	mWidget *prev,*next,*wg;
	mBox box1,box2;
	int diff,diffx,diffy;

	prev = info->wgprev;
	next = info->wgnext;

	mWidgetGetBox(prev, &box1);
	mWidgetGetBox(next, &box2);

	diff = p->spl.dragdiff;

	//

	if(_IS_VERT(p))
	{
		//------- 垂直

		box1.h += diff;
		box2.y += diff;
		box2.h -= diff;

		diffx = 0;
		diffy = diff;

		//上

		if(box1.h == 0)
		{
			mWidgetShow(prev, 0);
			box1.h = 1;
		}
		else
			mWidgetShow(prev, 1);

		//下

		if(box2.h == 0)
		{
			mWidgetShow(next, 0);
			box2.h = 1;
		}
		else
			mWidgetShow(next, 1);
	}
	else
	{
		//------ 水平

		box1.w += diff;
		box2.x += diff;
		box2.w -= diff;
		
		diffx = diff;
		diffy = 0;

		//左

		if(box1.w == 0)
		{
			mWidgetShow(prev, 0);
			box1.w = 1;
		}
		else
			mWidgetShow(prev, 1);

		//右

		if(box2.w == 0)
		{
			mWidgetShow(next, 0);
			box2.w = 1;
		}
		else
			mWidgetShow(next, 1);
	}

	//移動

	mWidgetMoveResize(prev, box1.x, box1.y, box1.w, box1.h);
	mWidgetMoveResize(next, box2.x, box2.y, box2.w, box2.h);
	mWidgetMove(M_WIDGET(p), p->wg.x + diffx, p->wg.y + diffy);

	//splitter と prev/next との間に1つ以上のウィジェットがある場合

	for(wg = p->wg.prev; wg != prev; wg = wg->prev)
		mWidgetMove(wg, wg->x - diffx, wg->y - diffy);

	for(wg = p->wg.next; wg != next; wg = wg->next)
		mWidgetMove(wg, wg->x + diffx, wg->y + diffy);

	//通知

	if(p->spl.style & MSPLITTER_S_NOTIFY_MOVED)
		mWidgetAppendEvent_notify(NULL, M_WIDGET(p), MSPLITTER_N_MOVED, diff, 0);
}

/** ドラッグ解除 */

static void _release_grab(mSplitter *p)
{
	if(p->spl.fdrag)
	{
		p->spl.fdrag = 0;
		
		_draw_dragbar(p);

		if(p->spl.dragdiff)
			_set_split(p);

		mWidgetUngrabPointer(M_WIDGET(p));
	}
}


//========================
// ハンドラ
//========================


/** イベント */

int mSplitterEventHandle(mWidget *wg,mEvent *ev)
{
	mSplitter *p = M_SPLITTER(wg);

	switch(ev->type)
	{
		case MEVENT_POINTER:
			if(ev->pt.type == MEVENT_POINTER_TYPE_MOTION)
			{
				//ドラッグ

				if(p->spl.fdrag)
					_event_motion(p, ev);
			}
			else if(ev->pt.type == MEVENT_POINTER_TYPE_PRESS)
			{
				//押し

				if(ev->pt.btt == M_BTT_LEFT && p->spl.fdrag == 0
					&& (p->spl.func_target)(p, &p->spl.info))
				{
					p->spl.fdrag = 1;
					p->spl.dragdiff = 0;

					if(_IS_VERT(p))
						p->spl.presspos = ev->pt.y;
					else
						p->spl.presspos = ev->pt.x;

					mGuiDraw();

					_draw_dragbar(p);

					mWidgetGrabPointer(wg);
				}
			}
			else if(ev->pt.type == MEVENT_POINTER_TYPE_RELEASE)
			{
				//離し

				if(ev->pt.btt == M_BTT_LEFT)
					_release_grab(p);
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

void mSplitterDrawHandle(mWidget *wg,mPixbuf *pixbuf)
{
	mSplitter *p = M_SPLITTER(wg);
	int i,n;

	//背景

	mPixbufFillBox(pixbuf, 0, 0, wg->w, wg->h, MSYSCOL(FACE));

	//つまみ

	if(p->spl.style & MSPLITTER_S_VERT)
	{
		n = (wg->w - 7) >> 1;

		for(i = 0; i < 3; i++)
			mPixbufLineV(pixbuf, n + i * 3, 1, wg->h - 2, MSYSCOL(FACE_DARKER));
	}
	else
	{
		n = (wg->h - 7) >> 1;

		for(i = 0; i < 3; i++)
			mPixbufLineH(pixbuf, 1, n + i * 3, wg->w - 2, MSYSCOL(FACE_DARKER)); 
	}
}

/** @} */
