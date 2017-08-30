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

/***************************************
 * 合成モード定義
 ***************************************/

#ifndef DEF_BLENDMODE_H
#define DEF_BLENDMODE_H

enum
{
	BLENDMODE_NORMAL,
	BLENDMODE_MUL,
	BLENDMODE_ADD,
	BLENDMODE_SUB,
	BLENDMODE_SCREEN,
	BLENDMODE_OVERLAY,
	BLENDMODE_HARD_LIGHT,
	BLENDMODE_SOFT_LIGHT,
	BLENDMODE_DODGE,
	BLENDMODE_BURN,
	BLENDMODE_LINEAR_BURN,
	BLENDMODE_VIVID_LIGHT,
	BLENDMODE_LINEAR_LIGHT,
	BLENDMODE_PIN_LIGHT,
	BLENDMODE_DARK,
	BLENDMODE_LIGHT,
	BLENDMODE_DIFF,

	BLENDMODE_NUM
};

#endif
