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
 * DockCanvasCtrl
 *
 * [dock] キャンバス操作
 *****************************************/

#include "mDef.h"
#include "mContainerDef.h"
#include "mWidget.h"
#include "mDockWidget.h"
#include "mContainer.h"
#include "mEvent.h"

#include "defWidgets.h"
#include "defDraw.h"
#include "DockObject.h"
#include "CanvasCtrlBar.h"

#include "draw_main.h"


//------------------------

#define DOCKCANVASCTRL(p)  ((DockCanvasCtrl *)(p))
#define _DCC_PTR   ((DockCanvasCtrl *)APP_WIDGETS->dockobj[DOCKWIDGET_CANVAS_CTRL])

typedef struct
{
	DockObject obj;

	CanvasCtrlBar *bar_zoom,*bar_angle;
}DockCanvasCtrl;

//------------------------


/** イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	DockCanvasCtrl *p = DOCKCANVASCTRL(wg->param);

	if(ev->type == MEVENT_NOTIFY)
	{
		if(ev->notify.widgetFrom == M_WIDGET(p->bar_zoom))
		{
			//------ 倍率

			switch(ev->notify.type)
			{
				//バー
				case CANVASCTRLBAR_N_BAR:
					if(ev->notify.param1 == CANVASCTRLBAR_N_BAR_TYPE_PRESS)
						drawCanvas_lowQuality();
					else if(ev->notify.param1 == CANVASCTRLBAR_N_BAR_TYPE_RELEASE)
						drawCanvas_normalQuality();

					drawCanvas_setZoomAndAngle(APP_DRAW, ev->notify.param2, 0, 1, TRUE);
					break;
				//ボタン
				case CANVASCTRLBAR_N_BUTTON:
					if(ev->notify.param1 == CANVASCTRLBAR_BTT_RESET)
						drawCanvas_setZoomAndAngle(APP_DRAW, 1000, 0, 1, TRUE);
					else
						drawCanvas_zoomStep(APP_DRAW, (ev->notify.param1 == CANVASCTRLBAR_BTT_ADD));
					
					CanvasCtrlBar_setPos(p->bar_zoom, APP_DRAW->canvas_zoom);
					break;
			}
		}
		else if(ev->notify.widgetFrom == M_WIDGET(p->bar_angle))
		{
			//------ 回転

			switch(ev->notify.type)
			{
				//バー
				case CANVASCTRLBAR_N_BAR:
					if(ev->notify.param1 == CANVASCTRLBAR_N_BAR_TYPE_PRESS)
						drawCanvas_lowQuality();
					else if(ev->notify.param1 == CANVASCTRLBAR_N_BAR_TYPE_RELEASE)
						drawCanvas_normalQuality();

					drawCanvas_setZoomAndAngle(APP_DRAW, 0, ev->notify.param2 * 10, 2, TRUE);
					break;
				//ボタン
				case CANVASCTRLBAR_N_BUTTON:
					if(ev->notify.param1 == CANVASCTRLBAR_BTT_RESET)
						drawCanvas_setZoomAndAngle(APP_DRAW, 0, 0, 2, TRUE);
					else
						drawCanvas_rotateStep(APP_DRAW, (ev->notify.param1 == CANVASCTRLBAR_BTT_SUB));
					
					CanvasCtrlBar_setPos(p->bar_angle, APP_DRAW->canvas_angle / 10);
					break;
			}
		}
	}

	return 1;
}

/** mDockWidget ハンドラ : 内容作成 */

static mWidget *_dock_func_contents(mDockWidget *dockwg,int id,mWidget *parent)
{
	DockCanvasCtrl *p = _DCC_PTR;
	mWidget *ct;

	//コンテナ

	ct = mContainerCreate(parent, MCONTAINER_TYPE_VERT, 0, 6, MLF_EXPAND_WH);

	ct->event = _event_handle;
	ct->notifyTarget = MWIDGET_NOTIFYTARGET_SELF;
	ct->param = (intptr_t)p;
	ct->margin.top = ct->margin.bottom = 4;
	ct->margin.left = ct->margin.right = 2;

	//ウィジェット

	p->bar_zoom = CanvasCtrlBar_new(ct, CANVASCTRLBAR_TYPE_ZOOM);
	p->bar_angle = CanvasCtrlBar_new(ct, CANVASCTRLBAR_TYPE_ANGLE);

	CanvasCtrlBar_setPos(p->bar_zoom, APP_DRAW->canvas_zoom);
	CanvasCtrlBar_setPos(p->bar_angle, APP_DRAW->canvas_angle / 10);

	return ct;
}


//========================
//
//========================


/** 作成 */

void DockCanvasCtrl_new(mDockWidgetState *state)
{
	DockObject *p;

	p = DockObject_new(DOCKWIDGET_CANVAS_CTRL,
			sizeof(DockCanvasCtrl), 0, 0,
			state, _dock_func_contents);

	mDockWidgetCreateWidget(p->dockwg);
}

/** 倍率/回転の値をセット */

void DockCanvasCtrl_setPos()
{
	DockCanvasCtrl *p = _DCC_PTR;

	if(DockObject_canDoWidget((DockObject *)p))
	{
		CanvasCtrlBar_setPos(p->bar_zoom, APP_DRAW->canvas_zoom);
		CanvasCtrlBar_setPos(p->bar_angle, APP_DRAW->canvas_angle / 10);
	}
}
