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

/***************************
 * sin テーブル
 ***************************/

#ifndef SINTABLE_H
#define SINTABLE_H

extern float g_sintable[512];

void SinTable_init();

#define SINTABLE_GETSIN(n)  g_sintable[n & 511]
#define SINTABLE_GETCOS(n)  g_sintable[(n + 128) & 511]

#endif
