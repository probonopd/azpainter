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
 * 効果
 **************************************/

#include <math.h>

#include "mDef.h"
#include "mRandXorShift.h"

#include "TileImage.h"
#include "TileImageDrawInfo.h"

#include "FilterDrawInfo.h"
#include "filter_sub.h"


/** グロー */

static void _glow_setsrc(RGBAFix15 *buf,int num,FilterDrawInfo *info)
{
	int i,n,min;

	//明るさ補正

	min = info->ntmp[0];

	for(; num; num--, buf++)
	{
		if(buf->a)
		{
			for(i = 0; i < 3; i++)
			{
				n = buf->c[i];
				if(n < min) n = min;

				buf->c[i] = ((n - min) << 15) / (0x8000 - min);
			}
		}
	}
}

static void _glow_setpixel(int x,int y,RGBAFix15 *pixg,FilterDrawInfo *info)
{
	RGBAFix15 pix;
	int i,n,a;

	TileImage_getPixel(info->imgsrc, x, y, &pix);
	if(pix.a == 0) return;

	//スクリーン合成 (imgsrc + ガウスぼかし後の色)

	a = info->ntmp[1] * pix.a >> 8;

	for(i = 0; i < 3; i++)
	{
		n = pix.c[i] + pixg->c[i] - (pix.c[i] * pixg->c[i] >> 15);

		pix.c[i] = ((n - pix.c[i]) * a >> 15) + pix.c[i];
	}

	(g_tileimage_dinfo.funcDrawPixel)(info->imgdst, x, y, &pix);
}

mBool FilterDraw_effect_glow(FilterDrawInfo *info)
{
	info->ntmp[0] = 0x8000 - Density100toFix15(info->val_bar[0]);
	info->ntmp[1] = (info->val_bar[2] << 8) / 100;

	return FilterSub_gaussblur(info, info->imgsrc, info->val_bar[1],
		_glow_setpixel, _glow_setsrc);
}

/** RGB ずらし */
/* ntmp : [0]R-x [1]R-y [2]B-x [3]B-y */

static mBool _commfunc_rgbshift(FilterDrawInfo *info,int x,int y,RGBAFix15 *pix)
{
	RGBAFix15 pix2;

	if(pix->a == 0) return FALSE;

	//R

	FilterSub_getPixelSrc_clip(info, x + info->ntmp[0], y + info->ntmp[1], &pix2);
	pix->r = pix2.r;

	//B
	
	FilterSub_getPixelSrc_clip(info, x + info->ntmp[2], y + info->ntmp[3], &pix2);
	pix->b = pix2.b;

	return TRUE;
}

mBool FilterDraw_effect_rgbshift(FilterDrawInfo *info)
{
	int i;

	//R と B のずらす方向

	for(i = 0; i < 4; i++)
		info->ntmp[i] = 0;

	i = info->val_bar[0];	//距離

	switch(info->val_combo[0])
	{
		//斜め
		case 0:
			info->ntmp[0] = info->ntmp[1] = info->ntmp[3] = i;
			info->ntmp[2] = -i;
			break;
		//横
		case 1:
			info->ntmp[0] = i;
			info->ntmp[2] = -i;
			break;
		//縦
		default:
			info->ntmp[1] = i;
			info->ntmp[3] = -i;
			break;
	}

	return FilterSub_drawCommon(info, _commfunc_rgbshift);
}

/** 油絵風 */

mBool FilterDraw_effect_oilpaint(FilterDrawInfo *info)
{
	int ix,iy,i,j,jx,jy,vmin,min[3],max[3];
	mPoint ptst[4],pted[4];
	double d[4],weight;
	RGBAFix15 pix,pixmin;
	TileImageSetPixelFunc setpix;

	FilterSub_getPixelFunc(&setpix);

	mMemzero(ptst, sizeof(mPoint) * 4);
	mMemzero(pted, sizeof(mPoint) * 4);

	i = info->val_bar[0];

	ptst[0].x = ptst[0].y = ptst[1].y = ptst[3].x = -i;
	pted[1].x = pted[2].x = pted[2].y = pted[3].y = i;

	weight = 1.0 / ((i + 1) * (i + 1));

	//

	for(iy = info->rc.y1; iy <= info->rc.y2; iy++)
	{
		for(ix = info->rc.x1; ix <= info->rc.x2; ix++)
		{
			vmin = 0;

			for(i = 0; i < 4; i++)
			{
				d[0] = d[1] = d[2] = d[3] = 0;

				min[0] = min[1] = min[2] = 0x8000;
				max[0] = max[1] = max[2] = 0;

				for(jy = ptst[i].y; jy <= pted[i].y; jy++)
				{
					for(jx = ptst[i].x; jx <= pted[i].x; jx++)
					{
						FilterSub_getPixelSrc_clip(info, ix + jx, iy + jy, &pix);

						FilterSub_addAdvColor(d, &pix);

						//RGB 最小、最大値

						for(j = 0; j < 3; j++)
						{
							if(pix.c[j] < min[j]) min[j] = pix.c[j];
							if(pix.c[j] > max[j]) max[j] = pix.c[j];
						}
					}
				}

				//

				j = max[0] - min[0] + max[1] - min[1] + max[2] - min[2];

				if(i == 0 || j < vmin)
				{
					vmin = j;
					FilterSub_getAdvColor(d, weight, &pixmin);
				}
			}

			(setpix)(info->imgdst, ix, iy, &pixmin);
		}

		FilterSub_progIncSubStep(info);
	}

	return TRUE;
}

/** エンボス */

mBool FilterDraw_effect_emboss(FilterDrawInfo *info)
{
	Filter3x3Info dat;
	int i;

	for(i = 0; i < 9; i++)
		dat.mul[i] = 0;

	//強さ

	i = info->val_bar[0];
	if(info->val_ckbtt[0]) i = -i;	//反転

	//方向

	switch(info->val_combo[0])
	{
		case 0:
			dat.mul[0] = dat.mul[1] = -i, dat.mul[7] = dat.mul[8] = i;
			break;
		case 1:
			dat.mul[3] = -i, dat.mul[5] = i;
			break;
		default:
			dat.mul[1] = -i, dat.mul[7] = i;
			break;
	}

	//

	dat.grayscale = info->val_ckbtt[1];

	if(dat.grayscale)
		dat.add = 0x4000;
	else
	{
		dat.mul[4] = 1;
		dat.add = 0;
	}

	dat.divmul = 1;

	return FilterSub_draw3x3(info, &dat);
}

/** ノイズ */

static mBool _commfunc_noise(FilterDrawInfo *info,int x,int y,RGBAFix15 *pix)
{
	int i,rnd,n,val;

	if(pix->a == 0) return FALSE;

	val = info->ntmp[1];

	for(i = 0; i < 3; i++)
	{
		if(info->ntmp[0] || i == 0)
			rnd = mRandXorShift_getIntRange(-val, val);

		n = pix->c[i] + rnd;

		if(n < 0) n = 0;
		else if(n > 0x8000) n = 0x8000;

		pix->c[i] = n;
	}

	return TRUE;
}

mBool FilterDraw_effect_noise(FilterDrawInfo *info)
{
	info->ntmp[0] = !(info->val_ckbtt[0]);	//RGBか
	info->ntmp[1] = Density100toFix15(info->val_bar[0]);

	return FilterSub_drawCommon(info, _commfunc_noise);
}

/** 拡散 */

static mBool _commfunc_diffusion(FilterDrawInfo *info,int x,int y,RGBAFix15 *pix)
{
	int val = info->val_bar[0];

	x += mRandXorShift_getIntRange(-val, val);
	y += mRandXorShift_getIntRange(-val, val);

	FilterSub_getPixelSrc_clip(info, x, y, pix);

	return TRUE;
}

mBool FilterDraw_effect_diffusion(FilterDrawInfo *info)
{
	return FilterSub_drawCommon(info, _commfunc_diffusion);
}

/** ひっかき */

mBool FilterDraw_effect_scratch(FilterDrawInfo *info)
{
	int ix,iy,yrange,xlen,ylen,x,y;
	double d;
	RGBAFix15 pix;
	mRect rc;

	yrange = info->val_bar[0] << 1;

	d = -info->val_bar[1] / 180.0 * M_MATH_PI;
	xlen = (int)(info->val_bar[0] * cos(d));
	ylen = (int)(info->val_bar[0] * sin(d));

	//

	FilterSub_copyImage_forPreview(info);

	rc = info->rc;

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		for(ix = rc.x1; ix <= rc.x2; ix++)
		{
			x = mRandXorShift_getIntRange(rc.x1, rc.x2);
			y = iy + mRandXorShift_getIntRange(-yrange, yrange);

			FilterSub_getPixelSrc_clip(info, x, y, &pix);

			x -= xlen >> 1;
			y -= ylen >> 1;

			TileImage_drawLine_forFilter(info->imgdst,
				x, y, x + xlen, y + ylen, &pix, &rc);
		}

		FilterSub_progIncSubStep(info);
	}

	return TRUE;
}

/** メディアン */

mBool FilterDraw_effect_median(FilterDrawInfo *info)
{
	int ix,iy,jx,jy,no,radius,num,pos;
	uint16_t col_v[7*7];
	RGBAFix15 *buf,pix;
	TileImageSetPixelFunc setpix;

	FilterSub_getPixelFunc(&setpix);

	radius = info->val_bar[0];
	num = radius * 2 + 1;
	num *= num;

	//バッファ

	buf = (RGBAFix15 *)mMalloc(8 * num, FALSE);
	if(!buf) return FALSE;

	//取得する値の位置

	ix = info->val_combo[0];

	if(ix == 0)
		pos = 0;
	else if(ix == 1)
		pos = num >> 1;
	else
		pos = num - 1;

	//

	for(iy = info->rc.y1; iy <= info->rc.y2; iy++)
	{
		for(ix = info->rc.x1; ix <= info->rc.x2; ix++)
		{
			//周囲の色と輝度を取得

			for(jy = -radius, no = 0; jy <= radius; jy++)
			{
				for(jx = -radius; jx <= radius; jx++, no++)
				{
					FilterSub_getPixelSrc_clip(info, ix + jx, iy + jy, &pix);

					buf[no] = pix;
					col_v[no] = RGB15_TO_GRAY(pix.r, pix.g, pix.b);
				}
			}

			//輝度の小さい順に並び替え

			for(jx = 0; jx < num - 1; jx++)
			{
				no = jx;

				for(jy = jx + 1; jy < num; jy++)
				{
					if(col_v[jy] < col_v[no]) no = jy;
				}

				if(no != jx)
				{
					jy = col_v[jx], col_v[jx] = col_v[no], col_v[no] = jy;
					pix = buf[jx], buf[jx] = buf[no], buf[no] = pix;
				}
			}

			//セット

			(setpix)(info->imgdst, ix, iy, buf + pos);
		}

		FilterSub_progIncSubStep(info);
	}

	mFree(buf);

	return TRUE;
}

/** ぶれ */

mBool FilterDraw_effect_blurring(FilterDrawInfo *info)
{
	int ix,iy,i;
	double d[4];
	mPoint pt[4];
	RGBAFix15 pix;
	TileImage *imgsrc;
	TileImageSetPixelFunc setpix;

	FilterSub_getPixelFunc(&setpix);

	i = info->val_bar[0];

	pt[0].x = pt[0].y = pt[1].x = pt[2].y = -i;
	pt[1].y = pt[2].x = pt[3].x = pt[3].y = i;

	//

	imgsrc = info->imgsrc;

	for(iy = info->rc.y1; iy <= info->rc.y2; iy++)
	{
		for(ix = info->rc.x1; ix <= info->rc.x2; ix++)
		{
			d[0] = d[1] = d[2] = d[3] = 0;

			for(i = 0; i < 4; i++)
				FilterSub_addAdvColor_at(imgsrc, ix + pt[i].x, iy + pt[i].y, d, info->clipping);

			FilterSub_getAdvColor(d, 1.0 / 4, &pix);

			(setpix)(info->imgdst, ix, iy, &pix);
		}

		FilterSub_progIncSubStep(info);
	}

	return TRUE;
}
