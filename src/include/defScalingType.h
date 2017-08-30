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

/************************************
 * 拡大縮小補間方法
 ************************************/

#ifndef DEF_SCALING_TYPE_H
#define DEF_SCALING_TYPE_H

enum
{
	SCALING_TYPE_NEAREST_NEIGHBOR,
	SCALING_TYPE_MITCHELL,
	SCALING_TYPE_LANGRANGE,
	SCALING_TYPE_LANCZOS2,
	SCALING_TYPE_LANCZOS3,

	SCALING_TYPE_NUM
};

#endif
