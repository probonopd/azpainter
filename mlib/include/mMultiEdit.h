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

#ifndef MLIB_MULTIEDIT_H
#define MLIB_MULTIEDIT_H

#include "mScrollView.h"

#define M_MULTIEDIT(p)  ((mMultiEdit *)(p))

typedef struct _mEditTextBuf mEditTextBuf;
typedef struct _mStr mStr;


typedef struct
{
	mEditTextBuf *buf;
	uint32_t style;
}mMultiEditData;

typedef struct _mMultiEdit
{
	mWidget wg;
	mScrollViewData sv;
	mMultiEditData me;
}mMultiEdit;


enum MMULTIEDIT_STYLE
{
	MMULTIEDIT_S_READONLY = 1<<0,
	MMULTIEDIT_S_NOTIFY_CHANGE = 1<<1
};

enum MMULTIEDIT_NOTIFY
{
	MMULTIEDIT_N_CHANGE
};


#ifdef __cplusplus
extern "C" {
#endif

void mMultiEditDestroyHandle(mWidget *p);
int mMultiEditEventHandle(mWidget *wg,mEvent *ev);

mMultiEdit *mMultiEditNew(int size,mWidget *parent,uint32_t style);
mMultiEdit *mMultiEditCreate(mWidget *parent,int id,uint32_t style,uint32_t fLayout,uint32_t marginB4);

void mMultiEditSetText(mMultiEdit *p,const char *text);
void mMultiEditGetTextStr(mMultiEdit *p,mStr *str);
int mMultiEditGetTextLen(mMultiEdit *p);
void mMultiEditSelectAll(mMultiEdit *p);

#ifdef __cplusplus
}
#endif

#endif
