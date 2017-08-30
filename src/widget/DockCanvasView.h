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

/***********************************
 * [dock] キャンバスビュー本体定義
 ***********************************/

#ifndef DOCK_CANVASVIEW_H
#define DOCK_CANVASVIEW_H

#include "DockObject.h"

#define DOCKCANVASVIEW(p)  ((DockCanvasView *)(p))

typedef struct _DockCanvasViewArea DockCanvasViewArea;

typedef struct _DockCanvasView
{
	DockObject obj;

	DockCanvasViewArea *area;
	mWidget *iconbtt;

	int zoom;
	double dscale,dscalediv,
		originX,originY;	//原点位置 (イメージ座標)
	mPoint ptScroll,		//スクロール位置
		ptLoupeCur;			//ルーペ時の現在カーソル位置 (イメージ座標)。比較用
}DockCanvasView;


void DockCanvasView_adjustScroll(DockCanvasView *p);
void DockCanvasView_setZoom_fit(DockCanvasView *p);
mBool DockCanvasView_setZoom(DockCanvasView *p,int zoom);

#endif
