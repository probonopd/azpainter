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

#ifndef MLIB_LISTVIEWITEMMANAGER_H
#define MLIB_LISTVIEWITEMMANAGER_H

#include "mListViewItem.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _mLVItemMan
{
	mList list;
	mListViewItem *itemFocus;
	mBool bMultiSel;
}mLVItemMan;


void mLVItemMan_free(mLVItemMan *p);
mLVItemMan *mLVItemMan_new();

mListViewItem *mLVItemMan_addItem(mLVItemMan *p,int size,
	const char *text,int icon,uint32_t flags,intptr_t param);
mListViewItem *mLVItemMan_insertItem(mLVItemMan *p,mListViewItem *ins,
	const char *text,int icon,uint32_t flags,intptr_t param);

void mLVItemMan_deleteAllItem(mLVItemMan *p);
mBool mLVItemMan_deleteItem(mLVItemMan *p,mListViewItem *item);
mBool mLVItemMan_deleteItemByIndex(mLVItemMan *p,int index);
void mLVItemMan_selectAll(mLVItemMan *p);
void mLVItemMan_unselectAll(mLVItemMan *p);

mListViewItem *mLVItemMan_getItemByIndex(mLVItemMan *p,int index);
int mLVItemMan_getItemIndex(mLVItemMan *p,mListViewItem *item);

mListViewItem *mLVItemMan_findItemParam(mLVItemMan *p,intptr_t param);
mListViewItem *mLVItemMan_findItemText(mLVItemMan *p,const char *text);
mBool mLVItemMan_moveItem_updown(mLVItemMan *p,mListViewItem *item,mBool down);

void mLVItemMan_setText(mListViewItem *pi,const char *text);

mBool mLVItemMan_setFocusItem(mLVItemMan *p,mListViewItem *item);
mBool mLVItemMan_setFocusItemByIndex(mLVItemMan *p,int index);
mBool mLVItemMan_setFocusHomeEnd(mLVItemMan *p,mBool home);

mBool mLVItemMan_updownFocus(mLVItemMan *p,mBool down);
mBool mLVItemMan_select(mLVItemMan *p,uint32_t state,mListViewItem *item);

#ifdef __cplusplus
}
#endif

#endif
