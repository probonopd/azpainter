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

#ifndef MLIB_SPLITTER_H
#define MLIB_SPLITTER_H

#include "mWidgetDef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_SPLITTER(p)  ((mSplitter *)(p))

typedef struct _mSplitter mSplitter;

typedef struct
{
	mWidget *wgprev,*wgnext;
	int prev_min,prev_max,prev_cur,
		next_min,next_max,next_cur;
	intptr_t param;
}mSplitterTargetInfo;

typedef int (*mSplitterCallbackGetTarget)(mSplitter *,mSplitterTargetInfo *);


typedef struct
{
	uint32_t style;
	int presspos,dragdiff;
	uint8_t fdrag;
	mSplitterTargetInfo info;
	mSplitterCallbackGetTarget func_target;
}mSplitterData;

struct _mSplitter
{
	mWidget wg;
	mSplitterData spl;
};


enum MSPLITTER_STYLE
{
	MSPLITTER_S_HORZ = 0,
	MSPLITTER_S_VERT = 1<<0,
	MSPLITTER_S_NOTIFY_MOVED = 1<<1
};

enum MSPLITTER_NOTIFY
{
	MSPLITTER_N_MOVED
};


void mSplitterDrawHandle(mWidget *p,mPixbuf *pixbuf);
int mSplitterEventHandle(mWidget *wg,mEvent *ev);

mSplitter *mSplitterNew(int size,mWidget *parent,uint32_t style);
void mSplitterSetCallback_getTarget(mSplitter *p,mSplitterCallbackGetTarget func,intptr_t param);

#ifdef __cplusplus
}
#endif

#endif
