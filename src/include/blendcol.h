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

/********************************
 * レイヤの色合成関数
 ********************************/

#ifndef BLENDCOL_H
#define BLENDCOL_H

typedef union _RGBFix15 RGBFix15;
typedef void (*BlendColorFunc)(RGBFix15 *,RGBFix15 *);

void BlendColor_setFuncTable();

void BlendColor_normal(RGBFix15 *src,RGBFix15 *dst);
void BlendColor_mul(RGBFix15 *src,RGBFix15 *dst);
void BlendColor_add(RGBFix15 *src,RGBFix15 *dst);
void BlendColor_sub(RGBFix15 *src,RGBFix15 *dst);
void BlendColor_screen(RGBFix15 *src,RGBFix15 *dst);
void BlendColor_overlay(RGBFix15 *src,RGBFix15 *dst);
void BlendColor_hardlight(RGBFix15 *src,RGBFix15 *dst);
void BlendColor_softlight(RGBFix15 *src,RGBFix15 *dst);
void BlendColor_dodge(RGBFix15 *src,RGBFix15 *dst);
void BlendColor_burn(RGBFix15 *src,RGBFix15 *dst);
void BlendColor_linearburn(RGBFix15 *src,RGBFix15 *dst);
void BlendColor_vividlight(RGBFix15 *src,RGBFix15 *dst);
void BlendColor_linearlight(RGBFix15 *src,RGBFix15 *dst);
void BlendColor_pinlight(RGBFix15 *src,RGBFix15 *dst);
void BlendColor_darken(RGBFix15 *src,RGBFix15 *dst);
void BlendColor_lighten(RGBFix15 *src,RGBFix15 *dst);
void BlendColor_difference(RGBFix15 *src,RGBFix15 *dst);

extern BlendColorFunc g_blendcolfuncs[17];

#endif
