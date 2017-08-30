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
 * DrawData 操作のメイン関数
 **********************************/

#ifndef DRAW_OP_MAIN_H
#define DRAW_OP_MAIN_H

mBool drawOp_onPress(DrawData *p,mEvent *ev);
mBool drawOp_onRelease(DrawData *p,mEvent *ev);
void drawOp_onMotion(DrawData *p,mEvent *ev);
mBool drawOp_onLBttDblClk(DrawData *p,mEvent *ev);
mBool drawOp_onKeyDown(DrawData *p,uint32_t key);
void drawOp_onUngrab(DrawData *p);

#endif
