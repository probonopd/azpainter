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
 * ColorWheel
 *****************************************/

#include <math.h>

#include "mDef.h"
#include "mWidgetDef.h"
#include "mWidget.h"
#include "mPixbuf.h"
#include "mEvent.h"
#include "mSysCol.h"
#include "mColorConv.h"
#include "mMenu.h"
#include "mTrans.h"

#include "ColorWheel.h"

#include "trgroup.h"


//------------------

struct _ColorWheel
{
	mWidget wg;

	mPixbuf *img;
	int type,
		fdrag;
	mPoint pt_sv;		//SV カーソル位置
	double hue,sat,val;	//HSV 値
};

//------------------

#define _CENTER    86
#define _WGSIZE    (_CENTER * 2 + 1)

#define _HUE_RADIUS_OUT     (_CENTER - 1)
#define _HUE_RADIUS_IN      70
#define _HUE_RADIUS_CURSOR  (_HUE_RADIUS_IN + (_HUE_RADIUS_OUT - _HUE_RADIUS_IN) / 2)

#define _TRI_SV_RADIUS    (_HUE_RADIUS_IN - 2)
#define _TRI_SV_TOP       (_CENTER - _HUE_RADIUS_IN)
#define _TRI_SV_BOTTOM    (_CENTER + _HUE_RADIUS_IN)

#define _RECT_SV_RADIUS   47
#define _RECT_SV_SIZE     (_RECT_SV_RADIUS * 2 + 1)
#define _RECT_SV_TOP      (_CENTER - _RECT_SV_RADIUS)

#define _BTTSIZE   17


enum
{
	_DRAG_NONE,
	_DRAG_HUE,
	_DRAG_SV
};

enum
{
	TRID_MENU_HSV_TRIANGLE,
	TRID_MENU_HSV_RECTANGLE
};

//------------------

static const uint8_t g_cursor_img[] = {
0x38,0x00,0x44,0x00,0x82,0x00,0x01,0x01,0x01,0x01,0x01,0x01,0x82,0x00,0x44,0x00,
0x38,0x00 };

//------------------


//=========================
// 描画
//=========================


/** SV (三角形) 描画 */

static void _draw_sv_triangle(ColorWheel *p)
{
	mPixbuf *img = p->img;
	int x,y,bpp,pitch;
	double dx,dy,d,sqrt3;
	uint32_t col;
	uint8_t *pd;

	sqrt3 = sqrt(3);

	pd = mPixbufGetBufPtFast(img, _TRI_SV_TOP, _TRI_SV_TOP);
	bpp = img->bpp;
	pitch = img->pitch_dir - bpp * (_TRI_SV_BOTTOM - _TRI_SV_TOP + 1);

	for(y = _TRI_SV_TOP; y <= _TRI_SV_BOTTOM; y++)
	{
		//半径にルート3を掛けると辺の長さ
		dy = (double)(y - _CENTER) / _TRI_SV_RADIUS * sqrt3;
	
		for(x = _TRI_SV_TOP; x <= _TRI_SV_BOTTOM; x++, pd += bpp)
		{
			//三角形の外接円の半径を 1 とするので、-1.0〜1.0 となる
			dx = (double)(x - _CENTER) / _TRI_SV_RADIUS;

			if(dx <= 1				//中心-右方向は半径 1.0 が最大
				&& dx >= -0.5		//中心-左方向は半径 -0.5 が最大
				&& dx + dy <= 1
				&& dx - dy <= 1)
			{
				d = dx - dy + 2;
				
				col = mHSVtoRGB_pac(p->hue,
					(1 + dx * 2) / d, d / 3);

				(img->setbuf)(pd, mRGBtoPix(col));
			}
		}

		pd += pitch;
	}
}

/** SV (四角形) 描画 */

static void _draw_sv_rectangle(ColorWheel *p)
{
	mPixbuf *img = p->img;
	int x,y,bpp,pitch,size;
	uint32_t col;
	uint8_t *pd;
	double val,div;

	size = _RECT_SV_SIZE;

	pd = mPixbufGetBufPtFast(img, _RECT_SV_TOP, _RECT_SV_TOP);
	bpp = img->bpp;
	pitch = img->pitch_dir - bpp * size;

	div = size - 1;

	for(y = 0; y < size; y++)
	{
		val = (size - 1 - y) / div;
	
		for(x = 0; x < size; x++, pd += bpp)
		{
			col = mHSVtoRGB_pac(p->hue, x / div, val);

			(img->setbuf)(pd, mRGBtoPix(col));
		}

		pd += pitch;
	}
}

/** SV 描画 */

static void _draw_sv(ColorWheel *p)
{
	if(p->type == COLORWHEEL_TYPE_HSV_TRIANGLE)
		_draw_sv_triangle(p);
	else
		_draw_sv_rectangle(p);
}

/** H カーソル描画 */

static void _draw_cursor_hue(ColorWheel *p)
{
	double d;
	int x,y;

	d = p->hue * M_MATH_PI * 2;

	x = (int)(_HUE_RADIUS_CURSOR * cos(d) + _CENTER + 0.5);
	y = (int)(_HUE_RADIUS_CURSOR * sin(d) + _CENTER + 0.5);

	mPixbufDrawBitPattern(p->img, x - 4, y - 4, g_cursor_img, 9, 9, MPIXBUF_COL_XOR);
}

/** SV カーソル描画 */

static void _draw_cursor_sv(ColorWheel *p)
{
	mPixbufDrawBitPattern(p->img,
		p->pt_sv.x - 4, p->pt_sv.y - 4, g_cursor_img, 9, 9, MPIXBUF_COL_XOR);
}

/** 初期描画 */

static void _init_draw(ColorWheel *p)
{
	mPixbuf *img = p->img;
	int x,y,xx,yy,bpp,pitch;
	double r,rd;
	uint32_t col;
	uint8_t *pd;

	//背景

	mPixbufFillBox(img, 0, 0, _WGSIZE, _WGSIZE, MSYSCOL(FACE));

	//ボタン

	mPixbufBox(img, _WGSIZE - _BTTSIZE, 0, _BTTSIZE, _BTTSIZE, MSYSCOL(FRAME));
	mPixbufFillBox(img, _WGSIZE - _BTTSIZE + 1, 1, _BTTSIZE - 2, _BTTSIZE - 2, MSYSCOL(FACE_DARK));
	mPixbufDrawArrowDown(img, _WGSIZE - _BTTSIZE + _BTTSIZE / 2, _BTTSIZE / 2, MSYSCOL(TEXT));

	//------ 色相

	pd = mPixbufGetBufPtFast(img, 0, 0);
	bpp = img->bpp;
	pitch = img->pitch_dir - bpp * _WGSIZE;

	for(y = 0; y < _WGSIZE; y++)
	{
		yy = y - _CENTER;
		yy *= yy;
	
		for(x = 0; x < _WGSIZE; x++)
		{
			//半径
			
			xx = x - _CENTER;
			
			r = sqrt(xx * xx + yy);

			//描画

			if(r >= _HUE_RADIUS_IN && r < _HUE_RADIUS_OUT)
			{
				//角度

				rd = atan2(y - _CENTER, xx);

				if(rd < 0)
					rd += M_MATH_PI * 2;

				//色

				col = mHSVtoRGB_pac(rd / (M_MATH_PI * 2), 1, 1);

				(img->setbuf)(pd, mRGBtoPix(col));
			}

			pd += bpp;
		}

		pd += pitch;
	}

	//SV

	_draw_sv(p);

	//カーソル
	
	_draw_cursor_hue(p);
	_draw_cursor_sv(p);
}


//=========================
// sub
//=========================


/** 色変更の通知 */

static void _notify(ColorWheel *p)
{
	int h,s,v;

	h = (int)(p->hue * 360 + 0.5) % 360;
	s = (int)(p->sat * 255 + 0.5);
	v = (int)(p->val * 255 + 0.5);

	mWidgetAppendEvent_notify(NULL, M_WIDGET(p),
		COLORWHEEL_N_CHANGE_COL,
		mHSVtoRGB_pac(p->hue, p->sat, p->val),
		(h << 16) | (s << 8) | v);
}

/** 色から SV カーソル位置計算 */

static void _set_sv_curpos(ColorWheel *p)
{
	double d;

	if(p->type == COLORWHEEL_TYPE_HSV_TRIANGLE)
	{
		d = (p->sat * p->val * 3 - 1) / 2;
	
		p->pt_sv.x = (int)(d * _TRI_SV_RADIUS + _CENTER + 0.5);
		p->pt_sv.y = (int)((d + 2 - p->val * 3) / sqrt(3) * _TRI_SV_RADIUS + _CENTER + 0.5);
	}
	else
	{
		p->pt_sv.x = (int)((p->sat - 0.5) * 2 * _RECT_SV_RADIUS + _CENTER + 0.5);
		p->pt_sv.y = (int)((0.5 - p->val) * 2 * _RECT_SV_RADIUS + _CENTER + 0.5);
	}
}


//=========================
// ポインタ操作
//=========================


/** 色相操作 */

static void _pointer_hue(ColorWheel *p,int xx,int yy)
{
	double rd;

	//角度

	rd = atan2(yy, xx);

	if(rd < 0)
		rd += M_MATH_PI * 2;

	//カーソル消去

	_draw_cursor_hue(p);
	_draw_cursor_sv(p);

	//H

	p->hue = rd / (M_MATH_PI * 2);

	//SV

	_draw_sv(p);

	_draw_cursor_sv(p);
	_draw_cursor_hue(p);

	mWidgetUpdate(M_WIDGET(p));

	_notify(p);
}

/** SV 操作位置変更 */

static void _pointer_sv_change(ColorWheel *p,int x,int y)
{
	_draw_cursor_sv(p);

	p->pt_sv.x = x;
	p->pt_sv.y = y;

	_draw_cursor_sv(p);

	mWidgetUpdate(M_WIDGET(p));

	_notify(p);
}

/** SV 操作 (三角形) */

static void _pointer_sv_triangle(ColorWheel *p,int x,int y)
{
	double dx,dy,d,sqrt3,s,v;

	sqrt3 = sqrt(3);

	dx = (double)x / _TRI_SV_RADIUS;
	dy = (double)y / _TRI_SV_RADIUS * sqrt3;

	if(dx < -0.5)
		dx = -0.5;
	else if(dx > 1)
		dx = 1;

	if(dx + dy > 1)
		dy = 1 - dx;

	if(dx - dy > 1)
		dy = dx - 1;

	//

	d = dx - dy + 2;

	s = (1 + dx * 2) / d;
	v = d / 3;

	//SV 変更

	if(s != p->sat || v != p->val)
	{
		p->sat = s;
		p->val = v;

		_notify(p);
	}

	//位置変更

	x = (int)(dx * _TRI_SV_RADIUS + _CENTER + 0.5);
	y = (int)(dy / sqrt3 * _TRI_SV_RADIUS + _CENTER + 0.5);

	if(x != p->pt_sv.x || y != p->pt_sv.y)
	{
		_draw_cursor_sv(p);

		p->pt_sv.x = x;
		p->pt_sv.y = y;

		_draw_cursor_sv(p);

		mWidgetUpdate(M_WIDGET(p));
	}
}

/** SV 操作 (四角形) */

static void _pointer_sv_rectangle(ColorWheel *p,int x,int y)
{
	//位置調整

	if(x < -_RECT_SV_RADIUS)
		x = -_RECT_SV_RADIUS;
	else if(x > _RECT_SV_RADIUS)
		x = _RECT_SV_RADIUS;

	if(y < -_RECT_SV_RADIUS)
		y = -_RECT_SV_RADIUS;
	else if(y > _RECT_SV_RADIUS)
		y = _RECT_SV_RADIUS;

	x += _CENTER;
	y += _CENTER;

	//変更

	if(p->pt_sv.x != x || p->pt_sv.y != y)
	{
		p->sat = (double)(x - _CENTER) / _RECT_SV_RADIUS * 0.5 + 0.5;
		p->val = 1 - ((double)(y - _CENTER) / _RECT_SV_RADIUS * 0.5 + 0.5);

		_pointer_sv_change(p, x, y);
	}
}

/** SV 操作 */

static void _pointer_sv(ColorWheel *p,int xx,int yy)
{
	if(p->type == COLORWHEEL_TYPE_HSV_TRIANGLE)
		_pointer_sv_triangle(p, xx, yy);
	else
		_pointer_sv_rectangle(p, xx, yy);
}

/** メニュー */

static void _run_menu(ColorWheel *p)
{
	mMenu *menu;
	mMenuItemInfo *mi;
	int id;
	mPoint pt;

	M_TR_G(TRGROUP_DOCK_COLOR_WHEEL);

	menu = mMenuNew();

	mMenuAddTrTexts(menu, TRID_MENU_HSV_TRIANGLE, 2);
	mMenuSetCheck(menu, p->type, 1);

	pt.x = _WGSIZE;
	pt.y = _BTTSIZE;
	mWidgetMapPoint(M_WIDGET(p), NULL, &pt);
	
	mi = mMenuPopup(menu, NULL, pt.x, pt.y, MMENU_POPUP_F_RIGHT);
	id = (mi)? mi->id: -1;

	mMenuDestroy(menu);

	//

	if(id != -1)
	{
		p->type = id;

		_set_sv_curpos(p);
		_init_draw(p);

		mWidgetUpdate(M_WIDGET(p));

		mWidgetAppendEvent_notify(NULL, M_WIDGET(p),
			COLORWHEEL_N_CHANGE_TYPE, id, 0);
	}
}

/** 押し時 */

static void _on_press(ColorWheel *p,int x,int y)
{
	int xx,yy;
	double r;

	if(x >= _WGSIZE - _BTTSIZE && y < _BTTSIZE)
	{
		//ボタン

		_run_menu(p);
	}
	else
	{
		//半径

		xx = x - _CENTER;
		yy = y - _CENTER;
		
		r = sqrt(xx * xx + yy * yy);

		//

		if(r >= _HUE_RADIUS_IN && r < _HUE_RADIUS_OUT)
		{
			//色相内

			_pointer_hue(p, xx, yy);

			p->fdrag = _DRAG_HUE;
			mWidgetGrabPointer(M_WIDGET(p));
		}
		else if(r <= _TRI_SV_RADIUS)
		{
			//SV

			_pointer_sv(p, xx, yy);

			p->fdrag = _DRAG_SV;
			mWidgetGrabPointer(M_WIDGET(p));
		}
	}
}

/** 移動時 */

static void _on_motion(ColorWheel *p,int x,int y)
{
	x -= _CENTER;
	y -= _CENTER;

	if(p->fdrag == _DRAG_HUE)
		_pointer_hue(p, x, y);
	else
		_pointer_sv(p, x, y);
}

/** グラブ解放 */

static void _release_grab(ColorWheel *p)
{
	if(p->fdrag)
	{
		p->fdrag = _DRAG_NONE;
		mWidgetUngrabPointer(M_WIDGET(p));
	}
}


//=========================
//
//=========================


/** イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	ColorWheel *p = (ColorWheel *)wg;

	switch(ev->type)
	{
		case MEVENT_POINTER:
			if(ev->pt.type == MEVENT_POINTER_TYPE_MOTION)
			{
				if(p->fdrag)
					_on_motion(p, ev->pt.x, ev->pt.y);
			}
			else if(ev->pt.type == MEVENT_POINTER_TYPE_PRESS)
			{
				if(ev->pt.btt == M_BTT_LEFT && p->fdrag == _DRAG_NONE)
					_on_press(p, ev->pt.x, ev->pt.y);
			}
			else if(ev->pt.type == MEVENT_POINTER_TYPE_RELEASE)
			{
				if(ev->pt.btt == M_BTT_LEFT)
					_release_grab(p);
			}
			break;

		case MEVENT_FOCUS:
			if(ev->focus.bOut)
				_release_grab(p);
			break;
	}

	return 1;
}

/** 描画 */

static void _draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	ColorWheel *p = (ColorWheel *)wg;

	mPixbufBlt(pixbuf, 0, 0, p->img, 0, 0, _WGSIZE, _WGSIZE);
}

/** 破棄ハンドラ */

static void _destroy_handle(mWidget *wg)
{
	mPixbufFree(((ColorWheel *)wg)->img);
}

/** 作成 */

ColorWheel *ColorWheel_new(mWidget *parent,int id,int type,uint32_t col)
{
	ColorWheel *p;
	double d[3];
	
	p = (ColorWheel *)mWidgetNew(sizeof(ColorWheel), parent);
	if(!p) return NULL;

	p->wg.id = id;
	p->wg.destroy = _destroy_handle;
	p->wg.draw = _draw_handle;
	p->wg.event = _event_handle;
	p->wg.fEventFilter |= MWIDGET_EVENTFILTER_POINTER;
	p->wg.fLayout = MLF_FIX_WH;
	p->wg.w = p->wg.h = _WGSIZE;

	p->type = type;

	//初期色

	mRGBtoHSV_pac(col, d);

	p->hue = d[0];
	p->sat = d[1];
	p->val = d[2];

	_set_sv_curpos(p);

	//イメージ

	p->img = mPixbufCreate(_WGSIZE, _WGSIZE);

	_init_draw(p);
	
	return p;
}

/** 色の変更
 *
 * @param hsv  TRUE でHSV値 [9:8:8bit (360:255:255)] */

void ColorWheel_setColor(ColorWheel *p,int col,mBool hsv)
{
	double d[3];

	_draw_cursor_hue(p);
	_draw_cursor_sv(p);

	//HSV

	if(hsv)
	{
		p->hue = (col >> 16) / 360.0;
		p->sat = ((col >> 8) & 255) / 255.0;
		p->val = (col & 255) / 255.0;
	}
	else
	{
		mRGBtoHSV_pac(col, d);

		p->hue = d[0];
		p->sat = d[1];
		p->val = d[2];
	}

	_set_sv_curpos(p);

	//

	_draw_sv(p);

	_draw_cursor_hue(p);
	_draw_cursor_sv(p);

	mWidgetUpdate(M_WIDGET(p));
}

/** テーマ変更時の再描画 */

void ColorWheel_redraw(ColorWheel *p)
{
	_init_draw(p);
}
