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
 * DockObject
 *
 * ドックウィジェット 共通処理
 *****************************************/

#include "mDef.h"
#include "mWindowDef.h"
#include "mWidget.h"
#include "mDockWidget.h"
#include "mTrans.h"

#include "defWidgets.h"
#include "defConfig.h"

#include "defMainWindow.h"
#include "DockObject.h"

#include "trgroup.h"


//------------------
/* 各ドックウィジェットの ID (作成時、配置の設定時に使う) */

static const char g_dock_id[] = {'t','o','l','b','c','p','r','v','i','f','w',0};

//------------------



//=========================
// sub
//=========================


/** 配置決定関数 */

static int _dock_func_position(mDockWidget *dockwg,int id1,int id2)
{
	char *pc,*p1,*p2;

	p1 = p2 = NULL;

	for(pc = APP_CONF->dock_layout; *pc; pc++)
	{
		if(*pc == id1)
			p1 = pc;
		else if(*pc == id2)
			p2 = pc;
	}

	return (p1 > p2);
}

/** Dock 番号から配置先の親をセット */

static void _set_dock_parent(mDockWidget *dockwg,int no)
{
	int n;

	n = DockObject_getPaneNo_fromNo(no);

	mDockWidgetSetDockParent(dockwg, APP_WIDGETS->pane[n]);
}

/** 内容作成時のハンドラ */

static mWidget *_create_handle(mDockWidget *dockwg,int id,mWidget *parent)
{
	DockObject *p;
	mWindow *win;

	p = (DockObject *)mDockWidgetGetParam(dockwg);

	//ウィンドウモード時は、アクセラレータをセット
	/* メインウィンドウがアクティブでなくてもショートカットキーが動作するように */

	win = mDockWidgetGetWindow(dockwg);
	if(win)
		win->win.accelerator = APP_WIDGETS->mainwin->win.accelerator;
	
	return (p->create)(dockwg, id, parent);
}


//=========================
// main
//=========================


/** 削除 */

void DockObject_destroy(DockObject *p)
{
	if(p)
	{
		if(p->destroy)
			(p->destroy)(p);

		mDockWidgetDestroy(p->dockwg);
		mFree(p);
	}
}

/** 作成 */

DockObject *DockObject_new(int no,int size,uint32_t dock_style,
	uint32_t window_style,mDockWidgetState *state,
	mWidget *(*func_create)(mDockWidget *,int,mWidget *))
{
	DockObject *p;
	mDockWidget *dockwg;

	//DockObject 作成

	p = (DockObject *)mMalloc(size, TRUE);

	APP_WIDGETS->dockobj[no] = p;

	p->create = func_create;

	//mDockWidget 作成

	dockwg = mDockWidgetNew(g_dock_id[no],
		MDOCKWIDGET_S_HAVE_CLOSE | MDOCKWIDGET_S_HAVE_SWITCH | MDOCKWIDGET_S_HAVE_FOLD
		 | MDOCKWIDGET_S_NO_FOCUS | dock_style);

	p->dockwg = dockwg;

	//設定

	mDockWidgetSetParam(dockwg, (intptr_t)p);
	mDockWidgetSetTitle(dockwg, M_TR_T2(TRGROUP_DOCK_NAME, no));
	mDockWidgetSetState(dockwg, state);
	mDockWidgetSetFont(dockwg, APP_WIDGETS->font_dock);

	_set_dock_parent(dockwg, no);

	mDockWidgetSetCallback_create(dockwg, _create_handle);
	mDockWidgetSetCallback_arrange(dockwg, _dock_func_position);

	mDockWidgetSetWindowInfo(dockwg, M_WINDOW(APP_WIDGETS->mainwin),
		MWINDOW_S_NORMAL | MWINDOW_S_OWNER | MWINDOW_S_NO_IM | window_style);

	return p;
}

/** 初期表示 (ウィンドウの表示) */

void DockObject_showStart(DockObject *p)
{
	if(p)
		mDockWidgetShowWindow(p->dockwg);
}

/** 内容ウィジェットに対して処理ができるか
 *
 * 閉じている (作成されていない) 場合は FALSE。
 * 画面上に表示されていなくても、作成されていれば TRUE。 */

mBool DockObject_canDoWidget(DockObject *p)
{
	return (p && mDockWidgetIsCreated(p->dockwg));
}

/** 内容ウィジェットに対して処理ができるか (画面上に表示されている)
 *
 * 閉じている、畳まれている、非表示時は FALSE */

mBool DockObject_canDoWidget_visible(DockObject *p)
{
	return (p && mDockWidgetIsVisibleContents(p->dockwg));
}

/** 現在の状態において、フォーカスを受け取ることができるか
 *
 * ウィンドウ表示なら常に受け取る。
 * ドッキング状態なら、MDOCKWIDGET_S_NO_FOCUS があれば受け取らない。 */

mBool DockObject_canTakeFocus(DockObject *p)
{
	return (p && mDockWidgetCanTakeFocus(p->dockwg));
}

/** オーナーウィンドウを取得
 *
 * ウィンドウモード状態なら、そのウィンドウ。
 * そうでなければ、メインウィンドウ。 */

mWindow *DockObject_getOwnerWindow(DockObject *p)
{
	mWindow *owner;

	owner = mDockWidgetGetWindow(p->dockwg);
	if(!owner) owner = M_WINDOW(APP_WIDGETS->mainwin);

	return owner;
}


//==========================
//
//==========================


/** ID から Dock 番号を取得 */

int DockObject_getDockNo_fromID(int id)
{
	const char *pc;
	int no = 0;
	
	for(pc = g_dock_id; *pc; pc++, no++)
	{
		if(*pc == id) return no;
	}

	return -1;
}

/** Dock 番号から配置するペインの番号を取得 */

int DockObject_getPaneNo_fromNo(int no)
{
	char *pc;
	int paneno = 0;

	no = g_dock_id[no];

	for(pc = APP_CONF->dock_layout; *pc && *pc != no; pc++)
	{
		if(*pc == ':') paneno++;
	}

	return (paneno > 2)? 2: paneno;
}

/** ウィンドウモードの状態の時のみ、再レイアウトを行う */

void DockObject_relayout_inWindowMode(int dockno)
{
	mDockWidget *p = APP_WIDGETS->dockobj[dockno]->dockwg;
	mWindow *win;

	win = mDockWidgetGetWindow(p);
	if(win)
		mWidgetReLayout(M_WIDGET(win));
}


//============================
// 全ドック処理
//============================


/** 再配置 */

void DockObjects_relocate()
{
	int i;

	for(i = 0; i < DOCKWIDGET_NUM; i++)
	{
		if(APP_WIDGETS->dockobj[i])
		{
			//親を再セット
			_set_dock_parent(APP_WIDGETS->dockobj[i]->dockwg, i);

			mDockWidgetRelocate(APP_WIDGETS->dockobj[i]->dockwg, FALSE);
		}
	}
}

/** 全てウィンドウモードまたは格納 */

void DockObjects_all_windowMode(int type)
{
	int i;

	for(i = 0; i < DOCKWIDGET_NUM; i++)
	{
		if(APP_WIDGETS->dockobj[i])
			mDockWidgetSetWindowMode(APP_WIDGETS->dockobj[i]->dockwg, type);
	}
}


//============================
// 設定
//============================


/** 配置設定を正規化 */

void DockObject_normalize_layout_config()
{
	char buf[DOCKWIDGET_NUM + 2];
	int i,pos,paneno,no;
	char *pc;

	//最後の2つは、ペイン2,3

	for(i = 0; i < DOCKWIDGET_NUM + 2; i++)
		buf[i] = -1;

	//位置

	pos = paneno = 0;

	for(pc = APP_CONF->dock_layout; *pc; pc++)
	{
		if(*pc == ':')
		{
			if(paneno < 2)
			{
				buf[DOCKWIDGET_NUM + paneno] = pos++;
				paneno++;
			}
		}
		else
		{
			no = DockObject_getDockNo_fromID(*pc);

			if(no != -1 && buf[no] == -1)
				buf[no] = pos++;
		}
	}

	//ペイン残り

	for(i = DOCKWIDGET_NUM; i < DOCKWIDGET_NUM + 2; i++)
	{
		if(buf[i] == -1)
			buf[i] = pos++;
	}

	//パネル残り

	for(i = 0; i < DOCKWIDGET_NUM; i++)
	{
		if(buf[i] == -1)
			buf[i] = pos++;
	}

	//再セット

	pc = APP_CONF->dock_layout;

	for(i = 0; i < DOCKWIDGET_NUM + 2; i++)
	{
		if(i >= DOCKWIDGET_NUM)
			no = ':';
		else
			no = g_dock_id[i];

		pos = buf[i];
		pc[pos] = no;
	}

	pc[DOCKWIDGET_NUM + 2] = 0;
}
