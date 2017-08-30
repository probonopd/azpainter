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
 * DockLayer
 *
 * [dock] レイヤ
 *****************************************/

#include "mDef.h"
#include "mWindowDef.h"
#include "mWidget.h"
#include "mContainer.h"
#include "mDockWidget.h"
#include "mIconButtons.h"
#include "mMenu.h"
#include "mEvent.h"
#include "mTrans.h"

#include "defWidgets.h"
#include "defDraw.h"
#include "defConfig.h"

#include "DockObject.h"
#include "AppResource.h"
#include "draw_layer.h"

#include "trgroup.h"
#include "trid_word.h"
#include "trid_mainmenu.h"


//------------------------

#define _DOCKLAYER(p)  ((DockLayer *)(p))
#define _DL_PTR        ((DockLayer *)APP_WIDGETS->dockobj[DOCKWIDGET_LAYER])

//------------------------

typedef struct _DockLayerArea DockLayerArea;

typedef struct
{
	DockObject obj;

	mWidget *param_ct;
	mIconButtons *ib;
	DockLayerArea *area;
}DockLayer;

//------------------------

/* DockLayer_param.c */

mWidget *DockLayer_parameter_new(mWidget *parent);
void DockLayer_parameter_setValue(mWidget *wg);

/* DockLayer_area.c */

DockLayerArea *DockLayerArea_new(mWidget *parent);
void DockLayerArea_drawLayer(DockLayerArea *p,LayerItem *pi);
void DockLayerArea_setScrollInfo(DockLayerArea *p);
void DockLayerArea_changeCurrent_visible(DockLayerArea *p,LayerItem *lastitem,mBool update_all);

//------------------------



/** ツールバー[新規レイヤ]のドロップダウンメニュー */

static void _run_new_dropdown_menu(DockLayer *p)
{
	mMenu *menu;
	mMenuItemInfo *pi;
	int type;
	mBox box;

	menu = mMenuNew();

	M_TR_G(TRGROUP_LAYERTYPE);

	mMenuAddText_static(menu, 100, M_TR_T2(TRGROUP_WORD, TRID_WORD_FOLDER));
	mMenuAddSep(menu);
	mMenuAddTrTexts(menu, 0, 4);

	mIconButtonsGetItemBox(p->ib, TRMENU_LAYER_NEW, &box, TRUE); 

	pi = mMenuPopup(menu, NULL, box.x, box.y + box.h, 0);

	type = (pi)? pi->id: -100;

	mMenuDestroy(menu);

	//

	if(type != -100)
		drawLayer_newLayer_direct(APP_DRAW, (type == 100)? -1: type);
}

/** イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_COMMAND)
	{
		if(ev->cmd.id == TRMENU_LAYER_NEW && ev->cmd.by == MEVENT_COMMAND_BY_ICONBUTTONS_DROP)
			//新規作成のドロップダウンメニュー
			_run_new_dropdown_menu(_DOCKLAYER(wg->param));
		else
			//メインウィンドウへ送る
			(((mWidget *)APP_WIDGETS->mainwin)->event)((mWidget *)APP_WIDGETS->mainwin, ev);
	}

	return 1;
}

/** mDockWidget ハンドラ : 内容作成 */

static mWidget *_dock_func_create(mDockWidget *dockwg,int id,mWidget *parent)
{
	DockLayer *p = _DL_PTR;
	mWidget *ct;
	mIconButtons *ib;
	int i;
	uint16_t bttid[] = {
		TRMENU_LAYER_NEW, TRMENU_LAYER_COPY, TRMENU_LAYER_ERASE,
		TRMENU_LAYER_DELETE, TRMENU_LAYER_COMBINE, TRMENU_LAYER_DROP,
		TRMENU_LAYER_TB_MOVE_UP, TRMENU_LAYER_TB_MOVE_DOWN
	};

	//コンテナ

	ct = mContainerCreate(parent, MCONTAINER_TYPE_VERT, 0, 0, MLF_EXPAND_WH);

	ct->event = _event_handle;
	ct->notifyTarget = MWIDGET_NOTIFYTARGET_SELF;
	ct->param = (intptr_t)p;

	//上部のパラメータ部分
	/* 起動中にパレットを破棄->作成した場合のためにパラメータセット。
	 * 起動時はカレントレイヤが NULL なのでその場合はセットしない。 */

	p->param_ct = DockLayer_parameter_new(ct);

	DockLayer_parameter_setValue(p->param_ct);

	//エリア (レイヤ名用に 12px フォントセット)

	p->area = DockLayerArea_new(ct);

	M_WIDGET(p->area)->font = APP_WIDGETS->font_dock12px;

	//ツールバー

	p->ib = ib = mIconButtonsNew(0, ct,
		MICONBUTTONS_S_TOOLTIP | MICONBUTTONS_S_DESTROY_IMAGELIST);

	ib->wg.fLayout = MLF_EXPAND_W;

	mIconButtonsSetImageList(ib, AppResource_loadIconImage("layer.png", APP_CONF->iconsize_layer));
	mIconButtonsSetTooltipTrGroup(ib, TRGROUP_DOCK_LAYER);

	for(i = 0; i < 8; i++)
	{
		mIconButtonsAdd(ib, bttid[i], i, i,
			(i == 0)? MICONBUTTONS_ITEMF_DROPDOWN: 0);
	}

	mIconButtonsCalcHintSize(ib);

	return ct;
}

/** 作成 */

void DockLayer_new(mDockWidgetState *state)
{
	DockObject *p;

	p = DockObject_new(DOCKWIDGET_LAYER,
			sizeof(DockLayer), MDOCKWIDGET_S_EXPAND, MWINDOW_S_ENABLE_DND,
			state, _dock_func_create);

	mDockWidgetCreateWidget(p->dockwg);
}


//===============================
// 外部からの呼び出し
//===============================


/** アイコンサイズ変更 */

void DockLayer_changeIconSize()
{
	DockLayer *p = _DL_PTR;

	if(DockObject_canDoWidget((DockObject *)p))
	{
		mIconButtonsReplaceImageList(p->ib,
			AppResource_loadIconImage("layer.png", APP_CONF->iconsize_layer));

		DockObject_relayout_inWindowMode(DOCKWIDGET_LAYER);
	}
}

/** すべて更新 */

void DockLayer_update_all()
{
	DockLayer *p = _DL_PTR;

	if(DockObject_canDoWidget((DockObject *)p))
	{
		DockLayer_parameter_setValue(p->param_ct);

		DockLayerArea_setScrollInfo(p->area);

		mWidgetUpdate(M_WIDGET(p->area));
	}
}

/** カレントレイヤの上部パラメータ更新 */

void DockLayer_update_curparam()
{
	DockLayer *p = _DL_PTR;

	if(DockObject_canDoWidget((DockObject *)p))
		DockLayer_parameter_setValue(p->param_ct);
}

/** カレントレイヤを更新 */

void DockLayer_update_curlayer(mBool setparam)
{
	DockLayer *p = _DL_PTR;

	if(DockObject_canDoWidget((DockObject *)p))
	{
		if(setparam)
			DockLayer_parameter_setValue(p->param_ct);

		DockLayerArea_drawLayer(p->area, APP_DRAW->curlayer);
	}
}

/** 指定レイヤのみ更新 (一覧部分のみ) */

void DockLayer_update_layer(LayerItem *item)
{
	DockLayer *p = _DL_PTR;

	if(item && DockObject_canDoWidget_visible((DockObject *)p))
		DockLayerArea_drawLayer(p->area, item);
}

/** 指定レイヤのみ更新
 *
 * 指定レイヤがカレントの場合は上部パラメータも更新 */

void DockLayer_update_layer_curparam(LayerItem *item)
{
	DockLayer *p = _DL_PTR;

	if(item)
	{
		if(DockObject_canDoWidget_visible((DockObject *)p))
			DockLayerArea_drawLayer(p->area, item);

		if(item == APP_DRAW->curlayer
			&& DockObject_canDoWidget((DockObject *)p))
			DockLayer_parameter_setValue(p->param_ct);
	}
}

/** カレント変更時の更新 (カレントが一覧上に見えるように)
 *
 * @param lastitem   カレント変更前のカレントレイヤ
 * @param update_all フォルダ展開などによりリスト全体を更新 */

void DockLayer_update_changecurrent_visible(LayerItem *lastitem,mBool update_all)
{
	DockLayer *p = _DL_PTR;

	if(DockObject_canDoWidget((DockObject *)p))
	{
		DockLayer_parameter_setValue(p->param_ct);

		if(update_all)
			DockLayerArea_setScrollInfo(p->area);

		DockLayerArea_changeCurrent_visible(p->area, lastitem, update_all);
	}
}

