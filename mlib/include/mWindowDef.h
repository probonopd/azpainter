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

#ifndef MLIB_WINDOWDEF_H
#define MLIB_WINDOWDEF_H

#include "mContainerDef.h"

typedef struct _mWindowSysDat mWindowSysDat;
typedef struct _mMenuBar mMenuBar;
typedef struct _mAccelerator mAccelerator;

typedef struct
{
	mWindowSysDat *sys;

	mWindow *owner;
	mWidget *focus_widget;
	mPixbuf *pixbuf;
	mMenuBar *menubar;
	mAccelerator *accelerator;
	
	uint32_t fStyle;
	mRect rcUpdate;
}mWindowData;

struct _mWindow
{
	mWidget wg;
	mContainerData ct;
	mWindowData win;
};

enum MWINDOW_STYLE
{
	MWINDOW_S_POPUP      = 1<<0,
	MWINDOW_S_TITLE      = 1<<1,
	MWINDOW_S_BORDER     = 1<<2,
	MWINDOW_S_CLOSE      = 1<<3,
	MWINDOW_S_SYSMENU    = 1<<4,
	MWINDOW_S_MINIMIZE   = 1<<5,
	MWINDOW_S_MAXIMIZE   = 1<<6,
	MWINDOW_S_OWNER      = 1<<7,
	MWINDOW_S_TOOL       = 1<<8,
	MWINDOW_S_TABMOVE    = 1<<9,
	MWINDOW_S_NO_IM      = 1<<10,
	MWINDOW_S_NO_RESIZE  = 1<<11,
	MWINDOW_S_DIALOG     = 1<<12,
	MWINDOW_S_ENABLE_DND = 1<<13,
	
	MWINDOW_S_NORMAL = MWINDOW_S_TITLE | MWINDOW_S_BORDER
		| MWINDOW_S_CLOSE | MWINDOW_S_SYSMENU | MWINDOW_S_MINIMIZE
		| MWINDOW_S_MAXIMIZE,

	MWINDOW_S_DIALOG_NORMAL = MWINDOW_S_NORMAL | MWINDOW_S_DIALOG
		| MWINDOW_S_TABMOVE | MWINDOW_S_OWNER
};

#endif
