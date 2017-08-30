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

/**********************************
 * DrawData レイヤ処理
 **********************************/

#ifndef DRAW_LAYER_H
#define DRAW_LAYER_H

void drawLayer_newLayer(DrawData *p,const char *name,int type,int blendmode,uint32_t col,LayerItem *pi_above);
void drawLayer_newFolder(DrawData *p,const char *name,LayerItem *pi_above);
void drawLayer_newLayer_direct(DrawData *p,int type);
mBool drawLayer_newLayer_fromImage(DrawData *p,TileImage *img,uint32_t col);

int drawLayer_newLayer_file(DrawData *p,const char *filename,mBool ignore_alpha,mRect *rcupdate);

void drawLayer_copy(DrawData *p);
void drawLayer_delete(DrawData *p,mBool update);
void drawLayer_erase(DrawData *p);
void drawLayer_setColorType(DrawData *p,LayerItem *item,int type,mBool bLumtoAlpha);
void drawLayer_editFullImage(DrawData *p,int type);

void drawLayer_combine(DrawData *p,mBool drop);
void drawLayer_combineMulti(DrawData *p,int target,mBool newlayer,int coltype);
void drawLayer_blendAll(DrawData *p);

mBool drawLayer_setCurrent(DrawData *p,LayerItem *item);
void drawLayer_setCurrent_visibleOnList(DrawData *p,LayerItem *item);
void drawLayer_setTexture(DrawData *p,const char *path);
void drawLayer_setLayerColor(DrawData *p,LayerItem *item,uint32_t col);

void drawLayer_revFolderExpand(DrawData *p,LayerItem *item);
void drawLayer_revVisible(DrawData *p,LayerItem *item);
void drawLayer_revLock(DrawData *p,LayerItem *item);
void drawLayer_revFillRef(DrawData *p,LayerItem *item);
void drawLayer_revChecked(DrawData *p,LayerItem *item);
void drawLayer_setLayerMask(DrawData *p,LayerItem *item,int type);

void drawLayer_showAll(DrawData *p,int type);
void drawLayer_showRevChecked(DrawData *p);
void drawLayer_showRevImage(DrawData *p);

void drawLayer_closeAllFolders(DrawData *p);

void drawLayer_allFlagsOff(DrawData *p,uint32_t flags);

void drawLayer_moveUpDown(DrawData *p,mBool bUp);
void drawLayer_moveDND(DrawData *p,LayerItem *dst,int type);
void drawLayer_moveCheckedToFolder(DrawData *p,LayerItem *dst);

void drawLayer_deleteForUndo(DrawData *p,LayerItem *item);
void drawLayer_afterMoveList_forUndo(DrawData *p);

void drawLayer_currentSelUpDown(DrawData *p,mBool up);
void drawLayer_selectPixelTopLayer(DrawData *p);

#endif
