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
 * MainWindow 関数
 ************************************/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

typedef struct _MainWindow MainWindow;
typedef struct _LayerItem  LayerItem;

enum
{
	MAINWINDOW_SAVEFILE_OVERWRITE,
	MAINWINDOW_SAVEFILE_RENAME,
	MAINWINDOW_SAVEFILE_DUP
};


void MainWindow_new();
void MainWindow_showStart(MainWindow *p);

void MainWindow_quit();

void MainWindow_setTitle(MainWindow *p);
void MainWindow_apperr(int err,const char *detail);
mBool MainWindow_confirmSave(MainWindow *p);
void MainWindow_updateNewCanvas(MainWindow *p,const char *filename);

void MainWindow_getProgressBarPos(mPoint *pt);

void MainWindow_newImage(MainWindow *p);
void MainWindow_openFile(MainWindow *p,int recentno);
mBool MainWindow_loadImage(MainWindow *p,const char *filename,int loadopt);
mBool MainWindow_saveFile(MainWindow *p,int savetype,int recentno);

void MainWindow_undoredo(MainWindow *p,mBool redo);
void MainWindow_onCanvasKeyCommand(int cmdid);

void MainWindow_layer_new_dialog(MainWindow *p,LayerItem *pi_above);
void MainWindow_layer_setoption(MainWindow *p,LayerItem *pi);
void MainWindow_layer_setcolor(MainWindow *p,LayerItem *pi);
void MainWindow_layer_settype(MainWindow *p,LayerItem *pi);

#endif
