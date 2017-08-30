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
 * [イベント・共通内部処理]
 *****************************************/

#include <stdio.h>

#include "mDef.h"
#include "mEvent.h"
#include "mWidget.h"

#include "mAppDef.h"
#include "mWindowDef.h"
#include "mKeyDef.h"

#include "mWidget_pv.h"
#include "mWindow_pv.h"
#include "mEventList.h"


//---------------------

mBool __mAccelerator_keyevent(mAccelerator *accel,uint32_t key,uint32_t state,int press);
mBool __mMenuBar_showhotkey(mMenuBar *mbar,uint32_t key,uint32_t state,int press);

//---------------------


//==========================
// sub
//==========================


/** キー処理時、現在のフォーカスウィジェット取得
 * 
 * @return NULL でウィンドウ側で処理する */

static mWidget *_get_key_focus_widget(mWindow *win,uint32_t key)
{
	mWidget *wg;

	wg = win->win.focus_widget;
	
	if(wg)
	{
		if(wg->fOption & MWIDGET_OPTION_ONFOCUS_ALLKEY_TO_WINDOW)
			//フォーカス時にすべてのキーをウィンドウ側で処理
			wg = NULL;
		else if(key == MKEY_TAB)
		{
			//各特殊キーは、フォーカス側が受け取ると設定されている場合のみ
			if(!(wg->fAcceptKeys & MWIDGET_ACCEPTKEY_TAB))
				wg = NULL;
		}
		else if(key == MKEY_ENTER)
		{
			if(!(wg->fAcceptKeys & MWIDGET_ACCEPTKEY_ENTER))
				wg = NULL;
		}
		else if(key == MKEY_ESCAPE)
		{
			if(!(wg->fAcceptKeys & MWIDGET_ACCEPTKEY_ESCAPE))
				wg = NULL;
		}
		else if(wg->fOption & MWIDGET_OPTION_ONFOCUS_NORMKEY_TO_WINDOW)
			//フォーカス時、通常キーはウィンドウ側で処理
			wg = NULL;
	}
	
	return wg;
}


//==========================
// イベント共通処理
//==========================


/** ENTER/LEAVE イベント追加
 * 
 * カーソル下のウィジェットが src -> dst へ移動した */

void __mEventAppendEnterLeave(mWidget *src,mWidget *dst)
{
	if(src == dst) return;

	if(src)
		mEventListAppend_widget(src, MEVENT_LEAVE);
	
	if(dst)
	{
		mEventListAppend_widget(dst, MEVENT_ENTER);

		//カーソル変更

		__mWindowSetCursor(dst->toplevel, dst->cursorCur);
	}
}

/** ウィジェットのグラブ解放時
 *
 * (グラブ中は ENTER/LEAVE イベントを抑制しているので、
 *  グラブ解放時に現状に合わせてイベントを追加) */

void __mEventUngrabPointer(mWidget *p)
{
	mWidget *wg;
	mPoint pt;

	//現在のカーソル下のウィジェット

	mWidgetGetCursorPos(M_WIDGET(p->toplevel), &pt);
	
	wg = mWidgetGetUnderWidget(M_WIDGET(p->toplevel), pt.x, pt.y);

	//LEAVE/ENTER

	__mEventAppendEnterLeave(MAPP->widgetOver, wg);
	
	MAPP->widgetOver = wg;
}

/** ポインタ時・カーソル下のウィジェット取得 */

mWidget *__mEventGetPointerWidget(mWindow *win,int x,int y)
{
	if(MAPP->widgetGrab)
		return MAPP->widgetGrab;
	else
		return mWidgetGetUnderWidget(M_WIDGET(win), x, y);
}

/** キー関連イベントの送り先ウィジェット取得
 * 
 * @return 対象ウィジェットに送る必要がない場合 NULL */

mWidget *__mEventGetKeySendWidget(mWindow *win,mWindow *popup)
{
	mWidget *wg;

	//送り先ウィンドウ
	
	if(popup) win = popup;
	
	//フォーカス
	
	wg = (win->win.focus_widget)? win->win.focus_widget: M_WIDGET(win);
	
	if(!(wg->fState & MWIDGET_STATE_ENABLED))
		return NULL;
	
	return wg;
}

/** キー処理
 * 
 * @return キーイベントの送り先 (NULL でイベント送らない) */

mWidget *__mEventProcKey(mWindow *win,mWindow *popup,
	uint32_t keycode,uint32_t state,int press)
{
	mWindow *target_win;
	mWidget *send_wg;

	//ポップアップ時は、常にポップアップウィンドウで処理
	
	target_win = (popup)? popup: win;
	
	//トップウィンドウの常時処理
	
	if(!MAPP->widgetGrab && !MAPP->widgetGrabKey)
	{
		//アクセラレータ
		
		if(win->win.accelerator
			&& __mAccelerator_keyevent(win->win.accelerator, keycode, state, press))
			return NULL;
	
		//メニューバーのホットキー (Alt+)
		
		if(win->win.menubar
			&& __mMenuBar_showhotkey(win->win.menubar, keycode, state, press))
			return NULL;
	}
	
	//送り先ウィジェット

	if(MAPP->widgetGrabKey)
		send_wg = MAPP->widgetGrabKey;
	else
		send_wg = _get_key_focus_widget(target_win, keycode);

	if(send_wg) return send_wg;

	//-------- send_wg == NULL の場合は、ウィンドウ側で処理

	//[TAB : press] フォーカス移動

	if(keycode == MKEY_TAB && (target_win->win.fStyle & MWINDOW_S_TABMOVE))
	{
		if(press && !MAPP->widgetGrab)
			__mWindowMoveNextFocus(target_win);
		
		return NULL;
	}
	
	//[ENTER : press/release] デフォルトウィジェットへ送る
	
	if(keycode == MKEY_ENTER)
	{
		send_wg = __mWidgetGetTreeNext_state(NULL,
					M_WIDGET(target_win), MWIDGET_STATE_ENTER_DEFAULT);

		if(send_wg) return send_wg;
	}

	return M_WIDGET(target_win);
}

/** 予期しないグラブ解放時のイベント追加 */

void __mEventAppendFocusOut_ungrab(mWindow *win)
{
	mEvent *ev;
	mWidget *grabwg;

	grabwg = MAPP->widgetGrab;

	//グラブ解放

	mWidgetUngrabPointer(grabwg);

	//グラブウィジェットの FOCUSOUT

	ev = mEventListAppend_widget(grabwg, MEVENT_FOCUS);
	if(ev)
	{
		ev->focus.bOut = TRUE;
		ev->focus.by   = MEVENT_FOCUS_BY_UNGRAB;
	}

	//ウィンドウの FOCUSOUT

	if(grabwg != M_WIDGET(win))
	{
		ev = mEventListAppend_widget(M_WIDGET(win), MEVENT_FOCUS);
		if(ev)
		{
			ev->focus.bOut = TRUE;
			ev->focus.by   = MEVENT_FOCUS_BY_UNGRAB;
		}
	}
}

/** フォーカス処理 */

void __mEventProcFocus(mWindow *win,int focusin)
{
	mWidget *wg;
	mEvent *ev;
	
	//子ウィジェットのフォーカス処理

	if(focusin)
	{
		//FOCUS IN
	
		win->wg.fState |= MWIDGET_STATE_FOCUSED;
				
		if(!win->win.focus_widget)
		{
			wg = __mWindowGetDefaultFocusWidget(win);
		
			if(wg)
			{
				win->win.focus_widget = wg;
				
				__mWidgetSetFocus(wg, MEVENT_FOCUS_BY_WINDOW);
			}
		}
	}
	else
	{
		//FOCUS OUT
	
		win->wg.fState &= ~MWIDGET_STATE_FOCUSED;
		
		if(win->win.focus_widget)
		{
			__mWidgetKillFocus(win->win.focus_widget, MEVENT_FOCUS_BY_WINDOW);
		
			win->win.focus_widget = NULL;
		}
	}
	
	//FOCUS イベント (ウィンドウに)
	
	ev = mEventListAppend_widget(M_WIDGET(win), MEVENT_FOCUS);
	if(ev)
	{
		ev->focus.bOut = (focusin == 0);
		ev->focus.by   = MEVENT_FOCUS_BY_WINDOW;
	}
}


//=================================
// デバッグ用イベント表示
//=================================


#define _PUT_EVENT_S(type)\
	fprintf(stderr, "%u:%p:EV_" #type "\n", cnt, p->widget)

#define _PUT_EVENT(type,fmt,...)\
	fprintf(stderr, "%u:%p:EV_" #type " " fmt "\n", cnt, p->widget, __VA_ARGS__)


/** イベント情報表示 */

void __mEventPutDebug(mEvent *p)
{
	static uint32_t cnt = 0;

	switch(p->type)
	{
		case MEVENT_POINTER:
			_PUT_EVENT(POINTER, "type:%d (%d,%d) [%d,%d] btt:%d state:0x%x",
				p->pt.type, p->pt.x, p->pt.y,
				p->pt.rootx, p->pt.rooty, p->pt.btt, p->pt.state);
			break;
		case MEVENT_PENTABLET:
			_PUT_EVENT(PENTABLET, "type:%d (%.1f,%.1f) [%.3f] btt:%d state:0x%x",
				p->pen.type, p->pen.x, p->pen.y, p->pen.pressure,
				p->pen.btt, p->pen.state);
			break;
		case MEVENT_POINTER_MODAL:
			_PUT_EVENT(POINTER_MODAL, "type:%d (%d,%d) [%d,%d] btt:%d state:0x%x",
				p->pt.type, p->pt.x, p->pt.y,
				p->pt.rootx, p->pt.rooty, p->pt.btt, p->pt.state);
			break;
		case MEVENT_SCROLL:
			_PUT_EVENT(SCROLL, "dir:%d state:0x%x", p->scr.dir, p->scr.state);
			break;
	
		case MEVENT_NOTIFY:
			_PUT_EVENT(NOTIFY, "id:%d type:%d param1:%ld param2:%ld",
				p->notify.id, p->notify.type, (long)p->notify.param1, (long)p->notify.param2);
			break;
		case MEVENT_COMMAND:
			_PUT_EVENT(COMMAND, "id:%d by:%d param:%ld",
				p->cmd.id, p->cmd.by, (long)p->cmd.param);
			break;
		case MEVENT_MENU_POPUP:
			_PUT_EVENT(MENU_POPUP, "menu:%p id:%d",
				p->popup.menu, p->popup.itemID);
			break;
		case MEVENT_CONSTRUCT:
			_PUT_EVENT_S(CONSTRUCT);
			break;

		case MEVENT_KEYDOWN:
			_PUT_EVENT(KEYDOWN, "code:0x%x vcode:0x%x raw:%d state:0x%x",
				p->key.code, p->key.sys_vcode, p->key.rawcode, p->key.state);
			break;
		case MEVENT_KEYUP:
			_PUT_EVENT(KEYUP, "code:0x%x vcode:0x%x raw:%d state:0x%x",
				p->key.code, p->key.sys_vcode, p->key.rawcode, p->key.state);
			break;
		case MEVENT_CHAR:
			_PUT_EVENT(CHAR, "char:'%c'", p->ch.ch);
			break;
		case MEVENT_STRING:
			_PUT_EVENT(STRING, "string:'%s' len:%d", (char *)p->data, p->str.len);
			break;
	
		case MEVENT_FOCUS:
			_PUT_EVENT(FOCUS, "%s by:%d", (p->focus.bOut)? "OUT": "IN", p->focus.by);
			break;
		case MEVENT_ENTER:
			_PUT_EVENT_S(ENTER);
			break;
		case MEVENT_LEAVE:
			_PUT_EVENT_S(LEAVE);
			break;
		case MEVENT_CONFIGURE:
			_PUT_EVENT(CONFIGURE, "f:%d (%d,%d)-(%dx%d)",
				p->config.flags, p->config.x, p->config.y, p->config.w, p->config.h);
			break;

		case MEVENT_MAP:
			_PUT_EVENT_S(MAP);
			break;
		case MEVENT_UNMAP:
			_PUT_EVENT_S(UNMAP);
			break;
		case MEVENT_CLOSE:
			_PUT_EVENT_S(CLOSE);
			break;
	}
	
	cnt++;
	
	fflush(stderr);
}
