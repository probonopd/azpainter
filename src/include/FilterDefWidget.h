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

/***************************************
 * フィルタの定義ウィジェット
 ***************************************/

#ifndef FILTER_DEF_WIDGET_H
#define FILTER_DEF_WIDGET_H

/* レベル補正 */

typedef struct _FilterWgLevel FilterWgLevel;

FilterWgLevel *FilterWgLevel_new(mWidget *parent,int id,uint32_t *histogram);
void FilterWgLevel_getValue(FilterWgLevel *p,int *buf);

/* 色置き換え */

typedef struct _FilterWgRepcol FilterWgRepcol;

#define FILTERWGREPCOL_N_UPDATE_PREV  0
#define FILTERWGREPCOL_N_CHANGE_EDIT  1

FilterWgRepcol *FilterWgRepcol_new(mWidget *parent,int id,uint32_t drawcol);
void FilterWgRepcol_getColor(FilterWgRepcol *p,int *dst);

#endif
