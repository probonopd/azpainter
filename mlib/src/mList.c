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
 * mList [双方向リスト]
 *****************************************/

#include "mDef.h"
#include "mList.h"


/************************//**

@defgroup list mList
@brief 双方向リスト

@ingroup group_data
@{

@file mList.h
@file mListDef.h
@struct _mList
@struct _mListItem

@def MLIST_INIT
mList 構造体の値を初期化

@def M_LISTITEM(p)
(mListItem *) に型変換

******************************/



/** アイテムのメモリを確保
 *
 * ゼロクリアされる。
 * 
 * @param size アイテム構造体の全サイズ */

mListItem *mListItemAlloc(int size,void (*destroy)(mListItem *))
{
	mListItem *p;

	if(size < sizeof(mListItem))
		size = sizeof(mListItem);
	
	p = (mListItem *)mMalloc(size, TRUE);
	if(!p) return NULL;
	
	p->destroy = destroy;
	
	return p;
}


//====================
// 追加
//====================


/** アイテムを新規作成し、最後に追加
 * 
 * @param size リストアイテムの構造体のサイズ */

mListItem *mListAppendNew(mList *list,int size,void (*destroy)(mListItem *))
{
	mListItem *pi;
	
	pi = mListItemAlloc(size, destroy);
	if(!pi) return NULL;

	mListLinkAppend(list, pi);

	list->num++;

	return pi;
}

/** アイテムを新規作成し、挿入 */

mListItem *mListInsertNew(mList *list,mListItem *pos,int size,void (*destroy)(mListItem *))
{
	mListItem *pi;
	
	pi = mListItemAlloc(size, destroy);
	if(!pi) return NULL;

	mListLinkInsert(list, pi, pos);

	list->num++;

	return pi;
}

/** すでに確保されたアイテムを追加 */

void mListAppend(mList *list,mListItem *item)
{
	if(item)
	{
		mListLinkAppend(list, item);
		list->num++;
	}
}

/** すでに確保されたアイテムを挿入 */

void mListInsert(mList *list,mListItem *pos,mListItem *item)
{
	if(item)
	{
		mListLinkInsert(list, item, pos);
		list->num++;
	}
}

/** アイテムを削除せずにリストから取り外す
 *
 * mList::num の値も変化する。 */

void mListRemove(mList *list,mListItem *item)
{
	if(item)
	{
		mListLinkRemove(list, item);
		list->num--;
	}
}

/** リスト全体を複製 (各アイテムは固定サイズ) */

mBool mListDup(mList *dst,mList *src,int itemsize)
{
	mListItem *ps,*pd;

	if(itemsize < sizeof(mListItem))
		itemsize = sizeof(mListItem);

	for(ps = src->top; ps; ps = ps->next)
	{
		pd = (mListItem *)mMemdup(ps, itemsize);
		if(!pd) return FALSE;

		mListAppend(dst, pd);
	}

	return TRUE;
}


//========================
// 削除
//========================


/** アイテムを全て削除 */

void mListDeleteAll(mList *list)
{
	mListItem *pi,*next;
	
	for(pi = list->top; pi; pi = next)
	{
		next = pi->next;
		
		if(pi->destroy)
			(pi->destroy)(pi);
		
		mFree(pi);
	}
	
	list->top = list->bottom = NULL;
	list->num = 0;
}

/** リストからアイテムを削除
 *
 * @param item NULL の場合は何もしない */

void mListDelete(mList *list,mListItem *item)
{
	if(item)
	{
		if(item->destroy)
			(item->destroy)(item);

		mListLinkRemove(list, item);
		
		mFree(item);
		
		list->num--;
	}
}

/** リストからアイテムを削除
 * 
 * アイテムの destroy() ハンドラは呼ばない。 */

void mListDeleteNoDestroy(mList *list,mListItem *item)
{
	mListLinkRemove(list, item);
	
	mFree(item);
	
	list->num--;
}

/** インデックス位置からアイテム削除
 * 
 * @return 削除されたか */

mBool mListDeleteByIndex(mList *list,int index)
{
	mListItem *p = mListGetItemByIndex(list, index);
	
	if(!p)
		return FALSE;
	else
	{
		mListDelete(list, p);
		return TRUE;
	}
}

/** 先頭から指定個数を削除 */

void mListDeleteTopNum(mList *list,int num)
{
	mListItem *p,*next;

	for(p = list->top; p && num > 0; num--, p = next)
	{
		next = p->next;

		mListDelete(list, p);
	}
}

/** 終端から指定個数を削除 */

void mListDeleteBottomNum(mList *list,int num)
{
	mListItem *p,*next;

	for(p = list->bottom; p && num > 0; num--, p = next)
	{
		next = p->prev;

		mListDelete(list, p);
	}
}


//========================


/** アイテムの位置を移動
 * 
 * @param dst 挿入位置。NULL で最後尾へ
 * @return 移動したか */

mBool mListMove(mList *list,mListItem *src,mListItem *dst)
{
	if(src == dst) return FALSE;
	
	if(!dst && list->bottom == src) return FALSE;
	
	mListLinkRemove(list, src);
	mListLinkInsert(list, src, dst);

	return TRUE;
}

/** アイテムを先頭へ移動 */

void mListMoveTop(mList *list,mListItem *item)
{
	if(list->top != item)
		mListMove(list, item, list->top);
}

/** アイテムを一つ上下に移動
 *
 * @return 移動したか */

mBool mListMoveUpDown(mList *list,mListItem *item,mBool up)
{
	if(!item) return FALSE;
	
	if(up)
	{
		if(item->prev)
		{
			mListMove(list, item, item->prev);
			return TRUE;
		}
	}
	else
	{
		if(item->next)
		{
			mListMove(list, item->next, item);
			return TRUE;
		}
	}

	return FALSE;
}

/** アイテムの位置を入れ替える */

void mListSwap(mList *list,mListItem *item1,mListItem *item2)
{
	mListItem *p;

	if(item1 != item2)
	{
		p = item1->next;
		
		mListMove(list, item1, item2->next);
		mListMove(list, item2, p);
	}
}

/** ソート
 * 
 * @param param 比較関数に渡すパラメータ */

void mListSort(mList *list,int (*comp)(mListItem *,mListItem *,intptr_t),intptr_t param)
{
	mList res,tmp;
	mListItem *pi,*pi_r,*pinext;
	int i,j,k,lcnt,rcnt;

	if(list->num <= 1) return;

	//マージソート

	res = *list;

	for(i = 1; 1; i <<= 1)
	{
		tmp.top = tmp.bottom = NULL;
		pi = res.top;

		//[i][i]... をソートして tmp に追加

		for(j = 0; pi; j++, pi = pi_r)
		{
			//右側の位置
			for(k = i, pi_r = pi; k > 0 && pi_r; k--, pi_r = pi_r->next);

			lcnt = rcnt = i;

			//左側・右側の先頭から、比較して小さい方を取り出して tmp に追加

			while(lcnt || rcnt)
			{
				if(lcnt && (rcnt == 0 || !pi_r || (comp)(pi, pi_r, param) <= 0))
				{
					//左側
					
					pinext = pi->next;
					mListLinkRemove(&res, pi);
					mListLinkAppend(&tmp, pi);
					pi = pinext;

					if(!pi) break;
					
					lcnt--;
				}
				else
				{
					//右側
					
					if(!pi_r) break;

					pinext = pi_r->next;
					mListLinkRemove(&res, pi_r);
					mListLinkAppend(&tmp, pi_r);
					pi_r = pinext;
					
					rcnt--;
				}
			}
		}

		res = tmp;

		//結合が1回しか行われなかった場合、終了
		if(j == 1) break;
	}

	list->top = res.top;
	list->bottom = res.bottom;
}


//========================


/** 2つのアイテムの位置関係を得る
 * 
 * @retval -1  item1 @< item2
 * @retval 0   item1 == item2
 * @retval 1   item1 @> item2 */

int mListGetDir(mList *list,mListItem *item1,mListItem *item2)
{
	mListItem *p;
	
	if(item1 == item2) return 0;
	
	for(p = item1->next; p; p = p->next)
	{
		if(p == item2) return -1;
	}
	
	return 1;
}

/** インデックス位置からアイテム取得 */

mListItem *mListGetItemByIndex(mList *list,int index)
{
	mListItem *p;
	int cnt;
	
	for(p = list->top, cnt = 0; p && cnt != index; p = p->next, cnt++);
	
	return p;
}

/** アイテムのインデックス位置取得 */

int mListGetItemIndex(mList *list,mListItem *item)
{
	int pos;
	mListItem *p;
	
	for(p = list->top, pos = 0; p; p = p->next, pos++)
	{
		if(p == item) return pos;
	}
	
	return -1;
}


//================================
// リンク操作
//================================


/** リストの最後にリンクする
 *
 * @attention mList::num の値は操作されない */

void mListLinkAppend(mList *list,mListItem *item)
{
	if(list->bottom)
	{
		item->prev = list->bottom;
		(list->bottom)->next = item;
	}
	else
	{
		item->prev = NULL;
		list->top = item;
	}
	
	item->next = NULL;
	list->bottom = item;
}

/** リストの指定位置にリンクを挿入
 * 
 * @param pos  挿入位置。NULL で最後尾  */

void mListLinkInsert(mList *list,mListItem *item,mListItem *pos)
{
	if(!pos)
		mListLinkAppend(list, item);
	else
	{
		if(pos->prev)
			(pos->prev)->next = item;
		else
			list->top = item;
		
		item->prev = pos->prev;
		pos->prev = item;
		item->next = pos;
	}
}

/** リストからアイテムのリンクを外す */

void mListLinkRemove(mList *list,mListItem *item)
{
	if(item->prev)
		(item->prev)->next = item->next;
	else
		list->top = item->next;
	
	if(item->next)
		(item->next)->prev = item->prev;
	else
		list->bottom = item->prev;
}

/** @} */
