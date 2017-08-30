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
 * mListViewPopup
 *****************************************/

#include "mDef.h"

#include "mPopupWindow.h"
#include "mLVItemMan.h"

#include "mListViewPopup.h"

#include "mListView.h"
#include "mListViewArea.h"
#include "mScrollBar.h"

#include "mWidget.h"
#include "mEvent.h"
#include "mKeyDef.h"


//-----------------

#define M_LISTVIEW_POPUP(p)  ((mListViewPopup *)(p))

typedef struct
{
	mLVItemMan *manager;
	mListViewItem *itemSelBegin;
	mListViewArea *area;
}mListViewPopupData;

typedef struct
{
	mWidget wg;
	mContainerData ct;
	mWindowData win;
	mPopupWindowData pop;
	mListViewPopupData lvpop;
}mListViewPopup;

//-----------------

static int _event_handle(mWidget *wg,mEvent *ev);

//-----------------


//======================
// sub
//======================


/** area のサイズ変更時 */

static void _area_onsize_handle(mWidget *wg)
{
	mListViewArea *p = M_LISTVIEWAREA(wg);
	mScrollBar *scr;

	//垂直スクロールバー

	scr = mScrollViewAreaGetScrollBar(M_SCROLLVIEWAREA(p), TRUE);

	if(scr)
		mScrollBarSetStatus(scr, 0, p->lva.itemH * p->lva.manager->list.num, wg->h);
}

/** ポップアップ終了ハンドラ */

mBool _popup_quit(mPopupWindow *p,mBool cancel)
{
	//キャンセル時は元の選択に戻す

	if(cancel)
	{
		mLVItemMan_setFocusItem(M_LISTVIEW_POPUP(p)->lvpop.manager,
			M_LISTVIEW_POPUP(p)->lvpop.itemSelBegin);
	}

	return TRUE;
}

/** ビューのイベントハンドラ
 *
 * キーイベントはフォーカスウィジェットを対象に来るので、ここで処理 */

static int _view_event_handle(mWidget *wg,mEvent *ev)
{
	mScrollView *p = M_SCROLLVIEW(wg);

	if(ev->type == MEVENT_KEYDOWN)
	{
		if(ev->key.code == MKEY_SPACE)
			//SPACE で決定
			mPopupWindowQuit(M_POPUPWINDOW(wg->parent), FALSE);
		else
			//ほかは area に渡す
			((p->sv.area)->wg.event)(M_WIDGET(p->sv.area), ev);
	}

	return TRUE;
}


//======================
//
//======================


/** ポップアップ作成
 *
 * @param callwg 呼び出し元のウィジェット
 * @param w 枠を含む幅
 * @param h 枠を含まない高さ */

static mListViewPopup *_popup_new(mWidget *callwg,mLVItemMan *manager,
	uint32_t style,int w,int h,int itemh)
{
	mListViewPopup *p;
	mScrollView *view;
	mListViewArea *area;

	//mListViewPopup 作成
	
	p = (mListViewPopup *)mPopupWindowNew(sizeof(mListViewPopup), NULL,
			MWINDOW_S_NO_IM);
	if(!p) return NULL;

	p->wg.event = _event_handle;
	p->wg.font = mWidgetGetFont(callwg);  //フォントは呼び出し元と同じに
	p->win.owner = callwg->toplevel;      //フォーカスアウトに対応するためセット

	p->pop.quit = _popup_quit;

	p->lvpop.manager = manager;
	p->lvpop.itemSelBegin = manager->itemFocus;

	//スクロールビュー

	view = mScrollViewNew(0, M_WIDGET(p),
		MSCROLLVIEW_S_FRAME
			| ((style & MLISTVIEWPOPUP_S_VERTSCR)? MSCROLLVIEW_S_VERT: 0));

	view->wg.fState |= MWIDGET_STATE_TAKE_FOCUS;
	view->wg.fLayout = MLF_EXPAND_WH;
	view->wg.fEventFilter |= MWIDGET_EVENTFILTER_KEY;
	view->wg.event = _view_event_handle;

	//リストビューエリア

	area = mListViewAreaNew(0, M_WIDGET(view),
			MLISTVIEWAREA_S_POPUP, 0,
			manager, callwg);

	area->wg.onSize = _area_onsize_handle;
	area->lva.itemH = itemh;

	//

	view->sv.area = M_SCROLLVIEWAREA(area);

	p->lvpop.area = area;

	//サイズ (高さは枠分を足す)

	mWidgetResize(M_WIDGET(p), w, h + 2);

	//選択アイテムが表示されるようにスクロール

	mListViewArea_scrollToFocus(area, 0, 0);

	//フォーカスセット

	mWidgetSetFocus(M_WIDGET(view));

	return p;
}

/** ポップアップ実行 */

void mListViewPopupRun(mWidget *callwg,mLVItemMan *manager,
	uint32_t style,int x,int y,int w,int h,int itemh)
{
	mListViewPopup *pop;

	pop = _popup_new(callwg, manager, style, w, h, itemh);
	if(!pop) return;

	mPopupWindowRun(M_POPUPWINDOW(pop), x, y);

	mWidgetDestroy(M_WIDGET(pop));
}

/** イベント */

int _event_handle(mWidget *wg,mEvent *ev)
{
	mListViewPopup *p = M_LISTVIEW_POPUP(wg);

	switch(ev->type)
	{
		case MEVENT_NOTIFY:
			//エリアから終了通知が来た時
			if(ev->notify.widgetFrom == (p->lvpop.area)->wg.parent
				&& ev->notify.type == MLISTVIEWAREA_N_POPUPEND)
				mPopupWindowQuit(M_POPUPWINDOW(p), FALSE);
			break;

		case MEVENT_KEYDOWN:
			//ENTER で決定
			if(ev->key.code == MKEY_ENTER)
				mPopupWindowQuit(M_POPUPWINDOW(p), FALSE);
			break;

		//フォーカスアウト
		case MEVENT_FOCUS:
			if(ev->focus.bOut)
				mPopupWindowQuit(M_POPUPWINDOW(p), TRUE);
			break;
	}

	return mPopupWindowEventHandle(wg, ev);
}
