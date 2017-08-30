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

/*********************************************
 * DockCanvasCtl の操作バー
 *********************************************/

#ifndef CANVASCTRLBAR_H
#define CANVASCTRLBAR_H

typedef struct _CanvasCtrlBar CanvasCtrlBar;

#define CANVASCTRLBAR_TYPE_ZOOM  0
#define CANVASCTRLBAR_TYPE_ANGLE 1

enum
{
	CANVASCTRLBAR_BTT_SUB,
	CANVASCTRLBAR_BTT_ADD,
	CANVASCTRLBAR_BTT_RESET
};

/* 通知タイプ */

enum
{
	CANVASCTRLBAR_N_BAR,
	CANVASCTRLBAR_N_BUTTON
};

/* 通知の param1 */

enum
{
	CANVASCTRLBAR_N_BAR_TYPE_PRESS,
	CANVASCTRLBAR_N_BAR_TYPE_RELEASE,
	CANVASCTRLBAR_N_BAR_TYPE_MOTION
};

CanvasCtrlBar *CanvasCtrlBar_new(mWidget *parent,int type);
void CanvasCtrlBar_setPos(CanvasCtrlBar *p,int pos);

#endif
