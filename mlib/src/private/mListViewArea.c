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
 * mListViewArea [リストビュー領域]
 *****************************************/

/*
 * mListView と、コンボボックスのポップアップで使われる。
 * 
 * itemH に項目の高さをセットしておくこと。
 * onSize(), isBarVisible() もセットすること。
 * 
 * [!] フォーカスアイテムが変更されたら MLISTVIEW_N_CHANGE_FOCUS を通知する。
 */

#include <string.h>

#include "mDef.h"

#include "mListView.h"
#include "mListViewArea.h"
#include "mLVItemMan.h"
#include "mImageList.h"

#include "mScrollBar.h"
#include "mListHeader.h"
#include "mList.h"
#include "mWidget.h"
#include "mPixbuf.h"
#include "mFont.h"
#include "mEvent.h"
#include "mSysCol.h"
#include "mKeyDef.h"


//--------------------

static int _event_handle(mWidget *wg,mEvent *ev);
static void _draw_handle(mWidget *wg,mPixbuf *pixbuf);

//--------------------


//=========================
// sub
//=========================


/** 通知 (親を通知元として、親の通知先に)
 *
 * MLISTVIEW_N_CHANGE_FOCUS, MLISTVIEW_N_CLICK_ON_FOCUS の場合は自動でパラメータ値セット */

static void _notify(mListViewArea *p,int type,intptr_t param1,intptr_t param2)
{
	if(type == MLISTVIEW_N_CHANGE_FOCUS || type == MLISTVIEW_N_CLICK_ON_FOCUS)
	{
		param1 = (intptr_t)(p->lva.manager->itemFocus);
		param2 = (p->lva.manager)->itemFocus->param;
	}

	mWidgetAppendEvent_notify(NULL, p->wg.parent, type, param1, param2);
}

/** 位置からアイテム取得 */

static mListViewItem *_getItemByPos(mListViewArea *p,int x,int y)
{
	mPoint pt;
	
	mScrollViewAreaGetScrollPos(M_SCROLLVIEWAREA(p), &pt);
	y += pt.y - p->lva.headerH;
	
	if(y < 0)
		return NULL; 
	else
		return mLVItemMan_getItemByIndex(p->lva.manager, y / p->lva.itemH);
}

/** レイアウトハンドラ
 *
 * ヘッダの幅を親に合わせる */

static void _layout_handle(mWidget *wg)
{
	mListViewArea *p = M_LISTVIEWAREA(wg);

	if(p->lva.header)
		mWidgetMoveResize(M_WIDGET(p->lva.header), 0, 0, wg->w, p->lva.headerH);
}


//=========================


/** 作成
 *
 * @param styleLV mListView のスタイル
 * @param owner   描画関数のオーナーウィジェット */

mListViewArea *mListViewAreaNew(int size,mWidget *parent,
	uint32_t style,uint32_t style_listview,
	mLVItemMan *manager,mWidget *owner)
{
	mListViewArea *p;
	mListHeader *header;

	//mScrollViewArea
	
	if(size < sizeof(mListViewArea)) size = sizeof(mListViewArea);
	
	p = (mListViewArea *)mScrollViewAreaNew(size, parent);
	if(!p) return NULL;
	
	p->wg.draw = _draw_handle;
	p->wg.event = _event_handle;
	p->wg.fEventFilter |= MWIDGET_EVENTFILTER_POINTER | MWIDGET_EVENTFILTER_SCROLL;
	p->wg.layout = _layout_handle;
	
	p->lva.manager = manager;
	p->lva.style = style;
	p->lva.styleLV = style_listview;
	p->lva.owner = owner;

	//mListHeader

	if(style_listview & MLISTVIEW_S_MULTI_COLUMN)
	{
		p->lva.header = header = mListHeaderNew(0, M_WIDGET(p),
			(style_listview & MLISTVIEW_S_HEADER_SORT)? MLISTHEADER_STYLE_SORT: 0);

		header->wg.notifyTarget = MWIDGET_NOTIFYTARGET_PARENT; //通知は mListView へ送る

		mListHeaderCalcHintHandle(M_WIDGET(header));

		//ヘッダを表示しない場合は非表示

		if(style_listview & MLISTVIEW_S_NO_HEADER)
			header->wg.fState &= ~MWIDGET_STATE_VISIBLE;
		else
			p->lva.headerH = header->wg.hintH;
	}
	
	return p;
}

/** アイコンイメージセット */

void mListViewArea_setImageList(mListViewArea *p,mImageList *img)
{
	p->lva.iconimg = img;
}

/** フォーカスアイテムが表示されない範囲にある場合、スクロール位置調整
 *
 * 現在のスクロール位置で問題ない場合はそのまま。
 * 上下キーでの移動時と、ポップアップ表示の開始時。
 *
 * @param dir [0]方向なし [-1]上方向 [1]下方向
 * @param margin_num  上下に余白を付けたい場合、余白のアイテム数 */

void mListViewArea_scrollToFocus(mListViewArea *p,int dir,int margin_num)
{
	mScrollBar *scr;
	int pos,y,itemh;
	
	scr = mScrollViewAreaGetScrollBar(M_SCROLLVIEWAREA(p), TRUE);
	if(!scr) return;

	itemh = p->lva.itemH;
	pos = scr->sb.pos;
	y = mLVItemMan_getItemIndex(p->lva.manager, NULL) * itemh;

	margin_num *= itemh;

	if(dir == 0)
	{
		/* スクロールせずに表示できる場合はそのまま。
		 * スクロールする場合は、アイテムが先頭に来るように。 */
	
		if(y < pos + margin_num
			|| y > pos + scr->sb.page - itemh - margin_num)
			mScrollBarSetPos(scr, y - margin_num);
	}
	else if(dir < 0)
	{
		//上移動 (上方向に見えない場合)

		if(y < pos + margin_num)
			mScrollBarSetPos(scr, y - margin_num);
	}
	else if(dir > 0)
	{
		//下移動 (下方向に見えない場合)

		if(y > pos + scr->sb.page - itemh - margin_num)
			mScrollBarSetPos(scr, y - scr->sb.page + itemh + margin_num);
	}
}


//========================
// イベントハンドラ sub
//========================


/** チェックボタンの ON/OFF 処理
 *
 * @return 処理されたか */

static mBool _btt_press_checkbox(mListViewArea *p,mListViewItem *item,mEvent *ev)
{
	int x,y,itemh;
	mPoint pt;

	if(p->lva.styleLV & MLISTVIEW_S_CHECKBOX)
	{
		mScrollViewAreaGetScrollPos(M_SCROLLVIEWAREA(p), &pt);

		itemh = p->lva.itemH;
		
		x = ev->pt.x + pt.x;
		y = ev->pt.y + pt.y - p->lva.headerH;
		
		y = y - (y / itemh) * itemh - (itemh - MLISTVIEW_DRAW_CHECKBOX_SIZE) / 2;
		
		if(x >= MLISTVIEW_DRAW_ITEM_MARGIN
			&& x < MLISTVIEW_DRAW_ITEM_MARGIN + MLISTVIEW_DRAW_CHECKBOX_SIZE
			&& y >= 0 && y < MLISTVIEW_DRAW_CHECKBOX_SIZE)
		{
			item->flags ^= MLISTVIEW_ITEM_F_CHECKED;
			
			_notify(p, MLISTVIEW_N_ITEM_CHECK, (intptr_t)item, item->param);
			
			return TRUE;
		}
	}
	
	return FALSE;
}

/** PageUp/Down */

static void _page_updown(mListViewArea *p,int up)
{
	mScrollBar *scrv;
	int pos,itemh;
	
	scrv = mScrollViewAreaGetScrollBar(M_SCROLLVIEWAREA(p), TRUE);
	if(!scrv) return;

	//スクロール位置
	
	pos = scrv->sb.pos;
	
	if(up)
		pos -= scrv->sb.page;
	else
		pos += scrv->sb.page;
	
	if(mScrollBarSetPos(scrv, pos))
	{
		mWidgetUpdate(M_WIDGET(p));
	
		/* フォーカスアイテム
		 * PageUp は上部、PageDown は下部の位置のアイテム*/
		
		itemh = p->lva.itemH;
		pos = scrv->sb.pos;
		
		if(up)
			pos += itemh - 1;
		else
			pos += scrv->sb.page - itemh;
		
		if(mLVItemMan_setFocusItemByIndex(p->lva.manager, pos / itemh))
			_notify(p, MLISTVIEW_N_CHANGE_FOCUS, 0, 0);
	}
}

/** Home/End */

static void _key_home_end(mListViewArea *p,int home)
{
	mScrollBar *scrv;

	//アイテム選択
	
	if(mLVItemMan_setFocusHomeEnd(p->lva.manager, home))
		_notify(p, MLISTVIEW_N_CHANGE_FOCUS, 0, 0);

	//スクロール

	scrv = mScrollViewAreaGetScrollBar(M_SCROLLVIEWAREA(p), TRUE);
	if(scrv)
	{
		if(home)
			mScrollBarSetPos(scrv, scrv->sb.min);
		else
			mScrollBarSetPosToEnd(scrv);
	}
	
	mWidgetUpdate(M_WIDGET(p));
}


//========================
// イベントハンドラ
//========================


/** ボタン押し (LEFT/RIGHT) */

static void _event_btt_press(mListViewArea *p,mEvent *ev)
{
	mListViewItem *pi;
	mBool ret;

	//ポップアップ時、左ボタン押しで即終了

	if((p->lva.style & MLISTVIEWAREA_S_POPUP) && ev->pt.btt == M_BTT_LEFT)
	{
		_notify(p, MLISTVIEWAREA_N_POPUPEND, 0, 0);
		return;
	}

	//親にフォーカスセット

	mWidgetSetFocus(p->wg.parent);

	//
	
	pi = _getItemByPos(p, ev->pt.x, ev->pt.y);

	if(pi)
	{
		//選択処理
		
		ret = mLVItemMan_select(p->lva.manager, ev->pt.state, pi);

		_notify(p, (ret)? MLISTVIEW_N_CHANGE_FOCUS: MLISTVIEW_N_CLICK_ON_FOCUS, 0, 0);
		
		//チェックボックス
		
		if(ev->pt.btt == M_BTT_LEFT)
			_btt_press_checkbox(p, pi, ev);

		mWidgetUpdate(M_WIDGET(p));
	}
			
	//右ボタン (アイテム外の場合 NULL)
	
	if(ev->pt.btt == M_BTT_RIGHT)
		_notify(p, MLISTVIEW_N_ITEM_RCLK, (intptr_t)pi, 0);
}

/** 左ダブルクリック */

static void _event_dblclk(mListViewArea *p,mEvent *ev)
{
	mListViewItem *pi;
	
	pi = _getItemByPos(p, ev->pt.x, ev->pt.y);
	
	if(pi)
	{
		//チェックボックス (範囲外の場合はダブルクリック通知)
		
		if(_btt_press_checkbox(p, pi, ev))
			mWidgetUpdate(M_WIDGET(p));
		else
			_notify(p, MLISTVIEW_N_ITEM_DBLCLK, (intptr_t)pi, pi->param);
	}
}

/** カーソル移動 (ポップアップ時) */

static void _event_motion_popup(mListViewArea *p,mEvent *ev)
{
	mListViewItem *pi;
	
	pi = _getItemByPos(p, ev->pt.x, ev->pt.y);

	if(pi)
	{
		if(mLVItemMan_select(p->lva.manager, 0, pi))
			mWidgetUpdate(M_WIDGET(p));
	}
}

/** ホイールスクロール */

static void _event_scroll(mListViewArea *p,mEvent *ev)
{
	mScrollBar *scrv;
	int pos;
	
	scrv = mScrollViewAreaGetScrollBar(M_SCROLLVIEWAREA(p), TRUE);
	
	if(scrv)
	{
		pos = scrv->sb.pos;

		if(ev->scr.dir == MEVENT_SCROLL_DIR_UP)
			pos -= p->lva.itemH * 3;
		else
			pos += p->lva.itemH * 3;
		
		if(mScrollBarSetPos(scrv, pos))
			mWidgetUpdate(M_WIDGET(p));
	}
}

/** キー押し */

static int _event_key_down(mListViewArea *p,mEvent *ev)
{
	int update = 0;

	switch(ev->key.code)
	{
		//上
		case MKEY_UP:
			if(mLVItemMan_updownFocus(p->lva.manager, FALSE))
			{
				mListViewArea_scrollToFocus(p, -1, 0);
				_notify(p, MLISTVIEW_N_CHANGE_FOCUS, 0, 0);
				update = 1;
			}
			break;
		//下
		case MKEY_DOWN:
			if(mLVItemMan_updownFocus(p->lva.manager, TRUE))
			{
				mListViewArea_scrollToFocus(p, 1, 0);
				_notify(p, MLISTVIEW_N_CHANGE_FOCUS, 0, 0);
				update = 1;
			}
			break;
		//PageUp/Down
		case MKEY_PAGEUP:
			_page_updown(p, TRUE);
			break;
		case MKEY_PAGEDOWN:
			_page_updown(p, FALSE);
			break;
		//Home/End
		case MKEY_HOME:
			_key_home_end(p, TRUE);
			break;
		case MKEY_END:
			_key_home_end(p, FALSE);
			break;
		
		//Ctrl+A (すべて選択)
		case 'A':
			if((p->lva.styleLV & MLISTVIEW_S_MULTI_SEL)
				&& (ev->key.state & M_MODS_CTRL))
			{
				mLVItemMan_selectAll(p->lva.manager);
				update = 1;
			}
			else
				return FALSE;
			break;
		
		default:
			return FALSE;
	}
	
	//更新
	
	if(update)
		mWidgetUpdate(M_WIDGET(p));
	
	return TRUE;
}

/** イベント */

int _event_handle(mWidget *wg,mEvent *ev)
{
	mListViewArea *p = M_LISTVIEWAREA(wg);

	switch(ev->type)
	{
		//ポインタ
		case MEVENT_POINTER:
			if(ev->pt.type == MEVENT_POINTER_TYPE_MOTION)
			{
				//移動
				if(p->lva.style & MLISTVIEWAREA_S_POPUP)
					_event_motion_popup(p, ev);
			}
			else if(ev->pt.type == MEVENT_POINTER_TYPE_PRESS)
			{
				//押し
				if(ev->pt.btt == M_BTT_LEFT || ev->pt.btt == M_BTT_RIGHT)
					_event_btt_press(p, ev);
			}
			else if(ev->pt.type == MEVENT_POINTER_TYPE_DBLCLK)
			{
				//ダブルクリック
				if(ev->pt.btt == M_BTT_LEFT)
					_event_dblclk(p, ev);
			}
			break;

		//通知
		case MEVENT_NOTIFY:
			if(ev->notify.widgetFrom == p->wg.parent)
			{
				//スクロール
				
				if((ev->notify.type == MSCROLLVIEWAREA_N_SCROLL_VERT
					|| ev->notify.type == MSCROLLVIEWAREA_N_SCROLL_HORZ)
					&& (ev->notify.param2 & MSCROLLBAR_N_HANDLE_F_CHANGE))
				{
					//ヘッダのスクロール
					
					if(ev->notify.type == MSCROLLVIEWAREA_N_SCROLL_HORZ
						&& p->lva.header)
						mListHeaderSetScrollPos(p->lva.header, ev->notify.param1);
				
					mWidgetUpdate(wg);
				}
			}
			else if(p->lva.header
				&& ev->notify.widgetFrom == (mWidget *)p->lva.header)
			{
				//ヘッダ

				if(ev->notify.type == MLISTHEADER_N_CHANGE_WIDTH)
				{
					//列サイズが変更 -> 再構成
					//(水平スクロールの表示/非表示が変わる場合があるため)

					mWidgetAppendEvent_only(M_WIDGET(p->wg.parent), MEVENT_CONSTRUCT);
				}
				else if(ev->notify.type == MLISTHEADER_N_CHANGE_SORT)
				{
					//列クリックでソート情報変更 -> 通知

					mWidgetAppendEvent_notify(NULL, M_WIDGET(p->wg.parent), MLISTVIEW_N_CHANGE_SORT,
						ev->notify.param1, ev->notify.param2);
				}
			}
			break;
		
		//ホイール
		case MEVENT_SCROLL:
			if(ev->scr.dir == MEVENT_SCROLL_DIR_UP
				|| ev->scr.dir == MEVENT_SCROLL_DIR_DOWN)
				_event_scroll(p, ev);
			break;

		case MEVENT_KEYDOWN:
			return _event_key_down(p, ev);
		
		default:
			return FALSE;
	}

	return TRUE;
}


//=======================
// 描画
//=======================


/** チェックボックスなど描画 */

static int _draw_item_left(mPixbuf *pixbuf,int x,int y,int itemh,
	mListViewArea *p,mListViewItem *pi)
{
	//チェックボックス
	
	if(p->lva.styleLV & MLISTVIEW_S_CHECKBOX)
	{
		mPixbufDrawCheckBox(pixbuf,
			x, y + (itemh - MLISTVIEW_DRAW_CHECKBOX_SIZE) / 2,
			(pi->flags & MLISTVIEW_ITEM_F_CHECKED)? MPIXBUF_DRAWCKBOX_CHECKED: 0);
		
		x += MLISTVIEW_DRAW_CHECKBOX_SIZE + MLISTVIEW_DRAW_CHECKBOX_SPACE;
	}

	//アイコン
	/* icon < 0 の場合は位置だけ進める */

	if(p->lva.iconimg)
	{
		mImageList *img = p->lva.iconimg;

		if(pi->icon >= 0)
			mImageListPutPixbuf(img, pixbuf, x, y + (itemh - img->h) / 2, pi->icon, FALSE);

		x += img->eachw + MLISTVIEW_DRAW_ICON_SPACE;
	}
	
	return x;
}

/** 描画 */

#define _F_FOCUSED       1
#define _F_SINGLE_COLUMN 2
#define _F_GRID_ROW      4

void _draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	mListViewArea *p = M_LISTVIEWAREA(wg);
	mFont *font;
	mListViewItem *pi;
	mListHeaderItem *hi;
	mPoint scr;
	int n,x,y,itemh,texty,flags,headerH,hindex;
	const char *ptxt,*ptxtend;
	mBool bsel,bFocusItem,bHeaderItem;
	mListViewItemDraw dinfo;
	
	font = mWidgetGetFont(wg);
	headerH = p->lva.headerH;
	itemh = p->lva.itemH;
	
	mScrollViewAreaGetScrollPos(M_SCROLLVIEWAREA(wg), &scr);

	//フラグ
	
	flags = 0;
	
	if((wg->parent)->fState & MWIDGET_STATE_FOCUSED) flags |= _F_FOCUSED;
	if(!(p->lva.styleLV & MLISTVIEW_S_MULTI_COLUMN)) flags |= _F_SINGLE_COLUMN;
	if(p->lva.styleLV & MLISTVIEW_S_GRID_ROW) flags |= _F_GRID_ROW;

	//背景
	
	mPixbufFillBox(pixbuf, 0, 0, wg->w, wg->h, MSYSCOL(FACE_LIGHTEST));

	//列のグリッド

	if((p->lva.styleLV & MLISTVIEW_S_GRID_COL) && p->lva.header)
	{
		x = -scr.x;
		
		for(hi = mListHeaderGetTopItem(p->lva.header); hi && hi->i.next; hi = M_LISTHEADER_ITEM(hi->i.next))
		{
			x += hi->width;
			mPixbufLineV(pixbuf, x - 1, headerH, wg->h - 1, MSYSCOL(FACE_DARK));
		}
	}
	
	//--------- 項目
	
	pi = M_LISTVIEWITEM(p->lva.manager->list.top);
	y = -scr.y + headerH;
	
	texty = (itemh - font->height) >> 1;
	
	//
	
	for(; pi; pi = M_LISTVIEWITEM(pi->i.next), y += itemh)
	{
		if(y + itemh <= headerH) continue;
		if(y >= wg->h) break;

		bsel = ((pi->flags & MLISTVIEW_ITEM_F_SELECTED) != 0);
		bFocusItem = ((flags & _F_FOCUSED) && p->lva.manager->itemFocus == pi);
		bHeaderItem = ((pi->flags & MLISTVIEW_ITEM_F_HEADER) != 0);
		
		//背景色

		if(bsel)
		{
			if(flags & _F_FOCUSED)
				n = (bFocusItem)? MSYSCOL(FACE_SELECT): MSYSCOL(FACE_SELECT_LIGHT);
			else
				n = MSYSCOL(FACE_SELECT_LIGHT);
		}
		else if(bHeaderItem)
			n = MSYSCOL(FACE_DARKER);
		else
			n = -1;

		if(n != -1)
			mPixbufFillBox(pixbuf, 0, y, wg->w, itemh, n);

		//横グリッド線
		
		if(flags & _F_GRID_ROW)
			mPixbufLineH(pixbuf, 0, y + itemh - 1, wg->w, MSYSCOL(FACE_DARK));

		//アイテム

		if(flags & _F_SINGLE_COLUMN)
		{
			//------ 単一列
						
			mPixbufSetClipBox_d(pixbuf,
				MLISTVIEW_DRAW_ITEM_MARGIN, y, wg->w - MLISTVIEW_DRAW_ITEM_MARGIN * 2, itemh);
			
			x = _draw_item_left(pixbuf, MLISTVIEW_DRAW_ITEM_MARGIN - scr.x, y, itemh, p, pi);

			if(pi->draw)
			{
				//描画関数

				dinfo.widget = p->lva.owner;
				dinfo.box.x = x;
				dinfo.box.y = y;
				dinfo.box.w = wg->w - x - MLISTVIEW_DRAW_ITEM_MARGIN;
				dinfo.box.h = itemh;
				dinfo.flags = 0;

				if(bsel)
					dinfo.flags |= MLISTVIEWITEMDRAW_F_SELECTED;

				if(flags & _F_FOCUSED)
					dinfo.flags |= MLISTVIEWITEMDRAW_F_FOCUSED;

				if(bFocusItem)
					dinfo.flags |= MLISTVIEWITEMDRAW_F_FOCUS_ITEM;

				mPixbufSetClipBox_box(pixbuf, &dinfo.box);

				(pi->draw)(pixbuf, pi, &dinfo);
			}
			else
			{
				//テキスト

				if(bHeaderItem) x += 4;
				
				mFontDrawText(font, pixbuf, x, y + texty, pi->text, pi->textlen,
					(bFocusItem || bHeaderItem)? MSYSCOL_RGB(TEXT_SELECT): MSYSCOL_RGB(TEXT));
			}
		}
		else if(p->lva.header)
		{
			//----- 複数列

			ptxt = ptxtend = pi->text;
			hindex = 0;
			x = -scr.x;

			for(hi = mListHeaderGetTopItem(p->lva.header); hi; hi = M_LISTHEADER_ITEM(hi->i.next), hindex++)
			{
				//列のテキスト範囲
				
				if(ptxt)
				{
					ptxtend = strchr(ptxt, '\t');
					if(!ptxtend) ptxtend = pi->text + pi->textlen;
				}

				//描画範囲内

				if(x + hi->width > 0 && x < wg->w)
				{
					n = x + MLISTVIEW_DRAW_ITEM_MARGIN;

					mPixbufSetClipBox_d(pixbuf,
						n, y, hi->width - MLISTVIEW_DRAW_ITEM_MARGIN * 2, itemh);

					//アイコンなど

					if(hindex == 0)
						n = _draw_item_left(pixbuf, n, y, itemh, p, pi);

					//

					if(pi->draw)
					{
						//---- 描画関数

						dinfo.widget = p->lva.owner;
						dinfo.box.x = n;
						dinfo.box.y = y;
						dinfo.box.w = hi->width - MLISTVIEW_DRAW_ITEM_MARGIN * 2;
						dinfo.box.h = itemh;
						dinfo.flags = 0;

						if(bsel)
							dinfo.flags |= MLISTVIEWITEMDRAW_F_SELECTED;

						if(flags & _F_FOCUSED)
							dinfo.flags |= MLISTVIEWITEMDRAW_F_FOCUSED;

						if(bFocusItem)
							dinfo.flags |= MLISTVIEWITEMDRAW_F_FOCUS_ITEM;

						mPixbufSetClipBox_box(pixbuf, &dinfo.box);

						(pi->draw)(pixbuf, pi, &dinfo);
					}
					else if(ptxt < ptxtend)
					{
						//---- 通常テキスト
						
						//右寄せ (2番目の列以降)

						if(hindex && (hi->flags & MLISTHEADER_ITEM_F_RIGHT))
							n = x + hi->width - mFontGetTextWidth(font, ptxt, ptxtend - ptxt) - MLISTVIEW_DRAW_ITEM_MARGIN;

						//描画

						mFontDrawText(font, pixbuf, n, y + texty,
							ptxt, ptxtend - ptxt,
							(bFocusItem || bHeaderItem)? MSYSCOL_RGB(TEXT_SELECT): MSYSCOL_RGB(TEXT));
					}
				}

				//次へ

				if(ptxt)
				{
					ptxt = ptxtend;
					if(*ptxt == '\t') ptxt++;
				}

				x += hi->width;
			}
		}
		
		mPixbufClipNone(pixbuf);
	}
}
