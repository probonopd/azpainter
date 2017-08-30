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
 * パネル配置設定ダイアログ
 *****************************************/

#include <string.h>

#include "mDef.h"
#include "mWidget.h"
#include "mContainer.h"
#include "mDialog.h"
#include "mWindow.h"
#include "mListView.h"
#include "mCheckButton.h"
#include "mArrowButton.h"
#include "mTrans.h"
#include "mEvent.h"
#include "mStr.h"

#include "defConfig.h"
#include "defWidgets.h"

#include "DockObject.h"

#include "trgroup.h"


//----------------------

typedef struct
{
	mWidget wg;
	mContainerData ct;
	mWindowData win;
	mDialogData dlg;

	mListView *list;
	mWidget *btt[4];

	int type;
	char pane_buf[4],
		panel_buf[DOCKWIDGET_NUM + 3];
	const char *text_canvas,
		*text_pane_name;
}_dialog;

//----------------------

enum
{
	WID_LIST = 100,
	WID_CK_PANEL,
	WID_CK_PANE,
	WID_BTT_UP,
	WID_BTT_DOWN,
	WID_BTT_LEFT,
	WID_BTT_RIGHT
};

enum
{
	TRID_TITLE,
	TRID_PANEL,
	TRID_PANE,
	TRID_PANE_NAME,
	TRID_CANVAS
};

//----------------------



/** ペインのリストセット */

static void _setlist_pane(_dialog *p)
{
	mStr str = MSTR_INIT;
	int i,no;

	for(i = 0; i < 4; i++)
	{
		no = p->pane_buf[i];
		
		if(no == 0)
			mStrSetText(&str, p->text_canvas);
		else
			mStrSetFormat(&str, p->text_pane_name, no);
		
		mListViewAddItem(p->list, str.buf, -1, 0, no);
	}

	mStrFree(&str);
}

/** ペインのヘッダアイテム追加
 *
 * param は ペイン番号 x -1 */

static void _additem_pane_header(_dialog *p,int no,mStr *str)
{
	mStrSetFormat(str, p->text_pane_name, no);
	mListViewAddItem(p->list, str->buf, -1, MLISTVIEW_ITEM_F_HEADER, -no);
}

/** パネルのリストセット */

static void _setlist_panel(_dialog *p)
{
	mListView *list = p->list;
	char *pc;
	mStr str = MSTR_INIT;
	int no,paneno = 2;

	_additem_pane_header(p, 1, &str);

	for(pc = p->panel_buf; *pc; pc++)
	{
		if(*pc == ':')
			_additem_pane_header(p, paneno++, &str);
		else
		{
			no = DockObject_getDockNo_fromID(*pc);
			
			mListViewAddItem(list, M_TR_T(no), -1, MLISTVIEW_ITEM_F_STATIC_TEXT, *pc);
		}
	}

	mStrFree(&str);
}

/** タイプの変更 */

static void _change_type(_dialog *p)
{
	int f;

	//リスト
	
	mListViewDeleteAllItem(p->list);

	if(p->type == 0)
		_setlist_panel(p);
	else
		_setlist_pane(p);

	//ボタン有効/無効

	f = (p->type == 0);

	mWidgetEnable(p->btt[2], f);
	mWidgetEnable(p->btt[3], f);
}

/** リストから作業用データにセット */

static void _list_to_workbuf(_dialog *p)
{
	mListViewItem *pi;
	char *pd;
	int n;

	if(p->type == 0)
	{
		//パネル

		pd = p->panel_buf;

		for(pi = mListViewGetTopItem(p->list); pi; pi = M_LISTVIEWITEM(pi->i.next))
		{
			n = pi->param;
			
			if(n < 0)
			{
				//パネル1のヘッダはスキップ
				
				if(n != -1)
					*(pd++) = ':';
			}
			else
				*(pd++) = n;
		}

		*pd = 0;
	}
	else
	{
		//ペイン

		pd = p->pane_buf;

		for(pi = mListViewGetTopItem(p->list); pi; pi = M_LISTVIEWITEM(pi->i.next))
			*(pd++) = pi->param;
	}
}

/** アイテム移動 */

static void _move_item(_dialog *p,int dir)
{
	mListViewItem *item,*pi;

	item = mListViewGetFocusItem(p->list);
	if(!item) return;

	//パネルのヘッダ

	if(item->param < 0) return;

	//

	if(dir < 2)
	{
		//----- 上下

		//パネル時、上限はペイン１ヘッダの下まで

		if(p->type == 0
			&& dir == 0 && item->i.prev
			&& M_LISTVIEWITEM(item->i.prev)->param == -1)
			return;

		mListViewMoveItem_updown(p->list, item, (dir == 1));
	}
	else if(dir == 2)
	{
		//上のペインの一番下へ移動

		for(pi = M_LISTVIEWITEM(item->i.prev); pi; pi = M_LISTVIEWITEM(pi->i.prev))
		{
			if(pi->param < -1) break;
		}

		if(pi)
			mListViewMoveItem(p->list, item, pi);
	}
	else
	{
		//下のペインの１番下へ移動

		for(pi = M_LISTVIEWITEM(item->i.next); pi; pi = M_LISTVIEWITEM(pi->i.next))
		{
			if(pi->param < -1) break;
		}

		if(pi)
		{
			for(pi = M_LISTVIEWITEM(pi->i.next); pi; pi = M_LISTVIEWITEM(pi->i.next))
			{
				if(pi->param < -1) break;
			}

			mListViewMoveItem(p->list, item, pi);
		}
	}
}

/** 設定データから作業用データセット */

static void _set_workbuf(_dialog *p)
{
	char *pc,*dst;

	//ペイン

	dst = p->pane_buf;

	for(pc = APP_CONF->pane_layout; *pc; pc++)
		*(dst++) = *pc - '0';

	//パネル

	strcpy(p->panel_buf, APP_CONF->dock_layout);
}

/** OK */

static void _set_config(_dialog *p)
{
	int i;
	char *pc;

	_list_to_workbuf(p);

	//ペイン

	pc = APP_CONF->pane_layout;

	for(i = 0; i < 4; i++)
		*(pc++) = p->pane_buf[i] + '0';

	*pc = 0;

	//パネル

	strcpy(APP_CONF->dock_layout, p->panel_buf);
}


//==========================


/** イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY)
	{
		_dialog *p = (_dialog *)wg;
		
		switch(ev->notify.id)
		{
			//タイプ
			case WID_CK_PANEL:
			case WID_CK_PANE:
				_list_to_workbuf(p);
				
				p->type = ev->notify.id - WID_CK_PANEL;

				_change_type(p);
				break;
			//移動ボタン
			case WID_BTT_UP:
			case WID_BTT_DOWN:
			case WID_BTT_LEFT:
			case WID_BTT_RIGHT:
				_move_item(p, ev->notify.id - WID_BTT_UP);
				break;
		
			//OK
			case M_WID_OK:
				_set_config(p);
				mDialogEnd(M_DIALOG(wg), TRUE);
				break;
			//キャンセル
			case M_WID_CANCEL:
				mDialogEnd(M_DIALOG(wg), FALSE);
				break;
		}
	}

	return mDialogEventHandle(wg, ev);
}

/** 作成 */

static _dialog *_create_dlg(mWindow *owner)
{
	_dialog *p;
	mWidget *ct,*ct2;
	mListView *list;
	int i;
	int bttstyle[4] = { MARROWBUTTON_S_UP, MARROWBUTTON_S_DOWN, MARROWBUTTON_S_LEFT, MARROWBUTTON_S_RIGHT };

	p = (_dialog *)mDialogNew(sizeof(_dialog), owner, MWINDOW_S_DIALOG_NORMAL);
	if(!p) return NULL;

	p->wg.event = _event_handle;

	//データ

	M_TR_G(TRGROUP_DLG_PANEL_LAYOUT_OPT);

	p->text_canvas = M_TR_T(TRID_CANVAS);
	p->text_pane_name = M_TR_T(TRID_PANE_NAME);

	_set_workbuf(p);

	//

	mContainerSetPadding_one(M_CONTAINER(p), 10);

	p->ct.sepW = 10;

	mWindowSetTitle(M_WINDOW(p), M_TR_T(TRID_TITLE));

	//------ ウィジェット

	//タイプ

	ct = mContainerCreate(M_WIDGET(p), MCONTAINER_TYPE_HORZ, 0, 3, 0);

	for(i = 0; i < 2; i++)
	{
		mCheckButtonCreate(ct, WID_CK_PANEL + i, MCHECKBUTTON_S_RADIO, 0,
			0, M_TR_T(TRID_PANEL + i), (i == 0));
	}
	
	//リスト

	ct = mContainerCreate(M_WIDGET(p), MCONTAINER_TYPE_HORZ, 0, 8, MLF_EXPAND_WH);

	list = p->list = mListViewNew(0, ct, 0, MSCROLLVIEW_S_VERT | MSCROLLVIEW_S_FRAME);

	list->wg.id = WID_LIST;
	list->wg.fLayout = MLF_EXPAND_WH;

	mWidgetSetInitSize_fontHeight(M_WIDGET(list), 20, 18);

	//ボタン

	ct2 = mContainerCreate(ct, MCONTAINER_TYPE_VERT, 0, 4, MLF_EXPAND_Y | MLF_BOTTOM);

	for(i = 0; i < 4; i++)
	{
		p->btt[i] = (mWidget *)mArrowButtonCreate(ct2, WID_BTT_UP + i,
			bttstyle[i] | MARROWBUTTON_S_FONTSIZE, 0, (i == 2)? M_MAKE_DW4(0,6,0,0): 0);
	}

	//OK/cancel

	ct = mContainerCreateOkCancelButton(M_WIDGET(p));
	ct->margin.top = 8;

	//パネル名のグループにしておく

	M_TR_G(TRGROUP_DOCK_NAME);

	//

	_change_type(p);

	return p;
}

/** ダイアログ実行 */

mBool PanelLayoutDlg_run(mWindow *owner)
{
	_dialog *p;

	p = _create_dlg(owner);
	if(!p) return FALSE;

	mWindowMoveResizeShow_hintSize(M_WINDOW(p));

	return mDialogRun(M_DIALOG(p), TRUE);
}
