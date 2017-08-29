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

#ifndef MLIB_MENU_H
#define MLIB_MENU_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _mMenu mMenu;
typedef struct _mMenuItemInfo mMenuItemInfo;
typedef struct _mMenuItem mMenuItem;

typedef struct _mMenuItemDraw
{
	mBox box;
	uint32_t flags;
}mMenuItemDraw;

struct _mMenuItemInfo
{
	int id;
	short width,height;
	uint32_t flags,shortcutkey;
	intptr_t param1,param2;
	const char *label;
	mMenu *submenu;
	void (*draw)(mPixbuf *,mMenuItemInfo *,mMenuItemDraw *);
};

/*---------*/

enum MMENUITEM_FLAGS
{
	MMENUITEM_F_LABEL_COPY = 1<<0,
	MMENUITEM_F_SEP        = 1<<1,
	MMENUITEM_F_DISABLE    = 1<<2,
	MMENUITEM_F_RADIO      = 1<<3,
	MMENUITEM_F_CHECKED    = 1<<4,
	MMENUITEM_F_AUTOCHECK  = 1<<5
};

enum MMENUITEM_DRAW_FLAGS
{
	MMENUITEM_DRAW_F_SELECTED = 1
};

enum MMENU_POPUP_FLAGS
{
	MMENU_POPUP_F_RIGHT  = 1<<0,
	MMENU_POPUP_F_NOCOMMAND = 1<<1
};

/*---------*/

mMenu *mMenuNew(void);
void mMenuDestroy(mMenu *p);

int mMenuGetNum(mMenu *menu);
mMenu *mMenuGetSubMenu(mMenu *menu,int id);
mMenuItemInfo *mMenuGetLastItemInfo(mMenu *menu);
mMenuItemInfo *mMenuGetItemByIndex(mMenu *menu,int index);

mMenuItemInfo *mMenuGetInfoInItem(mMenuItem *item);
mMenuItem *mMenuGetTopItem(mMenu *menu);
mMenuItem *mMenuGetNextItem(mMenuItem *item);

mMenuItemInfo *mMenuPopup(mMenu *menu,mWidget *notify,int x,int y,uint32_t flags);

void mMenuDeleteAll(mMenu *menu);
void mMenuDeleteByID(mMenu *menu,int id);
void mMenuDeleteByIndex(mMenu *menu,int index);

mMenuItemInfo *mMenuAdd(mMenu *menu,mMenuItemInfo *info);
mMenuItemInfo *mMenuAddNormal(mMenu *menu,int id,const char *label,uint32_t sckey,uint32_t flags);
mMenuItemInfo *mMenuAddText_static(mMenu *menu,int id,const char *label);
mMenuItemInfo *mMenuAddText_copy(mMenu *menu,int id,const char *label);
void mMenuAddSubmenu(mMenu *menu,int id,const char *label,mMenu *submenu);
void mMenuAddSep(mMenu *menu);
void mMenuAddCheck(mMenu *menu,int id,const char *label,mBool checked);
void mMenuAddRadio(mMenu *menu,int id,const char *label);

void mMenuAddTrTexts(mMenu *menu,int tridtop,int num);
void mMenuAddTrArray16(mMenu *menu,const uint16_t *buf);
void mMenuSetStrArray(mMenu *menu,int idtop,mStr *str,int num);
void mMenuAddStrArray(mMenu *menu,int idtop,mStr *str,int num);

void mMenuSetEnable(mMenu *menu,int id,int type);
void mMenuSetCheck(mMenu *menu,int id,int type);
void mMenuSetSubmenu(mMenu *menu,int id,mMenu *submenu);
void mMenuSetShortcutKey(mMenu *menu,int id,uint32_t sckey);

const char *mMenuGetTextByIndex(mMenu *menu,int index);

#ifdef __cplusplus
}
#endif

#endif
