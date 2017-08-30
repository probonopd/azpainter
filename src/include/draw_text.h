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

/*****************************
 * テキスト描画処理関数
 *****************************/

#ifndef DRAW_DRAWTEXT_H
#define DRAW_DRAWTEXT_H

void drawText_createFont();
void drawText_setHinting(DrawData *p);

void drawText_drawPreview(DrawData *p);
void drawText_setDrawPoint_inDialog(DrawData *p,int x,int y);

#endif
