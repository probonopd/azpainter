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
 * スピン、小数部つきの数値選択バー
 ************************************/

#ifndef VALUEBAR_H
#define VALUEBAR_H

typedef struct _ValueBar ValueBar;

#define VALUEBAR(p)  ((ValueBar *)(p))


ValueBar *ValueBar_new(mWidget *parent,int id,uint32_t fLayout,
	int dig,int min,int max,int pos);

int ValueBar_getPos(ValueBar *p);
mBool ValueBar_setPos(ValueBar *p,int pos);
void ValueBar_setStatus(ValueBar *p,int min,int max,int pos);
void ValueBar_setStatus_dig(ValueBar *p,int dig,int min,int max,int pos);

#endif
