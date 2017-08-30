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

/*****************************************
 * mLVItemMan [リストビューアイテム管理]
 *****************************************/

/*
 * 複数選択を有効にする場合は bMultiSel を TRUE にする。
 */

#include <string.h>

#include "mDef.h"
#include "mLVItemMan.h"

#include "mList.h"


//-----------------

#define _NUM_MAX   100000

#define _ITEM_TOP(p)   M_LISTVIEWITEM(p->list.top)

#define _ITEMFLAG_ON(p,f)  (p)->flags |= MLISTVIEW_ITEM_F_ ## f
#define _ITEMFLAG_OFF(p,f) (p)->flags &= ~(MLISTVIEW_ITEM_F_ ## f)

//-----------------


/** mListViewItem 破棄ハンドラ */

void mListViewItemDestroy(mListItem *item)
{
	mListViewItem *p = (mListViewItem *)item;

	if(!(p->flags & MLISTVIEW_ITEM_F_STATIC_TEXT))
	{
		mFree((void *)p->text);
		p->text = NULL;
	}
}


//====================


/** 解放 */

void mLVItemMan_free(mLVItemMan *p)
{
	if(p)
	{
		mListDeleteAll(&p->list);

		mFree(p);
	}
}

/** 確保 */

mLVItemMan *mLVItemMan_new()
{
	return (mLVItemMan *)mMalloc(sizeof(mLVItemMan), TRUE);
}


//=====================


/** テキストセット */

static void _set_text(mListViewItem *pi,const char *text,uint32_t flags)
{
	if(flags & MLISTVIEW_ITEM_F_STATIC_TEXT)
	{
		pi->text = text;
		pi->textlen = strlen(text);
	}
	else
	{
		mFree((void *)pi->text);
		
		pi->textlen = mStrdup2(text, (char **)&pi->text);
	}
}

/** アイテム追加 */

mListViewItem *mLVItemMan_addItem(mLVItemMan *p,int size,
	const char *text,int icon,uint32_t flags,intptr_t param)
{
	mListViewItem *pi;

	if(p->list.num > _NUM_MAX) return NULL;

	if(size < sizeof(mListViewItem))
		size = sizeof(mListViewItem);
	
	pi = (mListViewItem *)mListAppendNew(&p->list, size, mListViewItemDestroy);

	_set_text(pi, text, flags);

	pi->icon = icon;
	pi->flags = flags;
	pi->param = param;

	return pi;
}

/** アイテム挿入 */

mListViewItem *mLVItemMan_insertItem(mLVItemMan *p,mListViewItem *ins,
	const char *text,int icon,uint32_t flags,intptr_t param)
{
	mListViewItem *pi;

	if(p->list.num > _NUM_MAX) return NULL;
	
	pi = (mListViewItem *)mListInsertNew(&p->list, M_LISTITEM(ins),
			sizeof(mListViewItem), mListViewItemDestroy);
	
	_set_text(pi, text, flags);

	pi->icon = icon;
	pi->flags = flags;
	pi->param = param;

	return pi;
}

/** アイテムすべて削除 */

void mLVItemMan_deleteAllItem(mLVItemMan *p)
{
	mListDeleteAll(&p->list);

	p->itemFocus = NULL;
}

/** アイテム削除
 *
 * @return フォーカスアイテムが削除された */

mBool mLVItemMan_deleteItem(mLVItemMan *p,mListViewItem *item)
{
	mBool ret = FALSE;

	if(item)
	{
		if(item == p->itemFocus)
		{
			p->itemFocus = NULL;
			ret = TRUE;
		}

		mListDelete(&p->list, M_LISTITEM(item));
	}

	return ret;
}

/** アイテム削除 (番号から)
 *
 * @param index 負の値で現在のフォーカスアイテム */

mBool mLVItemMan_deleteItemByIndex(mLVItemMan *p,int index)
{
	return mLVItemMan_deleteItem(p,
				mLVItemMan_getItemByIndex(p, index));
}

/** すべて選択 */

void mLVItemMan_selectAll(mLVItemMan *p)
{
	mListViewItem *pi;

	if(p->bMultiSel)
	{
		for(pi = _ITEM_TOP(p); pi; pi = M_LISTVIEWITEM(pi->i.next))
			_ITEMFLAG_ON(pi, SELECTED);
	}
}

/** すべて選択解除 */

void mLVItemMan_unselectAll(mLVItemMan *p)
{
	mListViewItem *pi;
	
	if(p->bMultiSel)
	{
		for(pi = _ITEM_TOP(p); pi; pi = M_LISTVIEWITEM(pi->i.next))
			_ITEMFLAG_OFF(pi, SELECTED);
	}
}

/** アイテムのテキストをセット */

void mLVItemMan_setText(mListViewItem *pi,const char *text)
{
	_set_text(pi, text, pi->flags);
}


//======================
// 取得
//======================


/** 位置からアイテム取得
 *
 * @param index 負の値で現在のフォーカスアイテム */

mListViewItem *mLVItemMan_getItemByIndex(mLVItemMan *p,int index)
{
	if(index < 0)
		return p->itemFocus;
	else
		return (mListViewItem *)mListGetItemByIndex(&p->list, index);
}

/** アイテムのインデックス番号取得
 *
 * @param item NULL でフォーカスアイテム */

int mLVItemMan_getItemIndex(mLVItemMan *p,mListViewItem *item)
{
	if(!item) item = p->itemFocus;

	return mListGetItemIndex(&p->list, M_LISTITEM(item));
}

/** パラメータ値からアイテム検索 */

mListViewItem *mLVItemMan_findItemParam(mLVItemMan *p,intptr_t param)
{
	mListViewItem *pi;

	for(pi = _ITEM_TOP(p); pi; pi = M_LISTVIEWITEM(pi->i.next))
	{
		if(pi->param == param)
			return pi;
	}

	return NULL;
}

/** テキストからアイテム検索 */

mListViewItem *mLVItemMan_findItemText(mLVItemMan *p,const char *text)
{
	mListViewItem *pi;

	for(pi = _ITEM_TOP(p); pi; pi = M_LISTVIEWITEM(pi->i.next))
	{
		if(strcmp(pi->text, text) == 0)
			return pi;
	}

	return NULL;
}

/** アイテムの位置を上下に移動 */

mBool mLVItemMan_moveItem_updown(mLVItemMan *p,mListViewItem *item,mBool down)
{
	if(!item) item = p->itemFocus;

	return mListMoveUpDown(&p->list, M_LISTITEM(item), !down);
}


//=========================
// フォーカスアイテム
//=========================


/** フォーカスアイテムセット (単一選択)
 *
 * @return 変更されたか */

mBool mLVItemMan_setFocusItem(mLVItemMan *p,mListViewItem *item)
{
	if(p->itemFocus == item)
		return FALSE;
	else
	{
		//選択解除
		
		if(p->bMultiSel)
			mLVItemMan_unselectAll(p);
		else
		{
			if(p->itemFocus)
				_ITEMFLAG_OFF(p->itemFocus, SELECTED);
		}

		//選択
		
		if(item) _ITEMFLAG_ON(item, SELECTED);
		
		p->itemFocus = item;
		
		return TRUE;
	}
}

/** 指定位置のアイテムをフォーカスにセット
 *
 * @param index 負の値で選択をなしに */

mBool mLVItemMan_setFocusItemByIndex(mLVItemMan *p,int index)
{
	mListViewItem *pi;

	if(index < 0)
		pi = NULL;
	else
		pi = (mListViewItem *)mListGetItemByIndex(&p->list, index);

	return mLVItemMan_setFocusItem(p, pi);
}

/** フォーカスアイテムを先頭または終端にセット */

mBool mLVItemMan_setFocusHomeEnd(mLVItemMan *p,mBool home)
{
	return mLVItemMan_setFocusItem(p,
			(home)? M_LISTVIEWITEM(p->list.top): M_LISTVIEWITEM(p->list.bottom));
}


//=========================
// 操作
//=========================


/** フォーカスアイテムを上下に選択移動 */

mBool mLVItemMan_updownFocus(mLVItemMan *p,mBool down)
{
	mListViewItem *item;
	
	item = p->itemFocus;
	
	if(!item)
		item = _ITEM_TOP(p);
	else if(down)
	{
		//下
		if(item->i.next) item = M_LISTVIEWITEM(item->i.next);
	}
	else
	{
		//上
		if(item->i.prev) item = M_LISTVIEWITEM(item->i.prev);
	}
	
	return mLVItemMan_setFocusItem(p, item);
}

/** アイテム選択処理
 *
 * @return フォーカスが変更されたか */

mBool mLVItemMan_select(mLVItemMan *p,uint32_t state,mListViewItem *item)
{
	mListViewItem *focus,*pi,*top,*end;
	
	focus = p->itemFocus;

	if(!p->bMultiSel)
	{
		//単一選択

		if(focus) _ITEMFLAG_OFF(focus, SELECTED);
		
		_ITEMFLAG_ON(item, SELECTED);
	}
	else
	{
		//----- 複数選択

		//範囲選択時、カレントアイテムがなければ通常処理
		
		if((state & M_MODS_SHIFT) && !focus)
			state = 0;

		//

		if(state & M_MODS_CTRL)
		{
			//選択反転
			
			item->flags ^= MLISTVIEW_ITEM_F_SELECTED;
		}
		else if(state & M_MODS_SHIFT)
		{
			//範囲 (カレント位置から指定位置まで)

			mLVItemMan_unselectAll(p);
			
			if(mListGetDir(&p->list, M_LISTITEM(focus), M_LISTITEM(item)) < 0)
				top = focus, end = item;
			else
				top = item, end = focus;
			
			for(pi = top; pi; pi = M_LISTVIEWITEM(pi->i.next))
			{
				_ITEMFLAG_ON(pi, SELECTED);
			
				if(pi == end) break;
			}
		}
		else
		{
			//通常 (単一選択)
			
			mLVItemMan_unselectAll(p);
			
			_ITEMFLAG_ON(item, SELECTED);
		}
	}
	
	p->itemFocus = item;
	
	return (focus != item);
}
