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
 * 8bit R-G-B イメージ
 ************************************/

#ifndef IMAGEBUF24_H
#define IMAGEBUF24_H


typedef struct _ImageBuf24
{
	uint8_t *buf;
	int w,h,pitch;
}ImageBuf24;

/** キャンバス描画用データ */

typedef struct
{
	int scrollx,scrolly,
		mirror;
	double originx,originy,
		scalediv;
	uint32_t bkgndcol;
}ImageBuf24CanvasInfo;


ImageBuf24 *ImageBuf24_new(int w,int h);
void ImageBuf24_free(ImageBuf24 *p);

uint8_t *ImageBuf24_getPixelBuf(ImageBuf24 *p,int x,int y);

ImageBuf24 *ImageBuf24_loadFile(const char *filename);

void ImageBuf24_drawCanvas_nearest(ImageBuf24 *src,mPixbuf *dst,
	mBox *boxdst,ImageBuf24CanvasInfo *info);
void ImageBuf24_drawCanvas_oversamp(ImageBuf24 *src,mPixbuf *dst,
	mBox *boxdst,ImageBuf24CanvasInfo *info);

#endif
