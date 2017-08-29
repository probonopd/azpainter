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

#ifndef MLIB_HSVPICKER_H
#define MLIB_HSVPICKER_H

#include "mWidgetDef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_HSVPICKER(p)  ((mHSVPicker *)(p))

typedef struct
{
	mPixbuf *img;
	mRgbCol col;
	int curX,curY,curH,
		fbtt;
}mHSVPickerData;

typedef struct _mHSVPicker
{
	mWidget wg;
	mHSVPickerData hsv;
}mHSVPicker;


enum MHSVPICKER_NOTIFY
{
	MHSVPICKER_N_CHANGE_H,
	MHSVPICKER_N_CHANGE_SV
};


void mHSVPickerDestroyHandle(mWidget *p);
void mHSVPickerDrawHandle(mWidget *p,mPixbuf *pixbuf);
int mHSVPickerEventHandle(mWidget *wg,mEvent *ev);

mHSVPicker *mHSVPickerCreate(mWidget *parent,int id,int height,uint32_t marginB4);

mHSVPicker *mHSVPickerNew(int size,mWidget *parent,uint32_t style,int height);

mRgbCol mHSVPickerGetRGBColor(mHSVPicker *p);
void mHSVPickerGetHSVColor(mHSVPicker *p,double *dst);

void mHSVPickerSetHue(mHSVPicker *p,int hue);
void mHSVPickerSetSV(mHSVPicker *p,double s,double v);
void mHSVPickerSetHSVColor(mHSVPicker *p,double h,double s,double v);
void mHSVPickerSetRGBColor(mHSVPicker *p,mRgbCol col);

void mHSVPickerRedraw(mHSVPicker *p);

#ifdef __cplusplus
}
#endif

#endif
