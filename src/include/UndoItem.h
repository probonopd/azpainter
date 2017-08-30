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

/********************************
 * アンドゥアイテム
 ********************************/

#ifndef UNDOITEM_H
#define UNDOITEM_H

#include "mListDef.h"

#define UNDO_VAL_NUM  5

typedef struct _LayerItem LayerItem;
typedef struct _TileImage TileImage;
typedef struct _UndoUpdateInfo UndoUpdateInfo;

/** UNDO アイテム */

typedef struct
{
	mListItem i;

	uint8_t *buf;	//データのバッファ
	uint32_t size;	//バッファのデータサイズ
	int type,		//データの種類
		fileno,		//ファイル時、ファイル名の番号 (-1 でバッファ)
		val[UNDO_VAL_NUM];	//データ値
}UndoItem;

/** アンドゥ種類 */

enum
{
	UNDO_TYPE_TILES_IMAGE,				//タイル単位のイメージ
	UNDO_TYPE_LAYER_NEW,				//レイヤ追加
	UNDO_TYPE_LAYER_COPY,				//レイヤ複製
	UNDO_TYPE_LAYER_DELETE,				//レイヤ削除
	UNDO_TYPE_LAYER_CLEARIMG,			//レイヤクリア
	UNDO_TYPE_LAYER_COLORTYPE,			//レイヤ、カラータイプ変更
	UNDO_TYPE_LAYER_DROP_AND_COMBINE,	//下レイヤに移す、下レイヤに結合
	UNDO_TYPE_LAYER_COMBINE_ALL,		//すべての表示レイヤ結合
	UNDO_TYPE_LAYER_COMBINE_FOLDER,		//フォルダ内のレイヤ結合
	UNDO_TYPE_LAYER_FLAGS,				//レイヤ、フラグの変更
	UNDO_TYPE_LAYER_FULL_EDIT,			//レイヤ全体の左右/上下反転、回転
	UNDO_TYPE_LAYER_MOVE_OFFSET,		//レイヤのオフセット位置移動
	UNDO_TYPE_LAYER_MOVE_LIST,			//レイヤの順番移動
	UNDO_TYPE_LAYER_MOVE_LIST_MULTI,	//複数レイヤの順番移動
	UNDO_TYPE_RESIZECANVAS_MOVEOFFSET,	//キャンバスサイズ変更 (オフセット移動のみ)
	UNDO_TYPE_RESIZECANVAS_CROP,		//キャンバスサイズ変更 (範囲外切り取り)
	UNDO_TYPE_SCALECANVAS				//キャンバス拡大縮小
};

/** データ確保タイプ */

enum
{
	UNDO_ALLOC_FILE = -1,			//常にファイル
	UNDO_ALLOC_MEM_VARIABLE = -2	//メモリ優先、可変サイズ
};


/*---------------*/

/* UndoItem.c */

mBool UndoItem_setdat_movelayerlist_multi(UndoItem *p,int num,int16_t *buf);

mBool UndoItem_setdat_layer(UndoItem *p,LayerItem *item);
mBool UndoItem_restore_layer(UndoItem *p,int parent,int pos,mRect *rc);

mBool UndoItem_setdat_layerImage(UndoItem *p,LayerItem *item);
mBool UndoItem_restore_layerImage(UndoItem *p,LayerItem *item);

mBool UndoItem_setdat_layerImage_double(UndoItem *p,LayerItem *item);
mBool UndoItem_restore_layerImage_double(UndoItem *p,LayerItem *item);
mBool UndoItem_setdat_layerForCombine(UndoItem *p,int settype);
mBool UndoItem_restore_layerForCombine(UndoItem *p,int runtype,LayerItem *item,mRect *rcupdate);

mBool UndoItem_setdat_layerForFolder(UndoItem *p,LayerItem *item);
mBool UndoItem_setdat_layerAll(UndoItem *p);
mBool UndoItem_restore_layerMulti(UndoItem *p,mRect *rcupdate);

mBool UndoItem_setdat_layerImage_all(UndoItem *p);
mBool UndoItem_restore_layerImage_all(UndoItem *p);

//

mBool UndoItem_runLayerNew(UndoItem *p,UndoUpdateInfo *info,int runtype);
mBool UndoItem_runLayerCopy(UndoItem *p,UndoUpdateInfo *info,int runtype);
mBool UndoItem_runLayerDelete(UndoItem *p,UndoUpdateInfo *info,int runtype);
mBool UndoItem_runLayerClearImage(UndoItem *p,UndoUpdateInfo *info,int runtype);
mBool UndoItem_runLayerColorType(UndoItem *p,UndoUpdateInfo *info,int runtype);
mBool UndoItem_runLayerDropAndCombine(UndoItem *p,UndoUpdateInfo *info,int runtype);
mBool UndoItem_runLayerCombineAll(UndoItem *p,UndoUpdateInfo *info,int runtype);
mBool UndoItem_runLayerCombineFolder(UndoItem *p,UndoUpdateInfo *info,int runtype);
mBool UndoItem_runLayerFullEdit(UndoItem *p,UndoUpdateInfo *info);
mBool UndoItem_runLayerMoveOffset(UndoItem *p,UndoUpdateInfo *info);
mBool UndoItem_runLayerMoveList(UndoItem *p,UndoUpdateInfo *info);
mBool UndoItem_runLayerMoveList_multi(UndoItem *p,UndoUpdateInfo *info,int runtype);

mBool UndoItem_runCanvasResizeAndScale(UndoItem *p,UndoUpdateInfo *info);

/* UndoItem_sub.c */

LayerItem *UndoItem_getLayerFromNo(int no);
LayerItem *UndoItem_getLayerFromNo_forParent(int parent,int pos);

void UndoItem_setval_curlayer(UndoItem *p,int valno);
void UndoItem_setval_layerno(UndoItem *p,int valno,LayerItem *item);
void UndoItem_setval_layerno_forParent(UndoItem *p,int valno,LayerItem *item);

mBool UndoItem_setdat_layerInfo(UndoItem *p);
mBool UndoItem_restore_newLayerEmpty(UndoItem *p);

mBool UndoItem_beginWriteLayer(UndoItem *p);
void UndoItem_endWriteLayer(UndoItem *p);
mBool UndoItem_beginReadLayer(UndoItem *p);
void UndoItem_endReadLayer(UndoItem *p);

mBool UndoItem_writeLayerInfoAndImage(UndoItem *p,LayerItem *item);
mBool UndoItem_writeLayerInfo(UndoItem *p,LayerItem *li);
mBool UndoItem_writeTileImage(UndoItem *p,TileImage *img);

mBool UndoItem_readLayer(UndoItem *p,int parent_pos,int rel_pos,mRect *rcupdate);
mBool UndoItem_readLayerOverwrite(UndoItem *p,LayerItem *item,mRect *rcupdate);
mBool UndoItem_readLayerImage(UndoItem *p,LayerItem *item);

/* UndoItem_tileimg.c */

mBool UndoItem_setdat_smallimage(UndoItem *p,TileImage *img);
mBool UndoItem_restore_smallimage(UndoItem *p);

mBool UndoItem_setdat_tileimage(UndoItem *p,TileImageInfo *info);
mBool UndoItem_setdat_tileimage_reverse(UndoItem *dst,UndoItem *src,int settype);
mBool UndoItem_restore_tileimage(UndoItem *p,int runtype);

/* UndoItem_base.c */

void UndoItem_free(UndoItem *p);
mBool UndoItem_alloc(UndoItem *p,int size);

uint8_t *UndoItem_readData(UndoItem *p,int size);
mBool UndoItem_copyData(UndoItem *src,UndoItem *dst,int size);

mBool UndoItem_openWrite(UndoItem *p);
mBool UndoItem_write(UndoItem *p,const void *buf,int size);
mBool UndoItem_closeWrite(UndoItem *p);

mBool UndoItem_writeEncSize_temp();
void UndoItem_writeEncSize_real(uint32_t size);

mBool UndoItem_openRead(UndoItem *p);
mBool UndoItem_read(UndoItem *p,void *buf,int size);
void UndoItem_readSeek(UndoItem *p,int seek);
void UndoItem_closeRead(UndoItem *p);

#endif
