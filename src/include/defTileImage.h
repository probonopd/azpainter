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

/*****************************
 * TileImage 本体の定義
 *****************************/

#ifndef DEF_TILEIMAGE_H
#define DEF_TILEIMAGE_H

#include "ColorValue.h"

typedef struct _TileImage TileImage;

struct _TileImage
{
	uint8_t **ppbuf;	//タイル配列
	TileImage *link;	//リンク (塗りつぶし時など)
	int coltype,		//カラータイプ
		tilesize,		//1つのタイルのバイト数
		tilew,tileh,	//タイル配列の幅と高さ
		offx,offy;		//オフセット位置
	RGBFix15 rgb;		//線の色
};


#define TILEIMAGE_TILE_EMPTY  ((uint8_t *)1)

#define TILEIMAGE_GETTILE_BUFPT(p,tx,ty)  ((p)->ppbuf + (ty) * (p)->tilew + (tx))
#define TILEIMAGE_GETTILE_PT(p,tx,ty)     *((p)->ppbuf + (ty) * (p)->tilew + (tx))

#endif
