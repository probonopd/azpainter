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
 * メインウィンドウのキャンバスウィジェット
 ********************************************/

#ifndef DEF_MAINWINCANVAS_H
#define DEF_MAINWINCANVAS_H

#include "mScrollView.h"
#include "mScrollViewArea.h"

/** キャンバスメイン */

typedef struct _MainWinCanvas
{
	mWidget wg;
	mScrollViewData sv;

	mPoint ptScrollBase;  //スクロール基準位置
}MainWinCanvas;

/** キャンバス領域部分 */

typedef struct _MainWinCanvasArea
{
	mWidget wg;
	mScrollViewAreaData sva;

	mCursor cursor_restore;
	mBox boxUpdate;
	mPoint ptTimerScrollDir,	//スクロール方向と移動数
		ptTimerScrollSum;		//総スクロール移動数
	
	mBool bPressSpace;	//スペースキーが押されているか
	int press_rawkey;	//現在押されているキー (キャンバスキー判定用)
}MainWinCanvasArea;

#endif
