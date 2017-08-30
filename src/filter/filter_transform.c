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

/**************************************
 * フィルタ処理
 *
 * 変形
 **************************************/

#include <math.h>

#include "mDef.h"
#include "mPopupProgress.h"
#include "mRandXorShift.h"

#include "TileImage.h"

#include "FilterDrawInfo.h"
#include "filter_sub.h"


/** 波形 */

mBool FilterDraw_trans_wave(FilterDrawInfo *info)
{
	int ix,iy,type,flags;
	double factor,freq,dx,dy;
	RGBAFix15 pix;
	TileImageSetPixelFunc setpix;

	FilterSub_getPixelFunc(&setpix);

	factor = info->val_bar[0];
	freq = info->val_bar[1] * 0.1 * M_MATH_PI / 180;
	type = info->val_combo[0] + 1;

	flags = 0;
	if(info->clipping) flags |= 1;
	if(info->val_ckbtt[0]) flags |= 2;

	//

	for(iy = info->rc.y1; iy <= info->rc.y2; iy++)
	{
		for(ix = info->rc.x1; ix <= info->rc.x2; ix++)
		{
			dx = ix, dy = iy;

			if(type & 1) dx += factor * sin(freq * iy);
			if(type & 2) dy += factor * sin(freq * ix);

			FilterSub_getLinerColor(info->imgsrc, dx, dy, &pix, flags);

			(setpix)(info->imgdst, ix, iy, &pix);
		}

		FilterSub_progIncSubStep(info);
	}

	return TRUE;
}

/** 波紋 */

mBool FilterDraw_trans_ripple(FilterDrawInfo *info)
{
	int ix,iy;
	double factor,freq,dx,dy,r,rg;
	RGBAFix15 pix;
	TileImageSetPixelFunc setpix;

	FilterSub_getPixelFunc(&setpix);

	freq = info->val_bar[0] * 0.01;
	factor = info->val_bar[1] * 0.1;

	//

	for(iy = info->rc.y1; iy <= info->rc.y2; iy++)
	{
		for(ix = info->rc.x1; ix <= info->rc.x2; ix++)
		{
			dx = ix - info->imgx;
			dy = iy - info->imgy;

			r = sqrt(dx * dx + dy * dy);
			rg = atan2(dy, dx);

			r += sin(r * freq) * factor;

			dx = r * cos(rg) + info->imgx;
			dy = r * sin(rg) + info->imgy;
		
			FilterSub_getLinerColor(info->imgsrc, dx, dy, &pix, info->clipping);

			(setpix)(info->imgdst, ix, iy, &pix);
		}

		FilterSub_progIncSubStep(info);
	}

	return TRUE;
}

/** 極座標 */

mBool FilterDraw_trans_polar(FilterDrawInfo *info)
{
	int ix,iy,type,imgw,imgh,right,bottom,bkgnd;
	double dx,dy,xx,yy,cx,cy;
	RGBAFix15 pix;
	TileImageSetPixelFunc setpix;

	FilterSub_getImageSize(&imgw, &imgh);
	FilterSub_getPixelFunc(&setpix);

	type = info->val_combo[0];
	bkgnd = info->val_combo[1];

	right = imgw - 1; if(right == 0) right = 1;
	bottom = imgh - 1;

	cx = imgw * 0.5;
	cy = imgh * 0.5;

	//

	for(iy = info->rc.y1; iy <= info->rc.y2; iy++)
	{
		for(ix = info->rc.x1; ix <= info->rc.x2; ix++)
		{
			if(type)
			{
				//極座標 -> 直交座標

				xx = (double)(right - ix) / right * M_MATH_PI * 2 - M_MATH_PI / 2;
				yy = iy * 0.5;

				dx = yy * cos(xx) + cx;
				dy = yy * sin(xx) + cy;
			}
			else
			{
				//直交座標 -> 極座標

				xx = ix - cx;
				yy = cy - (bottom - iy);

				dx = (atan2(xx, yy) + M_MATH_PI) / (M_MATH_PI * 2) * right;
				dy = sqrt(xx * xx + yy * yy) * 2;
			}
		
			FilterSub_getLinerColor_bkgnd(info->imgsrc, dx, dy, ix, iy, &pix, bkgnd);

			(setpix)(info->imgdst, ix, iy, &pix);
		}

		FilterSub_progIncSubStep(info);
	}

	return TRUE;
}

/** 放射状ずらし */

mBool FilterDraw_trans_radial_shift(FilterDrawInfo *info)
{
	int ix,iy,n;
	int16_t *buf;
	double dx,dy,r,rg;
	RGBAFix15 pix;
	TileImageSetPixelFunc setpix;

	FilterSub_getPixelFunc(&setpix);

	//角度ごとのずらす量

	buf = (int16_t *)mMalloc(2 * 1024, FALSE);
	if(!buf) return FALSE;

	iy = info->val_bar[0];

	for(ix = 0; ix < 1024; ix++)
		buf[ix] = mRandXorShift_getIntRange(0, iy);

	//

	for(iy = info->rc.y1; iy <= info->rc.y2; iy++)
	{
		for(ix = info->rc.x1; ix <= info->rc.x2; ix++)
		{
			dx = ix - info->imgx;
			dy = iy - info->imgy;

			r = sqrt(dx * dx + dy * dy);
			rg = atan2(dy, dx);

			n = (int)(rg * 512 / M_MATH_PI) & 1023;

			r -= buf[n];

			if(r <= 0)
				TileImage_getPixel(info->imgsrc, info->imgx, info->imgy, &pix);
			else
			{
				dx = r * cos(rg) + info->imgx;
				dy = r * sin(rg) + info->imgy;

				FilterSub_getLinerColor(info->imgsrc, dx, dy, &pix, 0);
			}

			(setpix)(info->imgdst, ix, iy, &pix);
		}

		FilterSub_progIncSubStep(info);
	}

	mFree(buf);

	return TRUE;
}

/** 渦巻き */

mBool FilterDraw_trans_swirl(FilterDrawInfo *info)
{
	int ix,iy,bkgnd;
	double dx,dy,zoom,factor,r,rg;
	RGBAFix15 pix;
	TileImageSetPixelFunc setpix;

	FilterSub_getPixelFunc(&setpix);

	factor = info->val_bar[0] * 0.001;
	zoom = 1.0 / (info->val_bar[1] * 0.01);
	bkgnd = info->val_combo[0];

	//

	for(iy = info->rc.y1; iy <= info->rc.y2; iy++)
	{
		for(ix = info->rc.x1; ix <= info->rc.x2; ix++)
		{
			dx = ix - info->imgx;
			dy = iy - info->imgy;

			r = sqrt(dx * dx + dy * dy) * zoom;
			rg = atan2(dy, dx) + factor * r;

			dx = r * cos(rg) + info->imgx;
			dy = r * sin(rg) + info->imgy;
			
			FilterSub_getLinerColor_bkgnd(info->imgsrc, dx, dy, ix, iy, &pix, bkgnd);

			(setpix)(info->imgdst, ix, iy, &pix);
		}

		FilterSub_progIncSubStep(info);
	}

	return TRUE;
}

