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

/*********************************************
 * [dock] レイヤで使うウィジェット
 *
 * パラメータ部分、アルファマスク選択
 *********************************************/

#include "mDef.h"
#include "mContainerDef.h"
#include "mWidget.h"
#include "mContainer.h"
#include "mComboBox.h"
#include "mPixbuf.h"
#include "mSysCol.h"
#include "mEvent.h"
#include "mMenu.h"
#include "mTrans.h"

#include "defDraw.h"
#include "defBlendMode.h"

#include "ValueBar.h"
#include "SelMaterial.h"
#include "LayerItem.h"

#include "draw_main.h"
#include "draw_layer.h"
#include "Docks_external.h"

#include "trgroup.h"

#include "img_layer_amask.h"


//---------------------

enum
{
	WID_BAR_OPACITY = 100,
	WID_CB_BLENDMODE,
	WID_AMASK,
	WID_SEL_TEXTURE
};

enum
{
	TRID_AMASK_TOP = 100
};

//---------------------


/************************************
 * アルファマスク選択
 ************************************/


typedef struct
{
	mWidget wg;
	int curno;
}_alphamask;


/** メニュー */

static int _amask_run_menu(mWidget *wg)
{
	mMenu *menu;
	mMenuItemInfo *pi;
	int no;
	mBox box;

	menu = mMenuNew();

	M_TR_G(TRGROUP_DOCK_LAYER);

	mMenuAddTrTexts(menu, TRID_AMASK_TOP, 4);
	mMenuSetCheck(menu, TRID_AMASK_TOP + APP_DRAW->curlayer->alphamask, 1);

	mWidgetGetRootBox(wg, &box);

	pi = mMenuPopup(menu, NULL, box.x, box.y + box.h, 0);

	no = (pi)? pi->id - TRID_AMASK_TOP: -1;

	mMenuDestroy(menu);

	return no;
}

/** イベント */

static int _amask_event_handle(mWidget *wg,mEvent *ev)
{
	int amask;

	if(ev->type == MEVENT_POINTER
		&& ev->pt.type == MEVENT_POINTER_TYPE_PRESS)
	{
		if(ev->pt.btt == M_BTT_RIGHT
			|| (ev->pt.btt == M_BTT_LEFT && (ev->pt.state & M_MODS_CTRL)))
			//右クリック or Ctrl+左 => OFF
			amask = 0;
		else if(ev->pt.btt == M_BTT_LEFT)
			//左クリック => メニュー
			amask = _amask_run_menu(wg);
		else
			amask = -1;

		//選択変更

		if(amask != -1)
		{
			APP_DRAW->curlayer->alphamask = amask;

			((_alphamask *)wg)->curno = amask;

			mWidgetUpdate(wg);
		}
	}

	return 1;
}

/** 描画 */

static void _amask_draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	if(wg->fState & MWIDGET_STATE_ENABLED)
	{
		//有効
		mPixbufBltFromGray(pixbuf, 0, 0, g_img_layer_amask,
			((_alphamask *)wg)->curno * 16, 0, 16, 16, 16 * 4);
	}
	else
	{
		//無効
		mPixbufBox(pixbuf, 0, 0, 16, 16, MSYSCOL(FRAME_LIGHT));
		mPixbufFillBox(pixbuf, 1, 1, 14, 14, MSYSCOL(FACE));
	}
}

/** 作成 */

static _alphamask *_alphamask_new(mWidget *parent)
{
	_alphamask *p;

	p = (_alphamask *)mWidgetNew(sizeof(_alphamask), parent);
	if(!p) return NULL;

	p->wg.id = WID_AMASK;
	p->wg.hintW = p->wg.hintH = 16;
	p->wg.fLayout = MLF_MIDDLE;
	p->wg.fEventFilter |= MWIDGET_EVENTFILTER_POINTER;
	p->wg.draw = _amask_draw_handle;
	p->wg.event = _amask_event_handle;

	return p;
}


/**************************************
 * パラメータ部分のコンテナ
 **************************************/


typedef struct
{
	mWidget wg;
	mContainerData ct;

	SelMaterial *selmat;
	ValueBar *bar_opacity;
	mComboBox *cb_blend;
	_alphamask *amask;
}_paramct;



/** イベント */

static int _paramct_event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY)
	{
		switch(ev->notify.id)
		{
			//不透明度
			case WID_BAR_OPACITY:
				if(ev->notify.param2)
				{
					APP_DRAW->curlayer->opacity = (int)(ev->notify.param1 * 128.0 / 100.0 + 0.5);
				
					drawUpdate_rect_imgcanvas_canvasview_inLayerHave(APP_DRAW, NULL);
					DockLayer_update_curlayer(FALSE);
				}
				break;
			//合成モード
			case WID_CB_BLENDMODE:
				if(ev->notify.type == MCOMBOBOX_N_CHANGESEL)
				{
					APP_DRAW->curlayer->blendmode = ev->notify.param2;

					DockLayer_update_curlayer(FALSE);
					drawUpdate_rect_imgcanvas_canvasview_inLayerHave(APP_DRAW, NULL);
				}
				break;
			//テクスチャ
			case WID_SEL_TEXTURE:
				drawLayer_setTexture(APP_DRAW, (const char *)ev->notify.param1);
				break;
		}
	}

	return 1;
}

/** 作成 */

mWidget *DockLayer_parameter_new(mWidget *parent)
{
	_paramct *p;
	mWidget *ct;

	//作成

	p = (_paramct *)mContainerNew(sizeof(_paramct), parent);
	if(!p) return NULL;

	mContainerSetPadding_b4(M_CONTAINER(p), M_MAKE_DW4(3,5,3,5));

	p->wg.fLayout = MLF_EXPAND_W;
	p->wg.notifyTarget = MWIDGET_NOTIFYTARGET_SELF;
	p->wg.event = _paramct_event_handle;
	p->ct.sepW = 5;

	//[合成モード + アルファマスク]

	ct = mContainerCreate(M_WIDGET(p), MCONTAINER_TYPE_HORZ, 0, 6, MLF_EXPAND_W);

	//合成モード
	
	p->cb_blend = mComboBoxCreate(ct, WID_CB_BLENDMODE, 0, MLF_EXPAND_W, 0);

	M_TR_G(TRGROUP_BLENDMODE);
	mComboBoxAddTrItems(p->cb_blend, BLENDMODE_NUM, 0, 0);

	//アルファマスク

	p->amask = _alphamask_new(ct);

	//[濃度]

	p->bar_opacity = ValueBar_new(M_WIDGET(p), WID_BAR_OPACITY, MLF_EXPAND_W,
		0, 0, 100, 100);

	//[テクスチャ]

	p->selmat = SelMaterial_new(M_WIDGET(p), WID_SEL_TEXTURE, SELMATERIAL_TYPE_TEXTURE);

	return (mWidget *)p;
}

/** カレントレイヤの値をセット */

void DockLayer_parameter_setValue(mWidget *wg)
{
	_paramct *p = (_paramct *)wg;
	LayerItem *pi = APP_DRAW->curlayer;
	mBool bNormal;

	//初期作成時は除く
	if(!pi) return;

	bNormal = LAYERITEM_IS_IMAGE(pi);

	//不透明度

	ValueBar_setPos(p->bar_opacity, (int)(pi->opacity * 100 / 128.0 + 0.5));

	//合成モード

	mComboBoxSetSel_index(p->cb_blend, pi->blendmode);
	mWidgetEnable(M_WIDGET(p->cb_blend), bNormal);

	//Aマスク

	p->amask->curno = pi->alphamask;

	mWidgetEnable(M_WIDGET(p->amask), bNormal);
	mWidgetUpdate(M_WIDGET(p->amask));

	//テクスチャ

	SelMaterial_setName(p->selmat, pi->texture_path);
	mWidgetEnable(M_WIDGET(p->selmat), bNormal);
}
