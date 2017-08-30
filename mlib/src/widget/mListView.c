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
 * mListView [リストビュー]
 *****************************************/

#include "mDef.h"

#include "mListView.h"
#include "mListViewArea.h"
#include "mLVItemMan.h"
#include "mImageList.h"
#include "mListHeader.h"
#include "mEventList.h"

#include "mWidget.h"
#include "mScrollBar.h"
#include "mEvent.h"
#include "mList.h"
#include "mFont.h"
#include "mStr.h"


//----------------

#define _ITEM_TOP(p)  M_LISTVIEWITEM((p)->lv.manager->list.top)

//----------------


/************//**

@var mListViewData::itemHeight
各アイテムの高さ (任意の値を使う場合)。\n
0 以下でテキストの高さとなる。デフォルト = 0。

@var mListViewData::itemH
実際の各アイテムの高さ (内部で計算される)

*****************/


/************//**

@defgroup listview mListView
@brief リストビュー

<h3>継承</h3>
mWidget \> mScrollView \> mListView

<h3>使い方</h3>
- 各アイテムの高さを指定したい場合は、itemHeight に値をセットする (0 以下でテキストの高さとなる)
- 単一列の場合にエリアの幅を指定したい場合は、width_single に幅の値をセットする (width_single は、0 以下で水平スクロールバーを表示しない)
- 複数列の場合、テキストはタブで区切る。

@ingroup group_widget
@{

@file mListView.h
@file mListViewItem.h
@def M_LISTVIEW(p)
@struct mListView
@struct mListViewData
@enum MLISTVIEW_STYLE
@enum MLISTVIEW_NOTIFY

@var MLISTVIEW_STYLE::MLISTVIEW_S_MULTI_COLUMN
複数列にする

@var MLISTVIEW_STYLE::MLISTVIEW_S_NO_HEADER
複数列の場合、ヘッダを表示しない

@var MLISTVIEW_STYLE::MLISTVIEW_S_CHECKBOX
各項目にチェックボックスを付ける

@var MLISTVIEW_STYLE::MLISTVIEW_S_GRID_ROW
項目ごとのグリッド線を表示

@var MLISTVIEW_STYLE::MLISTVIEW_S_GRID_COL
列ごとのグリッド線を表示

@var MLISTVIEW_STYLE::MLISTVIEW_S_MULTI_SEL
複数選択を有効にする

@var MLISTVIEW_STYLE::MLISTVIEW_S_AUTO_WIDTH
アイテム変更時に自動でスクロール水平幅を計算する

@var MFILELISTVIEW_STYLE::MLISTVIEW_S_DESTROY_IMAGELIST
ウィジェット破棄時、イメージリストも削除


@var MLISTVIEW_NOTIFY::MLISTVIEW_N_CHANGE_FOCUS
フォーカスアイテムの選択が変わった時。選択解除時は来ない。\n
param1 : アイテムのポインタ。\n
param2 : アイテムのパラメータ値

@var MLISTVIEW_NOTIFY::MLISTVIEW_N_CLICK_ON_FOCUS
フォーカスアイテム上がクリックされた時。
param1 : アイテムのポインタ。\n
param2 : アイテムのパラメータ値

@var MLISTVIEW_NOTIFY::MLISTVIEW_N_ITEM_CHECK
アイテムのチェックボックスが変更された時。\n
param1 : アイテムのポインタ\n
param2 : アイテムのパラメータ値

@var MLISTVIEW_NOTIFY::MLISTVIEW_N_ITEM_DBLCLK
アイテムがダブルクリックされた時。\n
param1 : アイテムのポインタ。\n
param2 : アイテムのパラメータ値

@var MLISTVIEW_NOTIFY::MLISTVIEW_N_ITEM_RCLK
アイテムが右クリックされた時。\n
param1 : アイテムのポインタ。NULL でアイテムがない部分がクリックされた。

*****************/
	

//============================
// sub
//============================


/** 通常の一つのアイテムの高さ取得 */

static int _getItemHeight(mListView *p)
{
	int h;
	
	h = p->lv.itemHeight;

	//0 以下でフォントの高さ

	if(h <= 0)
		h = mWidgetGetFontHeight(M_WIDGET(p)) + 1;

	//チェックボックスがある場合
	
	if((p->lv.style & MLISTVIEW_S_CHECKBOX) && h < MLISTVIEW_DRAW_CHECKBOX_SIZE + 2)
		h = MLISTVIEW_DRAW_CHECKBOX_SIZE + 2;

	//アイコン

	if(p->lv.iconimg && h < p->lv.iconimg->h)
		h = p->lv.iconimg->h;
	
	return h;
}

/** アイテムすべての高さ取得 */

static int _getItemAllHeight(mListView *p)
{
	return p->lv.itemH * p->lv.manager->list.num;
}

/** スクロール情報セット */

static void _setScrollStatus(mListView *p)
{
	mListViewArea *lva = M_LISTVIEWAREA(p->sv.area);
	int max;

	//水平バー

	if(p->sv.scrh)
	{
		if(lva->lva.header)
			//複数列: ヘッダの幅
			max = mListHeaderGetFullWidth(lva->lva.header);
		else if(p->lv.width_single > 0)
			//単一列: 幅指定がある場合
			max = p->lv.width_single;
		else
			//単一列: 水平バーは表示しない
			max = -1;
	
		if(max >= 0)
			mScrollBarSetStatus(p->sv.scrh, 0, max, lva->wg.w);

		//ヘッダスクロール位置

		if(lva->lva.header)
			mListHeaderSetScrollPos(lva->lva.header, mScrollBarGetPos(p->sv.scrh));
	}

	//垂直バー

	if(p->sv.scrv)
		mScrollBarSetStatus(p->sv.scrv, 0, _getItemAllHeight(p), lva->wg.h - lva->lva.headerH);
}

/** mListViewArea のサイズ変更時 */

static void _area_onsize_handle(mWidget *wg)
{
	_setScrollStatus(M_LISTVIEW(wg->parent));
}

/** mListViewArea のスクロール表示判定 */

static mBool _area_isBarVisible(mScrollViewArea *area,int size,mBool horz)
{
	mListView *p = M_LISTVIEW(area->wg.parent);
	mListViewArea *lva = M_LISTVIEWAREA(p->sv.area);

	if(horz)
	{
		//水平

		if(lva->lva.header)
		{
			//複数列

			return (size < mListHeaderGetFullWidth(lva->lva.header));
		}
		else
		{
			//単一列の場合は、幅指定があればその幅で判定。なければ表示しない
			
			if(p->lv.width_single > 0)
				return (size < p->lv.width_single);
			else
				return FALSE;
		}
	}
	else
	{
		//垂直
		
		return (size < _getItemAllHeight(p) + lva->lva.headerH);
	}
}

/** CONSTRUCT イベント追加 */

static void _send_const(mListView *p)
{
	mWidgetAppendEvent_only(M_WIDGET(p), MEVENT_CONSTRUCT);
}

/** 再構成 */

static void _run_construct(mListView *p)
{
	if(p->wg.fUI & MWIDGET_UI_LAYOUTED)
	{
		//スクロール幅自動

		if(p->lv.style & MLISTVIEW_S_AUTO_WIDTH)
			mListViewSetWidthAuto(p, FALSE);
		
		//
	
		_setScrollStatus(p);
		mScrollViewConstruct(M_SCROLLVIEW(p));
		mWidgetUpdate(M_WIDGET(p->sv.area));
	}
}


//============================
// main
//============================


/** 解放処理 */

void mListViewDestroyHandle(mWidget *wg)
{
	mListView *p = M_LISTVIEW(wg);

	mLVItemMan_free(p->lv.manager);

	if(p->lv.style & MLISTVIEW_S_DESTROY_IMAGELIST)
		mImageListFree(p->lv.iconimg);
}

/** 作成
 *
 * @param scrv_style mScrollView スタイル */

mListView *mListViewNew(int size,mWidget *parent,
	uint32_t style,uint32_t scrv_style)
{
	mListView *p;
	mListViewArea *area;
	
	if(size < sizeof(mListView)) size = sizeof(mListView);
	
	p = (mListView *)mScrollViewNew(size, parent, scrv_style);
	if(!p) return NULL;
	
	p->wg.destroy = mListViewDestroyHandle;
	p->wg.calcHint = mListViewCalcHintHandle;
	p->wg.event = mListViewEventHandle;
	
	p->wg.fState |= MWIDGET_STATE_TAKE_FOCUS;
	p->wg.fEventFilter |= MWIDGET_EVENTFILTER_KEY;

	p->lv.style = style;

	//アイテムマネージャ

	p->lv.manager = mLVItemMan_new();

	p->lv.manager->bMultiSel = ((style & MLISTVIEW_S_MULTI_SEL) != 0);
	
	//mListViewArea
	
	area = mListViewAreaNew(0, M_WIDGET(p), 0, style, p->lv.manager, M_WIDGET(p));

	p->sv.area = M_SCROLLVIEWAREA(area);

	area->wg.onSize = _area_onsize_handle;
	area->sva.isBarVisible = _area_isBarVisible;

	return p;
}

/** アイコンのイメージリストセット */

void mListViewSetImageList(mListView *p,mImageList *img)
{
	p->lv.iconimg = img;

	mListViewArea_setImageList(M_LISTVIEWAREA(p->sv.area), img);
}

/** ヘッダのウィジェットポインタ取得 */

mListHeader *mListViewGetHeader(mListView *p)
{
	return M_LISTVIEWAREA(p->sv.area)->lva.header;
}

/** 通常テキスト時の一つのアイテムの高さ取得 */

int mListViewGetItemNormalHeight(mListView *p)
{
	return _getItemHeight(p);
}

/** 各列の余白幅を取得 (左右全体) */

int mListViewGetColumnMarginWidth()
{
	return MLISTVIEW_DRAW_ITEM_MARGIN * 2;
}


//=====================
// 取得
//=====================


/** 位置からアイテム取得
 *
 * @param index 負の値で現在のフォーカスアイテム */

mListViewItem *mListViewGetItemByIndex(mListView *p,int index)
{
	return mLVItemMan_getItemByIndex(p->lv.manager, index);
}

/** フォーカスアイテム取得 */

mListViewItem *mListViewGetFocusItem(mListView *p)
{
	return (p->lv.manager)->itemFocus;
}

/** 先頭アイテム取得 */

mListViewItem *mListViewGetTopItem(mListView *p)
{
	return (mListViewItem *)((p->lv.manager)->list.top);
}

/** アイテム数取得 */

int mListViewGetItemNum(mListView *p)
{
	return (p->lv.manager)->list.num;
}

/** アイテムすべて削除 */

void mListViewDeleteAllItem(mListView *p)
{
	mLVItemMan_deleteAllItem(p->lv.manager);

	_send_const(p);
}

/** アイテム削除 */

void mListViewDeleteItem(mListView *p,mListViewItem *item)
{
	if(item)
	{
		mLVItemMan_deleteItem(p->lv.manager, item);
		
		_send_const(p);
	}
}

/** アイテムを削除後、その上下のアイテムを選択する
 *
 * @param item  NULL で現在のフォーカス
 * @return 選択されたアイテム */

mListViewItem *mListViewDeleteItem_sel(mListView *p,mListViewItem *item)
{
	mListViewItem *sel;

	if(!item)
	{
		item = (p->lv.manager)->itemFocus;
		if(!item) return NULL;
	}

	sel = (mListViewItem *)((item->i.next)? item->i.next: item->i.prev);

	mListViewDeleteItem(p, item);
	if(sel) mListViewSetFocusItem(p, sel);

	return sel;
}


//==================
// アイテム追加
//==================


/** アイテム追加 */

mListViewItem *mListViewAddItem(mListView *p,
	const char *text,int icon,uint32_t flags,intptr_t param)
{
	_send_const(p);

	return mLVItemMan_addItem(p->lv.manager, 0, text, icon, flags, param);
}

/** アイテム追加: アイテムのサイズを指定 */

mListViewItem *mListViewAddItem_ex(mListView *p,int size,
	const char *text,int icon,uint32_t flags,intptr_t param)
{
	_send_const(p);

	return mLVItemMan_addItem(p->lv.manager, size, text, icon, flags, param);
}

/** アイテム追加: テキストのみ */

mListViewItem *mListViewAddItemText(mListView *p,const char *text)
{
	return mListViewAddItem(p, text, -1, 0, 0);
}

/** アイテム追加: テキストとパラメータ値 */

mListViewItem *mListViewAddItem_textparam(mListView *p,const char *text,intptr_t param)
{
	return mListViewAddItem(p, text, -1, 0, param);
}

/** アイテム挿入 */

mListViewItem *mListViewInsertItem(mListView *p,mListViewItem *ins,
	const char *text,int icon,uint32_t flags,intptr_t param)
{
	_send_const(p);

	return mLVItemMan_insertItem(p->lv.manager, ins, text, icon, flags, param);
}


//==================
// ほか
//==================


/** アイテムポインタからインデックス番号取得
 *
 * @param pi  NULL で現在のフォーカス */

int mListViewGetItemIndex(mListView *p,mListViewItem *pi)
{
	return mLVItemMan_getItemIndex(p->lv.manager, pi);
}

/** アイテムのパラメータ値取得 */

intptr_t mListViewGetItemParam(mListView *p,int index)
{
	mListViewItem *pi = mListViewGetItemByIndex(p, index);

	return (pi)? pi->param: 0;
}

/** 指定アイテムの指定列のテキスト取得 */

void mListViewGetItemColumnText(mListView *p,mListViewItem *pi,int index,mStr *str)
{
	mStrGetSplitText(str, pi->text, '\t', index);
}

/** パラメータ値からアイテム検索 */

mListViewItem *mListViewFindItemByParam(mListView *p,intptr_t param)
{
	return mLVItemMan_findItemParam(p->lv.manager, param);
}

/** テキストからアイテム検索 */

mListViewItem *mListViewFindItemByText(mListView *p,const char *text)
{
	return mLVItemMan_findItemText(p->lv.manager, text);
}

/** フォーカスアイテム変更 */

void mListViewSetFocusItem(mListView *p,mListViewItem *pi)
{
	if(mLVItemMan_setFocusItem(p->lv.manager, pi))
		mWidgetUpdate(M_WIDGET(p->sv.area));
}

/** フォーカスアイテム変更 (インデックス番号から) */

mListViewItem *mListViewSetFocusItem_index(mListView *p,int index)
{
	if(mLVItemMan_setFocusItemByIndex(p->lv.manager, index))
		mWidgetUpdate(M_WIDGET(p->sv.area));

	return (p->lv.manager)->itemFocus;
}

/** フォーカスアイテム変更 (パラメータ値から) */

mListViewItem *mListViewSetFocusItem_findParam(mListView *p,intptr_t param)
{
	mListViewItem *pi;

	pi = mLVItemMan_findItemParam(p->lv.manager, param);
	if(pi)
	{
		if(mLVItemMan_setFocusItem(p->lv.manager, pi))
			mWidgetUpdate(M_WIDGET(p->sv.area));
	}

	return pi;
}

/** フォーカスアイテム変更 (テキストから) */

mListViewItem *mListViewSetFocusItem_findText(mListView *p,const char *text)
{
	mListViewItem *pi;

	pi = mLVItemMan_findItemText(p->lv.manager, text);
	if(pi)
	{
		if(mLVItemMan_setFocusItem(p->lv.manager, pi))
			mWidgetUpdate(M_WIDGET(p->sv.area));
	}

	return pi;
}


//==================
// アイテム操作
//==================


/** アイテムのテキストをセット
 *
 * @param pi  NULL でフォーカスアイテム */

void mListViewSetItemText(mListView *p,mListViewItem *pi,const char *text)
{
	if(!pi)
	{
		pi = (p->lv.manager)->itemFocus;
		if(!pi) return;
	}

	mLVItemMan_setText(pi, text);

	if(p->lv.style & MLISTVIEW_S_AUTO_WIDTH)
		_send_const(p);
	else
		mWidgetUpdate(M_WIDGET(p->sv.area));
}

/** アイテムを上下に移動
 *
 * @param item  NULL でフォーカスアイテム */

mBool mListViewMoveItem_updown(mListView *p,mListViewItem *item,mBool down)
{
	if(mLVItemMan_moveItem_updown(p->lv.manager, item, down))
	{
		mWidgetUpdate(M_WIDGET(p->sv.area));
		return TRUE;
	}
	else
		return FALSE;
}

/** アイテムの位置を移動
 *
 * @param insert 挿入位置。NULL で終端へ */

mBool mListViewMoveItem(mListView *p,mListViewItem *item,mListViewItem *insert)
{
	if(mListMove(&p->lv.manager->list, M_LISTITEM(item), M_LISTITEM(insert)))
	{
		mWidgetUpdate(M_WIDGET(p->sv.area));
		return TRUE;
	}
	else
		return FALSE;
}

/** アイテムをソート */

void mListViewSortItem(mListView *p,
	int (*comp)(mListItem *,mListItem *,intptr_t),intptr_t param)
{
	mListSort(&p->lv.manager->list, comp, param);

	mWidgetUpdate(M_WIDGET(p->sv.area));
}

/** アイテムのテキスト幅から自動でスクロール水平幅セット (単一列時)
 *
 * @param bHint 推奨サイズにセット */

void mListViewSetWidthAuto(mListView *p,mBool bHint)
{
	mListViewItem *pi;
	mFont *font;
	int w,maxw = 0;

	font = mWidgetGetFont(M_WIDGET(p));

	//スクロール幅セット

	for(pi = _ITEM_TOP(p); pi; pi = M_LISTVIEWITEM(pi->i.next))
	{
		if(pi->text)
		{
			w = mFontGetTextWidth(font, pi->text, -1);

			if(w > maxw) maxw = w;
		}
	}

	maxw += MLISTVIEW_DRAW_ITEM_MARGIN * 2;

	if(p->lv.style & MLISTVIEW_S_CHECKBOX)
		maxw += MLISTVIEW_DRAW_CHECKBOX_SIZE + MLISTVIEW_DRAW_CHECKBOX_SPACE;

	if(p->lv.iconimg)
		maxw += p->lv.iconimg->eachw + MLISTVIEW_DRAW_ICON_SPACE;

	p->lv.width_single = maxw;

	//推奨サイズセット

	if(bHint)
	{
		if(p->sv.style & MSCROLLVIEW_S_FRAME) maxw += 2;
		if(p->sv.style & MSCROLLVIEW_S_VERT) maxw += MSCROLLBAR_WIDTH;
	
		p->wg.hintOverW = maxw;
	}

	//再構成

	if(!(p->lv.style & MLISTVIEW_S_AUTO_WIDTH))
		_send_const(p);
}

/** アイテムの位置を基準として垂直スクロール
 *
 * @param align [0]上端 [1]中央 */

void mListViewScrollToItem(mListView *p,mListViewItem *pi,int align)
{
	int y;

	if(p->sv.scrv)
	{
		if(p->lv.itemH == 0)
			p->lv.itemH = _getItemHeight(p);
	
		y = mLVItemMan_getItemIndex(p->lv.manager, pi) * p->lv.itemH;

		if(align == 1)
		{
			mListViewArea *lva = M_LISTVIEWAREA(p->sv.area);
			
			y -= (lva->wg.h - lva->lva.headerH - p->lv.itemH) / 2;
		}

		_setScrollStatus(p);

		if(mScrollBarSetPos(p->sv.scrv, y))
			mWidgetUpdate(M_WIDGET(p->sv.area));
	}
}

/** フォーカスアイテムがスクロールで隠れている場合、スクロール位置調整
 *
 * 上下に隠れている場合、フォーカスが先頭になるように。
 * 
 * @param margin_num 上下に余白を付けたい場合、余白のアイテム数 */

void mListViewAdjustScrollByFocusItem(mListView *p,int margin_num)
{
	/* 新規追加アイテムをフォーカスにした直後の場合、
	 * スクロール情報が更新されていないので、イベントがあればここで行う。 */

	if(mEventListDeleteLastEvent(M_WIDGET(p), MEVENT_CONSTRUCT))
		_run_construct(p);
	
	mListViewArea_scrollToFocus((mListViewArea *)p->sv.area, 0, margin_num);
}


//========================
// ハンドラ
//========================


/** サイズ計算 */

void mListViewCalcHintHandle(mWidget *wg)
{
	mListView *p = M_LISTVIEW(wg);

	//アイテム高さ (mListViewArea にもセット)

	p->lv.itemH = _getItemHeight(p);
	
	M_LISTVIEWAREA(p->sv.area)->lva.itemH = p->lv.itemH;
}

/** イベント */

int mListViewEventHandle(mWidget *wg,mEvent *ev)
{
	mListView *p = M_LISTVIEW(wg);

	switch(ev->type)
	{
		//mListViewArea へ送る
		case MEVENT_KEYDOWN:
			return ((p->sv.area)->wg.event)(M_WIDGET(p->sv.area), ev);

		//構成
		case MEVENT_CONSTRUCT:
			_run_construct(p);
			break;
		
		case MEVENT_FOCUS:
			mWidgetUpdate(M_WIDGET(p->sv.area));
			break;
		default:
			return FALSE;
	}

	return TRUE;
}

/** @} */
