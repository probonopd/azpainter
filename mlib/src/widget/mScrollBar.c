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
 * mScrollBar [スクロールバー]
 *****************************************/

#include "mDef.h"

#include "mScrollBar.h"

#include "mWidget.h"
#include "mPixbuf.h"
#include "mEvent.h"
#include "mSysCol.h"


//-----------------------

#define _TIMERID_NOTIFY    0
#define _TIMER_NOTIFY_TIME 15

//-----------------------


/************************//**

@defgroup scrollbar mScrollBar
@brief スクロールバー

mScrollBarData::handle(sb, pos, flags) に関数をセットすると、状態の変化があった時に通知が行われずにその関数が呼ばれる。 \n
このメンバにはデフォルトで MSCROLLBAR_N_HANDLE を通知する関数がセットされている。

<h3>継承</h3>
mWidget \> mScrollBar

@ingroup group_widget
@{

@file mScrollBar.h
@def M_SCROLLBAR(p)
@def MSCROLLBAR_WIDTH
@struct _mScrollBar
@struct mScrollBarData
@enum MSCROLLBAR_STYLE
@enum MSCROLLBAR_NOTIFY
@enum MSCROLLBAR_NOTIFY_HANDLE_FLAGS

@var MSCROLLBAR_NOTIFY::MSCROLLBAR_N_HANDLE
スクロールバーが操作された。\n
param1 : 現在位置\n
param2 : フラグ (MSCROLLBAR_NOTIFY_HANDLE_FLAGS)

@var MSCROLLBAR_NOTIFY_HANDLE::MSCROLLBAR_N_HANDLE_F_CHANGE
位置が変わった

@var MSCROLLBAR_NOTIFY_HANDLE::MSCROLLBAR_N_HANDLE_F_PRESS
ドラッグ開始

@var MSCROLLBAR_NOTIFY_HANDLE::MSCROLLBAR_N_HANDLE_F_MOTION
ドラッグ中

@var MSCROLLBAR_NOTIFY_HANDLE::MSCROLLBAR_N_HANDLE_F_RELEASE
ドラッグ終了

@var MSCROLLBAR_NOTIFY_HANDLE::MSCROLLBAR_N_HANDLE_F_PAGE
バー外クリック時のページ移動

**************************/


//=================================
// sub
//=================================


/** バーの情報取得
 * 
 * barw: 表示バー部分の幅, barpos: バーの表示位置 */

static void _getBarState(mScrollBar *p,int *barw,int *barpos)
{
	int pos,bw,sbw;
	
	if(p->sb.range == 0)
		//バー無し
		bw = pos = 0;
	else
	{
		//バー全体の幅
	
		sbw = (p->sb.style & MSCROLLBAR_S_VERT)? p->wg.h: p->wg.w;
		if(sbw < 0) sbw = 0;
		
		//表示バーの幅
		
		bw = (int)((double)sbw / (p->sb.max - p->sb.min) * p->sb.page + 0.5);
		if(bw < 7) bw = 7;
		
		//バー位置
		
		pos = (int)((double)(sbw - bw) / p->sb.range * (p->sb.pos - p->sb.min) + 0.5);
		
		if(pos < 0) pos = 0;
		else if(pos > sbw - bw) pos = sbw - bw;
	}
	
	*barw = bw;
	*barpos = pos;
}

/** 内部での位置変更時 */

static void _changePos(mScrollBar *p,int pos,int flags)
{
	//位置変更

	if(mScrollBarSetPos(p, pos))
		flags |= MSCROLLBAR_N_HANDLE_F_CHANGE;

	//通知

	if(p->sb.handle)
		(p->sb.handle)(p, p->sb.pos, flags);
}

/** handle() デフォルト */

static void _handle_default(mScrollBar *p,int pos,int flags)
{
	mWidgetAppendEvent_notify(NULL, M_WIDGET(p),
		MSCROLLBAR_N_HANDLE, pos, flags);
}


//=================================
// メイン
//=================================


/** 作成 */

mScrollBar *mScrollBarNew(int size,mWidget *parent,uint32_t style)
{
	mScrollBar *p;
	
	if(size < sizeof(mScrollBar)) size = sizeof(mScrollBar);
	
	p = (mScrollBar *)mWidgetNew(size, parent);
	if(!p) return NULL;
	
	p->wg.draw = mScrollBarDrawHandle;
	p->wg.event = mScrollBarEventHandle;
	
	p->wg.fEventFilter |= MWIDGET_EVENTFILTER_POINTER;
	p->wg.hintW = p->wg.hintH = MSCROLLBAR_WIDTH;
	
	p->sb.style = style;
	p->sb.handle = _handle_default;

	return p;
}

/** 現在位置が先頭か */

mBool mScrollBarIsTopPos(mScrollBar *p)
{
	return (p->sb.pos == p->sb.min);
}

/** 現在位置が終端か */

mBool mScrollBarIsBottomPos(mScrollBar *p)
{
	return (p->sb.pos == p->sb.max - p->sb.page);
}

/** 位置取得 */

int mScrollBarGetPos(mScrollBar *p)
{
	return p->sb.pos;
}

/** 値のステータス変更 */

void mScrollBarSetStatus(mScrollBar *p,int min,int max,int page)
{
	if(min >= max) max = min + 1;

	if(page < 1) page = 1;
	else if(page > max - min) page = max - min;

	p->sb.min   = min;
	p->sb.max   = max;
	p->sb.page  = page;
	p->sb.range = max - min - page;

	if(p->sb.pos < min)
		p->sb.pos = min;
	else if(p->sb.pos > max - page)
		p->sb.pos = max - page;

	mWidgetUpdate(M_WIDGET(p));
}

/** ページ値の変更
 * 
 * @attention 現在位置が変わる場合がある */

void mScrollBarSetPage(mScrollBar *p,int page)
{
	if(page < 1)
		page = 1;
	else if(page > p->sb.max - p->sb.min)
		page = p->sb.max - p->sb.min;
	
	if(page != p->sb.page)
	{
		p->sb.page = page;
		p->sb.range = p->sb.max - p->sb.min - page;
		
		if(p->sb.pos < p->sb.min)
			p->sb.pos = p->sb.min;
		else if(p->sb.pos > p->sb.max - p->sb.page)
			p->sb.pos = p->sb.max - p->sb.page;
		
		mWidgetUpdate(M_WIDGET(p));
	}
}

/** 位置セット */

mBool mScrollBarSetPos(mScrollBar *p,int pos)
{
	if(pos < p->sb.min)
		pos = p->sb.min;
	else if(pos > p->sb.max - p->sb.page)
		pos = p->sb.max - p->sb.page;
	
	if(pos == p->sb.pos)
		return FALSE;
	else
	{
		p->sb.pos = pos;
		mWidgetUpdate(M_WIDGET(p));
		return TRUE;
	}
}

/** 位置を終端にセット */

mBool mScrollBarSetPosToEnd(mScrollBar *p)
{
	return mScrollBarSetPos(p, p->sb.max - p->sb.page);
}

/** 指定方向に移動 */

mBool mScrollBarMovePos(mScrollBar *p,int dir)
{
	return mScrollBarSetPos(p, p->sb.pos + dir);
}


//========================
// ハンドラ
//========================


/** 左ボタン押し時 */

static void _event_press_left(mScrollBar *p,int x,int y)
{
	int barw,barpos,n;
	
	_getBarState(p, &barw, &barpos);
	
	//バー無しの場合は処理なし

	if(barw == 0) return;
	
	//
	
	n = (p->sb.style & MSCROLLBAR_S_VERT)? y: x;
	
	if(barpos <= n && n < barpos + barw)
	{
		//バー内 : ドラッグ開始
		
		p->sb.dragDiff = n - barpos;
		p->sb.fpress = 1;
		
		mWidgetGrabPointer(M_WIDGET(p));
		
		_changePos(p, p->sb.pos, MSCROLLBAR_N_HANDLE_F_PRESS);
	}
	else
	{
		//バー外 : ページスクロール
		
		_changePos(p,
			p->sb.pos + ((n < barpos)? -(p->sb.page): p->sb.page),
			MSCROLLBAR_N_HANDLE_F_PAGE);
	}
}

/** カーソル移動時 */

static void _event_motion(mScrollBar *p,int x,int y)
{
	int barw,barpos,n,pos,sbw;
	
	_getBarState(p, &barw, &barpos);
		
	if(p->sb.style & MSCROLLBAR_S_VERT)
		sbw = p->wg.h, n = y;
	else
		sbw = p->wg.w, n = x;
	
	pos = (int)((double)(n - p->sb.dragDiff) / (sbw - barw) * p->sb.range + p->sb.min + 0.5);
	
	//通知はタイマーで指定時間ごとに送る
	
	if(mScrollBarSetPos(p, pos))
		mWidgetTimerAdd_unexist(M_WIDGET(p), _TIMERID_NOTIFY, _TIMER_NOTIFY_TIME, 0);
}

/** ドラッグ終了 */

static void _end_drag(mScrollBar *p)
{
	if(p->sb.fpress)
	{
		int flags = MSCROLLBAR_N_HANDLE_F_RELEASE;
	
		p->sb.fpress = 0;
		mWidgetUngrabPointer(M_WIDGET(p));
		
		//タイマー削除
		//(タイマーが残っている場合、位置変更フラグ ON)
		
		if(mWidgetTimerDelete(M_WIDGET(p), _TIMERID_NOTIFY))
			flags |= MSCROLLBAR_N_HANDLE_F_CHANGE;
		
		_changePos(p, p->sb.pos, flags);
	}
}

/** イベント */

int mScrollBarEventHandle(mWidget *wg,mEvent *ev)
{
	mScrollBar *p = M_SCROLLBAR(wg);

	switch(ev->type)
	{
		case MEVENT_POINTER:
			if(ev->pt.type == MEVENT_POINTER_TYPE_MOTION)
			{
				//移動
				
				if(p->sb.fpress)
					_event_motion(p, ev->pt.x, ev->pt.y);
			}
			else if(ev->pt.type == MEVENT_POINTER_TYPE_PRESS
				|| ev->pt.type == MEVENT_POINTER_TYPE_DBLCLK)
			{
				//押し
				
				if(ev->pt.btt == M_BTT_LEFT && p->sb.fpress == 0)
					_event_press_left(p, ev->pt.x, ev->pt.y);
			}
			else if(ev->pt.type == MEVENT_POINTER_TYPE_RELEASE)
			{
				//離し
				
				_end_drag(p);
			}
			break;
		
		//タイマー (ドラッグ中)
		case MEVENT_TIMER:
			mWidgetTimerDelete(wg, ev->timer.id);
			
			_changePos(p, p->sb.pos,
				MSCROLLBAR_N_HANDLE_F_MOTION | MSCROLLBAR_N_HANDLE_F_CHANGE);
			break;

		case MEVENT_FOCUS:
			if(ev->focus.bOut)
				_end_drag(p);
			break;
		default:
			return FALSE;
	}

	return TRUE;
}

/** 描画 */

void mScrollBarDrawHandle(mWidget *wg,mPixbuf *pixbuf)
{
	mScrollBar *p = M_SCROLLBAR(wg);
	int bw,bpos;
		
	//背景
	
	mPixbufFillBox(pixbuf, 0, 0, wg->w, wg->h, MSYSCOL(FACE_DARK));

	//枠

	if(p->sb.style & MSCROLLBAR_S_VERT)
		mPixbufLineV(pixbuf, 0, 0, wg->h, MSYSCOL(FRAME));
	else
		mPixbufLineH(pixbuf, 0, 0, wg->w, MSYSCOL(FRAME));
	
	//バー
	
	_getBarState(p, &bw, &bpos);
	
	if(bw)
	{
		if(p->sb.style & MSCROLLBAR_S_VERT)
			mPixbufFillBox(pixbuf, 2, bpos, wg->w - 3, bw, MSYSCOL(FACE_DARKER));
		else
			mPixbufFillBox(pixbuf, bpos, 2, bw, wg->h - 3, MSYSCOL(FACE_DARKER));
	}
}

/** @} */
