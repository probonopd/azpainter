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

#ifndef MLIB_SCROLLVIEW_H
#define MLIB_SCROLLVIEW_H

#include "mWidgetDef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_SCROLLVIEW(p)  ((mScrollView *)(p))

typedef struct _mScrollBar mScrollBar;
typedef struct _mScrollViewArea mScrollViewArea;

typedef struct
{
	uint32_t style;
	mScrollBar *scrh,*scrv;
	mScrollViewArea *area;
}mScrollViewData;

typedef struct _mScrollView
{
	mWidget wg;
	mScrollViewData sv;
}mScrollView;

enum MSCROLLVIEW_STYLE
{
	MSCROLLVIEW_S_HORZ     = 1<<0,
	MSCROLLVIEW_S_VERT     = 1<<1,
	MSCROLLVIEW_S_FIX_HORZ = 1<<2,
	MSCROLLVIEW_S_FIX_VERT = 1<<3,
	MSCROLLVIEW_S_FRAME    = 1<<4,
	MSCROLLVIEW_S_SCROLL_NOTIFY_SELF = 1<<5,
	
	MSCROLLVIEW_S_HORZVERT = MSCROLLVIEW_S_HORZ | MSCROLLVIEW_S_VERT,
	MSCROLLVIEW_S_FIX_HORZVERT = MSCROLLVIEW_S_HORZVERT | MSCROLLVIEW_S_FIX_HORZ | MSCROLLVIEW_S_FIX_VERT,
	MSCROLLVIEW_S_HORZVERT_FRAME = MSCROLLVIEW_S_HORZVERT | MSCROLLVIEW_S_FRAME
};


mScrollView *mScrollViewNew(int size,mWidget *parent,uint32_t style);
void mScrollViewConstruct(mScrollView *p);

#ifdef __cplusplus
}
#endif

#endif
