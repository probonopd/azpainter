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

#ifndef MLIB_LISTVIEWITEM_H
#define MLIB_LISTVIEWITEM_H

#include "mListDef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_LISTVIEWITEM(p)  ((mListViewItem *)(p))

typedef struct _mListViewItem mListViewItem;

typedef struct _mListViewItemDraw
{
	mWidget *widget;
	mBox box;
	uint8_t flags;
}mListViewItemDraw;

struct _mListViewItem
{
	mListItem i;

	const char *text;
	int textlen,icon;
	uint32_t flags;
	intptr_t param;
	void (*draw)(mPixbuf *,mListViewItem *,mListViewItemDraw *);
};

enum MLISTVIEW_ITEM_FLAGS
{
	MLISTVIEW_ITEM_F_SELECTED = 1<<0,
	MLISTVIEW_ITEM_F_CHECKED  = 1<<1,
	MLISTVIEW_ITEM_F_HEADER   = 1<<2,
	MLISTVIEW_ITEM_F_STATIC_TEXT = 1<<3
};

enum MLISTVIEWITEMDRAW_FLAGS
{
	MLISTVIEWITEMDRAW_F_SELECTED = 1<<0,
	MLISTVIEWITEMDRAW_F_ENABLED  = 1<<1,
	MLISTVIEWITEMDRAW_F_FOCUSED  = 1<<2,
	MLISTVIEWITEMDRAW_F_FOCUS_ITEM = 1<<3
};

void mListViewItemDestroy(mListItem *p);

#ifdef __cplusplus
}
#endif

#endif
