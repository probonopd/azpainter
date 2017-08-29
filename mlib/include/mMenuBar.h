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

#ifndef MLIB_MENUBAR_H
#define MLIB_MENUBAR_H

#include "mWidgetDef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_MENUBAR(p)  ((mMenuBar *)(p))

typedef struct _mMenu mMenu;
typedef struct _mMenuItem mMenuItem;

typedef struct
{
	mMenu *menu;
	mMenuItem *itemCur;
}mMenuBarData;

typedef struct _mMenuBar
{
	mWidget wg;
	mMenuBarData mb;
}mMenuBar;


enum MMENUBAR_ARRAY16
{
	MMENUBAR_ARRAY16_END = 0xffff,
	MMENUBAR_ARRAY16_SEP = 0xfffe,
	MMENUBAR_ARRAY16_SUBMENU = 0xfffd,
	MMENUBAR_ARRAY16_AUTOCHECK = 0x8000,
	MMENUBAR_ARRAY16_RADIO = 0x4000
};


mMenuBar *mMenuBarNew(int size,mWidget *parent,uint32_t style);

mMenu *mMenuBarGetMenu(mMenuBar *bar);
void mMenuBarSetMenu(mMenuBar *bar,mMenu *menu);
void mMenuBarSetItemSubmenu(mMenuBar *bar,int id,mMenu *submenu);

void mMenuBarCreateMenuTrArray16(mMenuBar *bar,const uint16_t *buf,int idtop);
void mMenuBarCreateMenuTrArray16_radio(mMenuBar *bar,const uint16_t *buf,int idtop);

#ifdef __cplusplus
}
#endif

#endif
