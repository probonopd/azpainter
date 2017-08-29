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

#ifndef MLIB_LISTVIEW_H
#define MLIB_LISTVIEW_H

#include "mScrollView.h"
#include "mListViewItem.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_LISTVIEW(p)  ((mListView *)(p))

typedef struct _mLVItemMan mLVItemMan;
typedef struct _mListHeader mListHeader;

typedef struct
{
	mLVItemMan *manager;
	mImageList *iconimg;
	uint32_t style;
	int itemHeight,
		itemH,
		width_single;
}mListViewData;

typedef struct _mListView
{
	mWidget wg;
	mScrollViewData sv;
	mListViewData lv;
}mListView;


enum MLISTVIEW_STYLE
{
	MLISTVIEW_S_MULTI_COLUMN = 1<<0,
	MLISTVIEW_S_NO_HEADER = 1<<1,
	MLISTVIEW_S_CHECKBOX  = 1<<2,
	MLISTVIEW_S_GRID_ROW  = 1<<3,
	MLISTVIEW_S_GRID_COL  = 1<<4,
	MLISTVIEW_S_MULTI_SEL = 1<<5,
	MLISTVIEW_S_AUTO_WIDTH = 1<<6,
	MLISTVIEW_S_DESTROY_IMAGELIST = 1<<7,
	MLISTVIEW_S_HEADER_SORT = 1<<8
};

enum MLISTVIEW_NOTIFY
{
	MLISTVIEW_N_CHANGE_FOCUS,
	MLISTVIEW_N_CLICK_ON_FOCUS,
	MLISTVIEW_N_ITEM_CHECK,
	MLISTVIEW_N_ITEM_DBLCLK,
	MLISTVIEW_N_ITEM_RCLK,
	MLISTVIEW_N_CHANGE_SORT
};


/*-----------*/

void mListViewDestroyHandle(mWidget *p);
void mListViewCalcHintHandle(mWidget *p);
int mListViewEventHandle(mWidget *wg,mEvent *ev);

mListView *mListViewNew(int size,mWidget *parent,uint32_t style,uint32_t scrv_style);

void mListViewSetImageList(mListView *p,mImageList *img);
mListHeader *mListViewGetHeader(mListView *p);
int mListViewGetItemNormalHeight(mListView *p);
int mListViewGetColumnMarginWidth();

mListViewItem *mListViewGetItemByIndex(mListView *p,int index);
mListViewItem *mListViewGetFocusItem(mListView *p);
mListViewItem *mListViewGetTopItem(mListView *p);
int mListViewGetItemNum(mListView *p);

void mListViewDeleteAllItem(mListView *p);
void mListViewDeleteItem(mListView *p,mListViewItem *item);
mListViewItem *mListViewDeleteItem_sel(mListView *p,mListViewItem *item);

mListViewItem *mListViewAddItem(mListView *p,const char *text,int icon,uint32_t flags,intptr_t param);
mListViewItem *mListViewAddItem_ex(mListView *p,int size,const char *text,int icon,uint32_t flags,intptr_t param);
mListViewItem *mListViewAddItemText(mListView *p,const char *text);
mListViewItem *mListViewAddItem_textparam(mListView *p,const char *text,intptr_t param);
mListViewItem *mListViewInsertItem(mListView *p,mListViewItem *ins,const char *text,int icon,uint32_t flags,intptr_t param);

int mListViewGetItemIndex(mListView *p,mListViewItem *pi);
intptr_t mListViewGetItemParam(mListView *p,int index);
void mListViewGetItemColumnText(mListView *p,mListViewItem *pi,int index,mStr *str);

mListViewItem *mListViewFindItemByParam(mListView *p,intptr_t param);
mListViewItem *mListViewFindItemByText(mListView *p,const char *text);

void mListViewSetFocusItem(mListView *p,mListViewItem *pi);
mListViewItem *mListViewSetFocusItem_index(mListView *p,int index);
mListViewItem *mListViewSetFocusItem_findParam(mListView *p,intptr_t param);
mListViewItem *mListViewSetFocusItem_findText(mListView *p,const char *text);
void mListViewSetItemText(mListView *p,mListViewItem *pi,const char *text);

mBool mListViewMoveItem_updown(mListView *p,mListViewItem *item,mBool down);
mBool mListViewMoveItem(mListView *p,mListViewItem *item,mListViewItem *insert);

void mListViewSortItem(mListView *p,int (*comp)(mListItem *,mListItem *,intptr_t),intptr_t param);
void mListViewSetWidthAuto(mListView *p,mBool bHint);
void mListViewScrollToItem(mListView *p,mListViewItem *pi,int align);
void mListViewAdjustScrollByFocusItem(mListView *p,int margin_num);

#ifdef __cplusplus
}
#endif

#endif
