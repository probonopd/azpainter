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

/********************************
 * TileImage 内部処理
 ********************************/

#ifndef TILEIMAGE_PV_H
#define TILEIMAGE_PV_H

typedef struct _TileImage TileImage;
typedef struct _ImageBuf8 ImageBuf8;
typedef union  _RGBFix15 RGBFix15;
typedef union  _RGBAFix15 RGBAFix15;

typedef void (*TileImageFunc_BlendColor)(RGBFix15 *,RGBFix15 *);


#define TILEIMAGE_DOTSTYLE_RADIUS_MAX  50

/** 作業用データ */

typedef struct
{
	uint8_t *dotstyle;		//ドット描画時の円形形状 (1bit)
	RGBAFix15 *fingerbuf;	//指先の前回のイメージ
	int dot_size,		//セットされた形状の直径サイズ
		drawsubx,		//描画時の形状相対位置
		drawsuby;
}TileImageWorkData;

extern TileImageWorkData g_tileimage_work;

/** 合成用情報 */

typedef struct _TileImageBlendInfo
{
	uint8_t *tile;
	RGBFix15 *dst;
	ImageBuf8 *imgtex;
	int sx,sy,dx,dy,w,h,pitch_dst,opacity;
	TileImageFunc_BlendColor funcBlend;
}TileImageBlendInfo;


/* TileImage_pv.c */

TileImage *__TileImage_create(int type,int tilew,int tileh);
void __TileImage_setTileSize(TileImage *p);

mBool __TileImage_allocTileBuf(TileImage *p);
uint8_t **__TileImage_allocTileBuf_new(int w,int h,mBool clear);

mBool __TileImage_resizeTileBuf(TileImage *p,int movx,int movy,int neww,int newh);
mBool __TileImage_resizeTileBuf_clone(TileImage *p,TileImage *src);

void __TileImage_setBlendInfo(TileImageBlendInfo *info,int px,int py,mRect *rcclip);

/* TileImage_pixel.c */

uint8_t *__TileImage_getPixelBuf_new(TileImage *p,int x,int y);

#endif
