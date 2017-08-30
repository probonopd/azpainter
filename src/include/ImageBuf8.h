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
 * 8bit イメージ
 ************************************/

#ifndef IMAGEBUF8_H
#define IMAGEBUF8_H

typedef struct _ImageBuf24 ImageBuf24;

typedef struct _ImageBuf8
{
	uint8_t *buf;
	uint32_t bufsize;
	int w,h;
}ImageBuf8;

#define IMAGEBUF8_GETBUFPT(p,x,y)   ((p)->buf + (y) * (p)->w + (x))


void ImageBuf8_free(ImageBuf8 *p);
ImageBuf8 *ImageBuf8_new(int w,int h);

void ImageBuf8_clear(ImageBuf8 *p);
uint8_t ImageBuf8_getPixel_forTexture(ImageBuf8 *p,int x,int y);

ImageBuf8 *ImageBuf8_createFromImageBuf24(ImageBuf24 *src);
ImageBuf8 *ImageBuf8_createFromImageBuf24_forBrush(ImageBuf24 *src);

void ImageBuf8_drawTexturePreview(ImageBuf8 *p,mPixbuf *pixbuf,int x,int y,int w,int h);
void ImageBuf8_drawBrushPreview(ImageBuf8 *p,mPixbuf *pixbuf,int x,int y,int w,int h);

#endif
