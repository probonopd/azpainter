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
 * DrawData 操作関連の定義
 **********************************/

#ifndef DRAW_OP_DEF_H
#define DRAW_OP_DEF_H

/* DrawData::w.optype (現在の操作タイプ) */

enum
{
	DRAW_OPTYPE_NONE,
	DRAW_OPTYPE_GENERAL,
	DRAW_OPTYPE_DRAW_FREE,
	DRAW_OPTYPE_SELIMGMOVE,

	DRAW_OPTYPE_XOR_LINE,
	DRAW_OPTYPE_XOR_BOXAREA,
	DRAW_OPTYPE_XOR_BOXIMAGE,
	DRAW_OPTYPE_XOR_ELLIPSE,
	DRAW_OPTYPE_XOR_SUMLINE,
	DRAW_OPTYPE_XOR_BEZIER,
	DRAW_OPTYPE_XOR_POLYGON,
	DRAW_OPTYPE_XOR_LASSO,
	DRAW_OPTYPE_SPLINE
};

/* DrawData::w.opsubtype (操作タイプのサブ情報) */

enum
{
	DRAW_OPSUB_DRAW_LINE,
	DRAW_OPSUB_DRAW_FRAME,
	DRAW_OPSUB_DRAW_FILL,
	DRAW_OPSUB_DRAW_GRADATION,
	DRAW_OPSUB_DRAW_SUCCLINE,
	DRAW_OPSUB_DRAW_CONCLINE,
	DRAW_OPSUB_SET_STAMP,
	DRAW_OPSUB_SET_SELECT,
	DRAW_OPSUB_SET_BOXEDIT,

	DRAW_OPSUB_TO_BEZIER,
	DRAW_OPSUB_RULE_SETTING
};

/* funcAction() 時のアクション */

enum
{
	DRAW_FUNCACTION_RBTT_UP,
	DRAW_FUNCACTION_LBTT_DBLCLK,
	DRAW_FUNCACTION_KEY_ENTER,
	DRAW_FUNCACTION_KEY_ESC,
	DRAW_FUNCACTION_KEY_BACKSPACE,
	DRAW_FUNCACTION_UNGRAB
};

/* DrawData::w.opflags (操作中のオプションフラグ) */

enum
{
	DRAW_OPFLAGS_MOTION_DISABLE_STATE = 1<<0,	//ポインタ移動時、装飾キー無効
	DRAW_OPFLAGS_MOTION_POS_INT = 1<<1,			//ポインタ移動時、位置は整数値で取得
	DRAW_OPFLAGS_BRUSH_PRESSURE_MAX = 1<<2		//ブラシ描画時、常に筆圧最大
};

/* DrawData::w.drawinfo_flags */

#define DRAW_DRAWINFO_F_BRUSH_STROKE  1  //ブラシ描画でストローク重ね塗りを行う

#endif
