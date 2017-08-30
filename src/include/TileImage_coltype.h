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
 * TileImage カラータイプ別の関数
 **********************************/

#ifndef TILEIMAGE_COLTYPE_FUNCS_H
#define TILEIMAGE_COLTYPE_FUNCS_H

typedef struct _TileImageBlendInfo TileImageBlendInfo;

/* func table */

typedef mBool (*TileImageColFunc_isSameColor)(RGBAFix15 *,RGBAFix15 *);
typedef uint8_t *(*TileImageColFunc_getPixelBufAtTile)(TileImage *,uint8_t *,int,int);
typedef void (*TileImageColFunc_getPixelColAtTile)(TileImage *,uint8_t *,int,int,RGBAFix15 *);
typedef void (*TileImageColFunc_setPixel)(TileImage *,uint8_t *,int,int,RGBAFix15 *);
typedef mBool (*TileImageColFunc_isTransparentTile)(uint8_t *);
typedef void (*TileImageColFunc_blendTile)(TileImage *,TileImageBlendInfo *);
typedef void (*TileImageColFunc_getTileRGBA)(TileImage *,void *,uint8_t *);
typedef void (*TileImageColFunc_setTileFromRGBA)(uint8_t *,void *,mBool);
typedef void (*TileImageColFunc_editTile)(uint8_t *);
typedef void (*TileImageColFunc_getsetTileForSave)(uint8_t *,uint8_t *);

typedef struct
{
	TileImageColFunc_isSameColor isSameColor,
		isSameRGB;
	TileImageColFunc_getPixelBufAtTile getPixelBufAtTile;
	TileImageColFunc_getPixelColAtTile getPixelColAtTile;
	TileImageColFunc_setPixel setPixel;
	TileImageColFunc_isTransparentTile isTransparentTile;
	TileImageColFunc_blendTile blendTile;
	TileImageColFunc_getTileRGBA getTileRGBA;
	TileImageColFunc_setTileFromRGBA setTileFromRGBA;
	TileImageColFunc_editTile reverseHorz,
		reverseVert,
		rotateLeft,
		rotateRight;
	TileImageColFunc_getsetTileForSave getTileForSave,
		setTileForSave;
}TileImageColtypeFuncs;

extern TileImageColtypeFuncs g_tileimage_funcs[4];

/* set table */

void __TileImage_RGBA_setFuncTable(TileImageColtypeFuncs *p);
void __TileImage_Gray_setFuncTable(TileImageColtypeFuncs *p);
void __TileImage_A16_setFuncTable(TileImageColtypeFuncs *p);
void __TileImage_A1_setFuncTable(TileImageColtypeFuncs *p);

/* A1 */

void __TileImage_A1_blendXorToPixbuf(TileImage *p,mPixbuf *pixbuf,TileImageBlendInfo *info);

#endif
