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
 * PressureWidget
 *
 * 筆圧補正操作ウィジェット
 *****************************************/

#include <math.h>

#include "mDef.h"
#include "mWidgetDef.h"
#include "mWidget.h"
#include "mPixbuf.h"
#include "mEvent.h"
#include "mSysCol.h"
#include "mUtilStr.h"

#include "PressureWidget.h"


//-------------------------

struct _PressureWidget
{
	mWidget wg;

	int type,
		curve,		//曲線値
		fpress,
		ctlpt;		//操作中の点
	mPoint pt[2];	//線形の点 (x=in, y=out) [0-100]
};

#define _AREASIZE 101
#define _WGSIZE   (_AREASIZE + 2)

#define _SPIN_X  79
#define _SPIN_Y  89
#define _SPIN_SIZE  11
#define _SPIN_W  (_SPIN_SIZE * 2 - 1)

#define _CURVE_MIN  1
#define _CURVE_MAX  600

//-------------------------


//========================
// sub
//========================


/** 通知 */

static void _notify(PressureWidget *p)
{
	mWidgetAppendEvent_notify(NULL, M_WIDGET(p), 0, 0, 0);
}

/** [曲線] ポインタ位置から値セット */

static void _curve_setvalue_atpos(PressureWidget *p,int x,int y)
{
	int len,val,max;

	x -= _WGSIZE >> 1;
	y -= _WGSIZE >> 1;

	//中心からの距離
	len = (int)(sqrt(x * x + y * y) + 0.5);

	max = _WGSIZE - 10;
	if(len > max) len = max;

	//値

	if(x < 0 || y < 0)
		val = 100 - (100 - 1) * len / max;
	else
		val = (600 - 100) * len / max + 100;

	//中央に近ければ 100 に補正

	if(val >= 98 && val <= 102) val = 100;

	//変更

	if(val != p->curve)
	{
		p->curve = val;

		mWidgetUpdate(M_WIDGET(p));
	}
}

/** [曲線] ボタン押し時 */

static void _curve_onpress(PressureWidget *p,mEvent *ev)
{
	int x,y,n;

	x = ev->pt.x;
	y = ev->pt.y;

	if(ev->pt.btt == M_BTT_RIGHT
		|| (ev->pt.btt == M_BTT_LEFT && (ev->pt.state & M_MODS_CTRL)))
	{
		//右ボタン or Ctrl+左 : リセット

		if(p->curve != 100)
		{
			p->curve = 100;
			mWidgetUpdate(M_WIDGET(p));

			_notify(p);
		}
	}
	else if(ev->pt.btt == M_BTT_LEFT)
	{
		//左ボタン

		if(x >= _SPIN_X && x < _SPIN_X + _SPIN_W
			&& y >= _SPIN_Y && y < _SPIN_Y + _SPIN_SIZE)
		{
			//スピン

			n = p->curve;
			n += (x >= _SPIN_X + _SPIN_SIZE)? 1: -1;

			if(n < _CURVE_MIN) n = _CURVE_MIN;
			else if(n > _CURVE_MAX) n = _CURVE_MAX;

			if(n != p->curve)
			{
				p->curve = n;
				mWidgetUpdate(M_WIDGET(p));

				_notify(p);
			}
		}
		else
		{
			//曲線
			
			_curve_setvalue_atpos(p, x, y);

			p->fpress = TRUE;
			mWidgetGrabPointer(M_WIDGET(p));
		}
	}
}


/** [線形] ポインタ位置を筆圧の座標値に変換 */

static void _linear_area_to_pressure(mPoint *pt,int x,int y)
{
	if(x < 1) x = 1; else if(x > _AREASIZE) x = _AREASIZE;
	if(y < 1) y = 1; else if(y > _AREASIZE) y = _AREASIZE;

	pt->x = x - 1;
	pt->y = _AREASIZE - 1 - (y - 1);
}

/** [線形] 2点時の筆圧座標の調整 */

static void _linear_adjust(PressureWidget *p,mPoint *pt)
{
	if(p->ctlpt == 0)
	{
		if(pt->x > p->pt[1].x) pt->x = p->pt[1].x;
		if(pt->y > p->pt[1].y) pt->y = p->pt[1].y;
	}
	else
	{
		if(pt->x < p->pt[0].x) pt->x = p->pt[0].x;
		if(pt->y < p->pt[0].y) pt->y = p->pt[0].y;
	}
}

/** [線形] ボタン押し時 */

static void _linear_onpress(PressureWidget *p,int x,int y)
{
	mPoint pt;
	int n1,n2;

	_linear_area_to_pressure(&pt, x, y);

	//操作する点の選択

	if(p->type == 1)
	{
		//1点時

		p->ctlpt = 0;
	}
	else
	{
		//2点時は点に近い方を選択

		n1 = (pt.x - p->pt[0].x) * (pt.x - p->pt[0].x) + (pt.y - p->pt[0].y) * (pt.y - p->pt[0].y);
		n2 = (pt.x - p->pt[1].x) * (pt.x - p->pt[1].x) + (pt.y - p->pt[1].y) * (pt.y - p->pt[1].y);

		if(n1 != n2)
			p->ctlpt = (n1 > n2);
		else
			p->ctlpt = (pt.x > p->pt[0].x || pt.y > p->pt[0].y);

		_linear_adjust(p, &pt);
	}

	//位置変更

	p->pt[p->ctlpt] = pt;

	mWidgetUpdate(M_WIDGET(p));

	//グラブ

	p->fpress = TRUE;
	mWidgetGrabPointer(M_WIDGET(p));
}

/** [線形] ポインタ移動時 */

static void _linear_onmotion(PressureWidget *p,int x,int y)
{
	mPoint pt,*ptd;

	_linear_area_to_pressure(&pt, x, y);

	//2点の場合、調整

	if(p->type == 2)
		_linear_adjust(p, &pt);

	//セット

	ptd = p->pt + p->ctlpt;

	if(pt.x != ptd->x || pt.y != ptd->y)
	{
		*ptd = pt;

		mWidgetUpdate(M_WIDGET(p));
	}
}


//========================


/** グラブ解除 */

static void _release_grab(PressureWidget *p)
{
	if(p->fpress)
	{
		p->fpress = FALSE;
		mWidgetUngrabPointer(M_WIDGET(p));

		_notify(p);
	}
}

/** イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	PressureWidget *p = (PressureWidget *)wg;

	switch(ev->type)
	{
		case MEVENT_POINTER:
			if(ev->pt.type == MEVENT_POINTER_TYPE_MOTION)
			{
				//移動

				if(p->fpress)
				{
					if(p->type == 0)
						_curve_setvalue_atpos(p, ev->pt.x, ev->pt.y);
					else
						_linear_onmotion(p, ev->pt.x, ev->pt.y);
				}
			}
			else if(ev->pt.type == MEVENT_POINTER_TYPE_PRESS
				|| ev->pt.type == MEVENT_POINTER_TYPE_DBLCLK)
			{
				//押し

				if(!p->fpress)
				{
					if(p->type == 0)
						_curve_onpress(p, ev);
					else
					{
						if(ev->pt.btt == M_BTT_LEFT)
							_linear_onpress(p, ev->pt.x, ev->pt.y);
					}
				}
			}
			else if(ev->pt.type == MEVENT_POINTER_TYPE_RELEASE)
			{
				//離し
				
				if(ev->pt.btt == M_BTT_LEFT)
					_release_grab(p);
			}
			break;
		
		case MEVENT_FOCUS:
			if(ev->focus.bOut)
				_release_grab(p);
			break;
		default:
			return FALSE;
	}

	return TRUE;
}


//====================
// 描画
//====================


/** 曲線 */

static void _draw_curve(PressureWidget *p,mPixbuf *pixbuf,mBool disable)
{
	int lasty,i,y;
	double gamma;
	mPixCol col;
	char m[12];

	//枠、背景

	mPixbufBox(pixbuf, 0, 0, _WGSIZE, _WGSIZE, (disable)? MSYSCOL(FRAME_LIGHT): 0);
	mPixbufFillBox(pixbuf, 1, 1, _AREASIZE, _AREASIZE, (disable)? MSYSCOL(FACE): MSYSCOL(WHITE));

	//補助線

	mPixbufLine(pixbuf, 1, _AREASIZE, _AREASIZE, 1, mGraytoPix(192));

	//曲線 (x = in, y = out)

	col = mRGBtoPix(0x0000ff);

	gamma = p->curve * 0.01;
	lasty = _WGSIZE - 2;

	for(i = 1; i < _AREASIZE; i++)
	{
		y = _WGSIZE - 2 - (int)(pow((double)i / (_AREASIZE - 1), gamma) * (_AREASIZE - 1) + 0.5);

		mPixbufLine(pixbuf, i, lasty, i + 1, y, col);

		lasty = y;
	}

	//スピンボタン

	mPixbufBox(pixbuf, _SPIN_X, _SPIN_Y, _SPIN_W, _SPIN_SIZE, 0);
	mPixbufLineV(pixbuf, _SPIN_X + _SPIN_SIZE - 1, _SPIN_Y + 1, _SPIN_SIZE - 2, 0);
	mPixbufDrawArrowLeft(pixbuf, _SPIN_X + _SPIN_SIZE / 2, _SPIN_Y + _SPIN_SIZE / 2, 0);
	mPixbufDrawArrowRight(pixbuf, _SPIN_X + _SPIN_SIZE / 2 + _SPIN_SIZE - 1, _SPIN_Y + _SPIN_SIZE / 2, 0);

	//数値

	mFloatIntToStr(m, p->curve, 2);

	mPixbufDrawNumber_5x7(pixbuf, 82, 79, m, 0);
}

/** 線形 */

static void _draw_linear(PressureWidget *p,mPixbuf *pixbuf,mBool disable)
{
	int type,n;
	mPixCol col;
	mPoint pt[4];

	type = p->type;

	//枠、背景

	mPixbufBox(pixbuf, 0, 0, _WGSIZE, _WGSIZE, (disable)? MSYSCOL(FRAME_LIGHT): 0);
	mPixbufFillBox(pixbuf, 1, 1, _AREASIZE, _AREASIZE, (disable)? MSYSCOL(FACE): MSYSCOL(WHITE));

	//補助線

	col = mGraytoPix(192);

	mPixbufLineH(pixbuf, 1, _WGSIZE >> 1, _AREASIZE, col);
	mPixbufLineV(pixbuf, _WGSIZE >> 1, 1, _AREASIZE, col);

	//線

	pt[0].x = 1, pt[0].y = _WGSIZE - 2;
	pt[1].x = p->pt[0].x + 1, pt[1].y = _WGSIZE - 2 - p->pt[0].y;

	n = 2;

	if(type == 2)
	{
		pt[2].x = p->pt[1].x + 1, pt[2].y = _WGSIZE - 2 - p->pt[1].y;
		n = 3; 
	}

	pt[n].x = _WGSIZE - 2, pt[n].y = 1;

	mPixbufLines(pixbuf, pt, n + 1, mRGBtoPix(0xff));

	//点

	col = mRGBtoPix(0xff0000);

	for(n = 0; n < type; n++)
		mPixbufFillBox(pixbuf, pt[n + 1].x - 2, pt[n + 1].y - 2, 5, 5, col);
}

/** 描画 */

static void _draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	PressureWidget *p = (PressureWidget *)wg;
	mBool disable;

	disable = !(wg->fState & MWIDGET_STATE_ENABLED);

	if(p->type == 0)
		_draw_curve(p, pixbuf, disable);
	else
		_draw_linear(p, pixbuf, disable);
}


//====================
// main
//====================


/** 作成 */

PressureWidget *PressureWidget_new(mWidget *parent,int id)
{
	PressureWidget *p;

	p = (PressureWidget *)mWidgetNew(sizeof(PressureWidget), parent);
	if(!p) return NULL;

	p->wg.id = id;
	p->wg.fEventFilter |= MWIDGET_EVENTFILTER_POINTER;
	p->wg.event = _event_handle;
	p->wg.draw = _draw_handle;
	p->wg.hintW = p->wg.hintH = _WGSIZE;

	p->curve = 100;

	return p;
}

/** 値をセット */

void PressureWidget_setValue(PressureWidget *p,int type,uint32_t val)
{
	p->type = type;

	if(type == 0)
		p->curve = val;
	else
	{
		p->pt[0].x = (uint8_t)val;
		p->pt[0].y = (uint8_t)(val >> 8);
		p->pt[1].x = (uint8_t)(val >> 16);
		p->pt[1].y = (uint8_t)(val >> 24);
	}

	mWidgetUpdate(M_WIDGET(p));
}

/** 値を取得 */

uint32_t PressureWidget_getValue(PressureWidget *p)
{
	uint32_t v;

	if(p->type == 0)
		v = p->curve;
	else
		v = p->pt[0].x | (p->pt[0].y << 8) | (p->pt[1].x << 16) | (p->pt[1].y << 24);

	return v;
}
