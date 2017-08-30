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

/**************************************
 * ImageBuf8 のプレビューウィジェット
 **************************************/

#ifndef PREVIMAGE8_H
#define PREVIMAGE8_H

typedef struct _PrevImage8 PrevImage8;
typedef struct _ImageBuf8  ImageBuf8;

PrevImage8 *PrevImage8_new(mWidget *parent,int hintw,int hinth,uint32_t fLayout);
void PrevImage8_setImage(PrevImage8 *p,ImageBuf8 *img,mBool bBrush);

#endif
