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

#ifndef MLIB_TAB_H
#define MLIB_TAB_H

#include "mWidgetDef.h"
#include "mListDef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_TAB(p)     ((mTab *)(p))
#define M_TABITEM(p) ((mTabItem *)(p))

typedef struct
{
	mListItem i;
	char *label;
	intptr_t param;
	int pos,w;
}mTabItem;

typedef struct
{
	uint32_t style;
	mList list;

	mTabItem *focusitem;
}mTabData;

typedef struct _mTab
{
	mWidget wg;
	mTabData tab;
}mTab;


enum MTAB_STYLE
{
	MTAB_S_TOP    = 1<<0,
	MTAB_S_BOTTOM = 1<<1,
	MTAB_S_LEFT   = 1<<2,
	MTAB_S_RIGHT  = 1<<3,
	MTAB_S_HAVE_SEP = 1<<4,
	MTAB_S_FIT_SIZE = 1<<5
};

enum MTAB_NOTIFY
{
	MTAB_N_CHANGESEL
};


void mTabDestroyHandle(mWidget *p);
void mTabCalcHintHandle(mWidget *p);
void mTabDrawHandle(mWidget *p,mPixbuf *pixbuf);
int mTabEventHandle(mWidget *wg,mEvent *ev);
void mTabOnsizeHandle(mWidget *wg);

mTab *mTabCreate(mWidget *parent,int id,uint32_t style,uint32_t fLayout,uint32_t marginB4);

mTab *mTabNew(int size,mWidget *parent,uint32_t style);

void mTabAddItem(mTab *p,const char *label,int w,intptr_t param);
void mTabAddItemText(mTab *p,const char *label);
void mTabDelItem_index(mTab *p,int index);

int mTabGetSelItem_index(mTab *p);
mTabItem *mTabGetItemByIndex(mTab *p,int index);
intptr_t mTabGetItemParam_index(mTab *p,int index);

void mTabSetSel_index(mTab *p,int index);
void mTabSetHintSize_byItems(mTab *p);

#ifdef __cplusplus
}
#endif

#endif
