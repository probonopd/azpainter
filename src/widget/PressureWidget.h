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
 * 筆圧操作ウィジェット
 ************************************/

#ifndef PRESSURE_WIDGET_H
#define PRESSURE_WIDGET_H

typedef struct _PressureWidget PressureWidget;


PressureWidget *PressureWidget_new(mWidget *parent,int id);

void PressureWidget_setValue(PressureWidget *p,int type,uint32_t val);
uint32_t PressureWidget_getValue(PressureWidget *p);

#endif
