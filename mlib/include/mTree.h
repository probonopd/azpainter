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

#ifndef MLIB_TREE_H
#define MLIB_TREE_H

#include "mTreeDef.h"

#ifdef __cplusplus
extern "C" {
#endif

mTreeItem *mTreeAppendNew(mTree *tree,mTreeItem *parent,int size,void (*destroy)(mTreeItem *));
mTreeItem *mTreeInsertNew(mTree *tree,mTreeItem *insert,int size,void (*destroy)(mTreeItem *));

void mTreeLinkBottom(mTree *tree,mTreeItem *parent,mTreeItem *item);
void mTreeLinkInsert(mTree *tree,mTreeItem *ins,mTreeItem *item);
void mTreeLinkInsert_parent(mTree *tree,mTreeItem *parent,mTreeItem *ins,mTreeItem *item);
void mTreeLinkRemove(mTree *tree,mTreeItem *item);

void mTreeDeleteAll(mTree *p);
void mTreeDeleteItem(mTree *p,mTreeItem *item);
void mTreeMoveItem(mTree *p,mTreeItem *src,mTreeItem *dst);
void mTreeMoveItem_top(mTree *p,mTreeItem *src,mTreeItem *parent);
void mTreeMoveItem_bottom(mTree *p,mTreeItem *src,mTreeItem *parent);

mTreeItem *mTreeGetLastItem(mTree *p);
mTreeItem *mTreeGetFirstItem(mTree *p,mTreeItem *parent);

int mTreeItemGetNum(mTree *p);
void mTreeSortChild(mTree *p,mTreeItem *root,int (*comp)(mTreeItem *,mTreeItem *,intptr_t),intptr_t param);
void mTreeSortAll(mTree *p,int (*comp)(mTreeItem *,mTreeItem *,intptr_t),intptr_t param);

/*--------*/

mTreeItem *mTreeItemGetNext(mTreeItem *p);
mTreeItem *mTreeItemGetNext_root(mTreeItem *p,mTreeItem *root);
mTreeItem *mTreeItemGetNextPass(mTreeItem *p);
mTreeItem *mTreeItemGetNextPass_root(mTreeItem *p,mTreeItem *root);

mTreeItem *mTreeItemGetPrev(mTreeItem *p);
mTreeItem *mTreeItemGetPrev_root(mTreeItem *p,mTreeItem *root);
mTreeItem *mTreeItemGetPrevPass(mTreeItem *p);

mTreeItem *mTreeItemGetBottom(mTreeItem *p);

mBool mTreeItemIsChild(mTreeItem *p,mTreeItem *parent);

#ifdef __cplusplus
}
#endif

#endif
