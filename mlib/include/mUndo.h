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

#ifndef MLIB_UNDO_H
#define MLIB_UNDO_H

#include "mListDef.h"

typedef struct _mUndo mUndo;

struct _mUndo
{
	mList list;
	mListItem *current;
	int maxnum;

	mListItem *(*create)(mUndo *);
	mBool (*setreverse)(mUndo *,mListItem *,mListItem *,int);
	mBool (*run)(mUndo *,mListItem *,int);
};

enum MUNDO_RUNERR
{
	MUNDO_RUNERR_OK,
	MUNDO_RUNERR_NO_DATA,
	MUNDO_RUNERR_CREATE,
	MUNDO_RUNERR_RUN
};

enum MUNDO_TYPE
{
	MUNDO_TYPE_UNDO,
	MUNDO_TYPE_REDO
};


#ifdef __cplusplus
extern "C" {
#endif

void mUndoFree(mUndo *p);
mUndo *mUndoNew(int size);

mBool mUndoIsHave(mUndo *p,mBool redo);

void mUndoDeleteAll(mUndo *p);
void mUndoDelete_onlyUndo(mUndo *p);
void mUndoDelete_onlyRedo(mUndo *p);

void mUndoAdd(mUndo *p,mListItem *item);

int mUndoRunUndo(mUndo *p);
int mUndoRunRedo(mUndo *p);

#ifdef __cplusplus
}
#endif

#endif
