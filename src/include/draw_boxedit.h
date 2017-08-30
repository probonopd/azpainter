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

/**********************************
 * DrawData 矩形編集
 **********************************/

#ifndef DRAW_BOXEDIT_H
#define DRAW_BOXEDIT_H

void drawBoxEdit_clear_noupdate(DrawData *p);
void drawBoxEdit_restoreCursor(DrawData *p);
void drawBoxEdit_setBox(DrawData *p,mRect *rcsel);

void drawBoxEdit_run(DrawData *p,int type);

#endif
