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
 * レイヤアイテム
 *****************************************/

#include "mDef.h"
#include "mTree.h"
#include "mRectBox.h"
#include "mStr.h"
#include "mTrans.h"

#include "defTileImage.h"

#include "LayerItem.h"
#include "TileImage.h"
#include "MaterialImgList.h"
#include "file_apd_v3.h"

#include "trgroup.h"
#include "trid_word.h"


//-----------------------

#define _ITEM_PARENT(p)    ((LayerItem *)((p)->i.parent))
#define _ITEM_LAST(p)      ((LayerItem *)((p)->i.last))
#define _NEXTITEM_ROOT(p,root)  ((LayerItem *)mTreeItemGetNext_root((mTreeItem *)p, (mTreeItem *)root))

//-----------------------


//==============================
// セット
//==============================


/** 指定レイヤが表示されるようにする
 *
 * 親フォルダをすべて表示させる。
 * 指定レイヤはフォルダの場合は、下位もすべて表示させる。 */

void LayerItem_setVisible(LayerItem *item)
{
	LayerItem *pi;

	//item と親

	for(pi = item; pi; pi = _ITEM_PARENT(pi))
		pi->flags |= LAYERITEM_F_VISIBLE;

	//フォルダ下

	for(pi = item; pi; pi = _NEXTITEM_ROOT(pi, item))
		pi->flags |= LAYERITEM_F_VISIBLE;
}

/** 親をすべて展開させ、リスト上に表示されるようにする */

void LayerItem_setExpandParent(LayerItem *p)
{
	for(p = _ITEM_PARENT(p); p; p = _ITEM_PARENT(p))
		p->flags |= LAYERITEM_F_FOLDER_EXPAND;
}


/** アイテムリンクをセット
 *
 * @param pptop  先頭アイテム (NULL で p がセットされる)
 * @param pplast 最後のリンクアイテム */

void LayerItem_setLink(LayerItem *p,LayerItem **pptop,LayerItem **pplast)
{
	if(*pptop)
		(*pplast)->link = p;
	else
		*pptop = p;

	p->link = NULL;
	*pplast = p;
}

/** アイテムリンクをセット＆次のアイテムを返す
 *
 * - フォルダの場合は、フォルダ下のイメージをすべてリンク (フォルダ自身は除く)。
 * - exclude_lock が TRUE の場合、ロックされているレイヤは除く。 */

LayerItem *LayerItem_setLink_toNext(LayerItem *root,
	LayerItem **pptop,LayerItem **pplast,mBool exclude_lock)
{
	LayerItem *pi;

	for(pi = root; pi; pi = _NEXTITEM_ROOT(pi, root))
	{
		if(LAYERITEM_IS_IMAGE(pi))
		{
			//ロックされている場合は除く
			
			if(exclude_lock && LayerItem_isLock_real(pi))
				continue;

			LayerItem_setLink(pi, pptop, pplast);
		}
	}

	return (LayerItem *)mTreeItemGetNextPass((mTreeItem *)root);
}


//==============================
// 
//==============================


/** 名前を「"レイヤ"＋番号」にセット */

void LayerItem_setName_layerno(LayerItem *p,int no)
{
	mStr str = MSTR_INIT;

	mStrSetFormat(&str, "%s%d",
		M_TR_T2(TRGROUP_WORD, TRID_WORD_LAYER), no);

	mStrdup_ptr(&p->name, str.buf);

	mStrFree(&str);
}

/** テクスチャをセット */

void LayerItem_setTexture(LayerItem *p,const char *path)
{
	//現在のイメージを解放

	MaterialImgList_releaseImage(MATERIALIMGLIST_TYPE_TEXTURE, p->img8texture);

	//セット

	mStrdup_ptr(&p->texture_path, path);

	p->img8texture = MaterialImgList_getImage(MATERIALIMGLIST_TYPE_LAYER_TEXTURE, path, TRUE);
}

/** レイヤ色をセット */

void LayerItem_setLayerColor(LayerItem *p,uint32_t col)
{
	p->col = col;

	if(p->img)
		TileImage_setImageColor(p->img, col);
}

/** イメージを置換え (レイヤ色はコピーしない) */

void LayerItem_replaceImage(LayerItem *p,TileImage *img)
{
	if(p->img)
		TileImage_free(p->img);

	p->img = img;

	p->coltype = img->coltype;
}

/** 情報をコピー */

void LayerItem_copyInfo(LayerItem *dst,LayerItem *src)
{
	mStrdup_ptr(&dst->name, src->name);
	mStrdup_ptr(&dst->texture_path, src->texture_path);

	dst->opacity = src->opacity;
	dst->blendmode = src->blendmode;
	dst->alphamask = src->alphamask;
	dst->flags = src->flags;

	LayerItem_setLayerColor(dst, src->col);
}

/** イメージ全体の編集 (反転/回転)
 *
 * フォルダの場合は下位もすべて。ロックレイヤや空イメージは除く。
 *
 * @param type  0=左右反転 1=上下反転 2=左回転 3=右回転
 * @param rcupdate 更新範囲が入る
 * @return 一つでも処理されたか */

mBool LayerItem_editFullImage(LayerItem *item,int type,mRect *rcupdate)
{
	LayerItem *pi;
	mRect rc,rc2;
	mBool ret = FALSE;
	void (*func[4])(TileImage *) = {
		TileImage_fullReverse_horz, TileImage_fullReverse_vert,
		TileImage_fullRotate_left, TileImage_fullRotate_right
	};

	mRectEmpty(&rc);

	for(pi = item; pi; pi = _NEXTITEM_ROOT(pi, item))
	{
		if(pi->img && !LayerItem_isLock_real(pi))
		{
			//現在のイメージ範囲 (空イメージの場合は除く)
			
			if(!TileImage_getHaveImageRect_pixel(pi->img, &rc2, NULL))
				continue;

			mRectUnion(&rc, &rc2);

			//処理

			(func[type])(pi->img);

			//処理後のイメージ範囲

			if(TileImage_getHaveImageRect_pixel(pi->img, &rc2, NULL))
				mRectUnion(&rc, &rc2);

			ret = TRUE;
		}
	}

	*rcupdate = rc;

	return ret;
}


//==============================
// 情報取得
//==============================


/** レイヤ色が必要なカラータイプか */

mBool LayerItem_isType_layercolor(LayerItem *p)
{
	return (p->img &&
		(p->coltype == TILEIMAGE_COLTYPE_ALPHA16 || p->coltype == TILEIMAGE_COLTYPE_ALPHA1));
}

/** p がレイヤ一覧上に表示されている状態か
 *
 * 親がすべて展開されているか */

mBool LayerItem_isVisibleOnList(LayerItem *p)
{
	for(p = _ITEM_PARENT(p); p; p = _ITEM_PARENT(p))
	{
		if(!LAYERITEM_IS_EXPAND(p))
			return FALSE;
	}

	return TRUE;
}

/** 実際に表示されている状態か (親の状態を適用) */

mBool LayerItem_isVisible_real(LayerItem *p)
{
	for(; p; p = _ITEM_PARENT(p))
	{
		if(!LAYERITEM_IS_VISIBLE(p)) return FALSE;
	}
	
	return TRUE;
}

/** ロックされているか (親の状態も適用) */

mBool LayerItem_isLock_real(LayerItem *p)
{
	for(; p; p = _ITEM_PARENT(p))
	{
		if(LAYERITEM_IS_LOCK(p)) return TRUE;
	}

	return FALSE;
}

/** p が parent 以下の階層に存在するか */

mBool LayerItem_isChildItem(LayerItem *p,LayerItem *parent)
{
	return mTreeItemIsChild(M_TREEITEM(p), M_TREEITEM(parent));
}

/** ツリーの深さ取得 */

int LayerItem_getTreeDepth(LayerItem *p)
{
	mTreeItem *pi;
	int depth = 0;

	for(pi = p->i.parent; pi; pi = pi->parent, depth++);

	return depth;
}

/** 合成時の不透明度取得 (親の状態も適用) */

int LayerItem_getOpacity_real(LayerItem *p)
{
	int n = p->opacity;

	for(p = _ITEM_PARENT(p); p; p = _ITEM_PARENT(p))
	{
		if(p->opacity != 128)
			n = (n * p->opacity + 64) >> 7;
	}

	return n;
}

/** マスク類の状態をフラグで取得 */

int LayerItem_checkMasks(LayerItem *root)
{
	LayerItem *p;
	int ret = 0;

	//ロック

	for(p = root; p; p = _ITEM_PARENT(p))
	{
		if(p->flags & LAYERITEM_F_LOCK)
		{
			ret |= 1<<0;
			break;
		}
	}

	//

	if(root->img)
	{
		//下レイヤマスク

		if(root->flags & LAYERITEM_F_MASK_UNDER)
			ret |= 1<<1;

		//アルファマスク

		if(root->alphamask) ret |= 1<<2;
	}

	return ret;
}

/** 表示されているイメージの範囲取得 (キャンバス範囲外の部分も含む)
 *
 * フォルダの場合、フォルダ下の全ての表示レイヤの範囲。
 *
 * @return FALSE で表示範囲がない */

mBool LayerItem_getVisibleImageRect(LayerItem *item,mRect *rc)
{
	LayerItem *pi;
	mRect rc1,rc2;

	mRectEmpty(&rc1);

	if(LAYERITEM_IS_IMAGE(item))
	{
		//通常レイヤ

		if(LAYERITEM_IS_VISIBLE(item))
			TileImage_getHaveImageRect_pixel(item->img, &rc1, NULL);
	}
	else
	{
		//フォルダ

		pi = LayerItem_getNextVisibleImage(item, item);

		for(; pi; pi = LayerItem_getNextVisibleImage(pi, item))
		{
			if(TileImage_getHaveImageRect_pixel(pi->img, &rc2, NULL))
				mRectUnion(&rc1, &rc2);
		}
	}

	*rc = rc1;

	return !mRectIsEmpty(&rc1);
}

/** p を先頭としてリンクされているレイヤのイメージの表示されている範囲を取得 */

mBool LayerItem_getVisibleImageRect_link(LayerItem *p,mRect *rc)
{
	mRect rc1,rc2;

	mRectEmpty(&rc1);

	for(; p; p = p->link)
	{
		if(LayerItem_getVisibleImageRect(p, &rc2))
			mRectUnion(&rc1, &rc2);
	}

	*rc = rc1;

	return !mRectIsEmpty(&rc1);
}


//==============================
// アイテム取得
//==============================


/** 次のツリーアイテム */

LayerItem *LayerItem_getNext(LayerItem *p)
{
	return (LayerItem *)mTreeItemGetNext(M_TREEITEM(p));
}

/** 子を飛ばした次のツリーアイテム */

LayerItem *LayerItem_getNextPass(LayerItem *p)
{
	return (LayerItem *)mTreeItemGetNextPass(M_TREEITEM(p));
}

/** root 以下の次のツリーアイテム */

LayerItem *LayerItem_getNextRoot(LayerItem *p,LayerItem *root)
{
	return (LayerItem *)mTreeItemGetNext_root(M_TREEITEM(p), M_TREEITEM(root));
}

/** 一つ前のツリーアイテム */

LayerItem *LayerItem_getPrev(LayerItem *p)
{
	return (LayerItem *)mTreeItemGetPrev(M_TREEITEM(p));
}

/** 自身も含む、一つ前の表示イメージアイテム取得 */

LayerItem *LayerItem_getPrevVisibleImage_incSelf(LayerItem *p)
{
	while(p)
	{
		if(!LAYERITEM_IS_VISIBLE(p))
			//非表示 -> 前のアイテムへ
			p = (LayerItem *)mTreeItemGetPrevPass(M_TREEITEM(p));
		else
		{
			//表示

			if(LAYERITEM_IS_IMAGE(p))  //通常レイヤなら終了
				break;
			else if(p->i.last)         //フォルダで子がある場合、最後の子
				p = _ITEM_LAST(p);
			else                       //フォルダで子がなければ、前のアイテム
				p = (LayerItem *)mTreeItemGetPrevPass(M_TREEITEM(p));
		}
	}

	return p;
}

/** 一つ前の表示イメージアイテム取得 */

LayerItem *LayerItem_getPrevVisibleImage(LayerItem *p)
{
	p = (LayerItem *)mTreeItemGetPrevPass(M_TREEITEM(p));

	if(p)
		p = LayerItem_getPrevVisibleImage_incSelf(p);

	return p;
}

/** 一つ前の展開されたアイテム取得 */

LayerItem *LayerItem_getPrevExpand(LayerItem *p)
{
	if(!p->i.prev)
		//前がなければ親へ
		return (LayerItem *)p->i.parent;
	else
	{
		//展開されたフォルダの場合は最後のアイテム
	
		p = LAYERITEM(p->i.prev);

		for(; p && p->i.first && LAYERITEM_IS_EXPAND(p); p = LAYERITEM(p->i.last));

		return p;
	}
}

/** 前の通常アイテム取得 (PSD 保存用) */

LayerItem *LayerItem_getPrevNormal(LayerItem *p)
{
	p = (LayerItem *)mTreeItemGetPrev(M_TREEITEM(p));

	for(; p && !p->img; p = (LayerItem *)mTreeItemGetPrev(M_TREEITEM(p)));

	return p;
}

/** 次の展開されたアイテム取得 (レイヤ一覧表示用) */

LayerItem *LayerItem_getNextExpand(LayerItem *p)
{
	if(p->i.first && LAYERITEM_IS_EXPAND(p))
		//子があり展開されている場合は子へ
		return (LayerItem *)p->i.first;
	else
		//次へ
		return (LayerItem *)mTreeItemGetNextPass(M_TREEITEM(p));
}

/** 次の表示イメージアイテム取得
 *
 * @param root この下位のみ取得 */

LayerItem *LayerItem_getNextVisibleImage(LayerItem *p,LayerItem *root)
{
	LayerItem *top = p;

	while(p)
	{
		if(LAYERITEM_IS_FOLDER(p))
		{
			//フォルダ

			if(p->i.first && LAYERITEM_IS_VISIBLE(p))
				p = (LayerItem *)p->i.first;
			else
				//非表示 or 子がない
				p = (LayerItem *)mTreeItemGetNextPass_root(M_TREEITEM(p), M_TREEITEM(root));
		}
		else
		{
			//通常レイヤ

			if(p != top && LAYERITEM_IS_VISIBLE(p))
				break;
			else
				p = (LayerItem *)mTreeItemGetNextPass_root(M_TREEITEM(p), M_TREEITEM(root));
		}
	}

	return p;
}


//=================================
// APD (単体レイヤ) 保存
//=================================


/** APD 保存 */

mBool LayerItem_saveAPD_single(LayerItem *item,const char *filename,mPopupProgress *prog)
{
	saveAPDv3 *sav;

	sav = saveAPDv3_open(filename, prog);
	if(!sav) return FALSE;

	//レイヤヘッダ

	saveAPDv3_writeLayerHeader(sav, 1, 0);

	//レイヤ情報

	saveAPDv3_beginLayerInfo(sav);
	saveAPDv3_writeLayerInfo(sav, item);
	saveAPDv3_endThunk(sav);

	//タイル

	saveAPDv3_beginLayerTile(sav);
	saveAPDv3_writeLayerTile(sav, item);
	saveAPDv3_endThunk(sav);

	saveAPDv3_writeEnd(sav);

	saveAPDv3_close(sav);

	return TRUE;
}
