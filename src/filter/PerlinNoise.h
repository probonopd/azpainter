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
 * PerlinNoise 生成
 ***************************************/

#ifndef PERLIN_NOISE_H
#define PERLIN_NOISE_H

typedef struct _PerlinNoise PerlinNoise;

PerlinNoise *PerlinNoise_new(double freq,double persis);
void PerlinNoise_free(PerlinNoise *p);

double PerlinNoise_getNoise(PerlinNoise *p,double x,double y);

#endif
