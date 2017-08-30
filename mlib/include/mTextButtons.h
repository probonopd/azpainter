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

#ifndef MLIB_TEXTBUTTONS_H
#define MLIB_TEXTBUTTONS_H

#include "mWidgetDef.h"
#include "mListDef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_TEXTBUTTONS(p)  ((mTextButtons *)(p))

typedef struct
{
	mList list;
	int focusno,
		flags,presskey;
}mTextButtonsData;

typedef struct _mTextButtons
{
	mWidget wg;
	mTextButtonsData tb;
}mTextButtons;


enum MTEXTBUTTONS_NOTIFY
{
	MTEXTBUTTONS_N_PRESS
};


void mTextButtonsDestroyHandle(mWidget *p);
void mTextButtonsCalcHintHandle(mWidget *p);
void mTextButtonsDrawHandle(mWidget *p,mPixbuf *pixbuf);
int mTextButtonsEventHandle(mWidget *wg,mEvent *ev);

mTextButtons *mTextButtonsNew(int size,mWidget *parent,uint32_t style);

void mTextButtonsDeleteAll(mTextButtons *p);
void mTextButtonsAddButton(mTextButtons *p,const char *text);
void mTextButtonsAddButtonsTr(mTextButtons *p,int idtop,int num);

#ifdef __cplusplus
}
#endif

#endif
