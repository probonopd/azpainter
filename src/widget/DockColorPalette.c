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
 * DockColorPalette
 *
 * [dock] カラーパレット
 *****************************************/

#include "mDef.h"
#include "mWindowDef.h"
#include "mWidget.h"
#include "mContainer.h"
#include "mTab.h"
#include "mDockWidget.h"
#include "mEvent.h"
#include "mTrans.h"

#include "defWidgets.h"
#include "defConfig.h"
#include "defDraw.h"

#include "DockObject.h"
#include "Docks_external.h"
#include "draw_main.h"

#include "trgroup.h"


//------------------------

#define _DOCKCOLORPAL(p)  ((DockColorPalette *)(p))
#define _DCP_PTR          ((DockColorPalette *)APP_WIDGETS->dockobj[DOCKWIDGET_COLOR_PALETTE]);

typedef struct
{
	DockObject obj;

	mWidget *tab_parent,	//タブの親
		*contents;			//タブ内容のメインコンテナ
	mTab *tab;
}DockColorPalette;

//------------------------

enum
{
	WID_TAB = 100,
	WID_TAB_CONTENTS
};

enum
{
	TABNO_PALETTE,
	TABNO_HLS,
	TABNO_GRADATION
};

//------------------------

/* DockColorPalette_tab.c */
mWidget *DockColorPalette_tabPalette_create(mWidget *parent,int id);
mWidget *DockColorPalette_tabHLS_create(mWidget *parent,int id);
mWidget *DockColorPalette_tabGrad_create(mWidget *parent,int id);
void DockColorPalette_tabHLS_changeTheme(mWidget *wg);

//------------------------



//===========================
// sub
//===========================


/** タブ内容作成 */

static void _create_tab_contents(DockColorPalette *p,mBool relayout)
{
	mWidget *(*func[3])(mWidget *,int) = {
		DockColorPalette_tabPalette_create,
		DockColorPalette_tabHLS_create,
		DockColorPalette_tabGrad_create
	};

	//現在のウィジェット削除

	if(p->contents)
		mWidgetDestroy(p->contents);

	//作成

	p->contents = (func[APP_CONF->dockcolpal_tabno])(p->tab_parent, WID_TAB_CONTENTS);

	//ドッキング状態なら、フォーカスを受け取らないようにする

	if(!DockObject_canTakeFocus((DockObject *)p))
		mWidgetNoTakeFocus_under(p->contents);

	//再レイアウト

	if(relayout)
		mWidgetReLayout(p->tab_parent);
}

/** イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	DockColorPalette *p = _DOCKCOLORPAL(wg->param);

	if(ev->type == MEVENT_NOTIFY)
	{
		switch(ev->notify.id)
		{
			//タブ内容
			/* 描画色変更要求。param1 に色 */
			case WID_TAB_CONTENTS:
				if(APP_DRAW->col.drawcol != (uint32_t)ev->notify.param1)
				{
					drawColor_setDrawColor(ev->notify.param1);
					
					DockColor_changeDrawColor();
				}
				break;
			//タブ選択変更
			case WID_TAB:
				if(ev->notify.type == MTAB_N_CHANGESEL)
				{
					APP_CONF->dockcolpal_tabno = ev->notify.param1;

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
	DockColorPalette *p = _DCP_PTR;

	//mDockWidget が閉じられた場合、タブ内容のポインタをクリア

	if(p) p->contents = NULL;
}

/** mDockWidget ハンドラ : 内容作成 */

static mWidget *_dock_func_create(mDockWidget *dockwg,int id,mWidget *parent)
{
	DockColorPalette *p = _DCP_PTR;
	mWidget *ct;
	int i;

	//コンテナ (トップ)

	ct = mContainerCreate(parent, MCONTAINER_TYPE_VERT, 0, 5, MLF_EXPAND_WH);

	p->tab_parent = ct;

	ct->destroy = _mainct_destroy_handle;
	ct->event = _event_handle;
	ct->notifyTarget = MWIDGET_NOTIFYTARGET_SELF;
	ct->param = (intptr_t)p;
	ct->margin.top = ct->margin.bottom = 4;

	//タブ

	M_TR_G(TRGROUP_DOCK_COLOR_PALETTE);

	p->tab = mTabCreate(ct, WID_TAB, MTAB_S_TOP | MTAB_S_HAVE_SEP, MLF_EXPAND_W, 0);

	for(i = 0; i < 3; i++)
		mTabAddItemText(p->tab, M_TR_T(i));

	mTabSetSel_index(p->tab, APP_CONF->dockcolpal_tabno);

	_create_tab_contents(p, FALSE);

	return ct;
}

/** 作成 */

void DockColorPalette_new(mDockWidgetState *state)
{
	DockObject *p;

	p = DockObject_new(DOCKWIDGET_COLOR_PALETTE, sizeof(DockColorPalette), 0,
			MWINDOW_S_ENABLE_DND,
			state, _dock_func_create);

	mDockWidgetCreateWidget(p->dockwg);
}

/** テーマ変更時 */

void DockColorPalette_changeTheme()
{
	DockColorPalette *p = _DCP_PTR;

	if(DockObject_canDoWidget((DockObject *)p)
		&& APP_CONF->dockcolpal_tabno == TABNO_HLS)
	{
		//HLS

		DockColorPalette_tabHLS_changeTheme(p->contents);
	}
}
