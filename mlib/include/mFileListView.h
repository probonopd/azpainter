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

#ifndef MLIB_FILELISTVIEW_H
#define MLIB_FILELISTVIEW_H

#include "mListView.h"
#include "mStrDef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_FILELISTVIEW(p)  ((mFileListView *)(p))

typedef struct
{
	uint32_t style;
	mStr strDir,
		strFilter;
	uint8_t sort_type,sort_up;
}mFileListViewData;

typedef struct _mFileListView
{
	mWidget wg;
	mScrollViewData sv;
	mListViewData lv;
	mFileListViewData flv;
}mFileListView;

enum MFILELISTVIEW_STYLE
{
	MFILELISTVIEW_S_MULTI_SEL = 1<<0,
	MFILELISTVIEW_S_ONLY_DIR  = 1<<1,
	MFILELISTVIEW_S_SHOW_HIDDEN_FILES = 1<<2
};

enum MFILELISTVIEW_NOTIFY
{
	MFILELISTVIEW_N_SELECT_FILE,
	MFILELISTVIEW_N_DBLCLK_FILE,
	MFILELISTVIEW_N_CHANGE_DIR
};

enum MFILELISTVIEW_FILETYPE
{
	MFILELISTVIEW_FILETYPE_NONE,
	MFILELISTVIEW_FILETYPE_FILE,
	MFILELISTVIEW_FILETYPE_DIR
};


void mFileListViewDestroyHandle(mWidget *p);
int mFileListViewEventHandle(mWidget *wg,mEvent *ev);

mFileListView *mFileListViewNew(int size,mWidget *parent,uint32_t style);

void mFileListViewSetDirectory(mFileListView *p,const char *path);
void mFileListViewSetFilter(mFileListView *p,const char *filter);
void mFileListViewSetShowHiddenFiles(mFileListView *p,int type);
void mFileListViewSetSortType(mFileListView *p,int type,mBool up);

void mFileListViewUpdateList(mFileListView *p);
void mFileListViewMoveDir_parent(mFileListView *p);

int mFileListViewGetSelectFileName(mFileListView *p,mStr *str,mBool bFullPath);
int mFileListViewGetSelectMultiName(mFileListView *p,mStr *str);

#ifdef __cplusplus
}
#endif

#endif
