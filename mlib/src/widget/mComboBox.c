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
 * mComboBox
 *****************************************/

#include "mDef.h"

#include "mComboBox.h"

#include "mLVItemMan.h"
#include "mListViewPopup.h"

#include "mWidget.h"
#include "mPixbuf.h"
#include "mFont.h"
#include "mList.h"
#include "mStr.h"
#include "mEvent.h"
#include "mSysCol.h"
#include "mKeyDef.h"
#include "mUtilStr.h"
#include "mTrans.h"


//-----------------

#define _FRAME_W      1   //枠太さ
#define _PADDING_X    3   //X余白 (枠は含まない)
#define _PADDING_Y    2   //Y余白 (枠は含まない)
#define _ARROW_W      9

//-----------------


/**********************//**

@defgroup combobox mComboBox
@brief コンボボックス

<h3>継承</h3>
mWidget \> mComboBox

@ingroup group_widget
@{

@file mComboBox.h
@def M_COMBOBOX(p)
@struct mComboBoxData
@struct mComboBox
@enum MCOMBOBOX_NOTIFY

@var MCOMBOBOX_NOTIFY::MCOMBOBOX_N_CHANGESEL
選択が変更された時。\n
param1 : アイテムのポインタ。\n
param2 : アイテムのパラメータ値。

***************************/


//======================
// sub
//======================


/** アイテム高さ取得 */

int __mComboBox_getItemHeight(mComboBox *p)
{
	if(p->cb.itemHeight <= 0)
		return mWidgetGetFontHeight(M_WIDGET(p));
	else
		return p->cb.itemHeight;
}

/** 通知＆再描画 */

static void _notify_update(mComboBox *p)
{
	mWidgetAppendEvent_notify(NULL, M_WIDGET(p),
		MCOMBOBOX_N_CHANGESEL,
		(intptr_t)p->cb.manager->itemFocus,
		(p->cb.manager->itemFocus)->param);

	mWidgetUpdate(M_WIDGET(p));
}

/** 選択を上下に移動 */

static void _selUpDown(mComboBox *p,mBool down)
{
	if(mLVItemMan_updownFocus(p->cb.manager, down))
		_notify_update(p);
}

/** ポップアップ表示 */

static void _popupRun(mComboBox *p)
{
	mListViewItem *sel;
	int num,h,itemh,style = 0;
	mPoint pt;

	num = p->cb.manager->list.num;
	if(num == 0) return;

	sel = p->cb.manager->itemFocus;

	itemh = __mComboBox_getItemHeight(p);

	//高さ

	h = itemh * num;

	if(h > 300)
	{
		h = 300;
		style |= MLISTVIEWPOPUP_S_VERTSCR;
	}

	//位置

	pt.x = 0, pt.y = p->wg.h;

	mWidgetMapPoint(M_WIDGET(p), NULL, &pt);

	//表示

	mListViewPopupRun(M_WIDGET(p), p->cb.manager,
		style, pt.x, pt.y, p->wg.w, h, itemh);

	//選択が変わった場合

	if(sel != p->cb.manager->itemFocus)
		_notify_update(p);
}


//======================
//
//======================


/** 解放処理 */

void mComboBoxDestroyHandle(mWidget *p)
{
	mLVItemMan_free(M_COMBOBOX(p)->cb.manager);
}

/** 作成 */

mComboBox *mComboBoxCreate(mWidget *parent,int id,uint32_t style,
	uint32_t fLayout,uint32_t marginB4)
{
	mComboBox *p;

	p = mComboBoxNew(0, parent, style);
	if(!p) return NULL;

	p->wg.id = id;
	p->wg.fLayout = fLayout;

	mWidgetSetMargin_b4(M_WIDGET(p), marginB4);

	return p;
}

/** 作成 */

mComboBox *mComboBoxNew(int size,mWidget *parent,uint32_t style)
{
	mComboBox *p;
	
	if(size < sizeof(mComboBox)) size = sizeof(mComboBox);
	
	p = (mComboBox *)mWidgetNew(size, parent);
	if(!p) return NULL;
	
	p->wg.destroy = mComboBoxDestroyHandle;
	p->wg.calcHint = mComboBoxCalcHintHandle;
	p->wg.draw = mComboBoxDrawHandle;
	p->wg.event = mComboBoxEventHandle;
	
	p->wg.fState |= MWIDGET_STATE_TAKE_FOCUS;
	p->wg.fEventFilter |= MWIDGET_EVENTFILTER_POINTER | MWIDGET_EVENTFILTER_KEY
		| MWIDGET_EVENTFILTER_SCROLL;

	p->cb.style = style;
	
	//アイテムマネージャ

	p->cb.manager = mLVItemMan_new();

	return p;
}

/** アイテム高さセット */

void mComboBoxSetItemHeight(mComboBox *p,int height)
{
	p->cb.itemHeight = height;
}

/** アイテム追加 */

mListViewItem *mComboBoxAddItem(mComboBox *p,const char *text,intptr_t param)
{
	return mLVItemMan_addItem(p->cb.manager, 0, text, 0, 0, param);
}

/** 確保されたアイテム追加 */

void mComboBoxAddItem_ptr(mComboBox *p,mListViewItem *item)
{
	mListAppend(&p->cb.manager->list, M_LISTITEM(item));
}

/** 描画関数指定してアイテム追加 */

mListViewItem *mComboBoxAddItem_draw(mComboBox *p,const char *text,intptr_t param,
	void (*draw)(mPixbuf *,mListViewItem *,mListViewItemDraw *))
{
	mListViewItem *pi;

	pi = mLVItemMan_addItem(p->cb.manager, 0, text, 0, 0, param);
	if(pi) pi->draw = draw;

	return pi;
}

/** \\t で区切られた文字列から複数アイテム追加 */

void mComboBoxAddItems(mComboBox *p,const char *text,intptr_t paramtop)
{
	const char *top,*end;
	mStr str = MSTR_INIT;

	top = text;

	while(mGetStrNextSplit(&top, &end, '\t'))
	{
		mStrSetTextLen(&str, top, end - top);

		mComboBoxAddItem(p, str.buf, paramtop++);

		top = end;
	}

	mStrFree(&str);
}

/** 文字列IDから複数アイテム追加 */

void mComboBoxAddTrItems(mComboBox *p,int num,int tridtop,intptr_t paramtop)
{
	int i;

	for(i = 0; i < num; i++)
		mComboBoxAddItem(p, M_TR_T(tridtop + i), paramtop + i);
}


//==================

/** アイテムすべて削除 */

void mComboBoxDeleteAllItem(mComboBox *p)
{
	mLVItemMan_deleteAllItem(p->cb.manager);

	mWidgetUpdate(M_WIDGET(p));
}

/** アイテム削除 */

void mComboBoxDeleteItem(mComboBox *p,mListViewItem *item)
{
	if(mLVItemMan_deleteItem(p->cb.manager, item))
		mWidgetUpdate(M_WIDGET(p));
}

/** 番号からアイテム削除
 *
 * @param index 負の値で選択アイテム */

void mComboBoxDeleteItem_index(mComboBox *p,int index)
{
	if(mLVItemMan_deleteItemByIndex(p->cb.manager, index))
		mWidgetUpdate(M_WIDGET(p));
}

/** 現在の選択アイテムを削除し、その前後のアイテムを選択
 *
 * @return 新しく選択されたアイテム */

mListViewItem *mComboBoxDeleteItem_sel(mComboBox *p)
{
	mListViewItem *focus,*sel;

	focus = p->cb.manager->itemFocus;
	if(!focus) return NULL;

	sel = (mListViewItem *)((focus->i.next)? focus->i.next: focus->i.prev);

	mComboBoxDeleteItem(p, focus);
	mComboBoxSetSelItem(p, sel);

	return sel;
}

/** 先頭アイテム取得 */

mListViewItem *mComboBoxGetTopItem(mComboBox *p)
{
	return (mListViewItem *)p->cb.manager->list.top;
}

/** 選択アイテム取得 */

mListViewItem *mComboBoxGetSelItem(mComboBox *p)
{
	return p->cb.manager->itemFocus;
}

/** 位置からアイテム取得
 *
 * @param index 負の値で現在の選択アイテム */

mListViewItem *mComboBoxGetItemByIndex(mComboBox *p,int index)
{
	return mLVItemMan_getItemByIndex(p->cb.manager, index);
}

/** アイテム数取得 */

int mComboBoxGetItemNum(mComboBox *p)
{
	return p->cb.manager->list.num;
}

/** 選択アイテムのインデックス番号取得 */

int mComboBoxGetSelItemIndex(mComboBox *p)
{
	return mLVItemMan_getItemIndex(p->cb.manager, NULL);
}

/** アイテムポインタからインデックス番号取得 */

int mComboBoxGetItemIndex(mComboBox *p,mListViewItem *item)
{
	return mLVItemMan_getItemIndex(p->cb.manager, item);
}

//======================


/** アイテムのテキスト取得 */

void mComboBoxGetItemText(mComboBox *p,int index,mStr *str)
{
	mListViewItem *pi = mComboBoxGetItemByIndex(p, index);

	if(pi)
		mStrSetText(str, pi->text);
	else
		mStrEmpty(str);
}

/** アイテムのパラメータ値取得
 *
 * @param index 負の値で現在の選択アイテム
 * @return なかった場合、0 */

intptr_t mComboBoxGetItemParam(mComboBox *p,int index)
{
	mListViewItem *pi;

	pi = mComboBoxGetItemByIndex(p, index);

	return (pi)? pi->param: 0;
}

/** アイテムのテキスト変更 */

void mComboBoxSetItemText(mComboBox *p,int index,const char *text)
{
	mListViewItem *pi = mComboBoxGetItemByIndex(p, index);

	if(pi)
	{
		mLVItemMan_setText(pi, text);

		if(pi == p->cb.manager->itemFocus)
			mWidgetUpdate(M_WIDGET(p));
	}
}

//======================


/** 選択アイテム変更 (アイテム指定)
 *
 * @param item NULL で選択をなしに */

void mComboBoxSetSelItem(mComboBox *p,mListViewItem *item)
{
	if(mLVItemMan_setFocusItem(p->cb.manager, item))
		mWidgetUpdate(M_WIDGET(p));
}

/** 選択アイテム変更 (番号指定)
 *
 * @param index 負の値で選択なしに */

void mComboBoxSetSel_index(mComboBox *p,int index)
{
	if(mLVItemMan_setFocusItemByIndex(p->cb.manager, index))
		mWidgetUpdate(M_WIDGET(p));
}

/** 指定パラメータ値を検索し、見つかったアイテムを選択
 *
 * @return 選択されたか */

mBool mComboBoxSetSel_findParam(mComboBox *p,intptr_t param)
{
	mListViewItem *pi;

	pi = mLVItemMan_findItemParam(p->cb.manager, param);
	if(pi) mComboBoxSetSelItem(p, pi);

	return (pi != NULL);
}

/** 指定パラメータ値を検索し、見つかったアイテムを選択
 *
 * 見つからなかった場合、notfindindex の位置を選択 */

void mComboBoxSetSel_findParam_notfind(mComboBox *p,intptr_t param,int notfindindex)
{
	if(!mComboBoxSetSel_findParam(p, param))
		mComboBoxSetSel_index(p, notfindindex);
}

/** パラメータ値からアイテム検索
 *
 * @return NULL で見つからなかった */

mListViewItem *mComboBoxFindItemParam(mComboBox *p,intptr_t param)
{
	return mLVItemMan_findItemParam(p->cb.manager, param);
}

/** 全アイテムのテキスト幅から幅セット */

void mComboBoxSetWidthAuto(mComboBox *p)
{
	mListViewItem *pi;
	mFont *font;
	int w,max = 0;

	font = mWidgetGetFont(M_WIDGET(p));

	for(pi = M_LISTVIEWITEM(p->cb.manager->list.top); pi; pi = M_LISTVIEWITEM(pi->i.next))
	{
		w = mFontGetTextWidth(font, pi->text, -1);
		if(w > max) max = w;
	}

	p->wg.hintOverW = max + (_FRAME_W + _PADDING_X) * 2 + 2 + _ARROW_W;
}


//========================
// ハンドラ
//========================


/** サイズ計算 */

void mComboBoxCalcHintHandle(mWidget *p)
{
	//幅

	p->hintW = (_FRAME_W + _PADDING_X) * 2 + _ARROW_W;

	//高さ

	p->hintH = (_FRAME_W + _PADDING_Y) * 2 + __mComboBox_getItemHeight(M_COMBOBOX(p));
	if(p->hintH < 9) p->hintH = 9; 
}

/** イベント */

int mComboBoxEventHandle(mWidget *wg,mEvent *ev)
{
	mComboBox *p = M_COMBOBOX(wg);

	switch(ev->type)
	{
		case MEVENT_POINTER:
			if(ev->pt.type == MEVENT_POINTER_TYPE_PRESS)
			{
				//左ボタン押しでポップアップ表示
				
				if(ev->pt.btt == M_BTT_LEFT)
				{
					mWidgetSetFocus_update(wg, FALSE);
					_popupRun(p);
				}
			}
			break;

		//キー
		case MEVENT_KEYDOWN:
			if(ev->key.code == MKEY_UP)
				_selUpDown(p, FALSE);
			else if(ev->key.code == MKEY_DOWN)
				_selUpDown(p, TRUE);
			else if(ev->key.code == MKEY_SPACE)
				_popupRun(p);
			else
				return FALSE;
			break;

		//ホイール (選択を上下)
		case MEVENT_SCROLL:
			if(ev->scr.dir == MEVENT_SCROLL_DIR_UP
				|| ev->scr.dir == MEVENT_SCROLL_DIR_DOWN)
			{
				_selUpDown(p, (ev->scr.dir == MEVENT_SCROLL_DIR_DOWN));
			}
			break;
		
		case MEVENT_FOCUS:
			mWidgetUpdate(wg);
			break;
		default:
			return FALSE;
	}

	return TRUE;
}

/** 描画 */

void mComboBoxDrawHandle(mWidget *wg,mPixbuf *pixbuf)
{
	mComboBox *p = M_COMBOBOX(wg);
	mFont *font;
	mBool bDisable;
	int n;
	mListViewItem *pi;
	mListViewItemDraw dinfo;
	
	font = mWidgetGetFont(wg);

	bDisable = !(wg->fState & MWIDGET_STATE_ENABLED);

	//ボタン

	n = 0;
	if(wg->fState & MWIDGET_STATE_FOCUSED) n |= MPIXBUF_DRAWBTT_FOCUS;
	if(bDisable) n |= MPIXBUF_DRAWBTT_DISABLE;
	
	mPixbufDrawButton(pixbuf, 0, 0, wg->w, wg->h, n);

	//矢印

	mPixbufDrawArrowDown(pixbuf,
		wg->w - _FRAME_W - _ARROW_W + 2, wg->h / 2,
		(bDisable)? MSYSCOL(TEXT_DISABLE): MSYSCOL(TEXT));

	//アイテム

	pi = p->cb.manager->itemFocus;

	if(pi)
	{
		if(pi->draw)
		{
			//描画関数

			dinfo.widget = wg;
			dinfo.box.x = _FRAME_W + _PADDING_X;
			dinfo.box.y = _FRAME_W + _PADDING_Y;
			dinfo.box.w = wg->w - (_FRAME_W + _PADDING_X) * 2 - _ARROW_W;
			dinfo.box.h = wg->h - (_FRAME_W + _PADDING_Y) * 2;
			dinfo.flags = 0;

			if(!bDisable)
				dinfo.flags |= MLISTVIEWITEMDRAW_F_ENABLED;

			if(wg->fState & MWIDGET_STATE_FOCUSED)
				dinfo.flags |= MLISTVIEWITEMDRAW_F_FOCUSED | MLISTVIEWITEMDRAW_F_SELECTED;

			mPixbufSetClipBox_box(pixbuf, &dinfo.box);
		
			(pi->draw)(pixbuf, pi, &dinfo);
		}
		else
		{
			//テキスト

			mPixbufSetClipBox_d(pixbuf,
				_FRAME_W + _PADDING_X, 0,
				wg->w - (_FRAME_W + _PADDING_X) * 2 - _ARROW_W, wg->h);
			
			mFontDrawText(font, pixbuf,
				_FRAME_W + _PADDING_X, (wg->h - font->height) >> 1,
				pi->text, -1,
				(bDisable)? MSYSCOL_RGB(TEXT_DISABLE): MSYSCOL_RGB(TEXT));
		}
	}
}

/** @} */
