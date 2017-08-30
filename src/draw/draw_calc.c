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
 * DrawData
 *
 * 計算関連
 *****************************************/

#include <stdlib.h>
#include <math.h>

#include "mDef.h"
#include "mRectBox.h"

#include "defDraw.h"
#include "defConfig.h"
#include "defMacros.h"

#include "draw_calc.h"

#include "TileImage.h"



/** 現在の倍率&角度から、キャンバス用パラメータセット */

void drawCalc_setCanvasViewParam(DrawData *p)
{
	double d;

	d = p->canvas_zoom * 0.001;

	p->viewparam.scale = d;
	p->viewparam.scalediv = 1.0 / d;

	d = p->canvas_angle * M_MATH_PI / 18000.0;

	p->viewparam.rd = d;
	p->viewparam.cos = cos(d);
	p->viewparam.sin = sin(d);
	p->viewparam.cosrev = p->viewparam.cos;
	p->viewparam.sinrev = -(p->viewparam.sin);
}

/** 指定位置がキャンバスの範囲内か */

mBool drawCalc_isPointInCanvasArea(DrawData *p,mPoint *pt)
{
	return (pt->x >= 0 && pt->x < p->szCanvas.w && pt->y >= 0 && pt->y < p->szCanvas.h);
}


//==============================
// 座標変換
//==============================


/** 領域座標 (double) -> イメージ位置 (double) */

void drawCalc_areaToimage_double(DrawData *p,double *dx,double *dy,double x,double y)
{
	double xx,yy;

	x += p->ptScroll.x - (p->szCanvas.w >> 1);
	y += p->ptScroll.y - (p->szCanvas.h >> 1);

	xx = x * p->viewparam.cosrev - y * p->viewparam.sinrev;
	yy = x * p->viewparam.sinrev + y * p->viewparam.cosrev;

	if(p->canvas_mirror) xx = -xx;

	*dx = xx * p->viewparam.scalediv + p->imgoriginX;
	*dy = yy * p->viewparam.scalediv + p->imgoriginY;
}

/** 領域座標 (mPoint) -> イメージ位置 (mDoublePoint) */

void drawCalc_areaToimage_double_pt(DrawData *p,mDoublePoint *dst,mPoint *src)
{
	drawCalc_areaToimage_double(p, &dst->x, &dst->y, src->x, src->y);
}

/** 領域座標 (double) -> イメージ位置 (mPoint) */

void drawCalc_areaToimage_pt(DrawData *p,mPoint *pt,double x,double y)
{
	drawCalc_areaToimage_double(p, &x, &y, x, y);

	pt->x = floor(x);
	pt->y = floor(y);
}

/** キャンバス座標における相対値をイメージ座標での相対値に変換 */

void drawCalc_areaToimage_relative(DrawData *p,mPoint *dst,mPoint *src)
{
	dst->x = (int)((src->x * p->viewparam.cosrev - src->y * p->viewparam.sinrev) * p->viewparam.scalediv);
	dst->y = (int)((src->x * p->viewparam.sinrev + src->y * p->viewparam.cosrev) * p->viewparam.scalediv);

	if(p->canvas_mirror)
		dst->x = -(dst->x);
}

/** イメージ位置 (int) -> 領域座標 (mPoint) */

void drawCalc_imageToarea_pt(DrawData *p,mPoint *dst,int x,int y)
{
	double dx,dy;

	dx = (x - p->imgoriginX) * p->viewparam.scale;
	dy = (y - p->imgoriginY) * p->viewparam.scale;

	if(p->canvas_mirror) dx = -dx;

	dst->x = round((dx * p->viewparam.cos - dy * p->viewparam.sin) + (p->szCanvas.w >> 1) - p->ptScroll.x);
	dst->y = round((dx * p->viewparam.sin + dy * p->viewparam.cos) + (p->szCanvas.h >> 1) - p->ptScroll.y);
}

/** イメージ位置 (double) -> 領域座標 (mPoint) */

void drawCalc_imageToarea_pt_double(DrawData *p,mPoint *dst,double x,double y)
{
	double dx,dy;

	dx = (x - p->imgoriginX) * p->viewparam.scale;
	dy = (y - p->imgoriginY) * p->viewparam.scale;

	if(p->canvas_mirror) dx = -dx;

	dst->x = round((dx * p->viewparam.cos - dy * p->viewparam.sin) + (p->szCanvas.w >> 1) - p->ptScroll.x);
	dst->y = round((dx * p->viewparam.sin + dy * p->viewparam.cos) + (p->szCanvas.h >> 1) - p->ptScroll.y);
}

/** キャンバス中央におけるイメージ位置取得 */

void drawCalc_getImagePos_atCanvasCenter(DrawData *p,double *px,double *py)
{
	drawCalc_areaToimage_double(p, px, py, p->szCanvas.w * 0.5, p->szCanvas.h * 0.5);
}

/** イメージ座標 (mBox) -> 領域座標 (mBox)
 *
 * @return FALSE で範囲外 */

mBool drawCalc_imageToarea_box(DrawData *p,mBox *dst,mBox *src)
{
	mPoint pt[4];
	int x1,y1,x2,y2,i;

	//右・下端は+1

	x1 = src->x, y1 = src->y;
	x2 = x1 + src->w;
	y2 = y1 + src->h;

	//最小・最大値

	if(p->canvas_angle == 0)
	{
		drawCalc_imageToarea_pt(p, pt, x1, y1);
		drawCalc_imageToarea_pt(p, pt + 1, x2, y2);

		if(pt[0].x < pt[1].x)
			x1 = pt[0].x, x2 = pt[1].x;
		else
			x1 = pt[1].x, x2 = pt[0].x;

		if(pt[0].y < pt[1].y)
			y1 = pt[0].y, y2 = pt[1].y;
		else
			y1 = pt[1].y, y2 = pt[0].y;
	}
	else
	{
		drawCalc_imageToarea_pt(p, pt    , x1, y1);
		drawCalc_imageToarea_pt(p, pt + 1, x2, y1);
		drawCalc_imageToarea_pt(p, pt + 2, x1, y2);
		drawCalc_imageToarea_pt(p, pt + 3, x2, y2);

		x1 = x2 = pt[0].x;
		y1 = y2 = pt[0].y;

		for(i = 1; i < 4; i++)
		{
			if(x1 > pt[i].x) x1 = pt[i].x;
			if(y1 > pt[i].y) y1 = pt[i].y;
			if(x2 < pt[i].x) x2 = pt[i].x;
			if(y2 < pt[i].y) y2 = pt[i].y;
		}
	}

	//念のため範囲拡張

	x1--, y1--;
	x2++, y2++;

	//範囲外判定

	if(x2 < 0 || y2 < 0 || x1 >= p->szCanvas.w || y1 >= p->szCanvas.h)
		return FALSE;

	//調整

	if(x1 < 0) x1 = 0;
	if(y1 < 0) y1 = 0;
	if(x2 >= p->szCanvas.w) x2 = p->szCanvas.w - 1;
	if(y2 >= p->szCanvas.h) y2 = p->szCanvas.h - 1;

	//

	dst->x = x1, dst->y = y1;
	dst->w = x2 - x1 + 1;
	dst->h = y2 - y1 + 1;

	return TRUE;
}

/** 領域座標 (mBox) -> イメージ座標 (mBox)
 *
 * @return FALSE でイメージの範囲外 */

mBool drawCalc_areaToimage_box(DrawData *p,mBox *dst,mBox *src)
{
	mPoint pt[4];
	mRect rc;
	int i;

	//四隅をイメージ位置に変換

	drawCalc_areaToimage_pt(p, pt    , src->x - 1, src->y - 1);
	drawCalc_areaToimage_pt(p, pt + 1, src->x + src->w, src->y - 1);
	drawCalc_areaToimage_pt(p, pt + 2, src->x - 1, src->y + src->h);
	drawCalc_areaToimage_pt(p, pt + 3, src->x + src->w, src->y + src->h);

	//範囲

	rc.x1 = rc.x2 = pt[0].x;
	rc.y1 = rc.y2 = pt[0].y;

	for(i = 1; i < 4; i++)
	{
        if(pt[i].x < rc.x1) rc.x1 = pt[i].x;
        if(pt[i].y < rc.y1) rc.y1 = pt[i].y;
        if(pt[i].x > rc.x2) rc.x2 = pt[i].x;
        if(pt[i].y > rc.y2) rc.y2 = pt[i].y;
	}

	//イメージ範囲調整

	return drawCalc_getImageBox_rect(p, dst, &rc);
}


//==============================
// 範囲
//==============================


/** mRect を領域の範囲内にクリッピングして mBox へ
 *
 * @param dst 範囲がない場合、w = h = 0 */

mBool drawCalc_clipArea_toBox(DrawData *p,mBox *dst,mRect *src)
{
	int x1,y1,x2,y2;

	if(src->x1 > src->x2 || src->y1 > src->y2
		|| src->x2 < 0 || src->y2 < 0
		|| src->x1 >= p->szCanvas.w || src->y1 >= p->szCanvas.h)
	{
		dst->w = dst->h = 0;
		return FALSE;
	}
	else
	{
		x1 = (src->x1 < 0)? 0: src->x1;
		y1 = (src->y1 < 0)? 0: src->y1;
		x2 = (src->x2 >= p->szCanvas.w)? p->szCanvas.w - 1: src->x2;
		y2 = (src->y2 >= p->szCanvas.h)? p->szCanvas.h - 1: src->y2;

		dst->x = x1, dst->y = y1;
		dst->w = x2 - x1 + 1;
		dst->h = y2 - y1 + 1;

		return TRUE;
	}
}

/** mRect をイメージ範囲内にクリッピング */

mBool drawCalc_clipImageRect(DrawData *p,mRect *rc)
{
	int x1,y1,x2,y2;

	if(mRectIsEmpty(rc)) return FALSE;

	x1 = rc->x1, y1 = rc->y1;
	x2 = rc->x2, y2 = rc->y2;

	//範囲外

	if(x2 < 0 || y2 < 0 || x1 >= p->imgw || y1 >= p->imgh)
		return FALSE;

	//調整

	if(x1 < 0) x1 = 0;
	if(y1 < 0) y1 = 0;
	if(x2 >= p->imgw) x2 = p->imgw - 1;
	if(y2 >= p->imgh) y2 = p->imgh - 1;

	//セット

	rc->x1 = x1, rc->y1 = y1;
	rc->x2 = x2, rc->y2 = y2;

	return TRUE;
}

/** mRect をイメージの範囲内に収めて mBox へ
 *
 * @return FALSE で範囲外、または範囲が空 */

mBool drawCalc_getImageBox_rect(DrawData *p,mBox *dst,mRect *rc)
{
	mRect rc2;

	rc2 = *rc;

	if(!drawCalc_clipImageRect(p, &rc2))
	{
		dst->w = dst->h = 0;
		return FALSE;
	}
	else
	{
		dst->x = rc2.x1;
		dst->y = rc2.y1;
		dst->w = rc2.x2 - rc2.x1 + 1;
		dst->h = rc2.y2 - rc2.y1 + 1;

		return TRUE;
	}
}

/** mBox (イメージ座標) の範囲を結合
 *
 * x < 0 の場合は範囲なしとする。
 * 
 * @return FALSE で両方共範囲なし */

mBool drawCalc_unionImageBox(mBox *dst,mBox *src)
{
	if(dst->x < 0 && src->x < 0)
		//両方共範囲なし
		return FALSE;
	else if(src->x < 0)
		//src が範囲なし => dst は変化なし
		return TRUE;
	else if(dst->x < 0)
	{
		//dst が範囲なし => src をコピー

		*dst = *src;
		return TRUE;
	}
	else
	{
		//範囲合成

		mBoxUnion(dst, src);
		return TRUE;
	}
}

/** src と、src を mx,my 分相対移動した範囲を合成 */

void drawCalc_unionRect_relmove(mRect *dst,mRect *src,int mx,int my)
{
	*dst = *src;
	mRectRelMove(dst, mx, my);

	mRectUnion(dst, src);
}


//=============================
// 描画関連
//=============================


/** 直線を45度単位に調整
 *
 * @param pt  対象点。結果はここに入る。
 * @param ptstart 始点 */

void drawCalc_fitLine45(mPoint *pt,mPoint *ptstart)
{
	int dx,dy,adx,ady,n;

	dx = pt->x - ptstart->x;
	dy = pt->y - ptstart->y;

	adx = abs(dx);
	ady = abs(dy);

	n = (adx > ady)? adx: ady;

	if(abs(adx - ady) < n / 2)
	{
		//45度線

		if(adx < ady)
			pt->y = ptstart->y + ((dy < 0)? -adx: adx);
		else
			pt->x = ptstart->x + ((dx < 0)? -ady: ady);
	}
	else if(adx > ady)
		//水平
		pt->y = ptstart->y;
	else
		//垂直
		pt->x = ptstart->x;
}

/** pttmp[0,1] から線の角度をラジアンで取得 (イメージ位置に対する角度。反時計回り) */

double drawCalc_getLineRadian_forImage(DrawData *p)
{
	double x1,y1,x2,y2;

	drawCalc_areaToimage_double(p, &x1, &y1, p->w.pttmp[0].x, p->w.pttmp[0].y);
	drawCalc_areaToimage_double(p, &x2, &y2, p->w.pttmp[1].x, p->w.pttmp[1].y);

	return atan2(y2 - y1, x2 - x1);
}

/** イメージ移動時の計算
 *
 * @param img   相対移動数を計算するためのイメージ
 * @param ptret 相対移動数が入る
 * @return 1px でも移動するか */

mBool drawCalc_moveImage_onMotion(DrawData *p,TileImage *img,uint32_t state,mPoint *ptret)
{
	mPoint pt,pt2;
	double x,y;

	//カーソル移動距離

	x = p->w.dptAreaCur.x - p->w.dptAreaLast.x;
	y = p->w.dptAreaCur.y - p->w.dptAreaLast.y;

	if(state & M_MODS_CTRL)  x = 0;
	if(state & M_MODS_SHIFT) y = 0;

	//総移動数

	p->w.ptd_tmp[0].x += x;
	p->w.ptd_tmp[0].y += y;

	//総移動数を キャンバス -> イメージ 座標変換

	pt2.x = (int)p->w.ptd_tmp[0].x;
	pt2.y = (int)p->w.ptd_tmp[0].y;

	drawCalc_areaToimage_relative(p, &pt, &pt2);

	//移動開始時のオフセット位置を加算して絶対値に

	pt.x += p->w.pttmp[0].x;
	pt.y += p->w.pttmp[0].y;

	//現在位置からの相対移動数

	TileImage_getOffset(img, &pt2);

	pt.x -= pt2.x;
	pt.y -= pt2.y;

	if(pt.x == 0 && pt.y == 0)
		return FALSE;
	else
	{
		//総相対移動数
		p->w.pttmp[1].x += pt.x;
		p->w.pttmp[1].y += pt.y;
	
		*ptret = pt;

		return TRUE;
	}
}


//==============================
// ほか
//==============================


/** 1段階上げ/下げした表示倍率取得 */

int drawCalc_getZoom_step(DrawData *p,mBool zoomup)
{
	int n,step;

	n = p->canvas_zoom / 10;

	if((!zoomup && n <= 100) || (zoomup && n < 100))
	{
		//100% 以下

		step = APP_CONF->canvasZoomStep_low;

		if(zoomup)
		{
			n = 100 - (100 - n - 1) / step * step;
			if(n > 100) n = 100;
		}
		else
			n = 100 - (100 - n + step) / step * step;
	}
	else
	{
		//100% 以上

		step = APP_CONF->canvasZoomStep_hi;

		if(zoomup)
			n = (n + step) / step * step;
		else
		{
			n = (n - 1) / step * step;
			if(n < 100) n = 100;
		}
	}

	//

	n *= 10;

	if(n < CANVAS_ZOOM_MIN)
		n = CANVAS_ZOOM_MIN;
	else if(n > CANVAS_ZOOM_MAX)
		n = CANVAS_ZOOM_MAX;

	return n;
}

/** ウィンドウに収まる表示倍率を取得
 *
 * 最大で 100% */

int drawCalc_getZoom_fitWindow(DrawData *p)
{
	mBox box;
	int w,h;

	//領域の余白 10px 分を引く

	if(p->szCanvas.w < 20 || p->szCanvas.h < 20)
		w = h = 21;
	else
	{
		w = p->szCanvas.w - 20;
		h = p->szCanvas.h - 20;
	}

	//

	box.x = box.y = 0;
	box.w = p->imgw, box.h = p->imgh;

	mBoxScaleKeepAspect(&box, w, h, TRUE);

	//倍率

	return (int)((double)box.w / p->imgw * 1000 + 0.5);
}

/** キャンバスのスクロールバーの最大値取得
 *
 * 四隅をイメージ座標 -> 領域座標変換し、一番大きい半径の値+余白 */

void drawCalc_getCanvasScrollMax(DrawData *p,mSize *size)
{
	mPoint pt[4];
	int i,r,rmax = 0;
	double x,y;

	//イメージの四隅を領域座標に変換。
	//キャンバス中央からの距離の最大値を取得。

	mMemzero(pt, sizeof(mPoint) * 4);

	pt[1].x = pt[3].x = p->imgw;
	pt[2].y = pt[3].y = p->imgh;

	for(i = 0; i < 4; i++)
	{
		drawCalc_imageToarea_pt(p, pt + i, pt[i].x, pt[i].y);

		x = pt[i].x - (p->szCanvas.w >> 1);
		y = pt[i].y - (p->szCanvas.h >> 1);

		r = (int)(sqrt(x * x + y * y) + 0.5);
		if(rmax < r) rmax = r;
	}

	//

	size->w = rmax * 2 + p->szCanvas.w;
	size->h = rmax * 2 + p->szCanvas.h;
}


//===============================
// defCanvasInfo.h
//===============================


/** イメージ座標からキャンバス座標に変換 */

void CanvasDrawInfo_imageToarea(CanvasDrawInfo *p,double x,double y,double *px,double *py)
{
	x = (x - p->originx) * p->param->scale;
	y = (y - p->originy) * p->param->scale;
	if(p->mirror) x = -x;

	*px = (x * p->param->cos - y * p->param->sin) - p->scrollx;
	*py = (x * p->param->sin + y * p->param->cos) - p->scrolly;
}

/** イメージ座標からキャンバス座標に変換 (=> mPoint) */

void CanvasDrawInfo_imageToarea_pt(CanvasDrawInfo *p,double x,double y,mPoint *pt)
{
	x = (x - p->originx) * p->param->scale;
	y = (y - p->originy) * p->param->scale;
	if(p->mirror) x = -x;

	pt->x = floor((x * p->param->cos - y * p->param->sin) - p->scrollx);
	pt->y = floor((x * p->param->sin + y * p->param->cos) - p->scrolly);
}

/** イメージ座標を +1 した時のキャンバス座標の増加幅を取得
 *
 * @param dst 0:x-x 1:x-y 2:y-x 3:y-y */

void CanvasDrawInfo_getImageIncParam(CanvasDrawInfo *p,double *dst)
{
	dst[0] = p->param->scale * p->param->cos;
	dst[1] = p->param->scale * p->param->sin;
	dst[2] = -dst[1];
	dst[3] = dst[0];

	if(p->mirror)
	{
		dst[0] = -dst[0];
		dst[1] = -dst[1];
	}
}

