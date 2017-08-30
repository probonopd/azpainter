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
 * DockColor
 *
 * [dock] カラー
 *****************************************/

#include "mDef.h"
#include "mContainerDef.h"
#include "mWidget.h"
#include "mContainer.h"
#include "mTab.h"
#include "mDockWidget.h"
#include "mEvent.h"

#include "defWidgets.h"
#include "defConfig.h"
#include "defDraw.h"

#include "DockObject.h"
#include "Docks_external.h"


//------------------------

#define _DOCKCOLOR(p)  ((DockColor *)(p))
#define _DC_PTR        ((DockColor *)APP_WIDGETS->dockobj[DOCKWIDGET_COLOR]);

typedef void (*_changecol_func)(mWidget *,int);

//------------------------

typedef struct
{
	DockObject obj;

	mWidget *wg_drawcol,	//描画色
		*wg_colmask,		//色マスク
		*tab_parent,		//タブの親
		*contents;	//タブ内容のメインコンテナ
					/* mWidget::param に描画色変更時の関数ポインタがセットされている */
	mTab *tab;
}DockColor;

//------------------------

enum
{
	WID_DRAWCOL = 100,
	WID_COLMASK,
	WID_TAB,
	WID_TAB_CONTENTS
};

enum
{
	TABNO_RGB,
	TABNO_HSV,
	TABNO_HSV_MAP
};

enum
{
	_UPDATE_F_DRAWCOL = 1<<0,
	_UPDATE_F_TAB_CONTENTS = 1<<1
};

//------------------------

/* DockColor_widget.c */
mWidget *DockColorDrawCol_new(mWidget *parent,int id);
mWidget *DockColorColorMask_new(mWidget *parent,int id);

/* DockColor_tab.c */
mWidget *DockColor_tabRGB_create(mWidget *parent,int id);
mWidget *DockColor_tabHSV_create(mWidget *parent,int id);
mWidget *DockColor_tabHSVMap_create(mWidget *parent,int id);
void DockColor_tabHSVMap_changeTheme(mWidget *wg);

//------------------------



//===========================
// sub
//===========================


/** タブ内容作成 */

static void _create_tab_contents(DockColor *p,mBool relayout)
{
	mWidget *(*func[3])(mWidget *,int) = {
		DockColor_tabRGB_create, DockColor_tabHSV_create, DockColor_tabHSVMap_create
	};

	//現在のウィジェット削除

	if(p->contents)
		mWidgetDestroy(p->contents);

	//作成

	p->contents = (func[APP_CONF->dockcolor_tabno])(p->tab_parent, WID_TAB_CONTENTS);

	//内容をタブの左へ移動

	mWidgetMoveTree_first(p->contents);

	//ドッキング状態なら、フォーカスを受け取らないようにする

	if(!DockObject_canTakeFocus((DockObject *)p))
		mWidgetNoTakeFocus_under(p->contents);

	//再レイアウト

	if(relayout)
		mWidgetReLayout(p->tab_parent);
}

/** 色変更時 */

static void _change_color(DockColor *p,uint8_t flags,int hsvcol)
{
	//描画色

	if(flags & _UPDATE_F_DRAWCOL)
		mWidgetUpdate(p->wg_drawcol);

	//タブ内容更新

	if(flags & _UPDATE_F_TAB_CONTENTS)
		((_changecol_func)p->contents->param)(p->contents, -1);

	//カラーホイール

	DockColorWheel_changeDrawColor(hsvcol);
}

/** イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	DockColor *p = _DOCKCOLOR(wg->param);

	if(ev->type == MEVENT_NOTIFY)
	{
		switch(ev->notify.id)
		{
			//タブ内容からの通知 (バーなどでの色変更時)
			//param1: 負の値でRGB描画色、それ以外でHSV値
			case WID_TAB_CONTENTS:
				_change_color(p, _UPDATE_F_DRAWCOL, ev->notify.param1);
				break;

			//描画色/背景色入れ替え
			case WID_DRAWCOL:
				_change_color(p, _UPDATE_F_TAB_CONTENTS, -1);
				break;

			//色マスクからのスポイト時
			case WID_COLMASK:
				_change_color(p, _UPDATE_F_DRAWCOL | _UPDATE_F_TAB_CONTENTS, -1);
				break;

			//タブ切り替え
			case WID_TAB:
				if(ev->notify.type == MTAB_N_CHANGESEL)
				{
					APP_CONF->dockcolor_tabno = ev->notify.param1;

					_create_tab_contents(p, TRUE);
				}
				break;
		}
	}

	return 1;
}


//===========================
// 作成
//===========================


/** メインコンテナの破棄ハンドラ */

static void _mainct_destroy_handle(mWidget *wg)
{
	DockColor *p = _DC_PTR;

	/* ヘッダの閉じるボタンなどで閉じて破棄された場合、タブ内容のポインタをクリア。
	 *
	 * [!] アプリケーション終了時は、このハンドラが呼ばれる前に DockObject が削除される。
	 * 解放されたポインタにアクセスするのを防ぐため、DockObject 削除後は
	 * APP_WIDGETS->dockobj[..] の値は NULL にしている。 */

	if(p) p->contents = NULL;
}

/** mDockWidget ハンドラ : 内容作成 */

static mWidget *_dock_func_contents(mDockWidget *dockwg,int id,mWidget *parent)
{
	DockColor *p = _DC_PTR;
	mWidget *ct_top,*ct;
	int i;
	const char *tabname[] = {"RGB", "HSV", "MAP"};

	//コンテナ (トップ)

	ct_top = mContainerCreate(parent, MCONTAINER_TYPE_VERT, 0, 0, MLF_EXPAND_WH);

	ct_top->destroy = _mainct_destroy_handle;
	ct_top->event = _event_handle;
	ct_top->notifyTarget = MWIDGET_NOTIFYTARGET_SELF;
	ct_top->param = (intptr_t)p;
	ct_top->margin.top = ct_top->margin.bottom = 4;

	//------ 上部

	ct = mContainerCreate(ct_top, MCONTAINER_TYPE_HORZ, 0, 15, MLF_EXPAND_W);

	ct->margin.bottom = 9;
	ct->margin.left = ct->margin.right = 2;

	p->wg_drawcol = DockColorDrawCol_new(ct, WID_DRAWCOL);
	p->wg_colmask = DockColorColorMask_new(ct, WID_COLMASK);

	//------ タブ

	p->tab_parent = ct = mContainerCreate(ct_top, MCONTAINER_TYPE_HORZ, 0, 7, MLF_EXPAND_WH);

	ct->margin.left = ct->margin.right = 2;

	//タブ

	p->tab = mTabCreate(ct, WID_TAB, MTAB_S_RIGHT | MTAB_S_HAVE_SEP, MLF_EXPAND_H, 0);

	for(i = 0; i < 3; i++)
		mTabAddItemText(p->tab, tabname[i]);

	mTabSetSel_index(p->tab, APP_CONF->dockcolor_tabno);

	_create_tab_contents(p, FALSE);

	return ct_top;
}

/** 作成 */

void DockColor_new(mDockWidgetState *state)
{
	DockObject *p;

	p = DockObject_new(DOCKWIDGET_COLOR, sizeof(DockColor), 0, 0,
			state, _dock_func_contents);

	mDockWidgetCreateWidget(p->dockwg);
}


//===========================
// 外部からの呼び出し
//===========================


/** 描画色/背景色 変更時 (RGB) */

void DockColor_changeDrawColor()
{
	DockColor *p = _DC_PTR;

	if(DockObject_canDoWidget((DockObject *)p))
	{
		//描画色/背景色

		mWidgetUpdate(p->wg_drawcol);

		//タブ内容

		((_changecol_func)p->contents->param)(p->contents, -1);
	}

	//カラーホイール

	DockColorWheel_changeDrawColor(-1);
}

/** 描画色変更時 (HSV)
 *
 * カラーホイールからの変更時 */

void DockColor_changeDrawColor_hsv(int col)
{
	DockColor *p = _DC_PTR;

	if(DockObject_canDoWidget((DockObject *)p))
	{
		//描画色/背景色

		mWidgetUpdate(p->wg_drawcol);

		//タブ内容

		((_changecol_func)p->contents->param)(p->contents, col);
	}
}

/** カラーマスク変更時 */

void DockColor_changeColorMask()
{
	DockColor *p = _DC_PTR;

	if(DockObject_canDoWidget_visible((DockObject *)p))
		mWidgetUpdate(p->wg_colmask);
}

/** テーマ変更時 */

void DockColor_changeTheme()
{
	DockColor *p = _DC_PTR;

	if(DockObject_canDoWidget((DockObject *)p)
		&& APP_CONF->dockcolor_tabno == TABNO_HSV_MAP)
	{
		//HSV map

		DockColor_tabHSVMap_changeTheme(p->contents);
	}
}
