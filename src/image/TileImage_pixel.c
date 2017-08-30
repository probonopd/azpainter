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
 * ピクセル関連
 *****************************************/

#include <math.h>
#include <string.h>

#include "mDef.h"
#include "mRectBox.h"

#include "defTileImage.h"

#include "TileImage.h"
#include "TileImage_pv.h"
#include "TileImage_coltype.h"
#include "TileImageDrawInfo.h"

#include "blendcol.h"
#include "ImageBuf8.h"


//-------------------

typedef struct
{
	uint8_t **pptile;	//描画先のタイルバッファポインタ (NULL でタイル範囲外 [!]描画対象ではある)
	int tx,ty,			//タイル位置
		max_alpha;		//レイヤマスクによるアルファ値制限
	RGBAFix15 pixdst;	//描画先の色
}_setpixelinfo;


typedef struct
{
	int x,y;
	uint8_t *ptile;
}_colparam;

//-------------------



//================================
// sub
//================================


/** 色マスク判定
 *
 * @return TRUE で保護 */

static mBool _setpixeldraw_colormask(int type,RGBAFix15 *pix)
{
	RGBFix15 *pmc;
	int f;

	//描画先が透明の場合、[マスク]描画 [逆マスク]保護

	if(pix->a == 0)
		return (type == 2);

	//マスク色の中に描画先の色があるか

	for(pmc = g_tileimage_dinfo.colmask_col, f = 0; pmc->r != 0xffff; pmc++)
	{
		if(pmc->r == pix->r && pmc->g == pix->g && pmc->b == pix->b)
		{
			f = 1;
			break;
		}
	}

	//逆マスクの場合は判定を逆にする
	
	if(type == 2) f = !f;

	return f;
}

/** 描画前の元イメージの色を取得
 *
 * _setpixeldraw_dstpixel() 後に使用可能 */

static void _setpixeldraw_get_save_color(TileImage *p,int x,int y,
	_setpixelinfo *info,RGBAFix15 *pix)
{
	uint8_t **pptile,*buf;

	pix->v64 = 0;
	
	pptile = info->pptile;

	/* 描画先と保存イメージのタイル配列構成は同じなので、
	 * 描画先がタイル範囲外なら保存イメージも範囲外 */

	if(pptile)
	{
		//保存イメージのタイル
		
		buf = TILEIMAGE_GETTILE_PT(g_tileimage_dinfo.img_save, info->tx, info->ty);

		if(!buf)
		{
			//保存イメージのタイルが未確保なら、まだ描画による変更がないので、描画先から取得

			if(*pptile)
				(g_tileimage_funcs[p->coltype].getPixelColAtTile)(p, *pptile, x, y, pix);
		}
		else if(buf != TILEIMAGE_TILE_EMPTY)
			//タイルがある場合
			(g_tileimage_funcs[p->coltype].getPixelColAtTile)(g_tileimage_dinfo.img_save, buf, x, y, pix);
	}
}

/** 描画先の情報取得 + マスク処理
 *
 * @param pixdraw   描画色。テクスチャが適用されて返る。
 * @return TRUE で処理しない */

static mBool _setpixeldraw_dstpixel(TileImage *p,int x,int y,RGBAFix15 *pixdraw,
	_setpixelinfo *info)
{
	uint8_t **pptile;
	int tx,ty,maska;
	RGBAFix15 pix;

	//エラー時は処理しない

	if(g_tileimage_dinfo.err) return TRUE;

	//選択範囲 (点がない場合は描画しない)

	if(g_tileimage_dinfo.img_sel)
	{
		if(TileImage_isPixelTransparent(g_tileimage_dinfo.img_sel, x, y))
			return TRUE;
	}
	
	//テクスチャ適用
	/* [!] 色を上書きする場合は、透明時も色をセットしなければいけないので、
	 *     テクスチャの値が 0 でも処理を続ける。 */

	if(g_tileimage_dinfo.texture)
	{
		pixdraw->a = (pixdraw->a
			* ImageBuf8_getPixel_forTexture(g_tileimage_dinfo.texture, x, y) + 127) / 255;
	}

	//レイヤマスク値取得

	if(!g_tileimage_dinfo.img_mask)
		//マスクイメージなし
		maska = 0x8000;
	else
	{
		TileImage_getPixel(g_tileimage_dinfo.img_mask, x, y, &pix);
		maska = pix.a;

		//A=0 の場合は色は変化しない

		if(maska == 0) return TRUE;
	}

	//描画先の色とタイル位置取得
	//pix = 描画先の色

	pix.v64 = 0;

	if(TileImage_pixel_to_tile(p, x, y, &tx, &ty))
	{
		//タイル配列範囲内
		
		pptile = TILEIMAGE_GETTILE_BUFPT(p, tx, ty);

		if(*pptile)
			(g_tileimage_funcs[p->coltype].getPixelColAtTile)(p, *pptile, x, y, &pix);

		info->pptile = pptile;
		info->tx = tx;
		info->ty = ty;
	}
	else
	{
		//タイル配列範囲外

		info->pptile = NULL;

		//キャンバス範囲外なら処理しない

		if(x < 0 || y < 0 || x >= g_tileimage_dinfo.imgw || y >= g_tileimage_dinfo.imgh)
			return TRUE;
	}

	info->pixdst = pix;

	//描画前状態の色によるマスク

	if(g_tileimage_dinfo.alphamask_type >= 2 || g_tileimage_dinfo.colmask_type)
	{
		//描画前の色取得
	
		_setpixeldraw_get_save_color(p, x, y, info, &pix);

		//アルファマスク

		if(g_tileimage_dinfo.alphamask_type == 2 && pix.a == 0)
			//透明色保護
			return TRUE;
		else if(g_tileimage_dinfo.alphamask_type == 3 && pix.a)
			//不透明色保護
			return TRUE;

		//色マスク判定

		if(g_tileimage_dinfo.colmask_type
			&& _setpixeldraw_colormask(g_tileimage_dinfo.colmask_type, &pix))
			return TRUE;
	}

	//

	info->max_alpha = maska;

	return FALSE;
}

/** 描画前のタイル関連処理
 *
 * [ タイル作成、タイル配列リサイズ、アンドゥ用イメージコピー ]
 * これが終了した時点で、エラー時を除き info->pptile には必ず NULL 以外が入る。
 * 
 * @return FALSE で確保エラー */

static mBool _setpixeldraw_tile(TileImage *p,int x,int y,_setpixelinfo *info)
{
	TileImage *img_save;
	uint8_t **pptile,**pptmp;
	int tx,ty;
	mBool bEmptyTile = FALSE;

	pptile = info->pptile;
	tx = info->tx;
	ty = info->ty;

	img_save = g_tileimage_dinfo.img_save;

	//タイル配列が範囲外の場合

	if(!pptile)
	{
		//描画先のタイル配列リサイズ

		if(!TileImage_resizeTileBuf_includeImage(p)) return FALSE;

		//タイル位置を再取得

		TileImage_pixel_to_tile_nojudge(p, x, y, &tx, &ty);

		pptile = TILEIMAGE_GETTILE_BUFPT(p, tx, ty);

		//作業用イメージのリサイズ

		if(!__TileImage_resizeTileBuf_clone(img_save, p))
			return FALSE;

		if(g_tileimage_dinfo.img_brush_stroke
			&& !__TileImage_resizeTileBuf_clone(g_tileimage_dinfo.img_brush_stroke, p))
			return FALSE;
	}

	//描画先にタイルがない場合、確保

	if(!(*pptile))
	{
		*pptile = TileImage_allocTile(p, TRUE);
		if(!(*pptile)) return FALSE;

		bEmptyTile = TRUE;  //描画前は空のタイル
	}

	//imgsave に元イメージをコピー

	pptmp = TILEIMAGE_GETTILE_BUFPT(img_save, tx, ty);

	if(!(*pptmp))
	{
		if(bEmptyTile)
			//元が空の場合 1 をセット
			*pptmp = TILEIMAGE_TILE_EMPTY;
		else
		{
			//確保＆コピー

			*pptmp = TileImage_allocTile(img_save, FALSE);
			if(!(*pptmp)) return FALSE;
		
			memcpy(*pptmp, *pptile, p->tilesize);
		}
	}

	//描画先タイル情報

	info->pptile = pptile;
	info->tx = tx;
	info->ty = ty;

	return TRUE;
}

/** 結果の色をセット
 *
 * @return 0:OK 1:色が変化しない -1:エラー */

static int _setpixeldraw_setcolor(TileImage *p,int x,int y,
	RGBAFix15 *pixres,_setpixelinfo *info)
{
	uint8_t *buf;
	RGBAFix15 pix;

	pix = *pixres;

	//レイヤマスク適用

	if(pix.a > info->max_alpha)
		pix.a = info->max_alpha;

	//アルファマスク (アルファ値維持)

	if(g_tileimage_dinfo.alphamask_type == 1)
		pix.a = info->pixdst.a;

	//描画前と描画後が同じなら色をセットしない

	if((g_tileimage_funcs[p->coltype].isSameColor)(&pix, &info->pixdst))
		return 1;

	//タイル処理

	if(!_setpixeldraw_tile(p, x, y, info))
	{
		g_tileimage_dinfo.err = 1;
		return -1;
	}

	//描画先に色セット

	buf = (g_tileimage_funcs[p->coltype].getPixelBufAtTile)(p, *(info->pptile), x, y);

	(g_tileimage_funcs[p->coltype].setPixel)(p, buf, x, y, &pix);

	//描画範囲

	mRectIncPoint(&g_tileimage_dinfo.rcdraw, x, y);

	return 0;
}


//================================
// 描画用、色のセット
//================================
/*
 * タイル配列内でタイルがない場合は作成される。
 * タイル配列外の場合は、キャンバスイメージ範囲内なら配列がリサイズされる。
 *
 * img_save : 描画前の元イメージ保存先 (アンドゥ用)。
 *            元のタイルが空の場合は TILEIMAGE_TILE_EMPTY がポインタにセットされる。
 */


/** 直接描画で色セット */

void TileImage_setPixel_draw_direct(TileImage *p,int x,int y,RGBAFix15 *pixdraw)
{
	RGBAFix15 pixres,pixsrc;
	_setpixelinfo info;
	_colparam param;

	//描画先の情報取得 + マスク処理

	pixsrc = *pixdraw;

	if(_setpixeldraw_dstpixel(p, x, y, &pixsrc, &info))
		return;

	//色処理
	/* funcColor がぼかしの場合は _colparam が必要 */

	param.ptile = (info.pptile)? *info.pptile: NULL;
	param.x = x;
	param.y = y;

	pixres = info.pixdst;

	(g_tileimage_dinfo.funcColor)(p, &pixres, &pixsrc, &param);

	//色セット

	_setpixeldraw_setcolor(p, x, y, &pixres, &info);
}

/** ストローク重ね塗り描画 (ドットペン用)
 *
 * ドットペンは描画濃度が一定なので、作業用イメージは必要ない。 */

void TileImage_setPixel_draw_dot_stroke(TileImage *p,int x,int y,RGBAFix15 *pixdraw)
{
	RGBAFix15 pixres,pixsrc;
	_setpixelinfo info;

	//描画先の情報取得 + マスク処理

	pixsrc = *pixdraw;

	if(_setpixeldraw_dstpixel(p, x, y, &pixsrc, &info))
		return;

	//描画前の色を取得

	_setpixeldraw_get_save_color(p, x, y, &info, &pixres);

	//描画前+描画色で色処理

	(g_tileimage_dinfo.funcColor)(p, &pixres, &pixsrc, NULL);

	//色セット

	_setpixeldraw_setcolor(p, x, y, &pixres, &info);
}

/** ストローク重ね塗り描画 (ブラシ用)
 *
 * ストローク中の最大濃度を記録するための作業用イメージが必要。 */

void TileImage_setPixel_draw_brush_stroke(TileImage *p,int x,int y,RGBAFix15 *pixdraw)
{
	RGBAFix15 pixres,pixsrc;
	_setpixelinfo info;
	TileImage *img_stroke = g_tileimage_dinfo.img_brush_stroke;
	uint8_t **pptile,*buf;
	int stroke_a;
	mBool write_stroke_img;

	//描画先の情報取得 + マスク処理

	pixsrc = *pixdraw;

	if(_setpixeldraw_dstpixel(p, x, y, &pixsrc, &info))
		return;

	//濃度イメージから、ストローク中の現在濃度を取得

	pptile = NULL;
	buf = NULL;
	stroke_a = 0;

	if(info.pptile)
	{
		pptile = TILEIMAGE_GETTILE_BUFPT(img_stroke, info.tx, info.ty);

		if(*pptile)
		{
			buf = (g_tileimage_funcs[TILEIMAGE_COLTYPE_ALPHA16].getPixelBufAtTile)(img_stroke, *pptile, x, y);
			stroke_a = *((uint16_t *)buf);
		}
	}

	//濃度の大きい方で描画

	if(pixsrc.a > stroke_a)
		write_stroke_img = TRUE;
	else
	{
		pixsrc.a = stroke_a;

		write_stroke_img = FALSE;
	}

	//描画前の色を取得

	_setpixeldraw_get_save_color(p, x, y, &info, &pixres);

	//色処理

	(g_tileimage_dinfo.funcColor)(p, &pixres, &pixsrc, NULL);

	//色セット
	/* [!]ここでタイル配列がリサイズされる場合がある */

	if(_setpixeldraw_setcolor(p, x, y, &pixres, &info) == 0)
	{
		//ストローク濃度イメージに描画

		/* - _setpixeldraw_setcolor() を実行する前はタイル配列外の場合があり、
		 *   配列リサイズが必要なので、描画はここで行う。
		 * - 描画後に色が変化しない場合はタイル配列リサイズが行われないので、
		 *   この処理はしない。 */

		if(write_stroke_img && !g_tileimage_dinfo.err)
		{
			//配列がリサイズされたらタイルを再取得
			
			if(!pptile)
				pptile = TILEIMAGE_GETTILE_BUFPT(img_stroke, info.tx, info.ty);

			//タイル未確保なら作成

			if(!buf)
			{
				*pptile = TileImage_allocTile(img_stroke, TRUE);
				if(!(*pptile))
				{
					g_tileimage_dinfo.err = 1;
					return;
				}

				buf = (g_tileimage_funcs[TILEIMAGE_COLTYPE_ALPHA16].getPixelBufAtTile)(img_stroke, *pptile, x, y);
			}

			//セット

			*((uint16_t *)buf) = pixsrc.a;
		}
	}
}

/** ドット用の形状で描画 (指先) */

void TileImage_setPixel_draw_dotstyle_direct(TileImage *p,int x,int y,RGBAFix15 *pix)
{
	TileImageWorkData *work = &g_tileimage_work;
	uint8_t *ps,f,val;
	int size,ix,iy,xx,yy,ct;

	ps = work->dotstyle;
	size = work->dot_size;

	ct = size >> 1;

	//

	yy = y - ct;

	for(iy = 0, f = 0x80; iy < size; iy++, yy++)
	{
		xx = x - ct;

		work->drawsuby = iy;
	
		for(ix = 0; ix < size; ix++, xx++)
		{
			if(f == 0x80) val = *(ps++);

			if(val & f)
			{
				work->drawsubx = ix;
			
				TileImage_setPixel_draw_direct(p, xx, yy, pix);
			}

			f >>= 1;
			if(!f) f = 0x80;
		}
	}
}

/** 元の色に指定値を足して描画 (フィルタ用) */

void TileImage_setPixel_draw_addsub(TileImage *p,int x,int y,int v)
{
	RGBAFix15 pix;
	int i,n;

	TileImage_getPixel(p, x, y, &pix);

	if(pix.a)
	{
		for(i = 0; i < 3; i++)
		{
			n = pix.c[i] + v;
			if(n < 0) n = 0;
			else if(n > 0x8000) n = 0x8000;

			pix.c[i] = n;
		}

		TileImage_setPixel_draw_direct(p, x, y, &pix);
	}
}


//================================
// 色のセット
//================================


/** 色をセット (タイル作成。透明の場合もセット) */

void TileImage_setPixel_new(TileImage *p,int x,int y,RGBAFix15 *pix)
{
	uint8_t *buf;

	if((buf = __TileImage_getPixelBuf_new(p, x, y)))
		(g_tileimage_funcs[p->coltype].setPixel)(p, buf, x, y, pix);
}

/** 色をセット (タイル作成。透明の場合はセットしない) */

void TileImage_setPixel_new_notp(TileImage *p,int x,int y,RGBAFix15 *pix)
{
	if(pix->a)
	{
		uint8_t *buf;

		if((buf = __TileImage_getPixelBuf_new(p, x, y)))
			(g_tileimage_funcs[p->coltype].setPixel)(p, buf, x, y, pix);
	}
}

/** 色をセット
 *
 * タイルは作成するが、配列拡張は行わない。描画範囲の計算あり。
 * 透明の場合もセット。 */

void TileImage_setPixel_new_drawrect(TileImage *p,int x,int y,RGBAFix15 *pix)
{
	uint8_t *buf;

	if((buf = __TileImage_getPixelBuf_new(p, x, y)))
	{
		(g_tileimage_funcs[p->coltype].setPixel)(p, buf, x, y, pix);

		mRectIncPoint(&g_tileimage_dinfo.rcdraw, x, y);
	}
}

/** 色をセット
 *
 * 色計算あり。配列拡張なし。 */

void TileImage_setPixel_new_colfunc(TileImage *p,int x,int y,RGBAFix15 *pixdraw)
{
	RGBAFix15 pix,pixdst;
	uint8_t *buf;

	//描画後の色

	TileImage_getPixel(p, x, y, &pixdst);

	pix = pixdst;

	(g_tileimage_dinfo.funcColor)(p, &pix, pixdraw, NULL);

	//色が変わらない

	if((g_tileimage_funcs[p->coltype].isSameColor)(&pix, &pixdst))
		return;

	//セット
	
	if((buf = __TileImage_getPixelBuf_new(p, x, y)))
		(g_tileimage_funcs[p->coltype].setPixel)(p, buf, x, y, &pix);
}

/** 色をセット (作業用イメージなどへの描画時。マスクなどはなし)
 *
 * [ 配列拡張、色計算、範囲セット ] */

void TileImage_setPixel_subdraw(TileImage *p,int x,int y,RGBAFix15 *pixdraw)
{
	RGBAFix15 pixdst,pixres;
	int tx,ty;
	uint8_t **pptile,*buf;

	//描画後の色

	TileImage_getPixel(p, x, y, &pixdst);

	pixres = pixdst;

	(g_tileimage_dinfo.funcColor)(p, &pixres, pixdraw, NULL);

	//色が変わらない

	if((g_tileimage_funcs[p->coltype].isSameColor)(&pixres, &pixdst))
		return;

	//タイル位置

	if(!TileImage_pixel_to_tile(p, x, y, &tx, &ty))
	{
		//キャンバス範囲外なら何もしない

		if(x < 0 || y < 0 || x >= g_tileimage_dinfo.imgw || y >= g_tileimage_dinfo.imgh)
			return;

		//配列リサイズ & タイル位置再計算

		if(!TileImage_resizeTileBuf_includeImage(p))
			return;

		TileImage_pixel_to_tile(p, x, y, &tx, &ty);
	}

	//タイル確保

	pptile = TILEIMAGE_GETTILE_BUFPT(p, tx, ty);

	if(!(*pptile))
	{
		*pptile = TileImage_allocTile(p, TRUE);
		if(!(*pptile)) return;
	}

	//セット

	buf = (g_tileimage_funcs[p->coltype].getPixelBufAtTile)(p, *pptile, x, y);

	(g_tileimage_funcs[p->coltype].setPixel)(p, buf, x, y, &pixres);

	//範囲

	mRectIncPoint(&g_tileimage_dinfo.rcdraw, x, y);
}


//================================
// 取得
//================================


/** 指定 px のバッファ位置取得
 *
 * その位置にタイルがない場合は作成する。ただし、タイルバッファの拡張は行わない。
 *
 * @return 範囲外、または確保失敗で NULL */

uint8_t *__TileImage_getPixelBuf_new(TileImage *p,int x,int y)
{
	int tx,ty;
	uint8_t **pp;

	//タイル位置

	if(!TileImage_pixel_to_tile(p, x, y, &tx, &ty))
		return NULL;

	//タイルがない場合は確保

	pp = TILEIMAGE_GETTILE_BUFPT(p, tx, ty);

	if(!(*pp))
	{
		*pp = TileImage_allocTile(p, TRUE);
		if(!(*pp)) return NULL;
	}

	//バッファ位置

	return(g_tileimage_funcs[p->coltype].getPixelBufAtTile)(p, *pp, x, y);
}

/** 指定位置の色が不透明か (A=0 でないか) */

mBool TileImage_isPixelOpaque(TileImage *p,int x,int y)
{
	RGBAFix15 pix;

	TileImage_getPixel(p, x, y, &pix);

	return (pix.a != 0);
}

/** 指定位置の色が透明か */

mBool TileImage_isPixelTransparent(TileImage *p,int x,int y)
{
	RGBAFix15 pix;

	TileImage_getPixel(p, x, y, &pix);

	return (pix.a == 0);
}

/** 指定位置の色を取得 (範囲外は透明) */

void TileImage_getPixel(TileImage *p,int x,int y,RGBAFix15 *pix)
{
	int tx,ty;
	uint8_t *ptile;

	if(!TileImage_pixel_to_tile(p, x, y, &tx, &ty))
		pix->v64 = 0;
	else
	{
		ptile = TILEIMAGE_GETTILE_PT(p, tx, ty);

		if(ptile)
			(g_tileimage_funcs[p->coltype].getPixelColAtTile)(p, ptile, x, y, pix);
		else
			pix->v64 = 0;
	}
}

/** 指定位置の色を取得 (イメージ範囲外はクリッピング) */

void TileImage_getPixel_clip(TileImage *p,int x,int y,RGBAFix15 *pix)
{
	if(x < 0) x = 0;
	else if(x >= g_tileimage_dinfo.imgw) x = g_tileimage_dinfo.imgw - 1;

	if(y < 0) y = 0;
	else if(y >= g_tileimage_dinfo.imgh) y = g_tileimage_dinfo.imgh - 1;

	TileImage_getPixel(p, x, y, pix);
}

/** pix に対して色合成を行った色を取得 */

void TileImage_getBlendPixel(TileImage *p,int x,int y,RGBFix15 *pix,
	int opacity,int blendmode,ImageBuf8 *imgtex)
{
	RGBAFix15 pixsrc;
	RGBFix15 src,dst;
	int a;

	TileImage_getPixel(p, x, y, &pixsrc);
	if(pixsrc.a == 0) return;

	//アルファ値

	a = pixsrc.a;

	if(imgtex)
		a = a * ImageBuf8_getPixel_forTexture(imgtex, x, y) / 255;

	a = a * opacity >> 7;
	
	if(a == 0) return;

	//色合成

	src.r = pixsrc.r;
	src.g = pixsrc.g;
	src.b = pixsrc.b;

	dst = *pix;

	(g_blendcolfuncs[blendmode])(&src, &dst);

	//アルファ合成

	pix->r = ((src.r - dst.r) * a >> 15) + dst.r;
	pix->g = ((src.g - dst.g) * a >> 15) + dst.g;
	pix->b = ((src.b - dst.b) * a >> 15) + dst.b;
}


//============================
// 色処理関数
//============================


/** 通常合成 */

void TileImage_colfunc_normal(TileImage *p,RGBAFix15 *dst,RGBAFix15 *src,void *param)
{
	double sa,da,na;
	int a;

	if(src->a == 0x8000)
	{
		*dst = *src;
		return;
	}

	sa = (double)src->a / 0x8000;
	da = (double)dst->a / 0x8000;

	//A

	na = sa + da - sa * da;
	a = (int)(na * 0x8000 + 0.5);

	if(a == 0)
		dst->v64 = 0;
	else
	{
		//RGB
		
		da = da * (1.0 - sa);
		na = 1.0 / na;

		dst->r = (int)((src->r * sa + dst->r * da) * na + 0.5);
		dst->g = (int)((src->g * sa + dst->g * da) * na + 0.5);
		dst->b = (int)((src->b * sa + dst->b * da) * na + 0.5);
		dst->a = a;
	}
}

/** アルファ値を比較して大きければ上書き */

void TileImage_colfunc_compareA(TileImage *p,RGBAFix15 *dst,RGBAFix15 *src,void *param)
{
	if(src->a > dst->a)
		*dst = *src;
	else if(dst->a)
	{
		//アルファ値はそのままで色だけ変える
	
		dst->r = src->r;
		dst->g = src->g;
		dst->b = src->b;
	}
}

/** 完全上書き */

void TileImage_colfunc_overwrite(TileImage *p,RGBAFix15 *dst,RGBAFix15 *src,void *param)
{
	*dst = *src;
}

/** 消しゴム */

void TileImage_colfunc_erase(TileImage *p,RGBAFix15 *dst,RGBAFix15 *src,void *param)
{
	int a;

	a = dst->a - src->a;
	if(a < 0) a = 0;

	if(a == 0)
		dst->v64 = 0;
	else
		dst->a = a;
}

/** 覆い焼き */

void TileImage_colfunc_dodge(TileImage *p,RGBAFix15 *dst,RGBAFix15 *src,void *param)
{
	int i,c;
	double d;

	if(src->a == 0x8000)
		//最大の場合は白
		dst->r = dst->g = dst->b = 0x8000;
	else
	{
		d = (double)0x8000 / (0x8000 - src->a);

		for(i = 0; i < 3; i++)
		{
			c = (int)(dst->c[i] * d + 0.5);
			if(c > 0x8000) c = 0x8000;

			dst->c[i] = c;
		}
	}
}

/** 焼き込み */

void TileImage_colfunc_burn(TileImage *p,RGBAFix15 *dst,RGBAFix15 *src,void *param)
{
	int i,c;
	double d;

	if(src->a == 0x8000)
		//最大の場合は黒
		dst->r = dst->g = dst->b = 0;
	else
	{
		d = (double)0x8000 / (0x8000 - src->a);

		for(i = 0; i < 3; i++)
		{
			c = round(0x8000 - (0x8000 - dst->c[i]) * d);
			if(c < 0) c = 0;

			dst->c[i] = c;
		}
	}
}

/** 加算 */

void TileImage_colfunc_add(TileImage *p,RGBAFix15 *dst,RGBAFix15 *src,void *param)
{
	int i,n,a;

	a = src->a;

	for(i = 0; i < 3; i++)
	{
		n = dst->c[i] + (src->c[i] * a >> 15);
		if(n > 0x8000) n = 0x8000;

		dst->c[i] = n;
	}
}

/** ぼかし
 *
 * 3x3 の周囲の色の平均値 */

void TileImage_colfunc_blur(TileImage *p,RGBAFix15 *dst,RGBAFix15 *src,void *param)
{
	int x,y,tx,ty,ix,iy;
	uint8_t *ptile;
	double rr,gg,bb,aa,dd;
	RGBAFix15 pix;

	x = ((_colparam *)param)->x;
	y = ((_colparam *)param)->y;
	ptile = ((_colparam *)param)->ptile;

	tx = (x - p->offx) & 63;
	ty = (y - p->offy) & 63;

	//総和

	rr = gg = bb = aa = 0;

	if(ptile && tx >= 1 && tx <= 62 && ty >= 1 && ty <= 62
		&& x >= 1 && x <= g_tileimage_dinfo.imgw - 2
		&& y >= 1 && y <= g_tileimage_dinfo.imgh - 2)
	{
		/* タイルが存在し、参照する 3x3 がすべてタイル内に収まっていて、
		 * かつ参照範囲がイメージ範囲内の場合 */

		for(iy = -1; iy <= 1; iy++)
		{
			for(ix = -1; ix <= 1; ix++)
			{
				(g_tileimage_funcs[p->coltype].getPixelColAtTile)(p, ptile, x + ix, y + iy, &pix);

				dd = (double)pix.a / 0x8000;

				rr += pix.r * dd;
				gg += pix.g * dd;
				bb += pix.b * dd;
				aa += dd;
			}
		}
	}
	else
	{
		//タイルが存在しない、または参照範囲が他タイルに及ぶ場合

		for(iy = -1; iy <= 1; iy++)
		{
			for(ix = -1; ix <= 1; ix++)
			{
				TileImage_getPixel_clip(p, x + ix, y + iy, &pix);

				dd = (double)pix.a / 0x8000;

				rr += pix.r * dd;
				gg += pix.g * dd;
				bb += pix.b * dd;
				aa += dd;
			}
		}
	}

	//平均値

	if(aa != 0)
	{
		dd = 1.0 / aa;

		dst->r = (uint16_t)(rr * dd + 0.5);
		dst->g = (uint16_t)(gg * dd + 0.5);
		dst->b = (uint16_t)(bb * dd + 0.5);
		dst->a = (uint16_t)(aa / 9.0 * 0x8000 + 0.5);
	}
}

/** 指先 */

void TileImage_colfunc_finger(TileImage *p,RGBAFix15 *dst,RGBAFix15 *src,void *param)
{
	TileImageWorkData *work = &g_tileimage_work;
	RGBAFix15 *buf;
	double sa,da,na,strength;
	int a,x,y;

	//バッファ内の前回の色

	buf = work->fingerbuf + work->drawsuby * work->dot_size + work->drawsubx;

	//A

	strength = (double)src->a / 0x8000;
	sa = (double)buf->a / 0x8000;
	da = (double)dst->a / 0x8000;

	na = da + (sa - da) * strength;
	a = (int)(na * 0x8000 + 0.5);

	if(a == 0)
		dst->v64 = 0;
	else
	{
		//RGB

		sa *= strength;
		da *= 1.0 - strength;
		na = 1.0 / na;

		dst->r = (int)((buf->r * sa + dst->r * da) * na + 0.5);
		dst->g = (int)((buf->g * sa + dst->g * da) * na + 0.5);
		dst->b = (int)((buf->b * sa + dst->b * da) * na + 0.5);
		dst->a = a;
	}

	//現在の色をバッファにセット
	/* 描画位置がイメージ範囲内ならそのままセット。
	 * 範囲外ならクリッピングした位置の色。 */

	x = ((_colparam *)param)->x;
	y = ((_colparam *)param)->y;

	if(x >= 0 && x < g_tileimage_dinfo.imgw
		&& y >= 0 && y < g_tileimage_dinfo.imgh)
		*buf = *dst;
	else
		TileImage_getPixel_clip(p, x, y, buf);
}

/** アルファ値を反転 (選択範囲反転時) */

void TileImage_colfunc_inverse_alpha(TileImage *p,RGBAFix15 *dst,RGBAFix15 *src,void *param)
{
	if(src->a)
		dst->a = 0x8000 - dst->a;
	else
		dst->v64 = 0;
}
