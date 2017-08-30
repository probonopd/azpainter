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
 * フィルタダイアログ用の数値バー
 ************************************/

#ifndef FILTERBAR_H
#define FILTERBAR_H

typedef struct _FilterBar FilterBar;

#define FILTERBAR(p)  ((FilterBar *)(p))


FilterBar *FilterBar_new(mWidget *parent,int id,uint32_t fLayout,int initw,
	int min,int max,int pos,int center);

int FilterBar_getPos(FilterBar *p);
mBool FilterBar_setPos(FilterBar *p,int pos);
void FilterBar_setStatus(FilterBar *p,int min,int max,int pos);

#endif
