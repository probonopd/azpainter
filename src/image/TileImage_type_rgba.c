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
 * TileImage
 *
 * RGBA 16bit タイプ (1pixel = 8byte)
 *****************************************/
/*
 * R-G-B-A の順で 1px ごとに並んでいる。
 */

#include <string.h>
#include "mDef.h"

#include "defTileImage.h"
#include "TileImage_pv.h"
#include "TileImage_coltype.h"
#include "ImageBuf8.h"


/** 2色が同じ色か (タイルデータ上での色に置換えた時) */

static mBool _isSameColor(RGBAFix15 *p1,RGBAFix15 *p2)
{
	return ((p1->v64 == p2->v64) || (p1->a == 0 && p2->a == 0));
}

/** 2色が同じ色か (アルファ値は判定しない) */

static mBool _isSameRGB(RGBAFix15 *p1,RGBAFix15 *p2)
{
	return (p1->r == p2->r && p1->g == p2->g && p1->b == p2->b);
}

/** タイルと px 位置からバッファ位置取得 */

static uint8_t *_getPixelBufAtTile(TileImage *p,uint8_t *tile,int x,int y)
{
	x = (x - p->offx) & 63;
	y = (y - p->offy) & 63;

	return tile + (((y << 6) + x) << 3);
}

/** タイルと px 位置から色取得 */

static void _getPixelColAtTile(TileImage *p,uint8_t *tile,int x,int y,RGBAFix15 *dst)
{
	x = (x - p->offx) & 63;
	y = (y - p->offy) & 63;

	*dst = *((RGBAFix15 *)( tile + (((y << 6) + x) << 3) ));
}

/** 色をセット */

static void _setPixel(TileImage *p,uint8_t *buf,int x,int y,RGBAFix15 *rgb)
{
	*((RGBAFix15 *)buf) = *rgb;
}

/** タイルがすべて透明か */

static mBool _isTransparentTile(uint8_t *tile)
{
	RGBAFix15 *ps = (RGBAFix15 *)tile;
	int i;

	for(i = 64 * 64; i; i--, ps++)
	{
		if(ps->a) return FALSE;
	}

	return TRUE;
}

/** ImageBufRGB16 に合成 */

static void _blendTile(TileImage *p,TileImageBlendInfo *info)
{
	int pitchs,ix,iy,dx,dy,opacity,a;
	RGBFix15 *pd,src,dst;
	RGBAFix15 *ps;
	TileImageFunc_BlendColor funcBlend;
	ImageBuf8 *imgtex;

	pd = (RGBFix15 *)info->dst;
	ps = (RGBAFix15 *)info->tile + ((info->sy << 6) + info->sx);
	pitchs = 64 - info->w;
	opacity = info->opacity;
	funcBlend = info->funcBlend;
	imgtex = info->imgtex;

	for(iy = info->h, dy = info->dy; iy; iy--, dy++)
	{
		for(ix = info->w, dx = info->dx; ix; ix--, dx++, ps++, pd++)
		{
			a = ps->a;
			if(a == 0) continue;

			if(imgtex)
				a = a * ImageBuf8_getPixel_forTexture(imgtex, dx, dy) / 255;

			a = a * opacity >> 7;

			if(a)
			{
				//色合成

				src.r = ps->r;
				src.g = ps->g;
				src.b = ps->b;

				dst = *pd;

				(funcBlend)(&src, &dst);
				
				//アルファ合成

				if(a == 0x8000)
					*pd = src;
				else
				{
					pd->r = ((src.r - dst.r) * a >> 15) + dst.r;
					pd->g = ((src.g - dst.g) * a >> 15) + dst.g;
					pd->b = ((src.b - dst.b) * a >> 15) + dst.b;
				}
			}
		}

		ps += pitchs;
		pd += info->pitch_dst;
	}
}

/** RGBA タイルとして取得 */

static void _getTileRGBA(TileImage *p,void *dst,uint8_t *src)
{
	memcpy(dst, src, 64 * 64 * 8);
}

/** RGBA タイルからセット */

static void _setTileFromRGBA(uint8_t *dst,void *src,mBool bLumtoA)
{
	memcpy(dst, src, 64 * 64 * 8);
}

/** ファイル保存時用にタイル取得 */

static void _getTileForSave(uint8_t *dst,uint8_t *src)
{
	uint16_t *ps;
	int i,j;

	//BigEndian (R[64*64],G,...A)

	for(j = 0; j < 4; j++)
	{
		ps = (uint16_t *)src + j;
	
		for(i = 64 * 64; i; i--, dst += 2, ps += 4)
		{
			dst[0] = (uint8_t)(*ps >> 8);
			dst[1] = (uint8_t)*ps;
		}
	}
}

/** ファイル保存用データからタイルセット */

static void _setTileForSave(uint8_t *dst,uint8_t *src)
{
	uint16_t *pd;
	int i,j;

	for(j = 0; j < 4; j++)
	{
		pd = (uint16_t *)dst + j;
	
		for(i = 64 * 64; i; i--, pd += 4, src += 2)
			*pd = (src[0] << 8) | src[1];
	}
}

/** タイルを左右反転 */

static void _reverseHorz(uint8_t *tile)
{
	RGBAFix15 *p1,*p2,c;
	int ix,iy;

	p1 = (RGBAFix15 *)tile;
	p2 = p1 + 63;

	for(iy = 64; iy; iy--)
	{
		for(ix = 32; ix; ix--, p1++, p2--)
		{
			c   = *p1;
			*p1 = *p2;
			*p2 = c;
		}

		p1 += 64 - 32;
		p2 += 64 + 32;
	}
}

/** タイルを上下反転 */

static void _reverseVert(uint8_t *tile)
{
	RGBAFix15 *p1,*p2,c;
	int ix,iy;

	p1 = (RGBAFix15 *)tile;
	p2 = p1 + 63 * 64;

	for(iy = 32; iy; iy--, p2 -= 128)
	{
		for(ix = 64; ix; ix--, p1++, p2++)
		{
			c   = *p1;
			*p1 = *p2;
			*p2 = c;
		}
	}
}

/** タイルを左に90度回転 */

static void _rotateLeft(uint8_t *tile)
{
	RGBAFix15 *p1,*p2,*p3,*p4,c1,c2;
	int ix,iy;

	p1 = (RGBAFix15 *)tile;
	p2 = p1 + 63 * 64;
	p3 = p1 + 63;
	p4 = p2 + 63;

	for(iy = 32; iy; iy--)
	{
		for(ix = 32; ix; ix--, p1++, p2 -= 64, p3 += 64, p4--)
		{
			c1  = *p1;
			c2  = *p2;
			*p1 = *p3;
			*p2 = c1;
			*p3 = *p4;
			*p4 = c2;
		}

		p1 += 32;
		p2 += 32 * 64 + 1;
		p3 -= 32 * 64 + 1;
		p4 -= 32;
	}
}

/** タイルを右に90度回転 */

static void _rotateRight(uint8_t *tile)
{
	RGBAFix15 *p1,*p2,*p3,*p4,c1,c3;
	int ix,iy;

	p1 = (RGBAFix15 *)tile;
	p2 = p1 + 63 * 64;
	p3 = p1 + 63;
	p4 = p2 + 63;

	for(iy = 32; iy; iy--)
	{
		for(ix = 32; ix; ix--, p1++, p2 -= 64, p3 += 64, p4--)
		{
			c1 = *p1;
			c3 = *p3;

			*p1 = *p2;
			*p2 = *p4;
			*p3 = c1;
			*p4 = c3;
		}

		p1 += 32;
		p2 += 32 * 64 + 1;
		p3 -= 32 * 64 + 1;
		p4 -= 32;
	}
}

/** 関数テーブルにセット */

void __TileImage_RGBA_setFuncTable(TileImageColtypeFuncs *p)
{
	p->isSameColor = _isSameColor;
	p->isSameRGB = _isSameRGB;
	p->getPixelBufAtTile = _getPixelBufAtTile;
	p->getPixelColAtTile = _getPixelColAtTile;
	p->setPixel = _setPixel;
	p->isTransparentTile = _isTransparentTile;
	p->blendTile = _blendTile;
	p->getTileRGBA = _getTileRGBA;
	p->setTileFromRGBA = _setTileFromRGBA;
	p->reverseHorz = _reverseHorz;
	p->reverseVert = _reverseVert;
	p->rotateLeft = _rotateLeft;
	p->rotateRight = _rotateRight;
	p->getTileForSave = _getTileForSave;
	p->setTileForSave = _setTileForSave;
}
