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
 * DockImageViewerArea
 * 
 * [dock] イメージビューアの表示エリア
 *****************************************/

#include "mDef.h"
#include "mWidgetDef.h"
#include "mWidget.h"
#include "mEvent.h"

#include "defConfig.h"

#include "ImageBuf24.h"
#include "DockImageViewer.h"

#include "draw_main.h"
#include "Docks_external.h"


//-------------------

typedef struct _DockImageViewerArea
{
	mWidget wg;

	DockImageViewer *imgviewer;
	int dragbtt,
		optype,
		lastx,lasty;
}DockImageViewerArea;

//-------------------

#define _IS_ZOOM_FIT  (APP_CONF->imageviewer_flags & CONFIG_IMAGEVIEWER_F_FIT)

#define _IMGVAREA(p)  ((DockImageViewerArea *)(p))
#define _BKGND_COL    0xb0b0b0

enum
{
	_OPTYPE_SCROLL,
	_OPTYPE_ZOOM,
	_OPTYPE_SPOIT_DRAWCOL,
	_OPTYPE_SPOIT_BKGNDCOL
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
		//左ボタン (+装飾キー)
		
		if(state & M_MODS_CTRL)
			no = 1;
		else if(state & M_MODS_SHIFT)
			no = 2;
		else
			no = 0;
	}
	else if(btt == M_BTT_RIGHT)
		//右ボタン
		no = 3;
	else if(btt == M_BTT_MIDDLE)
		no = 4;

	if(no < 0) return -1;

	//設定データ

	no = APP_CONF->imageviewer_btt[no];

	//全体表示時は、スクロールと倍率変更は無効

	if(_IS_ZOOM_FIT && (no == _OPTYPE_SCROLL || no == _OPTYPE_ZOOM))
		return -1;

	return no;
}

/** スポイト */

static void _spoit(DockImageViewerArea *p,mEvent *ev,mBool bkgndcol)
{
	DockImageViewer *imgv = p->imgviewer;
	int x,y;
	uint8_t *buf;
	uint32_t col;

	//イメージ位置

	x = ev->pt.x + imgv->scrx - (p->wg.w >> 1);
	y = ev->pt.y + imgv->scry - (p->wg.h >> 1);

	if(APP_CONF->imageviewer_flags & CONFIG_IMAGEVIEWER_F_MIRROR)
		x = -x;

	x = (int)(x * imgv->dscalediv + imgv->img->w * 0.5);
	y = (int)(y * imgv->dscalediv + imgv->img->h * 0.5);

	//色取得

	buf = ImageBuf24_getPixelBuf(p->imgviewer->img, x, y);
	if(!buf) return;

	col = M_RGB(buf[0], buf[1], buf[2]);

	//セット

	if(bkgndcol)
		drawColor_setBkgndColor(col);
	else
		drawColor_setDrawColor(col);

	DockColor_changeDrawColor();
}


//==========================
// イベント
//==========================


/** ボタン押し時 */

static void _event_press(DockImageViewerArea *p,mEvent *ev)
{
	int type;

	type = _get_optype_from_button(ev->pt.btt, ev->pt.state);
	if(type < 0) return;

	if(type == _OPTYPE_SCROLL || type == _OPTYPE_ZOOM)
	{
		//スクロール or 倍率変更

		p->dragbtt = ev->pt.btt;
		p->optype = type;
		p->lastx = ev->pt.x;
		p->lasty = ev->pt.y;

		mWidgetGrabPointer(M_WIDGET(p));
	}
	else
		//スポイト
		_spoit(p, ev, (type == _OPTYPE_SPOIT_BKGNDCOL));
}

/** ポインタ移動時 */

static void _event_motion(DockImageViewerArea *p,mEvent *ev)
{
	if(p->optype == _OPTYPE_SCROLL)
	{
		//スクロール
	
		p->imgviewer->scrx += p->lastx - ev->pt.x;
		p->imgviewer->scry += p->lasty - ev->pt.y;

		DockImageViewer_adjustScroll(p->imgviewer);

		mWidgetUpdate(M_WIDGET(p));
	}
	else
	{
		//倍率変更
		/* 100% 以下は 2%、100% 以上は 10% 単位で上下 */

		int n,dir;

		dir = p->lasty - ev->pt.y;

		if(dir)
		{
			n = p->imgviewer->zoom;
			n += dir * ((n < 100)? 2: 10);

			if(n < 2) n = 2;
			else if(n > 1000) n = 1000;

			if(n < 100)
				n &= ~1;
			else
				n = n / 10 * 10;

			if(DockImageViewer_setZoom(p->imgviewer, n))
				mWidgetUpdate(M_WIDGET(p));
		}
	}

	p->lastx = ev->pt.x;
	p->lasty = ev->pt.y;
}

/** ドラッグ解除 */

static void _release_drag(DockImageViewerArea *p)
{
	if(p->dragbtt)
	{
		mWidgetUngrabPointer(M_WIDGET(p));
		p->dragbtt = 0;

		mWidgetUpdate(M_WIDGET(p));
	}
}

/** イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	DockImageViewerArea *p = _IMGVAREA(wg);

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
				//押し

				if(p->dragbtt == 0 && p->imgviewer->img)
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


//==========================
// ハンドラ
//==========================


/** サイズ変更時 */

static void _onsize_handle(mWidget *wg)
{
	DockImageViewer_setZoom_fit(_IMGVAREA(wg)->imgviewer);
}

/** 描画 */

static void _draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	DockImageViewerArea *p = _IMGVAREA(wg);
	DockImageViewer *imgv = p->imgviewer;

	if(!imgv->img)
		mWidgetDrawBkgnd(wg, NULL);
	else
	{
		ImageBuf24CanvasInfo info;
		mBox box;

		box.x = box.y = 0;
		box.w = wg->w;
		box.h = wg->h;

		info.originx = imgv->img->w * 0.5;
		info.originy = imgv->img->h * 0.5;
		info.scrollx = imgv->scrx - (wg->w >> 1);
		info.scrolly = imgv->scry - (wg->h >> 1);
		info.scalediv = imgv->dscalediv;
		info.mirror = APP_CONF->imageviewer_flags & CONFIG_IMAGEVIEWER_F_MIRROR;
		info.bkgndcol = _BKGND_COL;

		if(imgv->zoom < 100 && p->dragbtt == 0)
			//縮小時 (ドラッグ中は除く)
			ImageBuf24_drawCanvas_oversamp(imgv->img, pixbuf, &box, &info);
		else
			ImageBuf24_drawCanvas_nearest(imgv->img, pixbuf, &box, &info);
	}
}

/** D&D */

static int _dnd_handle(mWidget *wg,char **files)
{
	//画像読み込み
	DockImageViewer_loadImage(_IMGVAREA(wg)->imgviewer, *files);

	return 1;
}


//==========================
// main
//==========================


/** 作成 */

DockImageViewerArea *DockImageViewerArea_new(DockImageViewer *imgviewer,mWidget *parent)
{
	DockImageViewerArea *p;

	p = (DockImageViewerArea *)mWidgetNew(sizeof(DockImageViewerArea), parent);

	p->imgviewer = imgviewer;

	p->wg.event = _event_handle;
	p->wg.draw = _draw_handle;
	p->wg.onSize = _onsize_handle;
	p->wg.onDND = _dnd_handle;

	p->wg.fState |= MWIDGET_STATE_ENABLE_DROP;
	p->wg.fLayout = MLF_EXPAND_WH;
	p->wg.fEventFilter |= MWIDGET_EVENTFILTER_POINTER;

	return p;
}
