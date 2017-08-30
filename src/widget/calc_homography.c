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
 * 射影変換 計算
 *****************************************/

#include <string.h>

#include "mDef.h"


/** 4x4 逆行列にする */

static void _mat_reverse(double *mat)
{
	int i,j,k,n,ni;
	double d,src[16];

	memcpy(src, mat, sizeof(double) * 16);

	for(i = 0, n = 0; i < 4; i++)
		for(j = 0; j < 4; j++, n++)
			mat[n] = (i == j);

	for(i = 0; i < 4; i++)
	{
		ni = i << 2;

		/* [!] ここで、src の値が 0 だとゼロ除算になり
		 * 正しく計算できないので、
		 * 座標値が 0 にならないようにすること。 */

		d = 1 / src[ni + i];

		for(j = 0, n = ni; j < 4; j++, n++)
		{
			src[n] *= d;
			mat[n] *= d;
		}

		for(j = 0; j < 4; j++)
		{
			if(i != j)
			{
				d = src[(j << 2) + i];

				for(k = 0, n = j << 2; k < 4; k++, n++)
				{
					src[n] -= src[ni + k] * d;
					mat[n] -= mat[ni + k] * d;
				}
			}
		}
	}
}

/** xy 共通計算 */

static void _calc_xy(double *dst,double *t,
	double d1,double d2,double d3,double d4)
{
	_mat_reverse(t);

	dst[0] = t[0] * d1 + t[1] * d2 + t[2] * d3 + t[3] * d4;
	dst[1] = t[0] + t[1] + t[2] + t[3];

	dst[2] = t[4] * d1 + t[5] * d2 + t[6] * d3 + t[7] * d4;
	dst[3] = t[4] + t[5] + t[6] + t[7];

	dst[4] = t[8] * d1 + t[9] * d2 + t[10] * d3 + t[11] * d4;
	dst[5] = t[8] + t[9] + t[10] + t[11];

	dst[6] = t[12] * d1 + t[13] * d2 + t[14] * d3 + t[15] * d4;
	dst[7] = t[12] + t[13] + t[14] + t[15];
}

/** 射影変換 逆変換用パラメータ取得
 *
 * @param dst 9個 */

void getHomographyRevParam(double *dst,mDoublePoint *s,mDoublePoint *d)
{
	double t[16],xp[8],yp[8],m,c,f;

	//x

	t[0] = s[0].x, t[1] = s[0].y;
	t[2] = -s[0].x * d[0].x, t[3] = -s[0].y * d[0].x;

	t[4] = s[1].x, t[5] = s[1].y;
	t[6] = -s[1].x * d[1].x, t[7] = -s[1].y * d[1].x;

	t[8] = s[2].x, t[9] = s[2].y;
	t[10] = -s[2].x * d[2].x, t[11] = -s[2].y * d[2].x;

	t[12] = s[3].x, t[13] = s[3].y;
	t[14] = -s[3].x * d[3].x, t[15] = -s[3].y * d[3].x;

	_calc_xy(xp, t, d[0].x, d[1].x, d[2].x, d[3].x);

	//y

	t[0] = s[0].x, t[1] = s[0].y;
	t[2] = -s[0].x * d[0].y, t[3] = -s[0].y * d[0].y;

	t[4] = s[1].x, t[5] = s[1].y;
	t[6] = -s[1].x * d[1].y, t[7] = -s[1].y * d[1].y;

	t[8] = s[2].x, t[9] = s[2].y;
	t[10] = -s[2].x * d[2].y, t[11] = -s[2].y * d[2].y;

	t[12] = s[3].x, t[13] = s[3].y;
	t[14] = -s[3].x * d[3].y, t[15] = -s[3].y * d[3].y;

	_calc_xy(yp, t, d[0].y, d[1].y, d[2].y, d[3].y);

	//

	m = xp[5] * -yp[7] - (-yp[5] * xp[7]);
	if(m == 0) m = 0.0001;

	m = 1 / m;

	//変換用行列

	c = (-yp[7] * m) * (xp[4] - yp[4]) + (yp[5] * m) * (xp[6] - yp[6]);
	f = (-xp[7] * m) * (xp[4] - yp[4]) + (xp[5] * m) * (xp[6] - yp[6]);

	memset(t, 0, sizeof(double) * 16);

	t[0] = xp[0] - c * xp[1];
	t[1] = xp[2] - c * xp[3];
	t[2] = c;

	t[4] = yp[0] - f * yp[1];
	t[5] = yp[2] - f * yp[3];
	t[6] = f;

	t[8] = xp[4] - c * xp[5];
	t[9] = xp[6] - c * xp[7];
	t[10] = 1;

	t[15] = 1;

	//逆変換用パラメータ

	_mat_reverse(t);

	dst[0] = t[0];
	dst[1] = t[1];
	dst[2] = t[2];
	dst[3] = t[4];
	dst[4] = t[5];
	dst[5] = t[6];
	dst[6] = t[8];
	dst[7] = t[9];
	dst[8] = t[10];
}

