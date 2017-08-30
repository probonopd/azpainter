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

#ifndef MLIB_MENU_PV_H
#define MLIB_MENU_PV_H

#include "mListDef.h"

#define M_MENUITEM(p)  ((mMenuItem *)(p))

struct _mMenu
{
	mList list;
};

typedef struct _mMenuItem
{
	mListItem i;
	mMenuItemInfo item;

	char *labelcopy,*sctext;
	const char *labelsrc;
	
	short labelLeftW,
		labelRightW;
	char hotkey;
	uint8_t fTmp;
}mMenuItem;

#define MMENUITEM_TMPF_INIT_SIZE 1

#define MMENU_INITTEXT_LABEL    1
#define MMENU_INITTEXT_SHORTCUT 2
#define MMENU_INITTEXT_ALL      3

/*---------*/

#ifdef __cplusplus
extern "C" {
#endif

mBool __mMenuItemIsDisableItem(mMenuItem *p);
mBool __mMenuItemIsEnableSubmenu(mMenuItem *p);
void __mMenuItemCheckRadio(mMenuItem *p,int check);

void __mMenuInitText(mMenuItem *p,int flags);
void __mMenuInit(mMenu *p,mFont *font);
mMenuItem *__mMenuFindByHotkey(mMenu *menu,char key);

#ifdef __cplusplus
}
#endif

#endif
