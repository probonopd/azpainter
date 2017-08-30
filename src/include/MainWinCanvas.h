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
 * MainWinCanvas 関数
 ************************************/

#ifndef MAINWINCANVAS_H
#define MAINWINCANVAS_H

/* MainWinCanvas */

void MainWinCanvas_new(mWidget *parent);
void MainWinCanvas_setScrollInfo();
void MainWinCanvas_setScrollPos();

/* MainWinCanvasArea */

void MainWinCanvasArea_setTimer_updateArea(int time);
void MainWinCanvasArea_clearTimer_updateArea();

void MainWinCanvasArea_setTimer_updateRect(mBox *boximg);
void MainWinCanvasArea_clearTimer_updateRect(mBool update);

void MainWinCanvasArea_setTimer_updateMove();
void MainWinCanvasArea_clearTimer_updateMove();

void MainWinCanvasArea_setTimer_updateSelectMove();
void MainWinCanvasArea_clearTimer_updateSelectMove();

void MainWinCanvasArea_setTimer_updateSelectImageMove();
mBool MainWinCanvasArea_clearTimer_updateSelectImageMove();

void MainWinCanvasArea_setTimer_scroll(mPoint *ptdir);
void MainWinCanvasArea_clearTimer_scroll(mPoint *ptsum);

void MainWinCanvasArea_changeDrawCursor();
void MainWinCanvasArea_setCursor_forTool();
void MainWinCanvasArea_setCursor(int no);
void MainWinCanvasArea_setCursor_wait();
void MainWinCanvasArea_restoreCursor();

mBool MainWinCanvasArea_isPressKey_space();
int MainWinCanvasArea_getPressRawKey();

#endif
