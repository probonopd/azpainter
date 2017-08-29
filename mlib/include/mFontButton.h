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

#ifndef MLIB_FONTBUTTON_H
#define MLIB_FONTBUTTON_H

#include "mButton.h"
#include "mFont.h"

#define M_FONTBUTTON(p)  ((mFontButton *)(p))

typedef struct
{
	mFontInfo info;
	uint32_t mask;
}mFontButtonData;

typedef struct _mFontButton
{
	mWidget wg;
	mButtonData btt;
	mFontButtonData fbt;
}mFontButton;


enum MFONTBUTTON_NOTIFY
{
	MFONTBUTTON_N_CHANGEFONT
};

/*--------*/

#ifdef __cplusplus
extern "C" {
#endif

void mFontButtonDestroyHandle(mWidget *p);

mFontButton *mFontButtonCreate(mWidget *parent,int id,uint32_t style,uint32_t fLayout,uint32_t marginB4);

mFontButton *mFontButtonNew(int size,mWidget *parent,uint32_t style);

void mFontButtonSetMask(mFontButton *p,uint32_t mask);
void mFontButtonSetInfoFormat(mFontButton *p,const char *text);

void mFontButtonGetInfoFormat_str(mFontButton *p,mStr *str);

#ifdef __cplusplus
}
#endif

#endif
