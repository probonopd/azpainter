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

#ifndef MLIB_LIST_H
#define MLIB_LIST_H

#include "mListDef.h"

#ifdef __cplusplus
extern "C" {
#endif

mListItem *mListAppendNew(mList *list,int size,void (*destroy)(mListItem *));
mListItem *mListInsertNew(mList *list,mListItem *pos,int size,void (*destroy)(mListItem *));

void mListAppend(mList *list,mListItem *item);
void mListInsert(mList *list,mListItem *pos,mListItem *item);
void mListRemove(mList *list,mListItem *item);

mBool mListDup(mList *dst,mList *src,int itemsize);

void mListDeleteAll(mList *list);
void mListDelete(mList *list,mListItem *item);
void mListDeleteNoDestroy(mList *list,mListItem *item);
mBool mListDeleteByIndex(mList *list,int index);
void mListDeleteTopNum(mList *list,int num);
void mListDeleteBottomNum(mList *list,int num);

mBool mListMove(mList *list,mListItem *src,mListItem *dst);
void mListMoveTop(mList *list,mListItem *item);
mBool mListMoveUpDown(mList *list,mListItem *item,mBool up);
void mListSwap(mList *list,mListItem *item1,mListItem *item2);
void mListSort(mList *list,int (*comp)(mListItem *,mListItem *,intptr_t),intptr_t param);

int mListGetDir(mList *list,mListItem *item1,mListItem *item2);
mListItem *mListGetItemByIndex(mList *list,int index);
int mListGetItemIndex(mList *list,mListItem *item);

mListItem *mListItemAlloc(int size,void (*destroy)(mListItem *));
void mListLinkAppend(mList *list,mListItem *item);
void mListLinkInsert(mList *list,mListItem *item,mListItem *pos);
void mListLinkRemove(mList *list,mListItem *item);

#ifdef __cplusplus
}
#endif

#endif
