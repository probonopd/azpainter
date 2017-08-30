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
 * 図形描画
 *****************************************/

#include <stdlib.h>
#include <math.h>

#include "mDef.h"
#include "mRectBox.h"

#include "TileImage.h"
#include "TileImageDrawInfo.h"
#include "defCanvasInfo.h"

#include "FillPolygon.h"



//=================================

/** クリッピングフラグ取得 */

static int _get_clip_flags(int x,int y,mRect *rcclip)
{
	int flags = 0;

	if(x < rcclip->x1) flags |= 1;
	else if(x > rcclip->x2) flags |= 2;

	if(y < rcclip->y1) flags |= 4;
	else if(y > rcclip->y2) flags |= 8;

	return flags;
}

//=================================



/** ドット直線描画 (ブレゼンハム)
 *
 * @param nostart 開始位置は点を打たない */

void TileImage_drawLineB(TileImage *p,
	int x1,int y1,int x2,int y2,RGBAFix15 *pix,mBool nostart)
{
	int sx,sy,dx,dy,a,a1,e;
	void (*func)(TileImage *,int,int,RGBAFix15 *) = g_tileimage_dinfo.funcDrawPixel;

	if(x1 == x2 && y1 == y2)
	{
		if(!nostart)
			(func)(p, x1, y1, pix);
		return;
	}

	//

	dx = (x1 < x2)? x2 - x1: x1 - x2;
	dy = (y1 < y2)? y2 - y1: y1 - y2;
	sx = (x1 <= x2)? 1: -1;
	sy = (y1 <= y2)? 1: -1;

	if(dx >= dy)
	{
		//横長
		
		a  = 2 * dy;
		a1 = a - 2 * dx;
		e  = a - dx;

		if(nostart)
		{
			if(e >= 0) y1 += sy, e += a1;
			else e += a;
			x1 += sx;
		}

		while(x1 != x2)
		{
			(func)(p, x1, y1, pix);

			if(e >= 0) y1 += sy, e += a1;
			else e += a;
			x1 += sx;
		}
	}
	else
	{
		//縦長
		
		a  = 2 * dx;
		a1 = a - 2 * dy;
		e  = a - dy;

		if(nostart)
		{
			if(e >= 0) x1 += sx, e += a1;
			else e += a;
			y1 += sy;
		}

		while(y1 != y2)
		{
			(func)(p, x1, y1, pix);

			if(e >= 0) x1 += sx, e += a1;
			else e += a;
			y1 += sy;
		}
	}

	(func)(p, x1, y1, pix);
}

/** ドット直線描画 (フィルタ描画用) */

static void _drawlineForFilter_setpixel(TileImage *p,int x,int y,RGBAFix15 *pix,mRect *rc)
{
	if(rc->x1 <= x && x <= rc->x2 && rc->y1 <= y && y <= rc->y2)
		(g_tileimage_dinfo.funcDrawPixel)(p, x, y, pix);
}

void TileImage_drawLine_forFilter(TileImage *p,
	int x1,int y1,int x2,int y2,RGBAFix15 *pix,mRect *rcclip)
{
	int sx,sy,dx,dy,a,a1,e;

	if(x1 == x2 && y1 == y2)
	{
		_drawlineForFilter_setpixel(p, x1, y1, pix, rcclip);
		return;
	}

	//

	dx = (x1 < x2)? x2 - x1: x1 - x2;
	dy = (y1 < y2)? y2 - y1: y1 - y2;
	sx = (x1 <= x2)? 1: -1;
	sy = (y1 <= y2)? 1: -1;

	if(dx >= dy)
	{
		//横長
		
		a  = 2 * dy;
		a1 = a - 2 * dx;
		e  = a - dx;

		while(x1 != x2)
		{
			_drawlineForFilter_setpixel(p, x1, y1, pix, rcclip);

			if(e >= 0) y1 += sy, e += a1;
			else e += a;
			x1 += sx;
		}
	}
	else
	{
		//縦長
		
		a  = 2 * dx;
		a1 = a - 2 * dy;
		e  = a - dy;

		while(y1 != y2)
		{
			_drawlineForFilter_setpixel(p, x1, y1, pix, rcclip);

			if(e >= 0) x1 += sx, e += a1;
			else e += a;
			y1 += sy;
		}
	}

	_drawlineForFilter_setpixel(p, x1, y1, pix, rcclip);
}

/** クリッピング付き直線 */

void TileImage_drawLine_clip(TileImage *p,int x1,int y1,int x2,int y2,RGBAFix15 *pix,mRect *rcclip)
{
	int f,f1,f2,x,y;
	mRect rc;

	rc = *rcclip;

	f1 = _get_clip_flags(x1, y1, &rc);
	f2 = _get_clip_flags(x2, y2, &rc);

	while(f1 | f2)
	{
		//範囲外

		if(f1 & f2) return;

		//クリッピング

		f = (f1)? f1: f2;

		if(f & 8)
		{
			x = x1 + (x2 - x1) * (rc.y2 - y1) / (y2 - y1);
			y = rc.y2;
		}
		else if(f & 4)
		{
			x = x1 + (x2 - x1) * (rc.y1 - y1) / (y2 - y1);
			y = rc.y1;
		}
		else if(f & 2)
		{
			y = y1 + (y2 - y1) * (rc.x2 - x1) / (x2 - x1);
			x = rc.x2;
		}
		else
		{
			y = y1 + (y2 - y1) * (rc.x1 - x1) / (x2 - x1);
			x = rc.x1;
		}

		if(f == f1)
		{
			x1 = x, y1 = y;
			f1 = _get_clip_flags(x, y, &rc);
		}
		else
		{
			x2 = x, y2 = y;
			f2 = _get_clip_flags(x, y, &rc);
		}
	}

	TileImage_drawLineB(p, x1, y1, x2, y2, pix, FALSE);
}

/** 4つの点を線でつなぐ */

void TileImage_drawLinePoint4(TileImage *p,mPoint *pt,mRect *rcclip)
{
	RGBAFix15 pix;

	pix.v64 = 0;
	pix.a = 0x8000;

	TileImage_drawLine_clip(p, pt[0].x, pt[0].y, pt[1].x, pt[1].y, &pix, rcclip);
	TileImage_drawLine_clip(p, pt[1].x, pt[1].y, pt[2].x, pt[2].y, &pix, rcclip);
	TileImage_drawLine_clip(p, pt[2].x, pt[2].y, pt[3].x, pt[3].y, &pix, rcclip);
	TileImage_drawLine_clip(p, pt[3].x, pt[3].y, pt[0].x, pt[0].y, &pix, rcclip);
}

/** 楕円描画 */

void TileImage_drawEllipse(TileImage *p,double cx,double cy,double xr,double yr,
    RGBAFix15 *pix,mBool dotpen,CanvasViewParam *param,mBool mirror)
{
	int i,div;
	double add,rr,xx,yy,x1,y1,sx,sy,nx,ny,t = 0;

	//開始位置 (0度)

	x1 = xr * param->cosrev;
	if(mirror) x1 = -x1;

	sx = cx + x1;
	sy = cy + xr * param->sinrev;

	//分割数

	rr = (xr > yr)? xr: yr;

	div = (int)((rr * M_MATH_PI * 2) / 4.0);
	if(div < 30) div = 30; else if(div > 400) div = 400;

	//

	add = M_MATH_PI * 2 / div;
	rr = add;

	for(i = div; i > 0; i--, rr += add)
	{
		xx = xr * cos(rr);
		yy = yr * sin(rr);

		x1 = xx * param->cosrev + yy * param->sinrev; if(mirror) x1 = -x1;
		y1 = xx * param->sinrev - yy * param->cosrev;

		nx = x1 + cx;
		ny = y1 + cy;

		if(dotpen)
		{
			TileImage_drawLineB(p, (int)(sx + 0.5), (int)(sy + 0.5),
				(int)(nx + 0.5), (int)(ny + 0.5), pix, TRUE);
		}
		else
			t = TileImage_drawBrushLine(p, sx, sy, nx, ny, 1, 1, t);

		sx = nx, sy = ny;
	}
}

/** 四角枠描画 */

void TileImage_drawBox(TileImage *p,int x,int y,int w,int h,RGBAFix15 *pix)
{
	int i;
	void (*setpix)(TileImage *,int,int,RGBAFix15 *) = g_tileimage_dinfo.funcDrawPixel;

	//横線

	for(i = 0; i < w; i++)
	{
		(setpix)(p, x + i, y, pix);
		(setpix)(p, x + i, y + h - 1, pix);
	}

	//縦線

	for(i = 1; i < h - 1; i++)
	{
		(setpix)(p, x, y + i, pix);
		(setpix)(p, x + w - 1, y + i, pix);
	}
}

/** ベジェ曲線、8分割で長さを計算し、分割数を取得 */

static int _bezier_get_division(mDoublePoint *pt)
{
	int i,div;
	double sx,sy,t,len,tt,t1,t2,t3,t4,nx,ny;

	//曲線の長さ
	
	sx = pt[0].x, sy = pt[0].y;

	for(i = 8, t = 0.125, len = 0; i; i--, t += 0.125)
	{
		tt = 1 - t;

		t1 = tt * tt * tt;
		t2 = 3 * t * tt * tt;
		t3 = 3 * t * t * tt;
		t4 = t * t * t;

		nx = pt[0].x * t1 + pt[1].x * t2 + pt[2].x * t3 + pt[3].x * t4;
		ny = pt[0].y * t1 + pt[1].y * t2 + pt[2].y * t3 + pt[3].y * t4;

		len += sqrt((nx - sx) * (nx - sx) + (ny - sy) * (ny - sy));

		sx = nx, sy = ny;
	}

	if(len == 0) return -1;

	//分割数 (1つの線が 4px になるように)

	div = (int)(len / 4.0);
	if(div < 10) div = 10; else if(div > 400) div = 400;

	return div;
}

/** 補助線付きの3次ベジェ線描画 (XOR 描画用) */

void TileImage_drawBezier_forXor(TileImage *p,mPoint *pt,mBool drawline2)
{
	mDoublePoint dpt[4];
	RGBAFix15 pix;
	int div,i;
	double sx,sy,nx,ny,t,tt,t1,t2,t3,t4,inc;

	pix.a = 0x8000;

	//補助線1

	TileImage_drawLineB(p, pt[0].x, pt[0].y, pt[1].x, pt[1].y, &pix, FALSE);
	TileImage_drawBox(p, pt[1].x - 2, pt[1].y - 2, 5, 5, &pix);

	//補助線2

	if(drawline2)
	{
		TileImage_drawLineB(p, pt[2].x, pt[2].y, pt[3].x, pt[3].y, &pix, FALSE);
		TileImage_drawBox(p, pt[2].x - 2, pt[2].y - 2, 5, 5, &pix);
	}

	//------ 曲線

	for(i = 0; i < 4; i++)
	{
		dpt[i].x = pt[i].x;
		dpt[i].y = pt[i].y;
	}

	div = _bezier_get_division(dpt);
	if(div < 0) return;

	inc = 1.0 / div;

	sx = dpt[0].x, sy = dpt[0].y;

	for(i = div, t = inc; i; i--, t += inc)
	{
		tt = 1 - t;

		t1 = tt * tt * tt;
		t2 = 3 * t * tt * tt;
		t3 = 3 * t * t * tt;
		t4 = t * t * t;

		nx = dpt[0].x * t1 + dpt[1].x * t2 + dpt[2].x * t3 + dpt[3].x * t4;
		ny = dpt[0].y * t1 + dpt[1].y * t2 + dpt[2].y * t3 + dpt[3].y * t4;

		TileImage_drawLineB(p, sx, sy, nx, ny, &pix, FALSE);

		sx = nx, sy = ny;
	}
}

/** ベジェ曲線描画 */

void TileImage_drawBezier(TileImage *p,mDoublePoint *pt,RGBAFix15 *pix,
	mBool dotpen,uint32_t headtail)
{
	int div,i,head,tail,nt;
	double sx,sy,nx,ny,t,tt,t1,t2,t3,t4,inc;
	double in,out,press_s,press_n,tbrush = 0;

	//入り抜き

	if(!dotpen)
	{
		head = headtail >> 16;
		tail = headtail & 0xffff;

		if(tail > 1000 - head)
			tail = 1000 - head;

		in  = (head)? 1.0 / (head * 0.001): 0;
		out = (tail)? 1.0 / (tail * 0.001): 0;

		press_s = (head)? 0: 1.0;
	}

	//

	div = _bezier_get_division(pt);
	if(div < 0) return;

	inc = 1.0 / div;

	sx = pt[0].x, sy = pt[0].y;

	for(i = 0, t = inc; i < div; i++, t += inc)
	{
		tt = 1 - t;

		t1 = tt * tt * tt;
		t2 = 3 * t * tt * tt;
		t3 = 3 * t * t * tt;
		t4 = t * t * t;

		nx = pt[0].x * t1 + pt[1].x * t2 + pt[2].x * t3 + pt[3].x * t4;
		ny = pt[0].y * t1 + pt[1].y * t2 + pt[2].y * t3 + pt[3].y * t4;

		if(dotpen)
			TileImage_drawLineB(p, sx, sy, nx, ny, pix, (i != 0));
		else
		{
			nt = (int)(t * 1000 + 0.5);

			if(nt < head)
				press_n = t * in;
			else if(nt > 1000 - tail)
				press_n = (1.0 - t) * out;
			else
				press_n = 1.0;
			
			tbrush = TileImage_drawBrushLine(p, sx, sy, nx, ny,
				press_s, press_n, tbrush);

			press_s = press_n;
		}

		sx = nx, sy = ny;
	}
}

/** スプライン曲線の描画
 *
 * @param pt  5つ。始点とパラメータ
 * @param pressure   2つ。始点と終点の筆圧 (0 or 1)
 * @param ptlast_dot ドット描画時、前回の最後の点。NULL でブラシ描画。
 * @param first      最初の点か */

double TileImage_drawSpline(TileImage *p,mDoublePoint *pt,uint8_t *pressure,
	RGBAFix15 *pixdraw,double tbrush,mPoint *ptlast_dot,mBool first)
{
	int i,div,nx1,ny1,nx2,ny2;
	double t,tadd,x1,y1,x2,y2,press_st,press_ed,press_top,press_len,press_t,press_add;

	if(ptlast_dot)
	{
		//ドット時、前回の位置
		
		if(first)
		{
			nx1 = floor(pt->x);
			ny1 = floor(pt->y);
		}
		else
		{
			nx1 = ptlast_dot->x;
			ny1 = ptlast_dot->y;
		}
	}
	else
	{
		//ブラシ時、筆圧の開始値と幅
		
		press_top = pressure[0];
		press_len = pressure[1] - pressure[0];
		press_st = press_top;
	}

	//分割数 (pt[4].x = 長さ。一つが 4px 程度になるよう計算)

	div = pt[4].x / 4;
	
	if(div < 10) div = 10;
	else if(div > 400) div = 400;

	//

	x1 = pt->x, y1 = pt->y;
	tadd = pt[4].x / div;
	
	press_add = press_t = 1.0 / div;

	for(i = 0, t = tadd; i < div; i++, t += tadd, press_t += press_add)
	{
		x2 = ((pt[3].x * t + pt[2].x) * t + pt[1].x) * t + pt->x;
		y2 = ((pt[3].y * t + pt[2].y) * t + pt[1].y) * t + pt->y;

		if(ptlast_dot)
		{
			//ドット (一番最初の点は nostart を FALSE にする)
			
			nx2 = floor(x2);
			ny2 = floor(y2);
		
			TileImage_drawLineB(p, nx1, ny1, nx2, ny2, pixdraw, !(first && i == 0));

			nx1 = nx2;
			ny1 = ny2;
		}
		else
		{
			//ブラシ
			
			press_ed = press_top + press_len * press_t;
			
			tbrush = TileImage_drawBrushLine(p, x1, y1, x2, y2, press_st, press_ed, tbrush);

			press_st = press_ed;
		}

		x1 = x2;
		y1 = y2;
	}

	//ドット、最後の点

	if(ptlast_dot)
	{
		ptlast_dot->x = nx1;
		ptlast_dot->y = ny1;
	}

	return tbrush;
}


//================================
// 塗りつぶし
//================================


/** 四角形塗りつぶし */

void TileImage_drawFillBox(TileImage *p,int x,int y,int w,int h,RGBAFix15 *pix)
{
	int ix,iy;
	void (*setpix)(TileImage *,int,int,RGBAFix15 *) = g_tileimage_dinfo.funcDrawPixel;

	for(iy = 0; iy < h; iy++)
	{
		for(ix = 0; ix < w; ix++)
			(setpix)(p, x + ix, y + iy, pix);
	}
}

/** 楕円塗りつぶし */

void TileImage_drawFillEllipse(TileImage *p,
	double cx,double cy,double xr,double yr,RGBAFix15 *pixdraw,mBool antialias,
	CanvasViewParam *param,mBool mirror)
{
	int nx,ny,i,j,c,flag;
	double xx,yy,rr,x1,y1,mx,my,xx2,yy2,xt,dcos,dsin;
	mDoublePoint pt[4];
	mRect rc;
	RGBAFix15 pix;

	if(!antialias)
	{
		cx = floor(cx) + 0.5;
		cy = floor(cy) + 0.5;
		xr = round(xr);
		yr = round(yr);
	}

	//--------- 描画範囲計算 (10度単位)

	dcos = param->cosrev;
	dsin = param->sinrev;

	for(i = 0, rr = 0; i < 36; i++, rr += M_MATH_PI / 18)
	{
		xx = xr * cos(rr);
		yy = yr * sin(rr);

		x1 = xx * dcos + yy * dsin; if(mirror) x1 = -x1;
		y1 = xx * dsin - yy * dcos;

		nx = floor(x1 + cx);
		ny = floor(y1 + cy);

		if(i == 0)
		{
			rc.x1 = rc.x2 = nx;
			rc.y1 = rc.y2 = ny;
		}
		else
			mRectIncPoint(&rc, nx, ny);
	}

	//念の為拡張

	rc.x1 -= 2, rc.y1 -= 2;
	rc.x2 += 2, rc.y2 += 2;

	//描画可能範囲内に調整

	if(!TileImage_clipCanDrawRect(p, &rc)) return;

	//-------------

	if(xr < yr)
	{
		//縦長
		rr = xr * xr;
		mx = 1.0, my = xr / yr;
	}
	else
	{
		//横長
		rr = yr * yr;
		mx = yr / xr, my = 1.0;
	}

	//--------- 描画

	dcos = param->cos;
	dsin = param->sin;

	pix = *pixdraw;

	yy = rc.y1 - cy;

	for(ny = rc.y1; ny <= rc.y2; ny++, yy += 1.0)
	{
		xx = rc.x1 - cx;

		for(nx = rc.x1; nx <= rc.x2; nx++, xx += 1.0)
		{
			if(antialias)
			{
				//-------- アンチエイリアス (5x5)

				//内外判定

				pt[0].x = pt[3].x = xx;
				pt[0].y = pt[1].y = yy;
				pt[1].x = pt[2].x = xx + 1;
				pt[2].y = pt[3].y = yy + 1;

				for(i = 0, flag = 0; i < 4; i++)
				{
					xt = pt[i].x;
					if(mirror) xt = -xt;

					x1 = (xt * dcos - pt[i].y * dsin) * mx;
					y1 = (xt * dsin + pt[i].y * dcos) * my;

					if(x1 * x1 + y1 * y1 <= rr)
						flag |= (1 << i);
				}

				if(flag == 0)
					//円の範囲外
					continue;
				else if(flag == 15)
				{
					//四隅すべて範囲内 = 濃度最大
					TileImage_setPixel_draw_direct(p, nx, ny, pixdraw);
					continue;
				}

				//5x5 平均

				c = 0;

				for(i = 0, yy2 = yy; i < 5; i++, yy2 += 0.2)
				{
					for(j = 0, xx2 = xx; j < 5; j++, xx2 += 0.2)
					{
						xt = xx2;
						if(mirror) xt = -xt;

						x1 = (xt * dcos - yy2 * dsin) * mx;
						y1 = (xt * dsin + yy2 * dcos) * my;

						if(x1 * x1 + y1 * y1 < rr)
							c++;
					}
				}

				c = pixdraw->a * c / 25;
				if(c)
				{
					pix.a = c;
					TileImage_setPixel_draw_direct(p, nx, ny, &pix);
				}
			}
			else
			{
				//--------- 非アンチエイリアス

				xt = xx;
				if(mirror) xt = -xt;

				x1 = (xt * dcos - yy * dsin) * mx;
				y1 = (xt * dsin + yy * dcos) * my;

				if(x1 * x1 + y1 * y1 < rr)
					TileImage_setPixel_draw_direct(p, nx, ny, pixdraw);
			}
		}
	}
}

/** 多角形塗りつぶし */

mBool TileImage_drawFillPolygon(TileImage *p,FillPolygon *fillpolygon,
	RGBAFix15 *pixdraw,mBool antialias)
{
	mRect rc;
	int x,y,ymin,ymax,x1,x2,xmin,width;
	uint16_t *aabuf,*ps;
	RGBAFix15 pix;
	void (*setpix)(TileImage *,int,int,RGBAFix15 *) = g_tileimage_dinfo.funcDrawPixel;

	//描画する y の範囲

	TileImage_getCanDrawRect_pixel(p, &rc);

	FillPolygon_getMinMaxY(fillpolygon, &ymin, &ymax);

	if(ymin > rc.y2 || ymax < rc.y1) return FALSE;

	if(ymin < rc.y1) ymin = rc.y1;
	if(ymax > rc.y2) ymax = rc.y2;

	//開始

	if(!FillPolygon_beginDraw(fillpolygon, antialias))
		return FALSE;

	if(antialias)
		aabuf = FillPolygon_getAABuf(fillpolygon, &xmin, &width);

	pix = *pixdraw;

	//交点間描画

	for(y = ymin; y <= ymax; y++)
	{
		if(antialias)
		{
			//アンチエイリアス

			if(!FillPolygon_setXBuf_AA(fillpolygon, y))
				continue;

			for(x = 0, ps = aabuf; x < width; x++, ps++)
			{
				x1 = (*ps) >> 3;

				if(x1 == 255)
					(setpix)(p, xmin + x, y, pixdraw);
				else if(x1)
				{
					pix.a = x1 * pixdraw->a / 255;
					(setpix)(p, xmin + x, y, &pix);
				}
			}
		}
		else
		{
			//非アンチエイリアス

			if(!FillPolygon_getIntersection_noAA(fillpolygon, y))
				continue;

			while(FillPolygon_getNextLine_noAA(fillpolygon, &x1, &x2))
			{
				for(x = x1; x <= x2; x++)
					(setpix)(p, x, y, &pix);
			}
		}
	}

	return TRUE;
}


//===========================
// グラデーション
//===========================
/*
 * (uint16)
 * 色の数、アルファ値の数
 * <色>
 *   位置,R,G,B (15bit 固定少数)
 * <A>
 *   位置,A (15bit 固定少数)
 */


/** グラデーション色取得
 *
 * フィルタのグラデーションマップでも使う。 */

void TileImage_getGradationColor(RGBAFix15 *pixdst,double pos,TileImageDrawGradInfo *info)
{
	double dtmp;
	int npos,pos_l,pos_r,n1,n2;
	uint16_t *p1,*p2;
	RGBFix15 rgb_l,rgb_r;
	RGBAFix15 pix;

	//-------- 位置 [0.0-1.0] => 15bit 固定少数

	if(info->flags & TILEIMAGE_DRAWGRAD_F_LOOP)
	{
		//繰り返し
		
		pos = modf(pos, &dtmp);
		if(pos < 0) pos += 1.0;
	}
	else
	{
		//0.0-1.0 に収める
	
		if(pos < 0) pos = 0;
		else if(pos > 1) pos = 1;
	}

	npos = (int)(pos * (1<<15) + 0.5);

	//位置反転

	if(info->flags & TILEIMAGE_DRAWGRAD_F_REVERSE)
		npos = (1<<15) - npos;

	//------- RGB

	//データ位置

	for(p1 = info->buf + 2; p1[4] < npos; p1 += 4);

	//左右の位置と色

	pos_l = *p1;
	rgb_l = *((RGBFix15 *)(p1 + 1));
	
	pos_r = *(p1 + 4);
	rgb_r = *((RGBFix15 *)(p1 + 5));

	//色計算

	if(info->flags & TILEIMAGE_DRAWGRAD_F_SINGLE_COL)
		//単色
		*((RGBFix15 *)&pix) = rgb_l;
	else
	{
		//グラデーション

		n1 = npos - pos_l;
		n2 = pos_r - pos_l;

		pix.r = (rgb_r.r - rgb_l.r) * n1 / n2 + rgb_l.r;
		pix.g = (rgb_r.g - rgb_l.g) * n1 / n2 + rgb_l.g;
		pix.b = (rgb_r.b - rgb_l.b) * n1 / n2 + rgb_l.b;
	}

	//------- A

	p1 = info->buf;

	for(p1 += 2 + *p1 * 4; p1[2] < npos; p1 += 2);

	p2 = p1 + 2;

	pos_l = *p1;
	pos_r = *p2;

	n1 = (p2[1] - p1[1]) * (npos - pos_l) / (pos_r - pos_l) + p1[1];
	pix.a = n1 * info->opacity >> 8;

	//

	*pixdst = pix;
}

/** グラデーション、色セット */

static void _grad_setpixel(TileImage *p,
	int x,int y,double pos,TileImageDrawGradInfo *info)
{
	RGBAFix15 pix;

	TileImage_getGradationColor(&pix, pos, info);

	TileImage_setPixel_draw_direct(p, x, y, &pix);
}

/** グラデーション:線形 */

void TileImage_drawGradation_line(TileImage *p,
	int x1,int y1,int x2,int y2,mRect *rcarea,TileImageDrawGradInfo *info)
{
	double abx,aby,abab,abap,yy;
	int ix,iy;
	mRect rc;

	if(x1 == x2 && y1 == y2) return;

	rc = *rcarea;

	abx = x2 - x1;
	aby = y2 - y1;
	abab = abx * abx + aby * aby;

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		yy = aby * (iy - y1);
	
		for(ix = rc.x1; ix <= rc.x2; ix++)
		{
			abap = abx * (ix - x1) + yy;
		
			_grad_setpixel(p, ix, iy, abap / abab, info);
		}
	}
}

/** グラデーション:円形 */

void TileImage_drawGradation_circle(TileImage *p,
	int x1,int y1,int x2,int y2,mRect *rcarea,TileImageDrawGradInfo *info)
{
	int ix,iy;
	double dx,dy,len,yy;
	mRect rc;

	if(x1 == x2 && y1 == y2) return;

	rc = *rcarea;

	dx = x2 - x1;
	dy = y2 - y1;
	len = sqrt(dx * dx + dy * dy);

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		yy = iy - y1;
		yy *= yy;
	
		for(ix = rc.x1; ix <= rc.x2; ix++)
		{
			dx = ix - x1;
		
			_grad_setpixel(p, ix, iy, sqrt(dx * dx + yy) / len, info);
		}
	}
}

/** グラデーション:矩形 */

void TileImage_drawGradation_box(TileImage *p,
	int x1,int y1,int x2,int y2,mRect *rcarea,TileImageDrawGradInfo *info)
{
	int ix,iy,abs_y;
	double xx,yy,dx,dy,len,tmpx,tmp;
	mRect rc;

	if(x1 == x2 && y1 == y2) return;

	rc = *rcarea;

	xx = abs(x2 - x1);
	yy = abs(y2 - y1);
	len = sqrt(xx * xx + yy * yy);

	dx = (y1 == y2)? 0: xx / yy;
	dy = (x1 == x2)? 0: yy / xx;

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		abs_y = (y1 < iy)? iy - y1: y1 - iy;
		tmpx = abs_y * dx;
	
		for(ix = rc.x1; ix <= rc.x2; ix++)
		{
			xx = (x1 < ix)? ix - x1: x1 - ix;
			yy = abs_y;

			if(y1 == y2)
				yy = 0;
			else if(xx < tmpx)
				xx = tmpx;

			if(x1 == x2)
				xx = 0;
			else
			{
				tmp = xx * dy;
				if(yy < tmp) yy = tmp;
			}
		
		/*--------
			x = abs(x1 - ix);
			y = abs(y1 - iy);

			if(y1 == y2)
				y = 0;
			else
			{
				xx = y * abs(x1 - x2) / abs(y1 - y2);
				if(xx > x) x = xx;
			}

			if(x1 == x2)
				x = 0;
			else
			{
				yy = x * abs(y1 - y2) / abs(x1 - x2);
				if(yy > y) y = yy;
			}
		--------*/

			_grad_setpixel(p, ix, iy, sqrt(xx * xx + yy * yy) / len, info);
		}
	}
}

/** グラデーション:放射状 */

void TileImage_drawGradation_radial(TileImage *p,
	int x1,int y1,int x2,int y2,mRect *rcarea,TileImageDrawGradInfo *info)
{
	int ix,iy;
	double dy,top,pos,tmp;
	mRect rc;

	if(x1 == x2 && y1 == y2) return;

	rc = *rcarea;

	top = atan2(y2 - y1, x2 - x1);

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		dy = iy - y1;
	
		for(ix = rc.x1; ix <= rc.x2; ix++)
		{
			pos = (atan2(dy, ix - x1) - top) * 0.5 / M_MATH_PI;
			pos = modf(pos, &tmp);

			if(pos < 0) pos += 1.0;
		
			_grad_setpixel(p, ix, iy, pos, info);
		}
	}
}

