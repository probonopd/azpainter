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

/*****************************************
 * MaterialImgList
 * 
 * 素材画像の管理リスト
 *****************************************/
/*
 * - 現在使用されているブラシ形状/テクスチャ画像を管理する。
 * - 最近使われたものほどリストの先頭にある。
 * - ブラシ用の形状/テクスチャ画像は、最近使ったものを最大５つ保存しておく。
 * - レイヤのテクスチャは常に読み込んでおく。
 */

#include <string.h>

#include "mDef.h"
#include "mGui.h"
#include "mList.h"
#include "mPath.h"
#include "mStr.h"

#include "ImageBuf8.h"
#include "ImageBuf24.h"

#include "defConfig.h"
#include "defMacros.h"

#include "MaterialImgList.h"


//-----------------------

/** MaterialImgList 本体データ */

typedef struct _MaterialImgList
{
	mList list[2];
}MaterialImgList;

/** リストアイテムデータ */

typedef struct __item
{
	mListItem i;

	ImageBuf8 *img;
	char *path;		//ファイルパス
	int refcnt;		//参照カウンタ
		/* 参照される毎に加算していく。
		 * 明示的に解放されるまで保持される。
		 * 0 の場合は、保存数より後ろの位置に来た場合、削除される。 */
}_item;

//-----------------------

static MaterialImgList g_matimglist;

//-----------------------

#define _TOPITEM(list)  ((_item *)(list)->top)
#define _ITEM(p)        ((_item *)(p))
#define _NEXTITEM(p)    ((_item *)(p)->i.next)

#define _SAVE_NUM     5

#define _LIST_TEXTURE 0
#define _LIST_BRUSH   1

//-----------------------


//=========================
// sub
//=========================


/** リスト内に同じパスの画像があるか */

static _item *_search_path(mList *list,const char *path)
{
	_item *pi;

	for(pi = _TOPITEM(list); pi; pi = _NEXTITEM(pi))
	{
		if(mPathCompareEq(path, pi->path))
			return pi;
	}

	return NULL;
}

/** 古いものを削除 (新規追加前)
 *
 * relcnt が 0 のものが対象 */

static void _delete_old(mList *list)
{
	_item *pi,*next;
	int cnt = 0;

	for(pi = _TOPITEM(list); pi; pi = next)
	{
		next = _NEXTITEM(pi);
	
		if(pi->refcnt == 0)
		{
			cnt++;

			if(cnt >= _SAVE_NUM)
				mListDelete(list, M_LISTITEM(pi));
		}
	}
}

/** 画像読み込み */

static ImageBuf8 *_load_image(int type,const char *path)
{
	ImageBuf24 *img24;
	ImageBuf8 *img8;
	mStr str = MSTR_INIT;

	//パス

	if(path[0] == '/')
	{
		//システム

		mAppGetDataPath(&str,
			(type == MATERIALIMGLIST_TYPE_BRUSH_SHAPE)? APP_BRUSH_PATH: APP_TEXTURE_PATH);

		mStrPathAdd(&str, path + 1);
	}
	else
	{
		//ユーザー

		mStrCopy(&str,
			(type == MATERIALIMGLIST_TYPE_BRUSH_SHAPE)?
				&APP_CONF->strUserBrushDir: &APP_CONF->strUserTextureDir);

		mStrPathAdd(&str, path);
	}

	//24bit 読み込み

	img24 = ImageBuf24_loadFile(str.buf);

	mStrFree(&str);

	if(!img24) return NULL;

	//8bit 変換

	if(type == MATERIALIMGLIST_TYPE_BRUSH_SHAPE)
		img8 = ImageBuf8_createFromImageBuf24_forBrush(img24);
	else
		img8 = ImageBuf8_createFromImageBuf24(img24);

	ImageBuf24_free(img24);

	return img8;
}

/** アイテム破棄ハンドラ */

static void _destroy_item(mListItem *p)
{
	ImageBuf8_free(_ITEM(p)->img);
	mFree(_ITEM(p)->path);
}


//=========================
// main
//=========================


/** 解放 */

void MaterialImgList_free()
{
	mListDeleteAll(&g_matimglist.list[0]);
	mListDeleteAll(&g_matimglist.list[1]);
}

/** 初期化 */

void MaterialImgList_init()
{
	mMemzero(&g_matimglist, sizeof(MaterialImgList));
}

/** イメージ取得
 *
 * keep = TRUE にしたブラシ画像の場合は、必ず明示的に MaterialImgList_releaseImage() を実行。
 * 通常のブラシ画像の場合は解放する必要はない。
 *
 * @param keep  イメージを常に保持する (レイヤテクスチャの場合は常に TRUE) */

ImageBuf8 *MaterialImgList_getImage(int type,const char *path,mBool keep)
{
	mList *list;
	_item *pi;
	ImageBuf8 *img8;

	//パスが NULL or 空 => 画像なし

	if(!path || !path[0]) return NULL;

	//ブラシテクスチャでパスが "?" => オプション指定を使う

	if(type == MATERIALIMGLIST_TYPE_BRUSH_TEXTURE
		&& strcmp(path, "?") == 0)
		return NULL;

	//リスト内に同じものがあるか

	list = g_matimglist.list + ((type == MATERIALIMGLIST_TYPE_BRUSH_SHAPE)? _LIST_BRUSH: _LIST_TEXTURE);

	pi = _search_path(list, path);

	//新規追加

	if(!pi)
	{
		//画像読み込み

		img8 = _load_image(type, path);
		if(!img8) return NULL;

		//古いものを削除

		_delete_old(list);

		//リストに追加

		pi = (_item *)mListAppendNew(list, sizeof(_item), _destroy_item);
		if(!pi)
		{
			ImageBuf8_free(img8);
			return NULL;
		}

		pi->img  = img8;
		pi->path = mStrdup(path);
	}

	//アイテムを先頭へ移動

	mListMoveTop(list, M_LISTITEM(pi));

	//常に保持する場合は参照カウンタを +1

	if(type == MATERIALIMGLIST_TYPE_LAYER_TEXTURE || keep)
		pi->refcnt++;

	//[debug]

/*
	_item *pp;

	for(pp = _TOPITEM(list); pp; pp = _NEXTITEM(pp))
		mDebug("%d:%s\n", pp->refcnt, pp->path);

	mDebug("---\n");
*/

	return pi->img;
}

/** 指定イメージをリストから解放
 *
 * 参照カウンタを引く。
 * 参照カウンタが 0 になった場合は、保持数を外れたら自動解放される。 */

void MaterialImgList_releaseImage(int type,ImageBuf8 *img)
{
	_item *pi;

	if(!img) return;

	for(pi = _ITEM(g_matimglist.list[type].top); pi; pi = _NEXTITEM(pi))
	{
		if(pi->img == img)
		{
			if(pi->refcnt) pi->refcnt--;
			break;
		}
	}
}
