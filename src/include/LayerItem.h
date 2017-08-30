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

/************************************
 * レイヤアイテム定義
 ************************************/

#ifndef LAYERITEM_H
#define LAYERITEM_H

#include "mTreeDef.h" 

typedef struct _LayerItem LayerItem;
typedef struct _TileImage TileImage;
typedef struct _ImageBuf8 ImageBuf8;
typedef struct _mPopupProgress mPopupProgress;


/** レイヤアイテム */

struct _LayerItem
{
	mTreeItem i;

	TileImage *img;			//レイヤイメージ
	ImageBuf8 *img8texture;	//テクスチャイメージ
	LayerItem *link;		//作業用リンク
	
	char *name,     	//レイヤ名 (NULL で空文字列)
		*texture_path;	//テクスチャパス (NULL でなし)
	int	opacity,   		//レイヤの不透明度 [0-128]
		coltype,		//カラータイプ
		blendmode,		//合成モード
		alphamask;		//アルファマスクタイプ
	uint32_t flags, 	//フラグ
		col;			//レイヤ色

	int tmp1,tmp2;	//読み書き時の作業用
};

/* フラグ */

enum
{
	LAYERITEM_F_VISIBLE = 1<<0,
	LAYERITEM_F_FOLDER_EXPAND = 1<<1,
	LAYERITEM_F_LOCK     = 1<<2,
	LAYERITEM_F_FILLREF  = 1<<3,
	LAYERITEM_F_MASK_UNDER = 1<<4,
	LAYERITEM_F_CHECKED  = 1<<5
};

/* macros */

#define LAYERITEM_OPACITY_MAX    128

#define LAYERITEM(p)               ((LayerItem *)(p))
#define LAYERITEM_IS_VISIBLE(p)    ((p)->flags & LAYERITEM_F_VISIBLE)
#define LAYERITEM_IS_FOLDER(p)     (!(p)->img)
#define LAYERITEM_IS_IMAGE(p)      ((p)->img != 0)
#define LAYERITEM_IS_LOCK(p)       ((p)->flags & LAYERITEM_F_LOCK)
#define LAYERITEM_IS_EXPAND(p)     ((p)->flags & LAYERITEM_F_FOLDER_EXPAND)
#define LAYERITEM_IS_FILLREF(p)    ((p)->flags & LAYERITEM_F_FILLREF)
#define LAYERITEM_IS_CHECKED(p)    ((p)->flags & LAYERITEM_F_CHECKED)
#define LAYERITEM_IS_MASK_UNDER(p) ((p)->flags & LAYERITEM_F_MASK_UNDER)


/*---- function ----*/

void LayerItem_setVisible(LayerItem *p);
void LayerItem_setExpandParent(LayerItem *p);

void LayerItem_setLink(LayerItem *p,LayerItem **pptop,LayerItem **pplast);
LayerItem *LayerItem_setLink_toNext(LayerItem *p,LayerItem **pptop,LayerItem **pplast,mBool exclude_lock);

void LayerItem_setName_layerno(LayerItem *p,int no);
void LayerItem_setTexture(LayerItem *p,const char *path);
void LayerItem_setLayerColor(LayerItem *p,uint32_t col);
void LayerItem_replaceImage(LayerItem *p,TileImage *img);

void LayerItem_copyInfo(LayerItem *dst,LayerItem *src);
mBool LayerItem_editFullImage(LayerItem *item,int type,mRect *rcupdate);

/* get */

mBool LayerItem_isType_layercolor(LayerItem *p);

mBool LayerItem_isVisibleOnList(LayerItem *p);
mBool LayerItem_isVisible_real(LayerItem *p);
mBool LayerItem_isLock_real(LayerItem *p);
mBool LayerItem_isChildItem(LayerItem *p,LayerItem *parent);

int LayerItem_getTreeDepth(LayerItem *p);
int LayerItem_getOpacity_real(LayerItem *p);

int LayerItem_checkMasks(LayerItem *p);

mBool LayerItem_getVisibleImageRect(LayerItem *p,mRect *rc);
mBool LayerItem_getVisibleImageRect_link(LayerItem *p,mRect *rc);

/* item */

LayerItem *LayerItem_getNext(LayerItem *p);
LayerItem *LayerItem_getNextPass(LayerItem *p);
LayerItem *LayerItem_getNextRoot(LayerItem *p,LayerItem *root);
LayerItem *LayerItem_getPrev(LayerItem *p);
LayerItem *LayerItem_getPrevVisibleImage_incSelf(LayerItem *p);
LayerItem *LayerItem_getPrevVisibleImage(LayerItem *p);
LayerItem *LayerItem_getPrevExpand(LayerItem *p);
LayerItem *LayerItem_getPrevNormal(LayerItem *p);
LayerItem *LayerItem_getNextExpand(LayerItem *p);
LayerItem *LayerItem_getNextVisibleImage(LayerItem *p,LayerItem *root);

mBool LayerItem_saveAPD_single(LayerItem *item,const char *filename,mPopupProgress *prog);

#endif
