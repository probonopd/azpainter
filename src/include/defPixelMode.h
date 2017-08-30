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
 * 塗りタイプ
 **********************************/

#ifndef DEF_PIXELMODE_H
#define DEF_PIXELMODE_H

enum
{
	/* 通常ブラシ用 */

	PIXELMODE_BLEND_PIXEL = 0,
	PIXELMODE_BLEND_STROKE,
	PIXELMODE_COMPARE_A,
	PIXELMODE_OVERWRITE_STYLE,
	PIXELMODE_OVERWRITE_SQUARE,
	PIXELMODE_DODGE,
	PIXELMODE_BURN,
	PIXELMODE_ADD,

	PIXELMODE_BRUSH_NUM,

	/* ドットペン時は消しゴムを追加 */

	PIXELMODE_DOTPEN_ERASE = PIXELMODE_BRUSH_NUM,
	PIXELMODE_DOTPEN_NUM,

	/* 消しゴム用 */

	PIXELMODE_ERASE_PIXEL = 0,
	PIXELMODE_ERASE_STROKE,

	PIXELMODE_ERASE_NUM,

	/* ツール用 */

	PIXELMODE_TOOL_BLEND = 0,
	PIXELMODE_TOOL_COMPARE_A,
	PIXELMODE_TOOL_OVERWRITE,
	PIXELMODE_TOOL_ERASE
};

#endif
