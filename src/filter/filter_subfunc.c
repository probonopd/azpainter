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
 * サブ関数
 **************************************/

#include <math.h>

#include "mDef.h"
#include "mPopupProgress.h"
#include "mRandXorShift.h"
#include "mColorConv.h"

#include "defDraw.h"

#include "TileImage.h"
#include "TileImageDrawInfo.h"
#include "LayerItem.h"
#include "LayerList.h"

#include "FilterDrawInfo.h"
#include "filter_sub.h"



//========================
// 描画処理
//========================


/** 共通描画処理
 *
 * func で色を取得し、TRUE が返ったら色をセット。 */

mBool FilterSub_drawCommon(FilterDrawInfo *info,FilterDrawCommonFunc func)
{
	TileImage *imgsrc,*imgdst;
	TileImageSetPixelFunc setpix; 
	int ix,iy;
	mRect rc;
	RGBAFix15 pix;

	imgsrc = info->imgsrc;
	imgdst = info->imgdst;

	rc = info->rc;

	setpix = g_tileimage_dinfo.funcDrawPixel;

	//

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		for(ix = rc.x1; ix <= rc.x2; ix++)
		{
			TileImage_getPixel(imgsrc, ix, iy, &pix);

			if((func)(info, ix, iy, &pix))
				(setpix)(imgdst, ix, iy, &pix);
		}

		if(info->prog)
			mPopupProgressThreadIncSubStep(info->prog);
	}

	return TRUE;
}

/** 3x3 フィルタ (RGB のみ) */

mBool FilterSub_draw3x3(FilterDrawInfo *info,Filter3x3Info *dat)
{
	TileImage *imgsrc,*imgdst;
	int ix,iy,ixx,iyy,n;
	double d[3],divmul,add,dd;
	RGBAFix15 pix,pix2;
	mRect rc;
	TileImageSetPixelFunc setpix = g_tileimage_dinfo.funcDrawPixel; 

	imgsrc = info->imgsrc;
	imgdst = info->imgdst;
	rc = info->rc;

	divmul = dat->divmul;
	add = dat->add;

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		for(ix = rc.x1; ix <= rc.x2; ix++)
		{
			TileImage_getPixel(imgsrc, ix, iy, &pix);
			if(pix.a == 0) continue;

			//3x3

			d[0] = d[1] = d[2] = 0;

			for(iyy = -1, n = 0; iyy <= 1; iyy++)
			{
				for(ixx = -1; ixx <= 1; ixx++, n++)
				{
					if(info->clipping)
						TileImage_getPixel_clip(imgsrc, ix + ixx, iy + iyy, &pix2);
					else
						TileImage_getPixel(imgsrc, ix + ixx, iy + iyy, &pix2);

					dd = dat->mul[n];

					d[0] += pix2.r * dd;
					d[1] += pix2.g * dd;
					d[2] += pix2.b * dd;
				}
			}

			//RGB

			for(ixx = 0; ixx < 3; ixx++)
			{
				n = (int)(d[ixx] * divmul + add);
				if(n < 0) n = 0;
				else if(n > 0x8000) n = 0x8000;

				pix.c[ixx] = n;
			}

			//グレイスケール化

			if(dat->grayscale)
				pix.r = pix.g = pix.b = RGB15_TO_GRAY(pix.r, pix.g, pix.b);

			//セット

			(setpix)(imgdst, ix, iy, &pix);
		}

		FilterSub_progIncSubStep(info);
	}

	return TRUE;
}

/** ガウスぼかし処理
 *
 * @param setpix  結果の点描画関数。NULL で imgdst に通常描画。
 * @param setsrc  バッファにソース取得後、色を処理する関数。NULL でなし。 */

mBool FilterSub_gaussblur(FilterDrawInfo *info,
	TileImage *imgsrc,int radius,
	FilterDrawGaussBlurFunc setpix,FilterDrawGaussBlurSrcFunc setsrc)
{
	RGBAFix15 *buf1,*buf2,pix,*ps,*psY,*pd;
	int ix,iy,jx,jy,i,range,pixlen,xnum,ynum,pos,blurlen,srcpixnum;
	double d[4],*gaussbuf,dtmp,weight;
	mRect rc;
	TileImageSetPixelFunc setpix_def = g_tileimage_dinfo.funcDrawPixel;

	range = radius * 3;
	pixlen = range * 2 + 64;
	blurlen = range * 2 + 1;
	srcpixnum = pixlen * pixlen;

	//バッファ

	buf1 = (RGBAFix15 *)mMalloc(8 * pixlen * pixlen, FALSE);
	buf2 = (RGBAFix15 *)mMalloc(8 * 64 * pixlen, FALSE);

	if(!buf1 || !buf2)
	{
		mFree(buf1);
		return FALSE;
	}

	//ガウステーブル

	gaussbuf = (double *)mMalloc(sizeof(double) * blurlen, FALSE);
	if(!gaussbuf)
	{
		mFree(buf1);
		mFree(buf2);
		return FALSE;
	}

	dtmp = 1.0 / (2 * radius * radius);
	weight = 0;

	for(i = 0, pos = -range; i < blurlen; i++, pos++)
	{
		gaussbuf[i] = exp(-(pos * pos) * dtmp);
		weight += gaussbuf[i];
	}

	weight = 1.0 / weight;

	//--------------

	FilterSub_progBeginOneStep(info, 50, ((info->box.w + 63) / 64) * ((info->box.h + 63) / 64));

	rc.y1 = info->rc.y1 - range;
	rc.y2 = rc.y1 + pixlen - 1;

	for(iy = info->rc.y1; iy <= info->rc.y2; iy += 64)
	{
		rc.x1 = info->rc.x1 - range;
		rc.x2 = rc.x1 + pixlen - 1;
	
		for(ix = info->rc.x1; ix <= info->rc.x2; ix += 64, rc.x1 += 64, rc.x2 += 64)
		{
			//buf1 に 64x64 ソースセット (すべて透明なら処理なし)
		
			if(FilterSub_getPixelsToBuf(imgsrc, &rc, buf1, info->clipping))
			{
				FilterSub_progIncSubStep(info);
				continue;
			}

			//バッファ内の色を処理

			if(setsrc)
				(setsrc)(buf1, srcpixnum, info);

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

					for(i = 0; i < blurlen; i++)
						FilterSub_addAdvColor_weight(d, gaussbuf[i], ps + i - range);

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

					for(i = 0; i < blurlen; i++)
						FilterSub_addAdvColor_weight(d, gaussbuf[i], ps + i - range);

					FilterSub_getAdvColor(d, weight, &pix);

					if(setpix)
						(setpix)(ix + jx, iy + jy, &pix, info);
					else
						(setpix_def)(info->imgdst, ix + jx, iy + jy, &pix);
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
	mFree(gaussbuf);

	return TRUE;
}


//========================
// sub
//========================


/** 色テーブル作成 */

uint16_t *FilterSub_createColorTable(FilterDrawInfo *info)
{
	uint16_t *buf;

	buf = (uint16_t *)mMalloc((0x8000 + 1) * 2, FALSE);
	if(!buf) return NULL;

	info->ptmp[0] = buf;

	return buf;
}

/** img の rc の範囲の色 (RGBAFix15) をバッファに取得
 *
 * @param clipping キャンバス範囲外はクリッピング
 * @return TRUE ですべて透明 */

mBool FilterSub_getPixelsToBuf(TileImage *img,mRect *rcarea,RGBAFix15 *buf,mBool clipping)
{
	int ix,iy,f = 0;
	mRect rc = *rcarea;

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		for(ix = rc.x1; ix <= rc.x2; ix++, buf++)
		{
			if(clipping)
				TileImage_getPixel_clip(img, ix, iy, buf);
			else
				TileImage_getPixel(img, ix, iy, buf);

			if(buf->a) f = 1;
		}
	}

	return (f == 0);
}

/** 円形の形状 (1bit) バッファを作成
 *
 * @param pixnum  点の数が返る */

uint8_t *FilterSub_createCircleShape(int radius,int *pixnum)
{
	uint8_t *buf,*pd,f;
	int ix,iy,xx,yy,size,num = 0;

	size = radius * 2 + 1;

	buf = (uint8_t *)mMalloc((size * size + 7) >> 3, TRUE);
	if(!buf) return NULL;

	pd = buf;
	f = 0x80;

	for(iy = 0; iy < size; iy++)
	{
		yy = iy - radius;
		yy *= yy;

		for(ix = 0, xx = -radius; ix < size; ix++, xx++)
		{
			if(xx * xx + yy <= radius)
			{
				*pd |= f;
				num++;
			}

			f >>= 1;
			if(f == 0) f = 0x80, pd++;
		}
	}

	*pixnum = num;

	return buf;
}

/** ソース画像を出力先にコピー (プレビュー用)
 *
 * [!] imgdst は空状態である。 */

void FilterSub_copyImage_forPreview(FilterDrawInfo *info)
{
	if(info->in_dialog)
		TileImage_copyImage_rect(info->imgdst, info->imgsrc, &info->rc);
}


//========================
// 取得
//========================


/** キャンバスイメージサイズを取得 */

void FilterSub_getImageSize(int *w,int *h)
{
	*w = APP_DRAW->imgw;
	*h = APP_DRAW->imgh;
}

/** TileImage 点描画関数を取得 */

void FilterSub_getPixelFunc(TileImageSetPixelFunc *func)
{
	*func = g_tileimage_dinfo.funcDrawPixel;
}

/** ソース画像から色を取得 (クリッピング設定有効) */

void FilterSub_getPixelSrc_clip(FilterDrawInfo *info,int x,int y,RGBAFix15 *pix)
{
	if(info->clipping)
		TileImage_getPixel_clip(info->imgsrc, x, y, pix);
	else
		TileImage_getPixel(info->imgsrc, x, y, pix);
}

/** 指定タイプから描画する色を取得 */

void FilterSub_getDrawColor_fromType(FilterDrawInfo *info,int type,RGBAFix15 *pix)
{
	switch(type)
	{
		//描画色
		case 0:
			*pix = info->rgba15Draw;
			break;
		//背景色
		case 1:
			*pix = info->rgba15Bkgnd;
			break;
		//黒
		case 2:
			pix->v64 = 0;
			break;
		//白
		default:
			pix->r = pix->g = pix->b = 0x8000;
			break;
	}

	pix->a = 0x8000;
}

/** チェックされている先頭のレイヤの画像を取得 */

TileImage *FilterSub_getCheckedLayerImage()
{
	LayerItem *item;

	item = LayerList_setLink_checked(APP_DRAW->layerlist);

	return (item)? item->img: NULL;
}


//========================
// 平均色
//========================


/** 平均色を取得 */

void FilterSub_getAdvColor(double *d,double weight_mul,RGBAFix15 *pix)
{
	int a;
	double da;

	a = (int)(d[3] * weight_mul * 0x8000 + 0.5);

	if(a == 0)
		pix->v64 = 0;
	else
	{
		da = 1.0 / d[3];

		pix->r = (int)(d[0] * da + 0.5);
		pix->g = (int)(d[1] * da + 0.5);
		pix->b = (int)(d[2] * da + 0.5);
		pix->a = a;
	}
}

/** 平均色用に色を追加 */

void FilterSub_addAdvColor(double *d,RGBAFix15 *pix)
{
	if(pix->a == 0x8000)
	{
		d[0] += pix->r;
		d[1] += pix->g;
		d[2] += pix->b;
		d[3] += 1;
	}
	else if(pix->a)
	{
		double da = (double)pix->a / 0x8000;

		d[0] += pix->r * da;
		d[1] += pix->g * da;
		d[2] += pix->b * da;
		d[3] += da;
	}
}

/** 平均色用に色を追加 (重みあり) */

void FilterSub_addAdvColor_weight(double *d,double weight,RGBAFix15 *pix)
{
	if(pix->a == 0x8000)
	{
		d[0] += pix->r * weight;
		d[1] += pix->g * weight;
		d[2] += pix->b * weight;
		d[3] += weight;
	}
	else if(pix->a)
	{
		double a = (double)pix->a / 0x8000 * weight;

		d[0] += pix->r * a;
		d[1] += pix->g * a;
		d[2] += pix->b * a;
		d[3] += a;
	}
}

/** 平均色用に色を追加 (位置指定) */

void FilterSub_addAdvColor_at(TileImage *img,int x,int y,double *d,mBool clipping)
{
	RGBAFix15 pix;

	if(clipping)
		TileImage_getPixel_clip(img, x, y, &pix);
	else
		TileImage_getPixel(img, x, y, &pix);

	FilterSub_addAdvColor(d, &pix);
}

/** 平均色用に色を追加 (位置指定、重みあり) */

void FilterSub_addAdvColor_at_weight(TileImage *img,int x,int y,double *d,double weight,mBool clipping)
{
	RGBAFix15 pix;

	if(clipping)
		TileImage_getPixel_clip(img, x, y, &pix);
	else
		TileImage_getPixel(img, x, y, &pix);

	FilterSub_addAdvColor_weight(d, weight, &pix);
}


//========================
// 線形補間で色取得
//========================


/** 指定位置の色取得 */

static void _getlinercolor_getpixel(TileImage *img,int x,int y,RGBAFix15 *pix,uint8_t flags)
{
	//位置をループ

	if(flags & 2)
	{
		int max;

		max = g_tileimage_dinfo.imgw;

		while(x < 0) x += max;
		while(x >= max) x -= max;

		max = g_tileimage_dinfo.imgh;

		while(y < 0) y += max;
		while(y >= max) y -= max;
	}

	//色取得

	if(flags & 1)
		TileImage_getPixel_clip(img, x, y, pix);
	else
		TileImage_getPixel(img, x, y, pix);
}

/** 線形補間で指定位置の色取得
 *
 * @param flags 0bit:クリッピング 1bit:ループ */

void FilterSub_getLinerColor(TileImage *img,double x,double y,RGBAFix15 *pixdst,uint8_t flags)
{
	double d1,d2,fx1,fx2,fy1,fy2;
	int ix,iy,i,n;
	RGBAFix15 pix[4];

	d2 = modf(x, &d1); if(d2 < 0) d2 += 1;
	ix = floor(d1);
	fx1 = d2;
	fx2 = 1 - d2;

	d2 = modf(y, &d1); if(d2 < 0) d2 += 1;
	iy = floor(d1);
	fy1 = d2;
	fy2 = 1 - d2;

	//周囲4点の色取得

	_getlinercolor_getpixel(img, ix, iy, pix, flags);
	_getlinercolor_getpixel(img, ix + 1, iy, pix + 1, flags);
	_getlinercolor_getpixel(img, ix, iy + 1, pix + 2, flags);
	_getlinercolor_getpixel(img, ix + 1, iy + 1, pix + 3, flags);

	//全て透明なら透明に

	if(pix[0].a + pix[1].a + pix[2].a + pix[3].a == 0)
	{
		pixdst->v64 = 0;
		return;
	}

	//アルファ値

	d1 = fy2 * (fx2 * pix[0].a + fx1 * pix[1].a) +
	     fy1 * (fx2 * pix[2].a + fx1 * pix[3].a);

	pixdst->a = (int)(d1 + 0.5);

	//RGB

	if(pixdst->a == 0)
		pixdst->v64 = 0;
	else
	{
		for(i = 0; i < 3; i++)
		{
			d2 = fy2 * (fx2 * pix[0].c[i] * pix[0].a + fx1 * pix[1].c[i] * pix[1].a) +
			     fy1 * (fx2 * pix[2].c[i] * pix[2].a + fx1 * pix[3].c[i] * pix[3].a);

			n = (int)(d2 / d1 + 0.5);
			if(n > 0x8000) n = 0x8000;

			pixdst->c[i] = n;
		}
	}
}

/** 線形補間色取得 (イメージ範囲外時の背景指定あり)
 *
 * @param type 0:透明 1:端の色 2:元のまま */

void FilterSub_getLinerColor_bkgnd(TileImage *img,double dx,double dy,
	int x,int y,RGBAFix15 *pix,int type)
{
	int nx,ny;

	nx = floor(dx);
	ny = floor(dy);

	if(type == 1
		|| (nx >= 0 && nx < g_tileimage_dinfo.imgw && ny >= 0 && ny < g_tileimage_dinfo.imgh))
		//背景は端の色、または位置が範囲内の場合、クリッピングして取得
		FilterSub_getLinerColor(img, dx, dy, pix, 1);
	else if(type == 0)
		//範囲外:透明
		pix->v64 = 0;
	else if(type == 2)
		//範囲外:元のまま
		TileImage_getPixel(img, x, y, pix);
}


//===========================
// 点描画
//(ランダム点描画、縁取り)
//===========================


/** 描画色取得 */

static void _drawpoint_get_color(FilterDrawPointInfo *dat,RGBAFix15 *pix)
{
	int i,c[3];

	switch(dat->coltype)
	{
		//描画色
		case 0:
			*pix = dat->info->rgba15Draw;
			break;
		//ランダム (グレイスケール)
		case 1:
			pix->r = pix->g = pix->b = mRandXorShift_getIntRange(0, 0x8000);
			break;
		//ランダム (色相)
		case 2:
			mHSVtoRGB(mRandXorShift_getIntRange(0, 359) / 360.0, 1, 1, c);

			pix->r = RGBCONV_8_TO_FIX15(c[0]);
			pix->g = RGBCONV_8_TO_FIX15(c[1]);
			pix->b = RGBCONV_8_TO_FIX15(c[2]);
			break;
		//ランダム (RGB)
		default:
			for(i = 0; i < 3; i++)
				pix->c[i] = mRandXorShift_getIntRange(0, 0x8000);
			break;
	}

	pix->a = dat->opacity;
}

/** 1px 点を打つ */

static void _drawpoint_setpixel(int x,int y,RGBAFix15 *pix,FilterDrawPointInfo *dat)
{
	mRect rc;

	//縁取り時のマスク

	if(dat->masktype == 1)
	{
		//外側:不透明部分には描画しない
		if(TileImage_isPixelOpaque(dat->imgsrc, x, y)) return;
	}
	else if(dat->masktype == 2)
	{
		//内側:透明部分には描画しない
		if(TileImage_isPixelTransparent(dat->imgsrc, x, y)) return;
	}

	//範囲外は描画しない

	rc = dat->info->rc;

	if(rc.x1 <= x && x <= rc.x2 && rc.y1 <= y && y <= rc.y2)
		(g_tileimage_dinfo.funcDrawPixel)(dat->imgdst, x, y, pix);
}

/** 点を打つ (ドット円) */

static void _drawpoint_setpixel_dot(int x,int y,FilterDrawPointInfo *dat)
{
	int ix,iy,yy,r,rr;
	RGBAFix15 pix;

	_drawpoint_get_color(dat, &pix);

	r = dat->radius;
	rr = r * r;

	for(iy = -r; iy <= r; iy++)
	{
		yy = iy * iy;

		for(ix = -r; ix <= r; ix++)
		{
			if(ix * ix + yy <= rr)
				_drawpoint_setpixel(x + ix, y + iy, &pix, dat);
		}
	}
}

/** 点を打つ (アンチエイリアス円) */

static void _drawpoint_setpixel_aa(int x,int y,FilterDrawPointInfo *dat)
{
	int ix,iy,i,j,xtbl[4],ytbl[4],r,rr,srca,cnt;
	RGBAFix15 pix;

	_drawpoint_get_color(dat, &pix);
	srca = pix.a;

	r = dat->radius;
	rr = r << 2;
	rr *= rr;

	for(iy = -r; iy <= r; iy++)
	{
		//Y テーブル

		for(i = 0, j = (iy << 2) - 2; i < 4; i++, j++)
			ytbl[i] = j * j;

		//
	
		for(ix = -r; ix <= r; ix++)
		{
			//X テーブル

			for(i = 0, j = (ix << 2) - 2; i < 4; i++, j++)
				xtbl[i] = j * j;

			//4x4

			cnt = 0;

			for(i = 0; i < 4; i++)
			{
				for(j = 0; j < 4; j++)
				{
					if(xtbl[j] + ytbl[i] < rr) cnt++;
				}
			}

			//セット

			if(cnt)
			{
				pix.a = srca * cnt >> 4;
				_drawpoint_setpixel(x + ix, y + iy, &pix, dat);
			}
		}
	}
}

/** 点を打つ (柔らかい円) */

static void _drawpoint_setpixel_soft(int x,int y,FilterDrawPointInfo *dat)
{
	int ix,iy,i,j,xtbl[4],ytbl[4],r,srca;
	double da,dr,rr;
	RGBAFix15 pix;

	_drawpoint_get_color(dat, &pix);
	srca = pix.a;

	r = dat->radius;
	rr = 1.0 / ((r << 2) * (r << 2));

	for(iy = -r; iy <= r; iy++)
	{
		//Y テーブル

		for(i = 0, j = (iy << 2) - 2; i < 4; i++, j++)
			ytbl[i] = j * j;

		//
	
		for(ix = -r; ix <= r; ix++)
		{
			//X テーブル

			for(i = 0, j = (ix << 2) - 2; i < 4; i++, j++)
				xtbl[i] = j * j;

			//4x4

			da = 0;

			for(i = 0; i < 4; i++)
			{
				for(j = 0; j < 4; j++)
				{
					dr = (xtbl[j] + ytbl[i]) * rr;

					if(dr < 1.0) da += 1 - dr;
				}
			}

			//セット

			if(da != 0)
			{
				pix.a = (int)(srca * (da * (1.0 / 16)) + 0.5);
				_drawpoint_setpixel(x + ix, y + iy, &pix, dat);
			}
		}
	}
}

/** 点描画の関数を取得 */

void FilterSub_getDrawPointFunc(int type,FilterDrawPointFunc *dst)
{
	FilterDrawPointFunc func[3] = {
		_drawpoint_setpixel_dot, _drawpoint_setpixel_aa, _drawpoint_setpixel_soft
	};

	*dst = func[type];
}

/** 点描画の初期化 */

void FilterSub_initDrawPoint(FilterDrawInfo *info,mBool compare_alpha)
{
	//プレビュー時は色計算を有効に

	if(info->in_dialog)
		g_tileimage_dinfo.funcDrawPixel = TileImage_setPixel_new_colfunc;

	//色計算

	g_tileimage_dinfo.funcColor = (compare_alpha)? TileImage_colfunc_compareA: TileImage_colfunc_normal;
}


//========================
// 進捗
//========================


void FilterSub_progSetMax(FilterDrawInfo *info,int max)
{
	if(info->prog)
		mPopupProgressThreadSetMax(info->prog, max);
}

void FilterSub_progInc(FilterDrawInfo *info)
{
	if(info->prog)
		mPopupProgressThreadIncPos(info->prog);
}

void FilterSub_progIncSubStep(FilterDrawInfo *info)
{
	if(info->prog)
		mPopupProgressThreadIncSubStep(info->prog);
}

void FilterSub_progBeginOneStep(FilterDrawInfo *info,int step,int max)
{
	if(info->prog)
		mPopupProgressThreadBeginSubStep_onestep(info->prog, step, max);
}

void FilterSub_progBeginSubStep(FilterDrawInfo *info,int step,int max)
{
	if(info->prog)
		mPopupProgressThreadBeginSubStep(info->prog, step, max);
}


//========================
// 色変換
//========================


/** RGB -> YCrCb 変換 */

void FilterSub_RGBtoYCrCb(int *val)
{
	int c[3],i;

	/* Y  = 0.299 * R + 0.587 * G + 0.114 * B
	 * Cr = 0.5 * R - 0.418688 * G - 0.081312 * B
	 * Cb = -0.168736 * R - 0.331264 * G + 0.5 * B */

	c[0] = (val[0] * 4899 + val[1] * 9617 + val[2] * 1868) >> 14;
	c[1] = ((val[0] * 8192 - val[1] * 6860 - val[2] * 1332) >> 14) + 0x4000;
	c[2] = ((val[0] * -2765 - val[1] * 5427 + val[2] * 8192) >> 14) + 0x4000;

	for(i = 0; i < 3; i++)
	{
		if(c[i] < 0) c[i] = 0;
		else if(c[i] > 0x8000) c[i] = 0x8000;

		val[i] = c[i];
	}
}

/** YCrCb -> RGB 変換 */

void FilterSub_YCrCbtoRGB(int *val)
{
	int c[3],i,cr,cb;

	/* R = Y + 1.402 * Cr
	 * G = Y - 0.344136 * Cb - 0.714136 * Cr
	 * B = Y + 1.772 * Cb */

	cr = val[1] - 0x4000;
	cb = val[2] - 0x4000;

	c[0] = val[0] + (cr * 22970 >> 14);
	c[1] = val[0] - ((cb * 5638 + cr * 11700) >> 14);
	c[2] = val[0] + (cb * 29032 >> 14);

	for(i = 0; i < 3; i++)
	{
		if(c[i] < 0) c[i] = 0;
		else if(c[i] > 0x8000) c[i] = 0x8000;

		val[i] = c[i];
	}
}

/** RGB -> HSV (360:0x8000:0x8000) */

void FilterSub_RGBtoHSV(int *val)
{
	int min,max,r,g,b,d,rt,gt,bt,h;

	r = val[0];
	g = val[1];
	b = val[2];

	if(r >= g) max = r; else max = g; if(b > max) max = b;
	if(r <= g) min = r; else min = g; if(b < min) min = b;

	val[2] = max;

	d = max - min;

	if(max == 0)
		val[1] = 0;
	else
		val[1] = (d << 15) / max;

	if(val[1] == 0)
	{
		val[0] = 0;
		return;
	}

	rt = max - r * 60 / d;
	gt = max - g * 60 / d;
	bt = max - b * 60 / d;

	if(r == max)
		h = bt - gt;
	else if(g == max)
		h = 120 + rt - bt;
	else
		h = 240 + gt - rt;

	if(h < 0) h += 360;
	else if(h >= 360) h -= 360;

	val[0] = h;
}

/** HSV -> RGB */

void FilterSub_HSVtoRGB(int *val)
{
	int r,g,b,ht,d,t1,t2,t3,v;

	v = val[2];

	if(val[1] == 0)
		r = g = b = v;
	else
	{
		ht = val[0] * 6;
		d  = ht % 360;

		t1 = v * (0x8000 - val[1]) >> 15;
		t2 = v * (0x8000 - val[1] * d / 360) >> 15;
		t3 = v * (0x8000 - val[1] * (360 - d) / 360) >> 15;

		switch(ht / 360)
		{
			case 0: r = v, g = t3, b = t1; break;
			case 1: r = t2, g = v, b = t1; break;
			case 2: r = t1, g = v, b = t3; break;
			case 3: r = t1, g = t2, b = v; break;
			case 4: r = t3, g = t1, b = v; break;
			default: r = v, g = t1, b = t2;
		}
	}

	val[0] = r;
	val[1] = g;
	val[2] = b;
}
