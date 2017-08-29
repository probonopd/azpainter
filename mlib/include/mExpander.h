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

#ifndef MLIB_EXPANDER_H
#define MLIB_EXPANDER_H

#include "mContainerDef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_EXPANDER(p)  ((mExpander *)(p))

typedef struct
{
	uint32_t style;
	char *text;
	mRect padding;
	int headerH;
	mBool expand;
}mExpanderData;

typedef struct _mExpander
{
	mWidget wg;
	mContainerData ct;
	mExpanderData exp;
}mExpander;


enum MEXPANDER_STYLE
{
	MEXPANDER_S_BORDER_TOP  = 1<<0,
	MEXPANDER_S_HEADER_DARK = 1<<1
};

enum MEXPANDER_NOTIFY
{
	MEXPANDER_N_TOGGLE
};


void mExpanderDestroyHandle(mWidget *p);
void mExpanderCalcHintHandle(mWidget *p);
int mExpanderEventHandle(mWidget *wg,mEvent *ev);
void mExpanderDrawHandle(mWidget *wg,mPixbuf *pixbuf);

mExpander *mExpanderNew(int size,mWidget *parent,uint32_t style);
mExpander *mExpanderCreate(mWidget *parent,int id,uint32_t style,
	uint32_t fLayout,uint32_t marginB4,const char *text);

void mExpanderSetText(mExpander *p,const char *text);
void mExpanderSetPadding_b4(mExpander *p,uint32_t val);
void mExpanderSetExpand(mExpander *p,int type);

#ifdef __cplusplus
}
#endif

#endif
