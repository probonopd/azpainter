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

/******************************
 * カラー関連
 ******************************/

#ifndef COLORVALUE_H
#define COLORVALUE_H

#define RGBCONV_8_TO_FIX15(c)  ((((c) << 15) + 127) / 255)
#define RGBCONV_FIX15_TO_8(c)  (((c) * 255 + 0x4000) >> 15)

typedef union _RGB8
{
	struct{ uint8_t r,g,b; };
	uint8_t c[3];
}RGB8;

typedef union _RGBAFix15
{
	struct{ uint16_t r,g,b,a; };
	uint16_t c[4];
	uint64_t v64;
}RGBAFix15;

typedef union _RGBFix15
{
	struct{ uint16_t r,g,b; };
	uint16_t c[3];
}RGBFix15;

typedef union _RGBADouble
{
	struct{ double r,g,b,a; };
	double ar[4];
}RGBADouble;


int Density100toFix15(int n);

uint32_t RGBAFix15toRGB(RGBAFix15 *src);
void RGBtoRGBFix15(RGBFix15 *dst,uint32_t col);

#endif
