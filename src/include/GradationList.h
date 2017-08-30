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
 * グラデーションデータ
 ************************************/

#ifndef GRADATION_LIST_H
#define GRADATION_LIST_H

#include "mListDef.h"

typedef union _RGBFix15 RGBFix15;


#define GRADATION_MAX_ITEM  200  /* 選択番号のビット数が決まっているので、最大 254 */
#define GRADATION_MAX_POINT 30
#define GRADITEM(p)         ((GradItem *)(p))

#define GRADDAT_F_LOOP        1
#define GRADDAT_F_SINGLE_COL  2

#define GRADDAT_POS_BIT   10
#define GRADDAT_POS_MAX   (1 << GRADDAT_POS_BIT)
#define GRADDAT_POS_MASK  ((1<<11) - 1)

#define GRADDAT_ALPHA_BIT  10


typedef struct _GradItem
{
	mListItem i;
	uint8_t *buf;
	int size;
}GradItem;


/* list */

void GradationList_init();
void GradationList_free();

void GradationList_load();
void GradationList_save();

GradItem *GradationList_newItem();
void GradationList_delItem(GradItem *item);
GradItem *GradationList_copyItem(GradItem *item);

GradItem *GradationList_getTopItem();
uint8_t *GradationList_getBuf_atIndex(int index);
const uint8_t *GradationList_getDefaultDrawData();

/* item */

uint16_t *GradItem_convertDrawData(const uint8_t *src,RGBFix15 *drawcol,RGBFix15 *bkgndcol);
void GradItem_setDat(GradItem *p,uint8_t flags,int colnum,int anum,uint8_t *colbuf,uint8_t *abuf);
void GradItem_draw(mPixbuf *pixbuf,
	int x,int y,int w,int h,uint8_t *colbuf,uint8_t *abuf,int single_col);

#endif
