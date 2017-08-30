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
 * DockOption
 *
 * [Dock]オプション
 *****************************************/

#include "mDef.h"
#include "mContainerDef.h"
#include "mWidget.h"
#include "mDockWidget.h"
#include "mContainer.h"
#include "mEvent.h"
#include "mTab.h"
#include "mTrans.h"

#include "defConfig.h"
#include "defWidgets.h"
#include "defDraw.h"

#include "DockObject.h"
#include "DockOption_sub.h"

#include "draw_main.h"

#include "trgroup.h"


//------------------------

#define _DOCKOPTION(p)  ((DockOption *)(p))
#define _DO_PTR         ((DockOption *)APP_WIDGETS->dockobj[DOCKWIDGET_OPTION])

typedef struct
{
	DockObject obj;

	mTab *tab;
	mWidget *mainct,
		*contents;	//タブ内容
}DockOption;

//------------------------



//========================
// sub
//========================


/** タブの内容ウィジェット作成 */

static void _create_tab_contents(DockOption *p,mBool relayout)
{
	mWidget *(*createfunc[4])(mWidget *) = {
		DockOption_createTab_tool, DockOption_createTab_rule,
		DockOption_createTab_texture, DockOption_createTab_headtail
	};

	//現在のウィジェットを破棄

	if(p->contents)
	{
		mWidgetDestroy(p->contents);
		p->contents = NULL;
	}

	//作成

	p->contents = (createfunc[APP_CONF->dockopt_tabno])(p->mainct);

	//フォーカスを受け取らないようにする

	if(p->contents && !DockObject_canTakeFocus(&p->obj))
		mWidgetNoTakeFocus_under(p->contents);

	//再レイアウト

	if(relayout)
		mWidgetReLayout(p->mainct);
}


//========================


/** イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	DockOption *p = _DOCKOPTION(wg->param);

	if(ev->type == MEVENT_NOTIFY)
	{
		if(ev->notify.widgetFrom == (mWidget *)p->tab
			&& ev->notify.type == MTAB_N_CHANGESEL)
		{
			//タブ選択変更
			
			APP_CONF->dockopt_tabno = ev->notify.param1;

			_create_tab_contents(p, TRUE);
		}
	}

	return 1;
}

/** メインコンテナの破棄ハンドラ */

static void _destroy_handle(mWidget *wg)
{
	DockOption *p = _DO_PTR;

	if(p) p->contents = NULL;
}

/** mDockWidget ハンドラ : 内容作成 */

static mWidget *_dock_func_create(mDockWidget *dockwg,int id,mWidget *parent)
{
	DockOption *p = _DO_PTR;
	mWidget *ct;
	int i;

	//メインコンテナ

	ct = mContainerCreate(parent, MCONTAINER_TYPE_VERT, 0, 4, MLF_EXPAND_WH);

	p->mainct = ct;

	ct->destroy = _destroy_handle;
	ct->event = _event_handle;
	ct->notifyTarget = MWIDGET_NOTIFYTARGET_SELF;
	ct->param = (intptr_t)p;
	ct->margin.top = ct->margin.bottom = 2;

	//タブ

	p->tab = mTabCreate(ct, 0, MTAB_S_TOP | MTAB_S_HAVE_SEP | MTAB_S_FIT_SIZE,
			MLF_EXPAND_W, 0);

	M_TR_G(TRGROUP_DOCK_OPTION);

	for(i = 0; i < 4; i++)
		mTabAddItem(p->tab, M_TR_T(i), -1, i);

	mTabSetSel_index(p->tab, APP_CONF->dockopt_tabno);

	//タブ内容

	_create_tab_contents(p, FALSE);

	return ct;
}

/** 作成 */

void DockOption_new(mDockWidgetState *state)
{
	DockObject *p;

	p = DockObject_new(DOCKWIDGET_OPTION,
			sizeof(DockOption), 0, 0,
			state, _dock_func_create);

	mDockWidgetCreateWidget(p->dockwg);
}


//========================
// 外部からの呼び出し
//========================


/** ツール変更時 */

void DockOption_changeTool()
{
	DockOption *p = _DO_PTR;

	//「ツール」タブが選択されている場合、各ツールの内容に変更

	if(APP_CONF->dockopt_tabno == 0 && DockObject_canDoWidget((DockObject *)p))
		_create_tab_contents(p, TRUE);
}

/** スタンプイメージの変更時 */

void DockOption_changeStampImage()
{
	DockOption *p = _DO_PTR;

	if(DockObject_canDoWidget((DockObject *)p)
		&& APP_CONF->dockopt_tabno == 0 && p->contents
		&& APP_DRAW->tool.no == TOOL_STAMP)
	{
		DockOption_toolStamp_changeImage((mWidget *)p->contents->param);
	}
}
