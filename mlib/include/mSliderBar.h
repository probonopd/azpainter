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

#ifndef MLIB_SLIDERBAR_H
#define MLIB_SLIDERBAR_H

#include "mWidgetDef.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _mSliderBar mSliderBar;

#define M_SLIDERBAR(p)  ((mSliderBar *)(p))

typedef struct
{
	uint32_t style;
	int min,max,pos;
	uint8_t barw_hf,fpress;
	void (*handle)(mSliderBar *,int,int);
}mSliderBarData;

struct _mSliderBar
{
	mWidget wg;
	mSliderBarData sl;
};


enum MSLIDERBAR_STYLE
{
	MSLIDERBAR_S_VERT  = 1<<0,
	MSLIDERBAR_S_SMALL = 1<<1
};

enum MSLIDERBAR_NOTIFY
{
	MSLIDERBAR_N_HANDLE
};

enum MSLIDERBAR_HANDLE_FLAGS
{
	MSLIDERBAR_HANDLE_F_CHANGE  = 1<<0,
	MSLIDERBAR_HANDLE_F_KEY     = 1<<1,
	MSLIDERBAR_HANDLE_F_PRESS   = 1<<2,
	MSLIDERBAR_HANDLE_F_MOTION  = 1<<3,
	MSLIDERBAR_HANDLE_F_RELEASE = 1<<4
};


void mSliderBarDrawHandle(mWidget *p,mPixbuf *pixbuf);
int mSliderBarEventHandle(mWidget *wg,mEvent *ev);

mSliderBar *mSliderBarCreate(mWidget *parent,int id,uint32_t style,uint32_t fLayout,uint32_t marginB4);

mSliderBar *mSliderBarNew(int size,mWidget *parent,uint32_t style);

int mSliderBarGetPos(mSliderBar *p);
void mSliderBarSetStatus(mSliderBar *p,int min,int max,int pos);
void mSliderBarSetRange(mSliderBar *p,int min,int max);
mBool mSliderBarSetPos(mSliderBar *p,int pos);

#ifdef __cplusplus
}
#endif

#endif
