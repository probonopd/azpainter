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

#ifndef MLIB_SCROLLBAR_H
#define MLIB_SCROLLBAR_H

#include "mWidgetDef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_SCROLLBAR(p)  ((mScrollBar *)(p))

typedef struct _mScrollBar mScrollBar;

typedef struct
{
	uint32_t style;
	int min,max,page,pos,
		range,fpress,dragDiff;
	void (*handle)(mScrollBar *,int,int);
}mScrollBarData;

struct _mScrollBar
{
	mWidget wg;
	mScrollBarData sb;
};


enum MSCROLLBAR_STYLE
{
	MSCROLLBAR_S_HORZ = 0,
	MSCROLLBAR_S_VERT = 1<<0
};

enum MSCROLLBAR_NOTIFY
{
	MSCROLLBAR_N_HANDLE
};

enum MSCROLLBAR_NOTIFY_HANDLE_FLAGS
{
	MSCROLLBAR_N_HANDLE_F_CHANGE  = 1<<0,
	MSCROLLBAR_N_HANDLE_F_PRESS   = 1<<1,
	MSCROLLBAR_N_HANDLE_F_MOTION  = 1<<2,
	MSCROLLBAR_N_HANDLE_F_RELEASE = 1<<3,
	MSCROLLBAR_N_HANDLE_F_PAGE    = 1<<4
};


#define MSCROLLBAR_WIDTH  15


void mScrollBarDrawHandle(mWidget *p,mPixbuf *pixbuf);
int mScrollBarEventHandle(mWidget *wg,mEvent *ev);

mScrollBar *mScrollBarNew(int size,mWidget *parent,uint32_t style);

mBool mScrollBarIsTopPos(mScrollBar *p);
mBool mScrollBarIsBottomPos(mScrollBar *p);
int mScrollBarGetPos(mScrollBar *p);
void mScrollBarSetStatus(mScrollBar *p,int min,int max,int page);
void mScrollBarSetPage(mScrollBar *p,int page);
mBool mScrollBarSetPos(mScrollBar *p,int pos);
mBool mScrollBarSetPosToEnd(mScrollBar *p);
mBool mScrollBarMovePos(mScrollBar *p,int dir);

#ifdef __cplusplus
}
#endif

#endif
