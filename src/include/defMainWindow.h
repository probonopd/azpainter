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
 * MainWindow ウィジェット定義
 **********************************/

#ifndef DEF_MAINWINDOW_H
#define DEF_MAINWINDOW_H

#include "mWindowDef.h"
#include "mStrDef.h"

typedef struct _MainWindow
{
	mWidget wg;
	mContainerData ct;
	mWindowData win;

	mWidget *ct_main,
			*splitter[3],	//ペイン用スプリッター
			*toolbar;

	mMenu *menu_main,
		*menu_recentfile;

	mStr strFilename;		//現在の編集ファイル名 (空で新規作成)
	uint32_t fileformat;	//現在のファイルのフォーマットフラグ
	mBool saved;			//一度でも保存されたか (FALSE で、新規か読み込んだ後保存されていない)
}MainWindow;

#define MAINWINDOW(p)  ((MainWindow *)(p))
#define MAINWINDOW_CMDID_RECENTFILE  0x10000

#define MAINWINDOW_CMDID_ZOOM_TOP    10000
/* +2000 までが範囲。
 * ショートカットキーのコマンドとしても使うので、0xffff までの範囲にしなければいけない。 */

#endif
