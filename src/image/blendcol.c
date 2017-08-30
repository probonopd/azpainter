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
 * レイヤの色合成
 *****************************************/

#include "mDef.h"

#include "blendcol.h"
#include "ColorValue.h"


//-----------------

BlendColorFunc g_blendcolfuncs[17];

#define _MAXVAL  0x8000

//-----------------


/** 関数テーブルをセット */

void BlendColor_setFuncTable()
{
	BlendColorFunc *p = g_blendcolfuncs;

	p[0] = BlendColor_normal;
	p[1] = BlendColor_mul;
	p[2] = BlendColor_add;
	p[3] = BlendColor_sub;
	p[4] = BlendColor_screen;
	p[5] = BlendColor_overlay;
	p[6] = BlendColor_hardlight;
	p[7] = BlendColor_softlight;
	p[8] = BlendColor_dodge;
	p[9] = BlendColor_burn;
	p[10] = BlendColor_linearburn;
	p[11] = BlendColor_vividlight;
	p[12] = BlendColor_linearlight;
	p[13] = BlendColor_pinlight;
	p[14] = BlendColor_darken;
	p[15] = BlendColor_lighten;
	p[16] = BlendColor_difference;
}

/** 通常 */

void BlendColor_normal(RGBFix15 *src,RGBFix15 *dst)
{

}

/** 乗算 */

void BlendColor_mul(RGBFix15 *src,RGBFix15 *dst)
{
	src->r = src->r * dst->r >> 15;
	src->g = src->g * dst->g >> 15;
	src->b = src->b * dst->b >> 15;	
}

/** 加算 */

void BlendColor_add(RGBFix15 *src,RGBFix15 *dst)
{
	int i,n;

	for(i = 0; i < 3; i++)
	{
		n = src->c[i] + dst->c[i];
		if(n > _MAXVAL) n = _MAXVAL;

		src->c[i] = n;
	}
}

/** 減算 */

void BlendColor_sub(RGBFix15 *src,RGBFix15 *dst)
{
	int i,n;

	for(i = 0; i < 3; i++)
	{
		n = dst->c[i] - src->c[i];
		if(n < 0) n = 0;

		src->c[i] = n;
	}
}

/** スクリーン */

void BlendColor_screen(RGBFix15 *src,RGBFix15 *dst)
{
	int i,s,d;

	for(i = 0; i < 3; i++)
	{
		s = src->c[i];
		d = dst->c[i];
		
		src->c[i] = s + d - (s * d >> 15);
	}
}

/** オーバーレイ */

void BlendColor_overlay(RGBFix15 *src,RGBFix15 *dst)
{
	int i,s,d;

	for(i = 0; i < 3; i++)
	{
		s = src->c[i];
		d = dst->c[i];
		
		if(d < 0x4000)
			src->c[i] = s * d >> 14;
		else
			src->c[i] = _MAXVAL - ((_MAXVAL - d) * (_MAXVAL - s) >> 14);
	}
}

/** ハードライト */

void BlendColor_hardlight(RGBFix15 *src,RGBFix15 *dst)
{
	int i,s,d;

	for(i = 0; i < 3; i++)
	{
		s = src->c[i];
		d = dst->c[i];
		
		if(s < 0x4000)
			src->c[i] = s * d >> 14;
		else
			src->c[i] = _MAXVAL - ((_MAXVAL - d) * (_MAXVAL - s) >> 14);
	}
}

/** ソフトライト */

void BlendColor_softlight(RGBFix15 *src,RGBFix15 *dst)
{
	int i,s,d,n;

	for(i = 0; i < 3; i++)
	{
		s = src->c[i];
		d = dst->c[i];

		n = s * d >> 15;

		src->c[i] = n + (d * (_MAXVAL - n - ((_MAXVAL - s) * (_MAXVAL - d) >> 15)) >> 15);
	}
}

/** 覆い焼き */

void BlendColor_dodge(RGBFix15 *src,RGBFix15 *dst)
{
	int i,s,d,n;

	for(i = 0; i < 3; i++)
	{
		s = src->c[i];
		d = dst->c[i];

		if(s == _MAXVAL)
			n = _MAXVAL;
		else
		{
			n = (d << 15) / (_MAXVAL - s);
			if(n > _MAXVAL) n = _MAXVAL;
		}

		src->c[i] = n;
	}
}

/** 焼き込み */

void BlendColor_burn(RGBFix15 *src,RGBFix15 *dst)
{
	int i,s,d,n;

	for(i = 0; i < 3; i++)
	{
		s = src->c[i];
		d = dst->c[i];

		if(s == 0)
			n = 0;
		else
		{
			n = _MAXVAL - ((_MAXVAL - d) << 15) / s;
			if(n < 0) n = 0;
		}

		src->c[i] = n;
	}
}

/** 焼き込みリニア */

void BlendColor_linearburn(RGBFix15 *src,RGBFix15 *dst)
{
	int i,n;

	for(i = 0; i < 3; i++)
	{
		n = dst->c[i] + src->c[i] - _MAXVAL;
		if(n < 0) n = 0;
		
		src->c[i] = n;
	}
}

/** ビビットライト */

void BlendColor_vividlight(RGBFix15 *src,RGBFix15 *dst)
{
	int i,s,d,n;

	for(i = 0; i < 3; i++)
	{
		s = src->c[i];
		d = dst->c[i];

		if(s < 0x4000)
		{
			n = _MAXVAL - (s << 1);

			if(d <= n || s == 0)
				n = 0;
			else
				n = ((d - n) << 15) / (s << 1);
		}
		else
		{
			n = 0x10000 - (s << 1);

			if(d >= n || n == 0)
				n = _MAXVAL;
			else
				n = (d << 15) / n;
		}

		src->c[i] = n;
	}
}

/** リニアライト */

void BlendColor_linearlight(RGBFix15 *src,RGBFix15 *dst)
{
	int i,n;

	for(i = 0; i < 3; i++)
	{
		n = (src->c[i] << 1) + dst->c[i] - _MAXVAL;

		if(n < 0) n = 0;
		else if(n > _MAXVAL) n = _MAXVAL;
		
		src->c[i] = n;
	}
}

/** ピンライト */

void BlendColor_pinlight(RGBFix15 *src,RGBFix15 *dst)
{
	int i,s,d,n;

	for(i = 0; i < 3; i++)
	{
		s = src->c[i];
		d = dst->c[i];

		if(s > 0x4000)
		{
			n = (s << 1) - _MAXVAL;
			if(n < d) n = d;
		}
		else
		{
			n = s << 1;
			if(n > d) n = d;
		}

		src->c[i] = n;
	}
}

/** 比較(暗) */

void BlendColor_darken(RGBFix15 *src,RGBFix15 *dst)
{
	int i;

	for(i = 0; i < 3; i++)
	{
		if(dst->c[i] < src->c[i])
			src->c[i] = dst->c[i];
	}
}

/** 比較(明) */

void BlendColor_lighten(RGBFix15 *src,RGBFix15 *dst)
{
	int i;

	for(i = 0; i < 3; i++)
	{
		if(dst->c[i] > src->c[i])
			src->c[i] = dst->c[i];
	}
}

/** 差の絶対値 */

void BlendColor_difference(RGBFix15 *src,RGBFix15 *dst)
{
	int i,n;

	for(i = 0; i < 3; i++)
	{
		n = src->c[i] - dst->c[i];
		if(n < 0) n = -n;

		src->c[i] = n;
	}
}

