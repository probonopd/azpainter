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
 * DockCanvasViewArea
 * 
 * [dock] キャンバスビューの表示エリア
 *****************************************/

#include "mDef.h"
#include "mWidgetDef.h"
#include "mWidget.h"
#include "mEvent.h"
#include "mPixbuf.h"

#include "defConfig.h"
#include "defDraw.h"
#include "defCanvasInfo.h"

#include "ImageBufRGB16.h"
#include "PixbufDraw.h"

#include "draw_main.h"

#include "DockCanvasView.h"


//-------------------

typedef struct _DockCanvasViewArea
{
	mWidget wg;

	DockCanvasView *canvasview;
	int dragbtt,		//ドラッグ時の操作ボタン
		optype,			//現在の操作タイプ
		lastx,lasty;
}DockCanvasViewArea;

//-------------------

#define _CVAREA(p)      ((DockCanvasViewArea *)(p))
#define _IS_LOUPE_MODE  (APP_CONF->canvasview_flags & CONFIG_CANVASVIEW_F_LOUPE_MODE)
#define _IS_ZOOM_FIT    (APP_CONF->canvasview_flags & CONFIG_CANVASVIEW_F_FIT)

enum
{
	_OPTYPE_VIEW_SCROLL,
	_OPTYPE_CANVAS_SCROLL,
	_OPTYPE_VIEW_ZOOM
};

//-------------------


//==========================
// sub
//==========================


/** ボタンから操作タイプ取得 */

static int _get_optype_from_button(int btt,uint32_t state)
{
	int no = -1;

	if(btt == M_BTT_LEFT)
	{
		//左ボタン
		
		if(state & M_MODS_CTRL)
			no = 1;
		else if(state & M_MODS_SHIFT)
			no = 2;
		else
			no = 0;
	}
	else if(btt == M_BTT_RIGHT)
		no = 3;
	else if(btt == M_BTT_MIDDLE)
		no = 4;

	if(no < 0) return -1;

	//設定データ

	no = APP_CONF->canvasview_btt[no];

	//全体表示時は、キャンバススクロールのみ

	if(_IS_ZOOM_FIT && no != _OPTYPE_CANVAS_SCROLL)
		return -1;

	return no;
}

/** キャンバススクロール */

static void _canvas_scroll(DockCanvasViewArea *p,int x,int y)
{
	DockCanvasView *cv = p->canvasview;
	mDoublePoint pt;

	pt.x = (x + cv->ptScroll.x - (p->wg.w >> 1)) * cv->dscalediv + cv->originX;
	pt.y = (y + cv->ptScroll.y - (p->wg.h >> 1)) * cv->dscalediv + cv->originY;

	drawCanvas_setScrollReset_update(APP_DRAW, &pt);
}


//==========================
// ハンドラ
//==========================


/** ボタン押し時 */

static void _event_press(DockCanvasViewArea *p,mEvent *ev)
{
	int type;

	type = _get_optype_from_button(ev->pt.btt, ev->pt.state);
	if(type < 0) return;

	p->dragbtt = ev->pt.btt;
	p->optype = type;
	p->lastx = ev->pt.x;
	p->lasty = ev->pt.y;

	mWidgetGrabPointer(M_WIDGET(p));

	//キャンバススクロール

	if(type == _OPTYPE_CANVAS_SCROLL)
	{
		drawCanvas_lowQuality();
		
		_canvas_scroll(p, ev->pt.x, ev->pt.y);
	}
}

/** 移動時 - 倍率変更 */

static void _motion_zoom(DockCanvasViewArea *p,int y)
{
	int n,dir;
		
	n = p->canvasview->zoom;

	dir = p->lasty - y;
	if(dir == 0) return;

	n += dir * ((n < 100)? 2: 10);

	if(n < 2) n = 2;
	else if(n > 1000) n = 1000;

	if(n < 100)
		n &= ~1;
	else
		n = n / 10 * 10;

	//セット

	if(DockCanvasView_setZoom(p->canvasview, n))
		mWidgetUpdate(M_WIDGET(p));
}

/** ポインタ移動時 */

static void _event_motion(DockCanvasViewArea *p,mEvent *ev)
{
	switch(p->optype)
	{
		//ビュー内スクロール
		case _OPTYPE_VIEW_SCROLL:
			p->canvasview->ptScroll.x += p->lastx - ev->pt.x;
			p->canvasview->ptScroll.y += p->lasty - ev->pt.y;

			DockCanvasView_adjustScroll(p->canvasview);

			mWidgetUpdate(M_WIDGET(p));
			break;

		//キャンバススクロール
		case _OPTYPE_CANVAS_SCROLL:
			_canvas_scroll(p, ev->pt.x, ev->pt.y);
			break;

		//倍率変更
		case _OPTYPE_VIEW_ZOOM:
			_motion_zoom(p, ev->pt.y);
			break;
	}

	p->lastx = ev->pt.x;
	p->lasty = ev->pt.y;
}

/** ドラッグ解除 */

static void _release_drag(DockCanvasViewArea *p)
{
	if(p->dragbtt)
	{
		p->dragbtt = 0;

		mWidgetUngrabPointer(M_WIDGET(p));
		mWidgetUpdate(M_WIDGET(p));

		//キャンバス、元の品質に戻す

		if(p->optype == _OPTYPE_CANVAS_SCROLL)
		{
			drawCanvas_normalQuality();
			drawUpdate_canvasArea();
		}
	}
}

/** イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	DockCanvasViewArea *p = _CVAREA(wg);

	switch(ev->type)
	{
		case MEVENT_POINTER:
			if(ev->pt.type == MEVENT_POINTER_TYPE_MOTION)
			{
				//移動

				if(p->dragbtt)
					_event_motion(p, ev);
			}
			else if(ev->pt.type == MEVENT_POINTER_TYPE_PRESS)
			{
				//押し (ルーペ時は無効)

				if(!p->dragbtt && !_IS_LOUPE_MODE)
					_event_press(p, ev);
			}
			else if(ev->pt.type == MEVENT_POINTER_TYPE_RELEASE)
			{
				//離し

				if(ev->pt.btt == p->dragbtt)
					_release_drag(p);
			}
			break;

		case MEVENT_FOCUS:
			if(ev->focus.bOut)
				_release_drag(p);
			break;
	}

	return 1;
}

/** サイズ変更時 */

static void _onsize_handle(mWidget *wg)
{
	DockCanvasView_setZoom_fit(_CVAREA(wg)->canvasview);
	DockCanvasView_adjustScroll(_CVAREA(wg)->canvasview);
}


//==========================
// 描画
//==========================


/** ルーペ時の十字線描画 */

static void _draw_loupe_cross(DockCanvasViewArea *p,
	DockCanvasView *cv,mPixbuf *pixbuf,mBox *box)
{
	double d;

	if(cv->zoom >= 400)
	{
		//400% 以上の場合、破線

		d = -0.5 * cv->dscale;

		pixbufDraw_cross_dash(pixbuf,
			(p->wg.w >> 1) + d + 0.5, (p->wg.h >> 1) + d + 0.5,
			cv->zoom / 100, box);
	}
	else
	{
		//400% 未満の場合、1px 合成線

		mPixbufLineH_blend(pixbuf, box->x, p->wg.h >> 1, box->w, 0x0000ff, 100);
		mPixbufLineV_blend(pixbuf, p->wg.w >> 1, box->y, box->h, 0x0000ff, 100);
	}
}

/** キャンバス描画 */

static void _draw_canvas(DockCanvasViewArea *p,mPixbuf *pixbuf,mBox *box)
{
	DockCanvasView *cv = p->canvasview;
	CanvasDrawInfo info;
	CanvasViewParam param;

	info.boxdst = *box;
	info.originx = cv->originX;
	info.originy = cv->originY;
	info.scrollx = cv->ptScroll.x - p->wg.w / 2;
	info.scrolly = cv->ptScroll.y - p->wg.h / 2;
	info.mirror = 0;
	info.imgw = APP_DRAW->imgw;
	info.imgh = APP_DRAW->imgh;
	info.bkgndcol = APP_CONF->colCanvasBkgnd;
	info.param = &param;

	param.scalediv = cv->dscalediv;

	if(cv->zoom < 100 && p->dragbtt == 0)
		//縮小時 (ドラッグ時は除く)
		ImageBufRGB16_drawMainCanvas_oversamp(APP_DRAW->blendimg, pixbuf, &info);
	else
		ImageBufRGB16_drawMainCanvas_nearest(APP_DRAW->blendimg, pixbuf, &info);
}

/** (固定表示用) 範囲更新 */

void DockCanvasViewArea_drawRect(DockCanvasViewArea *p,mBox *box)
{
	mPixbuf *pixbuf;

	pixbuf = mWidgetBeginDirectDraw(M_WIDGET(p));

	if(pixbuf)
	{
		_draw_canvas(p, pixbuf, box);
	
		mWidgetEndDirectDraw(M_WIDGET(p), pixbuf);

		mWidgetUpdateBox_box(M_WIDGET(p), box);
	}
}

/** 描画ハンドラ */

static void _draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	DockCanvasViewArea *p = _CVAREA(wg);
	mBox box;

	box.x = box.y = 0;
	box.w = wg->w;
	box.h = wg->h;

	_draw_canvas(p, pixbuf, &box);

	//ルーペ十字線

	if(_IS_LOUPE_MODE)
		_draw_loupe_cross(p, p->canvasview, pixbuf, &box);
}


//==========================
// main
//==========================


/** 作成 */

DockCanvasViewArea *DockCanvasViewArea_new(DockCanvasView *canvasview,mWidget *parent)
{
	DockCanvasViewArea *p;

	p = (DockCanvasViewArea *)mWidgetNew(sizeof(DockCanvasViewArea), parent);

	p->wg.event = _event_handle;
	p->wg.draw = _draw_handle;
	p->wg.onSize = _onsize_handle;

	p->wg.fLayout = MLF_EXPAND_WH;
	p->wg.fEventFilter |= MWIDGET_EVENTFILTER_POINTER;

	p->canvasview = canvasview;

	return p;
}
