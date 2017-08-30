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
 * 素材画像管理リスト
 ************************************/

#ifndef MATERIALIMGLIST_H
#define MATERIALIMGLIST_H

typedef struct _ImageBuf8 ImageBuf8;

enum
{
	MATERIALIMGLIST_TYPE_LAYER_TEXTURE = 0,
	MATERIALIMGLIST_TYPE_BRUSH_TEXTURE,
	MATERIALIMGLIST_TYPE_BRUSH_SHAPE,

	MATERIALIMGLIST_TYPE_TEXTURE = 0,
	MATERIALIMGLIST_TYPE_BRUSH
};


void MaterialImgList_init();
void MaterialImgList_free();

ImageBuf8 *MaterialImgList_getImage(int type,const char *path,mBool keep);
void MaterialImgList_releaseImage(int type,ImageBuf8 *img);

#endif
