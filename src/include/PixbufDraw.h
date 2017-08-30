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
 * mPixbuf 描画関数
 ************************************/

#ifndef PIXBUFDRAW_H
#define PIXBUFDRAW_H

typedef struct _CanvasDrawInfo CanvasDrawInfo;

void pixbufDraw_cross_dash(mPixbuf *pixbuf,int x,int y,int size,mBox *box);
void pixbufDraw_dashBox_mono(mPixbuf *pixbuf,int x,int y,int w,int h);

void pixbufDrawGrid(mPixbuf *p,mBox *boxdst,mBox *boximg,
	int gridw,int gridh,uint32_t col,CanvasDrawInfo *info);

void pixbufDraw_setPixelSelectEdge(mPixbuf *pixbuf,int x,int y,mRect *rcclip);
void pixbufDraw_lineSelectEdge(mPixbuf *pixbuf,int x1,int y1,int x2,int y2,mRect *rcclip);

void pixbufDrawBoxEditFrame(mPixbuf *pixbuf,mBox *boximg,CanvasDrawInfo *info);

#endif
