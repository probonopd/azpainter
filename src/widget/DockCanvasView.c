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
 * DockCanvasView
 *
 * [dock] キャンバスビュー
 *****************************************/

#include "mDef.h"
#include "mContainerDef.h"
#include "mWidget.h"
#include "mDockWidget.h"
#include "mContainer.h"
#include "mIconButtons.h"
#include "mEvent.h"

#include "defWidgets.h"
#include "defConfig.h"
#include "defDraw.h"

#include "draw_main.h"
#include "draw_calc.h"

#include "DockCanvasView.h"
#include "PopupSliderZoom.h"
#include "AppResource.h"

#include "trgroup.h"


//-------------------

#define _DCV_PTR        ((DockCanvasView *)APP_WIDGETS->dockobj[DOCKWIDGET_CANVAS_VIEW])
#define _IS_LOUPE_MODE  (APP_CONF->canvasview_flags & CONFIG_CANVASVIEW_F_LOUPE_MODE)
#define _IS_ZOOM_FIT    (APP_CONF->canvasview_flags & CONFIG_CANVASVIEW_F_FIT)

enum
{
	CMDID_LOUPE_MODE,
	CMDID_FITMODE,
	CMDID_ZOOM,
	CMDID_OPTION
};

//-------------------

DockCanvasViewArea *DockCanvasViewArea_new(DockCanvasView *canvasview,mWidget *parent);
void DockCanvasViewArea_drawRect(DockCanvasViewArea *p,mBox *box);

/* DockCanvasView_dlg.c */
void DockCanvasView_runOptionDialog(mWindow *owner,mBool imageviewer);

//-------------------



//==============================
// sub
//==============================


/** イメージ位置 -> エリア座標変換 */

static void _image_to_area(DockCanvasView *p,mPoint *pt,int x,int y)
{
	pt->x = (x - p->originX) * p->dscale + (M_WIDGET(p->area)->w >> 1) - p->ptScroll.x + 0.5;
	pt->y = (y - p->originY) * p->dscale + (M_WIDGET(p->area)->h >> 1) - p->ptScroll.y + 0.5;
}

/** (固定表示用) スクロールリセット */

static void _reset_scroll(DockCanvasView *p)
{
	p->originX = APP_DRAW->imgw * 0.5;
	p->originY = APP_DRAW->imgh * 0.5;

	p->ptScroll.x = p->ptScroll.y = 0;
}

/** (固定表示用) スクロール位置調整 */

void DockCanvasView_adjustScroll(DockCanvasView *p)
{
	int imgw,imgh,aw,ah,x1,y1,x2,y2;

	if(!_IS_LOUPE_MODE && !_IS_ZOOM_FIT)
	{
		imgw = APP_DRAW->imgw;
		imgh = APP_DRAW->imgh;
		aw = M_WIDGET(p->area)->w >> 1;
		ah = M_WIDGET(p->area)->h >> 1;

		//左上/右下のキャンバス位置

		x1 = -(p->originX) * p->dscale + aw - p->ptScroll.x;
		y1 = -(p->originY) * p->dscale + ah - p->ptScroll.y;
		x2 = (imgw - p->originX) * p->dscale + aw - p->ptScroll.x;
		y2 = (imgh - p->originY) * p->dscale + ah - p->ptScroll.y;

		//調整

		if(x1 > aw)
			p->ptScroll.x = -(p->originX) * p->dscale;
		else if(x2 < aw)
			p->ptScroll.x = (imgw - p->originX) * p->dscale;

		if(y1 > ah)
			p->ptScroll.y = -(p->originY) * p->dscale;
		else if(y2 < ah)
			p->ptScroll.y = (imgh - p->originY) * p->dscale;
	}
}

/** 倍率をウィンドウに合わせる */

void DockCanvasView_setZoom_fit(DockCanvasView *p)
{
	if(!_IS_LOUPE_MODE && _IS_ZOOM_FIT)
	{
		double h,v;

		//倍率が低い方 (拡大もあり)

		h = (double)M_WIDGET(p->area)->w / APP_DRAW->imgw;
		v = (double)M_WIDGET(p->area)->h / APP_DRAW->imgh;

		if(h > v) h = v;
		if(h < 0.01) h = 0.01; else if(h > 16) h = 16;

		p->zoom = (int)(h * 100 + 0.5);
		p->dscale = h;
		p->dscalediv = 1.0 / h;

		_reset_scroll(p);
	}
}

/** (共通) 表示倍率セット
 *
 * @param zoom 100 = 等倍 */

mBool DockCanvasView_setZoom(DockCanvasView *p,int zoom)
{
	if(zoom == p->zoom)
		return FALSE;
	else
	{
		p->zoom = zoom;
		p->dscale = (double)zoom / 100;
		p->dscalediv = 1.0 / p->dscale;

		if(_IS_LOUPE_MODE)
			APP_CONF->canvasview_zoom_loupe = zoom;
		else
		{
			APP_CONF->canvasview_zoom_normal = zoom;

			DockCanvasView_adjustScroll(p);
		}

		return TRUE;
	}
}

/** ルーペの表示位置変更
 *
 * @param pt  NULL で (0,0) にリセット */

static void _change_loupe_pos(DockCanvasView *p,mPoint *pt)
{
	if(pt)
	{
		p->ptLoupeCur = *pt;
		p->originX = pt->x + 0.5;
		p->originY = pt->y + 0.5;
	}
	else
	{
		p->ptLoupeCur.x = p->ptLoupeCur.y = 0;
		p->originX = p->originY = 0.5;
	}

	p->ptScroll.x = p->ptScroll.y = 0;
}

/** エリア更新 */

static void _update_area(DockCanvasView *p)
{
	DockCanvasView_setZoom_fit(p);
	
	mWidgetUpdate(M_WIDGET(p->area));
}

/** アイコンボタンの有効/無効セット */

static void _enable_iconbutton(DockCanvasView *p)
{
	mBool fit,loupe;

	loupe = _IS_LOUPE_MODE;
	fit = _IS_ZOOM_FIT;

	//全体表示 (ルーペ時無効)

	mIconButtonsSetEnable(M_ICONBUTTONS(p->iconbtt), CMDID_FITMODE, !loupe);

	//表示倍率 (ルーペ時、または全体表示OFF時、有効)
	
	mIconButtonsSetEnable(M_ICONBUTTONS(p->iconbtt), CMDID_ZOOM, (loupe || !fit));
}

/** モード変更時 */

static void _change_mode(DockCanvasView *p)
{
	if(_IS_LOUPE_MODE)
	{
		//ルーペ

		_change_loupe_pos(p, NULL);

		DockCanvasView_setZoom(p, APP_CONF->canvasview_zoom_loupe);
	}
	else
	{
		//固定表示
		
		_reset_scroll(p);

		if(!_IS_ZOOM_FIT)
			DockCanvasView_setZoom(p, APP_CONF->canvasview_zoom_normal);
	}

	_enable_iconbutton(p);

	_update_area(p);
}

/** 範囲更新 (直接描画) */

static void _update_rect(DockCanvasView *p,mBox *boximg)
{
	mPoint pt1,pt2;
	mBox box;
	int aw,ah;

	aw = M_WIDGET(p->area)->w;
	ah = M_WIDGET(p->area)->h;

	//イメージ -> エリア座標

	_image_to_area(p, &pt1, boximg->x, boximg->y);
	_image_to_area(p, &pt2, boximg->x + boximg->w, boximg->y + boximg->h);

	pt1.x--, pt1.y--;
	pt2.x++, pt2.y++;

	//範囲外判定

	if(pt2.x < 0 || pt2.y < 0 || pt1.x >= aw || pt1.y >= ah)
		return;

	//調整

	if(pt1.x < 0) pt1.x = 0;
	if(pt1.y < 0) pt1.y = 0;
	if(pt2.x >= aw) pt2.x = aw - 1;
	if(pt2.y >= ah) pt2.y = ah - 1;

	//更新

	box.x = pt1.x, box.y = pt1.y;
	box.w = pt2.x - pt1.x + 1;
	box.h = pt2.y - pt1.y + 1;

	DockCanvasViewArea_drawRect(p->area, &box);	
}


//==============================
// 表示倍率スライダー
//==============================


/** 表示倍率スライダー、倍率変更時 */

static void _popupslider_handle(int zoom,void *param)
{
	if(DockCanvasView_setZoom((DockCanvasView *)param, zoom))
		_update_area((DockCanvasView *)param);
}

/** ポップアップ実行 */

static void _popupslider_run(DockCanvasView *p)
{
	mBox box;
	int min,max,low,hi;

	mIconButtonsGetItemBox(M_ICONBUTTONS(p->iconbtt), CMDID_ZOOM, &box, TRUE);

	if(_IS_LOUPE_MODE)
		min = 100, max = 1600, low = 0, hi = 100;
	else
		min = 2, max = 1000, low = 2, hi = 50;

	PopupSliderZoom_run(box.x + box.w + 2, box.y - 50,
		min, max, p->zoom, low, hi,
		_popupslider_handle, p);
}


//==============================
// ハンドラ
//==============================


/** イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	DockCanvasView *p = DOCKCANVASVIEW(wg->param);

	switch(ev->type)
	{
		//ツールバー
		case MEVENT_COMMAND:
			switch(ev->cmd.id)
			{
				//ルーペモード
				case CMDID_LOUPE_MODE:
					APP_CONF->canvasview_flags ^= CONFIG_CANVASVIEW_F_LOUPE_MODE;

					_change_mode(p);
					break;
				//全体表示
				case CMDID_FITMODE:
					APP_CONF->canvasview_flags ^= CONFIG_CANVASVIEW_F_FIT;

					_change_mode(p);
					break;
				//倍率
				case CMDID_ZOOM:
					if(_IS_LOUPE_MODE || !_IS_ZOOM_FIT)
						_popupslider_run(p);
					break;
				//設定
				case CMDID_OPTION:
					DockCanvasView_runOptionDialog(DockObject_getOwnerWindow((DockObject *)p), FALSE);
					break;
			}
			break;
	}

	return 1;
}


//==============================
// 作成
//==============================


/** mDockWidget ハンドラ : 内容作成 */

static mWidget *_dock_func_contents(mDockWidget *dockwg,int id,mWidget *parent)
{
	DockCanvasView *p = _DCV_PTR;
	mWidget *ct;
	mIconButtons *ib;
	int i;

	//コンテナ

	ct = mContainerCreate(parent, MCONTAINER_TYPE_HORZ, 0, 0, MLF_EXPAND_WH);

	ct->event = _event_handle;
	ct->notifyTarget = MWIDGET_NOTIFYTARGET_SELF;
	ct->param = (intptr_t)p;
	ct->draw = NULL;

	//アイコンボタン

	ib = mIconButtonsNew(0, ct,
		MICONBUTTONS_S_TOOLTIP | MICONBUTTONS_S_VERT | MICONBUTTONS_S_DESTROY_IMAGELIST);

	p->iconbtt = M_WIDGET(ib);

	ib->wg.fLayout = MLF_EXPAND_H;

	mIconButtonsSetImageList(ib, AppResource_loadIconImage("canvasview.png", APP_CONF->iconsize_other));

	mIconButtonsSetTooltipTrGroup(ib, TRGROUP_DOCK_CANVASVIEW);

	for(i = 0; i < 4; i++)
	{
		mIconButtonsAdd(ib, i, i, i,
			(i == CMDID_LOUPE_MODE || i == CMDID_FITMODE)? MICONBUTTONS_ITEMF_CHECKBUTTON: 0);
	}

	mIconButtonsCalcHintSize(ib);

	mIconButtonsSetCheck(ib, CMDID_LOUPE_MODE, _IS_LOUPE_MODE);
	mIconButtonsSetCheck(ib, CMDID_FITMODE, _IS_ZOOM_FIT);

	_enable_iconbutton(p);

	//表示エリア

	p->area = DockCanvasViewArea_new(p, ct);

	return ct;
}

/** 作成 */

void DockCanvasView_new(mDockWidgetState *state)
{
	DockCanvasView *p;

	//DockCanvasView 作成

	p = (DockCanvasView *)DockObject_new(DOCKWIDGET_CANVAS_VIEW,
			sizeof(DockCanvasView), 0, 0,
			state, _dock_func_contents);

	//初期化

	p->zoom = (_IS_LOUPE_MODE)? APP_CONF->canvasview_zoom_loupe: APP_CONF->canvasview_zoom_normal;
	p->dscale = p->zoom / 100.0;
	p->dscalediv = 1.0 / p->dscale;

	//

	mDockWidgetCreateWidget(p->obj.dockwg);
}


//==============================
// 外部からの呼び出し
//==============================


/** アイコンサイズ変更 */

void DockCanvasView_changeIconSize()
{
	DockCanvasView *p = _DCV_PTR;

	if(DockObject_canDoWidget((DockObject *)p))
	{
		mIconButtonsReplaceImageList(M_ICONBUTTONS(p->iconbtt),
			AppResource_loadIconImage("canvasview.png", APP_CONF->iconsize_other));

		DockObject_relayout_inWindowMode(DOCKWIDGET_CANVAS_VIEW);
	}
}

/** 全体を更新 */

void DockCanvasView_update()
{
	DockCanvasView *p = _DCV_PTR;

	if(DockObject_canDoWidget_visible((DockObject *)p))
		mWidgetUpdate(M_WIDGET(p->area));
}

/** 編集イメージサイズが変わった時 */

void DockCanvasView_changeImageSize()
{
	DockCanvasView *p = _DCV_PTR;

	if(DockObject_canDoWidget((DockObject *)p))
		_change_mode(p);
}

/** 固定表示用、範囲更新 */

void DockCanvasView_updateRect(mBox *boximg)
{
	DockCanvasView *p = _DCV_PTR;

	if(!_IS_LOUPE_MODE && DockObject_canDoWidget_visible((DockObject *)p))
		_update_rect(p, boximg);
}

/** ルーペ用、位置変更時
 *
 * @param pt イメージ位置 */

void DockCanvasView_changeLoupePos(mPoint *pt)
{
	DockCanvasView *p = _DCV_PTR;

	if(_IS_LOUPE_MODE && DockObject_canDoWidget_visible((DockObject *)p))
	{
		if(pt->x != p->ptLoupeCur.x || pt->y != p->ptLoupeCur.y)
		{
			_change_loupe_pos(p, pt);

			mWidgetUpdate(M_WIDGET(p->area));
		}
	}
}
