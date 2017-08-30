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
 * mTree [ツリーデータ]
 *****************************************/

#include "mDef.h"

#include "mTree.h"


/******************//**

@defgroup tree mTree
@brief ツリーデータ
@ingroup group_data
 
@{
@file mTreeDef.h
@file mTree.h

@struct mTree
@struct _mTreeItem

@def MTREE_INIT
mTree 構造体の初期化子

@def M_TREEITEM(p)
(mTreeItem *) に型変換

**********************/


//***********************************
// sub
//***********************************


/** アイテム作成 */

static mTreeItem *_create_item(int size,void (*destroy)(mTreeItem *))
{
	mTreeItem *p;

	if(size < sizeof(mTreeItem)) size = sizeof(mTreeItem);

	p = (mTreeItem *)mMalloc(size, TRUE);

	if(p) p->destroy = destroy;

	return p;
}

/** アイテム削除 */

static void _delete_item(mTreeItem *p)
{
	if(p->destroy)
		(p->destroy)(p);

	//前後をつなげる

	if(p->prev)
		p->prev->next = p->next;
	else if(p->parent)
		p->parent->first = p->next;

	if(p->next)
		p->next->prev = p->prev;
	else if(p->parent)
		p->parent->last = p->prev;

	//

	mFree(p);
}

/** 子アイテムをすべて削除 (root 自身は削除しない) */

static void _delete_children(mTreeItem *root)
{
	mTreeItem *p,*parent,*next;

	p = root->first;
	parent = root;

	while(p)
	{
		if(p->first)
		{
			//子アイテムありの場合 -> 子アイテムがないアイテムが来るまで繰り返す

			parent = p;
			p = p->first;
		}
		else
		{
			//子アイテムがない場合 -> 削除

			next = p->next;

			_delete_item(p);

			//次のアイテム (NULL の場合は親へ戻る)

			p = next;

			while(!p)
			{
				if(parent == root) break;

				p = parent;
				parent = p->parent;
			}
		}
	}
}


//***********************************
// mTreeItem
//***********************************


/** 次のアイテム取得 */

mTreeItem *mTreeItemGetNext(mTreeItem *p)
{
	if(p->first)
		return p->first;
	else
		return mTreeItemGetNextPass(p);
}

/** 次のアイテム取得 (root の下位のみ) */

mTreeItem *mTreeItemGetNext_root(mTreeItem *p,mTreeItem *root)
{
	if(p->first)
		return p->first;
	else
		return mTreeItemGetNextPass_root(p, root);
}

/** 次のアイテム取得 (p の子は通過する) */

mTreeItem *mTreeItemGetNextPass(mTreeItem *p)
{
	while(p)
	{
		if(p->next)
			return p->next;
		else
			p = p->parent;
	}

	return NULL;
}

/** 次のアイテム取得 (root の下位のみ。p の子は通過する) */

mTreeItem *mTreeItemGetNextPass_root(mTreeItem *p,mTreeItem *root)
{
	if(p == root) return NULL;

	while(p)
	{
		if(p->next)
			return p->next;
		else
		{
			p = p->parent;
			if(p == root) return NULL;
		}
	}

	return NULL;
}


/** 前のアイテム取得
 *
 * @return 前のアイテムに子アイテムがある場合は、その最後のアイテム。\n
 * 親の最後のアイテムの場合は、親が返る。 */

mTreeItem *mTreeItemGetPrev(mTreeItem *p)
{
	if(!p->prev)
		return p->parent;
	else
	{
		for(p = p->prev; p->last; p = p->last);

		return p;
	}
}

/** 前のアイテム取得 (root の下位のみ) */

mTreeItem *mTreeItemGetPrev_root(mTreeItem *p,mTreeItem *root)
{
	if(p == root)
		return NULL;
	else if(!p->prev)
		return (p->parent == root)? NULL: p->parent;
	else
	{
		for(p = p->prev; p->last; p = p->last);

		return p;
	}
}

/** 前のアイテム取得 (子アイテムは通過する) */

mTreeItem *mTreeItemGetPrevPass(mTreeItem *p)
{
	while(p)
	{
		if(p->prev)
			return p->prev;
		else
			p = p->parent;
	}

	return NULL;
}

/** p のツリー上において最後のアイテム取得
 *
 * @return 子がない場合 NULL */

mTreeItem *mTreeItemGetBottom(mTreeItem *p)
{
	for(p = p->last; p && p->last; p = p->last);

	return p;
}

/** parent の最初の子を取得
 *
 * @param parent NULL でルート */

mTreeItem *mTreeGetFirstItem(mTree *p,mTreeItem *parent)
{
	return (parent)? parent->first: p->top;
}

/** p が parent の子アイテムかどうか */

mBool mTreeItemIsChild(mTreeItem *p,mTreeItem *parent)
{
	for(p = p->parent; p; p = p->parent)
	{
		if(p == parent) return TRUE;
	}

	return FALSE;
}


//***********************************
// mTree
//***********************************


/** 新規作成して親の最後に追加
 *
 * @param parent NULL でルート */

mTreeItem *mTreeAppendNew(mTree *tree,mTreeItem *parent,int size,void (*destroy)(mTreeItem *))
{
	mTreeItem *pi;

	if((pi = _create_item(size, destroy)))
		mTreeLinkBottom(tree, parent, pi);

	return pi;
}

/** 新規作成して指定位置の前に挿入
 *
 * @param insert NULL の場合エラー */

mTreeItem *mTreeInsertNew(mTree *tree,mTreeItem *insert,int size,void (*destroy)(mTreeItem *))
{
	mTreeItem *pi;
	
	if(!insert) return NULL;

	if((pi = _create_item(size, destroy)))
		mTreeLinkInsert(tree, insert, pi);

	return pi;
}

/** すべて削除 */

void mTreeDeleteAll(mTree *p)
{
	mTreeItem *pi,*next;

	for(pi = p->top; pi; pi = next)
	{
		next = pi->next;

		_delete_children(pi);
		_delete_item(pi);
	}

	p->top = p->bottom = NULL;
}

/** 指定アイテムを削除 (子も削除される) */

void mTreeDeleteItem(mTree *p,mTreeItem *item)
{
	if(item)
	{
		mTreeLinkRemove(p, item);

		_delete_children(item);
		_delete_item(item);
	}
}

/** 終端にリンクする
 *
 * @param parent NULL でルート */

void mTreeLinkBottom(mTree *tree,mTreeItem *parent,mTreeItem *item)
{
	if(!item) return;
	
	item->parent = parent;

	if(parent)
	{
		//親アイテムあり

		item->prev = parent->last;
		parent->last = item;

		if(item->prev)
			item->prev->next = item;
		else
			parent->first = item;
	}
	else
	{
		//親がルート

		if(!tree->top)
			//データがひとつもない時
			tree->top = tree->bottom = item;
		else
		{
			//データが一つ以上存在する時

			tree->bottom->next = item;
			item->prev = tree->bottom;
			tree->bottom = item;
		}
	}
}

/** 挿入リンク */

void mTreeLinkInsert(mTree *tree,mTreeItem *ins,mTreeItem *item)
{
	if(!item) return;
	
	item->parent = ins->parent;

	/* すでに存在している ins の前に挿入するので、
	 * item が親の最後のアイテムになることはない。
	 * よって、tree->bottom や last は変化しない。 */

	if(ins->prev)
		ins->prev->next = item;     //前のアイテムあり -> 前の next = p
	else if(item->parent)
		item->parent->first = item; //ルートでなければ、親の先頭へ
	else
		tree->top = item;           //ルートの先頭

	//前後をつなげる

	item->prev = ins->prev;
	ins->prev = item;
	item->next = ins;
}

/** 親と挿入位置を指定してリンク
 *
 * @param parent  NULL でルート
 * @param ins     NULL で parent の終端 */

void mTreeLinkInsert_parent(mTree *tree,mTreeItem *parent,mTreeItem *ins,mTreeItem *item)
{
	if(ins)
		//ins の前へ
		mTreeLinkInsert(tree, ins, item);
	else
		//parent の終端へ
		mTreeLinkBottom(tree, parent, item);
}

/** リンクを外す (子のリンクはそのまま) */

void mTreeLinkRemove(mTree *tree,mTreeItem *item)
{
	if(!item) return;
	
	//前後をつなげる

	if(item->prev)
		item->prev->next = item->next;
	else if(item->parent)
		item->parent->first = item->next;
	else
		tree->top = item->next;

	if(item->next)
		item->next->prev = item->prev;
	else if(item->parent)
		item->parent->last = item->prev;
	else
		tree->bottom = item->prev;

	//

	item->parent = item->prev = item->next = NULL;
}

/** アイテム位置を移動 (src を dst の前に) */

void mTreeMoveItem(mTree *p,mTreeItem *src,mTreeItem *dst)
{
	if(src && dst && src != dst)
	{
		mTreeLinkRemove(p, src);
		mTreeLinkInsert(p, dst, src);
	}
}

/** アイテム位置を移動 (src を parent の先頭に)
 *
 * @param parent NULL でルート */

void mTreeMoveItem_top(mTree *p,mTreeItem *src,mTreeItem *parent)
{
	mTreeLinkRemove(p, src);

	if(parent)
		mTreeLinkInsert_parent(p, parent, parent->first, src);
	else
		mTreeLinkInsert_parent(p, NULL, p->top, src);
}

/** アイテム位置を移動 (src を parent の終端に)
 *
 * @param parent NULL でルート */

void mTreeMoveItem_bottom(mTree *p,mTreeItem *src,mTreeItem *parent)
{
	mTreeLinkRemove(p, src);
	mTreeLinkBottom(p, parent, src);
}

/** ツリー上において一番最後のアイテムを取得 */

mTreeItem *mTreeGetLastItem(mTree *tree)
{
	mTreeItem *p;

	for(p = tree->bottom; p && p->last; p = p->last);

	return p;
}

/** アイテムの全体数取得 */

int mTreeItemGetNum(mTree *tree)
{
	mTreeItem *p;
	int num = 0;

	for(p = tree->top; p; p = mTreeItemGetNext(p), num++);

	return num;
}

/** 子をソート (1階層のみ)
 *
 * @param root  親アイテム。NULL でルート
 * @param param 比較関数に渡すパラメータ */

void mTreeSortChild(mTree *p,mTreeItem *root,int (*comp)(mTreeItem *,mTreeItem *,intptr_t),intptr_t param)
{
	mTree res,tmp;
	mTreeItem *pi,*pi_r,*pinext;
	int i,j,k,lcnt,rcnt;

	//先頭と終端

	if(root)
	{
		res.top = root->first;
		res.bottom = root->last;
	}
	else
		res = *p;

	//子アイテムが1個以下ならソートなし

	if(res.top == res.bottom) return;

	//---- マージソート

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
					mTreeLinkRemove(&res, pi);
					mTreeLinkBottom(&tmp, NULL, pi);
					pi = pinext;

					if(!pi) break;
					
					lcnt--;
				}
				else
				{
					//右側
					
					if(!pi_r) break;

					pinext = pi_r->next;
					mTreeLinkRemove(&res, pi_r);
					mTreeLinkBottom(&tmp, NULL, pi_r);
					pi_r = pinext;
					
					rcnt--;
				}
			}
		}

		res = tmp;

		//結合が1回しか行われなかった場合、終了
		if(j == 1) break;
	}

	//適用

	if(root)
	{
		root->first = res.top;
		root->last = res.bottom;

		//親を再セット

		for(pi = res.top; pi; pi = pi->next)
			pi->parent = root;
	}
	else
	{
		p->top = res.top;
		p->bottom = res.bottom;
	}
}

/** 各親をすべてソート */

void mTreeSortAll(mTree *p,int (*comp)(mTreeItem *,mTreeItem *,intptr_t),intptr_t param)
{
	mTreeItem *pi;

	mTreeSortChild(p, NULL, comp, param);

	for(pi = p->top; pi; pi = mTreeItemGetNext(pi))
	{
		if(pi->first)
			mTreeSortChild(p, pi, comp, param);
	}
}


/** @} */
