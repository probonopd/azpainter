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

#ifndef UNDO_H
#define UNDO_H

typedef struct _LayerItem LayerItem;
typedef struct _TileImageInfo TileImageInfo;

typedef struct _UndoUpdateInfo
{
	int type;		//更新のタイプ
	mRect rc;		//イメージ範囲
	LayerItem *layer;	//指定レイヤ
}UndoUpdateInfo;

/** 更新のタイプ */
enum
{
	UNDO_UPDATE_NONE,
	UNDO_UPDATE_RECT,		//単体レイヤの一部イメージ (layer にそのレイヤ)
	UNDO_UPDATE_RECT_AND_LAYERLIST,
	UNDO_UPDATE_RECT_AND_LAYERLIST_ONE,		//レイヤ一覧は指定レイヤのみ更新 (パラメータ部分は更新なし)
	UNDO_UPDATE_RECT_AND_LAYERLIST_DOUBLE,	//レイヤ一覧は指定レイヤとその下のレイヤを更新
	UNDO_UPDATE_ALL_AND_LAYERLIST,	//イメージ全体とレイヤ一覧
	UNDO_UPDATE_LAYERLIST,			//レイヤ一覧全体を更新
	UNDO_UPDATE_LAYERLIST_ONE,		//レイヤ一覧上の指定レイヤのみを更新
	UNDO_UPDATE_CANVAS_RESIZE		//キャンバスサイズ変更 (rc.x1 = w, rc.y1 = h)
};

/*--------*/

mBool Undo_new();
void Undo_free();

void Undo_clearUpdateFlag();
void Undo_setMaxNum(int num);

mBool Undo_isHave(mBool redo);
mBool Undo_isChange();

void Undo_deleteAll();
mBool Undo_runUndoRedo(mBool redo,UndoUpdateInfo *info);


void Undo_addTilesImage(TileImageInfo *info,mRect *rc);

void Undo_addLayerNew(mBool with_image);
void Undo_addLayerCopy();
void Undo_addLayerDelete();
void Undo_addLayerClearImage();
void Undo_addLayerSetColorType(LayerItem *item,int type,mBool bLumtoA);
void Undo_addLayerDropAndCombine(mBool drop);
void Undo_addLayerCombineAll();
void Undo_addLayerCombineFolder();
void Undo_addLayerFlags(LayerItem *item,uint32_t flags);
void Undo_addLayerFullEdit(int type);
void Undo_addLayerMoveOffset(int mx,int my,LayerItem *item);
void Undo_addLayerMoveList(LayerItem *item,int parent,int pos);
void Undo_addLayerMoveList_multi(int num,int16_t *dat);

void Undo_addResizeCanvas_moveOffset(int mx,int my,int w,int h);
void Undo_addResizeCanvas_crop(int w,int h);
void Undo_addScaleCanvas();

#endif
