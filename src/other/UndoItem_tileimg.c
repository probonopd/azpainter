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
 * UndoItem
 *
 * タイルイメージ
 *****************************************/

#include <stdio.h>	//FILE*
#include <string.h> //memmove

#include "mDef.h"
#include "mUndo.h"

#include "defConfig.h"
#include "defDraw.h"

#include "defTileImage.h"
#include "TileImage.h"
#include "TileImage_coltype.h"
#include "LayerItem.h"

#include "Undo.h"
#include "UndoMaster.h"
#include "UndoItem.h"


//--------------------

#define _UNDO   g_app_undo

#define _FLAGS_EMPTY		1	//空タイル
#define _FLAGS_FIRST_EMPTY  2	//最初の書き込み時、元が空タイル

//--------------------
/* undo_compress.c */

int UndoByteEncode(uint8_t *dst,uint8_t *src,int srcsize);
void UndoByteDecode(uint8_t *dst,uint8_t *src,int srcsize);

int UndoWordEncode(uint8_t *dst,uint8_t *src,int srcsize);
void UndoWordDecode(uint8_t *dst,uint8_t *src,int srcsize);

//--------------------

static void _get_writetile_A16(uint8_t *dstf,uint16_t *dsta,uint16_t *tilesrc,uint8_t *cmpbuf,uint16_t *dstsize,mBool first);
static void _restore_tile_A16(uint16_t *dst,uint8_t *flagsrc,uint16_t *asrc,int asize,int rgbsize);

static void _get_writetile_gray(uint8_t *dstf,uint16_t *dstcol,uint16_t *tilesrc,uint8_t *cmpbuf,uint16_t *dstsize,mBool first);
static void _restore_tile_gray(uint16_t *dst,uint8_t *flagsrc,uint16_t *colsrcbuf,int asize,int rgbsize);

static void _get_writetile_rgba(uint8_t *dstf,uint16_t *dstcol,uint16_t *tilesrc,uint8_t *cmpbuf,uint16_t *dstsize,mBool first);
static void _restore_tile_rgba(uint16_t *dst,uint8_t *flagsrc,uint16_t *colsrcbuf,int asize,int rgbsize);

//--------------------

typedef void (*gettilefunc)(uint8_t *,uint16_t *,uint16_t *,uint8_t *,uint16_t *,mBool);
typedef void (*restoretilefunc)(uint16_t *,uint8_t *,uint16_t *,int,int);

static gettilefunc g_gettilefunc[3] = {
	_get_writetile_rgba, _get_writetile_gray, _get_writetile_A16
};

static restoretilefunc g_restoretilefunc[3] = {
	_restore_tile_rgba, _restore_tile_gray, _restore_tile_A16
};

//--------------------



//==================================
// sub
//==================================


/** 対象のレイヤイメージ取得 */

static TileImage *_get_layerimg(UndoItem *p)
{
	LayerItem *item;

	item = UndoItem_getLayerFromNo(p->val[0]);

	return (item)? item->img: NULL;
}

/** バイト圧縮データ読み込み */

static mBool _read_byte_decode(UndoItem *p,uint8_t *dstbuf,int rawsize,uint8_t *readbuf,int encsize)
{
	if(encsize == rawsize)
	{
		//無圧縮

		return UndoItem_read(p, dstbuf, encsize);
	}
	else
	{
		//展開

		if(!UndoItem_read(p, readbuf, encsize))
			return FALSE;
		else
		{
			UndoByteDecode(dstbuf, readbuf, encsize);
			return TRUE;
		}
	}
}

/** 2byte圧縮データ読み込み */

static mBool _read_word_decode(UndoItem *p,uint8_t *dstbuf,int rawsize,uint8_t *readbuf,int encsize)
{
	if(encsize == rawsize)
	{
		//無圧縮

		return UndoItem_read(p, dstbuf, encsize);
	}
	else
	{
		//展開

		if(!UndoItem_read(p, readbuf, encsize))
			return FALSE;
		else
		{
			UndoWordDecode(dstbuf, readbuf, encsize);
			return TRUE;
		}
	}
}


//==================================
// タイル書き込み
//==================================
/*
 * ## A1
 * 
 * [2byte] 圧縮サイズ
 * [*] タイル圧縮データ
 * 
 * ## A16/GRAY/RGBA
 *
 * [2byte x 6] 情報
 *  0: フラグ圧縮サイズ
 *  1: 色データ圧縮サイズ
 *  2: フラグデータサイズ
 *  3: 色データ(全体)サイズ
 *  4: Aデータサイズ
 *  5: RGB一つのチャンネルのデータサイズ
 * [*] フラグ圧縮データ
 * [*] 色圧縮データ
 */


/** タイル初回書き込み */

static mBool _write_first_tile(UndoItem *p,int coltype,uint8_t *tilesrc,uint8_t *tiledst)
{
	uint8_t *srcbuf,*encbuf,*flagsrc,*flagdst;
	uint16_t wdbuf[6];

	srcbuf = _UNDO->workbuf1;
	encbuf = _UNDO->workbuf2;

	//

	if(coltype == TILEIMAGE_COLTYPE_ALPHA1)
	{
		//------- A1

		wdbuf[0] = UndoByteEncode(encbuf, tilesrc, 8 * 64);

		return (UndoItem_write(p, wdbuf, 2)
			&& UndoItem_write(p, (wdbuf[0] == 8 * 64)? tilesrc: encbuf, wdbuf[0]));
	}
	else
	{
		//------ A16/GRAY/RGBA

		flagsrc = _UNDO->workbuf_flags;
		flagdst = flagsrc + 2048;

		//書き込みデータ取得

		(g_gettilefunc[coltype])(flagsrc,
			(uint16_t *)srcbuf, (uint16_t *)tilesrc, tiledst, wdbuf + 2, TRUE);

		//圧縮

		wdbuf[0] = UndoByteEncode(flagdst, flagsrc, wdbuf[2]);
		wdbuf[1] = UndoWordEncode(encbuf, srcbuf, wdbuf[3]);

		//書き込み

		return (UndoItem_write(p, wdbuf, 12)
			&& UndoItem_write(p, (wdbuf[0] == wdbuf[2])? flagsrc: flagdst, wdbuf[0])
			&& UndoItem_write(p, (wdbuf[1] == wdbuf[3])? srcbuf: encbuf, wdbuf[1]));
	}
}

/** 反転書き込み (src タイル読み込みとタイル書き込み)
 *
 * @param tilesrc NULL の場合、書き込み対象タイルが空のため、src のスキップのみ行う
 * @param havetile src にタイルイメージデータが続いているか (FALSE で空タイル) */

static mBool _write_rev_tile(UndoItem *dst,UndoItem *src,int coltype,uint8_t *tilesrc,mBool havetile)
{
	uint8_t *srcbuf,*encbuf,*flagsrc,*flagdst;
	uint16_t wdbuf[6];

	srcbuf = _UNDO->workbuf1;
	encbuf = _UNDO->workbuf2;

	if(coltype == TILEIMAGE_COLTYPE_ALPHA1)
	{
		//----- A1

		//src データスキップ

		if(havetile)
		{
			if(!UndoItem_read(src, wdbuf, 2)) return FALSE;
	
			UndoItem_readSeek(src, wdbuf[0]);
		}

		//書き込み

		if(!tilesrc)
			return TRUE;
		else
		{
			wdbuf[0] = UndoByteEncode(encbuf, tilesrc, 8 * 64);

			return (UndoItem_write(dst, wdbuf, 2)
				&& UndoItem_write(dst, (wdbuf[0] == 8 * 64)? tilesrc: encbuf, wdbuf[0]));
		}
	}
	else
	{
		//----- A16/GRAY/RGBA

		flagsrc = _UNDO->workbuf_flags;
		flagdst = flagsrc + 2048;
		
		//src のデータ取得 (フラグデータが必要)

		if(havetile)
		{
			if(!UndoItem_read(src, wdbuf, 12)
				|| !_read_byte_decode(src, flagdst, wdbuf[2], flagsrc, wdbuf[0]))
				return FALSE;

			//色データスキップ
			UndoItem_readSeek(src, wdbuf[1]);
		}

		if(!tilesrc) return TRUE;

		//書き込みデータ取得

		(g_gettilefunc[coltype])(flagsrc, (uint16_t *)srcbuf, (uint16_t *)tilesrc,
			(havetile)? flagdst: NULL, wdbuf + 2, FALSE);

		//圧縮

		wdbuf[0] = UndoByteEncode(flagdst, flagsrc, wdbuf[2]);
		wdbuf[1] = UndoWordEncode(encbuf, srcbuf, wdbuf[3]);

		//書き込み

		return (UndoItem_write(dst, wdbuf, 12)
			&& UndoItem_write(dst, (wdbuf[0] == wdbuf[2])? flagsrc: flagdst, wdbuf[0])
			&& UndoItem_write(dst, (wdbuf[1] == wdbuf[3])? srcbuf: encbuf, wdbuf[1]));
	}
}


//==================================
// 書き込みメイン
//==================================
/*
 * [TileImageInfo] 描画前のイメージ情報
 * [タイルデータ]
 *   <共通>
 *   2byte x 2 : タイル位置。両方 0xffff で終了。
 *   1byte : フラグ
 *
 *   <各カラータイプ別のデータ>
 */


/** アンドゥイメージ最初の書き込み */

mBool UndoItem_setdat_tileimage(UndoItem *p,TileImageInfo *info)
{
	TileImage *img,*imgcur;
	uint16_t tpos[2];
	int tw,th,ix,iy;
	uint8_t **pptile,flags,*tilecur;
	mBool ret = FALSE;

	//イメージ

	img = APP_DRAW->tileimgDraw;
	imgcur = APP_DRAW->curlayer->img;

	//書き込み開始

	if(!UndoItem_alloc(p, UNDO_ALLOC_MEM_VARIABLE)
		|| !UndoItem_openWrite(p))
		return FALSE;

	//イメージ情報

	if(!UndoItem_write(p, info, sizeof(TileImageInfo)))
		goto ERR;

	//タイルデータ

	pptile = img->ppbuf;
	tw = img->tilew;
	th = img->tileh;

	for(iy = 0; iy < th; iy++)
	{
		for(ix = 0; ix < tw; ix++, pptile++)
		{
			if(!(*pptile)) continue;

			//タイル位置、フラグ

			tpos[0] = ix;
			tpos[1] = iy;

			flags = (*pptile == TILEIMAGE_TILE_EMPTY)? _FLAGS_EMPTY | _FLAGS_FIRST_EMPTY: 0;

			if(!UndoItem_write(p, tpos, 4)
				|| !UndoItem_write(p, &flags, 1))
				goto ERR;

			//タイル
			/* 保存用イメージとカレントイメージのタイル配列構成は
			 * 同じ状態になっている (保存イメージは描画開始時の状態ではない) ので、
			 * そのまま同じタイル位置で取得可能。 */

			if(!(flags & _FLAGS_EMPTY))
			{
				tilecur = TILEIMAGE_GETTILE_PT(imgcur, ix, iy);
			
				if(!_write_first_tile(p, img->coltype, *pptile, tilecur))
					goto ERR;
			}
		}
	}

	//タイル終了

	tpos[0] = tpos[1] = 0xffff;

	if(!UndoItem_write(p, tpos, 4))
		goto ERR;

	//

	ret = TRUE;

ERR:
	UndoItem_closeWrite(p);
	
	return ret;
}

/** 反転イメージを書き込み */

mBool UndoItem_setdat_tileimage_reverse(UndoItem *dst,UndoItem *src,int settype)
{
	TileImage *img;
	TileImageInfo info;
	int toffx,toffy;
	uint16_t tpos[2];
	uint8_t *ptile,flags_src,flags_dst;
	mBool ret = FALSE;

	//対象イメージ

	img = _get_layerimg(dst);
	if(!img) return FALSE;

	//--------- 書き込み

	//元データ情報

	if(!UndoItem_openRead(src)) return FALSE;

	if(!UndoItem_read(src, &info, sizeof(TileImageInfo)))
		goto ERR;

	//

	toffx = (info.offx - img->offx) >> 6;
	toffy = (info.offy - img->offy) >> 6;

	//書き込み開始

	if(!UndoItem_alloc(dst, UNDO_ALLOC_MEM_VARIABLE)
		|| !UndoItem_openWrite(dst))
		goto ERR;

	//イメージ情報書き込み

	TileImage_getInfo(img, &info);

	if(!UndoItem_write(dst, &info, sizeof(TileImageInfo)))
		goto ERR;

	//----- タイル

	while(1)
	{
		//src タイル位置取得

		if(!UndoItem_read(src, tpos, 4)) goto ERR;

		//タイル終了

		if(tpos[0] == 0xffff && tpos[1] == 0xffff)
			break;
	
		//src フラグ読み込み

		if(!UndoItem_read(src, &flags_src, 1))
			goto ERR;

		//保存対象のタイル取得

		if(settype == MUNDO_TYPE_REDO)
		{
			/* src:UNDO -> dst:REDO
			 *
			 * 現在のタイル状態を保存。 */

			ptile = TILEIMAGE_GETTILE_PT(img, tpos[0], tpos[1]);
		}
		else
		{
			/* src:REDO -> dst:UNDO */

			if(flags_src & _FLAGS_FIRST_EMPTY)
				ptile = NULL;
			else
				ptile = TILEIMAGE_GETTILE_PT(img, tpos[0] + toffx, tpos[1] + toffy);
		}

		//フラグ

		flags_dst = flags_src & _FLAGS_FIRST_EMPTY;

		if(!ptile)
			flags_dst |= _FLAGS_EMPTY;

		//タイル位置、フラグ書き込み

		if(!UndoItem_write(dst, tpos, 4)
			|| !UndoItem_write(dst, &flags_dst, 1))
			goto ERR;

		//タイルデータ

		if(!_write_rev_tile(dst, src, img->coltype, ptile, !(flags_src & _FLAGS_EMPTY)))
			goto ERR;
	}

	//タイル終了

	tpos[0] = tpos[1] = 0xffff;

	if(!UndoItem_write(dst, tpos, 4))
		goto ERR;

	//

	ret = TRUE;

ERR:
	UndoItem_closeRead(src);
	UndoItem_closeWrite(dst);

	return ret;
}


//==================================
// 復元
//==================================


/** タイル復元 */

static mBool _restore_tile(UndoItem *p,int coltype,uint8_t *ptile)
{
	uint8_t *srcbuf,*dstbuf,*flagsrc,*flagdst;
	uint16_t wdbuf[6];

	srcbuf = _UNDO->workbuf1;
	dstbuf = _UNDO->workbuf2;

	//

	if(coltype == TILEIMAGE_COLTYPE_ALPHA1)
	{
		//------- A1

		if(!UndoItem_read(p, wdbuf, 2)) return FALSE;

		return _read_byte_decode(p, ptile, 8 * 64, srcbuf, wdbuf[0]);
	}
	else
	{
		//------- A16/GRAY/RGBA

		flagsrc = _UNDO->workbuf_flags;
		flagdst = flagsrc + 2048;

		//読み込み

		if(!UndoItem_read(p, wdbuf, 12)
			|| !_read_byte_decode(p, flagdst, wdbuf[2], flagsrc, wdbuf[0])
			|| !_read_word_decode(p, dstbuf, wdbuf[3], srcbuf, wdbuf[1]))
			return FALSE;

		//復元

		(g_restoretilefunc[coltype])((uint16_t *)ptile, flagdst, (uint16_t *)dstbuf, wdbuf[4], wdbuf[5]);
	}

	return TRUE;
}

/** タイルイメージ復元 */

mBool UndoItem_restore_tileimage(UndoItem *p,int runtype)
{
	TileImage *img;
	TileImageInfo info;
	uint16_t tpos[2];
	uint8_t *ptile,flags;
	mBool ret = FALSE;

	//対象イメージ

	img = _get_layerimg(p);
	if(!img) return FALSE;

	//読み込み開始

	if(!UndoItem_openRead(p)) return FALSE;

	if(!UndoItem_read(p, &info, sizeof(TileImageInfo)))
		goto ERR;

	//配列リサイズ (リドゥ時)

	if(runtype == MUNDO_TYPE_REDO
		&& !TileImage_resizeTileBuf_forUndo(img, &info))
		goto ERR;

	//タイル

	while(1)
	{
		//タイル位置

		if(!UndoItem_read(p, tpos, 4)) goto ERR;

		//タイル終了

		if(tpos[0] == 0xffff && tpos[1] == 0xffff)
			break;
	
		//フラグ

		if(!UndoItem_read(p, &flags, 1))
			goto ERR;

		//タイル復元

		if(flags & _FLAGS_EMPTY)
			//空タイルにする
			TileImage_freeTile_atPos(img, tpos[0], tpos[1]);
		else
		{
			//読み込み

			ptile = TileImage_getTileAlloc_atpos(img, tpos[0], tpos[1], TRUE);
			if(!ptile) goto ERR;

			if(!_restore_tile(p, img->coltype, ptile))
				goto ERR;
		}
	}

	//配列リサイズ (アンドゥ時)

	if(runtype == MUNDO_TYPE_UNDO
		&& !TileImage_resizeTileBuf_forUndo(img, &info))
		goto ERR;

	//

	ret = TRUE;

ERR:
	UndoItem_closeRead(p);

	return ret;
}


/*************************************
 * タイルデータ処理
 *************************************/
/*
 * tilesrc のタイルの状態を保存する。
 * tilesrc = NULL の場合はタイルなしとして記録されるので、ここの処理は行わない。
 *
 * データ自体をなるべく減らすため、変化しなかった点はフラグで最小ビットに抑え、
 * アルファ値で頻繁に使用される 0 と最大値(0x8000) もフラグで表現できるようにする。
 * 
 * <最初の書き込み時>
 * tilesrc = 描画前のタイル
 * cmpbuf  = 現在のタイル (NULL あり)
 *
 * <反転書き込み時>
 * tilesrc = 現在のタイル
 * cmpbuf  = 戻すタイルのフラグデータ (NULL で空タイル)
 */


//==================================
// ビットデータ処理
//==================================


/** ビット書き込み (書き込みビット数は 7bit 以下) */

static void _write_bits(uint8_t **ppdst,int *pbitpos,uint8_t val,uint8_t bits)
{
	uint8_t rbits,n,*pdst;

	pdst = *ppdst;

	//バイト内の残りビット数 (1-8)

	rbits = 8 - (*pbitpos & 7);

	//

	if(bits <= rbits)
	{
		//バイト内に収まる場合

		if(rbits == 8)
			//バイト内の最初のビットの場合
			*pdst = val << (8 - bits);
		else
			*pdst |= val << (rbits - bits);

		if(bits == rbits) pdst++;
	}
	else
	{
		//バイトをまたぐ場合

		n = bits - rbits; //次のバイトのビット数

		pdst[0] |= val >> n;
		pdst[1] = (val & ((1 << n) - 1)) << (8 - n);

		pdst++;
	}

	*ppdst = pdst;
	*pbitpos += bits;
}

/** ビットフラグ (GRAY/RGBA) 読み込み
 *
 * @return 0 で変化なし、0 以外でフラグ(3bit) */

static int _read_flagbit(uint8_t **ppsrc,int *pbitpos)
{
	uint8_t *psrc,rbits,nbits,bits = 1;
	int n;

	psrc = *ppsrc;

	//バイト内の残りビット数

	rbits = 8 - (*pbitpos & 7);

	//1bit 変化フラグ

	n = (*psrc >> (rbits - 1)) & 1;

	rbits--;
	if(rbits == 0)
	{
		psrc++;
		rbits = 8;
	}

	//変化フラグが ON -> 3bit フラグ

	if(n)
	{
		if(rbits >= 3)
		{
			//バイト内に収まっている
			
			n = (*psrc >> (rbits - 3)) & 7;
			if(rbits == 3) psrc++;
		}
		else
		{
			//次のバイトにまたがる

			nbits = 3 - rbits; //次のバイトのビット数
			
			n = (*psrc & ((1 << rbits) - 1)) << nbits;
			n |= psrc[1] >> (8 - nbits);

			psrc++;
		}

		bits += 3;
	}

	*ppsrc = psrc;
	*pbitpos += bits;

	return n;
}


//==================================
// A16 タイル
//==================================
/*
 * <flags : 64x64x2bit = 1024byte>
 *  0-変化なし 1-透明 2-0x8000 3-指定値(Aデータに)
 */


/** A16 データ取得
 *
 * @param first 初回書き込み時か
 * @param dstsize [0]フラグサイズ [1]色データ総サイズ [2]Aデータサイズ [3]RGBチャンネルサイズ */

void _get_writetile_A16(uint8_t *dstf,uint16_t *dsta,uint16_t *tilesrc,uint8_t *cmpbuf,uint16_t *dstsize,mBool first)
{
	int i,src,cmp,asize = 0;
	uint8_t val = 0,fs = 6,n;

	for(i = 64 * 64; i; i--)
	{
		src = *(tilesrc++);

		//比較用アルファ値

		if(!cmpbuf)
			//NULL の場合、空タイル
			cmp = 0;
		else if(first)
		{
			//初回書き込み時
			
			cmp = *((uint16_t *)cmpbuf);
			cmpbuf += 2;
		}
		else
		{
			//反転書き込み時

			if(((*cmpbuf >> fs) & 3) == 0)
				//変化なしにする
				cmp = src;
			else
				//変化ありなので src の値から再取得させる
				//(src == cmp の判定を FALSE にする)
				cmp = !src;

			if(!fs) cmpbuf++;
		}

		//セット

		if(src == cmp)
			n = 0;
		else if(src == 0)
			n = 1;
		else if(src == 0x8000)
			n = 2;
		else
		{
			n = 3;

			*(dsta++) = src;
			asize += 2;
		}

		val |= n << fs;

		if(fs)
			fs -= 2;
		else
		{
			*(dstf++) = val;
			fs = 6;
			val = 0;
		}
	}

	dstsize[0] = 1024;
	dstsize[1] = asize;
	dstsize[2] = asize;
	dstsize[3] = 0;
}

/** 復元 */

void _restore_tile_A16(uint16_t *dst,uint8_t *flagsrc,uint16_t *asrc,int asize,int rgbsize)
{
	int i,n;
	uint8_t fval,fs = 6;

	fval = *(flagsrc++);

	for(i = 64 * 64; i; i--, dst++)
	{
		n = (fval >> fs) & 3;

		if(n)
		{
			if(n == 1)
				*dst = 0;
			else if(n == 2)
				*dst = 0x8000;
			else
				*dst = *(asrc++);
		}

		if(fs)
			fs -= 2;
		else
		{
			fval = *(flagsrc++);
			fs = 6;
		}
	}
}



//==================================
// GRAY タイル
//==================================
/*
 * <フラグ>
 *  先頭ビットが 0 で変化なし、1 で変化あり。
 *  変化ありの場合、以下の 3bit が続く。
 *  最大 64x64x4bit = 2048byte。
 *
 *  [0-1bit] アルファ値
 *    0-変化なし 1-透明 2-0x8000 3-指定値(Aデータに)
 *  [2bit] 色変化フラグ
 *    色値が変化したか (ON で色データに値セット)
 */


/** GRAY データ取得 */

void _get_writetile_gray(uint8_t *dstf,uint16_t *dstcol,uint16_t *tilesrc,uint8_t *cmpbuf,uint16_t *dstsize,mBool first)
{
	int i,revf,bitpos = 0,flagbitpos = 0,asize = 0,colsize = 0;
	uint16_t srca,srcc,cmpa,cmpc,*pdstc,*pdsta,*pcmp16;
	uint8_t val;

	pdsta = dstcol;
	pdstc = dstcol + 64 * 64;
	pcmp16 = (uint16_t *)cmpbuf;

	for(i = 64 * 64; i; i--)
	{
		//src

		srcc = tilesrc[0];
		srca = tilesrc[1];
		tilesrc += 2;

		//

		if(first || !cmpbuf)
		{
			/* 初回書き込み時、または比較対象が空タイルの場合は
			 * 比較対象の値と比較してセット */

			//比較値

			if(!cmpbuf)
				cmpc = cmpa = 0;
			else
			{
				cmpc = pcmp16[0];
				cmpa = pcmp16[1];
				pcmp16 += 2;
			}

			//セット

			if((srcc == cmpc && srca == cmpa) || (srca == 0 && cmpa == 0))
				//---- 変化なし (1bit)
				_write_bits(&dstf, &bitpos, 0, 1);
			else
			{
				//---- 変化あり (4bit)

				val = 1<<3;

				//A

				if(srca != cmpa)
				{
					if(srca == 0)
						val |= 1;
					else if(srca == 0x8000)
						val |= 2;
					else
					{
						val |= 3;

						*(pdsta++) = srca;
						asize += 2;
					}
				}

				//色
				/* - 透明 <--> 不透明 に変化した場合は、強制的に色変化ありとする。
				 * - 不透明->透明になった時、色は変わらずにアルファ値のみが0になった場合、
				 *   アンドゥ復元時に色が0になるので、色を保存しないと黒色になってしまう。
				 * - 書き込む色が透明の場合 (srca == 0) は色は 0 とするので、
				 *   反転書き込み時の比較用にフラグだけ ON にする */

				if(srcc != cmpc || !srca || !cmpa)
				{
					val |= 4;

					if(srca)
					{
						*(pdstc++) = srcc;
						colsize += 2;
					}
				}
			
				_write_bits(&dstf, &bitpos, val, 4);
			}
		}
		else
		{
			/* 反転書き込みで元データが存在する場合
			 * (存在するタイルに上書き描画した場合)、
			 * フラグを元にセット */

			revf = _read_flagbit(&cmpbuf, &flagbitpos);

			//セット

			if(!revf)
				//---- 変化なし (1bit)
				_write_bits(&dstf, &bitpos, 0, 1);
			else
			{
				//---- 変化あり (4bit)

				val = 1<<3;

				//A変化

				if(revf & 3)
				{
					if(srca == 0)
						val |= 1;
					else if(srca == 0x8000)
						val |= 2;
					else
					{
						val |= 3;

						*(pdsta++) = srca;
						asize += 2;
					}
				}

				//色変化

				if(revf & 4)
				{
					val |= 4;

					if(srca)
					{
						*(pdstc++) = srcc;
						colsize += 2;
					}
				}
			
				_write_bits(&dstf, &bitpos, val, 4);
			}
		}
	}

	//Aデータの余白を詰める

	if(asize != 64 * 64 * 2)
		memmove((uint8_t *)dstcol + asize, dstcol + 64 * 64, colsize);

	//

	dstsize[0] = (bitpos + 7) >> 3;
	dstsize[1] = asize + colsize;
	dstsize[2] = asize;
	dstsize[3] = 0;
}

/** 復元 */

void _restore_tile_gray(uint16_t *dst,uint8_t *flagsrc,uint16_t *colsrcbuf,int asize,int rgbsize)
{
	int i,n,fa,bitpos = 0;
	uint16_t *psrca,*psrcc;

	psrca = colsrcbuf;
	psrcc = colsrcbuf + (asize >> 1);

	for(i = 64 * 64; i; i--, dst += 2)
	{
		n = _read_flagbit(&flagsrc, &bitpos);

		if(n)
		{
			fa = n & 3;
			
			//色変化あり (透明の場合は除く)
			
			if((n & 4) && fa != 1)
				dst[0] = *(psrcc++);

			//A変化

			if(fa)
			{
				if(fa == 1)
				{
					//透明の場合は色も 0 にする
					dst[0] = 0;
					dst[1] = 0;
				}
				else if(fa == 2)
					dst[1] = 0x8000;
				else
					dst[1] = *(psrca++);
			}	
		}
	}
}


//==================================
// RGBA タイル
//==================================
/*
 * <フラグ>
 *  先頭ビットが 0 で変化なし、1 で変化あり。
 *  変化ありの場合、以下の 3bit が続く。
 *  最大 64x64x4bit = 2048byte。
 *
 *  [0-1bit] アルファ値
 *    0-変化なし 1-透明 2-0x8000 3-指定値(Aデータに)
 *  [2bit] 色変化フラグ
 *    RGB値が変化したか (ON で色データに値セット)
 *
 * <色データ>
 *  A,R,G,B の順。
 */


/** RGBA データ取得 */

void _get_writetile_rgba(uint8_t *dstf,uint16_t *dstcol,uint16_t *tilesrc,uint8_t *cmpbuf,uint16_t *dstsize,mBool first)
{
	int i,revf,bitpos = 0,flagbitpos = 0,asize = 0,colnum = 0;
	uint16_t srcA,srcR,srcG,srcB,cmpA,cmpR,cmpG,cmpB;
	uint16_t *pcmp16,*pdstA,*pdstR,*pdstG,*pdstB;
	uint8_t val;

	pcmp16 = (uint16_t *)cmpbuf;

	pdstA = dstcol;
	pdstR = pdstA + 64 * 64;
	pdstG = pdstR + 64 * 64;
	pdstB = pdstG + 64 * 64;

	//

	for(i = 64 * 64; i; i--)
	{
		//src

		srcR = tilesrc[0];
		srcG = tilesrc[1];
		srcB = tilesrc[2];
		srcA = tilesrc[3];
		tilesrc += 4;

		//

		if(first || !cmpbuf)
		{
			/* 初回書き込み時、または比較対象が空タイルの場合は
			 * 比較対象の値と比較してセット */

			//比較値

			if(!cmpbuf)
				cmpR = cmpG = cmpB = cmpA = 0;
			else
			{
				cmpR = pcmp16[0];
				cmpG = pcmp16[1];
				cmpB = pcmp16[2];
				cmpA = pcmp16[3];
				pcmp16 += 4;
			}

			//セット

			if((srcA == 0 && cmpA == 0)
				|| (srcA == cmpA && srcR == cmpR && srcG == cmpG && srcB == cmpB))
				//---- 変化なし (1bit)
				_write_bits(&dstf, &bitpos, 0, 1);
			else
			{
				//---- 変化あり (4bit)

				val = 1<<3;

				//A

				if(srcA != cmpA)
				{
					if(srcA == 0)
						val |= 1;
					else if(srcA == 0x8000)
						val |= 2;
					else
					{
						val |= 3;

						*(pdstA++) = srcA;
						asize += 2;
					}
				}

				//色

				if(!srcA || !cmpA || srcR != cmpR || srcG != cmpG || srcB != cmpB)
				{
					val |= 4;

					if(srcA)
					{
						*(pdstR++) = srcR;
						*(pdstG++) = srcG;
						*(pdstB++) = srcB;
						
						colnum++;
					}
				}
			
				_write_bits(&dstf, &bitpos, val, 4);
			}
		}
		else
		{
			/* 反転書き込みで元データが存在する場合
			 * (存在するタイルに上書き描画した場合)、
			 * フラグを元にセット */

			revf = _read_flagbit(&cmpbuf, &flagbitpos);

			//セット

			if(!revf)
				//---- 変化なし (1bit)
				_write_bits(&dstf, &bitpos, 0, 1);
			else
			{
				//---- 変化あり (4bit)

				val = 1<<3;

				//A変化

				if(revf & 3)
				{
					if(srcA == 0)
						val |= 1;
					else if(srcA == 0x8000)
						val |= 2;
					else
					{
						val |= 3;

						*(pdstA++) = srcA;
						asize += 2;
					}
				}

				//色変化

				if(revf & 4)
				{
					val |= 4;

					if(srcA)
					{
						*(pdstR++) = srcR;
						*(pdstG++) = srcG;
						*(pdstB++) = srcB;
						
						colnum++;
					}
				}
			
				_write_bits(&dstf, &bitpos, val, 4);
			}
		}
	}

	//色データの余白を詰める

	i = colnum << 1;

	if(asize != 64 * 64 * 2)
		memmove((uint8_t *)dstcol + asize, dstcol + 64 * 64, i);

	if(asize != 64 * 64 * 2 || colnum != 64 * 64)
	{
		memmove((uint8_t *)dstcol + asize + i, dstcol + 64 * 64 * 2, i);
		memmove((uint8_t *)dstcol + asize + i + i, dstcol + 64 * 64 * 3, i);
	}

	//

	dstsize[0] = (bitpos + 7) >> 3;
	dstsize[1] = asize + colnum * 6;
	dstsize[2] = asize;
	dstsize[3] = colnum << 1;
}

/** 復元 */

void _restore_tile_rgba(uint16_t *dst,uint8_t *flagsrc,uint16_t *colsrcbuf,int asize,int rgbsize)
{
	int i,n,fa,bitpos = 0;
	uint16_t *psrcA,*psrcR,*psrcG,*psrcB;

	psrcA = colsrcbuf;
	psrcR = colsrcbuf + (asize >> 1);
	psrcG = psrcR + (rgbsize >> 1);
	psrcB = psrcG + (rgbsize >> 1);

	//

	for(i = 64 * 64; i; i--, dst += 4)
	{
		n = _read_flagbit(&flagsrc, &bitpos);

		if(n)
		{
			fa = n & 3;
			
			//色変化あり (透明の場合は除く)
			
			if((n & 4) && fa != 1)
			{
				dst[0] = *(psrcR++);
				dst[1] = *(psrcG++);
				dst[2] = *(psrcB++);
			}

			//A変化

			if(fa)
			{
				if(fa == 1)
				{
					//透明の場合は色も 0 にする
					dst[0] = dst[1] = dst[2] = dst[3] = 0;
				}
				else if(fa == 2)
					dst[3] = 0x8000;
				else
					dst[3] = *(psrcA++);
			}	
		}
	}
}

