/*$
 Copyright (C) 2013-2017 Azel.

 This file is part of AzPainter.

 AzPainter is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 AzPainter is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
$*/

/***************************************
 * フィルタのデータ保存管理
 ***************************************/
/*
 * a100b4c400...
 *
 * というように、英小文字を識別子とし、その後に数値が続く。
 */

#include <stdlib.h>
#include <stdio.h>

#include "mDef.h"
#include "mList.h"
#include "mIniRead.h"
#include "mIniWrite.h"


//----------------

typedef struct
{
	mListItem i;

	int cmdid;	//フィルタのコマンドID
	char *text;
}_item;

//----------------

static mList g_list = MLIST_INIT;

//----------------


//==========================
// sub
//==========================


/** アイテム破棄ハンドラ */

static void _destroy_item(mListItem *p)
{
	mFree(((_item *)p)->text);
}

/** コマンドIDからアイテム検索 */

static _item *_find_by_cmdid(int cmdid)
{
	_item *pi;

	for(pi = (_item *)g_list.top; pi; pi = (_item *)pi->i.next)
	{
		if(cmdid == pi->cmdid) return pi;
	}

	return NULL;
}


//==========================
//
//==========================


/** 解放 */

void FilterSaveData_free()
{
	mListDeleteAll(&g_list);
}

/** 設定ファイルからデータ読み込み */

void FilterSaveData_getConfig(mIniRead *ini)
{
	const char *key,*txt;
	_item *pi;

	if(mIniReadSetGroup(ini, "filtersave"))
	{
		while(mIniReadGetNextItem(ini, &key, &txt))
		{
			pi = (_item *)mListAppendNew(&g_list, sizeof(_item), _destroy_item);
			if(pi && txt)
			{
				pi->cmdid = atoi(key);
				pi->text = mStrdup(txt);
			}
		}
	}
}

/** 設定ファイルにセット */

void FilterSaveData_setConfig(void *fp_ptr)
{
	FILE *fp = (FILE *)fp_ptr;
	_item *pi;

	if(!g_list.top) return;

	mIniWriteGroup(fp, "filtersave");

	for(pi = (_item *)g_list.top; pi; pi = (_item *)pi->i.next)
		mIniWriteNoText(fp, pi->cmdid, pi->text);
}

/** 指定コマンドIDのテキストを取得 */

char *FilterSaveData_getText(int cmdid)
{
	_item *pi = _find_by_cmdid(cmdid);

	return (pi)? pi->text: NULL;
}

/** テキストから指定IDの値を取得 */

mBool FilterSaveData_getValue(char *text,char id,int *dst)
{
	char *p;

	if(!text) return FALSE;

	for(p = text; *p; )
	{
		if(*p == id)
		{
			//値取得

			*dst = atoi(p + 1);

			return TRUE;
		}
		
		//次へ (英小文字以外をスキップ)

		for(p++; *p && (*p < 'a' || *p > 'z'); p++);
	}

	return FALSE;
}

/** データを追加または変更 */

void FilterSaveData_setData(int cmdid,const char *text)
{
	_item *pi = _find_by_cmdid(cmdid);

	if(pi)
		mStrdup_ptr(&pi->text, text);
	else if(text)
	{
		//追加
		
		pi = (_item *)mListAppendNew(&g_list, sizeof(_item), _destroy_item);
		if(pi)
		{
			pi->cmdid = cmdid;
			pi->text = mStrdup(text);
		}
	}
}

