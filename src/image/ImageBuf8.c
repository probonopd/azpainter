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
 * ImageBuf8
 *
 * 8bit イメージ
 *****************************************/
/*
 * - 確保バッファサイズは 4byte 単位。
 * - テクスチャ画像で使用される。
 */


#include <string.h>

#include "mDef.h"
#include "mPixbuf.h"

#include "ImageBuf8.h"
#include "ImageBuf24.h"


#define _TOGRAY(r,g,b)  ((r * 77 + g * 150 + b * 29) >> 8)



/** 解放 */

void ImageBuf8_free(ImageBuf8 *p)
{
	if(p)
	{
		mFree(p->buf);
		mFree(p);
	}
}

/** 作成 */

ImageBuf8 *ImageBuf8_new(int w,int h)
{
	ImageBuf8 *p;
	uint32_t size;

	p = (ImageBuf8 *)mMalloc(sizeof(ImageBuf8), TRUE);
	if(!p) return NULL;

	size = (w * h + 3) & (~3);

	p->buf = (uint8_t *)mMalloc(size, FALSE);
	if(!p->buf)
	{
		mFree(p);
		return NULL;
	}

	p->w = w;
	p->h = h;
	p->bufsize = size;

	return p;
}

/** クリア */

void ImageBuf8_clear(ImageBuf8 *p)
{
	memset(p->buf, 0, p->bufsize);
}

/** テクスチャ画像として色を取得 */

uint8_t ImageBuf8_getPixel_forTexture(ImageBuf8 *p,int x,int y)
{
	if(x < 0)
		x += (-x / p->w + 1) * p->w;

	if(y < 0)
		y += (-y / p->h + 1) * p->h;

	return *(p->buf + (y % p->h) * p->w + (x % p->w));
}

/** ImageBuf24 からテクスチャ画像として作成 */

ImageBuf8 *ImageBuf8_createFromImageBuf24(ImageBuf24 *src)
{
	ImageBuf8 *img;
	uint8_t *pd,*ps;
	uint32_t i;

	img = ImageBuf8_new(src->w, src->h);
	if(!img) return NULL;

	//変換

	pd = img->buf;
	ps = src->buf;

	for(i = src->w * src->h; i; i--, ps += 3)
		*(pd++) = 255 - _TOGRAY(ps[0], ps[1], ps[2]);

	return img;
}

/** ブラシ用画像として作成
 *
 * src が正方形でない場合は、正方形に調整される。 */

ImageBuf8 *ImageBuf8_createFromImageBuf24_forBrush(ImageBuf24 *src)
{
	ImageBuf8 *img;
	uint8_t *ps,*pd;
	int sw,sh,size,pitchd,ix,iy;

	//作成

	sw = src->w, sh = src->h;

	size = (sw < sh)? sh: sw;

	img = ImageBuf8_new(size, size);
	if(!img) return NULL;

	ImageBuf8_clear(img);

	//変換

	pd = img->buf + ((size - sh) >> 1) * size + ((size - sw) >> 1);
	ps = src->buf;
	pitchd = size - sw;

	for(iy = sh; iy; iy--, pd += pitchd)
	{
		for(ix = sw; ix; ix--, ps += 3)
			*(pd++) = 255 - *ps;
	}

	return img;
}

/** mPixbuf にテクスチャプレビュー描画 */

void ImageBuf8_drawTexturePreview(ImageBuf8 *p,mPixbuf *pixbuf,int x,int y,int w,int h)
{
	mBox box;
	int ix,iy,sw,sh,pitchd,bpp,fx,fy,fxleft;
	uint8_t *pd,*psY;

	if(!mPixbufGetClipBox_d(pixbuf, &box, x, y, w, h))
		return;

	sw = p->w, sh = p->h;

	pd = mPixbufGetBufPtFast(pixbuf, box.x, box.y);
	bpp = pixbuf->bpp;
	pitchd = pixbuf->pitch_dir - box.w * bpp;

	fxleft = (box.x - x) % sw;
	fy = (box.y - y) % sh;

	psY = p->buf + fy * sw;

	//

	for(iy = box.h; iy; iy--)
	{
		fx = fxleft;

		for(ix = box.w; ix; ix--, pd += bpp)
		{
			(pixbuf->setbuf)(pd, mGraytoPix(255 - *(psY + fx)));

			fx++;
			if(fx == sw) fx = 0;
		}

		fy++;
		if(fy == sh)
			fy = 0, psY = p->buf;
		else
			psY += sw;

		pd += pitchd;
	}
}

/** mPixbuf にブラシ画像プレビュー描画
 *
 * 範囲内に収まるように縮小。左上寄せ。 */

void ImageBuf8_drawBrushPreview(ImageBuf8 *p,mPixbuf *pixbuf,int x,int y,int w,int h)
{
	uint8_t *pd,*psY;
	mBox box;
	int dsize,bpp,pitchd,finc,ix,iy,fx,fy,n;
	uint32_t colex;

	if(!mPixbufGetClipBox_d(pixbuf, &box, x, y, w, h))
		return;

	dsize = (w < h)? w: h;

	pd = mPixbufGetBufPtFast(pixbuf, box.x, box.y);
	bpp = pixbuf->bpp;
	pitchd = pixbuf->pitch_dir - box.w * bpp;

	finc = (int)((double)p->w / dsize * (1<<16) + 0.5);

	colex = mGraytoPix(128);

	//

	for(iy = box.h, fy = 0; iy; iy--, fy += finc)
	{
		n = fy >> 16;
		psY = (n < p->h)? p->buf + n * p->w: NULL;

		for(ix = box.w, fx = 0; ix; ix--, fx += finc, pd += bpp)
		{
			n = fx >> 16;

			if(!psY || n >= p->w)
				(pixbuf->setbuf)(pd, colex);
			else
				(pixbuf->setbuf)(pd, mGraytoPix(255 - *(psY + n)));
		}

		pd += pitchd;
	}
}
