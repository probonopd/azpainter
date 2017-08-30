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
 * DrawData
 *
 * ADW 読み込み (ver1,ver2)
 *****************************************/

#include <stdio.h>

#include "mDef.h"
#include "mPopupProgress.h"
#include "mZlib.h"
#include "mUtilStdio.h"
#include "mUtilCharCode.h"
#include "mUtil.h"

#include "defDraw.h"
#include "defLoadErr.h"

#include "LayerList.h"
#include "LayerItem.h"
#include "TileImage.h"

#include "draw_main.h"


//------------------

typedef struct
{
	mPopupProgress *prog;
	
	mZlibDecode *dec;
	uint8_t *tmpbuf;
}_load_adw;

//------------------


/** ver 2 読み込み (LE)
 *
 * Alpha-8bit 64x64 タイルイメージ */

static int _load_ver2(_load_adw *p,FILE *fp)
{
	int i;
	uint32_t size,tilenum,col;
	uint16_t wsize,imgw,imgh,dpi,layernum,layersel,layerinfosize,name[25],tx,ty;
	uint8_t opacity,flags,*tile;
	LayerItem *item;
	TileImage *img;
	mRect rc;

	//プレビューイメージをスキップ

	if(fseek(fp, 4, SEEK_CUR)			//幅、高さ
		|| !mFILEread32LE(fp, &size)	//イメージサイズ
		|| fseek(fp, size, SEEK_CUR))
		return LOADERR_CORRUPTED;

	//メイン情報

	if(!mFILEreadArgsLE(fp, "2222222",
		&wsize, &imgw, &imgh, &dpi, &layernum, &layersel, &layerinfosize))
		return LOADERR_CORRUPTED;

	fseek(fp, wsize - 12, SEEK_CUR);

	//新規イメージ

	if(!drawImage_new(APP_DRAW, imgw, imgh, dpi, -1))
		return LOADERR_ALLOC;

	//--------- レイヤ

	mPopupProgressThreadSetMax(p->prog, layernum * 6);

	for(item = NULL; layernum; layernum--)
	{
		//px 範囲とタイル数

		if(!mFILEreadArgsLE(fp, "44444",
			&rc.x1, &rc.y1, &rc.x2, &rc.y2, &tilenum))
			return LOADERR_CORRUPTED;

		//レイヤ追加

		item = LayerList_addLayer(APP_DRAW->layerlist, item);
		if(!item) return LOADERR_ALLOC;

		//イメージ作成 (A16)

		img = TileImage_newFromRect_forFile(TILEIMAGE_COLTYPE_ALPHA16, &rc);
		if(!img) return LOADERR_ALLOC;

		LayerItem_replaceImage(item, img);

		//レイヤ情報

		if(fread(name, 1, 50, fp) != 50
			|| !mFILEreadArgsLE(fp, "141", &opacity, &col, &flags)
			|| fseek(fp, 3, SEEK_CUR))
			return LOADERR_CORRUPTED;

		//

		item->opacity = opacity;

		if(!(flags & 1))
			item->flags &= ~LAYERITEM_F_VISIBLE;

		LayerItem_setLayerColor(item, col);

		//名前 (UTF-16 => UTF-8)

		for(i = 0; i < 24; i++)
			name[i] = mGetBuf16LE(name + i);

		name[24] = 0;

		item->name = mUTF16ToUTF8_alloc(name, -1, NULL);

		//seek

		fseek(fp, layerinfosize - 79, SEEK_CUR);

		//空イメージ

		if(tilenum == 0)
		{
			mPopupProgressThreadAddPos(p->prog, 6);
			continue;
		}

		//タイル
		
		mPopupProgressThreadBeginSubStep(p->prog, 6, tilenum);

		for(; tilenum; tilenum--)
		{
			//タイル位置、圧縮サイズ

			if(!mFILEreadArgsLE(fp, "222", &tx, &ty, &wsize))
				return LOADERR_CORRUPTED;

			if(wsize == 0 || wsize > 4096)
				return LOADERR_INVALID_VALUE;

			//タイル作成

			tile = TileImage_getTileAlloc_atpos(img, tx, ty, FALSE);
			if(!tile) return LOADERR_ALLOC;

			//読み込み

			if(wsize == 4096)
			{
				//無圧縮
				
				if(fread(tile, 1, 4096, fp) != 4096)
					return LOADERR_CORRUPTED;
			}
			else
			{
				//zlib
				
				mZlibDecodeReset(p->dec);

				if(mZlibDecodeReadOnce(p->dec, tile, 4096, wsize))
					return LOADERR_DECODE;
			}

			//A8 => A16 変換

			TileImage_convertTile_A8toA16(tile);

			//progress
			
			mPopupProgressThreadIncSubStep(p->prog);
		}

		//念のため空タイルを解放

		TileImage_freeEmptyTiles(img);
	}

	//カレントレイヤ

	APP_DRAW->curlayer = LayerList_getItem_byPos_topdown(APP_DRAW->layerlist, layersel);

	return LOADERR_OK;
}

/** ver 1 読み込み (LE)
 *
 * Alpha-8bit 幅x高さのベタイメージ */

static int _load_ver1(_load_adw *p,FILE *fp)
{
	uint32_t size,col;
	uint16_t imgw,imgh,layernum,layercnt,layersel;
	uint8_t comp,opacity,flags;
	int pitch,ix,iy;
	char name[32];
	LayerItem *item;
	TileImage *img;
	uint8_t *ps;
	RGBAFix15 pix;

	//メイン情報

	if(!mFILEreadArgsLE(fp, "222221",
		&imgw, &imgh, &layernum, &layercnt, &layersel, &comp))
		return LOADERR_CORRUPTED;

	if(comp != 0) return LOADERR_INVALID_VALUE;

	//プレビューイメージをスキップ

	if(!mFILEread32LE(fp, &size)
		|| fseek(fp, size, SEEK_CUR))
		return LOADERR_CORRUPTED;

	//新規イメージ

	if(!drawImage_new(APP_DRAW, imgw, imgh, -1, -1))
		return LOADERR_ALLOC;

	//作業用バッファ

	pitch = (imgw + 3) & (~3);

	p->tmpbuf = (uint8_t *)mMalloc(pitch, FALSE);
	if(!p->tmpbuf) return LOADERR_ALLOC;

	//------- レイヤ

	pix.v64 = 0;

	mPopupProgressThreadSetMax(p->prog, layernum * 6);

	for(item = NULL; layernum; layernum--)
	{
		//レイヤ追加 (下から順なので、現在のトップレイヤの上へ挿入)

		item = LayerList_addLayer(APP_DRAW->layerlist, item);
		if(!item) return LOADERR_ALLOC;

		//イメージ作成 (A16)

		img = TileImage_new(TILEIMAGE_COLTYPE_ALPHA16, imgw, imgh);
		if(!img) return LOADERR_ALLOC;

		LayerItem_replaceImage(item, img);

		//レイヤ情報

		if(fread(name, 1, 32, fp) != 32
			|| !mFILEreadArgsLE(fp, "1144", &opacity, &flags, &col, &size))
			return LOADERR_CORRUPTED;

		/* 名前は Shift-JIS。
		 * すべて ASCII 文字ならそのままセット。 */

		name[31] = 0;
		LayerList_setItemName_ascii(APP_DRAW->layerlist, item, name);

		item->opacity = opacity;

		if(!(flags & 1))
			item->flags &= ~LAYERITEM_F_VISIBLE;

		LayerItem_setLayerColor(item, col);

		//イメージ読み込み

		mZlibDecodeReset(p->dec);
		mZlibDecodeSetInSize(p->dec, size);

		mPopupProgressThreadBeginSubStep(p->prog, 6, imgh);

		for(iy = 0; iy < imgh; iy++)
		{
			if(mZlibDecodeRead(p->dec, p->tmpbuf, pitch))
				return LOADERR_DECODE;

			//横一列セット

			ps = p->tmpbuf;

			for(ix = 0; ix < imgw; ix++, ps++)
			{
				if(*ps)
				{
					pix.a = RGBCONV_8_TO_FIX15(*ps);

					TileImage_setPixel_new(img, ix, iy, &pix);
				}
			}

			mPopupProgressThreadIncSubStep(p->prog);
		}

		if(mZlibDecodeReadEnd(p->dec))
			return LOADERR_DECODE;
	}

	//カレントレイヤ (layersel は先頭からの位置)

	APP_DRAW->curlayer = LayerList_getItem_byPos_topdown(APP_DRAW->layerlist, layersel);

	return LOADERR_OK;
}


//==============================


/** 読み込みメイン */

static int _load_main(_load_adw *p,FILE *fp)
{
	uint8_t ver;
	int err;

	//ヘッダ

	if(!mFILEreadCompareStr(fp, "AZDWDAT"))
		return LOADERR_HEADER;

	//バージョン

	if(!mFILEreadByte(fp, &ver) || ver > 2)
		return LOADERR_VERSION;

	//zlib 初期化

	p->dec = mZlibDecodeNew(16 * 1024, MZLIBDEC_WINDOWBITS_ZLIB);
	if(!p->dec) return LOADERR_ALLOC;

	mZlibDecodeSetIO_stdio(p->dec, fp);

	//読み込み

	if(ver == 0)
		err = _load_ver1(p, fp);
	else
		err = _load_ver2(p, fp);

	//解放

	mZlibDecodeFree(p->dec);
	mFree(p->tmpbuf);

	return err;
}

/** ADW 読み込み
 *
 * @return LOADERR_* */

int drawFile_load_adw(const char *filename,mPopupProgress *prog)
{
	_load_adw dat;
	FILE *fp;
	int err;

	mMemzero(&dat, sizeof(_load_adw));
	dat.prog = prog;

	fp = mFILEopenUTF8(filename, "rb");
	if(!fp) return LOADERR_OPENFILE;

	err = _load_main(&dat, fp);

	fclose(fp);

	return err;
}
