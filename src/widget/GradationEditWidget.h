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

/************************************************
 * グラデーション編集ダイアログの編集ウィジェット
 ************************************************/

#ifndef GRADATION_EDIT_WIDGET_H
#define GRADATION_EDIT_WIDGET_H

typedef struct _GradEditWidget GradEditWidget;

enum
{
	GRADEDITWIDGET_N_CHANGE_CURRENT,
	GRADEDITWIDGET_N_MODIFY
};


GradEditWidget *GradEditWidget_new(mWidget *parent,int id,mBool mode_alpha,uint8_t *buf);

void GradEditWidget_setAnotherModeBuf(GradEditWidget *p,uint8_t *buf);
void GradEditWidget_setSingleColor(GradEditWidget *p,int type);

int GradEditWidget_getPointNum(GradEditWidget *p);
uint8_t *GradEditWidget_getBuf(GradEditWidget *p);
int GradEditWidget_getCurrentPos(GradEditWidget *p);
int GradEditWidget_getCurrentColor(GradEditWidget *p,uint32_t *col);
int GradEditWidget_getCurrentOpacity(GradEditWidget *p);
int GradEditWidget_getMaxSplitNum_toright(GradEditWidget *p);

mBool GradEditWidget_setCurrentPos(GradEditWidget *p,int pos);
void GradEditWidget_setOpacity(GradEditWidget *p,int val);
void GradEditWidget_setColorType(GradEditWidget *p,int type);
void GradEditWidget_setColorRGB(GradEditWidget *p,uint32_t col);

void GradEditWidget_moveCurrentPoint(GradEditWidget *p,mBool right);
void GradEditWidget_deletePoint(GradEditWidget *p,int index);
void GradEditWidget_moveCurrentPos_middle(GradEditWidget *p);
void GradEditWidget_evenAllPoints(GradEditWidget *p);
void GradEditWidget_reversePoints(GradEditWidget *p);
void GradEditWidget_splitPoint_toright(GradEditWidget *p,int num);

#endif
