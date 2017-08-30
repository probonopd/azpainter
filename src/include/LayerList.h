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
 * レイヤリスト
 ************************************/

#ifndef LAYERLIST_H
#define LAYERLIST_H

typedef struct _LayerList LayerList;
typedef struct _LayerItem LayerItem;
typedef struct _TileImage TileImage;
typedef struct _mPopupProgress mPopupProgress;



LayerList *LayerList_new();
void LayerList_free(LayerList *p);

void LayerList_clear(LayerList *p);

/* add, etc */

LayerItem *LayerList_addLayer(LayerList *p,LayerItem *insert);
LayerItem *LayerList_addLayer_image(LayerList *p,LayerItem *insert,int coltype);
LayerItem *LayerList_addLayer_pos(LayerList *p,int parent,int pos);
LayerItem *LayerList_addLayer_newimage(LayerList *p,int coltype);

LayerItem *LayerList_dupLayer(LayerList *p,LayerItem *src);
LayerItem *LayerList_deleteLayer(LayerList *p,LayerItem *item);

mBool LayerList_addLayers_onRoot(LayerList *p,int num);

/* 取得 */

int LayerList_getNum(LayerList *p);
int LayerList_getNormalLayerNum(LayerList *p);

int LayerList_getItemPos(LayerList *p,LayerItem *item);
void LayerList_getItemPos_forParent(LayerList *p,LayerItem *item,int *parent,int *relno);

LayerItem *LayerList_getItem_top(LayerList *p);
LayerItem *LayerList_getItem_byPos_topdown(LayerList *p,int pos);
LayerItem *LayerList_getItem_bottomVisibleImage(LayerList *p);
LayerItem *LayerList_getItem_bottomNormal(LayerList *p);
void LayerList_getItems_fromPos(LayerList *p,LayerItem **dst,int parent,int pos);
LayerItem *LayerList_getItem_topPixelLayer(LayerList *p,int x,int y);

int LayerList_getScrollInfo(LayerList *p,int *ret_maxdepth);

/* フラグ */

mBool LayerList_haveCheckedLayer(LayerList *p);
void LayerList_setVisible_all(LayerList *p,mBool on);
void LayerList_showRevChecked(LayerList *p,mRect *rcimg);
void LayerList_showRevImage(LayerList *p,mRect *rcimg);
void LayerList_allFlagsOff(LayerList *p,uint32_t flags);
void LayerList_closeAllFolders(LayerList *p,LayerItem *curitem);

/* 描画関連 */

TileImage *LayerList_getMaskImage(LayerList *p,LayerItem *current);
LayerItem *LayerList_setLink_combineMulti(LayerList *p,int target,LayerItem *current);
LayerItem *LayerList_setLink_movetool(LayerList *p,LayerItem *item);
LayerItem *LayerList_setLink_filltool(LayerList *p,LayerItem *current,mBool disable_ref);
LayerItem *LayerList_setLink_checked(LayerList *p);

/* アイテム移動 */

mBool LayerList_moveitem_updown(LayerList *p,LayerItem *item,mBool bUp);
void LayerList_moveitem(LayerList *p,LayerItem *src,LayerItem *dst,mBool infolder);
void LayerList_moveitem_forLoad_topdown(LayerList *p,LayerItem *src,LayerItem *parent);
mBool LayerList_moveitem_forUndo(LayerList *p,int *val,mRect *rcupdate);
int16_t *LayerList_moveitem_checkedToFolder(LayerList *p,LayerItem *dst,int *pnum,mRect *rcupdate);
mBool LayerList_moveitem_multi_forUndo(LayerList *p,int num,int16_t *buf,mRect *rcupdate,mBool redo);

/* ほか */

void LayerList_setItemName_curlayernum(LayerList *p,LayerItem *item);
void LayerList_setItemName_ascii(LayerList *p,LayerItem *item,const char *name);
void LayerList_setItemName_utf8(LayerList *p,LayerItem *item,const char *name);

void LayerList_moveOffset_rel_all(LayerList *p,int movx,int movy);

/* file */

LayerItem *LayerList_addLayer_apdv3(LayerList *p,LayerItem *insert,
	const char *filename,mPopupProgress *prog);

#endif
