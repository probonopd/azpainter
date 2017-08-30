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
 * HLS パレット
 *****************************************/

#include "mDef.h"
#include "mWidgetDef.h"
#include "mWidget.h"
#include "mSysCol.h"
#include "mPixbuf.h"
#include "mEvent.h"
#include "mColorConv.h"

#include "defDraw.h"


//---------------------

typedef struct
{
	mWidget wg;

	mPixbuf *img;
	int fdrag;
}HLSPal;

//---------------------

#define _TOP_H    10
#define _TOP_W    6
#define _BETWEEN  8
#define _PAL_W    12
#define _PAL_H    8
#define _PAL_XNUM 15
#define _PAL_YNUM 10
#define _PAL_TOPY  (_TOP_H + _BETWEEN)

#define _HUE_STEP      30
#define _HUE_STEP_VAL  (360 / _HUE_STEP)

#define _GET_L(x)   (0.08 + (x) * 0.06)
#define _GET_S(y)   (1 - (y) * 0.1)

enum
{
	_DRAGF_TOP = 1,
	_DRAGF_PALETTE
};

//---------------------


//========================
// 描画
//========================


/** H カーソル描画 */

static void _draw_cursor_top(HLSPal *p,mBool erase)
{
	mPixbufDrawArrowUp(p->img,
		1 + APP_DRAW->col.hlspal_sel * _TOP_W + _TOP_W / 2 - 1, _TOP_H + 1,
		(erase)? MSYSCOL(FACE): MSYSCOL(TEXT));
}

/** パレットカーソル描画 */

static void _draw_cursor_pal(HLSPal *p,mBool erase)
{
	mPixbufBox(p->img,
		APP_DRAW->col.hlspal_palx * _PAL_W,
		_PAL_TOPY + APP_DRAW->col.hlspal_paly * _PAL_H,
		_PAL_W + 1, _PAL_H + 1,
		(erase)? 0: MSYSCOL(WHITE));
}

/** パレット色の描画 */

static void _draw_palette(HLSPal *p)
{
	mPixbuf *img = p->img;
	int ix,iy,x,y,c[3],hue;
	double ds;

	hue = APP_DRAW->col.hlspal_sel * _HUE_STEP_VAL;

	for(iy = 0, y = _PAL_TOPY + 1; iy < _PAL_YNUM; iy++, y += _PAL_H)
	{
		ds = _GET_S(iy);
		
		for(ix = 0, x = 1; ix < _PAL_XNUM; ix++, x += _PAL_W)
		{
			mHLStoRGB(hue, _GET_L(ix), ds, c);
			mPixbufFillBox(img, x, y, _PAL_W - 1, _PAL_H - 1, mRGBtoPix2(c[0], c[1], c[2]));
		}
	}
}

/** 最初の描画 */

static void _draw_first(HLSPal *p)
{
	mPixbuf *img = p->img;
	int w = p->wg.hintW,i,n,h,c[3];

	//H 部分の枠

	mPixbufBox(img, 0, 0, w, _TOP_H, 0);

	for(i = _HUE_STEP - 1, n = _TOP_W; i; i--, n += _TOP_W)
		mPixbufLineV(img, n, 1, _TOP_H - 2, 0);

	//H 色

	for(i = 0, n = 1, h = 0; i < _HUE_STEP; i++, n += _TOP_W, h += _HUE_STEP_VAL)
	{
		mHLStoRGB(h, 0.5, 1.0, c);
		mPixbufFillBox(img, n, 1, _TOP_W - 1, _TOP_H - 2, mRGBtoPix2(c[0], c[1], c[2]));
	}

	//H とパレットの間

	mPixbufFillBox(img, 0, _TOP_H, w, _BETWEEN, MSYSCOL(FACE));

	//パレットの枠

	for(i = _PAL_XNUM + 1, n = 0; i; i--, n += _PAL_W)
		mPixbufLineV(img, n, _PAL_TOPY, _PAL_H * _PAL_YNUM + 1, 0);

	for(i = _PAL_YNUM + 1, n = _PAL_TOPY; i; i--, n += _PAL_H)
		mPixbufLineH(img, 0, n, _PAL_W * _PAL_XNUM + 1, 0);

	//パレット色、カーソル

	_draw_palette(p);
	_draw_cursor_top(p, FALSE);
	_draw_cursor_pal(p, FALSE);
}


//========================
// sub
//========================


/** 選択色を通知 */

static void _notify(HLSPal *p)
{
	uint32_t col;

	col = mHLStoRGB_pac(APP_DRAW->col.hlspal_sel * _HUE_STEP_VAL,
		_GET_L(APP_DRAW->col.hlspal_palx),
		_GET_S(APP_DRAW->col.hlspal_paly));

	/* DockColorPalette のタブ内容のメインコンテナに直接通知。
	 * 通知元は HLS パレットのタブ内容コンテナ。 */

	mWidgetAppendEvent_notify(MWIDGET_NOTIFYWIDGET_RAW, p->wg.parent, 0, col, 0);
}

/** HUE 選択 */

static void _on_motion_top(HLSPal *p,int x)
{
	x = (x - 1) / _TOP_W;

	if(x < 0) x = 0;
	else if(x >= _HUE_STEP) x = _HUE_STEP - 1;

	if(x != APP_DRAW->col.hlspal_sel)
	{
		_draw_cursor_top(p, TRUE);
		
		APP_DRAW->col.hlspal_sel = x;

		_draw_cursor_top(p, FALSE);
		_draw_palette(p);

		mWidgetUpdate(M_WIDGET(p));

		_notify(p);
	}
}

/** パレット選択 */

static void _on_motion_palette(HLSPal *p,int x,int y)
{
	//パレット位置

	x = (x - 1) / _PAL_W;
	y = (y - _PAL_TOPY - 1) / _PAL_H;

	if(x < 0) x = 0; else if(x >= _PAL_XNUM) x = _PAL_XNUM - 1;
	if(y < 0) y = 0; else if(y >= _PAL_YNUM) y = _PAL_YNUM - 1;

	//前回位置と比較

	if(x != APP_DRAW->col.hlspal_palx || y != APP_DRAW->col.hlspal_paly)
	{
		//カーソル

		_draw_cursor_pal(p, TRUE);

		APP_DRAW->col.hlspal_palx = x;
		APP_DRAW->col.hlspal_paly = y;

		_draw_cursor_pal(p, FALSE);

		mWidgetUpdate(M_WIDGET(p));

		//色を通知

		_notify(p);
	}
}


//========================
// ハンドラ
//========================


/** 押し時 */

static void _event_press(HLSPal *p,int x,int y)
{
	if(y < _PAL_TOPY)
	{
		//HUE

		p->fdrag = _DRAGF_TOP;

		_on_motion_top(p, x);
	}
	else
	{
		//パレット

		p->fdrag = _DRAGF_PALETTE;

		_on_motion_palette(p, x, y);
	}

	mWidgetGrabPointer(M_WIDGET(p));
}

/** グラブ解除 */

static void _grab_release(HLSPal *p)
{
	if(p->fdrag)
	{
		p->fdrag = 0;
		mWidgetUngrabPointer(M_WIDGET(p));
	}
}

/** イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	HLSPal *p = (HLSPal *)wg;

	switch(ev->type)
	{
		case MEVENT_POINTER:
			if(ev->pt.type == MEVENT_POINTER_TYPE_MOTION)
			{
				//移動

				if(p->fdrag == _DRAGF_PALETTE)
					_on_motion_palette(p, ev->pt.x, ev->pt.y);
				else if(p->fdrag == _DRAGF_TOP)
					_on_motion_top(p, ev->pt.x);
			}
			else if(ev->pt.type == MEVENT_POINTER_TYPE_PRESS)
			{
				//押し
				
				if(ev->pt.btt == M_BTT_LEFT && !p->fdrag)
					_event_press(p, ev->pt.x, ev->pt.y);
			}
			else if(ev->pt.type == MEVENT_POINTER_TYPE_RELEASE)
			{
				//離し

				if(ev->pt.btt == M_BTT_LEFT)
					_grab_release(p);
			}
			break;

		case MEVENT_FOCUS:
			if(ev->focus.bOut)
				_grab_release(p);
			break;
	}

	return 1;
}

/** 描画 */

static void _draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	mPixbufBlt(pixbuf, 0, 0, ((HLSPal *)wg)->img, 0, 0, -1, -1);
}

/** 破棄 */

static void _destroy_handle(mWidget *wg)
{
	mPixbufFree(((HLSPal *)wg)->img);
}

/** 作成 */

mWidget *DockColorPalette_HLSPalette_new(mWidget *parent)
{
	HLSPal *p;

	p = (HLSPal *)mWidgetNew(sizeof(HLSPal), parent);

	p->wg.fEventFilter |= MWIDGET_EVENTFILTER_POINTER;
	p->wg.hintW = _TOP_W * _HUE_STEP + 1;
	p->wg.hintH = _TOP_H + _BETWEEN + _PAL_H * _PAL_YNUM + 1;
	p->wg.destroy = _destroy_handle;
	p->wg.draw = _draw_handle;
	p->wg.event = _event_handle;

	//イメージ作成

	p->img = mPixbufCreate(p->wg.hintW, p->wg.hintH);

	_draw_first(p);

	return (mWidget *)p;
}

/** テーマ変更時 */

void DockColorPalette_HLSPalette_changeTheme(mWidget *wg)
{
	_draw_first((HLSPal *)wg);
}
