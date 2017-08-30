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
 * Alpha1bit タイプ
 *****************************************/
/*
 * - 上位ビットから順に x 0..7。
 * - アルファ値が 0 より大きければアルファ値を ON とする。
 */

#include <string.h>

#include "mDef.h"
#include "mPixbuf.h"

#include "defTileImage.h"
#include "TileImage_pv.h"
#include "TileImage_coltype.h"
#include "ImageBuf8.h"



/** 2色が同じ色か */

static mBool _isSameColor(RGBAFix15 *src,RGBAFix15 *dst)
{
	return ((!src->a) == (!dst->a));
}

/** 2色が同じ色か (アルファ値は判定しない) */

static mBool _isSameRGB(RGBAFix15 *p1,RGBAFix15 *p2)
{
	return TRUE;
}

/** タイルと px 位置からバッファ位置取得 */

static uint8_t *_getPixelBufAtTile(TileImage *p,uint8_t *tile,int x,int y)
{
	x = (x - p->offx) & 63;
	y = (y - p->offy) & 63;

	return tile + ((y << 3) + (x >> 3));
}

/** タイルと px 位置から色取得 */

static void _getPixelColAtTile(TileImage *p,uint8_t *tile,int x,int y,RGBAFix15 *dst)
{
	x = (x - p->offx) & 63;
	y = (y - p->offy) & 63;

	tile += (y << 3) + (x >> 3);

	dst->r = p->rgb.r;
	dst->g = p->rgb.g;
	dst->b = p->rgb.b;
	dst->a = ( *tile & (1 << (7 - (x & 7))) )? 0x8000: 0;
}

/** 色をセット */

static void _setPixel(TileImage *p,uint8_t *buf,int x,int y,RGBAFix15 *rgb)
{
	x = (x - p->offx) & 63;
	x = 1 << (7 - (x & 7));

	if(rgb->a)
		*buf |= x;
	else
		*buf &= ~x;
}

/** タイルがすべて透明か */

static mBool _isTransparentTile(uint8_t *tile)
{
	uint32_t *ps = (uint32_t *)tile;
	int i;

	//4byte 単位で比較

	for(i = 64 * 64 / 8 / 4; i; i--)
	{
		if(*(ps++)) return FALSE;
	}

	return TRUE;
}

/** ImageBufRGB16 に合成 */

static void _blendTile(TileImage *p,TileImageBlendInfo *info)
{
	int ix,iy,dx,dy,opacity,a,srca;
	RGBFix15 *pd,src,dst,imgcol;
	uint8_t *ps,*ps_bk,stf,f;
	TileImageFunc_BlendColor funcBlend;
	ImageBuf8 *imgtex;

	pd = (RGBFix15 *)info->dst;
	ps = info->tile + ((info->sy << 3) + (info->sx >> 3));
	opacity = info->opacity;
	funcBlend = info->funcBlend;
	imgcol = p->rgb;
	imgtex = info->imgtex;

	stf = 1 << (7 - (info->sx & 7));
	srca = opacity << 8; //0x8000 * opacity >> 7 = opacity << 15 >> 7

	//

	for(iy = info->h, dy = info->dy; iy; iy--, dy++)
	{
		ps_bk = ps;
		f = stf;
	
		for(ix = info->w, dx = info->dx; ix; ix--, dx++, pd++)
		{
			if(*ps & f)
			{
				if(imgtex)
					a = ((ImageBuf8_getPixel_forTexture(imgtex, dx, dy) << 15) / 255) * opacity >> 7;
				else
					a = srca;

				//色合成

				src = imgcol;
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

			f >>= 1;
			if(!f) f = 0x80, ps++;
		}

		ps = ps_bk + 8;
		pd += info->pitch_dst;
	}
}

/** RGBA タイルとして取得 */

static void _getTileRGBA(TileImage *p,void *dst,uint8_t *src)
{
	RGBAFix15 *pd = (RGBAFix15 *)dst;
	RGBFix15 col = p->rgb;
	int i;
	uint8_t f = 0x80;

	for(i = 64 * 64; i; i--, pd++)
	{
		pd->r = col.r;
		pd->g = col.g;
		pd->b = col.b;
		pd->a = (*src & f)? 0x8000: 0;

		f >>= 1;
		if(!f) f = 0x80, src++;
	}
}

/** RGBA タイルからセット */

static void _setTileFromRGBA(uint8_t *dst,void *src,mBool bLumtoA)
{
	RGBAFix15 *ps = (RGBAFix15 *)src;
	int i,j,a;
	uint8_t f,val;

	for(i = 64 * 8; i; i--)
	{
		for(j = 8, f = 0x80, val = 0; j; j--, f >>= 1, ps++)
		{
			if(bLumtoA)
			{
				if(ps->a)
					a = 0x8000 - ((ps->r * 77 + ps->g * 150 + ps->b * 29) >> 8);
				else
					a = 0;
			}
			else
				a = ps->a;

			if(a) val |= f;
		}

		*(dst++) = val;
	}
}

/** ファイル保存にタイル取得/セット (共通) */

static void _getsetTileForSave(uint8_t *dst,uint8_t *src)
{
	memcpy(dst, src, 64 * 64 / 8);
}

/** タイルを左右反転 */

static void _reverseHorz(uint8_t *tile)
{
	uint8_t *p1,*p2,s1,s2,d1,d2,f1,f2;
	int ix,iy,i;

	p1 = (uint8_t *)tile;
	p2 = p1 + 7;

	for(iy = 64; iy; iy--, p1 += 4, p2 += 12)
	{
		for(ix = 4; ix; ix--)
		{
			s1 = *p1, s2 = *p2;
			d1 = d2 = 0;
			f1 = 0x80, f2 = 1;

			for(i = 8; i; i--, f1 >>= 1, f2 <<= 1)
			{
				if(s1 & f1) d2 |= f2;
				if(s2 & f1) d1 |= f2;
			}

			*(p1++) = d1;
			*(p2--) = d2;
		}
	}
}

/** タイルを上下反転 */

static void _reverseVert(uint8_t *tile)
{
	uint8_t *p1,*p2,c;
	int ix,iy;

	p1 = (uint8_t *)tile;
	p2 = p1 + 63 * 8;

	for(iy = 32; iy; iy--, p2 -= 16)
	{
		for(ix = 8; ix; ix--, p1++, p2++)
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
	uint8_t *p1,*p2,*p3,*p4;
	int f1,f2,f3,f4,ix,iy;
	uint8_t c1,c2,c3,c4;

	p1 = tile;
	p2 = p1 + 63 * 8;
	p3 = p1 + 7;
	p4 = p2 + 7;

	f1 = f2 = 7;
	f3 = f4 = 0;

	for(iy = 32; iy; iy--)
	{
		for(ix = 32; ix; ix--)
		{
			c1 = (*p1 >> f1) & 1;
			c2 = (*p2 >> f2) & 1;
			c3 = (*p3 >> f3) & 1;
			c4 = (*p4 >> f4) & 1;

			if(c3) *p1 |= 1 << f1; else *p1 &= ~(1 << f1);
			if(c1) *p2 |= 1 << f2; else *p2 &= ~(1 << f2);
			if(c4) *p3 |= 1 << f3; else *p3 &= ~(1 << f3);
			if(c2) *p4 |= 1 << f4; else *p4 &= ~(1 << f4);

			f1--; if(f1 < 0) f1 = 7, p1++;
			p2 -= 8;
			p3 += 8;
			f4++; if(f4 > 7) f4 = 0, p4--;
		}

		p1 += 4;
		p2 += 32 * 8, f2--; if(f2 < 0) f2 = 7, p2++;
		p3 -= 32 * 8, f3++; if(f3 > 7) f3 = 0, p3--;
		p4 -= 4;
	}
}

/** タイルを右に90度回転 */

static void _rotateRight(uint8_t *tile)
{
	uint8_t *p1,*p2,*p3,*p4;
	int f1,f2,f3,f4,ix,iy;
	uint8_t c1,c2,c3,c4;

	p1 = tile;
	p2 = p1 + 63 * 8;
	p3 = p1 + 7;
	p4 = p2 + 7;

	f1 = f2 = 7;
	f3 = f4 = 0;

	for(iy = 32; iy; iy--)
	{
		for(ix = 32; ix; ix--)
		{
			c1 = (*p1 >> f1) & 1;
			c2 = (*p2 >> f2) & 1;
			c3 = (*p3 >> f3) & 1;
			c4 = (*p4 >> f4) & 1;

			if(c2) *p1 |= 1 << f1; else *p1 &= ~(1 << f1);
			if(c4) *p2 |= 1 << f2; else *p2 &= ~(1 << f2);
			if(c1) *p3 |= 1 << f3; else *p3 &= ~(1 << f3);
			if(c3) *p4 |= 1 << f4; else *p4 &= ~(1 << f4);

			f1--; if(f1 < 0) f1 = 7, p1++;
			p2 -= 8;
			p3 += 8;
			f4++; if(f4 > 7) f4 = 0, p4--;
		}

		p1 += 4;
		p2 += 32 * 8, f2--; if(f2 < 0) f2 = 7, p2++;
		p3 -= 32 * 8, f3++; if(f3 > 7) f3 = 0, p3--;
		p4 -= 4;
	}
}

/** 関数テーブルにセット */

void __TileImage_A1_setFuncTable(TileImageColtypeFuncs *p)
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
	p->getTileForSave = _getsetTileForSave;
	p->setTileForSave = _getsetTileForSave;
}


//===========================
// 特殊
//===========================


/** mPixbuf に XOR 合成 */

void __TileImage_A1_blendXorToPixbuf(TileImage *p,mPixbuf *pixbuf,TileImageBlendInfo *info)
{
	int ix,iy,pitchd,bpp;
	uint8_t *ps,*pd,*psY,f,fleft;

	pd = mPixbufGetBufPtFast(pixbuf, info->dx, info->dy);
	psY = info->tile + (info->sy << 3) + (info->sx >> 3);

	bpp = pixbuf->bpp;
	pitchd = pixbuf->pitch_dir - info->w * bpp;

	fleft = 1 << (7 - (info->sx & 7));

	for(iy = info->h; iy; iy--)
	{
		ps = psY;
		f  = fleft;
	
		for(ix = info->w; ix; ix--, pd += bpp)
		{
			if(*ps & f)
				(pixbuf->setbuf)(pd, MPIXBUF_COL_XOR);
			
			f >>= 1;
			if(!f) f = 0x80, ps++;
		}

		psY += 8;
		pd += pitchd;
	}
}
