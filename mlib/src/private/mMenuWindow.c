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
 * mMenuWindow [メニューウィンドウ]
 *****************************************/

#include <string.h>

#include "mDef.h"

#include "mMenu.h"
#include "mMenu_pv.h"
#include "mMenuWindow.h"
#include "mMenuBar.h"

#include "mGui.h"
#include "mWidget.h"
#include "mWindow.h"
#include "mEvent.h"

#include "mSysCol.h"
#include "mKeyDef.h"
#include "mPixbuf.h"
#include "mFont.h"


//------------------------

#define _FRAME_H     1
#define _FRAME_V     1
#define _LEFT_SPACE  18    //左枠とテキストの間の余白
#define _RIGHT_SPACE 12    //テキストと右枠の間の余白
#define _ACCEL_SEPW  11    //テキストとアクセラレータの間の余白
#define _SCROLL_H    12    //スクロール部分の高さ 

#define _TIMERID_SUBMENU 0
#define _TIMERID_SCROLL  1

#define _MENUITEM_TOP(p)   M_MENUITEM((p)->menu.dat->list.top)
#define _IS_ITEMFLAG(p,f)  ((p)->item.flags & MMENUITEM_F_ ## f)

//------------------------

mMenuItem *__mMenuBar_motion(mMenuBar *mbar,mMenuWindow *topmenu,int x,int y);

static int _event_handle(mWidget *wg,mEvent *ev);
static void _draw_handle(mWidget *wg,mPixbuf *pixbuf);

//------------------------

/*
 * トップのポップアップ表示と同時にポインタをグラブする。
 * 表示中は常にトップのポップアップがグラブ状態。
 * 
 * itemCur : 現在選択されているアイテム
 * itemRet : [top のみ] 戻り値
 * 
 * winTop    : トップのポップアップ
 * winSub    : 表示されているサブメニュー (NULL でなし)
 * winParent : サブメニュー時、親のポップアップ
 * winFocus  : [top のみ] フォーカスのあるポップアップ
 * winScroll : [top のみ] スクロール中の場合、スクロール対象のポップアップ
 * 
 * menubar : [top のみ] メニューバー
 * 
 */


//==============================
// sub : mMenuWindow
//==============================


/** メニューウィンドウ破棄 */

static void _destroy_menuwindow(mMenuWindow *menuwin)
{
	mMenuWindow *p,*sub;

	//下層のサブメニューウィンドウも削除
	
	for(p = menuwin; p; p = sub)
	{
		sub = p->menu.winSub;
	
		mWidgetDestroy(M_WIDGET(p));
	}
}

/** ウィンドウサイズセット */

static void _setWindowSize(mMenuWindow *menuwin)
{
	mMenuItem *pi;
	int left_w = 0,right_w = 0,h = 0,draw_w = 0,w;
	mBox box;
	
	//幅と高さ
	
	for(pi = _MENUITEM_TOP(menuwin); pi; pi = M_MENUITEM(pi->i.next))
	{
		if(left_w < pi->labelLeftW) left_w = pi->labelLeftW;
		if(right_w < pi->labelRightW) right_w = pi->labelRightW;
		if(pi->item.draw && draw_w < pi->item.width) draw_w = pi->item.width;
		
		h += pi->item.height;
	}
	
	//
	
	menuwin->menu.labelLeftMaxW = left_w;
	menuwin->menu.maxH = h;

	w = left_w + right_w;
	if(right_w) w += _ACCEL_SEPW;
	
	if(w) w += _LEFT_SPACE + _RIGHT_SPACE;
	
	if(w < draw_w) w = draw_w;
	
	w += _FRAME_H * 2;
	h += _FRAME_V * 2;
	
	//高さが画面より大きい場合、スクロールあり
	
	mGetDesktopWorkBox(&box);
	
	if(h > box.h)
	{
		menuwin->menu.flags |= MMENUWINDOW_F_HAVE_SCROLL;
		
		h = box.h;
	}
	
	//サイズ変更
	
	mWidgetResize(M_WIDGET(menuwin), w, h);
}

/** アイテムの表示位置取得 */

static void _getItemVisiblePos(mMenuWindow *menuwin,
	mMenuItem *item,mPoint *pt)
{
	mMenuItem *p;
	int y;
	
	y = _FRAME_V;

	if(menuwin->menu.flags & MMENUWINDOW_F_HAVE_SCROLL)
		y += _SCROLL_H - menuwin->menu.scrY;
	
	for(p = _MENUITEM_TOP(menuwin); p && p != item;
			y += p->item.height, p = M_MENUITEM(p->i.next));
	
	pt->x = 0;
	pt->y = y;
}

/** MEVENT_MENU_POPUP : ポップアップ表示イベント (直接実行) */

static void _runEventPopup(mMenuWindow *p,int itemid,mBool menubar)
{
	mEvent ev;

	if(p->menu.widgetNotify)
	{
		memset(&ev, 0, sizeof(mEvent));

		ev.widget = p->menu.widgetNotify;
		ev.type = MEVENT_MENU_POPUP;
		ev.popup.menu = p->menu.dat;
		ev.popup.itemID = itemid;
		ev.popup.bMenuBar = menubar;

		(ev.widget->event)(ev.widget, &ev);
	}
}

/** ポップアップ終了 [top] */

static void _endPopup(mMenuWindow *p,mMenuItem *ret)
{
	p->menu.itemRet = ret;
	
	mWidgetUngrabPointer(M_WIDGET(p));
	
	mAppQuit();
}

/** メニューバーの選択移動
 * 
 * @param type [0]item [1]item の前 [2]item の次 */

static void _menubar_motion(mMenuWindow *p,mMenuItem *item,int type)
{
	if(type >= 1)
	{
		do
		{
			item = (mMenuItem *)((type == 1)? item->i.prev: item->i.next);
		} while(item && !__mMenuItemIsEnableSubmenu(item));
	}

	//

	if(item)
	{
		p = p->menu.winTop;
		
		p->menu.flags |= MMENUWINDOW_F_MENUBAR_CHANGE;
		
		_endPopup(p, item);
	}
}

/** ポップアップ表示 (トップ) */

static void _showPopupTop(mMenuWindow *p,int rootx,int rooty,uint32_t flags)
{
	p->menu.winTop = p->menu.winFocus = p;
	
	_setWindowSize(p);
	
	//表示
	
	if(flags & MMENU_POPUP_F_RIGHT)
		rootx -= p->wg.w;
	
	mWindowMoveAdjust(M_WINDOW(p), rootx, rooty, TRUE);
	mWidgetShow(M_WIDGET(p), 1);
		
	//実行
	
	mAppRunPopup(M_WINDOW(p));
}

/** ポップアップ表示 (サブメニュー)
 * 
 * parent のカレントアイテムのサブメニューを表示 */

static void _showPopupSubmenu(mMenuWindow *parent)
{
	mMenuWindow *p;
	mMenuItem *item;
	mPoint pt;
	
	item = parent->menu.itemCur;

	//作成
	
	p = mMenuWindowNew(item->item.submenu);
	if(!p) return;
	
	parent->menu.winSub = p;
	
	p->menu.winTop = parent->menu.winTop;
	p->menu.winParent = parent;
	p->menu.widgetNotify = parent->menu.widgetNotify;
	
	//ポップアップ表示イベント
	
	_runEventPopup(p, item->item.id, ((p->menu.winTop)->menu.menubar != 0));

	//サイズ
	
	_setWindowSize(p);

	//位置
	
	_getItemVisiblePos(parent, item, &pt);
	mWidgetMapPoint(M_WIDGET(parent), NULL, &pt);
	
	mWindowMoveAdjust(M_WINDOW(p),
		pt.x + parent->wg.w - 3, pt.y - _FRAME_V - 1, TRUE);

	//表示

	mWidgetShow(M_WIDGET(p), 1);
}


//=============================
//
//=============================


/** メニューウィンドウ作成 */

mMenuWindow *mMenuWindowNew(mMenu *menu)
{
	mMenuWindow *p;
	
	p = (mMenuWindow *)mWindowNew(sizeof(mMenuWindow), NULL,
			MWINDOW_S_POPUP | MWINDOW_S_NO_IM);
	
	if(!p) return NULL;
	
	p->wg.event = _event_handle;
	p->wg.draw = _draw_handle;
	p->wg.fEventFilter |= MWIDGET_EVENTFILTER_POINTER | MWIDGET_EVENTFILTER_KEY;
	
	p->menu.dat = menu;
	
	//アイテム初期化
	
	__mMenuInit(menu, mWidgetGetFont(M_WIDGET(p)));
	
	return p;
}

/** 独立ポップアップ表示
 * 
 * @param notify イベント通知先ウィジェット (NULL でなし)
 * @return 選択項目 (NULL でキャンセル) */

mMenuItem *mMenuWindowShowPopup(mMenuWindow *menuwin,
	mWidget *notify,int rootx,int rooty,uint32_t flags)
{
	mMenuItem *item;
	
	menuwin->menu.widgetNotify = notify;
	
	_showPopupTop(menuwin, rootx, rooty, flags);
	
	//----- 終了
	
	item = menuwin->menu.itemRet;
	
	//MEVENT_COMMAND
	
	if(notify && item && !(flags & MMENU_POPUP_F_NOCOMMAND))
	{
		mWidgetAppendEvent_command(notify, item->item.id,
			item->item.param1, MEVENT_COMMAND_BY_MENU);
	}
	
	_destroy_menuwindow(menuwin);
	
	return item;
}

/** メニュバーから表示
 * 
 * @return カーソル移動による選択アイテム変更があった場合、新しいアイテム。
 *  NULL で終了。 */

mMenuItem *mMenuWindowShowMenuBar(mMenuWindow *win,
	mMenuBar *menubar,int rootx,int rooty,int itemid)
{
	mMenuItem *item;
	int bchange;

	win->menu.menubar = menubar;
	win->menu.widgetNotify = mWidgetGetNotifyWidget(M_WIDGET(menubar));
	
	//MEVENT_MENU_POPUP
	
	_runEventPopup(win, itemid, TRUE);
	
	//実行
	
	_showPopupTop(win, rootx - _FRAME_H, rooty, 0);
	
	//------- 終了
	
	item = win->menu.itemRet;
	bchange = win->menu.flags & MMENUWINDOW_F_MENUBAR_CHANGE;
		
	//MEVENT_COMMAND
	
	if(item && !bchange)
	{
		mWidgetAppendEvent_command(win->menu.widgetNotify,
			item->item.id, item->item.param1, MEVENT_COMMAND_BY_MENU);
	}
	
	//削除
	
	_destroy_menuwindow(win);
	
	return (bchange)? item: NULL;
}


//=============================
// sub
//=============================


/** 項目決定 */

static void _enterItem(mMenuWindow *menuwin,mMenuItem *item)
{
	if(item && !item->item.submenu && !__mMenuItemIsDisableItem(item))
	{
		//自動チェック
		
		if(_IS_ITEMFLAG(item, AUTOCHECK))
		{
			if(_IS_ITEMFLAG(item, RADIO))
				__mMenuItemCheckRadio(item, TRUE);
			else
				item->item.flags ^= MMENUITEM_F_CHECKED;
		}
		
		//終了 (top)
		
		_endPopup(menuwin->menu.winTop, item);
	}
}

/** カレントアイテム変更 [top/sub]
 * 
 * @param item NULL で解除
 * @param scritem スクロールあり時、指定アイテムが見えるようにスクロール */

static void _setCurrentItem(mMenuWindow *menuwin,
	mMenuItem *item,mBool scritem)
{
	if(menuwin->menu.itemCur == item) return;
	
	//サブメニュー用タイマー削除
	
	mWidgetTimerDelete(M_WIDGET(menuwin->menu.winTop), _TIMERID_SUBMENU);
	
	//現在のサブメニューを削除
	
	if(menuwin->menu.winSub)
	{
		_destroy_menuwindow(menuwin->menu.winSub);
		
		menuwin->menu.winSub = NULL;
	}
	
	//変更
	
	menuwin->menu.itemCur = item;
	
	mWidgetUpdate(M_WIDGET(menuwin));
	
	//
	
	if(item)
	{
		//項目が見える位置までスクロール
	
		if(scritem && (menuwin->menu.flags & MMENUWINDOW_F_HAVE_SCROLL))
		{
			mPoint pt;
			
			_getItemVisiblePos(menuwin, item, &pt);
			
			pt.y -= _FRAME_V + _SCROLL_H;
			
			if(pt.y < 0)
				menuwin->menu.scrY += pt.y;
			else if(pt.y + item->item.height > menuwin->wg.h - _FRAME_V * 2 - _SCROLL_H * 2)
			{
				menuwin->menu.scrY += pt.y + item->item.height -
					(menuwin->wg.h - _FRAME_V * 2 - _SCROLL_H * 2);
			}	
		}
	
		//サブメニューの場合、タイマーで遅延して表示
		
		if(__mMenuItemIsEnableSubmenu(item))
		{
			mWidgetTimerAdd(M_WIDGET(menuwin->menu.winTop),
				_TIMERID_SUBMENU, 200, (intptr_t)menuwin);
		}
	}
}

/** スクロール処理 */

static void _scroll(mMenuWindow *menuwin)
{
	int scr = menuwin->menu.scrY;
	
	if(menuwin->menu.flags & MMENUWINDOW_F_SCROLL_DOWN)
	{
		scr += 20;
		
		if(scr + menuwin->wg.h - _FRAME_V * 2 - _SCROLL_H * 2 > menuwin->menu.maxH)
			scr = menuwin->menu.maxH - (menuwin->wg.h - _FRAME_V * 2 - _SCROLL_H * 2);
	}
	else
	{
		scr -= 20;
		if(scr < 0) scr = 0;
	}
	
	if(scr != menuwin->menu.scrY)
	{
		menuwin->menu.scrY = scr;
		mWidgetUpdate(M_WIDGET(menuwin));
	}
}


//=============================
// キーボード関連
//=============================


/** カレント位置を上下に移動 */

static void _moveCurUpdown(mMenuWindow *menuwin,int up)
{
	mMenuItem *p;
	
	p = menuwin->menu.itemCur;

	if(up)
	{
		//上
		
		if(p)
			p = M_MENUITEM(p->i.prev);
		else
			p = _MENUITEM_TOP(menuwin);
		
		for(; p && __mMenuItemIsDisableItem(p); p = M_MENUITEM(p->i.prev));
	}
	else
	{
		//下
		
		if(p)
			p = M_MENUITEM(p->i.next);
		else
			p = _MENUITEM_TOP(menuwin);
		
		for(; p && __mMenuItemIsDisableItem(p); p = M_MENUITEM(p->i.next));
	}
	
	if(p) _setCurrentItem(menuwin, p, TRUE);
}

/** キー押し [top/sub] */

static void _event_keydown(mMenuWindow *menuwin,uint32_t key)
{
	mMenuItem *item;
	mMenuWindow *win;
	mMenuBar *mbar;

	//ホットキー
	
	if((key >= '0' && key <= '9') || (key >= 'A' && key <= 'Z'))
	{
		item = __mMenuFindByHotkey(menuwin->menu.dat, key);
		
		if(item)
		{
			if(item->item.submenu)
				_setCurrentItem(menuwin, item, TRUE);
			else
				_enterItem(menuwin, item);
		}
		
		return;
	}
	
	//

	switch(key)
	{
		//上
		case MKEY_UP:
			_moveCurUpdown(menuwin, TRUE);
			break;
		//下
		case MKEY_DOWN:
			_moveCurUpdown(menuwin, FALSE);
			break;
		//左
		case MKEY_LEFT:
			if(menuwin->menu.winParent)
			{
				//親へ戻る
			
				(menuwin->menu.winTop)->menu.winFocus = menuwin->menu.winParent;
				
				_setCurrentItem(menuwin, NULL, FALSE);
			}
			else
			{
				//メニューバーの一つ左へ
			
				mbar = (menuwin->menu.winTop)->menu.menubar;
				
				if(mbar)
					_menubar_motion(menuwin, mbar->mb.itemCur, 1);
			}
			break;
		//右
		case MKEY_RIGHT:
			win = menuwin->menu.winSub;
		
			if(win)
			{
				//サブメニューへ
			
				(menuwin->menu.winTop)->menu.winFocus = win;
				
				_setCurrentItem(win,
					_MENUITEM_TOP(win), FALSE);
			}
			else
			{
				//メニューバーの一つ右へ
				
				mbar = (menuwin->menu.winTop)->menu.menubar;
				
				if(mbar)
					_menubar_motion(menuwin, mbar->mb.itemCur, 2);
			}
			break;
		//SPACE/ENTER
		case MKEY_SPACE:
		case MKEY_ENTER:
			_enterItem(menuwin, menuwin->menu.itemCur);
			break;
	}
}


//=============================
// ポインタ関連
//=============================


/** 指定位置下のウィンドウ取得 [top]
 * 
 * @param pt 相対座標が入る (NULL 可) */

static mMenuWindow *_getPopupByPos(mMenuWindow *topwin,
	int rootx,int rooty,mPoint *pt_rel)
{
	mMenuWindow *win,*win_ret = NULL;
	mPoint pt,pt_ret;
	
	win = topwin;
	
	for(win = topwin; win; win = win->menu.winSub)
	{
		pt.x = rootx, pt.y = rooty;
		mWidgetMapPoint(NULL, M_WIDGET(win), &pt);
		
		if(mWidgetIsContain(M_WIDGET(win), pt.x, pt.y))
		{
			win_ret = win;
			pt_ret = pt;
		}
	}
	
	if(win_ret && pt_rel)
	{
		pt_rel->x = pt_ret.x;
		pt_rel->y = pt_ret.y;
	}
	
	return win_ret;
}

/** カーソル位置からアイテム取得 */

mMenuItem *_getItemByPos(mMenuWindow *win,int y)
{
	mMenuItem *p;
	int topy,bottom;
	
	topy = _FRAME_V;
	bottom = win->wg.h - _FRAME_V;
	
	if(win->menu.flags & MMENUWINDOW_F_HAVE_SCROLL)
	{
		if(y < _FRAME_V + _SCROLL_H) return NULL;
		
		topy += _SCROLL_H - win->menu.scrY;
		bottom -= _SCROLL_H;
	}
	
	//
	
	for(p = _MENUITEM_TOP(win); p; p = M_MENUITEM(p->i.next))
	{
		if(y < topy || topy >= bottom) break;
		
		if(topy <= y && y < topy + p->item.height)
			return p;
		
		topy += p->item.height;
	}
	
	return NULL;
}

/** カーソル位置がスクロールボタン上か */

static mBool _judgePosScroll(mMenuWindow *p,int rooty)
{
	mPoint pt;

	if(p->menu.flags & MMENUWINDOW_F_HAVE_SCROLL)
	{
		pt.x = 0, pt.y = rooty;
		
		mWidgetMapPoint(NULL, M_WIDGET(p), &pt);
		
		if(pt.y < _FRAME_V + _SCROLL_H)
		{
			p->menu.flags &= ~MMENUWINDOW_F_SCROLL_DOWN;
			return TRUE;
		}
		else if(pt.y >= p->wg.h - _FRAME_V - _SCROLL_H)
		{
			p->menu.flags |= MMENUWINDOW_F_SCROLL_DOWN;
			return TRUE;
		}
	}
	
	return FALSE;
}

/** ボタン押し [top] */

static void _event_btt_press(mMenuWindow *topwin,mEvent *ev)
{
	mMenuWindow *win;
	
	win = _getPopupByPos(topwin, ev->pt.rootx, ev->pt.rooty, NULL);
	
	if(!win)
		//ポップアップ部分以外のクリックで終了
		_endPopup(topwin, NULL);
	else if(ev->pt.btt == M_BTT_LEFT)
	{
		//スクロール部分 -> スクロール開始
	
		if(_judgePosScroll(win, ev->pt.rooty))
		{
			mWidgetTimerAdd(M_WIDGET(topwin), _TIMERID_SCROLL, 60, 0);
			
			topwin->menu.winScroll = win;
		}
	}
}

/** ボタン離し [top] */

static void _event_btt_release(mMenuWindow *topwin,mEvent *ev)
{
	mMenuWindow *win;

	if(ev->pt.btt == M_BTT_LEFT)
	{
		if(topwin->menu.winScroll)
		{
			//スクロール中
			
			topwin->menu.winScroll = NULL;
			
			mWidgetTimerDelete(M_WIDGET(topwin), _TIMERID_SCROLL);
		}
		else
		{
			//項目決定 (離した時点でのアイテム)
			
			win = _getPopupByPos(topwin, ev->pt.rootx, ev->pt.rooty, NULL);
			
			if(win && win->menu.itemCur)
				_enterItem(topwin, win->menu.itemCur);
		}
	}
}

/** ポインタ移動 [top] */

static void _event_motion(mMenuWindow *topwin,mEvent *ev)
{
	mMenuWindow *win;
	mPoint pt;
	
	//スクロール中は除く
	
	if(topwin->menu.winScroll) return;
	
	//
	
	win = _getPopupByPos(topwin, ev->pt.rootx, ev->pt.rooty, &pt);
	
	if(win)
	{
		/* ポップアップ内
		 * - 無効なアイテムもカレントとして選択させる(表示上は選択されない) */
		
		topwin->menu.winFocus = win;
		
		_setCurrentItem(win, _getItemByPos(win, pt.y), FALSE);
	}
	else
	{
		/* ポップアップ外
		 * - ボタン押されている間でも選択解除する */
		
		_setCurrentItem(topwin->menu.winFocus, NULL, FALSE);
		
		/* メニューバーの別の項目の上に乗った場合は、一度終了させて新たに表示する */
		
		if(topwin->menu.menubar)
		{
			mMenuItem *item;
			
			item = __mMenuBar_motion(topwin->menu.menubar, topwin,
					ev->pt.x, ev->pt.y);
			
			if(item)
				_menubar_motion(topwin, item, 0);
		}
	}
}


//=============================
// ハンドラ
//=============================


/** イベント */

int _event_handle(mWidget *wg,mEvent *ev)
{
	mMenuWindow *menuwin = M_MENUWINDOW(wg);

	switch(ev->type)
	{
		//[!] 常にトップがグラブされているので、トップにしか来ない
		case MEVENT_POINTER:
			if(ev->pt.type == MEVENT_POINTER_TYPE_MOTION)
				_event_motion(menuwin, ev);
			else if(ev->pt.type == MEVENT_POINTER_TYPE_PRESS)
				_event_btt_press(menuwin, ev);
			else if(ev->pt.type == MEVENT_POINTER_TYPE_RELEASE)
				_event_btt_release(menuwin, ev);
			break;
	
		//キー押し [top]
		case MEVENT_KEYDOWN:
			if(ev->key.code == MKEY_ESCAPE)
				_endPopup(menuwin, NULL);
			else if(menuwin->menu.winFocus)
				_event_keydown(menuwin->menu.winFocus, ev->key.code);
			break;
		
		//タイマー [top]
		case MEVENT_TIMER:
			if(ev->timer.id == _TIMERID_SCROLL)
			{
				//スクロール中
				
				if(menuwin->menu.winScroll)
					_scroll(menuwin->menu.winScroll);
			}
			else if(ev->timer.id == _TIMERID_SUBMENU)
			{
				//サブメニュー表示
				
				mWidgetTimerDelete(wg, _TIMERID_SUBMENU);
				
				_showPopupSubmenu(M_MENUWINDOW(ev->timer.param));
			}
			break;
		
		//表示 (トップの場合、グラブ開始)
		case MEVENT_MAP:
			if(!menuwin->menu.winParent)
				mWidgetGrabPointer(wg);
			break;
		//グラブ解放時など
		case MEVENT_FOCUS:
			if(ev->focus.bOut)
				_endPopup(menuwin->menu.winTop, NULL);
			break;
	}

	return 1;
}

/** 描画 */

void _draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	mMenuWindow *menuwin;
	mFont *font;
	mMenuItem *item;
	int y,itemh;
	uint32_t col;
	mBool bsel,bscr,bDisable;
	mMenuItemDraw drawi;
	
	menuwin = M_MENUWINDOW(wg);
	font = mWidgetGetFont(wg);
	
	bscr = ((menuwin->menu.flags & MMENUWINDOW_F_HAVE_SCROLL) != 0);

	//枠
	
	mPixbufBox(pixbuf, 0, 0, wg->w, wg->h, MSYSCOL(MENU_FRAME));
	
	//背景
	
	mPixbufFillBox(pixbuf, 1, 1, wg->w - 2, wg->h - 2, MSYSCOL(MENU_FACE));
	
	//スクロール部分
	
	if(bscr)
	{
		mPixbufDrawArrowUp(pixbuf,
			wg->w >> 1, _FRAME_V + _SCROLL_H / 2,
			(menuwin->menu.scrY == 0)? MSYSCOL(MENU_TEXT_DISABLE): MSYSCOL(MENU_TEXT));

		mPixbufDrawArrowDown(pixbuf,
			wg->w >> 1, wg->h - _FRAME_V - _SCROLL_H + _SCROLL_H / 2,
			(menuwin->menu.scrY >= menuwin->menu.maxH - (wg->h - _FRAME_V * 2 - _SCROLL_H * 2))
				? MSYSCOL(MENU_TEXT_DISABLE): MSYSCOL(MENU_TEXT));
	}

	//--------- 項目
	
	y = _FRAME_V;
	
	if(bscr)
	{
		y += _SCROLL_H - menuwin->menu.scrY;
		
		//クリッピング
		mPixbufSetClipBox_d(pixbuf,
			0, _FRAME_V + _SCROLL_H,
			wg->w, wg->h - _FRAME_V * 2 - _SCROLL_H * 2);
	}
	
	//
	
	for(item = _MENUITEM_TOP(menuwin); item;
			y += item->item.height, item = M_MENUITEM(item->i.next))
	{
		itemh = item->item.height;
	
		if(bscr)
		{
			if(y + itemh <= _FRAME_V + _SCROLL_H) continue;
			if(y >= wg->h - _FRAME_V - _SCROLL_H) break;
		}
	
		bsel = (item == menuwin->menu.itemCur);
	
		if(_IS_ITEMFLAG(item, SEP))
		{
			//セパレータ
			
			mPixbufLineH(pixbuf,
				_FRAME_H + 1, y + itemh / 2,
				wg->w - _FRAME_H * 2 - 2, MSYSCOL(MENU_SEP));
		}
		else if(item->item.draw)
		{
			//独自描画
			
			drawi.flags = 0;
			if(bsel) drawi.flags |= MMENUITEM_DRAW_F_SELECTED;
			
			drawi.box.x = _FRAME_H;
			drawi.box.y = y;
			drawi.box.w = wg->w - _FRAME_H * 2;
			drawi.box.h = itemh;
			
			(item->item.draw)(pixbuf, &item->item, &drawi);
		}
		else if(item->labelsrc)
		{
			//------- 通常・ラベルあり

			bDisable = _IS_ITEMFLAG(item, DISABLE);
		
			//カーソル
			
			if(bsel && !bDisable)
			{
				mPixbufFillBox(pixbuf,
					_FRAME_H, y, wg->w - _FRAME_H * 2, itemh,
					MSYSCOL(MENU_SELECT));
			}
			
			//テキスト色 (pix)

			col = (bDisable)? MSYSCOL(MENU_TEXT_DISABLE): MSYSCOL(MENU_TEXT);
			
			//チェック
		
			if(_IS_ITEMFLAG(item, CHECKED))
			{
				if(_IS_ITEMFLAG(item, RADIO))
				{
					mPixbufDrawMenuRadio(pixbuf,
						_FRAME_H + 6, y + (itemh - 5) / 2, col);
				}
				else
				{
					mPixbufDrawMenuChecked(pixbuf,
						_FRAME_H + 5, y + (itemh - 7) / 2, col);
				}
			}

			//サブメニュー矢印
			
			if(item->item.submenu)
			{
				mPixbufDrawArrowRight(pixbuf,
					wg->w - _FRAME_H - 6, y + (itemh >> 1), col);
			}

			//ラベル
			
			mFontDrawTextHotkey(font, pixbuf,
				_FRAME_H + _LEFT_SPACE,
				y + ((itemh - font->height) >> 1),
				item->labelsrc, -1,
				(bDisable)? MSYSCOL_RGB(MENU_TEXT_DISABLE): MSYSCOL_RGB(MENU_TEXT));
			
			//ショートカットキー文字列
			
			if(item->sctext)
			{
				mFontDrawText(font, pixbuf,
					wg->w - _FRAME_H - _RIGHT_SPACE - item->labelRightW,
					y + ((itemh - font->height) >> 1),
					item->sctext, -1,
					(bsel && !bDisable)? MSYSCOL_RGB(MENU_TEXT): MSYSCOL_RGB(MENU_TEXT_SHORTCUT));
			}
			
		}
	}
}
