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
 * 描画
 **************************************/

#include <math.h>

#include "mDef.h"
#include "mRandXorShift.h"

#include "TileImage.h"

#include "FilterDrawInfo.h"
#include "filter_sub.h"

#include "PerlinNoise.h"


/** 雲模様 */

mBool FilterDraw_cloud(FilterDrawInfo *info)
{
	PerlinNoise *perlin;
	int i,ix,iy,xx,yy,n;
	double resmul;
	mBool to_alpha;
	RGBAFix15 pix1,pix2,pix;
	TileImageSetPixelFunc setpix;

	FilterSub_getPixelFunc(&setpix);

	//PerlinNoise 初期化

	perlin = PerlinNoise_new(info->val_bar[0] / 400.0, info->val_bar[1] * 0.01);
	if(!perlin) return FALSE;

	//

	resmul = info->val_bar[2] * 600 + 5000;

	//色

	to_alpha = FALSE;

	switch(info->val_combo[0])
	{
		//黒/白
		case 0:
			pix1.v64 = 0;
			pix2.r = pix2.g = pix2.b = 0x8000;
			break;
		//描画色/背景色
		case 1:
			pix1 = info->rgba15Draw;
			pix2 = info->rgba15Bkgnd;
			break;
		//黒+アルファ値
		default:
			pix.v64 = 0;
			to_alpha = TRUE;
			break;
	}

	//

	pix.a = 0x8000;

	for(iy = info->rc.y1, yy = 0; iy <= info->rc.y2; iy++, yy++)
	{
		for(ix = info->rc.x1, xx = 0; ix <= info->rc.x2; ix++, xx++)
		{
			n = (int)(PerlinNoise_getNoise(perlin, xx, yy) * resmul + 0x4000);

			if(n < 0) n = 0;
			else if(n > 0x8000) n = 0x8000;

			if(to_alpha)
				pix.a = n;
			else
			{
				for(i = 0; i < 3; i++)
					pix.c[i] = ((pix2.c[i] - pix1.c[i]) * n >> 15) + pix1.c[i];
			}

			(setpix)(info->imgdst, ix, iy, &pix);
		}

		FilterSub_progIncSubStep(info);
	}
	
	PerlinNoise_free(perlin);

	return TRUE;
}

/** アミトーン描画
 *
 * @param len ボックスの1辺の長さ
 *
 * val_bar : [0] 線数またはサイズ [1] 密度 [2] 角度 [3] DPI */

static mBool _draw_amitone(FilterDrawInfo *info,double len)
{
	int density,c,ix,iy,ixx,iyy;
	double rr,dcos,dsin,len_half,dsx,dsy,xx,yy,cx,cy,dyy;
	RGBAFix15 pix;
	TileImageSetPixelFunc setpix;

	FilterSub_getPixelFunc(&setpix);

	FilterSub_getDrawColor_fromType(info, info->val_combo[0], &pix);

	density = info->val_bar[1];

	rr = -info->val_bar[2] / 180.0 * M_MATH_PI;
	dcos = cos(rr);
	dsin = sin(rr);

	c = (density > 50)? 100 - density: density;		//密度 50% 以上は反転
	rr = sqrt(len * len * (c * 0.01) / M_MATH_PI);	//点の半径
	rr *= rr;

	len_half = len * 0.5;

	//--------

	for(iy = info->rc.y1; iy <= info->rc.y2; iy++)
	{
		dsx = info->rc.x1;
		dsy = iy;

		xx = dsx * dcos - dsy * dsin;
		yy = dsx * dsin + dsy * dcos;

		for(ix = info->rc.x1; ix <= info->rc.x2; ix++)
		{
			//ボックスの中心位置

			cx = floor(xx / len) * len + len_half;
			cy = floor(yy / len) * len + len_half;

			//

			if(info->val_ckbtt[0])
			{
				//アンチエイリアス (5x5)

				c = 0;

				for(iyy = 0, dsy = yy - cy; iyy < 5; iyy++, dsy += 0.2)
				{
					dyy = dsy * dsy;
				
					for(ixx = 0, dsx = xx - cx; ixx < 5; ixx++, dsx += 0.2)
					{
						if(dsx * dsx + dyy < rr)
							c += 0x8000;
					}
				}

				c /= 25;
			}
			else
			{
				//非アンチエイリアス

				dsx = xx - cx;
				dsy = yy - cy;

				if(dsx * dsx + dsy * dsy < rr)
					c = 0x8000;
				else
					c = 0;
			}

			//50% 以上は反転

			if(density > 50)
				c = 0x8000 - c;

			pix.a = c;

			(setpix)(info->imgdst, ix, iy, &pix);

			//

			xx += dcos;
			yy += dsin;
		}

		FilterSub_progIncSubStep(info);
	}

	return TRUE;
}

/** アミトーン1 */

mBool FilterDraw_amitone1(FilterDrawInfo *info)
{
	return _draw_amitone(info, (double)info->val_bar[3] / (info->val_bar[0] * 0.1));
}

/** アミトーン2 */

mBool FilterDraw_amitone2(FilterDrawInfo *info)
{
	return _draw_amitone(info, info->val_bar[0] * 0.1);
}

/** ランダムに点描画 */

mBool FilterDraw_draw_randpoint(FilterDrawInfo *info)
{
	FilterDrawPointInfo dat;
	FilterDrawPointFunc setpix;
	int amount,opamax,rmax,rmin,rlen,opamin,opalen,ix,iy,n;

	FilterSub_getDrawPointFunc(info->val_combo[0], &setpix);

	//FilterDrawPointInfo

	dat.info = info;
	dat.imgdst = info->imgdst;
	dat.coltype = info->val_combo[1];
	dat.masktype = 0;

	//

	amount = info->val_bar[0];
	rmax = info->val_bar[1];
	opamax = Density100toFix15(info->val_bar[2]);
	rmin = (info->val_bar[3] << 8) / 100;
	rlen = 256 - rmin;
	opamin = (info->val_bar[4] << 8) / 100;
	opalen = 256 - opamin;

	FilterSub_initDrawPoint(info, FALSE);
	
	//プレビュー時は背景としてソースをコピー

	FilterSub_copyImage_forPreview(info);

	//

	for(iy = info->rc.y1; iy <= info->rc.y2; iy++)
	{
		for(ix = info->rc.x1; ix <= info->rc.x2; ix++)
		{
			//量調整
			
			if((mRandXorShift_getUint32() & 8191) >= amount)
				continue;

			//半径

			n = (mRandXorShift_getIntRange(0, 256) * rlen >> 8) + rmin;
			dat.radius = rmax * n >> 8;

			//濃度

			n = (mRandXorShift_getIntRange(0, 256) * opalen >> 8) + opamin;
			dat.opacity = opamax * n >> 8;

			//セット

			if(dat.radius > 0 && dat.opacity)
				(setpix)(ix, iy, &dat);
		}

		FilterSub_progIncSubStep(info);
	}
	
	return TRUE;
}

/** 縁に沿って点描画 */

mBool FilterDraw_draw_edgepoint(FilterDrawInfo *info)
{
	FilterDrawPointInfo dat;
	FilterDrawPointFunc setpix;
	int type,ix,iy;
	TileImage *imgsrc;

	type = info->val_combo[0];

	FilterSub_getDrawPointFunc(info->val_combo[1], &setpix);

	//FilterDrawPointInfo

	dat.info = info;
	dat.imgsrc = info->imgsrc;
	dat.imgdst = info->imgdst;
	dat.radius = info->val_bar[0];
	dat.opacity = Density100toFix15(info->val_bar[1]);
	dat.coltype = info->val_combo[2];
	dat.masktype = type + 1;

	//不透明部分の外側の場合は、アルファ値比較上書きで描画

	FilterSub_initDrawPoint(info, (type == 0));
	
	//プレビュー時はソースをコピー (点を描画しない部分はソースのままにするため)

	FilterSub_copyImage_forPreview(info);

	//

	imgsrc = info->imgsrc;

	for(iy = info->rc.y1; iy <= info->rc.y2; iy++)
	{
		for(ix = info->rc.x1; ix <= info->rc.x2; ix++)
		{
			if(type)
			{
				//不透明部分の内側

				if(TileImage_isPixelOpaque(imgsrc, ix, iy))
					continue;

				if(TileImage_isPixelTransparent(imgsrc, ix - 1, iy)
					&& TileImage_isPixelTransparent(imgsrc, ix + 1, iy)
					&& TileImage_isPixelTransparent(imgsrc, ix, iy - 1)
					&& TileImage_isPixelTransparent(imgsrc, ix, iy + 1))
					continue;
			}
			else
			{
				//不透明部分の外側

				if(TileImage_isPixelTransparent(imgsrc, ix, iy))
					continue;

				if(TileImage_isPixelOpaque(imgsrc, ix - 1, iy)
					&& TileImage_isPixelOpaque(imgsrc, ix + 1, iy)
					&& TileImage_isPixelOpaque(imgsrc, ix, iy - 1)
					&& TileImage_isPixelOpaque(imgsrc, ix, iy + 1))
					continue;
			}

			//セット

			(setpix)(ix, iy, &dat);
		}

		FilterSub_progIncSubStep(info);
	}
	
	return TRUE;
}

/** 枠線 */

mBool FilterDraw_draw_frame(FilterDrawInfo *info)
{
	int i,w,h,x,y;
	RGBAFix15 pix;

	FilterSub_getDrawColor_fromType(info, info->val_combo[0], &pix);

	x = y = 0;
	FilterSub_getImageSize(&w, &h);

	for(i = info->val_bar[0]; i && w > 0 && h > 0; i--, x++, y++)
	{
		TileImage_drawBox(info->imgdst, x, y, w, h, &pix);

		w -= 2;
		h -= 2;
	}

	return TRUE;
}

/** 水平線＆垂直線 */

mBool FilterDraw_draw_horzvertLine(FilterDrawInfo *info)
{
	int min_w,min_it,len_w,len_it,pos,w,ix,iy;
	mRect rc;
	RGBAFix15 pix;
	TileImageSetPixelFunc setpix;

	FilterSub_getPixelFunc(&setpix);
	FilterSub_getDrawColor_fromType(info, info->val_combo[0], &pix);

	rc = info->rc;

	min_w = info->val_bar[0];
	min_it = info->val_bar[2];
	len_w = info->val_bar[1] - min_w;
	len_it = info->val_bar[3] - min_it;

	if(len_w < 0) len_w = 0;
	if(len_it < 0) len_it = 0;

	FilterSub_progSetMax(info, 2);

	//横線

	if(info->val_ckbtt[0])
	{
		pos = rc.y1;

		while(pos <= rc.y2)
		{
			//太さ

			w = mRandXorShift_getIntRange(0, len_w) + min_w;
			if(pos + w > rc.y2) w = rc.y2 - pos + 1;

			//描画

			for(iy = 0; iy < w; iy++)
			{
				for(ix = rc.x1; ix <= rc.x2; ix++)
					(setpix)(info->imgdst, ix, pos + iy, &pix);
			}

			//次の位置

			pos += mRandXorShift_getIntRange(0, len_it) + min_it + w;
		}
	}

	FilterSub_progInc(info);
	
	//縦線

	if(info->val_ckbtt[1])
	{
		pos = rc.x1;

		while(pos <= rc.x2)
		{
			//太さ

			w = mRandXorShift_getIntRange(0, len_w) + min_w;
			if(pos + w > rc.x2) w = rc.x2 - pos + 1;

			//描画

			for(iy = rc.y1; iy <= rc.y2; iy++)
			{
				for(ix = 0; ix < w; ix++)
					(setpix)(info->imgdst, pos + ix, iy, &pix);
			}

			//次の位置

			pos += mRandXorShift_getIntRange(0, len_it) + min_it + w;
		}
	}

	FilterSub_progInc(info);

	return TRUE;
}

/** チェック柄 */

mBool FilterDraw_draw_plaid(FilterDrawInfo *info)
{
	int colw,roww,colmod,rowmod,ix,iy,xf,yf;
	RGBAFix15 pix;
	TileImageSetPixelFunc setpix;

	FilterSub_getPixelFunc(&setpix);
	FilterSub_getDrawColor_fromType(info, info->val_combo[0], &pix);

	//

	colw = info->val_bar[0];
	roww = info->val_bar[1];

	if(info->val_ckbtt[0]) roww = colw;

	colmod = colw << 1;
	rowmod = roww << 1;

	//

	for(iy = info->rc.y1; iy <= info->rc.y2; iy++)
	{
		yf = (iy % rowmod) / roww;
	
		for(ix = info->rc.x1; ix <= info->rc.x2; ix++)
		{
			xf = (ix % colmod) / colw;

			if((xf ^ yf) == 0)
				(setpix)(info->imgdst, ix, iy, &pix);
		}

		FilterSub_progIncSubStep(info);
	}
	
	return TRUE;
}

