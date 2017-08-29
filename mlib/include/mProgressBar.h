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

#ifndef MLIB_PROGRESSBAR_H
#define MLIB_PROGRESSBAR_H

#include "mWidgetDef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_PROGRESSBAR(p)  ((mProgressBar *)(p))

typedef struct
{
	uint32_t style;
	int min,max,pos,
		textlen,
		sub_step,sub_max,sub_toppos,
		sub_curcnt,sub_curstep,sub_nextcnt;
	char *text;
}mProgressBarData;

typedef struct _mProgressBar
{
	mWidget wg;
	mProgressBarData pb;
}mProgressBar;


enum MPROGRESSBAR_STYLE
{
	MPROGRESSBAR_S_FRAME = 1<<0,
	MPROGRESSBAR_S_TEXT  = 1<<1,
	MPROGRESSBAR_S_TEXT_PERS = 1<<2,
};


void mProgressBarDestroyHandle(mWidget *p);
void mProgressBarCalcHintHandle(mWidget *p);
void mProgressBarDrawHandle(mWidget *p,mPixbuf *pixbuf);

mProgressBar *mProgressBarNew(int size,mWidget *parent,uint32_t style);

void mProgressBarSetStatus(mProgressBar *p,int min,int max,int pos);
void mProgressBarSetText(mProgressBar *p,const char *text);
mBool mProgressBarSetPos(mProgressBar *p,int pos);
void mProgressBarIncPos(mProgressBar *p);
mBool mProgressBarAddPos(mProgressBar *p,int add);

void mProgressBarBeginSubStep(mProgressBar *p,int stepnum,int max);
void mProgressBarBeginSubStep_onestep(mProgressBar *p,int stepnum,int max);
mBool mProgressBarIncSubStep(mProgressBar *p);

#ifdef __cplusplus
}
#endif

#endif
