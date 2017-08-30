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
 * mIconButtons
 *****************************************/

#include "mDef.h"
#include "mGui.h"

#include "mWidget.h"
#include "mPixbuf.h"
#include "mImageList.h"
#include "mList.h"
#include "mEvent.h"
#include "mSysCol.h"
#include "mTrans.h"
#include "mToolTip.h"

#include "mIconButtons.h"


//------------------

#define _BTT_PADDING  3		//アイコン周りの余白
#define _BTT_SEPW     9		//ボタン、区切りの幅
#define _BTT_DROPW    15	//ドロップボタンの幅
#define _SEP_BOTTOM_WIDTH  5	//BOTTOM 区切りの幅

#define _PRESSF_BTT  1
#define _PRESSF_DROP 2

#define _IS_VERT(p)    ((p)->ib.style & MICONBUTTONS_S_VERT)
#define _ITEM_TOP(p)   (mIconButtonsItem *)((p)->ib.list.top)
#define _ITEM_PREV(pi) (mIconButtonsItem *)((pi)->i.prev)
#define _ITEM_NEXT(pi) (mIconButtonsItem *)((pi)->i.next)

//------------------

typedef struct
{
	mListItem i;
	int id,img,tooltip,
		x,y,w,h;
	uint32_t flags;
}mIconButtonsItem;

//------------------


/*******************//**

@defgroup iconbuttons mIconButtons
@brief 複数のアイコンボタン

- ボタンが押されたら、\b MEVENT_COMMAND で通知される。 \n
ドロップボックスがある場合にドロップボックス側が押されたら、ev->cmd.by に \b MEVENT_COMMAND_BY_ICONBUTTONS_DROP が入る。
- ツールチップは翻訳データの文字列 ID で指定する。
- 自動折り返しを行わない場合は、手動で mIconButtonsCalcHintSize() を使って推奨サイズを設定する。
- 自動折り返しを行う場合、親の onSize() 時に mIconButtonsCalcHintSize() を行うこと。 \n
ただし、親の幅に 100% フィットさせる場合のみ。

<h3>継承</h3>
mWidget \> mIconButtons

@ingroup group_widget
@{

@file mIconButtons.h
@def M_ICONBUTTONS(p)
@struct mIconButtonsData
@struct mIconButtons
@enum MICONBUTTONS_STYLE
@enum MICONBUTTONS_ITEM_FLAGS

@var MICONBUTTONS_STYLE::MICONBUTTONS_S_TOOLTIP
各ボタンでツールチップを表示する

@var MICONBUTTONS_STYLE::MICONBUTTONS_S_AUTO_WRAP
ボタンの自動折り返しを行う

@var MICONBUTTONS_STYLE::MICONBUTTONS_S_VERT
垂直に並べる

@var MICONBUTTONS_STYLE::MICONBUTTONS_S_SEP_BOTTOM
下端にセパレータを付ける

@var MICONBUTTONS_STYLE::MICONBUTTONS_S_DESTROY_IMAGELIST
ウィジェット削除時、イメージリストも削除する

**********************/


//===============================
// sub
//===============================


/** アイテムIDから検索 */

static mIconButtonsItem *_getItemByID(mIconButtons *p,int id)
{
	mIconButtonsItem *pi;

	for(pi = _ITEM_TOP(p); pi; pi = _ITEM_NEXT(pi))
	{
		if(pi->id == id) return pi;
	}

	return NULL;
}

/** アイテムのチェックがONの状態か */

static mBool _isItemCheckON(mIconButtonsItem *pi)
{
	return ((pi->flags & (MICONBUTTONS_ITEMF_CHECKBUTTON | MICONBUTTONS_ITEMF_CHECKGROUP))
		&& (pi->flags & MICONBUTTONS_ITEMF_CHECKED));
}

/** CONSTRUCT イベント追加 */

static void _send_const(mIconButtons *p)
{
	mWidgetAppendEvent_only(M_WIDGET(p), MEVENT_CONSTRUCT);
}

/** ボタンの通常サイズ取得 */

static void _get_button_normal_size(mIconButtons *p,mSize *sz)
{
	int w,h;

	if(p->ib.imglist)
	{
		w = (p->ib.imglist)->eachw;
		h = (p->ib.imglist)->h;
	}
	else
		w = h = 16;

	sz->w = w + _BTT_PADDING * 2;
	sz->h = h + _BTT_PADDING * 2;
}

/** 各ボタンアイテムのサイズ取得
 *
 * @param szdst あからじめ通常ボタンのサイズを入れておくこと */

static void _get_item_size(mIconButtons *p,
	mIconButtonsItem *pi,mSize *szdst)
{
	int w,h;

	w = szdst->w;
	h = szdst->h;

	//セパレータ

	if(pi->flags & MICONBUTTONS_ITEMF_SEP)
	{
		if(_IS_VERT(p))
			h = _BTT_SEPW;
		else
			w = _BTT_SEPW;
	}

	//ドロップボタン

	if(pi->flags & MICONBUTTONS_ITEMF_DROPDOWN)
		w += _BTT_DROPW;

	szdst->w = w;
	szdst->h = h;
}

/** ボタン位置・サイズセット */

static void _set_buttons_info(mIconButtons *p)
{
	mIconButtonsItem *pi;
	int x,y;
	mBool bVert;
	mSize szbtt,sznormal;

	x = y = 0;
	_get_button_normal_size(p, &sznormal);

	bVert = (_IS_VERT(p) != 0);

	//

	for(pi = _ITEM_TOP(p); pi; pi = _ITEM_NEXT(pi))
	{
		szbtt = sznormal;
		_get_item_size(p, pi, &szbtt);

		//水平、自動折り返し

		if(!bVert && (p->ib.style & MICONBUTTONS_S_AUTO_WRAP)
			&& x && x + szbtt.w > p->wg.w)
		{
			x = 0;
			y += szbtt.h;
		}

		//セット
	
		pi->x = x;
		pi->y = y;
		pi->w = szbtt.w;
		pi->h = szbtt.h;

		//次

		if(bVert)
			y += szbtt.h;
		else
		{
			if(pi->flags & MICONBUTTONS_ITEMF_WRAP)
			{
				x = 0;
				y += szbtt.h;
			}
			else
				x += szbtt.w;
		}
	}
}

/** 推奨サイズ取得 */

static void _get_hintsize(mIconButtons *p,mSize *size)
{
	mIconButtonsItem *pi;
	mSize szbtt,sznormal,szarea;
	int w,maxw,maxh,bvert;

	bvert = _IS_VERT(p);

	_get_button_normal_size(p, &sznormal);

	mWidgetGetLayoutMaxSize(M_WIDGET(p), &szarea);

	maxw = maxh = w = 0;

	if(!bvert) maxh = sznormal.h;

	//計算

	for(pi = _ITEM_TOP(p); pi; pi = _ITEM_NEXT(pi))
	{
		szbtt = sznormal;
		_get_item_size(p, pi, &szbtt);

		if(!bvert && (p->ib.style & MICONBUTTONS_S_AUTO_WRAP)
			&& w && w + szbtt.w > szarea.w)
		{
			w = 0;
			maxh += szbtt.h;
		}
		
		if(bvert)
		{
			//垂直
			
			if(maxw < szbtt.w) maxw = szbtt.w;
			maxh += szbtt.h;
		}
		else
		{
			//水平
			
			w += szbtt.w;

			if(maxw < w) maxw = w;

			if(pi->flags & MICONBUTTONS_ITEMF_WRAP)
			{
				w = 0;
				maxh += szbtt.h;
			}
		}
	}

	//セパレータ

	if(p->ib.style & MICONBUTTONS_S_SEP_BOTTOM)
		maxh += _SEP_BOTTOM_WIDTH;

	size->w = maxw;
	size->h = maxh;
}

/** グループのチェック処理 */

static void _set_check_group(mIconButtonsItem *pi)
{
	mIconButtonsItem *p;

	//前方向 OFF

	for(p = _ITEM_PREV(pi); p; p = _ITEM_PREV(p))
	{
		if(!(p->flags & MICONBUTTONS_ITEMF_CHECKGROUP)) break;

		p->flags &= ~MICONBUTTONS_ITEMF_CHECKED;
	}

	//次方向 OFF

	for(p = _ITEM_NEXT(pi); p; p = _ITEM_NEXT(p))
	{
		if(!(p->flags & MICONBUTTONS_ITEMF_CHECKGROUP)) break;

		p->flags &= ~MICONBUTTONS_ITEMF_CHECKED;
	}

	pi->flags |= MICONBUTTONS_ITEMF_CHECKED;
}

/** ツールチップ表示 */

static void _show_tooltip(mIconButtons *p,mIconButtonsItem *item)
{
	mPoint pt;

	mWidgetGetCursorPos(NULL, &pt);

	p->ib.tooltip = mToolTipShow(p->ib.tooltip,
		pt.x + 12, pt.y + 16,
		M_TR_T2(p->ib.trgroupid, item->tooltip));
}

/** ツールチップを消す */

static void _end_tooltip(mIconButtons *p)
{
	mWidgetTimerDelete(M_WIDGET(p), 0);

	mWidgetDestroy(M_WIDGET(p->ib.tooltip));
	p->ib.tooltip = NULL;
}

/** ツールチップ表示準備 */

static void _ready_tooltip(mIconButtons *p,mIconButtonsItem *item)
{
	if(p->ib.style & MICONBUTTONS_S_TOOLTIP)
	{
		if(item && item->tooltip >= 0)
		{
			if(p->ib.tooltip)
				_show_tooltip(p, item);
			else
				//タイマーで遅延
				mWidgetTimerAdd(M_WIDGET(p), 0, 500, 0);
		}
		else
			_end_tooltip(p);
	}
}


//===============================


/** 解放処理 */

void mIconButtonsDestroyHandle(mWidget *wg)
{
	mIconButtons *p = (mIconButtons *)wg;

	mListDeleteAll(&p->ib.list);

	if(p->ib.style & MICONBUTTONS_S_DESTROY_IMAGELIST)
		mImageListFree(p->ib.imglist);

	if(p->ib.tooltip)
		mWidgetDestroy(M_WIDGET(p->ib.tooltip));
}

/** サイズ変更時 */

static void _handle_onsize(mWidget *wg)
{
	mIconButtons *p = (mIconButtons *)wg;

	//自動折り返しの場合

	if(p->ib.style & MICONBUTTONS_S_AUTO_WRAP)
		_send_const(p);
}


//===============================


/** 作成 */

mIconButtons *mIconButtonsNew(int size,mWidget *parent,uint32_t style)
{
	mIconButtons *p;
	
	if(size < sizeof(mIconButtons)) size = sizeof(mIconButtons);
	
	p = (mIconButtons *)mWidgetNew(size, parent);
	if(!p) return NULL;
	
	p->wg.destroy = mIconButtonsDestroyHandle;
	p->wg.draw = mIconButtonsDrawHandle;
	p->wg.event = mIconButtonsEventHandle;
	p->wg.onSize = _handle_onsize;
	
	p->wg.fEventFilter |= MWIDGET_EVENTFILTER_POINTER;

	p->ib.style = style;
	
	return p;
}

/** イメージリストセット */

void mIconButtonsSetImageList(mIconButtons *p,mImageList *img)
{
	p->ib.imglist = img;
}

/** イメージリスト置き換え
 *
 * 現在のイメージは破棄される。\n
 * サイズを変更する場合は、サイズ変更の影響を受ける最上位の親ウィジェットを
 * 再レイアウトすること。 */

void mIconButtonsReplaceImageList(mIconButtons *p,mImageList *img)
{
	mImageListFree(p->ib.imglist);
	p->ib.imglist = img;

	mIconButtonsCalcHintSize(p);
	_set_buttons_info(p);

	mWidgetCalcHintSize(p->wg.parent);
	mWidgetUpdate(M_WIDGET(p));
}

/** ツールチップの翻訳文字列のグループIDセット */

void mIconButtonsSetTooltipTrGroup(mIconButtons *p,uint16_t gid)
{
	p->ib.trgroupid = gid;
}

/** 全ボタンから幅・高さを計算し、セット */

void mIconButtonsCalcHintSize(mIconButtons *p)
{
	mSize size;

	_get_hintsize(p, &size);
	
	p->wg.hintW = size.w;
	p->wg.hintH = size.h;
}


//==============================
// アイテム関連
//==============================


/** ボタンをすべて削除 */

void mIconButtonsDeleteAll(mIconButtons *p)
{
	mListDeleteAll(&p->ib.list);

	mWidgetUpdate(M_WIDGET(p));
}

/** ボタン追加 */

void mIconButtonsAdd(mIconButtons *p,int id,int img,int tooltip,uint32_t flags)
{
	mIconButtonsItem *pi;

	pi = (mIconButtonsItem *)mListAppendNew(&p->ib.list, sizeof(mIconButtonsItem), NULL);
	if(pi)
	{
		pi->id = id;
		pi->img = img;
		pi->tooltip = tooltip;
		pi->flags = flags;

		_send_const(p);
	}
}

/** セパレータ追加 */

void mIconButtonsAddSep(mIconButtons *p)
{
	mIconButtonsAdd(p, -1, 0, -1, MICONBUTTONS_ITEMF_SEP);
}

/** アイテムのチェック状態をセット
 *
 * @param type 正で ON、0 で OFF、負の値で反転 */

void mIconButtonsSetCheck(mIconButtons *p,int id,int type)
{
	mIconButtonsItem *pi = _getItemByID(p, id);

	if(pi && (pi->flags & (MICONBUTTONS_ITEMF_CHECKBUTTON | MICONBUTTONS_ITEMF_CHECKGROUP)))
	{
		if(pi->flags & MICONBUTTONS_ITEMF_CHECKGROUP)
		{
			//グループ

			if(type == 0)
				pi->flags &= ~MICONBUTTONS_ITEMF_CHECKED;
			else
				_set_check_group(pi);
		}
		else
		{
			//単体チェック

			if(type == 0)
				pi->flags &= ~MICONBUTTONS_ITEMF_CHECKED;
			else if(type > 0)
				pi->flags |= MICONBUTTONS_ITEMF_CHECKED;
			else
				pi->flags ^= MICONBUTTONS_ITEMF_CHECKED;
		}

		mWidgetUpdate(M_WIDGET(p));
	}
}

/** チェックされているか */

mBool mIconButtonsIsCheck(mIconButtons *p,int id)
{
	mIconButtonsItem *pi = _getItemByID(p, id);

	if(pi)
		return ((pi->flags & MICONBUTTONS_ITEMF_CHECKED) != 0);
	else
		return FALSE;
}

/** 有効/無効セット
 *
 * @param type 0 で無効、正で有効、負の値で反転 */

void mIconButtonsSetEnable(mIconButtons *p,int id,int type)
{
	mIconButtonsItem *pi = _getItemByID(p, id);

	if(pi)
	{
		if(type == 0)
			pi->flags |= MICONBUTTONS_ITEMF_DISABLE;
		else if(type > 0)
			pi->flags &= ~MICONBUTTONS_ITEMF_DISABLE;
		else
			pi->flags ^= MICONBUTTONS_ITEMF_DISABLE;

		mWidgetUpdate(M_WIDGET(p));
	}
}

/** ボタンの位置とサイズ取得
 *
 * @param bRoot TRUE でルート座標の位置。FALSE でウィジェットの位置。 */

void mIconButtonsGetItemBox(mIconButtons *p,int id,mBox *box,mBool bRoot)
{
	mIconButtonsItem *pi = _getItemByID(p, id);
	mPoint pt;

	if(pi)
	{
		pt.x = pi->x;
		pt.y = pi->y;

		if(bRoot)
			mWidgetMapPoint(M_WIDGET(p), NULL, &pt);

		box->x = pt.x;
		box->y = pt.y;
		box->w = pi->w;
		box->h = pi->h;
	}
}


//========================
// ハンドラ sub
//========================


/** カーソル位置下のボタン取得 */

static mIconButtonsItem *_getItemByPos(mIconButtons *p,int x,int y)
{
	mIconButtonsItem *pi;

	if(x < 0 || y < 0) return NULL;

	for(pi = _ITEM_TOP(p); pi; pi = _ITEM_NEXT(pi))
	{
		if(pi->x <= x && x < pi->x + pi->w
			&& pi->y <= y && y < pi->y + pi->h)
			return pi;
	}

	return NULL;
}

/** マウス移動時、カーソル下のボタンセット */

static void _set_motion_on(mIconButtons *p,int x,int y)
{
	mIconButtonsItem *pi;
	mWindow *modal;

	//モーダル状態で、かつ親ウィンドウがそのモーダルではない場合無効
	/* ダイアログ中は、他のウィンドウ上の mIconButtons の移動処理を行わない */

	modal = mAppGetCurrentModalWindow();

	if(modal && modal != p->wg.toplevel)
		pi = NULL;
	else
		pi = _getItemByPos(p, x, y);

	//セパレータの場合

	if(pi && (pi->flags & MICONBUTTONS_ITEMF_SEP))
		pi = NULL;

	//変更

	if((void *)pi != p->ib.itemOn)
	{
		p->ib.itemOn = pi;

		mWidgetUpdate(M_WIDGET(p));

		//ツールチップ

		_ready_tooltip(p, pi);
	}
}

/** ボタン押し時 */

static void _event_press(mIconButtons *p,mEvent *ev)
{
	mIconButtonsItem *pion;

	pion = (mIconButtonsItem *)p->ib.itemOn;

	//アイテム無効時

	if(pion->flags & MICONBUTTONS_ITEMF_DISABLE) return;

	//ツールチップ消去

	_end_tooltip(p);

	//

	if((pion->flags & MICONBUTTONS_ITEMF_DROPDOWN)
		&& ev->pt.x >= pion->x + pion->w - _BTT_DROPW)
		//---- ドロップボタン
		p->ib.fpress = _PRESSF_DROP;
	else
	{
		//---- アイコンボタン側
	
		p->ib.fpress = _PRESSF_BTT;

		//チェック処理

		if(pion->flags & MICONBUTTONS_ITEMF_CHECKBUTTON)
			pion->flags ^= MICONBUTTONS_ITEMF_CHECKED;
		else if(pion->flags & MICONBUTTONS_ITEMF_CHECKGROUP)
			_set_check_group(pion);
	}

	mWidgetUpdate(M_WIDGET(p));
	mWidgetGrabPointer(M_WIDGET(p));
}

/** グラブ解放 */

static void _release_grab(mIconButtons *p,mBool send)
{
	if(p->ib.fpress)
	{
		mPoint pt;

		//通知

		if(send)
		{
			mIconButtonsItem *pi = (mIconButtonsItem *)p->ib.itemOn;
		
			mWidgetAppendEvent_command(mWidgetGetNotifyWidget(M_WIDGET(p)),
				pi->id, 0,
				(p->ib.fpress == _PRESSF_DROP)?
					MEVENT_COMMAND_BY_ICONBUTTONS_DROP: MEVENT_COMMAND_BY_ICONBUTTONS_BUTTON);
		}

		//

		p->ib.fpress = 0;

		mWidgetUngrabPointer(M_WIDGET(p));
		mWidgetUpdate(M_WIDGET(p));

		//離された場所で再度判定

		mWidgetGetCursorPos(M_WIDGET(p), &pt);

		_set_motion_on(p, pt.x, pt.y);
	}
}


//========================
// ハンドラ
//========================


/** イベント */

int mIconButtonsEventHandle(mWidget *wg,mEvent *ev)
{
	mIconButtons *p = M_ICONBUTTONS(wg);

	switch(ev->type)
	{
		case MEVENT_POINTER:
			if(ev->pt.type == MEVENT_POINTER_TYPE_MOTION)
			{
				//移動

				if(p->ib.fpress == 0)
					_set_motion_on(p, ev->pt.x, ev->pt.y);
			}
			else if(ev->pt.type == MEVENT_POINTER_TYPE_PRESS ||
				ev->pt.type == MEVENT_POINTER_TYPE_DBLCLK)
			{
				//押し

				if(ev->pt.btt == M_BTT_LEFT && p->ib.fpress == 0 && p->ib.itemOn)
					_event_press(p, ev);
			}
			else if(ev->pt.type == MEVENT_POINTER_TYPE_RELEASE)
			{
				//離し

				if(ev->pt.btt == M_BTT_LEFT)
					_release_grab(p, TRUE);
			}
			break;

		case MEVENT_ENTER:
			{
			mPoint pt;

			mWidgetGetCursorPos(wg, &pt);
			_set_motion_on(p, pt.x, pt.y);
			}
			break;
		case MEVENT_LEAVE:
			_set_motion_on(p, -1, -1);
			break;

		//タイマー
		case MEVENT_TIMER:
			if(p->ib.itemOn)
				_show_tooltip(p, (mIconButtonsItem *)p->ib.itemOn);

			mWidgetTimerDelete(wg, ev->timer.id);
			break;

		case MEVENT_CONSTRUCT:
			_set_buttons_info(p);
			mWidgetUpdate(M_WIDGET(p));
			break;

		case MEVENT_FOCUS:
			if(ev->focus.bOut)
				_release_grab(p, FALSE);
			break;

		default:
			return FALSE;
	}

	return TRUE;
}

/** 描画 */

void mIconButtonsDrawHandle(mWidget *wg,mPixbuf *pixbuf)
{
	mIconButtons *p = M_ICONBUTTONS(wg);
	mIconButtonsItem *pi;
	mImageList *imglist;
	int x,y,w,h;
	mSize szbtt;
	mBool bDisable,bPress,bON,bCheck;
	uint8_t pat_drop[5] = {0x1f,0x1f,0x0e,0x0e,0x04};

	imglist = p->ib.imglist;

	_get_button_normal_size(p, &szbtt);

	//背景

	mPixbufFillBox(pixbuf, 0, 0, wg->w, wg->h, MSYSCOL(FACE));

	//ボタン

	for(pi = _ITEM_TOP(p); pi; pi = _ITEM_NEXT(pi))
	{
		x = pi->x, y = pi->y;
		w = pi->w, h = pi->h;

		if(pi->flags & MICONBUTTONS_ITEMF_SEP)
		{
			//==== セパレータ

			if(_IS_VERT(p))
				mPixbufLineH(pixbuf, x + 2, y + h / 2, w - 4, MSYSCOL(TEXT));
			else
				mPixbufLineV(pixbuf, x + w / 2, y + 2, h - 4, MSYSCOL(TEXT));
		}
		else
		{
			//==== ボタン
		
			bON = ((void *)pi == p->ib.itemOn);
			bCheck = _isItemCheckON(pi);
			bDisable = ((pi->flags & MICONBUTTONS_ITEMF_DISABLE)
						|| !(p->wg.fState & MWIDGET_STATE_ENABLED));

			//---- ボタン

			bPress = (bON && p->ib.fpress == _PRESSF_BTT);

			if(bON || bCheck)
			{
				//背景

				mPixbufFillBox(pixbuf, x + 1, y + 1, szbtt.w - 2, h - 2,
					MSYSCOL(ICONBTT_FACE_SELECT));

				//枠

				mPixbufBox(pixbuf, x, y, szbtt.w, h,
					(bON)? MSYSCOL(ICONBTT_FRAME_SELECT): MSYSCOL(FRAME_DARK));
			}

			//アイコン

			bPress |= bCheck;

			mImageListPutPixbuf(imglist, pixbuf,
				x + _BTT_PADDING + bPress, y + _BTT_PADDING + bPress,
				pi->img, bDisable);

			//---- ドロップボックス

			if(pi->flags & MICONBUTTONS_ITEMF_DROPDOWN)
			{
				x += szbtt.w;

				bPress = (bON && p->ib.fpress == _PRESSF_DROP);

				if(bON)
				{
					//背景

					mPixbufFillBox(pixbuf, x, y, _BTT_DROPW, h, MSYSCOL(ICONBTT_FACE_SELECT));

					//枠

					mPixbufBox(pixbuf, x, y, _BTT_DROPW, h, MSYSCOL(ICONBTT_FRAME_SELECT));
				}

				//矢印

				mPixbufDrawBitPattern(pixbuf,
					x + (_BTT_DROPW - 5) / 2 + bPress, y + (h - 5) / 2 + bPress,
					pat_drop, 5, 5,
					(bDisable)? MSYSCOL(TEXT_DISABLE): MSYSCOL(TEXT));
			}
		}
	}

	//セパレータ

	if(p->ib.style & MICONBUTTONS_S_SEP_BOTTOM)
		mPixbufLineH(pixbuf, 0, wg->h - 3, wg->w, MSYSCOL(TEXT));
}

/** @} */
