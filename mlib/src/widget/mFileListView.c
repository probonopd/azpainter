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
 * mFileListView
 *****************************************/

#include <string.h>
#include <time.h>

#include "mDef.h"

#include "mFileListView.h"
#include "mLVItemMan.h"
#include "mImageList.h"
#include "mListHeader.h"

#include "mWidget.h"
#include "mEvent.h"
#include "mStr.h"
#include "mDirEntry.h"
#include "mUtil.h"
#include "mUtilStr.h"
#include "mFileStat.h"
#include "mTrans.h"
#include "mFont.h"


//----------------------

#define _FILETYPE_PARENT  0
#define _FILETYPE_DIR     1
#define _FILETYPE_FILE    2

//----------------------

typedef struct
{
	mListViewItem i;
	uint64_t filesize,filemodify;
}_itemex;

//----------------------


//リストのアイコン PNG データ (16x16 x 2 [file,directory])
static const unsigned char g_icon_pngdat[163] = {
0x89,0x50,0x4e,0x47,0x0d,0x0a,0x1a,0x0a,0x00,0x00,0x00,0x0d,0x49,0x48,0x44,0x52,
0x00,0x00,0x00,0x20,0x00,0x00,0x00,0x10,0x08,0x02,0x00,0x00,0x00,0xf8,0x62,0xea,
0x0e,0x00,0x00,0x00,0x6a,0x49,0x44,0x41,0x54,0x18,0x95,0xed,0xcf,0xb1,0x09,0x00,
0x21,0x10,0x44,0xd1,0x29,0xe0,0x4a,0xb2,0x37,0x7b,0xb3,0x89,0xed,0xc2,0xd0,0xd4,
0x0b,0x84,0x63,0x59,0x75,0x5c,0xf0,0xcc,0xfc,0x0c,0x08,0x9b,0x3c,0x04,0x2a,0xce,
0xae,0x3d,0xcf,0xbc,0xdf,0x80,0xda,0xd5,0x8e,0x03,0xa3,0x6f,0x07,0x18,0x18,0x40,
0xc9,0x51,0x8f,0x19,0x1e,0xc0,0x1a,0x80,0xa4,0x60,0x36,0xfd,0x16,0x07,0x74,0x1c,
0x18,0x93,0x1c,0x10,0x95,0x01,0x4a,0x8e,0x7c,0x2e,0x40,0x4b,0x17,0xb8,0x80,0x07,
0x10,0x9a,0x01,0x24,0x85,0xe5,0x2c,0xb0,0x4c,0x03,0xde,0x3e,0xe0,0xdc,0x5e,0x5b,
0x8b,0x05,0x9b,0xfc,0x18,0x11,0xe6,0x00,0x00,0x00,0x00,0x49,0x45,0x4e,0x44,0xae,
0x42,0x60,0x82 };

//----------------------


/******************************//**

@defgroup filelistview mFileListView
@brief ファイルリストビュー

<h3>継承</h3>
mWidget \> mScrollView \> mListView \> mFileListView

@ingroup group_widget
@{

@file mFileListView.h
@def M_FILELISTVIEW(p)
@struct mFileListViewData
@struct mFileListView
@enum MFILELISTVIEW_STYLE
@enum MFILELISTVIEW_NOTIFY
@enum MFILELISTVIEW_FILETYPE

@var MFILELISTVIEW_STYLE::MFILELISTVIEW_S_MULTI_SEL
ファイルの複数選択を有効にする

@var MFILELISTVIEW_STYLE::MFILELISTVIEW_S_ONLY_DIR
ディレクトリのみ表示する

@var MFILELISTVIEW_STYLE::MFILELISTVIEW_S_SHOW_HIDDEN_FILES
隠しファイルを表示する

@var MFILELISTVIEW_NOTIFY::MFILELISTVIEW_N_SELECT_FILE
ファイルが選択された時

@var MFILELISTVIEW_NOTIFY::MFILELISTVIEW_N_DBLCLK_FILE
ファイルがダブルクリックされた時

@var MFILELISTVIEW_NOTIFY::MFILELISTVIEW_N_CHANGE_DIR
カレントディレクトリが変更された時

********************************/


//=============================
// sub
//=============================


/** ソート関数 */

static int _sortfunc_file(mListItem *item1,mListItem *item2,intptr_t param)
{
	_itemex *p1,*p2;
	int type,up,n;

	p1 = (_itemex *)item1;
	p2 = (_itemex *)item2;

	if(p1->i.param < p2->i.param)
		return -1;
	else if(p1->i.param > p2->i.param)
		return 1;
	else
	{
		type = param & 0xff;
		up = param >> 8;
		n = -2;

		//ファイルサイズ

		if(type == 1 && p1->i.param == _FILETYPE_FILE
			&& p1->filesize != p2->filesize)
		{
			n = (p1->filesize < p2->filesize)? -1: 1;
			if(up) n = -n;
		}

		//更新日時
		
		if(type == 2 && p1->filemodify != p2->filemodify)
		{
			n = (p1->filemodify < p2->filemodify)? -1: 1;
			if(up) n = -n;
		}

		//ファイル名 (上記の比較が行われなかった場合は常にファイル名-降順で)

		if(n == -2)
		{
			n = mStrcmp_endchar(p1->i.text, p2->i.text, '\t');
			if(up && type == 0) n = -n;
		}

		return n;
	}
}

/** アイテムソート */

static void _sortitem(mFileListView *p)
{
	mListViewSortItem(M_LISTVIEW(p), _sortfunc_file, (p->flv.sort_up << 8) | p->flv.sort_type);
}

/** アイテム追加 */

static void _add_listitem(mFileListView *p,const char *name,mFileStat *st,mStr *str)
{
	time_t t;
	char m[32];
	int directory;
	_itemex *pi;

	directory = ((st->flags & MFILESTAT_F_DIRECTORY) != 0);

	mStrSetText(str, name);
	mStrAppendChar(str, '\t');

	//サイズ

	if(!directory)
	{
		if(st->size < 1024)
			mStrAppendFormat(str, "%d byte", st->size);
		else if(st->size < 1024 * 1024)
			mStrAppendFormat(str, "%.1F KiB", st->size * 10 / 1024);
		else if(st->size < 1024 * 1024 * 1024)
			mStrAppendFormat(str, "%.1F MiB", st->size * 10 / (1024 * 1024));
		else
			mStrAppendFormat(str, "%.1F GiB", st->size * 10 / (1024 * 1024 * 1024));
	}

	mStrAppendChar(str, '\t');

	//更新日

	t = st->timeModify;
	strftime(m, 32, "%Y/%m/%d", localtime(&t));
	
	mStrAppendText(str, m);

	//追加
	
	pi = (_itemex *)mListViewAddItem_ex(M_LISTVIEW(p), sizeof(_itemex),
		str->buf, directory, 0,
		(directory)? _FILETYPE_DIR: _FILETYPE_FILE);

	pi->filesize = st->size;
	pi->filemodify = st->timeModify;
}

/** ファイルリストセット */

static void _setFileList(mFileListView *p)
{
	mDirEntry *dir;
	const char *fname;
	mStr str = MSTR_INIT;
	mFileStat st;
	mBool hide_hidden_files;

	//クリア

	mListViewDeleteAllItem(M_LISTVIEW(p));

	//親に戻る

	if(!mStrPathIsTop(&p->flv.strDir))
		mListViewAddItem(M_LISTVIEW(p), "..", 1, 0, _FILETYPE_PARENT);

	//------ ファイル

	hide_hidden_files = !(p->flv.style & MFILELISTVIEW_S_SHOW_HIDDEN_FILES);

	dir = mDirEntryOpen(p->flv.strDir.buf);
	if(!dir) return;

	while(mDirEntryRead(dir))
	{
		if(mDirEntryIsSpecName(dir)) continue;

		//隠しファイル

		if(hide_hidden_files && mDirEntryIsHiddenFile(dir))
			continue;

		//
		
		fname = mDirEntryGetFileName(dir);

		mDirEntryGetStat(dir, &st);

		if(st.flags & MFILESTAT_F_DIRECTORY)
		{
			//ディレクトリ
			_add_listitem(p, fname, &st, &str);
		}
		else
		{
			//--- ファイル

			//ディレクトリのみ時

			if(p->flv.style & MFILELISTVIEW_S_ONLY_DIR)
				continue;

			//フィルタ

			if(!mIsMatchStringSum(fname, p->flv.strFilter.buf, ';', TRUE))
				continue;

			//追加

			_add_listitem(p, fname, &st, &str);
		}
	}

	mStrFree(&str);

	mDirEntryClose(dir);

	//ソート

	_sortitem(p);
}

/** 通知 */

static void _notify(mFileListView *p,int type)
{
	mWidgetAppendEvent_notify(MWIDGET_NOTIFYWIDGET_RAW, M_WIDGET(p), type, 0, 0);
}

/** mStr に指定アイテムのファイル名追加 */

static void _append_itemname(mStr *str,mListViewItem *pi,mBool addpath)
{
	const char *pc = pi->text,*pcend;
	
	if(addpath)
		mStrPathAdd(str, NULL);

	if(pc)
	{
		pcend = mStrchr_end(pc, '\t');

		mStrAppendTextLen(str, pc, pcend - pc);
	}
}


//=============================
//
//=============================


/** 解放処理 */

void mFileListViewDestroyHandle(mWidget *wg)
{
	mFileListView *p = M_FILELISTVIEW(wg);

	mStrFree(&p->flv.strDir);
	mStrFree(&p->flv.strFilter);

	//リストビュー

	mListViewDestroyHandle(wg);
}

/** 作成 */

mFileListView *mFileListViewNew(int size,mWidget *parent,uint32_t style)
{
	mFileListView *p;
	mImageList *img;
	mListHeader *header;
	mFont *font;
	int margin;

	//mListView
	
	if(size < sizeof(mFileListView)) size = sizeof(mFileListView);
	
	p = (mFileListView *)mListViewNew(size, parent,
			((style & MFILELISTVIEW_S_MULTI_SEL)? MLISTVIEW_S_MULTI_SEL: 0)
			 | MLISTVIEW_S_DESTROY_IMAGELIST | MLISTVIEW_S_GRID_COL
			 | MLISTVIEW_S_MULTI_COLUMN | MLISTVIEW_S_HEADER_SORT,
			MSCROLLVIEW_S_HORZVERT_FRAME);
	if(!p) return NULL;

	p->wg.destroy = mFileListViewDestroyHandle;
	p->wg.event = mFileListViewEventHandle;
	p->wg.fLayout = MLF_EXPAND_WH;
	p->wg.notifyTargetInterrupt = MWIDGET_NOTIFYTARGET_INT_SELF; //エリアからの通知を自身で受ける

	p->lv.itemHeight = mListViewGetItemNormalHeight(M_LISTVIEW(p)) + 2;
	
	p->flv.style = style;

	//列

	font = mWidgetGetFont(M_WIDGET(p));
	margin = mListViewGetColumnMarginWidth();

	header = mListViewGetHeader(M_LISTVIEW(p));

	M_TR_G(M_TRGROUP_SYS);

	mListHeaderAddItem(header, M_TR_T(M_TRSYS_FILENAME), 100, MLISTHEADER_ITEM_F_EXPAND, 0);

	mListHeaderAddItem(header, M_TR_T(M_TRSYS_FILESIZE),
		mFontGetTextWidth(font, "9999.9 GiB", -1) + margin, MLISTHEADER_ITEM_F_RIGHT, 1);

	mListHeaderAddItem(header, M_TR_T(M_TRSYS_FILEMODIFY), mFontGetTextWidth(font, "9999/99/99", -1) + margin, 0, 2);

	mListHeaderSetSortParam(header, 0, 0);

	//アイコン

	img = mImageListLoadPNG_fromBuf(g_icon_pngdat, sizeof(g_icon_pngdat), 16, 0x00ff00);
	mListViewSetImageList(M_LISTVIEW(p), img);

	//パス

	mStrPathSetHomeDir(&p->flv.strDir);
	mStrSetChar(&p->flv.strFilter, '*');
	
	return p;
}

/** ディレクトリセット
 *
 * @param path NULL または空文字列でホームディレクトリ */

void mFileListViewSetDirectory(mFileListView *p,const char *path)
{
	if(!path || !(*path))
		mStrPathSetHomeDir(&p->flv.strDir);
	else
	{
		mStrSetText(&p->flv.strDir, path);
		mStrPathRemoveBottomPathSplit(&p->flv.strDir);
	}

	_setFileList(p);
}

/** フィルタ文字列セット
 *
 * ワイルドカード有効。複数指定する場合は ';' で区切る。NULL で '*'。
 *
 * @param filter NULL で '*' にセットする */

void mFileListViewSetFilter(mFileListView *p,const char *filter)
{
	if(filter)
		mStrSetText(&p->flv.strFilter, filter);
	else
		mStrSetChar(&p->flv.strFilter, '*');
}

/** 隠しファイルの表示を変更 */

void mFileListViewSetShowHiddenFiles(mFileListView *p,int type)
{
	if(mIsChangeState(type, p->flv.style & MFILELISTVIEW_S_SHOW_HIDDEN_FILES))
	{
		p->flv.style ^= MFILELISTVIEW_S_SHOW_HIDDEN_FILES;

		_setFileList(p);
	}
}

/** ソートタイプ変更 */

void mFileListViewSetSortType(mFileListView *p,int type,mBool up)
{
	p->flv.sort_type = type;
	p->flv.sort_up = up;

	mListHeaderSetSortParam(mListViewGetHeader(M_LISTVIEW(p)), type, up);

	_sortitem(p);
}

/** リストを更新 */

void mFileListViewUpdateList(mFileListView *p)
{
	_setFileList(p);
}

/** ディレクトリを一つ上の親に移動 */

void mFileListViewMoveDir_parent(mFileListView *p)
{
	mStrPathRemoveFileName(&p->flv.strDir);

	_setFileList(p);
	_notify(p, MFILELISTVIEW_N_CHANGE_DIR);
}

/** 現在選択されているファイルの名前を取得
 *
 * @return MFILELISTVIEW_FILETYPE_* */

int mFileListViewGetSelectFileName(mFileListView *p,mStr *strdst,mBool bFullPath)
{
	mListViewItem *pi;

	mStrEmpty(strdst);

	pi = p->lv.manager->itemFocus;

	if(!pi || pi->param == _FILETYPE_PARENT)
		return MFILELISTVIEW_FILETYPE_NONE;

	//パスセット

	if(bFullPath)
		mStrCopy(strdst, &p->flv.strDir);

	_append_itemname(strdst, pi, bFullPath);

	return (pi->param == _FILETYPE_FILE)?
		MFILELISTVIEW_FILETYPE_FILE: MFILELISTVIEW_FILETYPE_DIR;
}

/** 複数選択時、すべての選択ファイル名を取得
 *
 * @param str \c "directory\tfile1\tfile2\t...\t"
 *
 * @return ファイル数 */

int mFileListViewGetSelectMultiName(mFileListView *p,mStr *strdst)
{
	mListViewItem *pi;
	int num = 0;

	//ディレクトリ

	mStrCopy(strdst, &p->flv.strDir);
	mStrAppendChar(strdst, '\t');

	//ファイル

	for(pi = M_LISTVIEWITEM(p->lv.manager->list.top); pi; pi = M_LISTVIEWITEM(pi->i.next))
	{
		if((pi->flags & MLISTVIEW_ITEM_F_SELECTED) && pi->param == _FILETYPE_FILE)
		{
			_append_itemname(strdst, pi, FALSE);
			mStrAppendChar(strdst, '\t');

			num++;
		}
	}

	return num;
}


//========================
// ハンドラ
//========================


/** ダブルクリック時 */

static void _dblclk(mFileListView *p)
{
	mListViewItem *pi;

	pi = p->lv.manager->itemFocus;
	if(!pi) return;

	switch(pi->param)
	{
		//親へ
		case _FILETYPE_PARENT:
			mFileListViewMoveDir_parent(p);
			break;
		//ディレクトリ
		case _FILETYPE_DIR:
			_append_itemname(&p->flv.strDir, pi, TRUE);

			_setFileList(p);
			_notify(p, MFILELISTVIEW_N_CHANGE_DIR);
			break;
		//ファイル
		case _FILETYPE_FILE:
			_notify(p, MFILELISTVIEW_N_DBLCLK_FILE);
			break;
	}
}

/** イベント */

int mFileListViewEventHandle(mWidget *wg,mEvent *ev)
{
	mFileListView *p = M_FILELISTVIEW(wg);
	mListViewItem *pi;

	//mListView の通知イベント

	if(ev->type == MEVENT_NOTIFY
		&& ev->notify.widgetFrom == wg)
	{
		switch(ev->notify.type)
		{
			//フォーカスアイテム変更
			case MLISTVIEW_N_CHANGE_FOCUS:
				pi = p->lv.manager->itemFocus;

				if(pi && pi->param == _FILETYPE_FILE)
					_notify(p, MFILELISTVIEW_N_SELECT_FILE);
				break;
			//ダブルクリック
			case MLISTVIEW_N_ITEM_DBLCLK:
				_dblclk(p);
				break;
			//ソート情報変更
			case MLISTVIEW_N_CHANGE_SORT:
				p->flv.sort_type = M_LISTHEADER_ITEM(ev->notify.param1)->param;
				p->flv.sort_up = ev->notify.param2;

				_sortitem(p);
				break;
		}
	
		return TRUE;
	}

	return mListViewEventHandle(wg, ev);
}

/** @} */
