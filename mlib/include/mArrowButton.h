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

#ifndef MLIB_ARROWBUTTON_H
#define MLIB_ARROWBUTTON_H

#include "mButton.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_ARROWBUTTON(p)  ((mArrowButton *)(p))

typedef struct
{
	uint32_t style;
}mArrowButtonData;

typedef struct _mArrowButton
{
	mWidget wg;
	mButtonData btt;
	mArrowButtonData abtt;
}mArrowButton;

enum MARROWBUTTON_STYLE
{
	MARROWBUTTON_S_DOWN  = 0,
	MARROWBUTTON_S_UP    = 1<<0,
	MARROWBUTTON_S_LEFT  = 1<<1,
	MARROWBUTTON_S_RIGHT = 1<<2,
	MARROWBUTTON_S_FONTSIZE = 1<<3
};


void mArrowButtonDrawHandle(mWidget *p,mPixbuf *pixbuf);
void mArrowButtonCalcHintHandle(mWidget *p);

mArrowButton *mArrowButtonNew(int size,mWidget *parent,uint32_t style);
mArrowButton *mArrowButtonCreate(mWidget *parent,int id,uint32_t style,
	uint32_t fLayout,uint32_t marginB4);

#ifdef __cplusplus
}
#endif

#endif
