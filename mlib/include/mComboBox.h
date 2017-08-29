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

#ifndef MLIB_COMBOBOX_H
#define MLIB_COMBOBOX_H

#include "mWidgetDef.h"
#include "mListViewItem.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_COMBOBOX(p)  ((mComboBox *)(p))

typedef struct _mLVItemMan mLVItemMan;

typedef struct
{
	mLVItemMan *manager;
	int itemHeight;
	uint32_t style;
}mComboBoxData;

typedef struct _mComboBox
{
	mWidget wg;
	mComboBoxData cb;
}mComboBox;


enum MCOMBOBOX_NOTIFY
{
	MCOMBOBOX_N_CHANGESEL
};


void mComboBoxDestroyHandle(mWidget *p);
void mComboBoxCalcHintHandle(mWidget *p);
void mComboBoxDrawHandle(mWidget *p,mPixbuf *pixbuf);
int mComboBoxEventHandle(mWidget *wg,mEvent *ev);

mComboBox *mComboBoxCreate(mWidget *parent,int id,uint32_t style,uint32_t fLayout,uint32_t marginB4);

mComboBox *mComboBoxNew(int size,mWidget *parent,uint32_t style);

void mComboBoxSetItemHeight(mComboBox *p,int height);

mListViewItem *mComboBoxAddItem(mComboBox *p,const char *text,intptr_t param);
void mComboBoxAddItem_ptr(mComboBox *p,mListViewItem *item);
mListViewItem *mComboBoxAddItem_draw(mComboBox *p,const char *text,intptr_t param,
	void (*draw)(mPixbuf *,mListViewItem *,mListViewItemDraw *));

void mComboBoxAddItems(mComboBox *p,const char *text,intptr_t paramtop);
void mComboBoxAddTrItems(mComboBox *p,int num,int tridtop,intptr_t paramtop);

void mComboBoxDeleteAllItem(mComboBox *p);
void mComboBoxDeleteItem(mComboBox *p,mListViewItem *item);
void mComboBoxDeleteItem_index(mComboBox *p,int index);
mListViewItem *mComboBoxDeleteItem_sel(mComboBox *p);

mListViewItem *mComboBoxGetTopItem(mComboBox *p);
mListViewItem *mComboBoxGetSelItem(mComboBox *p);
mListViewItem *mComboBoxGetItemByIndex(mComboBox *p,int index);
int mComboBoxGetItemNum(mComboBox *p);
int mComboBoxGetSelItemIndex(mComboBox *p);
int mComboBoxGetItemIndex(mComboBox *p,mListViewItem *item);

void mComboBoxGetItemText(mComboBox *p,int index,mStr *str);
intptr_t mComboBoxGetItemParam(mComboBox *p,int index);

void mComboBoxSetItemText(mComboBox *p,int index,const char *text);

void mComboBoxSetSelItem(mComboBox *p,mListViewItem *item);
void mComboBoxSetSel_index(mComboBox *p,int index);
mBool mComboBoxSetSel_findParam(mComboBox *p,intptr_t param);
void mComboBoxSetSel_findParam_notfind(mComboBox *p,intptr_t param,int notfindindex);

mListViewItem *mComboBoxFindItemParam(mComboBox *p,intptr_t param);
void mComboBoxSetWidthAuto(mComboBox *p);

#ifdef __cplusplus
}
#endif

#endif
