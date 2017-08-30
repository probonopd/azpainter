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
 * 描画サブ
 *****************************************/

#include <string.h>

#include "mDef.h"
#include "mRectBox.h"

#include "defTileImage.h"
#include "TileImage.h"
#include "TileImageDrawInfo.h"
#include "TileImage_pv.h"


//-----------------

#define _BLEND_ALPHA(a,b)  ((a) + (b) - ((a) * (b) >> 15))

//-----------------



//==========================
// 指先用
//==========================


/** 水平線描画 */

static void _dotstyle_lineh(uint8_t *buf,int size,int x1,int y,int x2)
{
	int pos;
	uint8_t f;

	pos = x1 + y * size;

	buf += pos >> 3;
	f = 1 << (7 - (pos & 7));

	for(; x1 <= x2; x1++)
	{
		*buf |= f;

		f >>= 1;
		if(!f) f = 0x80, buf++;
	}
}

/** ドットの円形形状スタイルセット */

void TileImage_setDotStyle(int radius)
{
	uint8_t *buf;
	int size,ct,x,y,e;

	if(radius >= TILEIMAGE_DOTSTYLE_RADIUS_MAX)
		radius = TILEIMAGE_DOTSTYLE_RADIUS_MAX;

	//直径サイズ

	size = radius * 2 + 1;

	//現在セットされている値と同じならそのまま

	if(g_tileimage_work.dot_size == size) return;

	//

	buf = g_tileimage_work.dotstyle;

	memset(buf, 0, (size * size + 7) / 8);
	
	ct = radius;

	x = 0, y = ct;
	e = 3 - 2 * y;

	while(x <= y)
	{
		_dotstyle_lineh(buf, size, ct - x, ct - y, ct + x);
		_dotstyle_lineh(buf, size, ct - y, ct - x, ct + y);
		_dotstyle_lineh(buf, size, ct - y, ct + x, ct + y);
		_dotstyle_lineh(buf, size, ct - x, ct + y, ct + x);

		if(e < 0)
			e += 4 * x + 6;
		else
		{
			e += 4 * (x - y) + 10;
			y--;
		}

		x++;
	}

	//

	g_tileimage_work.dot_size = size;
}

/** 指定位置周辺の色を指先バッファにセット (指先描画開始時) */

void TileImage_setFingerBuf(TileImage *p,int x,int y)
{
	TileImageWorkData *work = &g_tileimage_work;
	uint8_t *ps,f,val;
	RGBAFix15 *pd;
	int size,ix,iy,xx,yy,ct;

	ps = work->dotstyle;
	pd = work->fingerbuf;
	size = work->dot_size;

	ct = size >> 1;

	//

	yy = y - ct;

	for(iy = size, f = 0x80; iy > 0; iy--, yy++)
	{
		xx = x - ct;
	
		for(ix = size; ix > 0; ix--, xx++, pd++)
		{
			if(f == 0x80) val = *(ps++);

			if(val & f)
				TileImage_getPixel_clip(p, xx, yy, pd);

			f >>= 1;
			if(!f) f = 0x80;
		}
	}
}


//==========================
// 塗りつぶし用
//==========================


/** src (A1) でイメージがある部分に点を描画する */

void TileImage_drawPixels_fromA1(TileImage *dst,TileImage *src,RGBAFix15 *pix)
{
	uint8_t **pptile,*ps,f;
	int x,y,ix,iy,jx,jy;
	void (*setpix)(TileImage *,int,int,RGBAFix15 *) = g_tileimage_dinfo.funcDrawPixel;

	pptile = src->ppbuf;
	y = src->offy;

	//src の全タイルを参照

	for(iy = src->tileh; iy; iy--, y += 64)
	{
		x = src->offx;

		for(ix = src->tilew; ix; ix--, x += 64, pptile++)
		{
			if(!(*pptile)) continue;

			//src のタイルの点を判定して描画

			ps = *pptile;
			f  = 0x80;

			for(jy = 0; jy < 64; jy++)
			{
				for(jx = 0; jx < 64; jx++)
				{
					if(*ps & f)
						(setpix)(dst, x + jx, y + jy, pix);

					f >>= 1;
					if(f == 0) f = 0x80, ps++;
				}
			}
		}
	}
}

/** ２つの A1 タイプをタイルごとに結合
 *
 * タイル配列構成は同じであること。 */

void TileImage_combine_forA1(TileImage *dst,TileImage *src)
{
	uint8_t **ppdst,**ppsrc;
	uint32_t *ps,*pd;
	uint32_t i,j;

	ppdst = dst->ppbuf;
	ppsrc = src->ppbuf;

	for(i = dst->tilew * dst->tileh; i; i--, ppsrc++, ppdst++)
	{
		if(*ppsrc)
		{
			if(*ppdst)
			{
				//両方あり : 4byte 単位で OR 結合

				ps = (uint32_t *)*ppsrc;
				pd = (uint32_t *)*ppdst;

				for(j = 8 * 64 / 4; j; j--)
					*(pd++) |= *(ps++);
			}
			else
			{
				//dst が空 : 確保してコピー

				*ppdst = TileImage_allocTile(dst, FALSE);
				if(*ppdst)
					memcpy(*ppdst, *ppsrc, 8 * 64);
			}
		}
	}
}

/** 高速水平線描画 (A1) */

mBool TileImage_drawLineH_forA1(TileImage *p,int x1,int x2,int y)
{
	TileImageTileRectInfo info;
	mBox box;
	uint8_t **pptile,*pd,f;
	int xx,yy,tw;

	//描画対象のタイル範囲

	box.x = x1, box.y = y;
	box.w = x2 - x1 + 1, box.h = 1;

	pptile = TileImage_getTileRectInfo(p, &info, &box);
	if(!pptile) return TRUE;	//NULL で範囲外

	tw = info.rctile.x2 - info.rctile.x1 + 1;

	//クリッピング

	if(x1 < info.pxtop.x) x1 = info.pxtop.x;
	if(x2 >= info.pxtop.x + tw * 64) x2 = info.pxtop.x + tw * 64 - 1;

	//先頭タイル確保

	if(!TileImage_allocTile_atptr(p, pptile, TRUE))
		return FALSE;

	//タイルごとに描画

	xx = (x1 - p->offx) & 63;
	yy = ((y - p->offy) & 63) << 3;
	f  = 1 << (7 - (xx & 7));
	pd = *pptile + yy + (xx >> 3);

	for(; tw; tw--)
	{
		//タイル一つ分横線を引く

		for(; xx < 64; xx++, x1++)
		{
			*pd |= f;

			//終了
			if(x1 == x2) return TRUE;

			f >>= 1;
			if(f == 0) f = 0x80, pd++;
		}

		//次のタイル

		pptile++;

		if(!TileImage_allocTile_atptr(p, pptile, TRUE))
			return FALSE;

		pd = *pptile + yy;
		xx = 0;
	}

	return TRUE;
}

/** 高速垂直線描画 (A1) */

mBool TileImage_drawLineV_forA1(TileImage *p,int y1,int y2,int x)
{
	TileImageTileRectInfo info;
	mBox box;
	uint8_t **pptile,*pd,f;
	int xx,yy,th;

	//描画タイル範囲

	box.x = x, box.y = y1;
	box.w = 1, box.h = y2 - y1 + 1;

	pptile = TileImage_getTileRectInfo(p, &info, &box);
	if(!pptile) return TRUE;	//NULL で範囲外

	th = info.rctile.y2 - info.rctile.y1 + 1;

	//クリッピング

	if(y1 < info.pxtop.y) y1 = info.pxtop.y;
	if(y2 >= info.pxtop.y + th * 64) y2 = info.pxtop.y + th * 64 - 1;

	//先頭タイル確保

	if(!TileImage_allocTile_atptr(p, pptile, TRUE))
		return FALSE;

	//

	xx = (x - p->offx) & 63;
	yy = (y1 - p->offy) & 63;
	f  = 1 << (7 - (xx & 7));
	xx >>= 3;
	pd = *pptile + (yy << 3) + xx;

	for(; th; th--)
	{
		//タイル一つ分縦線を引く

		for(; yy < 64; yy++, y1++, pd += 8)
		{
			*pd |= f;

			//終了
			if(y1 == y2) return TRUE;
		}

		//次のタイル

		pptile += p->tilew;

		if(!TileImage_allocTile_atptr(p, pptile, TRUE))
			return FALSE;

		pd = *pptile + xx;
		yy = 0;
	}

	return TRUE;
}
