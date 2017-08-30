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

/********************************
 * アプリケーション用カーソル
 ********************************/

#ifndef APP_CURSOR_H
#define APP_CURSOR_H

enum APP_CURSOR
{
	APP_CURSOR_DRAW,	//fix
	APP_CURSOR_WAIT,	//fix

	APP_CURSOR_HAND,
	APP_CURSOR_HAND_DRAG,
	APP_CURSOR_ROTATE,
	APP_CURSOR_ITEM_MOVE,
	APP_CURSOR_MOVE,
	APP_CURSOR_SPOIT,
	APP_CURSOR_SELECT,
	APP_CURSOR_SEL_MOVE,
	APP_CURSOR_STAMP,
	APP_CURSOR_TEXT,
	APP_CURSOR_ZOOM_DRAG,

	APP_CURSOR_LEFT_TOP,
	APP_CURSOR_RIGHT_TOP,
	APP_CURSOR_RESIZE_HORZ,
	APP_CURSOR_RESIZE_VERT
};

void AppCursor_init(uint8_t *drawcursor);
void AppCursor_free();

void AppCursor_setDrawCursor(uint8_t *buf);

mCursor AppCursor_getWaitCursor();
mCursor AppCursor_getForCanvas(int no);
mCursor AppCursor_getForDrag(int no);
mCursor AppCursor_getForDialog(int no);

#endif
