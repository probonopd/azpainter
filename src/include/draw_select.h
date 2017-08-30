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
 * DrawData 選択範囲関連
 **********************************/

#ifndef DRAW_SELECT_H
#define DRAW_SELECT_H

mBool drawSel_isHave();
void drawSel_release(DrawData *p,mBool update);

void drawSel_inverse(DrawData *p);
void drawSel_all(DrawData *p);
void drawSel_fill_erase(DrawData *p,mBool erase);
void drawSel_copy_cut(DrawData *p,mBool cut);
void drawSel_paste_newlayer(DrawData *p);
void drawSel_expand(DrawData *p,int cnt);

void drawSel_getFullDrawRect(DrawData *p,mRect *rc);
mBool drawSel_createImage(DrawData *p);
void drawSel_freeEmpty(DrawData *p);

#endif
