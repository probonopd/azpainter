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
 * <X11> イベント関連 サブ関数
 *****************************************/

#include <stdlib.h>

#define MINC_X11_ATOM
#include "mSysX11.h"

#include "mWindowDef.h"
#include "mAppPrivate.h"
#include "mEvent.h"
#include "mWindow_pv.h"

#include "mEventList.h"
#include "mEvent_pv.h"

#include "x11_event_sub.h"
#include "x11_window.h"
#include "x11_util.h"


/** XID からウィンドウ取得 */

mWindow *mX11Event_getWindow(Window id)
{
	mWindow *win;

	for(win = M_WINDOW(MAPP->widgetRoot->first); win; win = M_WINDOW(win->wg.next))
	{
		if(win->win.sys->xid == id)
			return win;
	}
	
	return NULL;
}

/** X イベントの時間を記録 */

void mX11Event_setTime(XEvent *p)
{
	Time user = 0, last = 0;

	switch(p->type)
	{
		case MotionNotify:
			last = p->xmotion.time;
			break;

		case ButtonPress:
			user = p->xbutton.time;
		case ButtonRelease:
			last = p->xbutton.time;
			break;

		case KeyPress:
			user = p->xkey.time;
		case KeyRelease:
			last = p->xkey.time;
			break;

		case PropertyNotify:
			last = p->xproperty.time;
			break;
		case EnterNotify:
		case LeaveNotify:
			last = p->xcrossing.time;
			break;
		case SelectionClear:
			last = p->xselectionclear.time;
			break;
	}
	
	if(user) MAPP_SYS->timeUser = user;
	if(last) MAPP_SYS->timeLast = last;
}

/** マウス/キーの装飾フラグ取得 */

uint32_t mX11Event_convertState(unsigned int state)
{
	uint32_t s = 0;

	if(state & ShiftMask) s |= M_MODS_SHIFT;
	if(state & ControlMask) s |= M_MODS_CTRL;
	if(state & Mod1Mask) s |= M_MODS_ALT;
	
	return s;
}

/** _NET_WM_STATE 変更時 */

void mX11Event_change_NET_WM_STATE(_X11_EVENT *p)
{
	Atom *patom;
	int i,cnt;
	mWindowSysDat *sys;

	//アトムリスト取得
	
	patom = (Atom *)mX11GetProperty32(p->xev->xproperty.window,
				MX11ATOM(NET_WM_STATE), XA_ATOM, &cnt);
	if(!patom) return;

	//
	
	sys = p->win->win.sys;
	
	sys->fStateReal &= ~(MX11_WIN_STATE_REAL_MAXIMIZED_HORZ
		| MX11_WIN_STATE_REAL_MAXIMIZED_VERT
		| MX11_WIN_STATE_REAL_HIDDEN
		| MX11_WIN_STATE_REAL_ABOVE);

	//fStateReal をセット
	
	for(i = 0; i < cnt; i++)
	{
		if(patom[i] == MX11ATOM(NET_WM_STATE_MAXIMIZED_HORZ))
		{
			sys->fStateReal |= MX11_WIN_STATE_REAL_MAXIMIZED_HORZ;
			sys->normalH = p->win->wg.h;
		}
		else if(patom[i] == MX11ATOM(NET_WM_STATE_MAXIMIZED_VERT))
		{
			sys->fStateReal |= MX11_WIN_STATE_REAL_MAXIMIZED_VERT;
			sys->normalW = p->win->wg.w;
		}
		else if(patom[i] == MX11ATOM(NET_WM_STATE_HIDDEN))
			sys->fStateReal |= MX11_WIN_STATE_REAL_HIDDEN;

		else if(patom[i] == MX11ATOM(NET_WM_STATE_ABOVE))
			sys->fStateReal |= MX11_WIN_STATE_REAL_ABOVE;
	}
	
	mFree(patom);
}


/** マップ(ウィンドウ表示)時の処理 */

void mX11Event_onMap(mWindow *p)
{
	Window xid;
	int f;

	xid = WINDOW_XID(p);
	f = p->win.sys->fMapRequest;

	//位置

	if(f & MX11_WIN_MAP_REQUEST_MOVE)
		__mWindowMove(p, p->wg.x, p->wg.y);

	//最小化

	if(f & MX11_WIN_MAP_REQUEST_HIDDEN)
		mX11Send_NET_WM_STATE(xid, 1, MAPP_SYS->atoms[MX11_ATOM_NET_WM_STATE_HIDDEN], 0);

	//最大化

	if(f & MX11_WIN_MAP_REQUEST_MAXIMIZE)
	{
		mX11Send_NET_WM_STATE(xid, 1,
			MAPP_SYS->atoms[MX11_ATOM_NET_WM_STATE_MAXIMIZED_HORZ],
			MAPP_SYS->atoms[MX11_ATOM_NET_WM_STATE_MAXIMIZED_VERT]);
	}
	
	//モーダル
	
	if(f & MX11_WIN_MAP_REQUEST_MODAL)
		mX11Send_NET_WM_STATE(xid, 1, MAPP_SYS->atoms[MX11_ATOM_NET_WM_STATE_MODAL], 0);
	
	//常に最前面
	
	if(f & MX11_WIN_MAP_REQUEST_ABOVE)
		mX11Send_NET_WM_STATE(xid, 1, MAPP_SYS->atoms[MX11_ATOM_NET_WM_STATE_ABOVE], 0);

	//全画面
	
	if(f & MX11_WIN_MAP_REQUEST_FULLSCREEN)
		mX11Send_NET_WM_STATE(xid, 1, mX11GetAtom("_NET_WM_STATE_FULLSCREEN"), 0);
	
	//

	XFlush(XDISP);
	
	p->win.sys->fMapRequest = 0;
}


//==========================
// 各イベント
//==========================


/** マウスボタンイベント */

mWidget *mX11Event_procButton_PressRelease(_X11_EVENT *p,
	int x,int y,int rootx,int rooty,int btt,uint32_t state,Time time,
	mBool press,int *evtype)
{
	mWidget *wg;
	mEvent *ev;
	int type;

	//_NET_WM_USER_TIME

	if(press)
		mX11WindowSetUserTime(p->win, time);

	//ブロック

	if(_IS_BLOCK_USERACTION) return NULL;

	//送り先

	wg = __mEventGetPointerWidget(p->win, x, y);
	
	//

	if(btt >= 4 && btt <= 7
		&& !(wg->fOption & MWIDGET_OPTION_DISABLE_SCROLL_EVENT))
	{
		//-------- スクロールボタン
		/* スクロールイベント無効時はポインタイベントとして扱う */
		
		if(press && !p->modalskip
			&& (wg->fEventFilter & MWIDGET_EVENTFILTER_SCROLL)
			&& (wg->fState & MWIDGET_STATE_ENABLED))
		{
			ev = mEventListAppend_widget(wg, MEVENT_SCROLL);
			if(ev)
			{
				ev->scr.dir = btt - 4;
				ev->scr.state = state;
			}
		}

		return NULL;
	}
	else
	{
		//-------- 通常ボタン
	
		type = (press)? MEVENT_POINTER_TYPE_PRESS: MEVENT_POINTER_TYPE_RELEASE;
		
		//ダブルクリック判定
		
		if(press)
		{
			if(WINDOW_XID(p->win) == MAPP_SYS->dbc_window
				&& btt == MAPP_SYS->dbc_btt
				&& time < MAPP_SYS->dbc_time + 350
				&& abs(rootx - MAPP_SYS->dbc_x) < 3 && abs(rooty - MAPP_SYS->dbc_y) < 3)
			{
				//ダブルクリック
				
				type = MEVENT_POINTER_TYPE_DBLCLK;
				MAPP_SYS->dbc_window = 0;
			}
			else
			{
				//通常クリック
			
				MAPP_SYS->dbc_window = WINDOW_XID(p->win);
				MAPP_SYS->dbc_time   = time;
				MAPP_SYS->dbc_x      = rootx;
				MAPP_SYS->dbc_y      = rooty;
				MAPP_SYS->dbc_btt    = btt;
			}
		}

		//無効ウィジェット

		if(!(wg->fState & MWIDGET_STATE_ENABLED)) return NULL;

		//POINTER イベント

		if(wg->fEventFilter & MWIDGET_EVENTFILTER_POINTER)
		{
			ev = mEventListAppend_widget(wg,
					(p->modalskip)? MEVENT_POINTER_MODAL: MEVENT_POINTER);

			if(ev)
			{
				ev->pt.type = type;
				ev->pt.x = x - wg->absX;
				ev->pt.y = y - wg->absY;
				ev->pt.rootx = rootx;
				ev->pt.rooty = rooty;
				ev->pt.btt = btt;
				ev->pt.state = state;
			}
		}

		*evtype = type;
	}
	
	return wg;
}

/** カーソル移動イベント時
 *
 * [!] グラブ中は、widgetOver は変化しない */

mWidget *mX11Event_procMotion(_X11_EVENT *p,
	int x,int y,int rootx,int rooty,uint32_t state)
{
	mWidget *wg;
	mEvent *ev;

	//カーソル下のウィジェット

	wg = __mEventGetPointerWidget(p->win, x, y);

	//ENTER/LEAVE (非グラブ時)
	
	if(!MAPP->widgetGrab && wg != MAPP->widgetOver)
	{
		__mEventAppendEnterLeave(MAPP->widgetOver, wg);
	
		MAPP->widgetOver = wg;
	}

	//ウィジェット無効

	if(!(wg->fState & MWIDGET_STATE_ENABLED)) return NULL;
	
	//MOTION イベント

	if(wg->fEventFilter & MWIDGET_EVENTFILTER_POINTER)
	{
		ev = mEventListAppend_widget(wg, MEVENT_POINTER);
		if(ev)
		{
			ev->pt.type = MEVENT_POINTER_TYPE_MOTION;
			ev->pt.x = x - wg->absX;
			ev->pt.y = y - wg->absY;
			ev->pt.rootx = rootx;
			ev->pt.rooty = rooty;
			ev->pt.btt = M_BTT_NONE;
			ev->pt.state = state;
		}
	}

	return wg;
}
