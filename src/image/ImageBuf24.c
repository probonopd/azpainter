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
 * ImageBuf24
 *
 * 8bit R-G-B イメージ
 *****************************************/
/*
 * - [R-G-B] のバイト順で並んでいる。
 */

#include "mDef.h"
#include "mLoadImage.h"
#include "mPixbuf.h"

#include "ImageBuf24.h"



/** 解放 */

void ImageBuf24_free(ImageBuf24 *p)
{
	if(p)
	{
		mFree(p->buf);
		mFree(p);
	}
}

/** 作成 */

ImageBuf24 *ImageBuf24_new(int w,int h)
{
	ImageBuf24 *p;

	p = (ImageBuf24 *)mMalloc(sizeof(ImageBuf24), TRUE);
	if(!p) return NULL;

	p->w = w;
	p->h = h;
	p->pitch = w * 3;

	//バッファ確保

	p->buf = (uint8_t *)mMalloc(w * h * 3, FALSE);
	if(!p->buf)
	{
		mFree(p);
		return NULL;
	}

	return p;
}

/** 指定位置のポインタを取得
 *
 * @return NULL で範囲外 */

uint8_t *ImageBuf24_getPixelBuf(ImageBuf24 *p,int x,int y)
{
	if(x < 0 || y < 0 || x >= p->w || y >= p->h)
		return NULL;
	else
		return p->buf + y * p->pitch + x * 3;
}


//==============================
// 画像読み込み
//==============================


typedef struct
{
	mLoadImage base;
	ImageBuf24 *img;
	uint8_t *dstbuf;
}_loadimageinfo;


/** 情報取得 */

static int _loadfile_getinfo(mLoadImage *param,mLoadImageInfo *info)
{
	_loadimageinfo *p = (_loadimageinfo *)param;

	//作成

	p->img = ImageBuf24_new(info->width, info->height);
	if(!p->img) return MLOADIMAGE_ERR_ALLOC;

	p->dstbuf = p->img->buf;
	if(info->bottomup) p->dstbuf += (info->height - 1) * p->img->pitch;

	return MLOADIMAGE_ERR_OK;
}

/** Y1行取得 */

static int _loadfile_getrow(mLoadImage *param,uint8_t *buf,int pitch)
{
	_loadimageinfo *p = (_loadimageinfo *)param;
	int i,a;
	uint8_t *pd;

	pd = p->dstbuf;

	for(i = param->info.width; i > 0; i--, buf += 4, pd += 3)
	{
		a = buf[3];

		if(a == 255)
			pd[0] = buf[0], pd[1] = buf[1], pd[2] = buf[2];
		else if(a == 0)
			pd[0] = pd[1] = pd[2] = 255;
		else
		{
			pd[0] = (buf[0] - 255) * a / 255 + 255;
			pd[1] = (buf[1] - 255) * a / 255 + 255;
			pd[2] = (buf[2] - 255) * a / 255 + 255;
		}
	}

	p->dstbuf += (param->info.bottomup)? -(p->img->pitch): p->img->pitch;
	
	return MLOADIMAGE_ERR_OK;
}

/** 画像ファイルを読み込み (BMP/PNG/GIF/JPEG)
 *
 * アルファ値または透過色がある場合は、背景が白として合成される */

ImageBuf24 *ImageBuf24_loadFile(const char *filename)
{
	_loadimageinfo *p;
	mLoadImageFunc func;
	ImageBuf24 *img = NULL;

	p = (_loadimageinfo *)mLoadImage_create(sizeof(_loadimageinfo));
	if(!p) return NULL;

	p->base.format = MLOADIMAGE_FORMAT_RGBA;
	p->base.src.type = MLOADIMAGE_SOURCE_TYPE_PATH;
	p->base.src.filename = filename;
	p->base.getinfo = _loadfile_getinfo;
	p->base.getrow = _loadfile_getrow;

	//実行

	if(mLoadImage_checkFormat(&p->base.src, &func, MLOADIMAGE_CHECKFORMAT_F_ALL))
	{
		if((func)(M_LOADIMAGE(p)))
			img = p->img;
		else
			ImageBuf24_free(p->img);
	}

	mLoadImage_free(M_LOADIMAGE(p));

	return img;
}


//============================================
// キャンバス描画 (イメージビューア)
//============================================


#define FIXF_BIT  28
#define FIXF_VAL  ((int64_t)1 << FIXF_BIT)


/** キャンバス描画 (ニアレストネイバー) */

void ImageBuf24_drawCanvas_nearest(ImageBuf24 *src,mPixbuf *dst,
	mBox *boxdst,ImageBuf24CanvasInfo *info)
{
	int sw,sh,dbpp,pitchd,ix,iy,n;
	uint8_t *pd,*psY,*ps;
	mPixCol pixbkgnd;
	mBox box;
	int64_t fincx,fincy,fxleft,fx,fy;
	double scale,scalex;

	//クリッピング

	if(!mPixbufGetClipBox_box(dst, &box, boxdst)) return;

	//

	sw = src->w, sh = src->h;

	pd = mPixbufGetBufPtFast(dst, box.x, box.y);
	dbpp = dst->bpp;
	pitchd = dst->pitch_dir - box.w * dbpp;
	pixbkgnd = mRGBtoPix(info->bkgndcol);

	//

	scale = scalex = info->scalediv;

	fincx = fincy = (int64_t)(scale * FIXF_VAL + 0.5);

	if(info->mirror)
		fincx = -fincx, scalex = -scalex;

	fxleft = (int64_t)((info->scrollx * scalex + info->originx) * FIXF_VAL)
		+ box.x * fincx;
	fy = (int64_t)((info->scrolly * scale + info->originy) * FIXF_VAL)
		+ box.y * fincy;

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

		psY = src->buf + n * src->pitch;

		for(ix = box.w, fx = fxleft; ix > 0; ix--, fx += fincx, pd += dbpp)
		{
			n = fx >> FIXF_BIT;

			if(fx < 0 || n >= sw)
				//範囲外
				(dst->setbuf)(pd, pixbkgnd);
			else
			{
				ps = psY + n * 3;
			
				(dst->setbuf)(pd, mRGBtoPix2(ps[0], ps[1], ps[2]));
			}
		}

		pd += pitchd;
	}
}

/** キャンバス描画 (4x4 オーバーサンプリング) */

void ImageBuf24_drawCanvas_oversamp(ImageBuf24 *src,mPixbuf *dst,
	mBox *boxdst,ImageBuf24CanvasInfo *info)
{
	int sw,sh,dbpp,pitchd,ix,iy,jx,jy,n,tblX[4],r,g,b;
	uint8_t *pd,*tbl_psY[4],*ps;
	mPixCol pixbkgnd;
	mBox box;
	int64_t fincx,fincy,fincx2,fincy2,fxleft,fx,fy,f;
	double scale,scalex;

	//クリッピング

	if(!mPixbufGetClipBox_box(dst, &box, boxdst)) return;

	//

	sw = src->w, sh = src->h;

	pd = mPixbufGetBufPtFast(dst, box.x, box.y);
	dbpp = dst->bpp;
	pitchd = dst->pitch_dir - box.w * dbpp;
	pixbkgnd = mRGBtoPix(info->bkgndcol);

	//

	scale = scalex = info->scalediv;

	fincx = fincy = (int64_t)(scale * FIXF_VAL + 0.5);

	if(info->mirror)
		fincx = -fincx, scalex = -scalex;

	fincx2 = fincx >> 2;
	fincy2 = fincy >> 2;

	fxleft = (int64_t)((info->scrollx * scalex + info->originx) * FIXF_VAL)
		+ box.x * fincx;
	fy = (int64_t)((info->scrolly * scale + info->originy) * FIXF_VAL)
		+ box.y * fincy;

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

			tbl_psY[jy] = src->buf + n * src->pitch;
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
					if(n >= sw) n = sw - 1;

					tblX[jx] = n * 3;
				}

				//オーバーサンプリング

				r = g = b = 0;

				for(jy = 0; jy < 4; jy++)
				{
					for(jx = 0; jx < 4; jx++)
					{
						ps = tbl_psY[jy] + tblX[jx];

						r += ps[0];
						g += ps[1];
						b += ps[2];
					}
				}
			
				(dst->setbuf)(pd, mRGBtoPix2(r >> 4, g >> 4, b >> 4));
			}
		}

		pd += pitchd;
	}
}
