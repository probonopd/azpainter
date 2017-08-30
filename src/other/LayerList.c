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
 * レイヤリスト
 *****************************************/
/*
 * - レイヤ数が増減した場合、LayerList::num (レイヤ総数) の値を変更すること。
 */


#include <stdio.h>

#include "mDef.h"
#include "mTree.h"
#include "mRectBox.h"
#include "mUtilCharCode.h"

#include "LayerList.h"
#include "LayerItem.h"
#include "MaterialImgList.h"
#include "TileImage.h"
#include "defTileImage.h"
#include "file_apd_v3.h"

#include "defDraw.h"


//---------------------

struct _LayerList
{
	mTree tree;
	int num;
};

//---------------------

#define _TOPITEM(p)        ((LayerItem *)p->tree.top)
#define _NEXT_ITEM(p)      (LayerItem *)((p)->i.next)
#define _NEXT_TREEITEM(p)  (LayerItem *)mTreeItemGetNext((mTreeItem *)(p))
#define _PREV_TREEITEM(p)  (LayerItem *)mTreeItemGetPrev((mTreeItem *)(p))

//---------------------



//=========================
// sub
//=========================


/** 破棄ハンドラ */

static void _destroy_item(mTreeItem *pi)
{
	LayerItem *p = (LayerItem *)pi;

	//マスクレイヤの場合

	if(APP_DRAW->masklayer)
		APP_DRAW->masklayer = NULL;

	//レイヤテクスチャ解放

	MaterialImgList_releaseImage(MATERIALIMGLIST_TYPE_TEXTURE, p->img8texture);

	//

	TileImage_free(p->img);

	mFree(p->name);
	mFree(p->texture_path);
}

/** アイテム確保 */

static LayerItem *_item_new(LayerList *list)
{
	LayerItem *p;

	if(list->num >= 500) return NULL;

	p = (LayerItem *)mMalloc(sizeof(LayerItem), TRUE);
	if(!p) return NULL;

	p->i.destroy = _destroy_item;

	p->opacity = 128;
	p->flags = LAYERITEM_F_VISIBLE;

	return p;
}


//=========================
// main
//=========================


/** レイヤリスト作成 */

LayerList *LayerList_new()
{
	return (LayerList *)mMalloc(sizeof(LayerList), TRUE);
}

/** レイヤリスト解放 */

void LayerList_free(LayerList *p)
{
	if(p)
	{
		mTreeDeleteAll(M_TREE(p));
		mFree(p);
	}
}

/** クリア */

void LayerList_clear(LayerList *p)
{
	mTreeDeleteAll(M_TREE(p));
	p->num = 0;
}

/** レイヤ追加
 *
 * イメージは作成しない。
 * 親フォルダが非展開の場合、展開させる。
 *
 * @param insert NULL でルートの最後。フォルダならフォルダ内の一番上、通常レイヤならその上。 */

LayerItem *LayerList_addLayer(LayerList *p,LayerItem *insert)
{
	LayerItem *pi;

	//アイテム作成

	pi = _item_new(p);
	if(!pi) return NULL;

	//リンク

	if(!insert)
		//insert = NULL : ルートの最後に追加
		mTreeLinkBottom(M_TREE(p), NULL, M_TREEITEM(pi));
	else if(LAYERITEM_IS_FOLDER(insert))
	{
		//insert = フォルダ : フォルダの先頭へ

		mTreeLinkInsert_parent(M_TREE(p), M_TREEITEM(insert), insert->i.first, M_TREEITEM(pi));

		insert->flags |= LAYERITEM_F_FOLDER_EXPAND;  //親を展開
	}
	else
		//insert の上に挿入
		mTreeLinkInsert(M_TREE(p), M_TREEITEM(insert), M_TREEITEM(pi));

	//個数

	p->num++;

	return pi;
}

/** レイヤ追加 (イメージを作成 [1x1]) */

LayerItem *LayerList_addLayer_image(LayerList *p,LayerItem *insert,int coltype)
{
	LayerItem *pi;
	TileImage *img;

	img = TileImage_new(coltype, 1, 1);
	if(!img) return NULL;

	pi = LayerList_addLayer(p, insert);
	if(!pi)
	{
		TileImage_free(img);
		return NULL;
	}

	LayerItem_replaceImage(pi, img);

	return pi;
}

/** 挿入位置を番号で指定してレイヤ追加 (ファイル読み込み/UNDO時)
 *
 * @param parent -1 でルート
 * @param pos    -1 で終端 */

LayerItem *LayerList_addLayer_pos(LayerList *p,int parent,int pos)
{
	LayerItem *pi,*ins[2];

	//作成

	pi = _item_new(p);
	if(!pi) return NULL;

	//リンク

	LayerList_getItems_fromPos(p, ins, parent, pos);

	if(ins[1])
		mTreeLinkInsert(M_TREE(p), M_TREEITEM(ins[1]), M_TREEITEM(pi));
	else
		mTreeLinkBottom(M_TREE(p), M_TREEITEM(ins[0]), M_TREEITEM(pi));

	p->num++;

	return pi;
}

/** レイヤ追加 (新規イメージ時のデフォルトレイヤ) */

LayerItem *LayerList_addLayer_newimage(LayerList *p,int coltype)
{
	LayerItem *pi;
	TileImage *img;

	//イメージ

	img = TileImage_new(coltype, 1, 1);
	if(!img) return NULL;

	//レイヤ

	pi = LayerList_addLayer(p, NULL);
	if(!pi)
	{
		TileImage_free(img);
		return NULL;
	}

	LayerItem_replaceImage(pi, img);

	//名前

	LayerItem_setName_layerno(pi, 0);

	return pi;
}

/** 複数レイヤをすべて親をルートにして追加 */

mBool LayerList_addLayers_onRoot(LayerList *p,int num)
{
	for(; num > 0; num--)
	{
		if(!LayerList_addLayer(p, NULL))
			return FALSE;
	}

	return TRUE;
}


//=========================
// レイヤ操作
//=========================


/** レイヤを複製 (通常レイヤのみ) */

LayerItem *LayerList_dupLayer(LayerList *p,LayerItem *src)
{
	LayerItem *pi;
	TileImage *img;

	//イメージ複製

	img = TileImage_newClone(src->img);
	if(!img) return NULL;

	//レイヤ追加

	pi = LayerList_addLayer(p, src);
	if(!pi)
	{
		TileImage_free(img);
		return NULL;
	}

	//情報

	LayerItem_replaceImage(pi, img);
	LayerItem_copyInfo(pi, src);

	pi->img8texture = MaterialImgList_getImage(MATERIALIMGLIST_TYPE_LAYER_TEXTURE, pi->texture_path, TRUE);

	return pi;
}

/** レイヤ削除
 *
 * @return 削除後の選択レイヤ (item がカレントとみなす) */

LayerItem *LayerList_deleteLayer(LayerList *p,LayerItem *item)
{
	LayerItem *next;

	next = LayerItem_getNextPass(item);
	if(!next)
		next = LayerItem_getPrevExpand(item);

	//削除

	mTreeDeleteItem(M_TREE(p), M_TREEITEM(item));

	//アイテム数再計算

	p->num = mTreeItemGetNum(M_TREE(p));

	return next;
}


//================================
// 取得
//================================


/** レイヤの総数を取得 */

int LayerList_getNum(LayerList *p)
{
	return p->num;
}

/** 通常レイヤの数を取得 (フォルダは除く) */

int LayerList_getNormalLayerNum(LayerList *p)
{
	LayerItem *pi;
	int num = 0;

	for(pi = _TOPITEM(p); pi; pi = _NEXT_TREEITEM(pi))
	{
		if(pi->img) num++;
	}

	return num;
}

/** 指定アイテムの先頭からの位置取得
 *
 * @return item が NULL または見つからなかった場合、-1 */

int LayerList_getItemPos(LayerList *p,LayerItem *item)
{
	LayerItem *pi;
	int pos = 0;

	if(!item) return -1;

	for(pi = _TOPITEM(p); pi; pi = _NEXT_TREEITEM(pi), pos++)
	{
		if(pi == item) return pos;
	}

	return -1;
}

/** 指定アイテムの親の位置と、フォルダ内の相対番号取得 */

void LayerList_getItemPos_forParent(LayerList *p,LayerItem *item,int *parent,int *relno)
{
	mTreeItem *pi;
	int no;

	for(pi = item->i.prev, no = 0; pi; pi = pi->prev, no++);

	*parent = LayerList_getItemPos(p, LAYERITEM(item->i.parent));
	*relno = no;
}

/** 一番上のアイテム取得 */

LayerItem *LayerList_getItem_top(LayerList *p)
{
	return _TOPITEM(p);
}

/** 先頭からの位置からアイテム取得
 *
 * フォルダの子アイテムもカウントに含む。
 *
 * @param pos 負の場合は NULL が返る */

LayerItem *LayerList_getItem_byPos_topdown(LayerList *p,int pos)
{
	mTreeItem *pi;
	int cnt;

	if(pos < 0) return NULL;

	for(pi = p->tree.top, cnt = 0; pi && pos != cnt; pi = mTreeItemGetNext(pi), cnt++);

	return (LayerItem *)pi;
}

/** 一番下の表示イメージアイテム取得 (合成時用) */

LayerItem *LayerList_getItem_bottomVisibleImage(LayerList *p)
{
	if(p->tree.bottom)
		return LayerItem_getPrevVisibleImage_incSelf(LAYERITEM(p->tree.bottom));
	else
		return NULL;
}

/** 一番下の通常レイヤアイテム取得 (PSD 保存用) */

LayerItem *LayerList_getItem_bottomNormal(LayerList *p)
{
	LayerItem *pi;

	pi = (LayerItem *)mTreeGetLastItem(&p->tree);

	for(; pi && !pi->img; pi = _PREV_TREEITEM(pi));

	return pi;
}

/** 位置番号から各アイテム取得
 *
 * @param dst    [0] 親 [1] pos の位置のアイテム
 * @param parent 親の位置 (負の値でルート)
 * @param pos    フォルダ内の相対位置 (負の値で NULL) */

void LayerList_getItems_fromPos(LayerList *p,LayerItem **dst,int parent,int pos)
{
	LayerItem *piparent,*pi;
	int i;

	//親

	dst[0] = piparent = LayerList_getItem_byPos_topdown(p, parent);

	//位置

	if(pos < 0)
		pi = NULL;
	else
	{
		if(piparent)
			pi = (LayerItem *)piparent->i.first;
		else
			pi = _TOPITEM(p);

		for(i = 0; pi && i != pos; pi = _NEXT_ITEM(pi), i++);
	}

	dst[1] = pi;
}

/** 表示イメージ上で、指定位置のピクセルに一番最初に点があるレイヤを取得 */

LayerItem *LayerList_getItem_topPixelLayer(LayerList *p,int x,int y)
{
	LayerItem *pi,*last = NULL;
	RGBAFix15 pix;

	//後ろから順に調べて最後のアイテム

	pi = LayerList_getItem_bottomVisibleImage(p);

	for(; pi; pi = LayerItem_getPrevVisibleImage(pi))
	{
		TileImage_getPixel(pi->img, x, y, &pix);
		if(pix.a) last = pi;
	}

	return last;
}

/** レイヤ一覧のスクロール情報取得
 *
 * @param ret_maxdepth  フォルダの最大深度が入る
 * @return 一覧上に表示されている全アイテムの数 */

int LayerList_getScrollInfo(LayerList *p,int *ret_maxdepth)
{
	LayerItem *pi;
	int num = 0,maxdepth = 0,depth;

	for(pi = _TOPITEM(p); pi; pi = LayerItem_getNextExpand(pi), num++)
	{
		depth = LayerItem_getTreeDepth(pi);
		if(depth > maxdepth) maxdepth = depth;
	}

	*ret_maxdepth = maxdepth;

	return num;
}


//===========================
// フラグ
//===========================


/** チェックフラグが ON のレイヤがあるか */

mBool LayerList_haveCheckedLayer(LayerList *p)
{
	LayerItem *pi;

	for(pi = _TOPITEM(p); pi; pi = _NEXT_TREEITEM(pi))
	{
		if(LAYERITEM_IS_CHECKED(pi) && pi->img)
			return TRUE;
	}

	return FALSE;
}

/** 全レイヤの表示フラグ変更 */

void LayerList_setVisible_all(LayerList *p,mBool on)
{
	LayerItem *pi;

	for(pi = _TOPITEM(p); pi; pi = _NEXT_TREEITEM(pi))
	{
		if(on)
			pi->flags |= LAYERITEM_F_VISIBLE;
		else
			pi->flags &= ~LAYERITEM_F_VISIBLE;
	}
}

/** チェックされたレイヤの表示を反転
 *
 * @param rcimg 反転したレイヤすべてのイメージ範囲 */

void LayerList_showRevChecked(LayerList *p,mRect *rcimg)
{
	LayerItem *pi;
	mRect rc,rc2;

	mRectEmpty(&rc);

	for(pi = _TOPITEM(p); pi; pi = _NEXT_TREEITEM(pi))
	{
		if(pi->img && LAYERITEM_IS_CHECKED(pi))
		{
			if(TileImage_getHaveImageRect_pixel(pi->img, &rc2, NULL))
				mRectUnion(&rc, &rc2);

			pi->flags ^= LAYERITEM_F_VISIBLE;
		}
	}

	*rcimg = rc;
}

/** フォルダを除くレイヤの表示反転 */

void LayerList_showRevImage(LayerList *p,mRect *rcimg)
{
	LayerItem *pi;
	mRect rc,rc2;

	mRectEmpty(&rc);

	for(pi = _TOPITEM(p); pi; pi = _NEXT_TREEITEM(pi))
	{
		if(pi->img)
		{
			if(TileImage_getHaveImageRect_pixel(pi->img, &rc2, NULL))
				mRectUnion(&rc, &rc2);

			pi->flags ^= LAYERITEM_F_VISIBLE;
		}
	}

	*rcimg = rc;
}

/** すべてのレイヤの指定フラグを OFF */

void LayerList_allFlagsOff(LayerList *p,uint32_t flags)
{
	LayerItem *pi;

	flags = ~flags;

	for(pi = _TOPITEM(p); pi; pi = _NEXT_TREEITEM(pi))
		pi->flags &= flags;
}

/** すべてのフォルダを閉じる
 *
 * カレントの親は閉じない。 */

void LayerList_closeAllFolders(LayerList *p,LayerItem *curitem)
{
	LayerItem *pi;

	//すべて閉じる

	for(pi = _TOPITEM(p); pi; pi = _NEXT_TREEITEM(pi))
	{
		if(LAYERITEM_IS_FOLDER(pi))
			pi->flags &= ~LAYERITEM_F_FOLDER_EXPAND;
	}

	//カレントの親は開く

	LayerItem_setExpandParent(curitem);
}


//=========================
// 描画処理など用
//=========================


/** レイヤマスクのイメージ取得 */

TileImage *LayerList_getMaskImage(LayerList *p,LayerItem *current)
{
	LayerItem *pi;

	/* 「下レイヤをマスクに」が OFF のレイヤが来るまで下を検索。
	 * (同じフォルダのレイヤのみ。フォルダが来た場合終了) */

	for(pi = _NEXT_ITEM(current);
		pi && pi->img && LAYERITEM_IS_MASK_UNDER(pi); pi = _NEXT_ITEM(pi));

	return (pi)? pi->img: NULL;
}

/** 複数レイヤ結合時のリンクをセット
 *
 * 結合順で、下位のレイヤから順番に
 *
 * @return 最初のレイヤ (一番下) */

LayerItem *LayerList_setLink_combineMulti(LayerList *p,int target,LayerItem *current)
{
	LayerItem *pi,*last,*top = NULL;

	switch(target)
	{
		//すべての表示レイヤ
		case 0:
			pi = LayerList_getItem_bottomVisibleImage(p);

			for(; pi; pi = LayerItem_getPrevVisibleImage(pi))
				LayerItem_setLink(pi, &top, &last);
			break;
		//フォルダ内の表示レイヤ
		case 1:
			pi = (LayerItem *)mTreeItemGetBottom(M_TREEITEM(current));

			for(; pi; pi = (LayerItem *)mTreeItemGetPrev_root(M_TREEITEM(pi), M_TREEITEM(current)))
			{
				if(pi->img && LayerItem_isVisible_real(pi))
					LayerItem_setLink(pi, &top, &last);
			}
			break;
		//チェックレイヤ (非表示も含む)
		default:
			pi = (LayerItem *)mTreeGetLastItem(&p->tree);

			for(; pi; pi = (LayerItem *)mTreeItemGetPrev(M_TREEITEM(pi)))
			{
				if(LAYERITEM_IS_CHECKED(pi) && pi->img)
					LayerItem_setLink(pi, &top, &last);
			}
			break;
	}

	return top;
}

/** 移動ツール時の移動対象リンクをセット
 *
 * [!] ロックされているレイヤ、フォルダ自身は除く
 *
 * @param item  NULL ですべて */

LayerItem *LayerList_setLink_movetool(LayerList *p,LayerItem *item)
{
	LayerItem *pi,*top = NULL,*last;
	mBool flag;

	for(pi = _TOPITEM(p); pi; )
	{
		//フォルダの場合、子をすべてリンクするか
	
		if(item)
			flag = (pi == item);
		else
			flag = TRUE;

		//リンクセット

		if(flag)
			pi = LayerItem_setLink_toNext(pi, &top, &last, TRUE);
		else
			pi = _NEXT_TREEITEM(pi);
	}

	return top;
}

/** 塗りつぶし判定元のリンクをセット & 先頭のレイヤを取得
 *
 * @param current  カレントレイヤ
 * @param disable_ref  判定元指定を無効
 * @return 判定元先頭のレイヤ (NULL でなし) */

LayerItem *LayerList_setLink_filltool(LayerList *p,LayerItem *current,mBool disable_ref)
{
	LayerItem *pi,*top = NULL,*last;

	//判定元有効時

	if(!disable_ref)
	{
		for(pi = _TOPITEM(p); pi; pi = _NEXT_TREEITEM(pi))
		{
			if(pi->img && LAYERITEM_IS_FILLREF(pi))
				LayerItem_setLink(pi, &top, &last);
		}
	}

	//一つも判定元指定がなければ、カレントレイヤ
	/* 自動選択時はカレントがフォルダの場合があるので、その場合は NULL */

	if(!top)
	{
		if(!current->img) return NULL;
	
		top = current;
		top->link = NULL;
	}

	//TileImage のリンクもセット

	for(pi = top; pi; pi = pi->link)
		pi->img->link = (pi->link)? pi->link->img: NULL;

	return top;
}

/** チェックレイヤのリンクをセット (フィルタ処理用) */

LayerItem *LayerList_setLink_checked(LayerList *p)
{
	LayerItem *pi,*top = NULL,*last;

	for(pi = LayerList_getItem_top(p); pi; pi = _NEXT_TREEITEM(pi))
	{
		if(pi->img && LAYERITEM_IS_CHECKED(pi))
			LayerItem_setLink(pi, &top, &last);
	}

	return top;
}


//=========================
// アイテム移動
//=========================


/** アイテムを上下に移動 */

mBool LayerList_moveitem_updown(LayerList *p,LayerItem *item,mBool bUp)
{
	mTreeItem *next;

	if(bUp)
	{
		//上へ
		
		if(!item->i.prev) return FALSE;
		
		mTreeMoveItem(&p->tree, M_TREEITEM(item), item->i.prev);
	}
	else
	{
		//下へ
	
		next = item->i.next;
		if(!next) return FALSE;
		
		next = next->next;

		if(next)
			mTreeMoveItem(&p->tree, M_TREEITEM(item), next);
		else
			mTreeMoveItem_bottom(&p->tree, M_TREEITEM(item), item->i.parent);
	}

	return TRUE;
}

/** アイテムを移動 */

void LayerList_moveitem(LayerList *p,LayerItem *src,LayerItem *dst,mBool infolder)
{
	if(!dst)
		//一番下へ
		mTreeMoveItem_bottom(&p->tree, M_TREEITEM(src), NULL);
	else if(!infolder)
		//dst の上へ移動
		mTreeMoveItem(&p->tree, M_TREEITEM(src), M_TREEITEM(dst));
	else
	{
		//フォルダの子へ

		if(dst->i.first)
			//フォルダの先頭
			mTreeMoveItem(&p->tree, M_TREEITEM(src), dst->i.first);
		else
			//フォルダに子がない
			mTreeMoveItem_bottom(&p->tree, M_TREEITEM(src), M_TREEITEM(dst));

		//展開させる

		dst->flags |= LAYERITEM_F_FOLDER_EXPAND;
	}
}

/** レイヤを一覧上で上から順に読み込む時の、追加後の位置移動 */

void LayerList_moveitem_forLoad_topdown(LayerList *p,LayerItem *src,LayerItem *parent)
{
	//親の最後に移動
	mTreeMoveItem_bottom(&p->tree, M_TREEITEM(src), M_TREEITEM(parent));
}

/** アンドゥによる位置移動 */

mBool LayerList_moveitem_forUndo(LayerList *p,int *val,mRect *rcupdate)
{
	LayerItem *src[2],*dst[2];

	//移動元レイヤ

	LayerList_getItems_fromPos(p, src, val[0], val[1]);
	if(!src[1]) return FALSE;

	//移動先レイヤ
	//(移動元のツリーリンクを外した状態で取得)

	mTreeLinkRemove(M_TREE(p), M_TREEITEM(src[1]));

	LayerList_getItems_fromPos(p, dst, val[2], val[3]);

	//移動 (リンクをつなげる)

	if(dst[1])
		mTreeLinkInsert(M_TREE(p), M_TREEITEM(dst[1]), M_TREEITEM(src[1]));
	else
		mTreeLinkBottom(M_TREE(p), M_TREEITEM(dst[0]), M_TREEITEM(src[1]));

	//更新範囲

	LayerItem_getVisibleImageRect(src[1], rcupdate);

	return TRUE;
}

/** チェックレイヤを指定フォルダに移動
 *
 * 移動後、チェックは OFF。
 *
 * @param pnum     移動レイヤ数が入る
 * @param rcupdate 更新範囲が入る
 * @return アンドゥ用データ (NULL で移動レイヤなし) */

int16_t *LayerList_moveitem_checkedToFolder(LayerList *p,LayerItem *dst,int *pnum,mRect *rcupdate)
{
	LayerItem *pi,*top = NULL,*last;
	mRect rc;
	int16_t *buf,*pbuf;
	int num = 0,no1,no2;

	//リンクセット＆レイヤ数取得 (最後のレイヤから逆順に)

	for(pi = (LayerItem *)mTreeGetLastItem(&p->tree); pi; pi = _PREV_TREEITEM(pi))
	{
		if(pi->img && LAYERITEM_IS_CHECKED(pi))
		{
			pi->flags ^= LAYERITEM_F_CHECKED;

			//すでに指定フォルダの子ならそのまま

			if((LayerItem *)pi->i.parent != dst)
			{
				LayerItem_setLink(pi, &top, &last);
				num++;
			}
		}
	}

	if(num == 0) return NULL;

	//バッファ確保

	buf = (int16_t *)mMalloc(num * 4 * 2, FALSE);
	if(!buf) return NULL;

	pbuf = buf;

	//移動
	/* フォルダの子の先頭位置へ、上から順に並べていくので、
	 * 一番下のレイヤから順にフォルダの先頭へ挿入していく。 */

	mRectEmpty(rcupdate);

	for(pi = top; pi; pi = pi->link)
	{
		//移動前の位置
		
		LayerList_getItemPos_forParent(p, pi, &no1, &no2);

		pbuf[0] = no1;
		pbuf[1] = no2;
		pbuf += 2;

		//移動

		if(dst->i.first)
			mTreeMoveItem(&p->tree, M_TREEITEM(pi), dst->i.first);
		else
			mTreeMoveItem_bottom(&p->tree, M_TREEITEM(pi), M_TREEITEM(dst));

		//移動後の位置

		LayerList_getItemPos_forParent(p, pi, &no1, &no2);

		pbuf[0] = no1;
		pbuf[1] = no2;
		pbuf += 2;

		//

		LayerItem_getVisibleImageRect(pi, &rc);
		mRectUnion(rcupdate, &rc);
	}

	//カレントがフォルダ下に移動する場合があるので、展開

	dst->flags |= LAYERITEM_F_FOLDER_EXPAND;

	*pnum = num;

	return buf;
}

/** 複数レイヤの順番移動 (アンドゥ時) */

mBool LayerList_moveitem_multi_forUndo(LayerList *p,int num,int16_t *buf,mRect *rcupdate,mBool redo)
{
	mRect rc;
	int no[4];

	//アンドゥ時は最後から逆順に

	if(!redo) buf += (num - 1) * 4;

	for(; num > 0; num--)
	{
		if(redo)
			no[0] = buf[0], no[1] = buf[1], no[2] = buf[2], no[3] = buf[3];
		else
			no[0] = buf[2], no[1] = buf[3], no[2] = buf[0], no[3] = buf[1];
	
		if(!LayerList_moveitem_forUndo(p, no, &rc))
			return FALSE;

		mRectUnion(rcupdate, &rc);

		if(redo)
			buf += 4;
		else
			buf -= 4;
	}

	return TRUE;
}


//===============================
// ほか
//===============================


/** レイヤ名をレイヤ数からセット
 *
 * レイヤ追加後の現在数からセットする。 */

void LayerList_setItemName_curlayernum(LayerList *p,LayerItem *item)
{
	LayerItem_setName_layerno(item, p->num - 1);
}

/** name を ASCII 文字列として名前セット
 *
 * name が ASCII 文字のみの場合はそのまま名前をセット。
 * そうでなければレイヤ数からセット。
 *
 * レイヤ名の文字コードが UTF-8 でないファイルからの読み込み用。 */

void LayerList_setItemName_ascii(LayerList *p,LayerItem *item,const char *name)
{
	const char *pc;

	//ASCII 文字のみか

	for(pc = name; *pc && *pc >= 0x20 && *pc <= 0x7e; pc++);

	//セット

	if(*pc == 0)
		mStrdup_ptr(&item->name, name);
	else
		LayerList_setItemName_curlayernum(p, item);
}

/** name を UTF-8 と想定してセット
 *
 * UTF-8 文字のみの場合はそのままセット。
 * そうでなければレイヤ数からセット。 */

void LayerList_setItemName_utf8(LayerList *p,LayerItem *item,const char *name)
{
	if(mUTF8ToUCS4(name, -1, NULL, 0) <= 0)
		//UCS4 変換して文字数が 0 またはエラー
		LayerList_setItemName_curlayernum(p, item);
	else
		mStrdup_ptr(&item->name, name);
}

/** すべてのレイヤイメージのオフセット位置を相対移動
 *
 * (キャンバスサイズ変更時) */

void LayerList_moveOffset_rel_all(LayerList *p,int movx,int movy)
{
	LayerItem *pi;

	for(pi = _TOPITEM(p); pi; pi = _NEXT_TREEITEM(pi))
	{
		if(pi->img)
			TileImage_moveOffset_rel(pi->img, movx, movy);
	}
}


//===============================
// apd v3 読み込み
//===============================


/** APD v3 ファイルからレイヤ読み込み */

LayerItem *LayerList_addLayer_apdv3(LayerList *p,LayerItem *insert,
	const char *filename,mPopupProgress *prog)
{
	loadAPDv3 *load;
	LayerItem *item = NULL;
	int lnum,lcur;
	mBool error = TRUE;

	load = loadAPDv3_open(filename, NULL, prog);
	if(!load) return NULL;

	//レイヤヘッダ

	if(!loadAPDv3_readLayerHeader(load, &lnum, &lcur))
		goto END;

	//レイヤ情報

	if(!loadAPDv3_beginLayerInfo(load))
		goto END;

	item = LayerList_addLayer(p, insert);
	if(!item) goto END;

	if(!loadAPDv3_readLayerInfo(load, item))
		goto END;

	loadAPDv3_endThunk(load);

	//レイヤタイル

	if(!loadAPDv3_beginLayerTile(load))
		goto END;

	if(!loadAPDv3_readLayerTile(load, item))
		goto END;

	error = FALSE;

	//終了

END:
	loadAPDv3_close(load);

	if(error)
	{
		LayerList_deleteLayer(p, item);
		item = NULL;
	}

	return item;
}
