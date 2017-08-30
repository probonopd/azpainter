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
 * DockBrush
 *
 * (Dock)ブラシ
 *****************************************/

#include "mDef.h"
#include "mContainerDef.h"
#include "mWidget.h"
#include "mDockWidget.h"
#include "mContainer.h"
#include "mSplitter.h"
#include "mEvent.h"

#include "defWidgets.h"
#include "DockObject.h"

#include "BrushList.h"


//------------------------

#define _DOCKBRUSH(p)  ((DockBrush *)(p))
#define _DB_PTR        ((DockBrush *)(APP_WIDGETS->dockobj[DOCKWIDGET_BRUSH]))

typedef struct _DockBrushSize DockBrushSize;
typedef struct _DockBrushList DockBrushList;
typedef struct _DockBrushValue DockBrushValue;

//------------------------

typedef struct
{
	DockObject obj;

	DockBrushSize *size;	//サイズ一覧
	DockBrushList *list;	//ブラシ一覧
	DockBrushValue *value;	//項目部分
}DockBrush;

//------------------------

DockBrushSize *DockBrushSize_new(mWidget *parent);

DockBrushList *DockBrushList_new(mWidget *parent,mFont *font);
void DockBrushList_update(DockBrushList *p);

DockBrushValue *DockBrushValue_new(mWidget *parent);
void DockBrushValue_setValue(DockBrushValue *p);
void DockBrushValue_setRadius(DockBrushValue *p,int val);

//------------------------


/** イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	DockBrush *p = _DOCKBRUSH(wg->param);

	if(ev->type == MEVENT_NOTIFY)
	{
		if(ev->notify.widgetFrom == M_WIDGET(p->list))
		{
			//ブラシリスト => 選択変更時など、項目値更新

			DockBrushValue_setValue(p->value);
		}
		else if(ev->notify.widgetFrom == M_WIDGET(p->size))
		{
			//サイズリスト => 選択時、半径値セット

			DockBrushValue_setRadius(p->value, ev->notify.param1);
		}
	}

	return 1;
}

/** mDockWidget ハンドラ : 内容作成 */

static mWidget *_dock_func_contents(mDockWidget *dockwg,int id,mWidget *parent)
{
	DockBrush *p = _DB_PTR;
	mWidget *ct;

	//コンテナ

	ct = mContainerCreate(parent, MCONTAINER_TYPE_VERT, 0, 0, MLF_EXPAND_WH);

	ct->event = _event_handle;
	ct->notifyTarget = MWIDGET_NOTIFYTARGET_SELF;
	ct->param = (intptr_t)p;
	ct->margin.top = ct->margin.bottom = 4;
	ct->margin.left = ct->margin.right = 2;

	//ウィジェット

	p->size = DockBrushSize_new(ct);

	mSplitterNew(0, ct, MSPLITTER_S_VERT);

	p->list = DockBrushList_new(ct, APP_WIDGETS->font_dock);

	mSplitterNew(0, ct, MSPLITTER_S_VERT);

	p->value = DockBrushValue_new(ct);

	//項目値セット

	DockBrushValue_setValue(p->value);

	return ct;
}

/** 作成 */

void DockBrush_new(mDockWidgetState *state)
{
	DockObject *p;

	p = DockObject_new(DOCKWIDGET_BRUSH, sizeof(DockBrush),
		MDOCKWIDGET_S_EXPAND, 0,
		state, _dock_func_contents);

	mDockWidgetCreateWidget(p->dockwg);
}



//===========================
// 外部からの呼び出し
//===========================


/** ブラシサイズ変更 (現在の選択ブラシ)
 *
 * +Shift ドラッグでのサイズ変更時など。 */

void DockBrush_changeBrushSize(int radius)
{
	DockBrush *p = _DB_PTR;

	//ウィジェットが作成されていない状態では、値のみ変更。

	DockBrushValue_setRadius(
		DockObject_canDoWidget((DockObject *)p)? p->value: NULL,
		radius);
}

/** ブラシの選択変更時 */

void DockBrush_changeBrushSel()
{
	DockBrush *p = _DB_PTR;

	if(DockObject_canDoWidget((DockObject *)p))
	{
		DockBrushList_update(p->list);
		DockBrushValue_setValue(p->value);
	}
}
