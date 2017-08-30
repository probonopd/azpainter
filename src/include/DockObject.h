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

/********************************************
 * DockObject
 ********************************************/

#ifndef DOCKOBJECT_H
#define DOCKOBJECT_H

typedef struct _DockObject  DockObject;
typedef struct _mDockWidget mDockWidget;
typedef struct _mDockWidgetState mDockWidgetState;

struct _DockObject
{
	mDockWidget *dockwg;
	void (*destroy)(DockObject *);	//破棄ハンドラ
	mWidget *(*create)(mDockWidget *,int,mWidget *);
};


/*-------*/

void DockObject_destroy(DockObject *p);

DockObject *DockObject_new(int no,int size,uint32_t dock_style,
	uint32_t window_style,mDockWidgetState *state,
	mWidget *(*func_create)(mDockWidget *,int,mWidget *));

void DockObject_showStart(DockObject *p);
mBool DockObject_canDoWidget(DockObject *p);
mBool DockObject_canDoWidget_visible(DockObject *p);
mBool DockObject_canTakeFocus(DockObject *p);
mWindow *DockObject_getOwnerWindow(DockObject *p);

int DockObject_getDockNo_fromID(int id);
int DockObject_getPaneNo_fromNo(int no);

void DockObject_relayout_inWindowMode(int dockno);

void DockObjects_relocate();
void DockObjects_all_windowMode(int type);
void DockObject_normalize_layout_config();

#endif
