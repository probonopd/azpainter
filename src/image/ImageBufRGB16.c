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
 * RGB 16bit イメージ (レイヤ合成後用)
 *****************************************/

#include <string.h>

#include "mDef.h"

#include "ImageBufRGB16.h"
#include "ColorValue.h"


/** 解放 */

void ImageBufRGB16_free(ImageBufRGB16 *p)
{
	if(p)
	{
		mFree(p->buf);
		mFree(p);
	}
}

/** 作成 */

ImageBufRGB16 *ImageBufRGB16_new(int width,int height)
{
	ImageBufRGB16 *p;

	p = (ImageBufRGB16 *)mMalloc(sizeof(ImageBufRGB16), TRUE);
	if(!p) return NULL;

	p->buf = (RGBFix15 *)mMalloc(width * height * sizeof(RGBFix15), FALSE);
	if(!p->buf)
	{
		mFree(p);
		return NULL;
	}

	p->width = width;
	p->height = height;

	return p;
}

/** 指定位置のバッファを取得 */

RGBFix15 *ImageBufRGB16_getBufPt(ImageBufRGB16 *p,int x,int y)
{
	return p->buf + p->width * y + x;
}

/** 指定色で埋める */

void ImageBufRGB16_fill(ImageBufRGB16 *p,RGBFix15 *rgb)
{
	RGBFix15 *pd,c;
	int i,size;

	//Y1列分

	pd = p->buf;
	c = *rgb;

	for(i = p->width; i; i--)
		*(pd++) = c;

	//以降はY1列分コピー

	size = p->width * sizeof(RGBFix15);

	for(i = p->height - 1; i > 0; i--, pd += p->width)
		memcpy(pd, pd - p->width, size);
}

/** 範囲を指定色で埋める */

void ImageBufRGB16_fillBox(ImageBufRGB16 *p,mBox *box,RGBFix15 *rgb)
{
	RGBFix15 *pd,c = *rgb;
	int i,pitch,size;

	pd = ImageBufRGB16_getBufPt(p, box->x, box->y);

	//Y1列分

	for(i = box->w; i > 0; i--)
		*(pd++) = c;

	pd += p->width - box->w;

	//以降はY1列分コピー

	pitch = p->width;
	size = box->w * sizeof(RGBFix15);

	for(i = box->h - 1; i > 0; i--, pd += pitch)
		memcpy(pd, pd - pitch, size);
}

/** 範囲をチェック柄で塗りつぶす */

void ImageBufRGB16_fillPlaidBox(ImageBufRGB16 *p,mBox *box,RGBFix15 *rgb1,RGBFix15 *rgb2)
{
	RGBFix15 *pd,*ps,c[2];
	int ix,iy,xx,yy,fy,pitch;

	c[0] = *rgb1;
	c[1] = *rgb2;

	//高さ 16px までを埋める

	pd = ImageBufRGB16_getBufPt(p, box->x, box->y);
	pitch = p->width - box->w;
	
	iy = (box->h > 16)? 16: box->h;
	yy = box->y & 15;

	for(; iy > 0; iy--)
	{
		xx = box->x & 15;
		fy = yy >> 3;

		for(ix = box->w; ix > 0; ix--)
		{
			*(pd++) = c[fy ^ (xx >> 3)];

			xx = (xx + 1) & 15;
		}

		pd += pitch;
		yy = (yy + 1) & 15;
	}

	//残りは 16px 前の行をコピー

	if(box->h > 16)
	{
		pitch = p->width;
		xx = box->w * sizeof(RGBFix15);
		ps = pd - (pitch << 4);

		for(iy = box->h - 16; iy > 0; iy--)
		{
			memcpy(pd, ps, xx);

			pd += pitch;
			ps += pitch;
		}
	}
}

/** 64x64 分を RGBA タイルバッファに取得 */

void ImageBufRGB16_getTileRGBA(ImageBufRGB16 *p,RGBAFix15 *dst,int x,int y)
{
	int w,h,ix,iy,pitchs;
	RGBFix15 *ps;

	w = h = 64;

	if(x + w > p->width) w = p->width - x;
	if(y + h > p->height) h = p->height - y;

	//64x64 分ない場合はクリア

	if(w != 64 || h != 64)
		memset(dst, 0, sizeof(RGBAFix15) * 64 * 64);

	//

	ps = p->buf + y * p->width + x;
	pitchs = p->width - w;

	for(iy = h; iy; iy--)
	{
		for(ix = w; ix; ix--, dst++, ps++)
		{
			dst->r = ps->r;
			dst->g = ps->g;
			dst->b = ps->b;
			dst->a = 0x8000;
		}

		dst += 64 - w;
		ps += pitchs;
	}
}

/** バッファのデータを RGB 8bit に変換 */

mBool ImageBufRGB16_toRGB8(ImageBufRGB16 *p)
{
	uint32_t i,j;
	uint8_t *pd,*tbl;
	uint16_t *ps;

	//テーブル作成 (fix15 -> 8bit)

	tbl = (uint8_t *)mMalloc(0x8000 + 1, FALSE);
	if(!tbl) return FALSE;

	for(i = 0, pd = tbl; i <= 0x8000; i++)
		*(pd++) = (i * 255 + 0x4000) >> 15;

	//変換

	ps = (uint16_t *)p->buf;
	pd = (uint8_t *)p->buf;

	for(i = p->width * p->height; i; i--)
	{
		for(j = 0; j < 3; j++)
			*(pd++) = tbl[*(ps++)];
	}

	mFree(tbl);

	return TRUE;
}
