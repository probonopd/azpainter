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
 * TileImage
 *
 * ブラシ描画
 *****************************************/
/*
 * - TileImageDrawInfo::rgba_brush にブラシ用に描画色を指定しておく。
 */

#include <math.h>

#include "mDef.h"
#include "mRandXorShift.h"

#include "TileImage.h"
#include "TileImageDrawInfo.h"

#include "BrushDrawParam.h"
#include "SinTable.h"
#include "ImageBuf8.h"


//-----------------

#define _BRPARAM    (g_tileimage_dinfo.brushparam)
#define _CURVE_NUM  20	//曲線の補間数

//-----------------

typedef struct
{
	double x,y,pressure;
}DrawPoint;

//-----------------
//作業用データ

typedef struct
{
	double t,lastx,lasty,lastpress;
	int angle;	//最終的な回転角度

	DrawPoint curve_pt[4];
	double curve_param[_CURVE_NUM][4];

	/* r,g,b = 0.0 - 32768.0  a = 0.0-1.0 */
	RGBADouble rgba_water,	//水彩色
		rgba_water_draw;	//水彩、開始時の描画色
}DrawBrushWork;

static DrawBrushWork g_work;

//-----------------

static void _drawbrush_point(TileImage *p,double x,double y,double radius,double opacity);

//-----------------



//==================================
// 水彩 sub
//==================================


/** ブラシ半径内の色の平均色を取得 */

static void _water_get_average_color(TileImage *p,
	double x,double y,double radius,RGBADouble *dst)
{
	int x1,y1,x2,y2,cnt,ix,iy;
	double rd,r,g,b,a,dx,dy,dd;
	RGBAFix15 pix;

	x1 = floor(x - radius);
	y1 = floor(y - radius);
	x2 = floor(x + radius);
	y2 = floor(y + radius);

	rd = radius * radius;

	r = g = b = a = 0;
	cnt = 0;

	for(iy = y1; iy <= y2; iy++)
	{
		dy = iy - y;
		dy *= dy;

		for(ix = x1; ix <= x2; ix++)
		{
			dx = ix - x;

			if(dx * dx + dy < rd)
			{
				TileImage_getPixel_clip(p, ix, iy, &pix);

				dd = (double)pix.a / 0x8000;

				r += pix.r * dd;
				g += pix.g * dd;
				b += pix.b * dd;
				a += dd;

				cnt++;
			}
		}
	}

	//平均色

	if(a == 0 || cnt == 0)
		dst->r = dst->g = dst->b = dst->a = 0;
	else
	{
		dd = 1.0 / a;

		dst->r = r * dd;
		dst->g = g * dd;
		dst->b = b * dd;
		dst->a = a / cnt;
	}
}

/** 水彩の混色 */
/*
 * やまかわ氏の "gimp-painter-" (MixBrush 改造版) のソースを参考にしています。
 * http://sourceforge.jp/projects/gimp-painter/releases/?package_id=6799
 */

static void _water_mix_color(RGBADouble *dst,
	RGBADouble *pix1,double op1,
    RGBADouble *pix2,double op2,mBool alpha)
{
    double a1,a2;

    a1 = pix1->a * op1;
    a2 = pix2->a * (1.0 - a1) * op2;

    if(a1 == 0)
    {
        *dst = *pix2;
        dst->a = a2;
    }
    else if(a2 == 0)
    {
        *dst = *pix1;
        dst->a = a1;
    }
    else
    {
        dst->a = a1 + a2;

        a1 = a1 / (a1 + a2);
        a2 = 1.0 - a1;

        dst->r = pix1->r * a1 + pix2->r * a2;
        dst->g = pix1->g * a1 + pix2->g * a2;
        dst->b = pix1->b * a1 + pix2->b * a2;
    }

    if(alpha)
    {
        op1 = pow(op1, 1.35);
        op2 = pow(op2, 1.15);
        a1  = op1 + op2;

        if(a1 == 0)
            dst->a = 0;
        else
        {
            a2 = op1 / a1;
            dst->a = (pix1->a * a2 + pix2->a * (1.0 - a2)) * (op1 + (1.0 - op1) * op2);
        }
    }
}

/** 水彩、描画色の計算 */

static void _water_calc_drawcol(TileImage *p,double x,double y,double radius,
	double *popacity,RGBAFix15 *pixdst)
{
	BrushDrawParam *param = _BRPARAM;
	RGBADouble pix1,pix2;
	int i,n;

	//描画色 + 前回の色 => pix1

	_water_mix_color(&pix1,
		&g_work.rgba_water_draw, param->water_param[0],
		&g_work.rgba_water, 1, FALSE);

	//pix1 + 描画先の色 => rgba_water

	_water_get_average_color(p, x, y, radius, &pix2);
	
	_water_mix_color(&g_work.rgba_water,
		&pix1, param->water_param[2],
		&pix2, param->water_param[1], TRUE);

	//結果の描画色

	for(i = 0; i < 3; i++)
	{
		n = round(g_work.rgba_water.ar[i]);

		if(n < 0) n = 0;
		else if(n > 0x8000) n = 0x8000;

		pixdst->c[i] = n;
	}

	//濃度 (rgba_water のアルファ値で制限)

	if(g_work.rgba_water.a < *popacity)
		*popacity = g_work.rgba_water.a;
}


//===========================
//
//===========================


/** 曲線補間初期化 */

void __TileImage_initCurve()
{
	int i;
	double t,tt,t1,t2,t3;

	tt = 1.0 / 6.0;

	for(i = 0, t = 1.0 / _CURVE_NUM; i < _CURVE_NUM; i++, t += 1.0 / _CURVE_NUM)
	{
		t1 = 4.0 - (t + 3.0);
		t2 = t + 2.0;
		t3 = t + 1.0;

		g_work.curve_param[i][0] = tt * t1 * t1 * t1;
		g_work.curve_param[i][1] = tt * ( 3 * t2 * t2 * t2 - 24 * t2 * t2 + 60 * t2 - 44);
		g_work.curve_param[i][2] = tt * (-3 * t3 * t3 * t3 + 12 * t3 * t3 - 12 * t3 + 4);
		g_work.curve_param[i][3] = tt * t * t * t;
	}
}


//===========================
// 描画開始
//===========================


/** 水彩開始 */

static void _begin_water(TileImage *p,double x,double y)
{
	if(_BRPARAM->flags & BRUSHDP_F_WATER)
	{
		RGBADouble *pd = &g_work.rgba_water_draw;

		//開始時の描画色

		pd->r = g_tileimage_dinfo.rgba_brush.r;
		pd->g = g_tileimage_dinfo.rgba_brush.g;
		pd->b = g_tileimage_dinfo.rgba_brush.b;
		pd->a = 1;

		//描画先の平均色

		_water_get_average_color(p, x, y, _BRPARAM->radius, &g_work.rgba_water);
	}
}

/** ブラシ自由線描画の開始 */

void TileImage_beginDrawBrush_free(TileImage *p,double x,double y,double pressure)
{
	g_work.lastx = x;
	g_work.lasty = y;
	g_work.lastpress = pressure;
	g_work.t = 0;

	//曲線補間

	if(_BRPARAM->flags & BRUSHDP_F_CURVE)
	{
		g_work.curve_pt[0].x = x;
		g_work.curve_pt[0].y = y;
		g_work.curve_pt[0].pressure = pressure;
	
		g_work.curve_pt[1] = g_work.curve_pt[0];
		g_work.curve_pt[2] = g_work.curve_pt[0];
	}

	//水彩

	_begin_water(p, x, y);
}

/** 自由線以外の開始時 */

void TileImage_beginDrawBrush_notfree(TileImage *p,double x,double y)
{
	_begin_water(p, x, y);
}


//================================
// 線の描画
//================================


/** 曲線補間の線描画 */

static void _drawbrush_curveline(TileImage *p)
{
	int i;
	double lastx,lasty,x,y,press_st,press_ed;
	double t1,t2,t3,t4;
	DrawPoint *pt = g_work.curve_pt;

	lastx = g_work.lastx;
	lasty = g_work.lasty;
	press_st = g_work.lastpress;

	//

	for(i = 0; i < _CURVE_NUM; i++)
	{
		t1 = g_work.curve_param[i][0];
		t2 = g_work.curve_param[i][1];
		t3 = g_work.curve_param[i][2];
		t4 = g_work.curve_param[i][3];

		//

		x = pt[0].x * t1 + pt[1].x * t2 + pt[2].x * t3 + pt[3].x * t4;
		y = pt[0].y * t1 + pt[1].y * t2 + pt[2].y * t3 + pt[3].y * t4;

		press_ed = pt[0].pressure * t1 + pt[1].pressure * t2 + pt[2].pressure * t3 + pt[3].pressure * t4;

		g_work.t = TileImage_drawBrushLine(p,
			lastx, lasty, x, y, press_st, press_ed, g_work.t);

		//

		lastx = x;
		lasty = y;
		press_st = press_ed;
	}

	//

	g_work.lastx = x;
	g_work.lasty = y;
	g_work.lastpress = press_ed;
}

/** ブラシで自由線描画 */

void TileImage_drawBrushFree(TileImage *p,double x,double y,double pressure)
{
	if(_BRPARAM->flags & BRUSHDP_F_CURVE)
	{
		//曲線

		g_work.curve_pt[3].x = x;
		g_work.curve_pt[3].y = y;
		g_work.curve_pt[3].pressure = pressure;

		_drawbrush_curveline(p);

		for(int i = 0; i < 3; i++)
			g_work.curve_pt[i] = g_work.curve_pt[i + 1];
	}
	else
	{
		//直線

		g_work.t = TileImage_drawBrushLine(p,
			g_work.lastx, g_work.lasty, x, y,
			g_work.lastpress, pressure, g_work.t);

		g_work.lastx = x;
		g_work.lasty = y;
		g_work.lastpress = pressure;
	}
}


//=====================================
// 線を引く
//=====================================


/** 筆圧補正 : 線形n点 */

double _pressure_linear(BrushDrawParam *p,double d)
{
	double *pv = p->pressure_val;

	if(p->pressure_type == 1)
	{
		//線形1点
		/* [0] in [1] out [2] 1 - in [3] 1 - out */

		if(d < pv[0])
			//0..in => 0..out
			return d / pv[0] * pv[1];
		else
		{
			//in..1 => out..1
			
			if(pv[2] == 0)
				return pv[1];
			else
				return (d - pv[0]) / pv[2] * pv[3] + pv[1];
		}
	}
	else
	{
		//線形2点
		/* [0] in1 [1] out1 [2] in2 [3] out2
		 * [4] in2 - in1 [5] out2 - out1 [6] 1 - in2 [7] 1 - out2 */

		if(d < pv[0])
			//0..in1 => 0..out1
			return d / pv[0] * pv[1];
		else if(d < pv[2])
		{
			//in1..in2 => out1..out2

			if(pv[4] == 0)
				return pv[1];
			else
				return (d - pv[0]) / pv[4] * pv[5] + pv[1];
		}
		else
		{
			//in2..1 => out2..1

			if(pv[6] == 0)
				return pv[3];
			else
				return (d - pv[2]) / pv[6] * pv[7] + pv[3];
		}
	}
}

/** ブラシで線を引く (線形補間)
 *
 * @return 次の t 値が返る */

double TileImage_drawBrushLine(TileImage *p,
	double x1,double y1,double x2,double y2,
	double press_st,double press_ed,double t_start)
{
	BrushDrawParam *param = _BRPARAM;
	double len,t,dx,dy,ttm,dtmp;
	double press_len,radius_len,opa_len;
	double press,press_r,press_o,radius,opacity,x,y;
	int angle,ntmp;

	//線の長さ

	dx = x2 - x1;
	dy = y2 - y1;

	len = sqrt(dx * dx + dy * dy);
	if(len == 0) return t_start;

	//間隔を長さ 1.0 に対する値に

	ttm = param->interval / len;

	//各幅

	radius_len = 1.0 - param->min_size;
	opa_len    = 1.0 - param->min_opacity;
	press_len  = press_ed - press_st;

	//ブラシ画像回転角度

	angle = param->angle;

	if(param->flags & BRUSHDP_F_TRAVELLING_DIR)
		angle += (int)(atan2(dy, dx) * 256 / M_MATH_PI);   //進行方向

	//------------------

	t = t_start / len;

	while(t < 1.0)
	{
		//--------- 筆圧 (0.0-1.0)

		//t 位置の筆圧

		press = press_len * t + press_st;

		//筆圧補正

		if(param->flags & BRUSHDP_F_PRESSURE)
		{
			if(param->pressure_type == 0)
				//曲線
				press = pow(press, param->pressure_val[0]);
			else
				//線形n点
				press = _pressure_linear(param, press);
		}

		//サイズ・濃度の最小補正

		press_r = press * radius_len + param->min_size;
		press_o = press * opa_len    + param->min_opacity;

		//--------- 半径・濃度

		if(param->flags & BRUSHDP_F_RANDOM_SIZE)
			press_r = press_r * (mRandXorShift_getDouble() * (1.0 - param->random_size_min) + param->random_size_min);

		radius  = press_r * param->radius;
		opacity = press_o * param->opacity;

		//---------- 位置

		x = x1 + dx * t;
		y = y1 + dy * t;

		//ランダム位置 (円範囲)

		if(param->flags & BRUSHDP_F_RANDOM_POS)
		{
			dtmp = (radius * param->random_pos_len) * mRandXorShift_getDouble();
			ntmp = mRandXorShift_getIntRange(0, 511);

			x += dtmp * SINTABLE_GETCOS(ntmp);
			y += dtmp * SINTABLE_GETSIN(ntmp);
		}

		//----------- ブラシ画像回転角度

		g_work.angle = angle;

		if(param->angle_random)
			g_work.angle += mRandXorShift_getIntRange(-(param->angle_random), param->angle_random);

		//------------ 点描画

		_drawbrush_point(p, x, y, radius, opacity);

		//------------ 間隔分を加算して次の点へ

		dtmp = radius * ttm;
		if(dtmp < 0.0001) dtmp = 0.0001;

		t += dtmp;
	}

	//次回の開始 t 値 (px に変換)

	return len * (t - 1.0);
}

/** 入り抜き指定ありの直線 */

void TileImage_drawBrushLine_headtail(TileImage *p,
	double x1,double y1,double x2,double y2,uint32_t headtail)
{
	int head,tail;
	double t,dx,dy,d,xx,yy,xx2,yy2;

	head = headtail >> 16;
	tail = headtail & 0xffff;

	if(tail > 1000 - head)
		tail = 1000 - head;

	if(head == 0 && tail == 0)
		//通常
		TileImage_drawBrushLine(p, x1, y1, x2, y2, 1, 1, 0);
	else
	{
		dx = x2 - x1;
		dy = y2 - y1;

		xx = x1, yy = y1;
		t = 0;

		//入り

		if(head)
		{
			d  = head * 0.001;
			xx = x1 + dx * d;
			yy = y1 + dy * d;

			t = TileImage_drawBrushLine(p, x1, y1, xx, yy, 0, 1, t);
		}

		//中間部分 (筆圧 = 1)

		if(tail != 1000 - head)
		{
			d   = (1000 - tail) * 0.001;
			xx2 = x1 + dx * d;
			yy2 = y1 + dy * d;

			t = TileImage_drawBrushLine(p, xx, yy, xx2, yy2, 1, 1, t);

			xx = xx2, yy = yy2;
		}

		//抜き

		if(tail)
			TileImage_drawBrushLine(p, xx, yy, x2, y2, 1, 0, t);
	}
}

/** 四角形描画 */

void TileImage_drawBrushBox(TileImage *p,mDoublePoint *pt)
{
	TileImage_drawBrushLine(p, pt[0].x, pt[0].y, pt[1].x, pt[1].y, 1, 1, 0);
	TileImage_drawBrushLine(p, pt[1].x, pt[1].y, pt[2].x, pt[2].y, 1, 1, 0);
	TileImage_drawBrushLine(p, pt[2].x, pt[2].y, pt[3].x, pt[3].y, 1, 1, 0);
	TileImage_drawBrushLine(p, pt[3].x, pt[3].y, pt[0].x, pt[0].y, 1, 1, 0);
}


//=====================================
// 点の描画
//=====================================
/* _drawbrush_point_*() 時の opacity : 0.0-32768.0 */

#define FIXF_BIT  10
#define FIXF_VAL  (1<<FIXF_BIT)


/** 通常円形 (硬さ最大時) */

static void _drawbrush_point_circle_max(TileImage *p,
	double drawx,double drawy,double radius,double opacity,RGBAFix15 *pixdraw)
{
	int subnum,x1,y1,x2,y2,ix,iy,px,py,a,rough;
	int xtbl[11],ytbl[11],subpos[11],fx,fy,fx_left,n,rr;
	int64_t fpos;
	double ddiv;
	mBool drawtp,noaa;
	RGBAFix15 pix;
	TileImageSetPixelFunc setpixfunc = g_tileimage_dinfo.funcDrawPixel;

	//半径の大きさによりサブピクセル数調整

	if(radius < 3) subnum = 11;
	else if(radius < 15) subnum = 5;
	else subnum = 3;

	//描画する範囲 (px)

	x1 = (int)floor(drawx - radius);
	y1 = (int)floor(drawy - radius);
	x2 = (int)floor(drawx + radius);
	y2 = (int)floor(drawy + radius);

	//

	fx_left = floor((x1 - drawx) * FIXF_VAL);
	fy = floor((y1 - drawy) * FIXF_VAL);
	rr = floor(radius * radius * FIXF_VAL);
	ddiv = 1.0 / (subnum * subnum);

	rough = _BRPARAM->rough;

	pix = *pixdraw;
	noaa   = ((_BRPARAM->flags & BRUSHDP_F_NO_ANTIALIAS) != 0);
	drawtp = ((_BRPARAM->flags & BRUSHDP_F_OVERWRITE_TP) != 0);

	for(ix = 0; ix < subnum; ix++)
		subpos[ix] = (ix << FIXF_BIT) / subnum;

	//------------------

	for(py = y1; py <= y2; py++, fy += FIXF_VAL)
	{
		//Y テーブル

		for(iy = 0; iy < subnum; iy++)
		{
			fpos = fy + subpos[iy];
			ytbl[iy] = fpos * fpos >> FIXF_BIT;
		}

		//

		for(px = x1, fx = fx_left; px <= x2; px++, fx += FIXF_VAL)
		{
			//X テーブル

			for(ix = 0; ix < subnum; ix++)
			{
				fpos = fx + subpos[ix];
				xtbl[ix] = fpos * fpos >> FIXF_BIT;
			}

			//オーバーサンプリング

			n = 0;

			for(iy = 0; iy < subnum; iy++)
			{
				for(ix = 0; ix < subnum; ix++)
				{
					if(xtbl[ix] + ytbl[iy] < rr) n++;
				}
			}

			//点をセット
			//(drawtp == TRUE : 透明もそのまま上書きする場合は a == 0 の時もセット)

			a = (int)(opacity * (n * ddiv) + 0.5);

			if(a || drawtp)
			{
				//荒さ

				if(rough && a)
				{
					a -= a * mRandXorShift_getIntRange(0, rough) >> 7;
					if(a < 0) a = 0;
				}

				//非アンチエイリアスの場合、点を打つなら濃度は最大
				
				if(noaa && a)
					a = (int)(opacity + 0.5);

				//セット

				pix.a = a;

				(setpixfunc)(p, px, py, &pix);
			}
		}
	}
}

/** 通常円形 (硬さのパラメータ値から) */

static void _drawbrush_point_circle(TileImage *p,
	double drawx,double drawy,double radius,double opacity,RGBAFix15 *pixdraw)
{
	int subnum,x1,y1,x2,y2,ix,iy,px,py,a,rough;
	int xtbl[11],ytbl[11],subpos[11],rr,fx,fy,fx_left;
	int64_t fpos;
	double hard_param,dd,dsum,ddiv;
	mBool drawtp,noaa;
	RGBAFix15 pix;
	TileImageSetPixelFunc setpixfunc = g_tileimage_dinfo.funcDrawPixel;

	//半径の大きさによりサブピクセル数調整

	if(radius < 3) subnum = 11;
	else if(radius < 15) subnum = 5;
	else subnum = 3;

	//描画する範囲 (px)

	x1 = (int)floor(drawx - radius);
	y1 = (int)floor(drawy - radius);
	x2 = (int)floor(drawx + radius);
	y2 = (int)floor(drawy + radius);

	//

	fx_left = floor((x1 - drawx) * FIXF_VAL);
	fy = floor((y1 - drawy) * FIXF_VAL);
	rr = floor(radius * radius * FIXF_VAL);
	ddiv = 1.0 / (subnum * subnum);

	hard_param = _BRPARAM->shape_hard;
	rough = _BRPARAM->rough;

	pix = *pixdraw;
	noaa   = ((_BRPARAM->flags & BRUSHDP_F_NO_ANTIALIAS) != 0);
	drawtp = ((_BRPARAM->flags & BRUSHDP_F_OVERWRITE_TP) != 0);

	for(ix = 0; ix < subnum; ix++)
		subpos[ix] = (ix << FIXF_BIT) / subnum;

	//------------------

	for(py = y1; py <= y2; py++, fy += FIXF_VAL)
	{
		//Y テーブル

		for(iy = 0; iy < subnum; iy++)
		{
			fpos = fy + subpos[iy];
			ytbl[iy] = fpos * fpos >> FIXF_BIT;
		}

		//

		for(px = x1, fx = fx_left; px <= x2; px++, fx += FIXF_VAL)
		{
			//X テーブル

			for(ix = 0; ix < subnum; ix++)
			{
				fpos = fx + subpos[ix];
				xtbl[ix] = fpos * fpos >> FIXF_BIT;
			}

			//オーバーサンプリング

			dsum = 0;

			for(iy = 0; iy < subnum; iy++)
			{
				for(ix = 0; ix < subnum; ix++)
				{
					if(xtbl[ix] + ytbl[iy] < rr)
					{
						dd = (double)(xtbl[ix] + ytbl[iy]) / rr;
						dd = dd + hard_param - dd * hard_param;
						if(dd < 0) dd = 0; else if(dd > 1) dd = 1;

						dsum += 1.0 - dd;
					}
				}
			}

			//点をセット
			//(drawtp == TRUE : 透明もそのまま上書きする場合は a == 0 の時もセット)

			a = (int)(opacity * (dsum * ddiv) + 0.5);

			if(a || drawtp)
			{
				//荒さ

				if(rough && a)
				{
					a -= a * mRandXorShift_getIntRange(0, rough) >> 7;
					if(a < 0) a = 0;
				}

				//非アンチエイリアスの場合、点を打つなら濃度は最大
				
				if(noaa && a)
					a = (int)(opacity + 0.5);

				//セット

				pix.a = a;

				(setpixfunc)(p, px, py, &pix);
			}
		}
	}
}

/** 点描画 : 形状画像あり、回転なし */

static void _drawbrush_point_shape(TileImage *p,
	double drawx,double drawy,double radius,double opacity,RGBAFix15 *pixdraw)
{
	int subnum,shape_size,x1,y1,x2,y2,px,py,ix,iy,xtbl[11],n,rough;
	double dsubadd,dadd,dx,dy,dx_left,dsumdiv,dtmp;
	uint8_t *shape_buf,*shape_bufy,*ytbl[11];
	mBool drawtp,noaa;
	RGBAFix15 pix;
	TileImageSetPixelFunc setpixfunc = g_tileimage_dinfo.funcDrawPixel;

	//サブピクセル数

	if(radius < 4) subnum = 11;
	else if(radius < 25) subnum = 9;
	else subnum = 3;

	//

	x1 = (int)floor(drawx - radius);
	y1 = (int)floor(drawy - radius);
	x2 = (int)floor(drawx + radius);
	y2 = (int)floor(drawy + radius);

	shape_buf  = _BRPARAM->img_shape->buf;
	shape_size = _BRPARAM->img_shape->w;

	dadd    = (double)shape_size / (radius * 2); //1px進むごとに加算する形状画像の位置
	dsubadd = dadd / subnum;
	dsumdiv = 1.0 / ((subnum * subnum) * 255.0);

	//

	rough = _BRPARAM->rough;

	pix = *pixdraw;
	noaa   = ((_BRPARAM->flags & BRUSHDP_F_NO_ANTIALIAS) != 0);
	drawtp = ((_BRPARAM->flags & BRUSHDP_F_OVERWRITE_TP) != 0);

	//--------------

	//(x1, y1) における形状画像の位置

	dtmp = shape_size * 0.5;
	dx_left = (x1 - drawx) * dadd + dtmp;
	dy = (y1 - drawy) * dadd + dtmp;

	//

	for(py = y1; py <= y2; py++, dy += dadd)
	{
		//Y テーブル (ポインタ位置)

		for(iy = 0, dtmp = dy; iy < subnum; iy++, dtmp += dsubadd)
		{
			n = (int)dtmp;
			ytbl[iy] = (n >= 0 && n < shape_size)? shape_buf + n * shape_size: NULL;
		}

		//

		for(px = x1, dx = dx_left; px <= x2; px++, dx += dadd)
		{
			//X テーブル (x 位置)

			for(ix = 0, dtmp = dx; ix < subnum; ix++, dtmp += dsubadd)
			{
				n = (int)dtmp;
				xtbl[ix] = (n >= 0 && n < shape_size)? n: -1;
			}

			//オーバーサンプリング

			n = 0;

			for(iy = 0; iy < subnum; iy++)
			{
				shape_bufy = ytbl[iy];
				if(!shape_bufy) continue;

				for(ix = 0; ix < subnum; ix++)
				{
					if(xtbl[ix] != -1)
						n += *(shape_bufy + xtbl[ix]);
				}
			}

			//1px 点を打つ

			n = (int)(opacity * (n * dsumdiv) + 0.5);

			if(n || drawtp)
			{
				//荒さ

				if(rough && n)
				{
					n -= n * mRandXorShift_getIntRange(0, rough) >> 7;
					if(n < 0) n = 0;
				}

				//非アンチエイリアス

				if(noaa && n)
					n = (int)(opacity + 0.5);

				//セット

				pix.a = n;
				(setpixfunc)(p, px, py, &pix);
			}
		}
	}
}

/** 点描画 : 形状画像あり、回転あり */

static void _drawbrush_point_shape_rotate(TileImage *p,
	double drawx,double drawy,double radius,double opacity,RGBAFix15 *pixdraw)
{
	int subnum,x1,y1,x2,y2,px,py,shape_size,c,ix,iy,nx,ny,rough;
	double dtmp,dscale,dsumdiv,dcos,dsin,_dx,_dy,daddcos,daddsin,dsubaddcos,dsubaddsin;
	double dx,dy,dx2,dy2,dx3,dy3;
	uint8_t *shape_buf;
	RGBAFix15 pix;
	mBool drawtp,noaa,exist;
	TileImageSetPixelFunc setpixfunc = g_tileimage_dinfo.funcDrawPixel;

	//サブピクセル数

	if(radius < 4) subnum = 11;
	else if(radius < 25) subnum = 7;
	else subnum = 3;

	//(回転後の範囲を考慮して描画範囲を広げる)

	dtmp = radius * 1.44;

	x1 = (int)floor(drawx - dtmp);
	y1 = (int)floor(drawy - dtmp);
	x2 = (int)floor(drawx + dtmp);
	y2 = (int)floor(drawy + dtmp);

	shape_buf  = _BRPARAM->img_shape->buf;
	shape_size = _BRPARAM->img_shape->w;

	dscale  = (double)shape_size / (radius * 2);
	dsumdiv = 1.0 / ((subnum * subnum) * 255.0);

	//

	dcos = SINTABLE_GETCOS(-(g_work.angle));
	dsin = SINTABLE_GETSIN(-(g_work.angle));

	dx = x1 - drawx;
	dy = y1 - drawy;
	dtmp = shape_size * 0.5;

	_dx = (dx * dcos - dy * dsin) * dscale + dtmp;
	_dy = (dx * dsin + dy * dcos) * dscale + dtmp;

	//

	dtmp = 1.0 / subnum;

	daddcos    = dcos * dscale;
	daddsin    = dsin * dscale;
	dsubaddcos = daddcos * dtmp;
	dsubaddsin = daddsin * dtmp;

	//

	rough = _BRPARAM->rough;

	pix = *pixdraw;
	noaa   = ((_BRPARAM->flags & BRUSHDP_F_NO_ANTIALIAS) != 0);
	drawtp = ((_BRPARAM->flags & BRUSHDP_F_OVERWRITE_TP) != 0);

	//----------------
	// xx,yy:cos, xy:sin, yx:-sin

	for(py = y1; py <= y2; py++)
	{
		dx = _dx;
		dy = _dy;

		for(px = x1; px <= x2; px++, dx += daddcos, dy += daddsin)
		{
			dx2 = dx;
			dy2 = dy;

			//オーバーサンプリング

			c = 0;
			exist = FALSE;

			for(iy = subnum; iy; iy--)
			{
				dx3 = dx2;
				dy3 = dy2;

				for(ix = subnum; ix; ix--)
				{
					nx = (int)dx3;
					ny = (int)dy3;

					if(nx >= 0 && nx < shape_size && ny >= 0 && ny < shape_size)
					{
						c += *(shape_buf + ny * shape_size + nx);
						exist = TRUE;
					}

					dx3 += dsubaddsin;
					dy3 += dsubaddcos;
				}

				dx2 -= dsubaddsin;
				dy2 += dsubaddcos;
			}

			//1px 点を打つ
			//完全上書き時、形状の矩形範囲外は除く

			c = (int)(opacity * (c * dsumdiv) + 0.5);
			
			if(c || (drawtp && exist))
			{
				//荒さ

				if(rough && c)
				{
					c -= c * mRandXorShift_getIntRange(0, rough) >> 7;
					if(c < 0) c = 0;
				}

				//非アンチエイリアス

				if(noaa && c)
					c = (int)(opacity + 0.5);

				//セット

				pix.a = c;
				(setpixfunc)(p, px, py, &pix);
			}
		}

		_dx -= daddsin;
		_dy += daddcos;
	}
}

/** ブラシ、点の描画 (メイン)
 *
 * @param opacity 0.0-1.0 */

void _drawbrush_point(TileImage *p,
	double x,double y,double radius,double opacity)
{
	RGBAFix15 pix;

	if(opacity != 0 && radius > 0.05)
	{
		pix = g_tileimage_dinfo.rgba_brush;

		//水彩、描画色の計算

		if(_BRPARAM->flags & BRUSHDP_F_WATER)
		{
			_water_calc_drawcol(p, x, y, radius, &opacity, &pix);

			if(opacity == 0) return;
		}

		//点描画

		opacity *= 0x8000;
	
		if(!_BRPARAM->img_shape)
		{
			//形状画像なし => 通常円形

			if(_BRPARAM->flags & BRUSHDP_F_SHAPE_HARD_MAX)
				_drawbrush_point_circle_max(p, x, y, radius, opacity, &pix);
			else
				_drawbrush_point_circle(p, x, y, radius, opacity, &pix);
		}
		else
		{
			//形状画像あり

			if(g_work.angle)
				_drawbrush_point_shape_rotate(p, x, y, radius, opacity, &pix);
			else
				_drawbrush_point_shape(p, x, y, radius, opacity, &pix);
		}
	}
}
