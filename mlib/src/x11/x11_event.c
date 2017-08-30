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
 * <X11> イベント処理
 *****************************************/

#include <stdio.h>

#define MINC_X11_ATOM
#define MINC_X11_UTIL
#define MINC_X11_XI2
#include "mSysX11.h"

#include "mWindowDef.h"

#include "mWidget.h"
#include "mWindow.h"
#include "mEvent.h"
#include "mUtilCharCode.h"

#include "mAppPrivate.h"
#include "mEventList.h"
#include "mWidget_pv.h"
#include "mWindow_pv.h"
#include "mEvent_pv.h"

#include "x11_event_sub.h"
#include "x11_util.h"
#include "x11_window.h"
#include "x11_clipboard.h"
#include "x11_dnd.h"
#include "x11_xinput2.h"


//--------------------------

#if 0
#define _PUT_DETAIL(fmt,...)  fprintf(stderr, "xev:" fmt, __VA_ARGS__)
#else
#define _PUT_DETAIL(fmt,...)
#endif

uint32_t __mSystemKeyConvert(uint32_t);

//--------------------------


//================================
// 各イベント処理
//================================


/** ボタン押し/離し (通常イベント時) */

static void _event_button_pressrelease(_X11_EVENT *p,mBool press)
{
	XButtonEvent *xev = (XButtonEvent *)p->xev;
	mWidget *wg;
	mEvent *ev;
	int evtype;
	uint32_t state;

	state = mX11Event_convertState(xev->state);

	//基本処理

	wg = mX11Event_procButton_PressRelease(p,
		xev->x, xev->y, xev->x_root, xev->y_root, xev->button,
		state, xev->time, press, &evtype);

	//PENTABLET イベントが必要だが XI2 が使われていない場合
	
	if(wg && (wg->fEventFilter & MWIDGET_EVENTFILTER_PENTABLET)
		&& MAPP_SYS->xi2_opcode == -1)
	{
		ev = mEventListAppend_widget(wg,
				(p->modalskip)? MEVENT_PENTABLET_MODAL: MEVENT_PENTABLET);
		if(ev)
		{
			ev->pen.type = evtype;
			ev->pen.x = xev->x - wg->absX;
			ev->pen.y = xev->y - wg->absY;
			ev->pen.rootx = xev->x_root;
			ev->pen.rooty = xev->y_root;
			ev->pen.btt = xev->button;
			ev->pen.state = state;
			ev->pen.pressure = 1;
			ev->pen.flags = 0;
		}
	}
}

/** カーソル移動 */

static void _event_motion(_X11_EVENT *p)
{
	XMotionEvent *xev = (XMotionEvent *)p->xev;
	mWidget *wg;
	mEvent *ev;
	uint32_t state;

	state = mX11Event_convertState(xev->state);

	wg = mX11Event_procMotion(p, xev->x, xev->y, xev->x_root, xev->y_root, state);

	//PENTABLET イベントが必要だが XI2 が使われていない場合
	
	if(wg && (wg->fEventFilter & MWIDGET_EVENTFILTER_PENTABLET)
		&& MAPP_SYS->xi2_opcode == -1)
	{
		ev = mEventListAppend_widget(wg, MEVENT_PENTABLET);
		if(ev)
		{
			ev->pen.type = MEVENT_POINTER_TYPE_MOTION;
			ev->pen.x = xev->x - wg->absX;
			ev->pen.y = xev->y - wg->absY;
			ev->pen.rootx = xev->x_root;
			ev->pen.rooty = xev->y_root;
			ev->pen.btt = M_BTT_NONE;
			ev->pen.state = state;
			ev->pen.pressure = 1;
			ev->pen.flags = 0;
		}
	}
}

/** enter/leave イベント
 *
 * [!] グラブ中は処理しない */

static void _event_enterleave(_X11_EVENT *p,int enter)
{
	mWidget *wg,*winwg;

	_PUT_DETAIL("[%s] 0x%lx mode:%d detail:%d\n",
		(enter)?"enter":"leave",
		p->xev->xcrossing.window, p->xev->xcrossing.mode, p->xev->xcrossing.detail);

	//無効

	if(p->modalskip || MAPP->widgetGrab) return;

	if(p->xev->xcrossing.mode != NotifyNormal) return;

	//

	winwg = M_WIDGET(p->win);
	
	if(enter)
	{
		//------ ENTER

		//ウィンドウの ENTER

		__mEventAppendEnterLeave(NULL, winwg);

		//カーソル下のウィジェットの ENTER
	
		wg = mWidgetGetUnderWidget(winwg,
				p->xev->xcrossing.x, p->xev->xcrossing.y);

		if(wg != winwg)
			__mEventAppendEnterLeave(NULL, wg);
	}
	else
	{
		//------ LEAVE

		//現在のカーソル下のウィジェットの LEAVE

		if(MAPP->widgetOver && p->win == MAPP->widgetOver->toplevel)
		{
			__mEventAppendEnterLeave(MAPP->widgetOver, NULL);
			wg = MAPP->widgetOver;
		}
		else
			wg = NULL;

		//ウィンドウの LEAVE

		if(wg != winwg)
			__mEventAppendEnterLeave(winwg, NULL);

		//

		wg = NULL;
	}

	//現在のカーソル下
	
	MAPP->widgetOver = wg;
}

/** キーイベント */

static void _event_key(_X11_EVENT *p,int press)
{
	XEvent evtmp;
	KeySym keysym;
	int len,send_string = 0;
	char text_char,*utf8_string;
	uint32_t code,state;
	mWidget *send_wg;
	mEvent *ev;

	//_NET_WM_USER_TIME

	if(press)
		mX11WindowSetUserTime(p->win, p->xev->xkey.time);

	/* KeyRelease 時、次のイベントが KeyPress の場合はオートリピートなので処理しない。
	 * (次の KeyPress は通常通り実行される)
	 * 
	 * [!] ずっとキーを押していると、たまにオートリピート判定が正しく行われず
	 *     すり抜けてしまう。 */

	if(!press && XPending(XDISP))
	{
		XPeekEvent(XDISP, &evtmp);
		
		if(evtmp.xkey.type == KeyPress
			&& evtmp.xkey.keycode == p->xev->xkey.keycode
			&& evtmp.xkey.time == p->xev->xkey.time)
			return;
	}
	
	//ブロック

	if(_IS_BLOCK_USERACTION) return;

	//コード＆文字取得
	
	keysym = 0;
	text_char = 0;
	utf8_string = NULL;
	 
	if(press && p->win->win.sys->im_xic)
	{
		//-------- IM 用
		
		wchar_t wc[32],*pwc = NULL;
		Status ret;
		
		//文字列とキー識別子取得
		
		len = XwcLookupString(p->win->win.sys->im_xic, &p->xev->xkey,
				wc, 32, &keysym, &ret);
				
		if(ret == XBufferOverflow)
		{
			pwc = (wchar_t *)mMalloc(sizeof(wchar_t) * len, FALSE);
			if(!pwc) return;
			
			len = XwcLookupString(p->win->win.sys->im_xic, &p->xev->xkey,
					pwc, len, &keysym, &ret);
		}
		
		//文字列がある場合
		
		if(ret == XLookupChars || ret == XLookupBoth)
		{
			if(len == 1 && *wc < 128)
			{
				if(*wc >= 32 && *wc < 127)
					text_char = (char)*wc;
			}
			else
			{
				//UTF-8 に変換
			
				utf8_string = mWideToUTF8_alloc(
					(pwc)? pwc: wc, len, &len);
			}
		}
		
		if(pwc) mFree(pwc);
	}
	else
	{
		//-------- 通常
		/* [!] NULL 文字はセットされない場合がある */
	
		char m[16];

		len = XLookupString((XKeyEvent *)p->xev, m, 16, &keysym, NULL);
		
		if(len == 1 && *m >= 32 && *m < 127)
			text_char = *m;
	}
	
	//
	
	code  = __mSystemKeyConvert(keysym);
	state = mX11Event_convertState(p->xev->xkey.state);

	//イベント
	
	send_wg = __mEventProcKey(p->win, p->rundat->popup, code, state, press);
	
	if(send_wg && (send_wg->fState & MWIDGET_STATE_ENABLED))
	{
		//KEYDOWN/UP (code が未定義の場合もあり)

		if(send_wg->fEventFilter & MWIDGET_EVENTFILTER_KEY)
		{
			ev = mEventListAppend_widget(send_wg,
					(press)? MEVENT_KEYDOWN: MEVENT_KEYUP);
		
			ev->key.code = code;
			ev->key.sys_vcode = keysym;
			ev->key.rawcode = p->xev->xkey.keycode;
			ev->key.state = state;
			ev->key.bGrab = (MAPP->widgetGrab != NULL);
		}
		
		//CHAR (制御文字は除く)
		
		if(press && text_char
			&& (send_wg->fEventFilter & MWIDGET_EVENTFILTER_CHAR))
		{
			ev = mEventListAppend_widget(send_wg, MEVENT_CHAR);

			ev->ch.ch = text_char;
		}
		
		//STRING
		
		if(utf8_string && (send_wg->fEventFilter & MWIDGET_EVENTFILTER_STRING))
		{
			ev = mEventListAppend_widget(send_wg, MEVENT_STRING);
			
			ev->data = utf8_string;
			ev->str.len = len;
			
			send_string = 1;
		}
	}
	
	//EV_STRING を送らなかった場合
	
	if(utf8_string && !send_string)
		mFree(utf8_string);
}

/** ConfigureNotify イベント */

static int _event_configure(_X11_EVENT *p)
{
	XConfigureEvent *xev = (XConfigureEvent *)p->xev;
	XEvent evtmp;
	mRect rc;
	int x,y,w,h,fmove;
	uint32_t flags;

	//表示されていない状態では無効

	if(!(p->win->win.sys->fStateReal & MX11_WIN_STATE_REAL_MAP))
		return -1;

	/* 同じウィンドウの ConfigureNotify イベントを削除してまとめる。
	 * send_event が 0 以外の場合、x,y はルートウィンドウ座標での位置 (枠含まない)
	 * send_event が 0 の場合、x,y はウィンドウ (枠含む) の左上位置からの相対位置
	 */
	
	x = xev->x, y = xev->y;
	w = xev->width, h = xev->height;
	fmove = (xev->send_event != 0);
	
	while(XCheckTypedWindowEvent(XDISP, xev->window, ConfigureNotify, &evtmp))
	{
		w = evtmp.xconfigure.width;
		h = evtmp.xconfigure.height;

		if(evtmp.xconfigure.send_event)
		{
			x = evtmp.xconfigure.x;
			y = evtmp.xconfigure.y;
			fmove = 1;
		}
	}
	
	//
	
	flags = 0;
	
	p->ev->config.x = p->win->wg.x;
	p->ev->config.y = p->win->wg.y;
	p->ev->config.w = w;
	p->ev->config.h = h;

	//位置
	
	if(fmove)
	{
		//枠を含める
		 
		mWindowGetFrameWidth(p->win, &rc);

		x -= rc.x1;
		y -= rc.y1;

		//位置が変わった

		if(p->win->wg.x != x || p->win->wg.y != y)
		{
			flags |= MEVENT_CONFIGURE_FLAGS_MOVE;
			p->ev->config.x = x;
			p->ev->config.y = y;
			
			p->win->wg.x = x;
			p->win->wg.y = y;
		}
	}
	
	//サイズ
	
	if(w != p->win->wg.w || h != p->win->wg.h)
	{
		flags |= MEVENT_CONFIGURE_FLAGS_SIZE;
		
		__mWindowOnResize(p->win, w, h);
		__mWidgetOnResize(M_WIDGET(p->win));
	}
	
	//
	
	if(!flags)
		return -1;
	else
	{
		p->ev->config.flags = flags;
		return MEVENT_CONFIGURE;
	}
}

/** フォーカス */

static int _event_focus(_X11_EVENT *p,int focusin)
{
	int mode;

	mode = p->xev->xfocus.mode;

	_PUT_DETAIL("[%s] 0x%lx mode:%d detail:%d\n",
		(focusin)?"focusin":"focusout", p->xev->xfocus.window,
		mode, p->xev->xfocus.detail);

	/* [mode]
	 * NotifyNormal (0) : 通常のフォーカスイベント
	 * NotifyGrab (1)   : キーボードグラブによるフォーカスアウト
	 * NotifyUngrab (2) : キーボードグラブによるフォーカスイン
	 * NotifyWhileGrabbed (3) : グラブ中のフォーカスイベント
	 *
	 * Alt+Tab 時は、
	 * <キー押し直後>
	 * FocusOut : mode=NotifyGrab detail=NotifyAncestor
	 * FocusOut : mode=NotifyGrab detail=NotifyPointer
	 * <他のウィンドウ選択後>
	 * FocusOut : mode=NotifyNormal detail=NotifyNonlinear
	 *
	 * ウィンドウなどが出ないショートカットキー動作時は、
	 * <キー押し直後>
	 * FocusOut : mode=NotifyGrab detail=NotifyAncestor
	 * FocusOut : mode=NotifyUngrab detail=NotifyPointer
	 * <キー離し後>
	 * FocusIn : mode=NotifyUngrab detail=NotifyAncestor */

	if(mode != NotifyGrab && mode != NotifyUngrab)
	{
		//フォーカスアウト
	
		if(!focusin)
		{
			//グラブ中だった場合は、UNGRAB でイベント追加

			if(MAPP->widgetGrab && MAPP->widgetGrab->toplevel == p->win)
				__mEventAppendFocusOut_ungrab(p->win);

			//ポップアップウィンドウのフォーカスアウト
			/* ポップアップウィンドウの場合は、そもそもそのウィンドウにフォーカスが移っていない。
			 * なので、ポップアップのオーナーウィンドウがフォーカスアウトした場合は
			 * ポップアップもフォーカスアウトしたとみなす。 */

			if(p->rundat->popup
				&& (p->rundat->popup)->win.owner == p->win)
				__mEventProcFocus(p->rundat->popup, focusin);
		}

		//フォーカスイベント

		__mEventProcFocus(p->win, focusin);
	}
	
	return -1;
}

/** WM_PROTOCOLS */

static int _event_wm_protocols(_X11_EVENT *p)
{
	XClientMessageEvent *xev = (XClientMessageEvent *)p->xev;
	XClientMessageEvent evtmp;
	Atom mestype;
	mWindow *pwin;

	//timeLast 更新

	if((Time)xev->data.l[1] > MAPP_SYS->timeLast)
		MAPP_SYS->timeLast = (Time)xev->data.l[1];
	
	//
	
	mestype = (Atom)xev->data.l[0];
	
	if(mestype == MX11ATOM(WM_TAKE_FOCUS))
	{
		/* 指定ウィンドウにフォーカスをセット。
		 * モーダル時はそのウィンドウ以外にフォーカスをセットしない。
		 *
		 * [!] XSetInputFocus() を行う場合、
		 * 一つのダイアログ終了後に続けてダイアログを表示しようとすると
		 * X エラーが出るので、XErrHandler() でエラーを無効にしている。 */

		_PUT_DETAIL("[WM_TAKE_FOCUS] 0x%lx\n", xev->window);

		if(MAPP_SYS->timeUser == 0)
			MAPP_SYS->timeUser = MAPP_SYS->timeLast;
	
		pwin = (p->rundat->modal)? p->rundat->modal: p->win;

		mX11WindowSetUserTime(pwin, MAPP_SYS->timeUser);
		
		XSetInputFocus(XDISP, WINDOW_XID(pwin), RevertToParent, xev->data.l[1]);
	}
	else if(mestype == MX11ATOM(NET_WM_PING))
	{
		/* アプリが応答可能かどうかを判断するためウィンドウマネージャから送られるので、返答 */

		evtmp = *xev;
		evtmp.window = XROOTWINDOW;
		
		XSendEvent(XDISP, evtmp.window, False,
			SubstructureRedirectMask | SubstructureNotifyMask, (XEvent *)&evtmp);
	}
	else if(mestype == MX11ATOM(WM_DELETE_WINDOW))
	{
		/* 閉じるボタンが押された */

		mX11WindowSetUserTime(p->win, MAPP_SYS->timeLast);

		if(!p->modalskip && !_IS_BLOCK_USERACTION)
			return MEVENT_CLOSE;
	}
	
	return -1;
}


//==========================


/** イベントを一つ処理
 * 
 * @return 追加するイベントタイプ (-1 でなし) */

static int _event_translate(_X11_EVENT *p)
{
	int type;

	type = p->xev->xany.type;
	
	switch(type)
	{
		/* 描画 */
		case Expose:
		case GraphicsExpose:
			{
			mRect rc;
			
			rc.x1 = p->xev->xexpose.x;
			rc.y1 = p->xev->xexpose.y;
			rc.x2 = p->xev->xexpose.x + p->xev->xexpose.width - 1;
			rc.y2 = p->xev->xexpose.y + p->xev->xexpose.height - 1;
			
			mWindowUpdateRect(p->win, &rc);
			}
			break;

		/* ポインタ移動 */
		case MotionNotify:
			_event_motion(p);
			return -1;
	
		/* ボタン押し/離し */
		case ButtonPress:
		case ButtonRelease:
			_event_button_pressrelease(p, (type == ButtonPress));
			return -1;
	
		/* キー */
		case KeyPress:
		case KeyRelease:
			_event_key(p, (type == KeyPress));
			return -1;
	
		/* ウィンドウの構成変更 */
		case ConfigureNotify:
			return _event_configure(p);
	
		/* Enter/Leave */
		case EnterNotify:
		case LeaveNotify:
			_event_enterleave(p, (type == EnterNotify));
			return -1;
	
		/* フォーカス */
		case FocusIn:
			return _event_focus(p, TRUE);
		case FocusOut:
			return _event_focus(p, FALSE);
	
		/* プロパティ変更 */
		case PropertyNotify:
			if(p->xev->xproperty.atom == MX11ATOM(NET_WM_STATE))
				mX11Event_change_NET_WM_STATE(p);
			break;

		/* クライアントメッセージ */
		case ClientMessage:
			if(p->xev->xclient.message_type == MX11ATOM(WM_PROTOCOLS))
				return _event_wm_protocols(p);
			else
				mX11DND_onClientMessage(MAPP_SYS->dnd, (XClientMessageEvent *)p->xev);
			break;
		
		/* セレクション要求 */
		case SelectionRequest:
			if(p->xev->xselectionrequest.selection == MX11ATOM(CLIPBOARD))
			{
				mX11ClipboardSelectionRequest(MAPP_SYS->clipb,
					(XSelectionRequestEvent *)p->xev);
			}
			break;
		
		/* セレクション所有権消去 */
		case SelectionClear:
			if(p->xev->xselectionclear.selection == MX11ATOM(CLIPBOARD))
				mX11ClipboardFreeDat(MAPP_SYS->clipb);
			break;
		
		/* ウィンドウ表示 */
		case MapNotify:
			//最初のウィンドウが表示状態になった時、StartupNotify 完了通知を送る
			
			if(!(MAPP_SYS->fState & MX11_STATE_MAP_FIRST_WINDOW))
			{
				MAPP_SYS->fState |= MX11_STATE_MAP_FIRST_WINDOW;
				mX11SendStartupNotify_complete();
			}
			
			p->win->win.sys->fStateReal |= MX11_WIN_STATE_REAL_MAP;
			mX11Event_onMap(p->win);
			return MEVENT_MAP;
		
		/* ウィンドウ非表示 */
		case UnmapNotify:
			p->win->win.sys->fStateReal &= ~MX11_WIN_STATE_REAL_MAP;
			return MEVENT_UNMAP;

		/* キーボードマッピング */
		case MappingNotify:
			if(p->xev->xmapping.request != MappingPointer)
				XRefreshKeyboardMapping((XMappingEvent *)p->xev);
			break;
	}
	
	return -1;
}

/** XInput2 イベント */

#if defined(MLIB_ENABLE_PENTABLET) && defined(HAVE_XEXT_XINPUT2)

static void _event_xinput2(_X11_EVENT *p)
{
	XIDeviceEvent *dev;
	mWindow *win;
	mEvent *ev;
	mWidget *wg;
	int type,evtype;
	uint32_t state;

	dev = (XIDeviceEvent *)p->xev->xcookie.data;
	type = dev->evtype;

	//時間

	MAPP_SYS->timeLast = dev->time;

	//ウィンドウ

	win = mX11Event_getWindow(dev->event);
	if(!win) return;

	p->win = win;
	p->modalskip = (p->rundat->modal && p->rundat->modal != win);

	//_NET_WM_USER_TIME

	if(type == XI_ButtonPress || type == XI_KeyPress)
		mX11WindowSetUserTime(win, dev->time);

	//処理

	if(type == XI_ButtonPress
		|| type == XI_ButtonRelease
		|| type == XI_Motion)
	{
		//グラブ中のデバイス指定があれば、指定デバイス以外のイベントは無視

		if(MAPP_SYS->xi2_grab && MAPP_SYS->xi2_grab_deviceid > 0
			&& dev->sourceid != MAPP_SYS->xi2_grab_deviceid)
			return;
	
		//装飾フラグ

		state = mX11Event_convertState(dev->mods.base);

		//基本処理
	
		if(type == XI_Motion)
		{
			wg = mX11Event_procMotion(p, dev->event_x, dev->event_y,
				dev->root_x, dev->root_y, state);

			evtype = MEVENT_POINTER_TYPE_MOTION;
		}
		else
		{
			wg = mX11Event_procButton_PressRelease(p,
				dev->event_x, dev->event_y, dev->root_x, dev->root_y,
				dev->detail, state, dev->time,
				(type == XI_ButtonPress), &evtype);
		}

		//PENTABLET イベント

		if(wg)
		{
			ev = mEventListAppend_widget(wg,
					(p->modalskip)? MEVENT_PENTABLET_MODAL: MEVENT_PENTABLET);
			if(ev)
			{
				ev->pen.type = evtype;
				ev->pen.x = dev->event_x - wg->absX;
				ev->pen.y = dev->event_y - wg->absY;
				ev->pen.rootx = dev->root_x;
				ev->pen.rooty = dev->root_y;
				ev->pen.btt = (type == XI_Motion)? M_BTT_NONE: dev->detail;
				ev->pen.state = state;
				ev->pen.device_id = dev->sourceid;

				ev->pen.flags = mX11XI2_getEventInfo(dev, &ev->pen.pressure, &ev->pen.bPenTablet);
			}
		}
	}
}

#endif


//===============================


/** キュー内の X イベントを処理して内部リストに追加 */

void mX11EventTranslate(void)
{
	XEvent xev;
	mEvent event;
	mWindow *win;
	_X11_EVENT evx11;
	int type;
	
	evx11.xev    = &xev;
	evx11.ev     = &event;
	evx11.rundat = MAPP_PV->runCurrent;
	
	while(XPending(XDISP))
	{
		//X イベント取得
	
		XNextEvent(XDISP, &xev);
		
		//時間を記録
		
		mX11Event_setTime(&xev);
		
		//IM イベント処理

		if(XFilterEvent(&xev, None)) continue;

		//

		if(xev.xany.type == GenericEvent)
		{
			//----- GenericEvent イベント
			
		#if defined(MLIB_ENABLE_PENTABLET) && defined(HAVE_XEXT_XINPUT2)
			if(xev.xcookie.extension == MAPP_SYS->xi2_opcode)
			{
				if(XGetEventData(XDISP, &xev.xcookie))
				{
					_event_xinput2(&evx11);
				
					XFreeEventData(XDISP, &xev.xcookie);
				}
			}
		#endif
		}
		else
		{
			//----- 他イベント
			
			//mWindow 取得

			win = mX11Event_getWindow(xev.xany.window);
			if(!win) continue;

			//
			
			evx11.win = win;
			evx11.modalskip = (evx11.rundat->modal && evx11.rundat->modal != win);

			event.widget = (mWidget *)win;
			event.data   = NULL;
			
			//イベント処理

			type = _event_translate(&evx11);
			
			if(type != -1)
			{
				event.type = type;
				mEventListAppend(&event);
			}
		}
	}
}
