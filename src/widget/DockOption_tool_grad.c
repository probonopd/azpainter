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
 * [dock]オプションのタブ内容処理
 * 
 * <ツール - グラデーション>
 *****************************************/
/*
 * - コンボボックスの各アイテムの param には GradItem のポインタ。
 */

#include "mDef.h"
#include "mContainerDef.h"
#include "mWidget.h"
#include "mContainer.h"
#include "mEvent.h"
#include "mPixbuf.h"
#include "mComboBox.h"
#include "mCheckButton.h"
#include "mArrowButton.h"
#include "mMenu.h"
#include "mTrans.h"
#include "mUtilStr.h"
#include "mSysCol.h"

#include "defDraw.h"
#include "defPixelMode.h"
#include "macroToolOpt.h"
#include "GradationList.h"

#include "DockOption_sub.h"

#include "trgroup.h"


//------------------

typedef struct
{
	mWidget wg;
	mContainerData ct;
	
	mComboBox *cblist;
}_tab_grad;

//------------------

enum
{
	WID_CK_DRAWCOL_BKCOL = 100,
	WID_CK_CUSTOM,
	WID_CB_LIST,
	WID_BTT_LISTMENU,
	WID_BAR_OPACITY,
	WID_CB_PIXMODE,
	WID_CK_REVERSE,
	WID_CK_LOOP
};

enum
{
	TRID_DRAWCOL_BKCOL,
	TRID_CUSTOM,
	TRID_REVERSE,
	TRID_LOOP,

	TRID_MENU_EDIT = 100,
	TRID_MENU_NEW,
	TRID_MENU_COPY,
	TRID_MENU_DELETE
};

//------------------

mBool GradationEditDlg_run(mWindow *owner,GradItem *item);

//------------------



/** 選択のインデックスをセット
 *
 * @param index 負の値で現在の選択 */

static void _set_optgrad_selno(_tab_grad *p,int index)
{
	if(index < 0)
	{
		index = mComboBoxGetSelItemIndex(p->cblist);
		if(index == -1) index = 0;
	}

	GRAD_SET_SELNO(&APP_DRAW->tool.opt_grad, index);
}

/** コンボボックスアイテム描画 */

static void _combo_itemdraw(mPixbuf *pixbuf,mListViewItem *pi,mListViewItemDraw *pd)
{
	mBox *box = &pd->box;
	uint8_t *buf;
	char m[8];
	int n,x,y;

	//枠

	mPixbufBox(pixbuf, box->x + 3, box->y + 1, box->w - 6, box->h - 2, 0);

	//グラデーション

	buf = GRADITEM(pi->param)->buf;

	GradItem_draw(pixbuf, box->x + 4, box->y + 2, box->w - 8, box->h - 4,
		buf + 3, buf + 3 + buf[1] * 5, *buf & 2);

	//番号

	n = mIntToStr(m, mComboBoxGetItemIndex(M_COMBOBOX(pd->widget), pi) + 1);

	x = box->x + 5 + box->w - 8 - n * 5;
	y = box->y + box->h - 2 - 8;

	mPixbufFillBox(pixbuf, x - 1, y, n * 5, 8, 0);
	mPixbufDrawNumber_5x7(pixbuf, x, y + 1, m, MSYSCOL(WHITE));
}

/** 新規追加 */

static void _item_new(_tab_grad *p)
{
	GradItem *pi;
	int no;

	if((pi = GradationList_newItem()))
	{
		no = mComboBoxGetItemNum(p->cblist);

		mComboBoxAddItem_draw(p->cblist, NULL, (intptr_t)pi, _combo_itemdraw);
		mComboBoxSetSel_index(p->cblist, no);

		_set_optgrad_selno(p, no);
	}
}

/** 削除 */

static void _item_delete(_tab_grad *p)
{
	GradItem *pi;

	pi = (GradItem *)mComboBoxGetItemParam(p->cblist, -1);
	if(pi)
	{
		GradationList_delItem(pi);
	
		mComboBoxDeleteItem_sel(p->cblist);

		_set_optgrad_selno(p, -1);
	}
}

/** 複製 */

static void _item_copy(_tab_grad *p)
{
	GradItem *src,*pi;
	int no;

	src = (GradItem *)mComboBoxGetItemParam(p->cblist, -1);
	
	if((pi = GradationList_copyItem(src)))
	{
		no = mComboBoxGetItemNum(p->cblist);

		mComboBoxAddItem_draw(p->cblist, NULL, (intptr_t)pi, _combo_itemdraw);
		mComboBoxSetSel_index(p->cblist, no);

		_set_optgrad_selno(p, no);
	}
}

/** 編集 */

static void _item_edit(_tab_grad *p)
{
	GradItem *item;

	item = (GradItem *)mComboBoxGetItemParam(p->cblist, -1);

	if(GradationEditDlg_run(p->wg.toplevel, item))
	{
		//コンボボックス更新
		mWidgetUpdate(M_WIDGET(p->cblist));
	}
}

/** メニュー */

static void _menu_btt(_tab_grad *p,mWidget *bttwg)
{
	mMenu *menu;
	mMenuItemInfo *mi;
	int i;
	mBox box;
	uint16_t id[] = {
		TRID_MENU_EDIT, 0xfffe, TRID_MENU_NEW, TRID_MENU_COPY, TRID_MENU_DELETE,
		0xffff
	};

	//メニュー

	M_TR_G(TRGROUP_DOCKOPT_GRAD);

	menu = mMenuNew();

	mMenuAddTrArray16(menu, id);

	if(!mComboBoxGetSelItem(p->cblist))
	{
		mMenuSetEnable(menu, TRID_MENU_EDIT, 0);
		mMenuSetEnable(menu, TRID_MENU_COPY, 0);
		mMenuSetEnable(menu, TRID_MENU_DELETE, 0);
	}

	mWidgetGetRootBox(bttwg, &box);

	mi = mMenuPopup(menu, NULL, box.x + box.w, box.y + box.h, MMENU_POPUP_F_RIGHT);
	i = (mi)? mi->id: -1;

	mMenuDestroy(menu);

	//コマンド

	if(i == -1) return;

	switch(i)
	{
		//編集
		case TRID_MENU_EDIT:
			_item_edit(p);
			break;
		//新規追加
		case TRID_MENU_NEW:
			_item_new(p);
			break;
		//複製
		case TRID_MENU_COPY:
			_item_copy(p);
			break;
		//削除
		case TRID_MENU_DELETE:
			_item_delete(p);
			break;
	}
}


//=============================


/** イベント */

static int _ct_event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY)
	{
		uint32_t *pd = &APP_DRAW->tool.opt_grad;
	
		switch(ev->notify.id)
		{
			//描画色->背景色
			case WID_CK_DRAWCOL_BKCOL:
				GRAD_SET_CUSTOM_OFF(pd);
				break;
			//カスタム
			case WID_CK_CUSTOM:
				GRAD_SET_CUSTOM_ON(pd);
				break;
			//リスト選択
			case WID_CB_LIST:
				if(ev->notify.type == MCOMBOBOX_N_CHANGESEL)
					_set_optgrad_selno((_tab_grad *)wg, -1);
				break;
			//濃度
			case WID_BAR_OPACITY:
				if(ev->notify.param2)
					GRAD_SET_OPACITY(pd, ev->notify.param1);
				break;
			//塗り
			case WID_CB_PIXMODE:
				if(ev->notify.type == MCOMBOBOX_N_CHANGESEL)
					GRAD_SET_PIXMODE(pd, ev->notify.param2);
				break;
			//反転
			case WID_CK_REVERSE:
				GRAD_TOGGLE_REVERSE(pd);
				break;
			//繰り返し
			case WID_CK_LOOP:
				GRAD_TOGGLE_LOOP(pd);
				break;
			//メニューボタン
			case WID_BTT_LISTMENU:
				_menu_btt((_tab_grad *)wg, ev->notify.widgetFrom);
				break;
		}
	}

	return 1;
}

/** タブ内容作成 */

mWidget *DockOption_createTab_tool_grad(mWidget *parent)
{
	_tab_grad *p;
	mWidget *wg,*ct;
	GradItem *item;
	uint32_t val;
	uint8_t pixmode[] = {
		PIXELMODE_TOOL_BLEND, PIXELMODE_TOOL_COMPARE_A,
		PIXELMODE_TOOL_OVERWRITE, PIXELMODE_TOOL_ERASE, 255
	};

	val = APP_DRAW->tool.opt_grad;

	//メインコンテナ

	p = (_tab_grad *)DockOption_createMainContainer(
			sizeof(_tab_grad), parent, _ct_event_handle);

	M_TR_G(TRGROUP_DOCKOPT_GRAD);

	//------ 選択

	ct = mContainerCreate(M_WIDGET(p), MCONTAINER_TYPE_VERT, 0, 2, MLF_EXPAND_W);

	mCheckButtonCreate(ct, WID_CK_DRAWCOL_BKCOL,
		MCHECKBUTTON_S_RADIO, 0, 0, M_TR_T(TRID_DRAWCOL_BKCOL),
		GRAD_IS_NOT_CUSTOM(val));

	mCheckButtonCreate(ct, WID_CK_CUSTOM, MCHECKBUTTON_S_RADIO, 0, 0,
		M_TR_T(TRID_CUSTOM), GRAD_IS_CUSTOM(val));

	//------ カスタムリスト

	ct = mContainerCreate(M_WIDGET(p), MCONTAINER_TYPE_HORZ, 0, 6, MLF_EXPAND_W);
	ct->margin.bottom = 3;

	//リスト

	p->cblist = mComboBoxCreate(ct, WID_CB_LIST, 0, MLF_EXPAND_W, 0);
	p->cblist->wg.hintOverW = 50;
	
	mComboBoxSetItemHeight(p->cblist, 18);

	for(item = GradationList_getTopItem(); item; item = GRADITEM(item->i.next))
		mComboBoxAddItem_draw(p->cblist, NULL, (intptr_t)item, _combo_itemdraw);

	mComboBoxSetSel_index(p->cblist, GRAD_GET_SELNO(val));

	//メニューボタン

	wg = (mWidget *)mArrowButtonNew(0, ct, MARROWBUTTON_S_DOWN);
	wg->id = WID_BTT_LISTMENU;
	wg->fLayout = MLF_MIDDLE;

	//------ 濃度/塗り

	ct = mContainerCreateGrid(M_WIDGET(p), 2, 4, 5, MLF_EXPAND_W);

	//濃度

	DockOption_createDensityBar(ct, WID_BAR_OPACITY, GRAD_GET_OPACITY(val));

	//塗り

	DockOption_createPixelModeCombo(ct, WID_CB_PIXMODE, pixmode,
		GRAD_GET_PIXMODE(val));

	//------ チェック

	M_TR_G(TRGROUP_DOCKOPT_GRAD);

	ct = mContainerCreate(M_WIDGET(p), MCONTAINER_TYPE_HORZ, 0, 6, 0);

	mCheckButtonCreate(ct, WID_CK_REVERSE, 0, 0, 0, M_TR_T(TRID_REVERSE),
		GRAD_IS_REVERSE(val));

	mCheckButtonCreate(ct, WID_CK_LOOP, 0, 0, 0, M_TR_T(TRID_LOOP),
		GRAD_IS_LOOP(val));

	return (mWidget *)p;
}
