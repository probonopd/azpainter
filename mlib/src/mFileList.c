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
 * mFileList
 *****************************************/

#include <string.h>

#include "mDef.h"

#include "mFileList.h"

#include "mDirEntry.h"
#include "mPath.h"


/**
@defgroup filelist mFileList
@brief ファイルリスト

ディレクトリ内のファイルを列挙して、mList のリストに作成。

@ingroup group_etc
 
@{
@file mFileList.h
@struct mFileListItem

@def M_FILELISTITEM
(mFileListItem *) に型変換
*/


/** ファイル名比較関数 */

static int _comp_name(mListItem *pitem1,mListItem *pitem2,intptr_t param)
{
	return strcmp(M_FILELISTITEM(pitem1)->fname,
				M_FILELISTITEM(pitem2)->fname);
}

/** アイテム破棄ハンドラ */

static void _item_destroy(mListItem *p)
{
	mFree(M_FILELISTITEM(p)->fname);
}

/** ファイルリスト作成
 *
 * @param func リストに追加するかどうかを判定する関数。0 で返すと追加しない。NULL ですべて。 */

mBool mFileListGetList(mList *list,const char *path,int (*func)(const char *,mFileStat *))
{
	mDirEntry *dir;
	mFileListItem *pi;
	mFileStat st;
	const char *fname;

	mListDeleteAll(list);

	//

	dir = mDirEntryOpen(path);
	if(!dir) return FALSE;

	while(mDirEntryRead(dir))
	{
		if(mDirEntryIsSpecName(dir)) continue;

		fname = mDirEntryGetFileName(dir);

		//ステータス

		if(!mDirEntryGetStat(dir, &st)) continue;

		//追加判定

		if(func && (func)(fname, &st) == 0) continue;

		//追加

		pi = (mFileListItem *)mListAppendNew(list, sizeof(mFileListItem), _item_destroy);
		if(!pi) break;

		pi->fname = mStrdup(fname);
		pi->flags = st.flags;
	}

	mDirEntryClose(dir);

	return TRUE;
}

/** ファイル名でソート */

void mFileListSortName(mList *list)
{
	mListSort(list, _comp_name, 0);
}

/** ファイル名から検索 */

mFileListItem *mFileListFindByName(mList *list,const char *name)
{
	mFileListItem *pi;

	for(pi = M_FILELISTITEM(list->top); pi; pi = M_FILELISTITEM(pi->i.next))
	{
		if(mPathCompareEq(name, pi->fname))
			return pi;
	}

	return pi;
}

/** ファイル追加判定関数 : ディレクトリ以外を対象にする */

int mFileListFunc_notdir(const char *fname,mFileStat *st)
{
	return !(st->flags & MFILESTAT_F_DIRECTORY);
}

/** @} */
