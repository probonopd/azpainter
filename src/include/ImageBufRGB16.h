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

/******************************
 * ImageBufRGB16 定義
 ******************************/

#ifndef DEF_IMAGEBUFRGB16_H
#define DEF_IMAGEBUFRGB16_H

typedef struct _mPixbuf mPixbuf;
typedef struct _CanvasDrawInfo CanvasDrawInfo;
typedef union _RGBFix15 RGBFix15;
typedef union _RGBAFix15 RGBAFix15;

typedef struct _ImageBufRGB16
{
	RGBFix15 *buf;
	int width,height;
}ImageBufRGB16;



void ImageBufRGB16_free(ImageBufRGB16 *p);
ImageBufRGB16 *ImageBufRGB16_new(int width,int height);

RGBFix15 *ImageBufRGB16_getBufPt(ImageBufRGB16 *p,int x,int y);

void ImageBufRGB16_fill(ImageBufRGB16 *p,RGBFix15 *rgb);
void ImageBufRGB16_fillBox(ImageBufRGB16 *p,mBox *box,RGBFix15 *rgb);
void ImageBufRGB16_fillPlaidBox(ImageBufRGB16 *p,mBox *box,RGBFix15 *rgb1,RGBFix15 *rgb2);

void ImageBufRGB16_getTileRGBA(ImageBufRGB16 *p,RGBAFix15 *dst,int x,int y);
mBool ImageBufRGB16_toRGB8(ImageBufRGB16 *p);

void ImageBufRGB16_drawMainCanvas_nearest(ImageBufRGB16 *src,mPixbuf *dst,CanvasDrawInfo *info);
void ImageBufRGB16_drawMainCanvas_oversamp(ImageBufRGB16 *src,mPixbuf *dst,CanvasDrawInfo *info);
void ImageBufRGB16_drawMainCanvas_rotate_normal(ImageBufRGB16 *src,mPixbuf *dst,CanvasDrawInfo *info);
void ImageBufRGB16_drawMainCanvas_rotate_oversamp(ImageBufRGB16 *src,mPixbuf *dst,CanvasDrawInfo *info);

#endif
