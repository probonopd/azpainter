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
 * Undo
 *****************************************/

#include <stdio.h>
#include <string.h>

#include "mDef.h"
#include "mRectBox.h"
#include "mUndo.h"

#include "defDraw.h"

#include "LayerList.h"
#include "LayerItem.h"
#include "defTileImage.h"
#include "TileImage.h"

#include "Undo.h"
#include "UndoItem.h"
#include "UndoMaster.h"


//--------------------

UndoMaster *g_app_undo = NULL;

#define _UNDO  g_app_undo

//--------------------


//========================
// sub
//========================


/** アイテム破棄ハンドラ */

static void _destroy_item_handle(mListItem *item)
{
	UndoItem_free((UndoItem *)item);
}

/** ハンドラ:アイテム作成 */

static mListItem *_create_handle(mUndo *p)
{
	UndoItem *pi;

	//作成

	pi = (UndoItem *)mMalloc(sizeof(UndoItem), TRUE);

	if(pi)
	{
		pi->i.destroy = _destroy_item_handle;
		pi->fileno = -1;

		/* fileno を -1 にしておかないと、アイテム破棄時に
		 * 番号 0 のアンドゥファイルが削除される。 */
	}

	//変更フラグ
	/* アンドゥ追加、アンドゥ/リドゥ実行時は
	 * イメージが変更されたということなので、フラグ ON */

	_UNDO->change = TRUE;

	return (mListItem *)pi;
}

/** ハンドラ: アンドゥ <=> リドゥ のデータセット */

static mBool _setreverse_handle(mUndo *undo,mListItem *dstitem,mListItem *srcitem,int settype)
{
	UndoItem *dst,*src;

	dst = (UndoItem *)dstitem;
	src = (UndoItem *)srcitem;

	//ソースの情報コピー

	dst->type = src->type;
	memcpy(dst->val, src->val, sizeof(int) * UNDO_VAL_NUM);

	//

	switch(src->type)
	{
		//タイルイメージ
		case UNDO_TYPE_TILES_IMAGE:
			return UndoItem_setdat_tileimage_reverse(dst, src, settype);

		//レイヤ追加
		case UNDO_TYPE_LAYER_NEW:
			if(settype == MUNDO_TYPE_REDO)
			{
				if(src->val[2])
				{
					//イメージあり
					return UndoItem_setdat_layer(dst,
						UndoItem_getLayerFromNo_forParent(src->val[0], src->val[1]));
				}
				else
					//イメージなし
					return UndoItem_setdat_layerInfo(dst);
			}
			break;

		//レイヤ削除
		case UNDO_TYPE_LAYER_DELETE:
			if(settype == MUNDO_TYPE_UNDO)
				return UndoItem_setdat_layerForFolder(dst, UndoItem_getLayerFromNo(src->val[0]));
			break;

		//レイヤイメージクリア
		case UNDO_TYPE_LAYER_CLEARIMG:
			if(settype == MUNDO_TYPE_UNDO)
				return UndoItem_setdat_layerImage(dst, UndoItem_getLayerFromNo(src->val[0]));
			break;

		//レイヤ、カラータイプ変更
		case UNDO_TYPE_LAYER_COLORTYPE:
			if(settype == MUNDO_TYPE_UNDO)
				return UndoItem_setdat_layerImage(dst, UndoItem_getLayerFromNo(src->val[0]));
			break;

		//下レイヤに移す/結合
		case UNDO_TYPE_LAYER_DROP_AND_COMBINE:
			if(src->val[0])
			{
				return UndoItem_setdat_layerImage_double(dst,
					UndoItem_getLayerFromNo_forParent(src->val[1], src->val[2]));
			}
			else
				return UndoItem_setdat_layerForCombine(dst, settype);
			break;

		//すべてのレイヤ結合
		case UNDO_TYPE_LAYER_COMBINE_ALL:
			if(settype == MUNDO_TYPE_REDO)
				return UndoItem_setdat_layer(dst, LayerList_getItem_top(APP_DRAW->layerlist));
			else
				return UndoItem_setdat_layerAll(dst);
			break;

		//フォルダレイヤ結合
		case UNDO_TYPE_LAYER_COMBINE_FOLDER:
			if(settype == MUNDO_TYPE_REDO)
			{
				return UndoItem_setdat_layer(dst,
					UndoItem_getLayerFromNo_forParent(src->val[0], src->val[1]));
			}
			else
			{
				return UndoItem_setdat_layerForFolder(dst,
					UndoItem_getLayerFromNo_forParent(src->val[0], src->val[1]));
			}
			break;

		//レイヤ全体の左右/上下反転、回転
		case UNDO_TYPE_LAYER_FULL_EDIT:
			//回転の場合は逆回転にする
			if(src->val[1] >= 2)
				dst->val[1] = (src->val[1] == 2)? 3: 2;
			break;
	
		//レイヤのオフセット移動
		case UNDO_TYPE_LAYER_MOVE_OFFSET:
			dst->val[0] = -dst->val[0];
			dst->val[1] = -dst->val[1];
			break;

		//レイヤの順番移動
		case UNDO_TYPE_LAYER_MOVE_LIST:
			dst->val[0] = src->val[2];
			dst->val[1] = src->val[3];
			dst->val[2] = src->val[0];
			dst->val[3] = src->val[1];
			break;

		//複数レイヤの順番移動
		case UNDO_TYPE_LAYER_MOVE_LIST_MULTI:
			return UndoItem_copyData(src, dst, src->val[0] * 4 * 2);

		//キャンバスサイズ変更 (オフセット移動)
		case UNDO_TYPE_RESIZECANVAS_MOVEOFFSET:
			dst->val[0] = -src->val[0];
			dst->val[1] = -src->val[1];
			dst->val[2] = APP_DRAW->imgw;
			dst->val[3] = APP_DRAW->imgh;
			break;

		//キャンバスサイズ変更 (切り取り)
		//キャンバス拡大縮小
		case UNDO_TYPE_RESIZECANVAS_CROP:
		case UNDO_TYPE_SCALECANVAS:
			dst->val[0] = APP_DRAW->imgw;
			dst->val[1] = APP_DRAW->imgh;

			if(src->type == UNDO_TYPE_SCALECANVAS)
				dst->val[2] = APP_DRAW->imgdpi;

			return UndoItem_setdat_layerImage_all(dst);
	}

	return TRUE;
}

/** ハンドラ:実行 */

static mBool _run_handle(mUndo *pundo,mListItem *runitem,int runtype)
{
	UndoMaster *undo = (UndoMaster *)pundo;
	UndoItem *item = (UndoItem *)runitem;
	LayerItem *layer;
	UndoUpdateInfo *update;

	update = &undo->update;

	//デフォルトで更新なし

	update->type = UNDO_UPDATE_NONE;
	mRectEmpty(&update->rc);

	//

	switch(item->type)
	{
		//タイルイメージ
		case UNDO_TYPE_TILES_IMAGE:
			update->type = UNDO_UPDATE_RECT;
			update->rc.x1 = item->val[1];
			update->rc.y1 = item->val[2];
			update->rc.x2 = item->val[3];
			update->rc.y2 = item->val[4];
			update->layer = UndoItem_getLayerFromNo(item->val[0]);
		
			return UndoItem_restore_tileimage(item, runtype);

		//レイヤ追加
		case UNDO_TYPE_LAYER_NEW:
			return UndoItem_runLayerNew(item, update, runtype);
	
		//レイヤ複製
		case UNDO_TYPE_LAYER_COPY:
			return UndoItem_runLayerCopy(item, update, runtype);

		//レイヤ削除
		case UNDO_TYPE_LAYER_DELETE:
			return UndoItem_runLayerDelete(item, update, runtype);

		//レイヤイメージクリア
		case UNDO_TYPE_LAYER_CLEARIMG:
			return UndoItem_runLayerClearImage(item, update, runtype);

		//レイヤ、カラータイプ変更
		case UNDO_TYPE_LAYER_COLORTYPE:
			return UndoItem_runLayerColorType(item, update, runtype);

		//下レイヤに移す/結合
		case UNDO_TYPE_LAYER_DROP_AND_COMBINE:
			return UndoItem_runLayerDropAndCombine(item, update, runtype);

		//すべてのレイヤ結合
		case UNDO_TYPE_LAYER_COMBINE_ALL:
			return UndoItem_runLayerCombineAll(item, update, runtype);

		//フォルダレイヤ結合
		case UNDO_TYPE_LAYER_COMBINE_FOLDER:
			return UndoItem_runLayerCombineFolder(item, update, runtype);
	
		//レイヤのオフセット位置移動
		case UNDO_TYPE_LAYER_MOVE_OFFSET:
			return UndoItem_runLayerMoveOffset(item, update);

		//レイヤのフラグ変更 (反転)
		case UNDO_TYPE_LAYER_FLAGS:
			layer = UndoItem_getLayerFromNo(item->val[0]);
			if(!layer) return FALSE;

			layer->flags ^= item->val[1];

			update->type = UNDO_UPDATE_LAYERLIST_ONE;
			update->layer = layer;
			break;

		//レイヤ全体の左右/上下反転、回転
		case UNDO_TYPE_LAYER_FULL_EDIT:
			return UndoItem_runLayerFullEdit(item, update);

		//レイヤの順番移動
		case UNDO_TYPE_LAYER_MOVE_LIST:
			return UndoItem_runLayerMoveList(item, update);

		//複数レイヤの順番移動
		case UNDO_TYPE_LAYER_MOVE_LIST_MULTI:
			return UndoItem_runLayerMoveList_multi(item, update, runtype);

		//キャンバスサイズ変更 (オフセット移動)
		case UNDO_TYPE_RESIZECANVAS_MOVEOFFSET:
			LayerList_moveOffset_rel_all(APP_DRAW->layerlist, item->val[0], item->val[1]);

			update->type = UNDO_UPDATE_CANVAS_RESIZE;
			update->rc.x1 = item->val[2];
			update->rc.y1 = item->val[3];
			break;

		//キャンバスサイズ変更 (切り取り)
		//キャンバス拡大縮小
		case UNDO_TYPE_RESIZECANVAS_CROP:
		case UNDO_TYPE_SCALECANVAS:
			return UndoItem_runCanvasResizeAndScale(item, update);
	}

	return TRUE;
}

/** アンドゥデータ追加 */

static UndoItem *_add_item(int type)
{
	UndoItem *pi;

	pi = (UndoItem *)_create_handle(&_UNDO->undo);
	if(!pi) return NULL;

	mUndoAdd(&_UNDO->undo, M_LISTITEM(pi));

	pi->type = type;

	return pi;
}

/** データセット失敗時 */

static void _failed_set()
{
	mUndoDeleteAll(&_UNDO->undo);
}


//========================
// main
//========================


/** 作業用バッファ解放 */

static void _free_workbuf(UndoMaster *p)
{
	mFree(p->writetmpbuf);
	mFree(p->workbuf1);
	mFree(p->workbuf2);
	mFree(p->workbuf_flags);
}

/** 作成 */

mBool Undo_new()
{
	UndoMaster *p;

	p = (UndoMaster *)mMalloc(sizeof(UndoMaster), TRUE);
	if(!p) return FALSE;

	//バッファ確保

	p->writetmpbuf = (uint8_t *)mMalloc(UNDO_WRITETEMPBUFSIZE, FALSE);
	p->workbuf1 = (uint8_t *)mMalloc(64 * 64 * 8, FALSE);
	p->workbuf2 = (uint8_t *)mMalloc(64 * 64 * 8, FALSE);
	p->workbuf_flags = (uint8_t *)mMalloc(2048 * 2, FALSE);
	
	if(!p->writetmpbuf || !p->workbuf1 || !p->workbuf2 || !p->workbuf_flags)
	{
		_free_workbuf(p);
		mFree(p);
		return FALSE;
	}

	//
	
	g_app_undo = p;

	p->undo.maxnum = 20;
	p->undo.create = _create_handle;
	p->undo.setreverse = _setreverse_handle;
	p->undo.run = _run_handle;

	return TRUE;
}

/** 解放 */

void Undo_free()
{
	if(g_app_undo)
	{
		mUndoDeleteAll(&_UNDO->undo);

		_free_workbuf(g_app_undo);
		mFree(g_app_undo);
	}
}

/** データ更新用フラグを OFF */

void Undo_clearUpdateFlag()
{
	_UNDO->change = FALSE;
}

/** アンドゥ最大回数をセット */

void Undo_setMaxNum(int num)
{
	_UNDO->undo.maxnum = num;
}

/** アンドゥ/リドゥデータがあるか */

mBool Undo_isHave(mBool redo)
{
	return mUndoIsHave(&_UNDO->undo, redo);
}

/** データが変更されているか */

mBool Undo_isChange()
{
	/* フラグは、新規作成時やファイル保存時はクリアされる。
	 * アンドゥの追加、アンドゥ/リドゥ実行が行われた場合、
	 * 状態が変更されたということなので、保存が必要であると判断する。 */

	return _UNDO->change;
}

/** すべて削除 */

void Undo_deleteAll()
{
	mUndoDeleteAll(&_UNDO->undo);
}

/** アンドゥ/リドゥ実行
 *
 * @return 更新を行うかどうか。
 * (実行処理中に失敗した場合は、中途半端な所で止まっている場合が
 *  あるので、実行に失敗しても更新は行う) */

mBool Undo_runUndoRedo(mBool redo,UndoUpdateInfo *info)
{
	int ret;

	if(redo)
		ret = mUndoRunRedo(&_UNDO->undo);
	else
		ret = mUndoRunUndo(&_UNDO->undo);

	if(ret == MUNDO_RUNERR_OK || ret == MUNDO_RUNERR_RUN)
	{
		*info = _UNDO->update;
		return TRUE;
	}
	else
		return FALSE;
}


//========================
// アンドゥ追加
//========================


/** タイルイメージ
 *
 * @param rc 更新されたイメージの範囲 */

void Undo_addTilesImage(TileImageInfo *info,mRect *rc)
{
	UndoItem *pi = _add_item(UNDO_TYPE_TILES_IMAGE);

	//[0] レイヤ番号 [1..4] 更新イメージ範囲

	if(pi)
	{
		UndoItem_setval_curlayer(pi, 0);

		pi->val[1] = rc->x1;
		pi->val[2] = rc->y1;
		pi->val[3] = rc->x2;
		pi->val[4] = rc->y2;

		if(!UndoItem_setdat_tileimage(pi, info))
			_failed_set();
	}
}

/** 新規レイヤ追加
 *
 * レイヤ追加 & カレントレイヤ変更後に実行。
 *
 * @param with_image イメージがあるか */

void Undo_addLayerNew(mBool with_image)
{
	UndoItem *pi = _add_item(UNDO_TYPE_LAYER_NEW);

	//[0][1] レイヤ番号 [2] イメージがあるか

	if(pi)
	{
		UndoItem_setval_layerno_forParent(pi, 0, APP_DRAW->curlayer);

		pi->val[2] = with_image;
	}
}

/** レイヤ複製 */

void Undo_addLayerCopy()
{
	UndoItem *pi = _add_item(UNDO_TYPE_LAYER_COPY);

	if(pi)
		UndoItem_setval_curlayer(pi, 0);
}

/** レイヤ削除 */

void Undo_addLayerDelete()
{
	UndoItem *pi = _add_item(UNDO_TYPE_LAYER_DELETE);

	if(pi)
	{
		UndoItem_setval_curlayer(pi, 0);

		if(!UndoItem_setdat_layerForFolder(pi, APP_DRAW->curlayer))
			_failed_set();
	}
}

/** レイヤイメージクリア */

void Undo_addLayerClearImage()
{
	UndoItem *pi = _add_item(UNDO_TYPE_LAYER_CLEARIMG);

	if(pi)
	{
		UndoItem_setval_curlayer(pi, 0);

		if(!UndoItem_setdat_layerImage(pi, APP_DRAW->curlayer))
			_failed_set();
	}
}

/** レイヤ、カラータイプ変更 */

void Undo_addLayerSetColorType(LayerItem *item,int type,mBool bLumtoA)
{
	UndoItem *pi = _add_item(UNDO_TYPE_LAYER_COLORTYPE);

	/* [0] レイヤ番号
	 * [1] 元のカラータイプ
	 * [2] 変換後のカラータイプ
	 * [3] 輝度からアルファ値変換 */

	if(pi)
	{
		UndoItem_setval_layerno(pi, 0, item);

		pi->val[1] = item->coltype;
		pi->val[2] = type;
		pi->val[3] = bLumtoA;

		if(!UndoItem_setdat_layerImage(pi, item))
			_failed_set();
	}
}

/** 下レイヤに移す/結合 */

void Undo_addLayerDropAndCombine(mBool drop)
{
	UndoItem *pi = _add_item(UNDO_TYPE_LAYER_DROP_AND_COMBINE);

	//[0] 下レイヤに移すか [1][2] レイヤ番号

	if(pi)
	{
		pi->val[0] = drop;
		
		UndoItem_setval_layerno_forParent(pi, 1, APP_DRAW->curlayer);

		if(drop)
			UndoItem_setdat_layerImage_double(pi, APP_DRAW->curlayer);
		else
			UndoItem_setdat_layerForCombine(pi, MUNDO_TYPE_UNDO);
	}
}

/** すべてのレイヤの結合 */

void Undo_addLayerCombineAll()
{
	UndoItem *pi = _add_item(UNDO_TYPE_LAYER_COMBINE_ALL);

	if(pi)
	{
		if(!UndoItem_setdat_layerAll(pi))
			_failed_set();
	}
}

/** フォルダレイヤの結合 */

void Undo_addLayerCombineFolder()
{
	UndoItem *pi = _add_item(UNDO_TYPE_LAYER_COMBINE_FOLDER);

	//[0] レイヤ番号

	if(pi)
	{
		UndoItem_setval_layerno_forParent(pi, 0, APP_DRAW->curlayer);

		if(!UndoItem_setdat_layerForFolder(pi, APP_DRAW->curlayer))
			_failed_set();
	}
}

/** レイヤのフラグ変更 */

void Undo_addLayerFlags(LayerItem *item,uint32_t flags)
{
	UndoItem *pi = _add_item(UNDO_TYPE_LAYER_FLAGS);

	//[0]レイヤ番号 [1]フラグ

	if(pi)
	{
		UndoItem_setval_layerno(pi, 0, item);
		pi->val[1] = flags;
	}
}

/** レイヤ全体の左右/上下反転、回転 */

void Undo_addLayerFullEdit(int type)
{
	UndoItem *pi = _add_item(UNDO_TYPE_LAYER_FULL_EDIT);

	//[0]レイヤ番号 [1]タイプ

	if(pi)
	{
		UndoItem_setval_curlayer(pi, 0);
		
		pi->val[1] = (type < 2)? type: !(type - 2) + 2;
	}
}

/** レイヤのオフセット位置移動
 *
 * @param item NULL ですべて */

void Undo_addLayerMoveOffset(int mx,int my,LayerItem *item)
{
	UndoItem *pi = _add_item(UNDO_TYPE_LAYER_MOVE_OFFSET);

	/* [0] x 相対移動数
	 * [1] y 相対移動数
	 * [2] レイヤ番号 (-1 ですべて) */

	if(pi)
	{
		pi->val[0] = -mx;
		pi->val[1] = -my;

		if(item)
			UndoItem_setval_layerno(pi, 2, item);
		else
			pi->val[2] = -1;
	}
}

/** レイヤの順番移動 (移動後に呼び出す) */

void Undo_addLayerMoveList(LayerItem *item,int parent,int pos)
{
	UndoItem *pi = _add_item(UNDO_TYPE_LAYER_MOVE_LIST);

	/* [0] 移動後の親位置
	 * [1] 移動後の相対位置
	 * [2] 移動前の親位置
	 * [3] 移動前の相対位置 */

	if(pi)
	{
		UndoItem_setval_layerno_forParent(pi, 0, item);
		pi->val[2] = parent;
		pi->val[3] = pos;
	}
}

/** 複数レイヤの順番移動 */

void Undo_addLayerMoveList_multi(int num,int16_t *dat)
{
	UndoItem *pi = _add_item(UNDO_TYPE_LAYER_MOVE_LIST_MULTI);

	if(pi)
	{
		if(!UndoItem_setdat_movelayerlist_multi(pi, num, dat))
			_failed_set();
	}
}

/** キャンバスサイズ変更
 *
 * 範囲外を切り取らず、レイヤ全体をオフセット移動した時。 */

void Undo_addResizeCanvas_moveOffset(int mx,int my,int w,int h)
{
	UndoItem *pi = _add_item(UNDO_TYPE_RESIZECANVAS_MOVEOFFSET);

	if(pi)
	{
		pi->val[0] = -mx;
		pi->val[1] = -my;
		pi->val[2] = w;
		pi->val[3] = h;
	}
}

/** キャンバスサイズ変更 (範囲外切り取り時) */

void Undo_addResizeCanvas_crop(int w,int h)
{
	UndoItem *pi = _add_item(UNDO_TYPE_RESIZECANVAS_CROP);

	if(pi)
	{
		pi->val[0] = w;
		pi->val[1] = h;

		if(!UndoItem_setdat_layerImage_all(pi))
			_failed_set();
	}
}

/** キャンバス拡大縮小 */

void Undo_addScaleCanvas()
{
	UndoItem *pi = _add_item(UNDO_TYPE_SCALECANVAS);

	if(pi)
	{
		pi->val[0] = APP_DRAW->imgw;
		pi->val[1] = APP_DRAW->imgh;
		pi->val[2] = APP_DRAW->imgdpi;

		if(!UndoItem_setdat_layerImage_all(pi))
			_failed_set();
	}
}

