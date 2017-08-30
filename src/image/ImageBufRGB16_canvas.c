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
 * ImageBufRGB16
 *
 * mPixbuf へのキャンバス描画
 *****************************************/

#include "mDef.h"
#include "mPixbuf.h"

#include "ImageBufRGB16.h"
#include "defCanvasInfo.h"
#include "ColorValue.h"


//---------------

#define FIXF_BIT  28
#define FIXF_VAL  ((int64_t)1 << FIXF_BIT)

#define RGBFIX_TO_PIX(p)  mRGBtoPix2((p)->r * 255 >> 15, (p)->g * 255 >> 15, (p)->b * 255 >> 15)

//---------------


//=========================================
// メインウィンドウ キャンバス用
//=========================================


/** メインウィンドウキャンバス描画 (ニアレストネイバー) */

void ImageBufRGB16_drawMainCanvas_nearest(ImageBufRGB16 *src,mPixbuf *dst,CanvasDrawInfo *info)
{
	int sw,sh,dbpp,pitchd,ix,iy,n;
	uint8_t *pd;
	RGBFix15 *psY,*ps;
	mPixCol pixbkgnd;
	mBox box;
	int64_t fincx,fincy,fxleft,fx,fy;
	double scalex;

	//クリッピング

	if(!mPixbufGetClipBox_box(dst, &box, &info->boxdst)) return;

	//

	sw = src->width, sh = src->height;

	pd = mPixbufGetBufPtFast(dst, box.x, box.y);
	dbpp = dst->bpp;
	pitchd = dst->pitch_dir - box.w * dbpp;
	pixbkgnd = mRGBtoPix(info->bkgndcol);

	scalex = info->param->scalediv;

	fincx = fincy = (int64_t)(scalex * FIXF_VAL + 0.5);

	if(info->mirror)
		scalex = -scalex, fincx = -fincx;

	fxleft = (int64_t)((info->scrollx * scalex + info->originx) * FIXF_VAL) + box.x * fincx;
	fy = (int64_t)((info->scrolly * info->param->scalediv + info->originy) * FIXF_VAL) + box.y * fincy;

	//

	for(iy = box.h; iy > 0; iy--, fy += fincy)
	{
		n = fy >> FIXF_BIT;

		//Yが範囲外

		if(fy < 0 || n >= sh)
		{
			mPixbufRawLineH(dst, pd, box.w, pixbkgnd);

			pd += dst->pitch_dir;
			continue;
		}

		//X

		psY = src->buf + n * src->width;

		for(ix = box.w, fx = fxleft; ix > 0; ix--, fx += fincx, pd += dbpp)
		{
			n = fx >> FIXF_BIT;

			if(fx < 0 || n >= sw)
				//範囲外
				(dst->setbuf)(pd, pixbkgnd);
			else
			{
				ps = psY + n;
			
				(dst->setbuf)(pd, RGBFIX_TO_PIX(ps));
			}
		}

		pd += pitchd;
	}
}

/** メインウィンドウキャンバス描画 (4x4 オーバーサンプリング) */

void ImageBufRGB16_drawMainCanvas_oversamp(ImageBufRGB16 *src,mPixbuf *dst,CanvasDrawInfo *info)
{
	int sw,sh,dbpp,pitchd,ix,iy,jx,jy,n,tblX[4],r,g,b;
	uint8_t *pd;
	RGBFix15 *tbl_psY[4],*ps;
	mPixCol pixbkgnd;
	mBox box;
	int64_t fincx,fincy,fincx2,fincy2,fxleft,fx,fy,f;
	double scalex;

	//クリッピング

	if(!mPixbufGetClipBox_box(dst, &box, &info->boxdst)) return;

	//

	sw = src->width, sh = src->height;

	pd = mPixbufGetBufPtFast(dst, box.x, box.y);
	dbpp = dst->bpp;
	pitchd = dst->pitch_dir - box.w * dbpp;
	pixbkgnd = mRGBtoPix(info->bkgndcol);

	//

	scalex = info->param->scalediv;

	fincx = fincy = (int64_t)(scalex * FIXF_VAL + 0.5);

	if(info->mirror)
		scalex = -scalex, fincx = -fincx;

	fincx2 = fincx >> 2;
	fincy2 = fincy >> 2;

	fxleft = (int64_t)((info->scrollx * scalex + info->originx) * FIXF_VAL) + box.x * fincx;
	fy = (int64_t)((info->scrolly * info->param->scalediv + info->originy) * FIXF_VAL) + box.y * fincy;

	//

	for(iy = box.h; iy > 0; iy--, fy += fincy)
	{
		n = fy >> FIXF_BIT;

		//Yが範囲外

		if(n < 0 || n >= sh)
		{
			mPixbufRawLineH(dst, pd, box.w, pixbkgnd);
			pd += dst->pitch_dir;
			continue;
		}

		//Yテーブル

		for(jy = 0, f = fy; jy < 4; jy++, f += fincy2)
		{
			n = f >> FIXF_BIT;
			if(n >= sh) n = sh - 1;

			tbl_psY[jy] = src->buf + n * src->width;
		}

		//---- X

		for(ix = box.w, fx = fxleft; ix > 0; ix--, fx += fincx, pd += dbpp)
		{
			n = fx >> FIXF_BIT;

			if(n < 0 || n >= sw)
				//範囲外
				(dst->setbuf)(pd, pixbkgnd);
			else
			{
				//X テーブル

				for(jx = 0, f = fx; jx < 4; jx++, f += fincx2)
				{
					n = f >> FIXF_BIT;
					if(n < 0) n = 0; else if(n >= sw) n = sw - 1;

					tblX[jx] = n;
				}

				//オーバーサンプリング

				r = g = b = 0;

				for(jy = 0; jy < 4; jy++)
				{
					for(jx = 0; jx < 4; jx++)
					{
						ps = tbl_psY[jy] + tblX[jx];

						r += ps->r;
						g += ps->g;
						b += ps->b;
					}
				}

				r = (r >> 4) * 255 >> 15;
				g = (g >> 4) * 255 >> 15;
				b = (b >> 4) * 255 >> 15;
			
				(dst->setbuf)(pd, mRGBtoPix2(r, g, b));
			}
		}

		pd += pitchd;
	}
}

/** メインウィンドウキャンバス描画 (回転あり、補間なし) */

void ImageBufRGB16_drawMainCanvas_rotate_normal(ImageBufRGB16 *src,mPixbuf *dst,CanvasDrawInfo *info)
{
	mBox box;
	mPixCol pixbkgnd;
	int sw,sh,dbpp,pitchd,ix,iy,sx,sy;
	uint8_t *pd;
	RGBFix15 *ps;
	int64_t fx,fy,fleftx,flefty,fincxx,fincxy,fincyx,fincyy;
	double scalex;

	//クリッピング

	if(!mPixbufGetClipBox_box(dst, &box, &info->boxdst)) return;

	//

	sw = src->width, sh = src->height;

	pd = mPixbufGetBufPtFast(dst, box.x, box.y);
	dbpp = dst->bpp;
	pitchd = dst->pitch_dir - box.w * dbpp;
	pixbkgnd = mRGBtoPix(info->bkgndcol);

	//xx,yy:cos, xy:sin, yx:-sin

	fincxx = (int64_t)(info->param->cosrev * info->param->scalediv * FIXF_VAL + 0.5);
	fincxy = (int64_t)(info->param->sinrev * info->param->scalediv * FIXF_VAL + 0.5);
	fincyx = -fincxy;
	fincyy = fincxx;

	if(info->mirror)
		fincxx = -fincxx, fincyx = -fincyx;

	//

	ix = info->scrollx;
	iy = info->scrolly;

	scalex = info->param->scalediv;
	if(info->mirror) scalex = -scalex;

	fleftx = (int64_t)(((ix * info->param->cosrev - iy * info->param->sinrev)
		* scalex + info->originx) * FIXF_VAL)
		+ box.x * fincxx + box.y * fincyx;

	flefty = (int64_t)(((ix * info->param->sinrev + iy * info->param->cosrev)
		* info->param->scalediv + info->originy) * FIXF_VAL)
		+ box.x * fincxy + box.y * fincyy;

	//

	for(iy = box.h; iy; iy--)
	{
		fx = fleftx, fy = flefty;

		for(ix = box.w; ix; ix--, pd += dbpp)
		{
			sx = fx >> FIXF_BIT;
			sy = fy >> FIXF_BIT;

			if(sx < 0 || sy < 0 || sx >= sw || sy >= sh)
				(dst->setbuf)(pd, pixbkgnd);
			else
			{
				ps = src->buf + sy * src->width + sx;

				(dst->setbuf)(pd, RGBFIX_TO_PIX(ps));
			}

			fx += fincxx;
			fy += fincxy;
		}

		fleftx += fincyx;
		flefty += fincyy;
		pd += pitchd;
	}
}

/** メインウィンドウキャンバス描画 (回転あり、補間あり) */

void ImageBufRGB16_drawMainCanvas_rotate_oversamp(ImageBufRGB16 *src,mPixbuf *dst,CanvasDrawInfo *info)
{
	mBox box;
	mPixCol pixbkgnd;
	int sw,sh,dbpp,pitchd,ix,iy,jx,jy,sx,sy,r,g,b;
	uint8_t *pd;
	RGBFix15 *ps;
	int64_t fx,fy,fleftx,flefty,fincxx,fincxy,fincyx,fincyy,
		fjx,fjy,fjx2,fjy2,fincxx2,fincxy2,fincyx2,fincyy2;
	double scalex;

	//クリッピング

	if(!mPixbufGetClipBox_box(dst, &box, &info->boxdst)) return;

	//

	sw = src->width, sh = src->height;

	pd = mPixbufGetBufPtFast(dst, box.x, box.y);
	dbpp = dst->bpp;
	pitchd = dst->pitch_dir - box.w * dbpp;
	pixbkgnd = mRGBtoPix(info->bkgndcol);

	//xx,yy:cos, xy:sin, yx:-sin

	fincxx = (int64_t)(info->param->cosrev * info->param->scalediv * FIXF_VAL + 0.5);
	fincxy = (int64_t)(info->param->sinrev * info->param->scalediv * FIXF_VAL + 0.5);
	fincyx = -fincxy;
	fincyy = fincxx;

	if(info->mirror)
		fincxx = -fincxx, fincyx = -fincyx;

	fincxx2 = fincxx >> 2, fincxy2 = fincxy >> 2;
	fincyx2 = fincyx >> 2, fincyy2 = fincyy >> 2;

	//

	ix = info->scrollx;
	iy = info->scrolly;

	scalex = info->param->scalediv;
	if(info->mirror) scalex = -scalex;

	fleftx = (int64_t)(((ix * info->param->cosrev - iy * info->param->sinrev)
		* scalex + info->originx) * FIXF_VAL)
		+ box.x * fincxx + box.y * fincyx;

	flefty = (int64_t)(((ix * info->param->sinrev + iy * info->param->cosrev)
		* info->param->scalediv + info->originy) * FIXF_VAL)
		+ box.x * fincxy + box.y * fincyy;

	//

	for(iy = box.h; iy; iy--)
	{
		fx = fleftx, fy = flefty;

		for(ix = box.w; ix; ix--, pd += dbpp)
		{
			sx = fx >> FIXF_BIT;
			sy = fy >> FIXF_BIT;

			if(sx < 0 || sy < 0 || sx >= sw || sy >= sh)
				//範囲外
				(dst->setbuf)(pd, pixbkgnd);
			else
			{
				r = g = b = 0;
				fjx2 = fx, fjy2 = fy;

				for(jy = 4; jy; jy--, fjx2 += fincyx2, fjy2 += fincyy2)
				{
					fjx = fjx2, fjy = fjy2;

					for(jx = 4; jx; jx--, fjx += fincxx2, fjy += fincxy2)
					{
						sx = fjx >> FIXF_BIT;
						sy = fjy >> FIXF_BIT;

						if(sx < 0) sx = 0; else if(sx >= sw) sx = sw - 1;
						if(sy < 0) sy = 0; else if(sy >= sh) sy = sh - 1;

						ps = src->buf + sy * src->width + sx;

						r += ps->r;
						g += ps->g;
						b += ps->b;
					}
				}

				r = (r >> 4) * 255 >> 15;
				g = (g >> 4) * 255 >> 15;
				b = (b >> 4) * 255 >> 15;
			
				(dst->setbuf)(pd, mRGBtoPix2(r, g, b));
			}

			fx += fincxx;
			fy += fincxy;
		}

		fleftx += fincyx;
		flefty += fincyy;
		pd += pitchd;
	}
}
