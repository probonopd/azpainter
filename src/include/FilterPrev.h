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
 * フィルタ、ダイアログ中のプレビュー
 ************************************/

#ifndef FILTERPREV_H
#define FILTERPREV_H

typedef struct _FilterPrev FilterPrev;
typedef struct _TileImage TileImage;

FilterPrev *FilterPrev_new(mWidget *parent,int id,int w,int h,TileImage *imgsrc,int fullw,int fullh);

void FilterPrev_getDrawArea(FilterPrev *p,mRect *rc,mBox *box);
void FilterPrev_drawImage(FilterPrev *p,TileImage *img);

#endif
