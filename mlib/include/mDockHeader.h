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

#ifndef MLIB_DOCKHEADER_H
#define MLIB_DOCKHEADER_H

#include "mWidgetDef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_DOCKHEADER(p)  ((mDockHeader *)(p))

typedef struct
{
	uint32_t style;
	int fpress;
	const char *title;
}mDockHeaderData;

typedef struct _mDockHeader
{
	mWidget wg;
	mDockHeaderData dh;
}mDockHeader;

enum
{
	MDOCKHEADER_S_HAVE_CLOSE  = 1<<0,
	MDOCKHEADER_S_HAVE_SWITCH = 1<<1,
	MDOCKHEADER_S_HAVE_FOLD   = 1<<2,
	MDOCKHEADER_S_FOLDED      = 1<<3,
	MDOCKHEADER_S_SWITCH_DOWN = 1<<4
};

#define MDOCKHEADER_N_BUTTON  0

enum
{
	MDOCKHEADER_BTT_CLOSE = 1,
	MDOCKHEADER_BTT_SWITCH,
	MDOCKHEADER_BTT_FOLD
};


void mDockHeaderDrawHandle(mWidget *p,mPixbuf *pixbuf);
int mDockHeaderEventHandle(mWidget *wg,mEvent *ev);
void mDockHeaderCalcHintHandle(mWidget *wg);

mDockHeader *mDockHeaderNew(int size,mWidget *parent,uint32_t style);

void mDockHeaderSetTitle(mDockHeader *p,const char *title);
void mDockHeaderSetFolded(mDockHeader *p,int type);

#ifdef __cplusplus
}
#endif

#endif
