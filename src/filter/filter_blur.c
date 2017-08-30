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
 * ぼかし
 **************************************/

#include <math.h>

#include "mDef.h"

#include "TileImage.h"

#include "FilterDrawInfo.h"
#include "filter_sub.h"


/** ぼかし */

mBool FilterDraw_blur(FilterDrawInfo *info)
{
	RGBAFix15 *buf1,*buf2,pix,*ps,*psY,*pd;
	int ix,iy,jx,jy,i,range,pixlen,xnum,ynum,pos;
	double d[4],weight;
	mRect rc;
	TileImageSetPixelFunc setpix;

	FilterSub_getPixelFunc(&setpix);

	range = info->val_bar[0];
	pixlen = range * 2 + 64;
	weight = 1.0 / (range * 2 + 1);

	//バッファ

	buf1 = (RGBAFix15 *)mMalloc(8 * pixlen * pixlen, FALSE);
	buf2 = (RGBAFix15 *)mMalloc(8 * 64 * pixlen, FALSE);

	if(!buf1 || !buf2)
	{
		mFree(buf1);
		return FALSE;
	}

	//64x64 取得範囲

	rc.y1 = info->rc.y1 - range;
	rc.y2 = rc.y1 + pixlen - 1;

	//64x64 を 1 とする

	FilterSub_progBeginOneStep(info, 50, ((info->box.w + 63) / 64) * ((info->box.h + 63) / 64));

	//64x64 単位で、バッファに取得して処理

	for(iy = info->rc.y1; iy <= info->rc.y2; iy += 64)
	{
		rc.x1 = info->rc.x1 - range;
		rc.x2 = rc.x1 + pixlen - 1;
	
		for(ix = info->rc.x1; ix <= info->rc.x2; ix += 64, rc.x1 += 64, rc.x2 += 64)
		{
			//buf1 に 64x64 のソースセット (すべて透明なら処理なし)
		
			if(FilterSub_getPixelsToBuf(info->imgsrc, &rc, buf1, info->clipping))
			{
				FilterSub_progIncSubStep(info);
				continue;
			}

			//水平方向
			/* src: buf1 [x]pixlen - [y]pixlen
			 * dst: buf2 [y]pixlen - [x]64 */

			ps = buf1 + range;
			pd = buf2;

			for(jy = 0; jy < pixlen; jy++, pd++)
			{
				for(jx = 0, pos = 0; jx < 64; jx++, ps++, pos += pixlen)
				{
					d[0] = d[1] = d[2] = d[3] = 0;

					for(i = -range; i <= range; i++)
						FilterSub_addAdvColor(d, ps + i);

					FilterSub_getAdvColor(d, weight, pd + pos);
				}

				ps += pixlen - 64;
			}

			//垂直方向

			xnum = ynum = 64;
			if(ix + 63 > info->rc.x2) xnum = info->rc.x2 + 1 - ix;
			if(iy + 63 > info->rc.y2) ynum = info->rc.y2 + 1 - iy;

			psY = buf2 + range;

			for(jx = 0; jx < xnum; jx++)
			{
				for(jy = 0, ps = psY; jy < ynum; jy++, ps++)
				{
					d[0] = d[1] = d[2] = d[3] = 0;

					for(i = -range; i <= range; i++)
						FilterSub_addAdvColor(d, ps + i);

					FilterSub_getAdvColor(d, weight, &pix);

					(setpix)(info->imgdst, ix + jx, iy + jy, &pix);
				}

				psY += pixlen;
			}

			FilterSub_progIncSubStep(info);
		}

		rc.y1 += 64;
		rc.y2 += 64;
	}

	mFree(buf1);
	mFree(buf2);

	return TRUE;
}

/** ガウスぼかし */

mBool FilterDraw_gaussblur(FilterDrawInfo *info)
{
	return FilterSub_gaussblur(info, info->imgsrc, info->val_bar[0], NULL, NULL);
}

/** モーションブラー (0.5px 間隔でガウスぼかし) */

mBool FilterDraw_motionblur(FilterDrawInfo *info)
{
	int ix,iy,i,range,pixnum,stx,sty,incx,incy,fx,fy;
	double *gaussbuf,d,dtmp,weight,dsin,dcos,dc[4];
	RGBAFix15 pix;
	TileImageSetPixelFunc setpix;
	TileImage *imgsrc;

	FilterSub_getPixelFunc(&setpix);

	range = info->val_bar[0] * 3;
	pixnum = (range * 2 + 1) * 2;  //0.5px 間隔なので2倍

	//ガウステーブル

	gaussbuf = (double *)mMalloc(sizeof(double) * pixnum, FALSE);
	if(!gaussbuf) return FALSE;

	d = -range;
	dtmp = 1.0 / (2 * info->val_bar[0] * info->val_bar[0]);
	weight = 0;

	for(i = 0; i < pixnum; i++, d += 0.5)
	{
		gaussbuf[i] = exp(-(d * d) * dtmp);

		weight += gaussbuf[i];
	}

	weight = 1.0 / weight;

	//角度

	d = -info->val_bar[1] / 180.0 * M_MATH_PI;
	dsin = sin(d);
	dcos = cos(d);

	stx = (int)(dcos * -range * (1<<16));
	sty = (int)(dsin * -range * (1<<16));
	incx = (int)(dcos * 0.5 * (1<<16));
	incy = (int)(dsin * 0.5 * (1<<16));

	//

	imgsrc = info->imgsrc;

	for(iy = info->rc.y1; iy <= info->rc.y2; iy++)
	{
		for(ix = info->rc.x1; ix <= info->rc.x2; ix++)
		{
			fx = stx, fy = sty;

			dc[0] = dc[1] = dc[2] = dc[3] = 0;

			for(i = 0; i < pixnum; i++)
			{
				FilterSub_addAdvColor_at_weight(imgsrc,
					ix + (fx >> 16), iy + (fy >> 16), dc, gaussbuf[i], info->clipping);

				fx += incx;
				fy += incy;
			}
			
			//セット

			FilterSub_getAdvColor(dc, weight, &pix);

			(setpix)(info->imgdst, ix, iy, &pix);
		}

		FilterSub_progIncSubStep(info);
	}

	mFree(gaussbuf);

	return TRUE;
}

/** 放射状ぼかし */

mBool FilterDraw_radialblur(FilterDrawInfo *info)
{
	int ix,iy,len,i;
	double factor,rr,dx,dy,rd,dsin,dcos,xx,yy,d[4];
	RGBAFix15 pix;
	TileImageSetPixelFunc setpix;
	TileImage *imgsrc;

	FilterSub_getPixelFunc(&setpix);

	factor = info->val_bar[0] * 0.01;
	imgsrc = info->imgsrc;

	for(iy = info->rc.y1; iy <= info->rc.y2; iy++)
	{
		dy = iy - info->imgy;
		
		for(ix = info->rc.x1; ix <= info->rc.x2; ix++)
		{
			dx = ix - info->imgx;

			//中央位置からの距離に強さを適用して処理長さ計算

			rr = sqrt(dx * dx + dy * dy);
			
			len = (int)(rr * factor);

			if(len < 1)
			{
				//ソースの点をコピー
				TileImage_getPixel(imgsrc, ix, iy, &pix);
				(setpix)(info->imgdst, ix, iy, &pix);
				continue;
			}

			//角度

			rd = atan2(dy, dx);
			dsin = sin(rd);
			dcos = cos(rd);

			//角度と長さの範囲から色取得

			d[0] = d[1] = d[2] = d[3] = 0;

			xx = (rr - len) * dcos + info->imgx;
			yy = (rr - len) * dsin + info->imgy;

			for(i = 0; i <= len; i++, xx += dcos, yy += dsin)
			{
				FilterSub_getLinerColor(imgsrc, xx, yy, &pix, info->clipping);

				FilterSub_addAdvColor(d, &pix);
			}

			//セット

			FilterSub_getAdvColor(d, 1.0 / (len + 1), &pix);

			(setpix)(info->imgdst, ix, iy, &pix);
		}

		FilterSub_progIncSubStep(info);
	}

	return TRUE;
}

/** レンズぼかし */

mBool FilterDraw_lensblur(FilterDrawInfo *info)
{
	int ix,iy,radius,jx,jy;
	double *tablebuf,br_base,br_res,weight,d[4],a;
	uint8_t *circlebuf,*ps,f;
	RGBAFix15 pix;
	TileImageSetPixelFunc setpix;

	FilterSub_getPixelFunc(&setpix);

	radius = info->val_bar[0];
	br_base = 1.010 + info->val_bar[1] * 0.001;
	br_res = (1<<7) / log(br_base);

	//テーブル

	tablebuf = (double *)mMalloc(sizeof(double) * 257, FALSE);
	if(!tablebuf) return FALSE;

	for(ix = 0; ix < 257; ix++)
		tablebuf[ix] = pow(br_base, ix);

	//円形データ (1bit)

	circlebuf = FilterSub_createCircleShape(radius, &ix);
	if(!circlebuf)
	{
		mFree(tablebuf);
		return FALSE;
	}

	weight = 1.0 / ix;
	
	//

	for(iy = info->rc.y1; iy <= info->rc.y2; iy++)
	{
		for(ix = info->rc.x1; ix <= info->rc.x2; ix++)
		{
			//円形範囲の平均色

			d[0] = d[1] = d[2] = d[3] = 0;

			ps = circlebuf;
			f = 0x80;

			for(jy = -radius; jy <= radius; jy++)
			{
				for(jx = -radius; jx <= radius; jx++)
				{
					if(*ps & f)
					{
						FilterSub_getPixelSrc_clip(info, ix + jx, iy + jy, &pix);

						a = (double)pix.a / 0x8000;

						d[0] += tablebuf[pix.r >> 7] * a;
						d[1] += tablebuf[pix.g >> 7] * a;
						d[2] += tablebuf[pix.b >> 7] * a;
						d[3] += a;
					}

					f >>= 1;
					if(f == 0) f = 0x80, ps++;
				}
			}

			//

			pix.a = (int)(d[3] * weight * 0x8000 + 0.5);

			if(pix.a == 0)
				pix.v64 = 0;
			else
			{
				a = 1.0 / d[3];

				for(jx = 0; jx < 3; jx++)
				{
					jy = (int)(log(d[jx] * a) * br_res);

					if(jy < 0) jy = 0;
					else if(jy > 0x8000) jy = 0x8000;

					pix.c[jx] = jy;
				}
			}

			(setpix)(info->imgdst, ix, iy, &pix);
		}

		FilterSub_progIncSubStep(info);
	}

	mFree(tablebuf);
	mFree(circlebuf);

	return TRUE;
}
