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

/*************************************
 * [dock] イメージビューア本体定義
 *************************************/

#ifndef DOCK_IMAGEVIEWER_H
#define DOCK_IMAGEVIEWER_H

#include "mStrDef.h"
#include "DockObject.h"

#define DOCKIMAGEVIEWER(p)  ((DockImageViewer *)(p))

typedef struct _DockImageViewerArea DockImageViewerArea;

typedef struct _DockImageViewer
{
	DockObject obj;

	DockImageViewerArea *area;
	mWidget *iconbtt;
	
	ImageBuf24 *img;
	int scrx,scry,
		zoom;
	double dscale,dscalediv;

	mStr strFileName;
}DockImageViewer;


void DockImageViewer_adjustScroll(DockImageViewer *p);
void DockImageViewer_setZoom_fit(DockImageViewer *p);
mBool DockImageViewer_setZoom(DockImageViewer *p,int zoom);
void DockImageViewer_loadImage(DockImageViewer *p,const char *filename);

#endif
