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

/**************************************
 * PerlinNoise 生成
 **************************************/

#include <string.h>
#include <math.h>

#include "mDef.h"
#include "mRandXorShift.h"


//------------------

typedef struct
{
	uint8_t *buf;
	double freq,persis;
}PerlinNoise;

//------------------


//=========================
// sub
//=========================


static double _lerp(double t,double a,double b)
{
	return a + t * (b - a);
}

static double _grad(int hash,double x,double y)
{
	int h = hash & 15;
	double u,v;

	u = (h < 8)? x: y;
	v = (h < 4)? y: ((h == 12 || h == 14)? x: 0);

	return ((h & 1) == 0? u: -u) + ((h & 2) == 0? v: -v);
}

static double _noise(uint8_t *buf,double x,double y)
{
	double fx,fy,u,v,d1,d2;
	int nx,ny,a,aa,ab,b,ba,bb;

	fx = floor(x);
	fy = floor(y);

	nx = ((int)fx) & 255;
	ny = ((int)fy) & 255;

	x -= fx;
	y -= fy;

	u = x * x * (3 - 2 * x);
	v = y * y * (3 - 2 * y);

	a  = buf[nx] + ny;
	aa = buf[a];
	ab = buf[a + 1];
	b  = buf[nx + 1] + ny;
	ba = buf[b];
	bb = buf[b + 1];

	//

	d1 = _lerp(u, _grad(buf[aa], x, y), _grad(buf[ba], x - 1, y));
	d2 = _lerp(u, _grad(buf[ab], x, y - 1), _grad(buf[bb], x - 1, y - 1));

	return _lerp(v, d1, d2);
}


//=========================
// main
//=========================


/** 解放 */

void PerlinNoise_free(PerlinNoise *p)
{
	if(p)
	{
		mFree(p->buf);
		mFree(p);
	}
}

/** 作成 */

PerlinNoise *PerlinNoise_new(double freq,double persis)
{
	PerlinNoise *p;
	uint8_t *pd,tmp;
	int i,pos;

	p = (PerlinNoise *)mMalloc(sizeof(PerlinNoise), TRUE);
	if(!p) return NULL;

	p->freq = freq;
	p->persis = persis;

	//バッファ確保

	p->buf = (uint8_t *)mMalloc(512, FALSE);
	if(!p->buf)
	{
		mFree(p);
		return NULL;
	}

	//バッファセット

	pd = p->buf;

	for(i = 0; i < 256; i++)
		*(pd++) = i;

	//入れ替え

	pd = p->buf;
	
	for(i = 0; i < 256; i++)
	{
		pos = mRandXorShift_getIntRange(0, 255);

		tmp = pd[i];
		pd[i] = pd[pos];
		pd[pos] = tmp;
	}

	//コピー

	memcpy(pd + 256, pd, 256);

	return p;
}

/** 取得 */

double PerlinNoise_getNoise(PerlinNoise *p,double x,double y)
{
	double total,amp;
	int i;

	total = 0;
	amp = p->persis;

	x *= p->freq;
	y *= p->freq;

	for(i = 0; i < 8; i++)
	{
		total += _noise(p->buf, x, y) * amp;

		x *= 2;
		y *= 2;
		amp *= p->persis;
	}

	return total;
}
