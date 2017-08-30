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

/********************************************************
 * 外部からドックウィジェットの内容を変更する時の関数
 ********************************************************/

#ifndef DOCKS_EXTERNAL_H
#define DOCKS_EXTERNAL_H

typedef struct _LayerItem LayerItem;

//ツール

void DockTool_changeIconSize();
void DockTool_changeTool();
void DockTool_changeToolSub();

//オプション

void DockOption_changeTool();
void DockOption_changeStampImage();

//レイヤ

void DockLayer_changeIconSize();
void DockLayer_update_all();
void DockLayer_update_curparam();
void DockLayer_update_curlayer(mBool setparam);
void DockLayer_update_layer(LayerItem *item);
void DockLayer_update_layer_curparam(LayerItem *item);
void DockLayer_update_changecurrent_visible(LayerItem *lastitem,mBool update_all);

//ブラシ

void DockBrush_changeBrushSize(int radius);
void DockBrush_changeBrushSel();

//カラー

void DockColor_changeDrawColor();
void DockColor_changeDrawColor_hsv(int col);
void DockColor_changeColorMask();
void DockColor_changeTheme();

//カラーホイール

void DockColorWheel_changeDrawColor(int hsvcol);
void DockColorWheel_changeTheme();

//カラーパレット

void DockColorPalette_changeTheme();

//キャンバス操作

void DockCanvasCtrl_setPos();

//キャンバスビュー

void DockCanvasView_changeIconSize();
void DockCanvasView_update();
void DockCanvasView_changeImageSize();
void DockCanvasView_updateRect(mBox *boximg);
void DockCanvasView_changeLoupePos(mPoint *pt);

//イメージビューア

void DockImageViewer_changeIconSize();

#endif
