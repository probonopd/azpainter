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
 * mMenu [メニュー]
 *****************************************/

#include <string.h>

#include "mDef.h"

#include "mMenu.h"
#include "mMenu_pv.h"
#include "mMenuWindow.h"

#include "mStr.h"
#include "mList.h"
#include "mTrans.h"


/***********************//**

@defgroup menu mMenu
@brief メニューデータ (一つのポップアップ)

- デフォルトで静的文字列となる。
- 文字列を内部にコピーしたい場合は、\b MMENUITEM_F_LABEL_COPY フラグを ON にする。

<h3>mMenuItemInfo</h3>

| メンバ | 説明 |
| ------ | ---- |
| width | 独自描画時の幅 |
| height | 高さ (独自描画時は高さを指定する) |
| draw | 描画関数 |
| label | ラベル文字列。'&' の次の文字はホットキー (0-9,A-Z)。\n\b MMENUITEM_F_LABEL_COPY フラグが OFF の場合は、指定されたポインタの文字列をそのまま使う (静的文字列)。\n ON の場合は、文字列をコピーして使う。 |

<h3>イベント</h3>
- \b MEVENT_MENU_POPUP\n
サブポップアップメニューが表示されようとしている時。
- \b MEVENT_COMMAND\n
項目が選択された時。param = 項目のパラメータ値

@ingroup group_data
@{

@file mMenu.h
@struct _mMenuItemInfo
@struct mMenuItemDraw
@enum MMENUITEM_FLAGS
@enum MMENUITEM_DRAW_FLAGS
@enum MMENU_POPUP_FLAGS

***************************/


//============================
// sub
//============================


/** アイテム削除時 */

static void _destroy_item(mListItem *pi)
{
	mMenuItem *p = M_MENUITEM(pi);

	//ショートカットキー文字列

	if(p->sctext)
		mFree(p->sctext);

	//ラベル文字列 (複製)

	if(p->labelcopy)
		mFree(p->labelcopy);
	
	//サブメニュー削除
	
	if(p->item.submenu)
		mMenuDestroy(p->item.submenu);
}

/** インデックス位置からアイテム取得 */

static mMenuItem *_getByIndex(mMenu *menu,int index)
{
	return (mMenuItem *)mListGetItemByIndex(&menu->list, index);
}

/** ID からアイテム検索 (サブメニュー以下も含む) */

static mMenuItem *_findByID(mMenu *menu,int id,mMenu **ppOwner)
{
	mMenuItem *p,*psub;
	
	for(p = M_MENUITEM(menu->list.top); p; p = M_MENUITEM(p->i.next))
	{
		if(p->item.id == id)
		{
			if(ppOwner) *ppOwner = menu;
			return p;
		}
		
		if(p->item.submenu)
		{
			psub = _findByID(p->item.submenu, id, ppOwner);
			if(psub) return psub;
		}
	}
	
	return NULL;
}


//============================


/** メニュー削除
 * 
 * 下位のサブメニューもすべて削除される。 */

void mMenuDestroy(mMenu *p)
{
	if(p)
	{
		mListDeleteAll(&p->list);
		
		mFree(p);
	}
}

/** メニュー作成 */

mMenu *mMenuNew(void)
{
	return (mMenu *)mMalloc(sizeof(mMenu), TRUE);
}

/** アイテム個数取得 */

int mMenuGetNum(mMenu *menu)
{
	return menu->list.num;
}

/** 最後のアイテム取得 */

mMenuItemInfo *mMenuGetLastItemInfo(mMenu *menu)
{
	mMenuItem *pi;

	pi = M_MENUITEM(menu->list.bottom);

	return (pi)? &pi->item: NULL;
}

/** 指定IDの項目のサブメニュー取得 */

mMenu *mMenuGetSubMenu(mMenu *menu,int id)
{
	mMenuItem *p = _findByID(menu, id, NULL);

	return (p)? p->item.submenu: NULL;
}

/** インデックス位置からアイテムデータ取得 */

mMenuItemInfo *mMenuGetItemByIndex(mMenu *menu,int index)
{
	mListItem *pi;

	pi = mListGetItemByIndex(&menu->list, index);

	return (pi)? &M_MENUITEM(pi)->item: NULL;
}

/** mMenuItem からアイテム情報のポインタ取得 */

mMenuItemInfo *mMenuGetInfoInItem(mMenuItem *item)
{
	return &item->item;
}

/** メニューの先頭アイテムを mMenuItem で取得 */

mMenuItem *mMenuGetTopItem(mMenu *menu)
{
	return M_MENUITEM(menu->list.top);
}

/** 次のアイテム取得 */

mMenuItem *mMenuGetNextItem(mMenuItem *item)
{
	return M_MENUITEM(item->i.next);
}

/** ポップアップ表示
 * 
 * @param notify イベントの通知先
 * @return 選択されたアイテムのデータのポインタ。NULL で選択されなかった。 */

mMenuItemInfo *mMenuPopup(mMenu *menu,mWidget *notify,
	int x,int y,uint32_t flags)
{
	mMenuWindow *menuwin;
	mMenuItem *item;
	
	if(menu->list.num == 0) return NULL;
	
	menuwin = mMenuWindowNew(menu);
	if(!menuwin) return NULL;
	
	item = mMenuWindowShowPopup(menuwin, notify, x, y, flags);
	
	return (item)? &item->item: NULL;
}

/** 項目をすべて削除
 * 
 * 下位のサブメニューも含む。 */

void mMenuDeleteAll(mMenu *menu)
{
	mListDeleteAll(&menu->list);
}

/** ID からアイテム削除 */

void mMenuDeleteByID(mMenu *menu,int id)
{
	mMenuItem *p;
	mMenu *owner;
	
	p = _findByID(menu, id, &owner);
	if(p) mListDelete(&owner->list, M_LISTITEM(p));
}

/** 位置番号からアイテム削除 */

void mMenuDeleteByIndex(mMenu *menu,int index)
{
	mListDeleteByIndex(&menu->list, index);
}


//========================
// アイテム追加
//========================


/** メニューアイテム追加
 *
 * @return 追加されたアイテムの mMenuItemInfo が返る。直接値をセット可能。 */

mMenuItemInfo *mMenuAdd(mMenu *menu,mMenuItemInfo *info)
{
	mMenuItem *p;

	if(menu->list.num > 10000) return NULL;
	
	p = (mMenuItem *)mListAppendNew(&menu->list, sizeof(mMenuItem), _destroy_item);
	if(!p) return NULL;

	p->item = *info;
	p->fTmp = MMENUITEM_TMPF_INIT_SIZE;

	__mMenuInitText(p, MMENU_INITTEXT_ALL);

	return &p->item;
}

/** アイテム追加:通常 */

mMenuItemInfo *mMenuAddNormal(mMenu *menu,int id,const char *label,uint32_t sckey,uint32_t flags)
{
	mMenuItemInfo info;
	
	memset(&info, 0, sizeof(mMenuItemInfo));
	
	info.id = id;
	info.label = label;
	info.shortcutkey = sckey;
	info.flags = flags;
	
	return mMenuAdd(menu, &info);
}

/** アイテム追加:通常テキスト (静的文字列) */

mMenuItemInfo *mMenuAddText_static(mMenu *menu,int id,const char *label)
{
	return mMenuAddNormal(menu, id, label, 0, 0);
}

/** アイテム追加:通常テキスト (コピー文字列) */

mMenuItemInfo * mMenuAddText_copy(mMenu *menu,int id,const char *label)
{
	return mMenuAddNormal(menu, id, label, 0, MMENUITEM_F_LABEL_COPY);
}

/** アイテム追加:サブメニュー
 *
 * @param label 静的文字列 */

void mMenuAddSubmenu(mMenu *menu,int id,const char *label,mMenu *submenu)
{
	mMenuItemInfo info;
	
	memset(&info, 0, sizeof(mMenuItemInfo));
	
	info.id = id;
	info.label = label;
	info.submenu = submenu;
	
	mMenuAdd(menu, &info);
}

/** セパレータ追加 */

void mMenuAddSep(mMenu *menu)
{
	mMenuItemInfo info;
	
	memset(&info, 0, sizeof(mMenuItemInfo));
	
	info.id = -1;
	info.flags = MMENUITEM_F_SEP;
	
	mMenuAdd(menu, &info);
}

/** チェックアイテム追加 */

void mMenuAddCheck(mMenu *menu,int id,const char *label,mBool checked)
{
	mMenuItemInfo info;
	
	memset(&info, 0, sizeof(mMenuItemInfo));
	
	info.id = id;
	info.label = label;
	info.flags = MMENUITEM_F_AUTOCHECK;
	if(checked) info.flags |= MMENUITEM_F_CHECKED;
	
	mMenuAdd(menu, &info);
}

/** ラジオアイテム追加 */

void mMenuAddRadio(mMenu *menu,int id,const char *label)
{
	mMenuItemInfo info;
	
	memset(&info, 0, sizeof(mMenuItemInfo));
	
	info.id = id;
	info.label = label;
	info.flags = MMENUITEM_F_AUTOCHECK | MMENUITEM_F_RADIO;
	
	mMenuAdd(menu, &info);
}

/** 複数の連続した文字列IDから追加 */

void mMenuAddTrTexts(mMenu *menu,int tridtop,int num)
{
	int i;

	for(i = 0; i < num; i++)
		mMenuAddText_static(menu, tridtop + i, M_TR_T(tridtop + i));
}

/** 文字列IDの配列データから追加
 *
 * 文字列IDは 0x8000 未満の値であること。\n
 * 0xffff で終了、0xfffe でセパレータ、0x8000 が ON で自動チェックタイプ。 */

void mMenuAddTrArray16(mMenu *menu,const uint16_t *buf)
{
	uint16_t val,id;

	while(*buf != 0xffff)
	{
		val = *(buf++);

		if(val == 0xfffe)
			mMenuAddSep(menu);
		else
		{
			id = val & 0x7fff;

			if(val & 0x8000)
				mMenuAddCheck(menu, id, M_TR_T(id), FALSE);
			else
				mMenuAddText_static(menu, id, M_TR_T(id));
		}
	}
}

/** mStr 配列からアイテムをセット
 *
 * すべてのアイテムをクリア後、追加される。\n
 * 空文字列以降はセットされない。 */

void mMenuSetStrArray(mMenu *menu,int idtop,mStr *str,int num)
{
	int i;

	mMenuDeleteAll(menu);

	for(i = 0; i < num; i++)
	{
		if(mStrIsEmpty(str + i)) break;

		mMenuAddText_copy(menu, idtop + i, str[i].buf);
	}
}

/** mStr 配列からアイテムを追加
 *
 * 空文字列以降はセットされない。 */

void mMenuAddStrArray(mMenu *menu,int idtop,mStr *str,int num)
{
	int i;

	for(i = 0; i < num; i++)
	{
		if(mStrIsEmpty(str + i)) break;

		mMenuAddText_copy(menu, idtop + i, str[i].buf);
	}
}


//==========================
// 状態変更
//==========================


/** 有効/無効状態セット
 * 
 * @param type [0]disable [1]enable [2]toggle */

void mMenuSetEnable(mMenu *menu,int id,int type)
{
	mMenuItem *p;
	
	p = _findByID(menu, id, NULL);
	if(!p) return;
	
	if(type == 2)
		type = ((p->item.flags & MMENUITEM_F_DISABLE) != 0);
	
	if(type)
		M_FLAG_OFF(p->item.flags, MMENUITEM_F_DISABLE);
	else
		M_FLAG_ON(p->item.flags, MMENUITEM_F_DISABLE);
}

/** チェック状態セット (サブメニュー以下すべて対象)
 * 
 * @param type [0]OFF [正]ON [負]反転 */

void mMenuSetCheck(mMenu *menu,int id,int type)
{
	mMenuItem *p;
	
	p = _findByID(menu, id, NULL);
	if(!p) return;
	
	//ラジオのチェックOFF
	
	if(p->item.flags & MMENUITEM_F_RADIO)
		__mMenuItemCheckRadio(p, FALSE);
	
	//
	
	if(type < 0)
		type = !(p->item.flags & MMENUITEM_F_CHECKED);
	
	if(type)
		M_FLAG_ON(p->item.flags, MMENUITEM_F_CHECKED);
	else
		M_FLAG_OFF(p->item.flags, MMENUITEM_F_CHECKED);
}

/** サブメニューをセット */

void mMenuSetSubmenu(mMenu *menu,int id,mMenu *submenu)
{
	mMenuItem *p;
	
	p = _findByID(menu, id, NULL);
	if(p) p->item.submenu = submenu;
}

/** ショートカットキーセット */

void mMenuSetShortcutKey(mMenu *menu,int id,uint32_t sckey)
{
	mMenuItem *p = _findByID(menu, id, NULL);

	if(p)
	{
		M_FREE_NULL(p->sctext);
	
		p->item.shortcutkey = sckey;

		__mMenuInitText(p, MMENU_INITTEXT_SHORTCUT);

		M_FLAG_ON(p->fTmp, MMENUITEM_TMPF_INIT_SIZE);
	}
}


//===========================
// 取得
//===========================


/** ラベル文字列を取得 */

const char *mMenuGetTextByIndex(mMenu *menu,int index)
{
	mMenuItem *p = _getByIndex(menu, index);

	return (p)? p->labelsrc: NULL;
}

/** @} */
