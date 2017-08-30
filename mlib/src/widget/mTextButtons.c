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
 * mTextButtons
 *****************************************/

#include "mDef.h"

#include "mTextButtons.h"

#include "mList.h"
#include "mWidget.h"
#include "mPixbuf.h"
#include "mFont.h"
#include "mEvent.h"
#include "mSysCol.h"
#include "mKeyDef.h"
#include "mUtilStr.h"
#include "mTrans.h"


//------------------------

#define _PADDING_X  4
#define _PADDING_Y  3

#define _FLAGS_PRESS_BTT 1
#define _FLAGS_PRESS_KEY 2

#define _IS_PRESSED(p)  ((p)->tb.flags & (_FLAGS_PRESS_BTT|_FLAGS_PRESS_KEY))
#define _ITEM(p)        ((mTextButtonsItem *)(p))

typedef struct
{
	mListItem i;
	char *label;
	int w;
}mTextButtonsItem;

//------------------------


/**
@defgroup textbuttons mTextButtons
@brief 複数のテキストボタン

<h3>継承</h3>
mWidget \> mTextButtons

@ingroup group_widget
@{

@file mTextButtons.h
@def M_TEXTBUTTONS(p)
@struct mTextButtons
@struct mTextButtonsData
@enum MTEXTBUTTONS_NOTIFY

@var MTEXTBUTTONS_NOTIFY::MTEXTBUTTONS_N_PRESS
ボタンが押された時。 \n
param1 : 押されたボタンのインデックス番号 (0〜)
*/


//====================
// sub
//====================


/** アイテム削除時 */

static void _item_destroy(mListItem *p)
{
	mFree(_ITEM(p)->label);
}

/** アイテム追加 */

static mTextButtonsItem *_add_item(mTextButtons *p,const char *label,mFont *font)
{
	mTextButtonsItem *pi;

	pi = (mTextButtonsItem *)mListAppendNew(&p->tb.list,
			sizeof(mTextButtonsItem), _item_destroy);

	if(pi)
	{
		pi->label = mStrdup(label);
		pi->w = mFontGetTextWidth(font, label, -1) + _PADDING_X * 2;
	}

	return pi;
}

/** 押し状態解除 */

static void _release_press(mTextButtons *p,mBool notify)
{
	if(_IS_PRESSED(p))
	{
		p->tb.flags &= ~(_FLAGS_PRESS_BTT | _FLAGS_PRESS_KEY);

		mWidgetUngrabPointer(M_WIDGET(p));
		mWidgetUpdate(M_WIDGET(p));

		//通知

		if(notify)
		{
			mWidgetAppendEvent_notify(NULL, M_WIDGET(p),
				MTEXTBUTTONS_N_PRESS, p->tb.focusno, 0);
		}
	}
}


//====================


/** 解放処理 */

void mTextButtonsDestroyHandle(mWidget *wg)
{
	mListDeleteAll(&M_TEXTBUTTONS(wg)->tb.list);
}

/** 作成 */

mTextButtons *mTextButtonsNew(int size,mWidget *parent,uint32_t style)
{
	mTextButtons *p;
	
	if(size < sizeof(mTextButtons)) size = sizeof(mTextButtons);
	
	p = (mTextButtons *)mWidgetNew(size, parent);
	if(!p) return NULL;
	
	p->wg.destroy = mTextButtonsDestroyHandle;
	p->wg.calcHint = mTextButtonsCalcHintHandle;
	p->wg.draw = mTextButtonsDrawHandle;
	p->wg.event = mTextButtonsEventHandle;
	
	p->wg.fState |= MWIDGET_STATE_TAKE_FOCUS;
	p->wg.fEventFilter |= MWIDGET_EVENTFILTER_POINTER | MWIDGET_EVENTFILTER_KEY;
	p->wg.fAcceptKeys = MWIDGET_ACCEPTKEY_ENTER;

	return p;
}

/** すべてのボタンを削除 */

void mTextButtonsDeleteAll(mTextButtons *p)
{
	mListDeleteAll(&p->tb.list);

	p->tb.focusno = 0;
}

/** ボタンを追加 */

void mTextButtonsAddButton(mTextButtons *p,const char *text)
{
	_add_item(p, text, mWidgetGetFont(M_WIDGET(p)));

	mWidgetCalcHintSize(M_WIDGET(p));
	mWidgetUpdate(M_WIDGET(p));
}

/** 連続した文字列IDからボタンを追加 */

void mTextButtonsAddButtonsTr(mTextButtons *p,int idtop,int num)
{
	int i;
	mFont *font;

	font = mWidgetGetFont(M_WIDGET(p));

	for(i = 0; i < num; i++)
		_add_item(p, M_TR_T(idtop + i), font);

	mWidgetCalcHintSize(M_WIDGET(p));
	mWidgetUpdate(M_WIDGET(p));
}


//========================
// ハンドラ
//========================


/** サイズ計算 */

void mTextButtonsCalcHintHandle(mWidget *wg)
{
	mTextButtons *p = (mTextButtons *)wg;
	mTextButtonsItem *pi;
	int w = 1;

	for(pi = _ITEM(p->tb.list.top); pi; pi = _ITEM(pi->i.next))
		w += pi->w - 1;

	wg->hintW = w;
	wg->hintH = mWidgetGetFontHeight(wg) + _PADDING_Y * 2;
}

/** キー押し */

static void _event_keydown(mTextButtons *p,mEvent *ev)
{
	mBool update = FALSE;

	switch(ev->key.code)
	{
		case MKEY_LEFT:
			if(p->tb.focusno)
			{
				p->tb.focusno--;
				update = TRUE;
			}
			break;
		case MKEY_RIGHT:
			if(p->tb.focusno < p->tb.list.num - 1)
			{
				p->tb.focusno++;
				update = TRUE;
			}
			break;
		case MKEY_SPACE:
		case MKEY_ENTER:
			p->tb.flags |= _FLAGS_PRESS_KEY;
			p->tb.presskey = ev->key.code;
			update = TRUE;
			break;
	}

	if(update)
		mWidgetUpdate(M_WIDGET(p));
}

/** ボタン押し */

static void _event_btt_press(mTextButtons *p,mEvent *ev)
{
	mTextButtonsItem *pi;
	int x,no;

	mWidgetSetFocus_update(M_WIDGET(p), FALSE);

	//ボタン

	x = no = 0;

	for(pi = _ITEM(p->tb.list.top); pi; pi = _ITEM(pi->i.next), no++)
	{
		if(x <= ev->pt.x && ev->pt.x < x + pi->w) break;

		x += pi->w - 1;
	}

	//押し

	if(pi)
	{
		p->tb.focusno = no;
		p->tb.flags |= _FLAGS_PRESS_BTT;

		mWidgetGrabPointer(M_WIDGET(p));
		mWidgetUpdate(M_WIDGET(p));
	}
}

/** イベント */

int mTextButtonsEventHandle(mWidget *wg,mEvent *ev)
{
	mTextButtons *p = M_TEXTBUTTONS(wg);

	switch(ev->type)
	{
		case MEVENT_POINTER:
			if(ev->pt.type == MEVENT_POINTER_TYPE_PRESS
				|| ev->pt.type == MEVENT_POINTER_TYPE_DBLCLK)
			{
				//押し
				
				if(ev->pt.btt == M_BTT_LEFT && !_IS_PRESSED(p))
					_event_btt_press(p, ev);
			}
			else if(ev->pt.type == MEVENT_POINTER_TYPE_RELEASE)
			{
				//離し
				
				if(ev->pt.btt == M_BTT_LEFT && (p->tb.flags & _FLAGS_PRESS_BTT))
					_release_press(p, TRUE);
			}
			break;

		//キー
		case MEVENT_KEYDOWN:
			if(!_IS_PRESSED(p))
				_event_keydown(p, ev);
			break;
		case MEVENT_KEYUP:
			if((p->tb.flags & _FLAGS_PRESS_KEY)
				&& p->tb.presskey == ev->key.code)
				_release_press(p, TRUE);
			break;
		
		case MEVENT_FOCUS:
			if(ev->focus.bOut)
				_release_press(p, FALSE);
			
			mWidgetUpdate(wg);
			break;
		default:
			return FALSE;
	}

	return TRUE;
}

/** 描画 */

void mTextButtonsDrawHandle(mWidget *wg,mPixbuf *pixbuf)
{
	mTextButtons *p = M_TEXTBUTTONS(wg);
	mFont *font;
	mTextButtonsItem *pi;
	int x,y,no;
	uint32_t colframe,coltext;
	mBool bPress;

	font = mWidgetGetFont(wg);

	//背景

	mPixbufFillBox(pixbuf, 0, 0, wg->w, wg->h, MSYSCOL(FACE_DARK));

	//枠色・テキスト色

	if(!(wg->fState & MWIDGET_STATE_ENABLED))
	{
		colframe = MSYSCOL(FRAME_LIGHT);
		coltext = MSYSCOL_RGB(TEXT_DISABLE);
	}
	else
	{
		colframe = (wg->fState & MWIDGET_STATE_FOCUSED)? MSYSCOL(FRAME_FOCUS): MSYSCOL(FRAME);
		coltext = MSYSCOL_RGB(TEXT);
	}

	//ボタン

	x = 0;
	y = (wg->h - font->height) >> 1;

	for(pi = _ITEM(p->tb.list.top), no = 0; pi; pi = _ITEM(pi->i.next), no++)
	{
		bPress = (no == p->tb.focusno && _IS_PRESSED(p));

		//枠
	
		if(bPress)
		{
			mPixbufLineH(pixbuf, x, 0, pi->w, colframe);
			mPixbufLineV(pixbuf, x, 1, wg->h - 1, colframe);
		}
		else
			mPixbufBox(pixbuf, x, 0, pi->w, wg->h, colframe);

		//テキスト

		mFontDrawText(font, pixbuf,
			x + _PADDING_X + bPress, y + bPress, pi->label, -1, coltext);

		//フォーカス枠

		if(no == p->tb.focusno && !bPress && (wg->fState & MWIDGET_STATE_FOCUSED))
			mPixbufBoxDash(pixbuf, x + 2, 2, pi->w - 4, wg->h - 4, MSYSCOL(TEXT));

		x += pi->w - 1;
	}
}

/** @} */
