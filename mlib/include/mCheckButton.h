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

#ifndef MLIB_CHECKBUTTON_H
#define MLIB_CHECKBUTTON_H

#include "mWidgetDef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_CHECKBUTTON(p)  ((mCheckButton *)(p))

typedef struct _mWidgetLabelTextRowInfo mWidgetLabelTextRowInfo;

typedef struct
{
	char *text;
	mWidgetLabelTextRowInfo *rowbuf;
	uint32_t style,flags;
	mSize sztext;
}mCheckButtonData;

typedef struct _mCheckButton
{
	mWidget wg;
	mCheckButtonData ck;
}mCheckButton;

enum MCHECKBUTTON_STYLE
{
	MCHECKBUTTON_S_RADIO = 1<<0,
	MCHECKBUTTON_S_RADIO_GROUP = 1<<1
};

enum MCHECKBUTTON_NOTIFY
{
	MCHECKBUTTON_N_PRESS
};


void mCheckButtonDestroyHandle(mWidget *p);
void mCheckButtonCalcHintHandle(mWidget *p);
void mCheckButtonDrawHandle(mWidget *p,mPixbuf *pixbuf);
int mCheckButtonEventHandle(mWidget *wg,mEvent *ev);

mCheckButton *mCheckButtonCreate(mWidget *parent,int id,
	uint32_t style,uint32_t fLayout,uint32_t marginB4,const char *text,int checked);

mCheckButton *mCheckButtonNew(int size,mWidget *parent,uint32_t style);
void mCheckButtonSetText(mCheckButton *p,const char *text);
void mCheckButtonSetState(mCheckButton *p,int type);
void mCheckButtonSetRadioSel(mCheckButton *p,int no);
mBool mCheckButtonIsChecked(mCheckButton *p);
mCheckButton *mCheckButtonGetRadioInfo(mCheckButton *start,mCheckButton **ppTop,mCheckButton **ppEnd);
int mCheckButtonGetGroupSelIndex(mCheckButton *p);

#ifdef __cplusplus
}
#endif

#endif
