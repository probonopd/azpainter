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
 * MainWindow 内部関数
 ************************************/

#ifndef MAINWINDOW_PV_H
#define MAINWINDOW_PV_H

typedef struct _MainWindow MainWindow;

void MainWindow_createToolBar(MainWindow *p,mBool init);

void MainWindow_layercmd(MainWindow *p,int id);
void MainWindow_filtercmd(int id);

/* MainWindow_file.c */

void MainWindow_setRecentFileMenu(MainWindow *p);

int MainWindow_runMenu_toolbarDrop_opensave(MainWindow *p,mBool save);
int MainWindow_runMenu_toolbarDrop_savedup(MainWindow *p);
void MainWindow_runMenu_toolbar_recentfile(MainWindow *p);

/* MainWindow_cmd.c */

void MainWindow_pane_layout(MainWindow *p);
void MainWindow_toggle_show_pane_splitter(MainWindow *p);

void MainWindow_cmd_show_panel_all(MainWindow *p);
void MainWindow_cmd_show_panel_toggle(int no);

void MainWindow_cmd_changeDPI(MainWindow *p);
void MainWindow_cmd_selectExpand(MainWindow *p);
void MainWindow_cmd_checkMaskState(MainWindow *p);
void MainWindow_cmd_envoption(MainWindow *p);
void MainWindow_cmd_panelLayout(MainWindow *p);
void MainWindow_cmd_resizeCanvas(MainWindow *p);
void MainWindow_cmd_scaleCanvas(MainWindow *p);
void MainWindow_cmd_selectOutputPNG(MainWindow *p);

#endif
