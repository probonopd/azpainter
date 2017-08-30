/*$
 Copyright (C) 2013-2017 Azel.

 This file is part of AzPainter.

 AzPainter is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 AzPainter is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
$*/

/*****************************************
 * DockFilterList
 *
 * [Dock] フィルタ一覧
 *****************************************/

#include "mDef.h"
#include "mContainerDef.h"
#include "mWidget.h"
#include "mDockWidget.h"
#include "mContainer.h"
#include "mListView.h"
#include "mEvent.h"
#include "mMenu.h"
#include "mAppDef.h"

#include "defWidgets.h"
#include "defConfig.h"
#include "DockObject.h"

#include "defMainWindow.h"
#include "trid_mainmenu.h"

#include "trgroup.h"


//-------------------

typedef struct
{
	DockObject obj;

	mListView *list;
}DockFilterList;

//-------------------

#define _DOCKFL(p)  ((DockFilterList *)(p))
#define _DFL_PTR    ((DockFilterList *)APP_WIDGETS->dockobj[DOCKWIDGET_FILTER_LIST])

//-------------------



//========================
// ハンドラ
//========================


/** イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY && ev->notify.id == 100
		&& ev->notify.param2 != -1)
	{
		int send;
		
		//通知タイプチェック

		if(APP_CONF->optflags & CONFIG_OPTF_FILTERLIST_DBLCLK)
		{
			//ダブルクリック時
			send = (ev->notify.type == MLISTVIEW_N_ITEM_DBLCLK);
		}
		else
		{
			//リスト上を左クリックした時 (フォーカスがすでにあっても有効)
			send = (ev->notify.type == MLISTVIEW_N_CHANGE_FOCUS || ev->notify.type == MLISTVIEW_N_CLICK_ON_FOCUS);
		}

		//コマンドイベント送る
	
		if(send)
		{
			mWidgetAppendEvent_command(M_WIDGET(APP_WIDGETS->mainwin),
				ev->notify.param2, 0, MEVENT_COMMAND_BY_MENU);
		}
	}

	return 1;
}

/** mDockWidget ハンドラ : 内容作成 */

static mWidget *_dock_func_create(mDockWidget *dockwg,int id,mWidget *parent)
{
	DockFilterList *p = _DFL_PTR;
	mWidget *ct;
	mListView *list;
	mMenu *menu;
	mMenuItem *pi,*pi2;
	mMenuItemInfo *info;

	//コンテナ

	ct = mContainerCreate(parent, MCONTAINER_TYPE_VERT, 0, 0, MLF_EXPAND_WH);

	ct->event = _event_handle;
	ct->notifyTarget = MWIDGET_NOTIFYTARGET_SELF;
	ct->param = (intptr_t)p;

	//リストビュー

	p->list = list = mListViewNew(0, ct, 0, MSCROLLVIEW_S_HORZVERT | MSCROLLVIEW_S_FRAME);

	list->wg.id = 100;
	list->wg.fLayout = MLF_EXPAND_WH;
	list->wg.font = MAPP->fontDefault;

	//項目リスト (メニューアイテムから)

	menu = mMenuGetSubMenu(APP_WIDGETS->mainwin->menu_main, TRMENU_TOP_FILTER);

	for(pi = mMenuGetTopItem(menu); pi; pi = mMenuGetNextItem(pi))
	{
		info = mMenuGetInfoInItem(pi);

		if(!info->label) continue;

		//サブメニューなしの項目、またはサブメニューのヘッダ

		mListViewAddItem(list, info->label, -1,
			MLISTVIEW_ITEM_F_STATIC_TEXT | ((info->submenu)? MLISTVIEW_ITEM_F_HEADER: 0),
			(info->submenu)? -1: info->id);

		//サブメニュー

		if(info->submenu)
		{
			for(pi2 = mMenuGetTopItem(info->submenu); pi2; pi2 = mMenuGetNextItem(pi2))
			{
				info = mMenuGetInfoInItem(pi2);

				if(info->label)
					mListViewAddItem(list, info->label, -1, MLISTVIEW_ITEM_F_STATIC_TEXT, info->id);
			}
		}
	}

	mListViewSetWidthAuto(list, FALSE);
	
	return ct;
}

/** 作成 */

void DockFilterList_new(mDockWidgetState *state)
{
	DockObject *p;

	p = DockObject_new(DOCKWIDGET_FILTER_LIST, sizeof(DockFilterList), 0, 0,
			state, _dock_func_create);

	mDockWidgetCreateWidget(p->dockwg);
}
