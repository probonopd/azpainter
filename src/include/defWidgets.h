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
 * 全ウィジェットデータの定義
 ********************************************/

#ifndef DEF_WIDGETS_H
#define DEF_WIDGETS_H

typedef struct _MainWindow MainWindow;
typedef struct _MainWinCanvas MainWinCanvas;
typedef struct _MainWinCanvasArea MainWinCanvasArea;
typedef struct _DockObject DockObject;
typedef struct _StatusBar StatusBar;

/** ドックウィジェットのインデックス番号 */

enum
{
	DOCKWIDGET_TOOL,
	DOCKWIDGET_OPTION,
	DOCKWIDGET_LAYER,
	DOCKWIDGET_BRUSH,
	DOCKWIDGET_COLOR,
	DOCKWIDGET_COLOR_PALETTE,
	DOCKWIDGET_CANVAS_CTRL,
	DOCKWIDGET_CANVAS_VIEW,
	DOCKWIDGET_IMAGE_VIEWER,
	DOCKWIDGET_FILTER_LIST,
	DOCKWIDGET_COLOR_WHEEL,

	DOCKWIDGET_NUM
};

/** ウィジェット関連データ */

typedef struct _WidgetsData
{
	MainWindow *mainwin;
	mWidget *pane[3];		//パネルのペイン
	MainWinCanvas *canvas;
	MainWinCanvasArea *canvas_area;
	StatusBar *statusbar;
	DockObject *dockobj[DOCKWIDGET_NUM];

	mFont *font_dock,	//dock のフォント
		*font_dock12px;	//dock フォント (12px)
}WidgetsData;

//---------

extern WidgetsData *g_app_widgets;
#define APP_WIDGETS g_app_widgets

#endif
