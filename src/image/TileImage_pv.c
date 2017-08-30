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

/**********************************
 * TileImage
 *
 * 内部処理
 **********************************/

#include "mDef.h"

#include "defTileImage.h"
#include "TileImage.h"
#include "TileImage_pv.h"



//==========================
// 作成
//==========================


/** TileImage 作成 */

TileImage *__TileImage_create(int type,int tilew,int tileh)
{
	TileImage *p;

	p = (TileImage *)mMalloc(sizeof(TileImage), TRUE);
	if(!p) return NULL;

	p->coltype = type;
	p->tilew = tilew;
	p->tileh = tileh;

	__TileImage_setTileSize(p);

	//タイルバッファ確保

	if(!__TileImage_allocTileBuf(p))
	{
		mFree(p);
		return NULL;
	}

	return p;
}

/** タイルサイズをセット */

void __TileImage_setTileSize(TileImage *p)
{
	int tsize[4] = {64*64*4*2, 64*64*2*2, 64*64*2, 64*64/8};

	p->tilesize = tsize[p->coltype];
}


//==========================
// タイルバッファ
//==========================


/** タイルバッファを確保
 *
 * tilew,tileh はあらかじめセットしておく。 */

mBool __TileImage_allocTileBuf(TileImage *p)
{
	p->ppbuf = (uint8_t **)mMalloc(p->tilew * p->tileh * sizeof(void *), TRUE);

	return (p->ppbuf != 0);
}

/** リサイズなど用に新規タイル配列確保 */

uint8_t **__TileImage_allocTileBuf_new(int w,int h,mBool clear)
{
	return (uint8_t **)mMalloc(w * h * sizeof(void *), clear);
}

/** タイル配列リサイズ
 * 
 * 拡張処理のみ。サイズの縮小処理は行わない。
 * 左/上方向への拡張ならば、オフセット位置をずらす。
 *
 * @param movx,movy  負の値の場合、タイル位置の移動数。0 以上なら右/下方向への拡張。 */
 
mBool __TileImage_resizeTileBuf(TileImage *p,int movx,int movy,int neww,int newh)
{
	uint8_t **ppnew,**ppd;
	int ix,iy,tx,ty,srcw,srch;

	if(movx > 0) movx = 0;
	if(movy > 0) movy = 0;

	srcw = p->tilew;
	srch = p->tileh;

	//変更なし

	if(neww == srcw && newh == srch)
		return TRUE;

	//新規確保

	ppnew = __TileImage_allocTileBuf_new(neww, newh, TRUE);
	if(!ppnew) return FALSE;

	//元データコピー

	ppd = ppnew;

	for(iy = newh, ty = movy; iy > 0; iy--, ty++)
	{
		for(ix = neww, tx = movx; ix > 0; ix--, tx++, ppd++)
		{
			if(tx >= 0 && tx < srcw && ty >= 0 && ty < srch)
				*ppd = *(p->ppbuf + ty * srcw + tx);
		}
	}

	//入れ替え

	mFree(p->ppbuf);

	p->ppbuf = ppnew;
	p->tilew = neww;
	p->tileh = newh;
	p->offx += movx << 6;
	p->offy += movy << 6;

	return TRUE;
}

/** 指定イメージと同じ構成になるようにタイル配列リサイズ
 *
 * src の配列リサイズ後すぐに実行すること。 */

mBool __TileImage_resizeTileBuf_clone(TileImage *p,TileImage *src)
{
	return __TileImage_resizeTileBuf(p,
		(src->offx - p->offx) >> 6, (src->offy - p->offy) >> 6,
		src->tilew, src->tileh);
}


//==============================
// 合成用
//==============================


/** タイル毎の合成時用の情報セット */

void __TileImage_setBlendInfo(TileImageBlendInfo *info,int px,int py,mRect *rcclip)
{
	int dx,dy;

	dx = px, dy = py;

	info->sx = info->sy = 0;
	info->w = info->h = 64;

	if(dx < rcclip->x1)
	{
		info->w  -= rcclip->x1 - dx;
		info->sx += rcclip->x1 - dx;
		dx = rcclip->x1;
	}

	if(dy < rcclip->y1)
	{
		info->h  -= rcclip->y1 - dy;
		info->sy += rcclip->y1 - dy;
		dy = rcclip->y1;
	}

	if(dx + info->w > rcclip->x2) info->w = rcclip->x2 - dx;
	if(dy + info->h > rcclip->y2) info->h = rcclip->y2 - dy;

	info->dx = dx, info->dy = dy;
}
