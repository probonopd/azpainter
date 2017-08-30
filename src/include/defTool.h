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
 * ツールの列挙値定義
 ********************************************/

#ifndef DEF_TOOL_H
#define DEF_TOOL_H

/* ツール番号 */

enum TOOLNO
{
	TOOL_BRUSH,
	TOOL_FILL_POLYGON,
	TOOL_FILL_POLYGON_ERASE,
	TOOL_FILL,
	TOOL_FILL_ERASE,
	TOOL_GRADATION,
	TOOL_TEXT,
	TOOL_MOVE,
	TOOL_MAGICWAND,
	TOOL_SELECT,
	TOOL_BOXEDIT,
	TOOL_STAMP,
	TOOL_CANVAS_MOVE,
	TOOL_CANVAS_ROTATE,
	TOOL_SPOIT,

	TOOL_NUM
};

/* 描画タイプ */

enum TOOLSUBNO
{
	TOOLSUB_DRAW_FREE,
	TOOLSUB_DRAW_LINE,
	TOOLSUB_DRAW_BOX,
	TOOLSUB_DRAW_ELLIPSE,
	TOOLSUB_DRAW_SUCCLINE,
	TOOLSUB_DRAW_CONCLINE,
	TOOLSUB_DRAW_BEZIER,
	TOOLSUB_DRAW_SPLINE,

	TOOLSUB_DRAW_NUM
};

/* 選択範囲サブ */

enum
{
	TOOLSUB_SEL_BOX,
	TOOLSUB_SEL_POLYGON,
	TOOLSUB_SEL_LASSO,
	TOOLSUB_SEL_IMGMOVE,
	TOOLSUB_SEL_IMGCOPY,
	TOOLSUB_SEL_POSMOVE,

	TOOLSUB_SEL_NUM
};

#endif
