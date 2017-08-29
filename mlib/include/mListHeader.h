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

#ifndef MLIB_LISTHEADER_H
#define MLIB_LISTHEADER_H

#include "mWidgetDef.h"
#include "mListDef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_LISTHEADER(p)      ((mListHeader *)(p))
#define M_LISTHEADER_ITEM(p) ((mListHeaderItem *)(p))

typedef struct _mListHeaderItem
{
	mListItem i;
	char *text;
	int width;
	uint32_t flags;
	intptr_t param;
}mListHeaderItem;


typedef struct
{
	mList list;
	uint32_t style;
	int sortup,scrx,fpress,dragLeft;
	mListHeaderItem *dragitem,*sortitem;
}mListHeaderData;

typedef struct _mListHeader
{
	mWidget wg;
	mListHeaderData lh;
}mListHeader;


enum MLISTHEADER_STYLE
{
	MLISTHEADER_STYLE_SORT = 1<<0
};

enum MLISTHEADER_NOTIFY
{
	MLISTHEADER_N_CHANGE_WIDTH,
	MLISTHEADER_N_CHANGE_SORT
};

enum MLISTHEADER_ITEM_FLAGS
{
	MLISTHEADER_ITEM_F_RIGHT  = 1<<0,
	MLISTHEADER_ITEM_F_FIX    = 1<<1,
	MLISTHEADER_ITEM_F_EXPAND = 1<<2
};


void mListHeaderDestroyHandle(mWidget *p);
void mListHeaderCalcHintHandle(mWidget *p);
int mListHeaderEventHandle(mWidget *wg,mEvent *ev);

mListHeader *mListHeaderNew(int size,mWidget *parent,uint32_t style);

mListHeaderItem *mListHeaderGetTopItem(mListHeader *p);
void mListHeaderAddItem(mListHeader *p,const char *text,int width,uint32_t flags,intptr_t param);
int mListHeaderGetFullWidth(mListHeader *p);
void mListHeaderSetScrollPos(mListHeader *p,int scrx);
void mListHeaderSetSortParam(mListHeader *p,int index,mBool up);

#ifdef __cplusplus
}
#endif

#endif
