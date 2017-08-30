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
 **********************************/

#include <string.h>

#include "mDef.h"
#include "mRectBox.h"
#include "mPixbuf.h"

#include "defTileImage.h"

#include "TileImage.h"
#include "TileImage_pv.h"
#include "TileImage_coltype.h"
#include "TileImageDrawInfo.h"

#include "ImageBufRGB16.h"
#include "blendcol.h"


//--------------------------
/* グローバル変数 */

TileImageDrawInfo g_tileimage_dinfo;	//描画用情報
TileImageWorkData g_tileimage_work;		//作業用データ
TileImageColtypeFuncs g_tileimage_funcs[4];	//各カラータイプ毎の関数

//--------------------------

void __TileImage_initCurve();

//--------------------------



//==========================
// TileImageDrawInfo
//==========================


/** TileImageDrawInfo の rcdraw の範囲をクリア */

void TileImageDrawInfo_clearDrawRect()
{
	g_tileimage_dinfo.rcdraw.x1 = g_tileimage_dinfo.rcdraw.y1 = 1;
	g_tileimage_dinfo.rcdraw.x2 = g_tileimage_dinfo.rcdraw.y2 = 0;
}


//==========================
// 全体の初期化/解放
//==========================


/** 初期化 */

mBool TileImage_init()
{
	TileImageWorkData *work = &g_tileimage_work;
	int size;

	work->dot_size = 0;

	//作業用バッファ確保

	size = TILEIMAGE_DOTSTYLE_RADIUS_MAX * 2 + 1;
	size *= size;

	work->dotstyle = (uint8_t *)mMalloc((size + 7) / 8, FALSE);
	if(!work->dotstyle) return FALSE;

	work->fingerbuf = (RGBAFix15 *)mMalloc(size * 8, FALSE);
	if(!work->fingerbuf) return FALSE; 

	//曲線補間用パラメータ

	__TileImage_initCurve();

	//カラータイプ毎の関数テーブル

	__TileImage_RGBA_setFuncTable(g_tileimage_funcs + TILEIMAGE_COLTYPE_RGBA);
	__TileImage_Gray_setFuncTable(g_tileimage_funcs + TILEIMAGE_COLTYPE_GRAY);
	__TileImage_A16_setFuncTable(g_tileimage_funcs + TILEIMAGE_COLTYPE_ALPHA16);
	__TileImage_A1_setFuncTable(g_tileimage_funcs + TILEIMAGE_COLTYPE_ALPHA1);

	return TRUE;
}

/** 終了時 */

void TileImage_finish()
{
	mFree(g_tileimage_work.dotstyle);
	mFree(g_tileimage_work.fingerbuf);
}


//==========================
// 解放
//==========================


/** すべて解放 */

void TileImage_free(TileImage *p)
{
	uint8_t **pp;
	uint32_t i;

	if(p)
	{
		if(p->ppbuf)
		{
			pp = p->ppbuf;
			
			for(i = p->tilew * p->tileh; i; i--, pp++)
			{
				if(*pp && *pp != TILEIMAGE_TILE_EMPTY)
					mFree(*pp);
			}

			mFree(p->ppbuf);
		}

		mFree(p);
	}
}

/** 一つのタイルを解放 */

void TileImage_freeTile(uint8_t **pptile)
{
	if(*pptile)
	{
		if(*pptile != TILEIMAGE_TILE_EMPTY)
			mFree(*pptile);

		*pptile = NULL;
	}
}

/** 指定タイル位置のタイルを解放 (UNDO用) */

void TileImage_freeTile_atPos(TileImage *p,int tx,int ty)
{
	uint8_t **pp = TILEIMAGE_GETTILE_BUFPT(p, tx, ty);

	if(*pp)
		TileImage_freeTile(pp);
}

/** 指定px位置のタイルを解放 (UNDO用) */

void TileImage_freeTile_atPixel(TileImage *p,int x,int y)
{
	int tx,ty;

	if(TileImage_pixel_to_tile(p, x, y, &tx, &ty))
		TileImage_freeTile_atPos(p, tx, ty);
}

/** 全タイルとタイルバッファを解放 (TileImage 自体は残す) */

void TileImage_freeTileBuf(TileImage *p)
{
	if(p)
	{
		TileImage_freeAllTiles(p);

		mFree(p->ppbuf);
		p->ppbuf = NULL;
	}
}

/** タイルをすべて解放 (タイル配列は残す) */

void TileImage_freeAllTiles(TileImage *p)
{
	uint8_t **pp;
	int i;

	if(p && p->ppbuf)
	{
		pp = p->ppbuf;

		for(i = p->tilew * p->tileh; i; i--, pp++)
			TileImage_freeTile(pp);
	}
}

/** 空のタイルをすべて解放
 *
 * @return すべて空なら TRUE */

mBool TileImage_freeEmptyTiles(TileImage *p)
{
	uint8_t **ppbuf;
	uint32_t i;
	TileImageColFunc_isTransparentTile func;
	mBool empty = TRUE;

	if(p && p->ppbuf)
	{
		ppbuf = p->ppbuf;
		func = g_tileimage_funcs[p->coltype].isTransparentTile;
		
		for(i = p->tilew * p->tileh; i; i--, ppbuf++)
		{
			if(*ppbuf)
			{
				if((func)(*ppbuf))
					TileImage_freeTile(ppbuf);
				else
					empty = FALSE;
			}
		}
	}

	return empty;
}

/** 空のタイルをすべて解放 (アンドゥ側でタイルが確保されている部分のみ)
 *
 * アンドゥ用イメージでタイルが確保されている部分は、イメージが変更された部分。
 * それらの中で、消しゴム等ですべて透明になったタイルは解放して、メモリ負担を軽減させる。 */

void TileImage_freeEmptyTiles_byUndo(TileImage *p,TileImage *imgundo)
{
	uint8_t **ppbuf,**ppundo;
	uint32_t i;
	TileImageColFunc_isTransparentTile func = g_tileimage_funcs[p->coltype].isTransparentTile;

	if(imgundo)
	{
		ppbuf = p->ppbuf;
		ppundo = imgundo->ppbuf;

		for(i = p->tilew * p->tileh; i; i--, ppbuf++, ppundo++)
		{
			if(*ppundo && *ppbuf && (func)(*ppbuf))
				TileImage_freeTile(ppbuf);
		}
	}
}



//==========================
// 作成/複製
//==========================


/** 作成 (px サイズ指定) */

TileImage *TileImage_new(int type,int w,int h)
{
	return __TileImage_create(type, (w + 63) / 64, (h + 63) / 64);
}

/** 作成 (TileImageInfo から) */

TileImage *TileImage_newFromInfo(int type,TileImageInfo *info)
{
	TileImage *p;

	if(info->tilew <= 0 || info->tileh <= 0)
		return TileImage_new(type, 1, 1);
	else
	{
		p = __TileImage_create(type, info->tilew, info->tileh);
		if(p)
		{
			p->offx = info->offx;
			p->offy = info->offy;
		}

		return p;
	}
}

/** 作成 (ピクセル範囲から) */

TileImage *TileImage_newFromRect(int type,mRect *rc)
{
	TileImageInfo info;

	info.offx = rc->x1;
	info.offy = rc->y1;
	info.tilew = (rc->x2 - rc->x1 + 1 + 63) >> 6;
	info.tileh = (rc->y2 - rc->y1 + 1 + 63) >> 6;

	return TileImage_newFromInfo(type, &info);
}

/** 作成 (ピクセル範囲から。ファイル読み込み用)
 *
 * @param rc  x2,y2 は +1。x1 >= x2 or y1 >= y2 で空状態。 */

TileImage *TileImage_newFromRect_forFile(int type,mRect *rc)
{
	TileImageInfo info;

	if(rc->x1 >= rc->x2 && rc->y1 >= rc->y2)
	{
		//空
		
		info.offx = info.offy = 0;
		info.tilew = info.tileh = 1;
	}
	else
	{
		info.tilew = (rc->x2 - rc->x1) >> 6;
		info.tileh = (rc->y2 - rc->y1) >> 6;

		if(info.tilew <= 0) info.tilew = 1;
		if(info.tileh <= 0) info.tileh = 1;

		info.offx = rc->x1;
		info.offy = rc->y1;
	}

	return TileImage_newFromInfo(type, &info);
}

/** 複製を新規作成 (タイルイメージも含む) */

TileImage *TileImage_newClone(TileImage *src)
{
	TileImage *p;
	uint32_t i;
	uint8_t **ppsrc,**ppdst;

	//作成

	p = __TileImage_create(src->coltype, src->tilew, src->tileh);
	if(!p) return NULL;

	p->offx = src->offx;
	p->offy = src->offy;
	p->rgb  = src->rgb;

	//タイルコピー

	ppsrc = src->ppbuf;
	ppdst = p->ppbuf;

	for(i = p->tilew * p->tileh; i; i--, ppsrc++, ppdst++)
	{
		if(*ppsrc)
		{
			*ppdst = TileImage_allocTile(p, FALSE);
			if(!(*ppdst))
			{
				TileImage_free(p);
				return NULL;
			}

			memcpy(*ppdst, *ppsrc, p->tilesize);
		}
	}

	return p;
}


//==========================
//
//==========================


/** src と同じ構成になるように作成、または配列リサイズ
 *
 * 描画時の元イメージ保存用、またはストローク用イメージなどの作成に使う。
 * p が NULL なら新規作成。構成が同じならそのまま p が返る。
 * エラーなら解放されて NULL が返る。
 *
 * @param type カラータイプ。負の値で src と同じ */

TileImage *TileImage_createSame(TileImage *p,TileImage *src,int type)
{
	if(type < 0) type = src->coltype;

	if(!p)
	{
		//------ 新規作成
		
		TileImageInfo info;

		TileImage_getInfo(src, &info);

		return TileImage_newFromInfo(type, &info);
	}
	else
	{
		//------ 作成済み。構成が同じならそのまま返す

		//移動ツールで移動した場合があるので、常にコピー

		p->offx = src->offx;
		p->offy = src->offy;

		//構成が異なる

		if(p->coltype != type || p->tilew != src->tilew || p->tileh != src->tileh)
		{
			//タイプ

			p->coltype = type;

			__TileImage_setTileSize(p);
		
			//配列再作成
			
			TileImage_freeTileBuf(p);

			p->tilew = src->tilew;
			p->tileh = src->tileh;

			if(!__TileImage_allocTileBuf(p))
			{
				TileImage_free(p);
				p = NULL;
			}
		}

		return p;
	}
}

/** イメージをクリアする
 *
 * タイルを削除して、タイル配列を 1x1 にする。 */

void TileImage_clear(TileImage *p)
{
	TileImage_freeTileBuf(p);

	p->offx = p->offy = 0;
	p->tilew = p->tileh = 1;

	__TileImage_allocTileBuf(p);
}


//===============================
// タイル配列
//===============================


/** タイル配列をイメージサイズから再作成 */

mBool TileImage_reallocTileBuf_fromImageSize(TileImage *p,int w,int h)
{
	TileImage_freeTileBuf(p);

	p->offx = p->offy = 0;
	p->tilew = (w + 63) >> 6;
	p->tileh = (h + 63) >> 6;

	return __TileImage_allocTileBuf(p);
}

/** タイル配列リサイズ (現在の画像サイズ全体を含むように) */

mBool TileImage_resizeTileBuf_includeImage(TileImage *p)
{
	mRect rc;

	//描画可能な範囲

	TileImage_getCanDrawRect_pixel(p, &rc);

	//px -> タイル位置

	TileImage_pixel_to_tile_rect(p, &rc);

	//リサイズ

	return __TileImage_resizeTileBuf(p, rc.x1, rc.y1,
			rc.x2 - rc.x1 + 1, rc.y2 - rc.y1 + 1);
}

/** 2つのイメージを結合した範囲でタイル配列リサイズ */

mBool TileImage_resizeTileBuf_combine(TileImage *p,TileImage *src)
{
	mRect rc,rc2;
	int tx1,ty1,tx2,ty2;

	//px 範囲を結合

	TileImage_getAllTilesPixelRect(p, &rc);
	TileImage_getAllTilesPixelRect(src, &rc2);

	mRectUnion(&rc, &rc2);

	//px 範囲 -> p におけるタイル位置

	TileImage_pixel_to_tile_nojudge(p, rc.x1, rc.y1, &tx1, &ty1);
	TileImage_pixel_to_tile_nojudge(p, rc.x2, rc.y2, &tx2, &ty2);

	return __TileImage_resizeTileBuf(p, tx1, ty1, tx2 - tx1 + 1, ty2 - ty1 + 1);
}

/** タイル配列リサイズ (UNDO用) */

mBool TileImage_resizeTileBuf_forUndo(TileImage *p,TileImageInfo *info)
{
	uint8_t **ppnew,**ppd;
	int ttx,tty,ix,iy,tx,ty,sw,sh;

	//配列は変更なし

	if(info->tilew == p->tilew && info->tileh == p->tileh)
		return TRUE;

	//新規確保

	ppnew = __TileImage_allocTileBuf_new(info->tilew, info->tileh, TRUE);
	if(!ppnew) return FALSE;

	//データコピー

	ppd = ppnew;
	ttx = (info->offx - p->offx) >> 6;
	tty = (info->offy - p->offy) >> 6;
	sw = p->tilew;
	sh = p->tileh;

	for(iy = info->tileh, ty = tty; iy > 0; iy--, ty++)
	{
		for(ix = info->tilew, tx = ttx; ix > 0; ix--, tx++, ppd++)
		{
			if(tx >= 0 && tx < sw && ty >= 0 && ty < sh)
				*ppd = *(p->ppbuf + ty * sw + tx);
		}
	}

	//入れ替え

	mFree(p->ppbuf);

	p->ppbuf = ppnew;
	p->tilew = info->tilew;
	p->tileh = info->tileh;
	p->offx  = info->offx;
	p->offy  = info->offy;

	return TRUE;
}


//==========================
// タイル
//==========================


/** 一つのタイルを確保 */

uint8_t *TileImage_allocTile(TileImage *p,mBool clear)
{
	return (uint8_t *)mMalloc(p->tilesize, clear);
}

/** ポインタの位置にタイルがなければ確保
 *
 * @return FALSE で、タイルの新規確保に失敗 */

mBool TileImage_allocTile_atptr(TileImage *p,uint8_t **ppbuf,mBool clear)
{
	if(!(*ppbuf))
	{
		*ppbuf = TileImage_allocTile(p, clear);
		if(!(*ppbuf)) return FALSE;
	}

	return TRUE;
}

/** 指定タイル位置のタイルを取得 (タイルが確保されていなければ確保する)
 *
 * @return タイルポインタ。位置が範囲外や、確保失敗の場合は NULL */

uint8_t *TileImage_getTileAlloc_atpos(TileImage *p,int tx,int ty,mBool clear)
{
	uint8_t **pp;

	if(tx < 0 || ty < 0 || tx >= p->tilew || ty >= p->tileh)
		return NULL;
	else
	{
		pp = TILEIMAGE_GETTILE_BUFPT(p, tx, ty);

		if(!(*pp))
			*pp = TileImage_allocTile(p, clear);

		return *pp;
	}
}


//==========================
// セット/変更
//==========================


/** オフセット位置をセット */

void TileImage_setOffset(TileImage *p,int offx,int offy)
{
	p->offx = offx;
	p->offy = offy;
}

/** オフセット位置を相対移動 */

void TileImage_moveOffset_rel(TileImage *p,int x,int y)
{
	p->offx += x;
	p->offy += y;
}

/** A16/1 時の線の色セット */

void TileImage_setImageColor(TileImage *p,uint32_t col)
{
	RGBtoRGBFix15(&p->rgb, col);
}


//==========================
// 計算など
//==========================


/** px 位置からタイル位置を取得
 *
 * @return FALSE で範囲外 */

mBool TileImage_pixel_to_tile(TileImage *p,int x,int y,int *tilex,int *tiley)
{
	x = (x - p->offx) >> 6;
	y = (y - p->offy) >> 6;

	*tilex = x;
	*tiley = y;

	return (x >= 0 && x < p->tilew && y >= 0 && y < p->tileh);
}

/** px 位置からタイル位置を取得 (範囲外判定を行わない) */

void TileImage_pixel_to_tile_nojudge(TileImage *p,int x,int y,int *tilex,int *tiley)
{
	*tilex = (x - p->offx) >> 6;
	*tiley = (y - p->offy) >> 6;
}

/** px -> タイル位置 (mRect) */

void TileImage_pixel_to_tile_rect(TileImage *p,mRect *rc)
{
	TileImage_pixel_to_tile_nojudge(p, rc->x1, rc->y1, &rc->x1, &rc->y1);
	TileImage_pixel_to_tile_nojudge(p, rc->x2, rc->y2, &rc->x2, &rc->y2);
}

/** タイル位置から左上の px 位置取得 */

void TileImage_tile_to_pixel(TileImage *p,int tx,int ty,int *px,int *py)
{
	*px = (tx << 6) + p->offx;
	*py = (ty << 6) + p->offy;
}

/** すべてのタイルの範囲を px で取得 */

void TileImage_getAllTilesPixelRect(TileImage *p,mRect *rc)
{
	int x1,y1,x2,y2;

	TileImage_tile_to_pixel(p, 0, 0, &x1, &y1);
	TileImage_tile_to_pixel(p, p->tilew - 1, p->tileh - 1, &x2, &y2);

	rc->x1 = x1, rc->y1 = y1;
	rc->x2 = x2 + 63, rc->y2 = y2 + 63;
}


//======================
// 情報取得
//======================


/** オフセット位置を取得 */

void TileImage_getOffset(TileImage *p,mPoint *pt)
{
	pt->x = p->offx;
	pt->y = p->offy;
}

/** イメージ情報取得 */

void TileImage_getInfo(TileImage *p,TileImageInfo *info)
{
	info->tilew = p->tilew;
	info->tileh = p->tileh;
	info->offx  = p->offx;
	info->offy  = p->offy;
}

/** 現在描画可能な範囲を px で取得
 *
 * タイル全体とキャンバス範囲を含む。 */

void TileImage_getCanDrawRect_pixel(TileImage *p,mRect *rc)
{
	mRect rc1,rc2;

	TileImage_getAllTilesPixelRect(p, &rc1);

	rc2.x1 = rc2.y1 = 0;
	rc2.x2 = g_tileimage_dinfo.imgw - 1;
	rc2.y2 = g_tileimage_dinfo.imgh - 1;

	//範囲を合成

	mRectUnion(&rc1, &rc2);

	*rc = rc1;
}

/** 指定範囲 (px) 全体を含むタイル範囲を取得
 *
 * @param rc  タイル範囲が入る
 * @param box イメージの範囲 (px)
 * @return FALSE で全体がタイル範囲外 */

mBool TileImage_getTileRect_fromPixelBox(TileImage *p,mRect *rc,mBox *box)
{
	int tx1,ty1,tx2,ty2,f;

	//box 四隅のタイル位置

	TileImage_pixel_to_tile_nojudge(p, box->x, box->y, &tx1, &ty1);
	TileImage_pixel_to_tile_nojudge(p, box->x + box->w - 1, box->y + box->h - 1, &tx2, &ty2);

	//各位置が範囲外かどうかのフラグ

	f = (tx1 < 0) | ((tx2 < 0) << 1)
		| ((tx1 >= p->tilew) << 2) | ((tx2 >= p->tilew) << 3)
		| ((ty1 < 0) << 4) | ((ty2 < 0) << 5)
		| ((ty1 >= p->tileh) << 6) | ((ty2 >= p->tileh) << 7);

	//全体が範囲外か

	if((f & 0x03) == 0x03 || (f & 0x30) == 0x30
		|| (f & 0x0c) == 0x0c || (f & 0xc0) == 0xc0)
		return FALSE;

	//調整

	if(f & 1) tx1 = 0;
	if(f & 8) tx2 = p->tilew - 1;
	if(f & 0x10) ty1 = 0;
	if(f & 0x80) ty2 = p->tileh - 1;

	//セット

	rc->x1 = tx1, rc->y1 = ty1;
	rc->x2 = tx2, rc->y2 = ty2;

	return TRUE;
}

/** box の範囲 (px) を含むタイルを処理する際の情報を取得
 *
 * @return タイルの開始位置のポインタ (NULL で範囲外) */

uint8_t **TileImage_getTileRectInfo(TileImage *p,TileImageTileRectInfo *info,mBox *box)
{
	//タイル範囲

	if(!TileImage_getTileRect_fromPixelBox(p, &info->rctile, box))
		return NULL;

	//左上タイルの px 位置

	TileImage_tile_to_pixel(p,
		info->rctile.x1, info->rctile.y1, &info->pxtop.x, &info->pxtop.y);

	//box を mRect へ (クリッピング用)

	info->rcclip.x1 = box->x;
	info->rcclip.y1 = box->y;
	info->rcclip.x2 = box->x + box->w;
	info->rcclip.y2 = box->y + box->h;

	info->pitch = p->tilew - (info->rctile.x2 - info->rctile.x1 + 1);

	//タイル位置

	return TILEIMAGE_GETTILE_BUFPT(p, info->rctile.x1, info->rctile.y1);
}

/** イメージが存在する部分の px 範囲取得
 *
 * 透明な部分は除く。キャンバスの範囲外部分も含む。
 *
 * @param tilenum  確保されているタイル数が入る。NULL でなし
 * @return FALSE で範囲なし */

mBool TileImage_getHaveImageRect_pixel(TileImage *p,mRect *rcdst,int *tilenum)
{
	uint8_t **pp;
	mRect rc;
	int x,y,num = 0;

	mRectEmpty(&rc);

	//確保されているタイルの最小/最大位置

	if(p)
	{
		pp = p->ppbuf;

		for(y = 0; y < p->tileh; y++)
		{
			for(x = 0; x < p->tilew; x++, pp++)
			{
				if(*pp)
				{
					mRectIncPoint(&rc, x, y);
					num++;
				}
			}
		}
	}

	if(tilenum) *tilenum = num;

	//タイル位置 -> px

	if(mRectIsEmpty(&rc))
	{
		*rcdst = rc;
		return FALSE;
	}
	else
	{
		TileImage_tile_to_pixel(p, 0, 0, &x, &y);

		rcdst->x1 = x + (rc.x1 << 6);
		rcdst->y1 = y + (rc.y1 << 6);
		rcdst->x2 = x + (rc.x2 << 6) + 63;
		rcdst->y2 = y + (rc.y2 << 6) + 63;
		
		return TRUE;
	}
}

/** 確保されているタイルの数を取得
 *
 * APD v3 書き込み時 */

int TileImage_getHaveTileNum(TileImage *p)
{
	uint8_t **pp;
	int i,num = 0;

	pp = p->ppbuf;

	for(i = p->tilew * p->tileh; i; i--, pp++)
	{
		if(*pp) num++;
	}

	return num;
}

/** rc を描画可能な範囲内にクリッピング
 *
 * @return FALSE で全体が範囲外 */

mBool TileImage_clipCanDrawRect(TileImage *p,mRect *rc)
{
	mRect rc1;

	TileImage_getCanDrawRect_pixel(p, &rc1);

	//範囲外

	if(rc->x1 > rc1.x2 || rc->y1 > rc1.y2
		|| rc->x2 < rc1.x1 || rc->y2 < rc1.y1)
		return FALSE;

	//調整

	if(rc->x1 < rc1.x1) rc->x1 = rc1.x1;
	if(rc->y1 < rc1.y1) rc->y1 = rc1.y1;
	if(rc->x2 > rc1.x2) rc->x2 = rc1.x2;
	if(rc->y2 > rc1.y2) rc->y2 = rc1.y2;

	return TRUE;
}


//=============================
// いろいろ
//=============================


/** ImageBufRGB16 に合成 */

void TileImage_blendToImageRGB16(TileImage *p,ImageBufRGB16 *dst,
	mBox *boxdst,int opacity,int blendmode,ImageBuf8 *imgtex)
{
	TileImageTileRectInfo info;
	TileImageBlendInfo binfo;
	uint8_t **pp;
	int ix,iy,px,py,tw;
	TileImageColFunc_blendTile func = g_tileimage_funcs[p->coltype].blendTile;

	if(opacity == 0) return;

	if(!(pp = TileImage_getTileRectInfo(p, &info, boxdst)))
		return;

	binfo.opacity = opacity;
	binfo.funcBlend = g_blendcolfuncs[blendmode];
	binfo.imgtex = imgtex;

	//タイル毎に合成

	tw = info.rctile.x2 - info.rctile.x1 + 1;
	iy = info.rctile.y2 - info.rctile.y1 + 1;

	for(py = info.pxtop.y; iy; iy--, py += 64, pp += info.pitch)
	{
		for(ix = tw, px = info.pxtop.x; ix; ix--, px += 64, pp++)
		{
			if(!(*pp)) continue;

			__TileImage_setBlendInfo(&binfo, px, py, &info.rcclip);

			binfo.tile = *pp;
			binfo.dst = dst->buf + binfo.dy * dst->width + binfo.dx;
			binfo.pitch_dst = dst->width - binfo.w;

			(func)(p, &binfo);
		}
	}
}

/** A1 タイプイメージを mPixbuf に XOR 合成 */

void TileImage_blendXorToPixbuf(TileImage *p,mPixbuf *pixbuf,mBox *boxdst)
{
	TileImageTileRectInfo info;
	TileImageBlendInfo binfo;
	uint8_t **pptile;
	int ix,iy,px,py,tw;
	mBox box;

	if(!mPixbufGetClipBox_box(pixbuf, &box, boxdst)) return;

	//描画先に相当するタイル範囲

	if(!(pptile = TileImage_getTileRectInfo(p, &info, &box)))
		return;

	//タイル毎に合成

	tw = info.rctile.x2 - info.rctile.x1 + 1;
	iy = info.rctile.y2 - info.rctile.y1 + 1;

	for(py = info.pxtop.y; iy; iy--, py += 64, pptile += info.pitch)
	{
		for(ix = tw, px = info.pxtop.x; ix; ix--, px += 64, pptile++)
		{
			if(*pptile)
			{
				__TileImage_setBlendInfo(&binfo, px, py, &info.rcclip);

				binfo.tile = *pptile;

				__TileImage_A1_blendXorToPixbuf(p, pixbuf, &binfo);
			}
		}
	}
}

/** プレビューを mPixbuf に描画
 *
 * レイヤ一覧用。チェック柄を背景に。 */

void TileImage_drawPreview(TileImage *p,mPixbuf *pixbuf,
	int x,int y,int boxw,int boxh,int sw,int sh)
{
	double dscale,d;
	mBox box;
	uint8_t *pd,ckcol8[2] = {224,192};
	int ix,iy,pitchd,bpp,finc,finc2,fy,fx,fxleft,n,jx,jy,ff;
	int ytbl[3],xtbl[3],ckx,cky,rr,gg,bb,aa,ckcol15[2] = {28783,24671};
	uint32_t colex;
	RGBAFix15 pix;

	if(!mPixbufGetClipBox_d(pixbuf, &box, x, y, boxw, boxh))
		return;

	colex = mRGBtoPix(0xb0b0b0);

	//倍率 (倍率の低い方、拡大はしない)

	dscale = (double)boxw / sw;
	d = (double)boxh / sh;

	if(d < dscale) dscale = d;
	if(d > 1.0) d = 1.0;

	//

	pd = mPixbufGetBufPtFast(pixbuf, box.x, box.y);
	bpp = pixbuf->bpp;
	pitchd = pixbuf->pitch_dir - box.w * bpp;

	x = box.x - x;
	y = box.y - y;

	finc = (int)((1<<16) / dscale + 0.5);
	finc2 = finc / 3;

	fxleft = (int)(((x - boxw * 0.5) / dscale + sw * 0.5) * (1<<16));
	fy = (int)(((y - boxh * 0.5) / dscale + sh * 0.5) * (1<<16));

	cky = y & 7;

	//

	for(iy = box.h; iy; iy--, fy += finc, cky = (cky + 1) & 7)
	{
		n = fy >> 16;

		if(n < 0 || n >= sh)
		{
			mPixbufRawLineH(pixbuf, pd, box.w, colex);
			pd += pixbuf->pitch_dir;
			continue;
		}

		//Yテーブル

		for(jy = 0, ff = fy; jy < 3; jy++, ff += finc2)
		{
			n = ff >> 16;
			if(n >= sh) n = sh - 1;
			
			ytbl[jy] = n;
		}
	
		//----- X
	
		ckx = x & 7;

		for(ix = box.w, fx = fxleft; ix; ix--, fx += finc, pd += bpp, ckx = (ckx + 1) & 7)
		{
			n = fx >> 16;

			if(n < 0 || n >= sw)
			{
				(pixbuf->setbuf)(pd, colex);
				continue;
			}

			//Xテーブル

			for(jx = 0, ff = fx; jx < 3; jx++, ff += finc2)
			{
				n = ff >> 16;
				if(n >= sw) n = sw - 1;

				xtbl[jx] = n;
			}

			//オーバーサンプリング

			rr = gg = bb = aa = 0;

			for(jy = 0; jy < 3; jy++)
			{
				for(jx = 0; jx < 3; jx++)
				{
					TileImage_getPixel(p, xtbl[jx], ytbl[jy], &pix);

					if(pix.a)
					{
						rr += pix.r * pix.a >> 15;
						gg += pix.g * pix.a >> 15;
						bb += pix.b * pix.a >> 15;
						aa += pix.a;
					}
				}
			}

			//チェック柄と合成

			n = (ckx >> 2) ^ (cky >> 2);

			if(aa == 0)
				rr = gg = bb = ckcol8[n];
			else
			{
				rr = ((int64_t)rr << 15) / aa;
				gg = ((int64_t)gg << 15) / aa;
				bb = ((int64_t)bb << 15) / aa;
				aa /= 9;

				n = ckcol15[n];
				
				rr = (((rr - n) * aa >> 15) + n) * 255 >> 15;
				gg = (((gg - n) * aa >> 15) + n) * 255 >> 15;
				bb = (((bb - n) * aa >> 15) + n) * 255 >> 15;
			}
			
			(pixbuf->setbuf)(pd, mRGBtoPix2(rr,gg,bb));
		}

		pd += pitchd;
	}
}

/** フィルタプレビュー描画 (イメージとチェック柄背景を合成)
 *
 * @param box イメージ座標 */

void TileImage_drawFilterPreview(TileImage *p,mPixbuf *pixbuf,mBox *box)
{
	uint8_t *pd;
	int ix,iy,pitch,w,h,fy,bkgnd,r,g,b,plaidcol[2] = {28784,24672};
	RGBAFix15 pix;

	w = box->w, h = box->h;

	pd = mPixbufGetBufPtFast(pixbuf, 1, 1);
	pitch = pixbuf->pitch_dir - pixbuf->bpp * w;

	for(iy = 0; iy < h; iy++, pd += pitch)
	{
		fy = (iy >> 3) & 1;
		
		for(ix = 0; ix < w; ix++, pd += pixbuf->bpp)
		{
			TileImage_getPixel(p, box->x + ix, box->y + iy, &pix);

			bkgnd = plaidcol[fy ^ ((ix >> 3) & 1)];

			if(pix.a == 0)
				pix.r = pix.g = pix.b = bkgnd;
			else if(pix.a != 255)
			{
				pix.r = ((pix.r - bkgnd) * pix.a >> 15) + bkgnd;
				pix.g = ((pix.g - bkgnd) * pix.a >> 15) + bkgnd;
				pix.b = ((pix.b - bkgnd) * pix.a >> 15) + bkgnd;
			}

			r = RGBCONV_FIX15_TO_8(pix.r);
			g = RGBCONV_FIX15_TO_8(pix.g);
			b = RGBCONV_FIX15_TO_8(pix.b);

			(pixbuf->setbuf)(pd, mRGBtoPix2(r, g, b));
		}
	}
}

/** ヒストグラム取得
 *
 * (0-256 = 257個) x 4byte。
 * RGBA または GRAY のみ。 */

uint32_t *TileImage_getHistogram(TileImage *p)
{
	uint32_t *buf;
	uint8_t **pptile;
	uint32_t i,j;
	mBool type_rgba;
	RGBAFix15 *ps;
	uint16_t *ps16;

	buf = (uint32_t *)mMalloc(257 * 4, TRUE);
	if(!buf) return NULL;

	type_rgba = (p->coltype == TILEIMAGE_COLTYPE_RGBA);

	//タイルごと

	pptile = p->ppbuf;

	for(i = p->tilew * p->tileh; i; i--, pptile++)
	{
		if(!(*pptile)) continue;

		if(type_rgba)
		{
			//RGBA
			
			ps = (RGBAFix15 *)(*pptile);

			for(j = 64 * 64; j; j--, ps++)
			{
				if(ps->a)
					buf[(ps->r * 77 + ps->g * 150 + ps->b * 29) >> (8 + 15 - 8)]++;
			}
		}
		else
		{
			//GRAY

			ps16 = (uint16_t *)(*pptile);

			for(j = 64 * 64; j; j--, ps16 += 2)
			{
				if(ps16[1])
					buf[ps16[0] >> (15 - 8)]++;
			}
		}
	}

	return buf;
}
