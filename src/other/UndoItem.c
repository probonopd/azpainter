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
 * レイヤの読み書き、実行処理
 *****************************************/
/*
 * [!] 実行処理時に失敗しても更新は行われるので、更新情報は常にセットすること。
 *
 * [!] 現在のレイヤのリスト状態によっては、
 *     処理実行後にカレントレイヤが削除されたり非展開フォルダに隠れたりするので、
 *     カレントレイヤが正しくリスト上に表示されるよう注意すること。
 */

#include "mDef.h"
#include "mRectBox.h"
#include "mUndo.h"

#include "defDraw.h"

#include "LayerList.h"
#include "LayerItem.h"
#include "TileImage.h"

#include "Undo.h"
#include "UndoItem.h"

#include "draw_calc.h"
#include "draw_layer.h"



//==============================
// sub
//==============================


/** レイヤ削除処理 共通
 *
 * 更新範囲のセット + レイヤ削除 */

static mBool _run_layerDelete(LayerItem *item,UndoUpdateInfo *info)
{
	if(!item) return FALSE;

	//更新範囲
	/* 削除前に行う。
	 * 空レイヤの場合、更新範囲は空となるが、レイヤ一覧は常に更新しなければらなないので注意。 */

	info->type = UNDO_UPDATE_RECT_AND_LAYERLIST;

	LayerItem_getVisibleImageRect(item, &info->rc);

	//削除 (カレントレイヤの調整あり)

	drawLayer_deleteForUndo(APP_DRAW, item);

	return TRUE;
}


//==============================
//
//==============================


/** 複数レイヤの順番移動 */

mBool UndoItem_setdat_movelayerlist_multi(UndoItem *p,int num,int16_t *buf)
{
	mBool ret;

	p->val[0] = num;

	if(!UndoItem_alloc(p, num * 4 * 2)
		|| !UndoItem_openWrite(p))
		return FALSE;

	ret = UndoItem_write(p, buf, num * 4 * 2);
	
	UndoItem_closeWrite(p);

	return ret;
}


//==============================
// 単体レイヤ
//==============================


/** 単体レイヤ書き込み */

mBool UndoItem_setdat_layer(UndoItem *p,LayerItem *item)
{
	mBool ret;

	if(!UndoItem_beginWriteLayer(p)) return FALSE;

	ret = UndoItem_writeLayerInfoAndImage(p, item);

	UndoItem_endWriteLayer(p);

	return ret;
}

/** 単体レイヤ復元
 *
 * @param rc  更新範囲が追加される */

mBool UndoItem_restore_layer(UndoItem *p,int parent,int pos,mRect *rc)
{
	mBool ret;

	if(!UndoItem_beginReadLayer(p)) return FALSE;

	ret = UndoItem_readLayer(p, parent, pos, rc);

	UndoItem_endReadLayer(p);

	return ret;
}

/** 単体レイヤのイメージを書き込み */

mBool UndoItem_setdat_layerImage(UndoItem *p,LayerItem *item)
{
	mBool ret;

	if(!UndoItem_beginWriteLayer(p))
		return FALSE;

	ret = UndoItem_writeTileImage(p, item->img);

	UndoItem_endWriteLayer(p);

	return ret;
}

/** 単体レイヤのイメージを復元 */

mBool UndoItem_restore_layerImage(UndoItem *p,LayerItem *item)
{
	mBool ret;

	if(!UndoItem_beginReadLayer(p)) return FALSE;

	ret = UndoItem_readLayerImage(p, item);

	UndoItem_endReadLayer(p);

	return ret;
}


//==================================
// 下レイヤへ移す
//==================================


/** 下レイヤへ移す時の書き込み */

mBool UndoItem_setdat_layerImage_double(UndoItem *p,LayerItem *item)
{
	mBool ret;

	if(!item
		|| !UndoItem_beginWriteLayer(p))
		return FALSE;

	ret = (UndoItem_writeTileImage(p, item->img)
		&& UndoItem_writeTileImage(p, LAYERITEM(item->i.next)->img));

	UndoItem_endWriteLayer(p);

	return ret;
}

/** 下レイヤへ移す時の復元 */

mBool UndoItem_restore_layerImage_double(UndoItem *p,LayerItem *item)
{
	mBool ret;

	if(!UndoItem_beginReadLayer(p)) return FALSE;

	ret = (UndoItem_readLayerImage(p, item)
		&& UndoItem_readLayerImage(p, LayerItem_getNext(item)));

	UndoItem_endReadLayer(p);

	return ret;
}


//==================================
// 下レイヤへ結合
//==================================


/** 下レイヤへ結合時の書き込み */

mBool UndoItem_setdat_layerForCombine(UndoItem *p,int settype)
{
	LayerItem *item;
	mBool ret = FALSE;

	item = UndoItem_getLayerFromNo_forParent(p->val[1], p->val[2]);
	if(!item) return FALSE;

	if(!UndoItem_beginWriteLayer(p))
		return FALSE;

	//上のレイヤ or 結合後のレイヤ

	if(!UndoItem_writeLayerInfoAndImage(p, item))
		goto ERR;

	//下のレイヤ

	if(settype == MUNDO_TYPE_UNDO)
	{
		item = LayerItem_getNext(item);

		if(!UndoItem_writeLayerInfoAndImage(p, item))
			goto ERR;
	}

	ret = TRUE;

ERR:
	UndoItem_endWriteLayer(p);

	return ret;
}

/** 下レイヤへ結合時の復元 */

mBool UndoItem_restore_layerForCombine(UndoItem *p,int runtype,LayerItem *item,mRect *rcupdate)
{
	mRect rc;
	mBool ret = FALSE;

	mRectEmpty(&rc);

	if(!UndoItem_beginReadLayer(p)) return FALSE;

	if(runtype == MUNDO_TYPE_UNDO)
	{
		//[UNDO] 上のレイヤを復元 + 下レイヤを上書き復元

		if(UndoItem_readLayer(p, p->val[1], p->val[2], &rc))
			ret = UndoItem_readLayerOverwrite(p, item, &rc);
	}
	else
	{
		//[REDO] 結合後のレイヤを上書き復元 + 上のレイヤを削除
		//item = 上のレイヤ

		TileImage_getHaveImageRect_pixel(item->img, &rc, NULL);

		if(UndoItem_readLayerOverwrite(p, LayerItem_getNext(item), &rc))
		{
			drawLayer_deleteForUndo(APP_DRAW, item);

			ret = TRUE;
		}
	}

	UndoItem_endReadLayer(p);

	if(ret) *rcupdate = rc;

	return ret;
}


//==============================
// 複数レイヤ
//==============================


/** 単体レイヤ書き込み (フォルダの場合は下位全て) */

mBool UndoItem_setdat_layerForFolder(UndoItem *p,LayerItem *item)
{
	LayerItem *li;
	int16_t lno[2];
	int no1,no2;
	mBool ret = FALSE;

	if(!item) return FALSE;

	if(!UndoItem_beginWriteLayer(p))
		return FALSE;

	for(li = item; li; li = LayerItem_getNextRoot(li, item))
	{
		//レイヤ番号、レイヤ情報とイメージ (フォルダの場合はイメージなし)

		LayerList_getItemPos_forParent(APP_DRAW->layerlist, li, &no1, &no2);

		lno[0] = no1;
		lno[1] = no2;

		if(!UndoItem_write(p, lno, 4)
			|| !UndoItem_writeLayerInfoAndImage(p, li))
			goto ERR;
	}

	//終了

	lno[0] = lno[1] = -1;
	ret = UndoItem_write(p, lno, 4);

ERR:
	UndoItem_endWriteLayer(p);

	return ret;
}

/** すべてのレイヤを書き込み */

mBool UndoItem_setdat_layerAll(UndoItem *p)
{
	LayerItem *li;
	int16_t lno[2];
	int no1,no2;
	mBool ret = FALSE;

	if(!UndoItem_beginWriteLayer(p))
		return FALSE;

	for(li = LayerList_getItem_top(APP_DRAW->layerlist); li; li = LayerItem_getNext(li))
	{
		//レイヤ番号、レイヤ情報とイメージ (フォルダの場合はイメージなし)

		LayerList_getItemPos_forParent(APP_DRAW->layerlist, li, &no1, &no2);

		lno[0] = no1;
		lno[1] = no2;

		if(!UndoItem_write(p, lno, 4)
			|| !UndoItem_writeLayerInfoAndImage(p, li))
			goto ERR;
	}

	//終了

	lno[0] = lno[1] = -1;
	ret = UndoItem_write(p, lno, 4);

ERR:
	UndoItem_endWriteLayer(p);

	return ret;
}

/** 複数レイヤ復元
 *
 * レイヤ位置+レイヤデータ */

mBool UndoItem_restore_layerMulti(UndoItem *p,mRect *rcupdate)
{
	int16_t lno[2];
	mBool ret = FALSE;

	mRectEmpty(rcupdate);

	if(!UndoItem_beginReadLayer(p)) return FALSE;

	while(1)
	{
		//レイヤ番号
		
		if(!UndoItem_read(p, lno, 4)) break;

		if(lno[1] == -1)
		{
			ret = TRUE;
			break;
		}

		//レイヤ復元

		if(!UndoItem_readLayer(p, lno[0], lno[1], rcupdate))
			break;
	}

	UndoItem_endReadLayer(p);

	return ret;
}


//==============================
// 複数レイヤ イメージ
//==============================


/** すべてのレイヤのイメージを書き込み (フォルダは除く) */

mBool UndoItem_setdat_layerImage_all(UndoItem *p)
{
	LayerItem *li;
	int16_t lno;
	mBool ret = FALSE;

	if(!UndoItem_beginWriteLayer(p))
		return FALSE;

	for(li = LayerList_getItem_top(APP_DRAW->layerlist); li; li = LayerItem_getNext(li))
	{
		if(li->img)
		{
			//レイヤ番号、レイヤイメージ

			lno = LayerList_getItemPos(APP_DRAW->layerlist, li);

			if(!UndoItem_write(p, &lno, 2)
				|| !UndoItem_writeTileImage(p, li->img))
				goto ERR;
		}
	}

	//終了

	lno = -1;
	ret = UndoItem_write(p, &lno, 2);

ERR:
	UndoItem_endWriteLayer(p);

	return ret;
}

/** 全レイヤのイメージを復元 */

mBool UndoItem_restore_layerImage_all(UndoItem *p)
{
	int16_t lno;
	mBool ret = FALSE;

	if(!UndoItem_beginReadLayer(p)) return FALSE;

	while(1)
	{
		//レイヤ番号
		
		if(!UndoItem_read(p, &lno, 2)) break;

		if(lno == -1)
		{
			ret = TRUE;
			break;
		}

		//イメージ復元

		if(!UndoItem_readLayerImage(p, UndoItem_getLayerFromNo(lno)))
			break;
	}

	UndoItem_endReadLayer(p);

	return ret;
}


//====================================
// レイヤ関連 実行処理
//====================================


/** レイヤ新規追加 */

mBool UndoItem_runLayerNew(UndoItem *p,UndoUpdateInfo *info,int runtype)
{
	if(runtype == MUNDO_TYPE_UNDO)
	{
		//レイヤ削除

		return _run_layerDelete(
			UndoItem_getLayerFromNo_forParent(p->val[0], p->val[1]), info);
	}
	else
	{
		//レイヤ復元

		if(p->val[2])
		{
			//イメージあり
			
			info->type = UNDO_UPDATE_RECT_AND_LAYERLIST;

			return UndoItem_restore_layer(p, p->val[0], p->val[1], &info->rc);
		}
		else
		{
			//イメージ空

			info->type = UNDO_UPDATE_LAYERLIST;

			return UndoItem_restore_newLayerEmpty(p);
		}	
	}
}

/** レイヤ複製 */

mBool UndoItem_runLayerCopy(UndoItem *p,UndoUpdateInfo *info,int runtype)
{
	LayerItem *src,*item;

	src = UndoItem_getLayerFromNo(p->val[0]);
	if(!src) return FALSE;

	if(runtype == MUNDO_TYPE_UNDO)
	{
		//レイヤ削除

		return _run_layerDelete(src, info);
	}
	else
	{
		//レイヤ再複製

		item = LayerList_dupLayer(APP_DRAW->layerlist, src);
		if(!item) return FALSE;

		info->type = UNDO_UPDATE_RECT_AND_LAYERLIST;

		LayerItem_getVisibleImageRect(item, &info->rc);

		return TRUE;
	}
}

/** レイヤ削除 */

mBool UndoItem_runLayerDelete(UndoItem *p,UndoUpdateInfo *info,int runtype)
{
	if(runtype == MUNDO_TYPE_REDO)
	{
		//レイヤ削除

		return _run_layerDelete(UndoItem_getLayerFromNo(p->val[0]), info);
	}
	else
	{
		//レイヤ復元

		info->type = UNDO_UPDATE_RECT_AND_LAYERLIST;

		return UndoItem_restore_layerMulti(p, &info->rc);
	}

	return TRUE;
}

/** レイヤイメージクリア */

mBool UndoItem_runLayerClearImage(UndoItem *p,UndoUpdateInfo *info,int runtype)
{
	LayerItem *item;
	mBool ret;

	item = UndoItem_getLayerFromNo(p->val[0]);
	if(!item) return FALSE;

	info->type = UNDO_UPDATE_RECT_AND_LAYERLIST_ONE;
	info->layer = item;

	if(runtype == MUNDO_TYPE_UNDO)
	{
		//イメージ復元

		ret = UndoItem_restore_layerImage(p, item);

		if(ret)
			LayerItem_getVisibleImageRect(item, &info->rc);

		return ret;
	}
	else
	{
		//クリア

		LayerItem_getVisibleImageRect(item, &info->rc);

		TileImage_clear(item->img);
	}

	return TRUE;
}

/** レイヤ、カラータイプ変更 */

mBool UndoItem_runLayerColorType(UndoItem *p,UndoUpdateInfo *info,int runtype)
{
	LayerItem *item;

	item = UndoItem_getLayerFromNo(p->val[0]);
	if(!item) return FALSE;

	info->type = UNDO_UPDATE_RECT_AND_LAYERLIST_ONE;
	info->layer = item;

	if(runtype == MUNDO_TYPE_UNDO)
	{
		//元のイメージ復元

		item->coltype = p->val[1];

		if(!UndoItem_restore_layerImage(p, item))
			return FALSE;

		LayerItem_getVisibleImageRect(item, &info->rc);
	}
	else
	{
		//再変換

		LayerItem_getVisibleImageRect(item, &info->rc);

		TileImage_convertType(item->img, p->val[2], p->val[3]);

		item->coltype = p->val[2];
	}

	return TRUE;
}

/** 下レイヤに移す/結合 */

mBool UndoItem_runLayerDropAndCombine(UndoItem *p,UndoUpdateInfo *info,int runtype)
{
	LayerItem *item;

	item = UndoItem_getLayerFromNo_forParent(p->val[1], p->val[2]);
	if(!item) return FALSE;

	/* どちらかが非表示状態の場合でも正しい範囲を更新できるようにするため、
	 * LayerItem_getVisibleImageRect() は使わない。 */

	if(p->val[0])
	{
		//------ 下レイヤに移す (item = 上のレイヤ)

		/* UNDO : 復元した上のレイヤのイメージ範囲
		 * REDO : 結合しようとしている上のレイヤのイメージ範囲
		 *
		 * イメージが変化する範囲 = 上のレイヤのイメージの範囲。 */

		info->type = UNDO_UPDATE_RECT_AND_LAYERLIST_DOUBLE;
		info->layer = item;

		if(runtype == MUNDO_TYPE_REDO)
			TileImage_getHaveImageRect_pixel(item->img, &info->rc, NULL);

		if(!UndoItem_restore_layerImage_double(p, item))
			return FALSE;

		if(runtype == MUNDO_TYPE_UNDO)
			TileImage_getHaveImageRect_pixel(item->img, &info->rc, NULL);

		return TRUE;
	}
	else
	{
		//------- 下レイヤに結合

		info->type = UNDO_UPDATE_RECT_AND_LAYERLIST;

		return UndoItem_restore_layerForCombine(p, runtype, item, &info->rc);
	}
}

/** すべてのレイヤの結合 */

mBool UndoItem_runLayerCombineAll(UndoItem *p,UndoUpdateInfo *info,int runtype)
{
	LayerItem *item;
	mBool ret = FALSE;

	if(runtype == MUNDO_TYPE_UNDO)
	{
		//UNDO : 全レイヤ復元 + 結合後レイヤを削除

		item = LayerList_getItem_top(APP_DRAW->layerlist);

		if(UndoItem_restore_layerMulti(p, &info->rc))
		{
			drawLayer_deleteForUndo(APP_DRAW, item);
			
			ret = TRUE;
		}
	}
	else
	{
		//REDO : レイヤすべて削除 + 結合後レイヤを復元

		LayerList_clear(APP_DRAW->layerlist);

		ret = UndoItem_restore_layer(p, -1, 0, &info->rc);

		APP_DRAW->curlayer = LayerList_getItem_top(APP_DRAW->layerlist);
	}

	info->type = UNDO_UPDATE_ALL_AND_LAYERLIST;

	return ret;
}

/** フォルダ内のすべてのレイヤ結合 */

mBool UndoItem_runLayerCombineFolder(UndoItem *p,UndoUpdateInfo *info,int runtype)
{
	LayerItem *item;
	mRect rc;
	mBool ret;

	item = UndoItem_getLayerFromNo_forParent(p->val[0], p->val[1]);
	if(!item) return FALSE;

	/* [UNDO] フォルダの全レイヤ復元 + 結合後レイヤ削除
	 * [REDO] 結合後レイヤ復元 + フォルダ削除 */

	//復元

	if(runtype == MUNDO_TYPE_UNDO)
		ret = UndoItem_restore_layerMulti(p, &info->rc);
	else
		ret = UndoItem_restore_layer(p, p->val[0], p->val[1], &info->rc);

	//結合後レイヤ、またはフォルダを削除

	if(ret)
	{
		//範囲追加
	
		if(LayerItem_getVisibleImageRect(item, &rc))
			mRectUnion(&info->rc, &rc);

		//削除

		drawLayer_deleteForUndo(APP_DRAW, item);
	}

	//

	info->type = UNDO_UPDATE_RECT_AND_LAYERLIST;

	return ret;
}

/** レイヤ全体の左右/上下反転、回転
 *
 * [!] レイヤのロックが実行時と同じ状態でなければならない。 */

mBool UndoItem_runLayerFullEdit(UndoItem *p,UndoUpdateInfo *info)
{
	LayerItem *li;

	li = UndoItem_getLayerFromNo(p->val[0]);
	if(!li) return FALSE;

	if(LayerItem_editFullImage(li, p->val[1], &info->rc))
		info->type = UNDO_UPDATE_RECT;

	return TRUE;
}

/** レイヤオフセット位置移動 */
/*
 * <val>
 * [0] x 相対移動値
 * [1] y 相対移動値
 * [2] レイヤ番号 (-1 ですべて)
 *
 * 指定レイヤがフォルダの場合は、フォルダ以下すべて移動。
 */

mBool UndoItem_runLayerMoveOffset(UndoItem *p,UndoUpdateInfo *info)
{
	LayerItem *li;
	mRect rc,rc1,rc2;
	int mx,my;

	//リンクをセット

	li = (p->val[2] < 0)? NULL: UndoItem_getLayerFromNo(p->val[2]);

	li = LayerList_setLink_movetool(APP_DRAW->layerlist, li);

	//各レイヤ移動

	mx = p->val[0];
	my = p->val[1];

	mRectEmpty(&rc);

	for(; li; li = li->link)
	{
		TileImage_getHaveImageRect_pixel(li->img, &rc1, NULL);

		TileImage_moveOffset_rel(li->img, mx, my);

		drawCalc_unionRect_relmove(&rc2, &rc1, mx, my);
		mRectUnion(&rc, &rc2);
	}

	//更新

	info->type = UNDO_UPDATE_RECT;
	info->rc = rc;

	return TRUE;
}

/** レイヤ順番移動 */

mBool UndoItem_runLayerMoveList(UndoItem *p,UndoUpdateInfo *info)
{
	if(!LayerList_moveitem_forUndo(APP_DRAW->layerlist, p->val, &info->rc))
		return FALSE;

	drawLayer_afterMoveList_forUndo(APP_DRAW);

	info->type = UNDO_UPDATE_RECT_AND_LAYERLIST;

	return TRUE;
}

/** 複数レイヤ順番移動 */

mBool UndoItem_runLayerMoveList_multi(UndoItem *p,UndoUpdateInfo *info,int runtype)
{
	int16_t *buf;
	mBool ret;

	//読み込み

	buf = (int16_t *)UndoItem_readData(p, p->val[0] * 4 * 2);
	if(!buf) return FALSE;

	//移動

	info->type = UNDO_UPDATE_RECT_AND_LAYERLIST;

	ret = LayerList_moveitem_multi_forUndo(APP_DRAW->layerlist, p->val[0], buf,
		&info->rc, (runtype == MUNDO_TYPE_REDO));

	mFree(buf);

	//

	drawLayer_afterMoveList_forUndo(APP_DRAW);

	return ret;
}


//=============================
//
//=============================


/** キャンバスイメージ変更(切り取り)/キャンバス拡大縮小 */

mBool UndoItem_runCanvasResizeAndScale(UndoItem *p,UndoUpdateInfo *info)
{
	mBool ret;

	//イメージ復元

	ret = UndoItem_restore_layerImage_all(p);

	//dpi

	if(p->type == UNDO_TYPE_SCALECANVAS)
		APP_DRAW->imgdpi = p->val[2];

	//更新

	info->type = UNDO_UPDATE_CANVAS_RESIZE;
	info->rc.x1 = p->val[0];
	info->rc.y1 = p->val[1];

	return ret;
}
