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

#ifndef MLIB_LINEEDIT_H
#define MLIB_LINEEDIT_H

#include "mWidgetDef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_LINEEDIT(p)  ((mLineEdit *)(p))

typedef struct _mEditTextBuf mEditTextBuf;

typedef struct
{
	mEditTextBuf *buf;

	uint32_t style;
	int curX,scrX,
		fpress,posLast,
		numMin,numMax,numDig;
	uint8_t padding;
}mLineEditData;

typedef struct _mLineEdit
{
	mWidget wg;
	mLineEditData le;
}mLineEdit;


enum MLINEEDIT_STYLE
{
	MLINEEDIT_S_NOTIFY_CHANGE = 1<<0,
	MLINEEDIT_S_NOTIFY_ENTER  = 1<<1,
	MLINEEDIT_S_READONLY = 1<<2,
	MLINEEDIT_S_SPIN     = 1<<3,
	MLINEEDIT_S_NO_FRAME = 1<<4
};

enum MLINEEDIT_NOTIFY
{
	MLINEEDIT_N_CHANGE,
	MLINEEDIT_N_ENTER
};


void mLineEditDestroyHandle(mWidget *p);
void mLineEditCalcHintHandle(mWidget *p);
void mLineEditDrawHandle(mWidget *p,mPixbuf *pixbuf);
int mLineEditEventHandle(mWidget *wg,mEvent *ev);

mLineEdit *mLineEditCreate(mWidget *parent,int id,uint32_t style,uint32_t fLayout,uint32_t marginB4);

mLineEdit *mLineEditNew(int size,mWidget *parent,uint32_t style);

void mLineEditSetNumStatus(mLineEdit *p,int min,int max,int dig);
void mLineEditSetWidthByLen(mLineEdit *p,int len);

void mLineEditSetText(mLineEdit *p,const char *text);
void mLineEditSetText_ucs4(mLineEdit *p,const uint32_t *text);
void mLineEditSetInt(mLineEdit *p,int n);
void mLineEditSetNum(mLineEdit *p,int num);
void mLineEditSetDouble(mLineEdit *p,double d,int dig);

mBool mLineEditIsEmpty(mLineEdit *p);
void mLineEditGetTextStr(mLineEdit *p,mStr *str);
int mLineEditGetInt(mLineEdit *p);
double mLineEditGetDouble(mLineEdit *p);
int mLineEditGetNum(mLineEdit *p);

void mLineEditSelectAll(mLineEdit *p);
void mLineEditCursorToRight(mLineEdit *p);

#ifdef __cplusplus
}
#endif

#endif
