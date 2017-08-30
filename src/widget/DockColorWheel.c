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
 * DockColorWheel
 *
 * [Dock] カラーホイール
 *****************************************/

#include "mDef.h"
#include "mContainerDef.h"
#include "mWidget.h"
#include "mDockWidget.h"
#include "mContainer.h"
#include "mEvent.h"

#include "defWidgets.h"
#include "defConfig.h"
#include "defDraw.h"

#include "draw_main.h"

#include "DockObject.h"
#include "Docks_external.h"
#include "ColorWheel.h"


//-------------------

typedef struct
{
	DockObject obj;

	ColorWheel *wheel;
}DockColorWheel;

//-------------------

#define _DOCKCW(p)  ((DockColorWheel *)(p))
#define _DCW_PTR    ((DockColorWheel *)APP_WIDGETS->dockobj[DOCKWIDGET_COLOR_WHEEL])

//-------------------



//========================
// ハンドラ
//========================


/** イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY && ev->notify.id == 100)
	{
		if(ev->notify.type == COLORWHEEL_N_CHANGE_COL)
		{
			//色変更
			//param1:RGB param2:HSV
			
			drawColor_setDrawColor(ev->notify.param1);
			
			DockColor_changeDrawColor_hsv(ev->notify.param2);
		}
		else
			//タイプ変更
			APP_CONF->dockcolwheel_type = ev->notify.param1;
	}

	return 1;
}

/** mDockWidget ハンドラ : 内容作成 */

static mWidget *_dock_func_create(mDockWidget *dockwg,int id,mWidget *parent)
{
	DockColorWheel *p = _DCW_PTR;
	mWidget *ct;

	//コンテナ

	ct = mContainerCreate(parent, MCONTAINER_TYPE_VERT, 0, 0, MLF_EXPAND_WH);

	ct->event = _event_handle;
	ct->notifyTarget = MWIDGET_NOTIFYTARGET_SELF;
	ct->param = (intptr_t)p;

	M_CONTAINER(ct)->ct.padding.top = 2;

	//ホイール

	p->wheel = ColorWheel_new(ct, 100, APP_CONF->dockcolwheel_type, APP_DRAW->col.drawcol);
	
	return ct;
}

/** 作成 */

void DockColorWheel_new(mDockWidgetState *state)
{
	DockObject *p;

	p = DockObject_new(DOCKWIDGET_COLOR_WHEEL, sizeof(DockColorWheel), 0, 0,
			state, _dock_func_create);

	mDockWidgetCreateWidget(p->dockwg);
}


//=========================


/** 描画色変更時
 *
 * @param hsvcol  負の値でRGB描画色、それ以外でHSV値 */

void DockColorWheel_changeDrawColor(int hsvcol)
{
	DockColorWheel *p = _DCW_PTR;

	if(DockObject_canDoWidget((DockObject *)p))
	{
		if(hsvcol < 0)
			ColorWheel_setColor(p->wheel, APP_DRAW->col.drawcol, FALSE);
		else
			ColorWheel_setColor(p->wheel, hsvcol, TRUE);
	}
}

/** テーマ変更時 */

void DockColorWheel_changeTheme()
{
	DockColorWheel *p = _DCW_PTR;

	if(DockObject_canDoWidget((DockObject *)p))
		ColorWheel_redraw(p->wheel);
}
