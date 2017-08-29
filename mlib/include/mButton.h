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

#ifndef MLIB_BUTTON_H
#define MLIB_BUTTON_H

#include "mWidgetDef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_BUTTON(p)  ((mButton *)(p))

typedef struct _mButton mButton;

typedef struct
{
	char *text;
	int textW;
	uint32_t style,flags,press_key;
	void (*onPressed)(mButton *);
}mButtonData;

struct _mButton
{
	mWidget wg;
	mButtonData btt;
};

enum MBUTTON_STYLE
{
	MBUTTON_S_REAL_W = 1<<0,
	MBUTTON_S_REAL_H = 1<<1,
	
	MBUTTON_S_REAL_WH = MBUTTON_S_REAL_W | MBUTTON_S_REAL_H
};

enum MBUTTON_NOTIFY
{
	MBUTTON_N_PRESS
};

/*--------*/

void mButtonDestroyHandle(mWidget *p);
void mButtonCalcHintHandle(mWidget *p);
void mButtonDrawHandle(mWidget *p,mPixbuf *pixbuf);
int mButtonEventHandle(mWidget *wg,mEvent *ev);

mButton *mButtonCreate(mWidget *parent,int id,uint32_t style,uint32_t fLayout,uint32_t marginB4,const char *text);

mButton *mButtonNew(int size,mWidget *parent,uint32_t style);
void mButtonSetText(mButton *p,const char *text);
void mButtonSetPress(mButton *p,mBool press);
mBool mButtonIsPressed(mButton *p);

void mButtonDrawBase(mButton *p,mPixbuf *pixbuf,mBool pressed);

#ifdef __cplusplus
}
#endif

#endif
