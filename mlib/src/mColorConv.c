/*$
Copyright (c) 2017, Azel
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERSAND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
$*/

/*****************************************
 * 色変換関数
 *****************************************/

#include "mDef.h"


/*****************//**

@defgroup colorconv mColorConv
@brief 色変換関数

@ingroup group_util
@{

@file mColorConv.h

**********************/


//===============================
// HSV -> RGB
//===============================


/** HSV (0.0-1.0) -> RGB
 *
 * @param dst 0-255 の RGB 値３つが入る */

void mHSVtoRGB(double h,double s,double v,int *dst)
{
	double c1,c2,c3,r,g,b,f;
	int hi;

	if(s == 0)
		r = g = b = v;
	else
	{
		h *= 6.0;
		hi = (int)h;

		f = h - hi;
		hi %= 6;
	
		c1 = v * (1 - s);
		c2 = v * (1 - s * f);
		c3 = v * (1 - s * (1 - f));

		switch(hi)
		{
			case 0: r = v;  g = c3; b = c1; break;
			case 1: r = c2; g = v;  b = c1; break;
			case 2: r = c1; g = v;  b = c3; break;
			case 3: r = c1; g = c2; b = v;  break;
			case 4: r = c3; g = c1; b = v;  break;
			default: r = v;  g = c1; b = c2; break;
		}
	}

	dst[0] = (int)(r * 255.0 + 0.5);
	dst[1] = (int)(g * 255.0 + 0.5);
	dst[2] = (int)(b * 255.0 + 0.5);
}

/** HSV -> RGB (カラー値で返す) */

mRgbCol mHSVtoRGB_pac(double h,double s,double v)
{
	int c[3];

	mHSVtoRGB(h, s, v, c);

	return (c[0] << 16) | (c[1] << 8) | c[2];
}

/** HSV -> RGB (整数高速版)
 *
 * @param h 0-359
 * @param s,v 0-255 */

mRgbCol mHSVtoRGB_fast(int h,int s,int v)
{
	int c1,c2,c3,r,g,b;
	int t;

	if(s == 0)
		r = g = b = v;
	else
	{
		t  = (h * 6) % 360;
		c1 = (v * (255 - s)) / 255;
		c2 = (v * (255 - s * t / 360)) / 255;
		c3 = (v * (255 - s * (360 - t) / 360)) / 255;

		switch(h / 60)
		{
			case 0: r = v;  g = c3; b = c1; break;
			case 1: r = c2; g = v;  b = c1; break;
			case 2: r = c1; g = v;  b = c3; break;
			case 3: r = c1; g = c2; b = v;  break;
			case 4: r = c3; g = c1; b = v;  break;
			default: r = v;  g = c1; b = c2; break;
		}
	}

	return (r << 16) | (g << 8) | b;
}


//===============================
// RGB -> HSV
//===============================


/** RGB -> HSV (0.0-1.0) */

void mRGBtoHSV(int r,int g,int b,double *dst)
{
	double dr,dg,db,max,min,h,s;

	dr = r / 255.0;
	dg = g / 255.0;
	db = b / 255.0;

	//min,max

	max = (r >= g)? dr: dg;
	if(db > max) max = db;

	min = (r <= g)? dr: dg;
	if(db < min) min = db;

	//

	h = max - min;
	s = (max == 0)? 0: h / max;

	if(h > 0)
	{
		if(max == dr)
			h = (dg - db) / h;
		else if(max == dg)
			h = 2.0 + (db - dr) / h;
		else
			h = 4.0 + (dr - dg) / h;

		if(h < 0) h += 6.0;
		h /= 6.0;

		if(h >= 1.0) h -= 1.0;
	}

	dst[0] = h;
	dst[1] = s;
	dst[2] = max;
}

/** RGB -> HSV */

void mRGBtoHSV_pac(mRgbCol rgb,double *dst)
{
	mRGBtoHSV((rgb >> 16) & 0xff, (rgb >> 8) & 0xff, rgb & 0xff, dst);
}


//===============================
// HLS <-> RGB
//===============================


static double _hlstorgb_get_rgb(int h,double min,double max)
{
	if(h >= 360) h -= 360;
	else if(h < 0) h += 360;

	if(h < 60)
		return min + (max - min) * h / 60;
	else if(h < 180)
		return max;
	else if(h < 240)
		return min + (max - min) * (240 - h) / 60;

	return min;
}


/** HLS -> RGB */

void mHLStoRGB(int h,double l,double s,int *dst)
{
	double dr,dg,db,max,min;

	if(l <= 0.5)
		max = l * (1 + s);
	else
		max = l * (1 - s) + s;

	min = 2 * l - max;

	if(s == 0)
		dr = dg = db = l;
	else
	{
		dr = _hlstorgb_get_rgb(h + 120, min, max);
		dg = _hlstorgb_get_rgb(h, min, max);
		db = _hlstorgb_get_rgb(h - 120, min, max);
	}

	dst[0] = (int)(dr * 255 + 0.5);
	dst[1] = (int)(dg * 255 + 0.5);
	dst[2] = (int)(db * 255 + 0.5);
}

/** HLS -> RGB */

mRgbCol mHLStoRGB_pac(int h,double l,double s)
{
	int c[3];

	mHLStoRGB(h, l, s, c);

	return (c[0] << 16) | (c[1] << 8) | c[2];
}

/** RGB -> HLS (0.0-1.0) */

void mRGBtoHLS(int r,int g,int b,double *dst)
{
	double dr,dg,db,min,max,d,h,l,s;

	dr = r / 255.0;
	dg = g / 255.0;
	db = b / 255.0;

	//min,max

	max = (r >= g)? dr: dg;
	if(db > max) max = db;

	min = (r <= g)? dr: dg;
	if(db < min) min = db;

	//

	l = (max + min) * 0.5;

	if(max == min)
		h = s = 0;
	else
	{
		d = max - min;

		//S

		if(l <= 0.5)
			s = d / (max + min);
		else
			s = d / (2 - max - min);

		//H

		if(dr == max)
			h = (dg - db) / d;
		else if(dg == max)
			h = 2 + (db - dr) / d;
		else
			h = 4 + (dr - dg) / d;

		if(h < 0) h += 6.0;
		h /= 6.0;

		if(h >= 1.0) h -= 1.0;
	}

	dst[0] = h;
	dst[1] = l;
	dst[2] = s;
}


/* @} */
