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
 * mTab
 *****************************************/

#include "mDef.h"

#include "mTab.h"

#include "mWidget.h"
#include "mPixbuf.h"
#include "mFont.h"
#include "mEvent.h"
#include "mSysCol.h"
#include "mKeyDef.h"
#include "mList.h"


//--------------------

#define _PADDING_TABX  4
#define _PADDING_TABY  3
#define _MINSPACE      12
#define _MINSPACE_VERT 14

#define _IS_VERTTAB(p)  (p->tab.style & (MTAB_S_LEFT | MTAB_S_RIGHT))
#define _IS_HAVE_SEP(p) (p->tab.style & MTAB_S_HAVE_SEP)
#define _IS_FIT_SIZE(p) (p->tab.style & MTAB_S_FIT_SIZE)

//--------------------


/**********************//**

@defgroup tab mTab
@brief タブウィジェット

<h3>継承</h3>
mWidget \> mTab

@ingroup group_widget
@{

@file mTab.h
@def M_TAB(p)
@def M_TABITEM(p)
@struct mTab
@struct mTabData
@struct mTabItem
@enum MTAB_STYLE
@enum MTAB_NOTIFY

@var MTAB_STYLE::MTAB_S_FIT_SIZE
表示されないタブがある場合、全体が幅にフィットするようにタブを詰める

@var MTAB_NOTIFY::MTAB_N_CHANGESEL
タブの選択が変更された時。\n
param1 : インデックス番号\n
param2 : アイテムポインタ

***************************/


//========================
// sub
//========================


/** 各タブの位置を計算 (水平) */

static void _calc_tabpos_horz(mTab *p)
{
	mTabItem *pi,*pi2,*pitop;
	int pos,max,remain,flag,f;

	//通常位置をセット

	pos = 0, max = 1;

	for(pi = M_TABITEM(p->tab.list.top); pi; pi = M_TABITEM(pi->i.next))
	{
		pi->pos = pos;
		pos += pi->w - 1;
		max += pi->w - 1;
	}

	//詰めるか

	if(!_IS_FIT_SIZE(p)) return;

	remain = max - p->wg.w;
	if(remain <= 0) return;

	//------ 詰める

	//先頭から2番目のアイテムから開始
	
	pitop = (mTabItem *)mListGetItemByIndex(&p->tab.list, 1);
	flag = TRUE;

	while(remain > 0 && flag)
	{
		for(pi = pitop, flag = FALSE; pi && remain; pi = M_TABITEM(pi->i.next))
		{
			pi2 = M_TABITEM(pi->i.prev);

			//左のタブの左端〜現在のタブの左端との間に隙間があるか

			if(pi->pos - pi2->pos <= _MINSPACE) continue;

			//現在のタブの右端と全左側タブの右端との間に隙間があるか

			pos = pi->pos + pi->w;

			for(f = 0; pi2; pi2 = M_TABITEM(pi2->i.prev))
			{
				if(pos - pi2->pos - pi2->w <= _MINSPACE)
				{
					f = 1;
					break;
				}
			}

			if(f) continue;

			//隙間があれば、右側のタブ位置をずらす

			for(pi2 = pi; pi2; pi2 = M_TABITEM(pi2->i.next))
				pi2->pos--;

			remain--;
			flag = TRUE;
		}
	}
}

/** 各タブの位置を計算 (垂直) */

static void _calc_tabpos_vert(mTab *p)
{
	mTabItem *pi,*pi2,*pitop;
	int pos,max,remain,itemh,n,i;

	itemh = mWidgetGetFontHeight(M_WIDGET(p)) + _PADDING_TABY * 2;

	//通常位置をセット

	pos = 0, max = 1;

	for(pi = M_TABITEM(p->tab.list.top); pi; pi = M_TABITEM(pi->i.next))
	{
		pi->pos = pos;
		pos += itemh - 1;
		max += itemh - 1;
	}

	//詰めるか

	if(p->tab.list.num < 2 || !_IS_FIT_SIZE(p)) return;

	remain = max - p->wg.h;
	if(remain <= 0) return;

	//------ 詰める

	pitop = (mTabItem *)mListGetItemByIndex(&p->tab.list, 1);

	//各タブのずらせる最大幅

	max = itemh - _MINSPACE_VERT;
	if(max <= 0) return;

	//各タブを均等にずらす

	n = remain / (p->tab.list.num - 1);
	if(n > max) n = max;

	for(pi = pitop, i = n; pi; pi = M_TABITEM(pi->i.next), i += n)
		pi->pos -= i;

	//残り

	remain -= n * (p->tab.list.num - 1);

	if(remain && n < max)
	{
		for(pi = pitop; pi && remain > 0; pi = M_TABITEM(pi->i.next))
		{
			for(pi2 = pi; pi2; pi2 = M_TABITEM(pi2->i.next))
				pi2->pos--;
		
			remain--;
		}
	}
}

/** 選択アイテム変更 */

static void _changeSel(mTab *p,mTabItem *pi)
{
	if(pi && pi != p->tab.focusitem)
	{
		p->tab.focusitem = pi;

		mWidgetUpdate(M_WIDGET(p));

		//通知

		mWidgetAppendEvent_notify(NULL, M_WIDGET(p),
			MTAB_N_CHANGESEL,
			mListGetItemIndex(&p->tab.list, M_LISTITEM(pi)),
			(intptr_t)pi);
	}
}

/** 前後のアイテム取得 */

static mTabItem *_getNextPrevItem(mTabItem *pi,mBool next)
{
	return (mTabItem *)(next? pi->i.next: pi->i.prev);
}

/** カーソル位置からアイテム取得 */

static mTabItem *_getItemByPos(mTab *p,int x,int y)
{
	mTabItem *pi,*focus;
	mBool bnext;
	int itemh;

	focus = p->tab.focusitem;

	if(!focus) return NULL;

	if(_IS_VERTTAB(p))
	{
		//------ 垂直

		itemh = mWidgetGetFontHeight(M_WIDGET(p)) + _PADDING_TABY * 2;

		//フォーカスアイテムを優先

		if(focus->pos <= y && y < focus->pos + itemh)
			return focus;

		//フォーカスより上または下

		bnext = (y > focus->pos);

		for(pi = _getNextPrevItem(focus, bnext); pi; pi = _getNextPrevItem(pi, bnext))
		{
			if(pi->pos <= y && y < pi->pos + itemh)
				return pi;
		}
	}
	else
	{
		//------ 水平

		//フォーカスアイテムを優先

		if(focus->pos <= x && x < focus->pos + focus->w)
			return focus;

		//フォーカスより左または右

		bnext = (x > focus->pos);

		for(pi = _getNextPrevItem(focus, bnext); pi; pi = _getNextPrevItem(pi, bnext))
		{
			if(pi->pos <= x && x < pi->pos + pi->w)
				return pi;
		}
	}

	return NULL;
}

/** 最大幅取得 */

static int _getMaxWidth(mTab *p)
{
	mTabItem *pi;
	int max = 0;

	for(pi = M_TABITEM(p->tab.list.top); pi; pi = M_TABITEM(pi->i.next))
	{
		if(max < pi->w) max = pi->w;
	}

	return max;
}

/** アイテム削除ハンドラ */

static void _item_destroy(mListItem *p)
{
	mFree(M_TABITEM(p)->label);
}


//========================
// メイン
//========================


/** 解放処理 */

void mTabDestroyHandle(mWidget *wg)
{
	mListDeleteAll(&M_TAB(wg)->tab.list);
}

/** 作成 */

mTab *mTabCreate(mWidget *parent,int id,uint32_t style,uint32_t fLayout,uint32_t marginB4)
{
	mTab *p;

	p = mTabNew(0, parent, style);
	if(p)
	{
		p->wg.id = id;
		p->wg.fLayout = fLayout;

		mWidgetSetMargin_b4(M_WIDGET(p), marginB4);
	}

	return p;
}

/** 作成 */

mTab *mTabNew(int size,mWidget *parent,uint32_t style)
{
	mTab *p;
	
	if(size < sizeof(mTab)) size = sizeof(mTab);
	
	p = (mTab *)mWidgetNew(size, parent);
	if(!p) return NULL;
	
	p->wg.destroy = mTabDestroyHandle;
	p->wg.calcHint = mTabCalcHintHandle;
	p->wg.draw = mTabDrawHandle;
	p->wg.event = mTabEventHandle;
	p->wg.onSize = mTabOnsizeHandle;
	
	p->wg.fState |= MWIDGET_STATE_TAKE_FOCUS;
	p->wg.fEventFilter |= MWIDGET_EVENTFILTER_POINTER | MWIDGET_EVENTFILTER_KEY;

	p->tab.style = style;
	
	return p;
}

/** アイテム追加
 *
 * @param w タブ幅。負の値で自動 */

void mTabAddItem(mTab *p,const char *label,int w,intptr_t param)
{
	mTabItem *pi;

	//幅

	if(w < 0)
		w = mFontGetTextWidth(mWidgetGetFont(M_WIDGET(p)), label, -1) + _PADDING_TABX * 2;

	//追加

	pi = (mTabItem *)mListAppendNew(&p->tab.list, sizeof(mTabItem), _item_destroy);
	if(pi)
	{
		pi->label = mStrdup(label);
		pi->param = param;
		pi->w = w;

		mWidgetAppendEvent_only(M_WIDGET(p), MEVENT_CONSTRUCT);
	}
}

/** アイテム追加:ラベルのみ */

void mTabAddItemText(mTab *p,const char *label)
{
	mTabAddItem(p, label, -1, 0);
}

/** インデックス位置からタブ削除 */

void mTabDelItem_index(mTab *p,int index)
{
	mTabItem *pi = mTabGetItemByIndex(p, index);

	if(pi)
	{
		mListDelete(&p->tab.list, M_LISTITEM(pi));

		if(pi == p->tab.focusitem)
			p->tab.focusitem = M_TABITEM(p->tab.list.top);

		mWidgetAppendEvent_only(M_WIDGET(p), MEVENT_CONSTRUCT);
	}
}

/** インデックス位置からタブアイテム取得
 *
 * @param index 負の値で現在の選択 */

mTabItem *mTabGetItemByIndex(mTab *p,int index)
{
	if(index < 0)
		return p->tab.focusitem;
	else
		return (mTabItem *)mListGetItemByIndex(&p->tab.list, index);
}

/** 現在の選択タブの位置を取得
 *
 * @return -1 でなし */

int mTabGetSelItem_index(mTab *p)
{
	return mListGetItemIndex(&p->tab.list, M_LISTITEM(p->tab.focusitem));
}

/** インデックス位置からアイテムのパラメータ値取得 */

intptr_t mTabGetItemParam_index(mTab *p,int index)
{
	mTabItem *pi = mTabGetItemByIndex(p, index);

	return (pi)? pi->param: 0;
}

/** 選択タブセット */

void mTabSetSel_index(mTab *p,int index)
{
	mTabItem *pi = mTabGetItemByIndex(p, index);

	if(pi != p->tab.focusitem)
	{
		p->tab.focusitem = pi;

		mWidgetUpdate(M_WIDGET(p));
	}
}

/** アイテム数から推奨サイズセット
 *
 * 垂直タブなら高さをセット。 */

void mTabSetHintSize_byItems(mTab *p)
{
	p->wg.hintH = (mWidgetGetFontHeight(M_WIDGET(p)) + _PADDING_TABY * 2 - 1) * p->tab.list.num + 1;
}


//========================
// ハンドラ
//========================


/** サイズ計算 */

void mTabCalcHintHandle(mWidget *wg)
{
	mTab *p = M_TAB(wg);

	if(_IS_VERTTAB(p))
	{
		//垂直

		wg->hintW = _getMaxWidth(p) + 1; //フォーカスタブ用に +1

		if(_IS_HAVE_SEP(p)) wg->hintW++;
	}
	else
	{
		//水平
		
		wg->hintH = mWidgetGetFontHeight(wg) + _PADDING_TABY * 2;

		if(_IS_HAVE_SEP(p)) wg->hintH++;
	}
}

/** サイズ変更時ハンドラ */

void mTabOnsizeHandle(mWidget *wg)
{
	mTab *p = M_TAB(wg);

	if(_IS_FIT_SIZE(p))
	{
		if(_IS_VERTTAB(p))
			_calc_tabpos_vert(p);
		else
			_calc_tabpos_horz(p);
	}
}

/** キー押し時 */

static void _event_key(mTab *p,mEvent *ev)
{
	int dir = 0;
	mListItem *pi;

	if(!p->tab.focusitem) return;

	//移動方向

	if(_IS_VERTTAB(p))
	{
		if(ev->key.code == MKEY_UP)
			dir = -1;
		else if(ev->key.code == MKEY_DOWN)
			dir = 1;
	}
	else
	{
		if(ev->key.code == MKEY_LEFT)
			dir = -1;
		else if(ev->key.code == MKEY_RIGHT)
			dir = 1;
	}

	if(dir == 0) return;

	//移動

	pi = M_LISTITEM(p->tab.focusitem);
	pi = (dir < 0)? pi->prev: pi->next;

	if(pi) _changeSel(p, M_TABITEM(pi));
}

/** イベント */

int mTabEventHandle(mWidget *wg,mEvent *ev)
{
	mTab *p = M_TAB(wg);

	switch(ev->type)
	{
		case MEVENT_POINTER:
			if(ev->pt.type == MEVENT_POINTER_TYPE_PRESS
				&& ev->pt.type == M_BTT_LEFT)
			{
				//フォーカスセット

				mWidgetSetFocus_update(M_WIDGET(p), FALSE);

				//選択アイテム変更

				_changeSel(p, _getItemByPos(p, ev->pt.x, ev->pt.y));
			}
			break;

		//キー
		case MEVENT_KEYDOWN:
			_event_key(p, ev);
			break;

		//再構成
		case MEVENT_CONSTRUCT:
			if(_IS_VERTTAB(p))
				_calc_tabpos_vert(p);
			else
				_calc_tabpos_horz(p);

			mWidgetUpdate(wg);
			break;
		
		case MEVENT_FOCUS:
			mWidgetUpdate(wg);
			break;
		default:
			return FALSE;
	}

	return TRUE;
}


//==========================
// 描画
//==========================


/** 水平の各タブ描画 */

static void _draw_horz_tab(mTab *p,mPixbuf *pixbuf,mFont *font,mTabItem *pi,mBool bFocus)
{
	int x,h,sep,nofocus;
	mBool bottom;

	x = pi->pos;
	h = p->wg.h;
	sep = (_IS_HAVE_SEP(p) != 0);
	nofocus = !bFocus;
	bottom = ((p->tab.style & MTAB_S_BOTTOM) != 0);

	//枠

	mPixbufBox(pixbuf, x, (bottom)? sep: nofocus,
		pi->w, h - sep - nofocus, MSYSCOL(FRAME));

	//背景

	mPixbufFillBox(pixbuf, x + 1, (bottom)? (nofocus && sep): 1 + nofocus,
		pi->w - 2, h - 1 - nofocus - (nofocus && sep),
		(bFocus)? MSYSCOL(FACE): MSYSCOL(FACE_DARK));

	//フォーカス枠

	if(bFocus && (p->wg.fState & MWIDGET_STATE_FOCUSED))
		mPixbufBoxDash(pixbuf, x + 2, 2, pi->w - 4, h - 4, MSYSCOL(TEXT));

	//テキスト

	mPixbufSetClipBox_d(pixbuf,
		x + _PADDING_TABX, 0, pi->w - _PADDING_TABX * 2, h);

	mFontDrawText(font, pixbuf,
		x + _PADDING_TABX,
		(bottom? bFocus: nofocus) + ((h - font->height) >> 1),
		pi->label, -1,
		(p->wg.fState & MWIDGET_STATE_ENABLED)? MSYSCOL_RGB(TEXT): MSYSCOL_RGB(TEXT_DISABLE));

	mPixbufClipNone(pixbuf);
}

/** 垂直の各タブ描画 */

static void _draw_vert_tab(mTab *p,mPixbuf *pixbuf,mFont *font,mTabItem *pi,mBool bFocus)
{
	int y,w,itemh,sep,nofocus;
	mBool right;

	y = pi->pos;
	w = p->wg.w;
	itemh = font->height + _PADDING_TABY * 2;
	sep = (_IS_HAVE_SEP(p) != 0);
	nofocus = !bFocus;
	right = ((p->tab.style & MTAB_S_RIGHT) != 0);

	//枠

	mPixbufBox(pixbuf, (right)? sep: nofocus, y, w - sep - nofocus, itemh, MSYSCOL(FRAME));

	//背景

	mPixbufFillBox(pixbuf,
		(right)? (nofocus && sep): 1 + nofocus, y + 1,
		w - 1 - nofocus - (nofocus && sep), itemh - 2,
		(bFocus)? MSYSCOL(FACE): MSYSCOL(FACE_DARK));

	//フォーカス枠

	if(bFocus && (p->wg.fState & MWIDGET_STATE_FOCUSED))
		mPixbufBoxDash(pixbuf, 2, y + 2, w - 4, itemh - 4, MSYSCOL(TEXT));

	//テキスト

	mPixbufSetClipBox_d(pixbuf,
		_PADDING_TABX, y, w - _PADDING_TABX * 2, itemh);

	mFontDrawText(font, pixbuf,
		(right? sep + bFocus: nofocus) + _PADDING_TABX,
		y + _PADDING_TABY,
		pi->label, -1,
		(p->wg.fState & MWIDGET_STATE_ENABLED)? MSYSCOL_RGB(TEXT): MSYSCOL_RGB(TEXT_DISABLE));

	mPixbufClipNone(pixbuf);
}

/** 描画 */

void mTabDrawHandle(mWidget *wg,mPixbuf *pixbuf)
{
	mTab *p = M_TAB(wg);
	mFont *font;
	mBool vert;
	mTabItem *pi;
	void (*drawtab)(mTab *p,mPixbuf *pixbuf,mFont *font,mTabItem *pi,mBool bFocus);

	font = mWidgetGetFont(wg);
	vert = (_IS_VERTTAB(p) != 0);

	//背景

	mWidgetDrawBkgnd(wg, NULL);

	//セパレータ

	if(_IS_HAVE_SEP(p))
	{
		if(vert)
		{
			mPixbufLineV(pixbuf,
				(p->tab.style & MTAB_S_RIGHT)? 0: wg->w - 1, 0,
				wg->h, MSYSCOL(FRAME));
		}
		else
		{
			mPixbufLineH(pixbuf,
				0, (p->tab.style & MTAB_S_BOTTOM)? 0: wg->h - 1,
				wg->w, MSYSCOL(FRAME));
		}
	}

	//---- 各タブ

	drawtab = (vert)? _draw_vert_tab: _draw_horz_tab;

	//フォーカスより左/上

	for(pi = M_TABITEM(p->tab.list.top); pi && pi != p->tab.focusitem; pi = M_TABITEM(pi->i.next))
		(drawtab)(p, pixbuf, font, pi, FALSE);

	//フォーカスより右/下

	for(pi = M_TABITEM(p->tab.list.bottom); pi && pi != p->tab.focusitem; pi = M_TABITEM(pi->i.prev))
		(drawtab)(p, pixbuf, font, pi, FALSE);

	//フォーカスタブ

	if(p->tab.focusitem)
		(drawtab)(p, pixbuf, font, p->tab.focusitem, TRUE);
}

/** @} */
