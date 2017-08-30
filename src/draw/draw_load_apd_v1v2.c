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
 * APD 読み込み (ver 1,2)
 *****************************************/

#include <stdio.h>

#include "mDef.h"
#include "mPopupProgress.h"
#include "mZlib.h"
#include "mUtilStdio.h"
#include "mUtilCharCode.h"
#include "mUtil.h"

#include "defDraw.h"
#include "defBlendMode.h"
#include "defLoadErr.h"

#include "LayerList.h"
#include "LayerItem.h"
#include "defTileImage.h"
#include "TileImage.h"
#include "TileImage_coltype.h"
#include "ImageBufRGB16.h"

#include "draw_main.h"


//------------------

typedef struct
{
	mPopupProgress *prog;
	
	mZlibDecode *dec;
	uint8_t *tmpbuf;
}_load_apd;

//------------------

//v1 合成モード変換テーブル (「色相」以降はなし)

static const uint8_t g_apd_v1_blendmode[] = {
	BLENDMODE_NORMAL, BLENDMODE_MUL, BLENDMODE_ADD, BLENDMODE_SUB,
	BLENDMODE_SCREEN, BLENDMODE_OVERLAY, BLENDMODE_SOFT_LIGHT, BLENDMODE_HARD_LIGHT,
	BLENDMODE_DODGE, BLENDMODE_BURN, BLENDMODE_LINEAR_BURN, BLENDMODE_VIVID_LIGHT,
	BLENDMODE_LINEAR_LIGHT, BLENDMODE_PIN_LIGHT, BLENDMODE_DIFF,
	BLENDMODE_LIGHT, BLENDMODE_DARK
};

//------------------



//==============================
// ver 1 (LE)
//==============================
/*
 * AzPainter(Windows) ver1,2 で保存されたもの。
 * BGRA 8bit 幅x高さのベタイメージ (bottom-up)
 *
 * blendimg に RGBA8 としてセットして変換。
 */


/** レイヤ読み込み */

static int _load_ver1_layers(_load_apd *p,FILE *fp,int layernum,int layerinfo_seek)
{
	LayerItem *item;
	TileImage *img;
	int i,imgw,imgh,pitch;
	uint32_t size;
	uint8_t blendmode,opacity,flags;
	char name[32];
	uint8_t *tmpbuf,*pd,rep;

	imgw = APP_DRAW->imgw;
	imgh = APP_DRAW->imgh;

	tmpbuf = (uint8_t *)APP_DRAW->blendimg->buf;
	pitch = imgw << 2;
	
	mPopupProgressThreadSetMax(p->prog, layernum * 6);

	//

	for(item = NULL; layernum > 0; layernum--)
	{
		//レイヤ追加

		item = LayerList_addLayer(APP_DRAW->layerlist, item);
		if(!item) return LOADERR_ALLOC;

		//イメージ作成

		img = TileImage_new(TILEIMAGE_COLTYPE_RGBA, imgw, imgh);
		if(!img) return LOADERR_ALLOC;

		LayerItem_replaceImage(item, img);

		//レイヤ情報

		if(fread(name, 1, 32, fp) != 32
			|| !mFILEreadByte(fp, &blendmode)
			|| !mFILEreadByte(fp, &opacity)
			|| !mFILEreadByte(fp, &flags)
			|| fseek(fp, layerinfo_seek, SEEK_CUR)
			|| !mFILEread32LE(fp, &size))
			return LOADERR_CORRUPTED;

		//ASCII 文字のみならそのままセット

		name[31] = 0;
		LayerList_setItemName_ascii(APP_DRAW->layerlist, item, name);

		//

		item->opacity = opacity;

		if(!(flags & 1))
			item->flags &= ~LAYERITEM_F_VISIBLE;

		//合成モード

		if(blendmode >= 17)
			//"色相"以上は通常
			item->blendmode = BLENDMODE_NORMAL;
		else
			item->blendmode = g_apd_v1_blendmode[blendmode];

		//tmpbuf にデコード (bottom-up なので下から順に)

		mZlibDecodeReset(p->dec);
		mZlibDecodeSetInSize(p->dec, size);

		pd = tmpbuf + (imgh - 1) * pitch;

		for(i = imgh; i; i--, pd -= pitch)
		{
			if(mZlibDecodeRead(p->dec, pd, pitch))
				return LOADERR_DECODE;
		}

		if(mZlibDecodeReadEnd(p->dec))
			return LOADERR_DECODE;

		//BGRA -> RGBA 変換

		pd = tmpbuf;

		for(i = imgw * imgh; i; i--, pd += 4)
		{
			rep   = pd[0];
			pd[0] = pd[2];
			pd[2] = rep;
		}

		//セット

		TileImage_setImage_fromRGBA8(img, tmpbuf, imgw, imgh, p->prog, 6);
	}

	return LOADERR_OK;
}

/** ver 1 読み込み */

static int _load_ver1(_load_apd *p,FILE *fp)
{
	uint32_t size,layerinfosize;
	uint16_t imgw,imgh,layernum,layercnt,layersel;
	int ret;

	//メイン情報

	if(!mFILEreadArgsLE(fp, "422222",
		&size, &imgw, &imgh, &layernum, &layercnt, &layersel))
		return LOADERR_CORRUPTED;
	
	fseek(fp, size - 10, SEEK_CUR);

	//プレビューイメージをスキップ

	if(fseek(fp, 4, SEEK_CUR)
		|| !mFILEread32LE(fp, &size)
		|| fseek(fp, size, SEEK_CUR))
		return LOADERR_CORRUPTED;

	//レイヤ情報サイズ

	if(!mFILEread32LE(fp, &layerinfosize))
		return LOADERR_CORRUPTED;

	//新規イメージ

	if(!drawImage_new(APP_DRAW, imgw, imgh, -1, -1))
		return LOADERR_ALLOC;

	//レイヤ

	ret = _load_ver1_layers(p, fp, layernum, layerinfosize - 35);

	//カレントレイヤ

	APP_DRAW->curlayer = LayerList_getItem_byPos_topdown(APP_DRAW->layerlist, layersel);

	return ret;
}


//==============================
// ver 2 (BigEndian)
//==============================
/*
 * AzPainter(Linux) ver 1.x で保存されたもの。
 * 64x64 タイルイメージ。レイヤカラータイプあり。
 */


/** タイル読み込み */

static int _load_ver2_tile(_load_apd *p,FILE *fp,TileImage *img,uint32_t tilenum)
{
	uint32_t pos,size,maxpos,tilesize;
	uint8_t *ptile;
	TileImageColFunc_getsetTileForSave settilefunc;

	maxpos = img->tilew * img->tileh;
	tilesize = img->tilesize;
	settilefunc = g_tileimage_funcs[img->coltype].setTileForSave;

	mPopupProgressThreadBeginSubStep(p->prog, 6, tilenum);

	for(; tilenum; tilenum--)
	{
		//タイル位置、圧縮サイズ
		
		if(!mFILEread32BE(fp, &pos)
			|| !mFILEread32BE(fp, &size)
			|| pos >= maxpos
			|| size > tilesize)
			return LOADERR_CORRUPTED;

		//タイル確保

		ptile = TileImage_getTileAlloc_atpos(img, pos % img->tilew, pos / img->tilew, FALSE);
		if(!ptile) return LOADERR_ALLOC;

		//読み込み

		if(size == tilesize)
		{
			//無圧縮
			
			if(fread(p->tmpbuf, 1, size, fp) != size)
				return LOADERR_CORRUPTED;
		}
		else
		{
			//zlib
			
			mZlibDecodeReset(p->dec);

			if(mZlibDecodeReadOnce(p->dec, p->tmpbuf, tilesize, size))
				return LOADERR_DECODE;
		}

		//変換

		(settilefunc)(ptile, p->tmpbuf);

		//progpress

		mPopupProgressThreadIncSubStep(p->prog);
	}

	return LOADERR_OK;
}

/** レイヤ読み込み */

static int _load_ver2_layers(_load_apd *p,FILE *fp)
{
	uint32_t col,flags,tilenum;
	int32_t offx,offy;
	uint16_t layernum,parent,tilew,tileh;
	uint8_t treeflags,info[4];
	int ret;
	LayerItem *item;
	TileImage *img;
	char *name;
	TileImageInfo imginfo;

	//レイヤ数

	if(!mFILEread16BE(fp, &layernum))
		return LOADERR_ALLOC;

	//タイルバッファ

	p->tmpbuf = (uint8_t *)mMalloc(64 * 64 * 2 * 4, FALSE);
	if(!p->tmpbuf) return LOADERR_ALLOC;

	//

	mPopupProgressThreadSetMax(p->prog, layernum * 6);

	while(1)
	{
		//ツリー情報
		/* parent: 親のレイヤ位置 [0xfffe で終了、0xffff でルート] */
	
		if(!mFILEread16BE(fp, &parent) || parent == 0xfffe)
			break;

		//フラグ
		/* 0bit: フォルダ
		 * 1bit: カレントレイヤ */
		
		if(!mFILEreadByte(fp, &treeflags))
			return LOADERR_CORRUPTED;

		//レイヤ追加

		item = LayerList_addLayer_pos(APP_DRAW->layerlist, (parent == 0xffff)? -1: parent, -1);
		if(!item) return LOADERR_ALLOC;

		//カレントレイヤ

		if(treeflags & 2)
			APP_DRAW->curlayer = item;

		//レイヤ情報
		/* info[4]: [0]coltype [1]opacity [2]blendmode [3]amask */

		if(mFILEreadStr_variableLen(fp, &name) < 0
			|| fread(info, 1, 4, fp) != 4	//4byte 読み込み
			|| info[0] > 3					//カラータイプ
			|| !mFILEread32BE(fp, &col)		//レイヤ色
			|| !mFILEread32BE(fp, &flags))	//フラグ
			return LOADERR_CORRUPTED;

		//

		item->name = name;
		item->opacity = info[1];
		item->alphamask = (info[3] > 3)? 0: info[3];
		item->blendmode = (info[2] >= 17)? 0: info[2]; //合成モードは同じ

		//フラグ

		item->flags = 0;
		if(flags & 1) item->flags |= LAYERITEM_F_VISIBLE;
		if(flags & 2) item->flags |= LAYERITEM_F_FOLDER_EXPAND;
		if(flags & 4) item->flags |= LAYERITEM_F_LOCK;
		if(flags & 8) item->flags |= LAYERITEM_F_FILLREF;
		if(flags & (1<<5)) item->flags |= LAYERITEM_F_MASK_UNDER;
		if(flags & (1<<7)) item->flags |= LAYERITEM_F_CHECKED;

		//

		if(treeflags & 1)
		{
			//フォルダ

			mPopupProgressThreadAddPos(p->prog, 6);
		}
		else
		{
			//----- イメージ

			//情報

			if(!mFILEreadArgsBE(fp, "44224",
				&offx, &offy, &tilew, &tileh, &tilenum))
				return LOADERR_CORRUPTED;

			//イメージ作成

			imginfo.offx = offx;
			imginfo.offy = offy;
			imginfo.tilew = tilew;
			imginfo.tileh = tileh;

			img = TileImage_newFromInfo(info[0], &imginfo);
			if(!img) return LOADERR_ALLOC;

			LayerItem_replaceImage(item, img);
			LayerItem_setLayerColor(item, col);

			//タイル

			if(tilenum == 0)
				//空イメージ
				mPopupProgressThreadAddPos(p->prog, 6);
			else
			{
				ret = _load_ver2_tile(p, fp, img, tilenum);
				if(ret != LOADERR_OK) return ret;
			}
		}
	}

	//カレントレイヤがセットされていない場合

	if(!APP_DRAW->curlayer)
		APP_DRAW->curlayer = LayerList_getItem_top(APP_DRAW->layerlist);

	return LOADERR_OK;
}

/** ver 2 読み込み (BE) */

static int _load_ver2(_load_apd *p,FILE *fp)
{
	uint32_t size;
	uint16_t info[3];
	uint8_t sig,bImgInfo = FALSE;
	int i;

	while(mFILEreadByte(fp, &sig))
	{
		if(sig == 'L')
		{
			//レイヤ

			if(!bImgInfo) return LOADERR_CORRUPTED;

			return _load_ver2_layers(p, fp);
		}
		else
		{
			//------ サイズ + データ
		
			if(!mFILEread32BE(fp, &size))
				return LOADERR_CORRUPTED;

			//イメージ情報
			
			if(sig == 'I' && !bImgInfo)
			{
				/* 0:imgw 1:imgh 2:dpi */

				for(i = 0; i < 3; i++)
				{
					if(!mFILEread16BE(fp, info + i))
						return LOADERR_CORRUPTED;
				}

				//新規イメージ

				if(!drawImage_new(APP_DRAW, info[0], info[1], info[2], -1))
					return LOADERR_ALLOC;

				size -= 6;
				bImgInfo = TRUE;
			}

			//次へ

			if(fseek(fp, size, SEEK_CUR))
				return LOADERR_CORRUPTED;
		}
	}

	return LOADERR_OK;
}


//==============================
// 共通
//==============================


/** 読み込みメイン */

static int _load_main(_load_apd *p,FILE *fp)
{
	uint8_t ver;
	int err;

	//ヘッダ

	if(!mFILEreadCompareStr(fp, "AZPDATA"))
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

/** APD v1/v2 読み込み
 *
 * @return LOADERR_* */

int drawFile_load_apd_v1v2(const char *filename,mPopupProgress *prog)
{
	_load_apd dat;
	FILE *fp;
	int err;

	mMemzero(&dat, sizeof(_load_apd));
	dat.prog = prog;

	fp = mFILEopenUTF8(filename, "rb");
	if(!fp) return LOADERR_OPENFILE;

	err = _load_main(&dat, fp);

	fclose(fp);

	return err;
}
