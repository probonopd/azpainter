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
 * サブ関数、レイヤの読み書き実処理
 *****************************************/

#include <stdio.h>	//FILE*
#include <string.h>	//strlen

#include "mDef.h"
#include "mRectBox.h"
#include "mUndo.h"
#include "mZlib.h"
#include "mUtil.h"

#include "defDraw.h"

#include "LayerList.h"
#include "LayerItem.h"
#include "defTileImage.h"
#include "TileImage.h"

#include "Undo.h"
#include "UndoMaster.h"
#include "UndoItem.h"


//-----------------------

/** レイヤ情報 (文字列は除く) */

typedef struct
{
	uint8_t coltype,	//255 でフォルダ
		opacity,
		alphamask,
		blendmode;
	uint32_t col,
		flags;
}UndoLayerInfo;

/** レイヤ情報 (文字列含む) */

typedef struct
{
	UndoLayerInfo i;
	char *name,
		*texture_path;
}UndoLayerInfo_str;


#define UNDOLAYERINFO_COLTYPE_FOLDER 255

//-----------------------

#define _UNDO g_app_undo

//-----------------------



//==================================
// レイヤ番号取得/セット
//==================================


/** レイヤ番号からレイヤ取得 */

LayerItem *UndoItem_getLayerFromNo(int no)
{
	return LayerList_getItem_byPos_topdown(APP_DRAW->layerlist, no);
}

/** 親と相対位置のレイヤ番号からレイヤ取得 */

LayerItem *UndoItem_getLayerFromNo_forParent(int parent,int pos)
{
	LayerItem *item[2];

	LayerList_getItems_fromPos(APP_DRAW->layerlist, item, parent, pos);

	return item[1];
}

/** val にカレントレイヤの番号をセット */

void UndoItem_setval_curlayer(UndoItem *p,int valno)
{
	p->val[valno] = LayerList_getItemPos(APP_DRAW->layerlist, APP_DRAW->curlayer);
}

/** val に指定レイヤの番号 (上から順) をセット */

void UndoItem_setval_layerno(UndoItem *p,int valno,LayerItem *item)
{
	p->val[valno] = LayerList_getItemPos(APP_DRAW->layerlist, item);
}

/** val に指定レイヤの親番号とレイヤ番号をセット */

void UndoItem_setval_layerno_forParent(UndoItem *p,int valno,LayerItem *item)
{
	LayerList_getItemPos_forParent(APP_DRAW->layerlist, item,
		p->val + valno, p->val + valno + 1);
}


//==============================
// レイヤ読み書き sub
//==============================


/** 文字列書込み */

static mBool _write_string(UndoItem *p,const char *text)
{
	uint16_t len;

	len = (text)? strlen(text): 0;

	return (UndoItem_write(p, &len, 2)
		&& UndoItem_write(p, text, len));
}

/** 文字列読み込み */

static mBool _read_string(UndoItem *p,char **dst)
{
	char *pc;
	uint16_t len;

	*dst = NULL;

	//長さ

	if(!UndoItem_read(p, &len, 2)) return FALSE;

	//空文字列

	if(len == 0) return TRUE;

	//文字列

	pc = (char *)mMalloc(len + 1, FALSE);
	if(!pc) return FALSE;

	if(!UndoItem_read(p, pc, len)) return FALSE;

	pc[len] = 0;

	*dst = pc;

	return TRUE;
}

/** レイヤ情報解放 */

static void _free_layerinfo(UndoLayerInfo_str *p)
{
	mFree(p->name);
	mFree(p->texture_path);
}

/** レイヤ情報読み込み */

static mBool _read_layerinfo(UndoItem *p,UndoLayerInfo_str *info)
{
	if(_read_string(p, &info->name)
		&& _read_string(p, &info->texture_path)
		&& UndoItem_read(p, &info->i, sizeof(UndoLayerInfo)))
		return TRUE;
	else
	{
		_free_layerinfo(info);
		return FALSE;
	}
}

/** レイヤ情報をセット */

static void _set_layerinfo(LayerItem *item,UndoLayerInfo_str *info)
{
	item->coltype = (info->i.coltype == UNDOLAYERINFO_COLTYPE_FOLDER)? 0: info->i.coltype;
	item->blendmode = info->i.blendmode;
	item->opacity = info->i.opacity;
	item->alphamask = info->i.alphamask;
	item->col = info->i.col;
	item->flags = info->i.flags;

	mStrdup_ptr(&item->name, info->name);

	//テクスチャ

	LayerItem_setTexture(item, info->texture_path);
}

/** レイヤ情報読み込み＆レイヤ作成 */

static LayerItem *_create_restore_layer(UndoItem *p,int parent_pos,int rel_pos,int *is_folder)
{
	UndoLayerInfo_str info;
	LayerItem *item;

	//レイヤ情報読み込み

	if(!_read_layerinfo(p, &info))
		return NULL;

	//レイヤ作成

	item = LayerList_addLayer_pos(APP_DRAW->layerlist, parent_pos, rel_pos);

	//情報をセット

	if(item) _set_layerinfo(item, &info);

	_free_layerinfo(&info);

	//フォルダか

	*is_folder = (info.i.coltype == UNDOLAYERINFO_COLTYPE_FOLDER);

	return item;
}


//==============================
// レイヤ情報のみ
//==============================


/** 単体レイヤ情報書き込み
 *
 * 新規レイヤ (空) のリドゥデータセット時 */

mBool UndoItem_setdat_layerInfo(UndoItem *p)
{
	LayerItem *li;
	int size;
	mBool ret;

	//レイヤ

	li = UndoItem_getLayerFromNo_forParent(p->val[0], p->val[1]);
	if(!li) return FALSE;

	//書き込み

	size = 2 + mStrlen(li->name) + 2 + mStrlen(li->texture_path) + sizeof(UndoLayerInfo);

	if(!UndoItem_alloc(p, size)
		|| !UndoItem_openWrite(p))
		return FALSE;

	ret = UndoItem_writeLayerInfo(p, li);

	UndoItem_closeWrite(p);

	return ret;
}

/** 新規レイヤ (空) の復元 */

mBool UndoItem_restore_newLayerEmpty(UndoItem *p)
{
	LayerItem *item;
	TileImage *img;
	int is_folder;

	if(!UndoItem_openRead(p)) return FALSE;

	//レイヤ情報読み込み＆作成

	item = _create_restore_layer(p, p->val[0], p->val[1], &is_folder);
	if(!item) goto ERR;

	//イメージ作成

	if(!is_folder)
	{
		img = TileImage_new(item->coltype, 1, 1);
		if(!img) goto ERR;

		LayerItem_replaceImage(item, img);

		TileImage_setImageColor(img, item->col);
	}

	return TRUE;

ERR:
	UndoItem_closeRead(p);
	return FALSE;
}


//==============================
// レイヤ読み書き 開始/終了
//==============================
/*
 * - レイヤ書き込み先は常にファイル。
 * - 読み書きを開始した場合、処理失敗時にも必ず終了させること。
 */


/** レイヤ書き込み開始 */

mBool UndoItem_beginWriteLayer(UndoItem *p)
{
	//zlib
	
	_UNDO->zenc = mZlibEncodeNew_simple(16 * 1024, 4);
	if(!_UNDO->zenc) return FALSE;

	//書き込み開く

	if(!UndoItem_alloc(p, UNDO_ALLOC_FILE)
		|| !UndoItem_openWrite(p))
	{
		mZlibEncodeFree(_UNDO->zenc);
		return FALSE;
	}

	mZlibEncodeSetIO_stdio(_UNDO->zenc, _UNDO->writefp);

	return TRUE;
}

/** レイヤ書き込み終了 */

void UndoItem_endWriteLayer(UndoItem *p)
{
	UndoItem_closeWrite(p);

	mZlibEncodeFree(_UNDO->zenc);
}

/** レイヤ読み込み開始 */

mBool UndoItem_beginReadLayer(UndoItem *p)
{
	//zlib

	_UNDO->zdec = mZlibDecodeNew(16 * 1024, 15);
	if(!_UNDO->zdec) return FALSE;

	//開く

	if(!UndoItem_openRead(p))
	{
		mZlibDecodeFree(_UNDO->zdec);
		return FALSE;
	}

	mZlibDecodeSetIO_stdio(_UNDO->zdec, _UNDO->readfp);

	return TRUE;
}

/** レイヤ読み込み終了 */

void UndoItem_endReadLayer(UndoItem *p)
{
	UndoItem_closeRead(p);

	mZlibDecodeFree(_UNDO->zdec);
}


//==================================
// レイヤ書き込み
//==================================


/** レイヤ情報とイメージ書き込み */

mBool UndoItem_writeLayerInfoAndImage(UndoItem *p,LayerItem *item)
{
	return (UndoItem_writeLayerInfo(p, item)
		&& UndoItem_writeTileImage(p, item->img));
}

/** レイヤ情報書き込み */

mBool UndoItem_writeLayerInfo(UndoItem *p,LayerItem *li)
{
	UndoLayerInfo info;

	//レイヤ名、テクスチャパス

	if(!_write_string(p, li->name)
		|| !_write_string(p, li->texture_path))
		return FALSE;

	//情報

	info.coltype = (li->img)? li->coltype: UNDOLAYERINFO_COLTYPE_FOLDER;
	info.opacity = li->opacity;
	info.blendmode = li->blendmode;
	info.alphamask = li->alphamask;
	info.col = li->col;
	info.flags = li->flags;

	return UndoItem_write(p, &info, sizeof(UndoLayerInfo));
}

/** タイルイメージ書き込み */

/* [TileImageInfo]
 * [圧縮サイズ]
 * [タイルイメージ]
 *   2byte: タイルX位置 (X,Y が 0xffff で終了)
 *   2byte: タイルY位置
 *   タイルデータ
 */

mBool UndoItem_writeTileImage(UndoItem *p,TileImage *img)
{
	TileImageInfo info;
	mZlibEncode *enc = _UNDO->zenc;
	uint8_t **pptile;
	int ix,iy,tw,th;
	uint16_t tpos[2];

	//NULL = フォルダなので、成功とする
	
	if(!img) return TRUE;

	//タイルイメージ情報

	TileImage_getInfo(img, &info);

	if(!UndoItem_write(p, &info, sizeof(TileImageInfo)))
		return FALSE;

	//圧縮サイズ (仮)

	if(!UndoItem_writeEncSize_temp())
		return FALSE;

	//各タイル

	mZlibEncodeReset(enc);

	pptile = img->ppbuf;
	tw = img->tilew;
	th = img->tileh;

	for(iy = 0; iy < th; iy++)
	{
		for(ix = 0; ix < tw; ix++, pptile++)
		{
			if(*pptile)
			{
				//タイル位置、タイルデータ

				tpos[0] = ix;
				tpos[1] = iy;

				if(!mZlibEncodeSend(enc, tpos, 4)
					|| !mZlibEncodeSend(enc, *pptile, img->tilesize))
					return FALSE;
			}
		}
	}

	//終了

	tpos[0] = 0xffff;
	tpos[1] = 0xffff;

	if(!mZlibEncodeSend(enc, tpos, 4)
		|| !mZlibEncodeFlushEnd(enc))
		return FALSE;

	//圧縮サイズ

	UndoItem_writeEncSize_real(mZlibEncodeGetWriteSize(enc));

	return TRUE;
}


//==================================
// レイヤ復元
//==================================


/** レイヤを一つ復元
 *
 * @param rcupdate  更新範囲が追加される */

mBool UndoItem_readLayer(UndoItem *p,int parent_pos,int rel_pos,mRect *rcupdate)
{
	LayerItem *item;
	mRect rc;
	int is_folder;

	//レイヤ情報読み込み＆作成

	item = _create_restore_layer(p, parent_pos, rel_pos, &is_folder);
	if(!item) return FALSE;

	//イメージ作成 & 読み込み

	if(!is_folder)
	{
		if(!UndoItem_readLayerImage(p, item)) return FALSE;

		//更新範囲
		/* [!] 複数レイヤの復元時は、
		 * レイヤが非表示状態でも範囲として必要な場合があるので (結合時など)
		 * LayerItem_getVisibleImageRect() で取得しないこと。 */

		if(TileImage_getHaveImageRect_pixel(item->img, &rc, NULL))
			mRectUnion(rcupdate, &rc);
	}

	return TRUE;
}

/** 既存のレイヤに情報とイメージを上書き復元
 *
 * @param rcupdate イメージのタイルがある範囲を追加 */

mBool UndoItem_readLayerOverwrite(UndoItem *p,LayerItem *item,mRect *rcupdate)
{
	UndoLayerInfo_str info;
	mRect rc;

	//レイヤ情報 読み込み＆セット

	if(!_read_layerinfo(p, &info)) return FALSE;

	_set_layerinfo(item, &info);

	_free_layerinfo(&info);

	//イメージ

	if(!UndoItem_readLayerImage(p, item))
		return FALSE;

	//範囲追加

	if(TileImage_getHaveImageRect_pixel(item->img, &rc, NULL))
		mRectUnion(rcupdate, &rc);

	return TRUE;
}

/** レイヤイメージの復元
 *
 * - item の各情報は設定済みであること。
 * - 既存のイメージを置き換える場合もあり。 */

mBool UndoItem_readLayerImage(UndoItem *p,LayerItem *item)
{
	TileImageInfo info;
	TileImage *img;
	mZlibDecode *dec = _UNDO->zdec;
	uint8_t *tile;
	uint16_t tpos[2];
	uint32_t encsize;

	if(!item) return FALSE;

	//イメージ情報

	if(!UndoItem_read(p, &info, sizeof(TileImageInfo)))
		return FALSE;

	//既存イメージは削除

	TileImage_free(item->img);
	item->img = NULL;

	//イメージ作成

	img = TileImage_newFromInfo(item->coltype, &info);
	if(!img) return FALSE;

	item->img = img;

	TileImage_setImageColor(img, item->col);

	//------ タイル読み込み

	//圧縮サイズ

	if(!UndoItem_read(p, &encsize, 4)) return FALSE;

	mZlibDecodeReset(dec);
	mZlibDecodeSetInSize(dec, encsize);

	//

	while(1)
	{
		//タイル位置

		if(mZlibDecodeRead(dec, tpos, 4)) return FALSE;

		if(tpos[0] == 0xffff && tpos[1] == 0xffff) break;

		//タイル作成

		tile = TileImage_getTileAlloc_atpos(img, tpos[0], tpos[1], FALSE);
		if(!tile) return FALSE;

		//タイルデータ

		if(mZlibDecodeRead(dec, tile, img->tilesize))
			return FALSE;
	}

	if(mZlibDecodeReadEnd(dec)) return FALSE;

	return TRUE;
}

