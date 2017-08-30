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

/*****************************************
 * ColorValue
 *****************************************/

#include "mDef.h"

#include "ColorValue.h"


/** 0-100 の値を 0-0x8000 に変換 */

int Density100toFix15(int n)
{
	return (int)(n / 100.0 * 0x8000 + 0.5);
}


/** RGBAFix15 -> RGB */

uint32_t RGBAFix15toRGB(RGBAFix15 *src)
{
	int r,g,b;

	r = (src->r * 255 + 0x4000) >> 15;
	g = (src->g * 255 + 0x4000) >> 15;
	b = (src->b * 255 + 0x4000) >> 15;

	return (r << 16) | (g << 8) | b;
}

/** RGB 値 -> RGBFix15 */

void RGBtoRGBFix15(RGBFix15 *dst,uint32_t col)
{
	dst->r = ((M_GET_R(col) << 15) + 127) / 255;
	dst->g = ((M_GET_G(col) << 15) + 127) / 255;
	dst->b = ((M_GET_B(col) << 15) + 127) / 255;
}
