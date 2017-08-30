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

#ifndef MLIB_MENUWINDOW_H
#define MLIB_MENUWINDOW_H

#include "mWindowDef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_MENUWINDOW(p) ((mMenuWindow *)(p))

typedef struct _mMenuWindow mMenuWindow;
typedef struct _mMenuBar mMenuBar;

typedef struct
{
	mMenu *dat;

	mMenuItem *itemCur,
		*itemRet;
	mMenuWindow *winSub,
		*winTop,
		*winParent,
		*winFocus,
		*winScroll;
	mWidget *widgetNotify;
	mMenuBar *menubar;
	
	int labelLeftMaxW,
		maxH,
		scrY;
	uint32_t flags;
}mMenuWindowData;

struct _mMenuWindow
{
	mWidget wg;
	mContainerData ct;
	mWindowData win;
	mMenuWindowData menu;
};

#define MMENUWINDOW_F_HAVE_SCROLL    1
#define MMENUWINDOW_F_SCROLL_DOWN    2
#define MMENUWINDOW_F_MENUBAR_CHANGE 4


mMenuWindow *mMenuWindowNew(mMenu *menu);

mMenuItem *mMenuWindowShowPopup(mMenuWindow *win,
	mWidget *send,int rootx,int rooty,uint32_t flags);

mMenuItem *mMenuWindowShowMenuBar(mMenuWindow *win,
	mMenuBar *menubar,int rootx,int rooty,int itemid);

#ifdef __cplusplus
}
#endif

#endif
