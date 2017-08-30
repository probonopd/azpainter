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
 * mInputAccelKey
 *****************************************/

#include "mDef.h"

#include "mInputAccelKey.h"

#include "mWidget.h"
#include "mPixbuf.h"
#include "mFont.h"
#include "mEvent.h"
#include "mSysCol.h"
#include "mKeyDef.h"
#include "mAccelerator.h"


#define _PADDING_X  3
#define _PADDING_Y  3


/******************//**

@defgroup inputaccelkey mInputAccelKey
@brief アクセラレータキーの入力
@ingroup group_widget

<h3>継承</h3>
mWidget \> mInputAccelKey

@{

@file mInputAccelKey.h
@def M_INPUTACCELKEY(p)
@struct mInputAccelKeyData
@struct mInputAccelKey
@enum MINPUTACCELKEY_STYLE
@enum MINPUTACCELKEY_NOTIFY

@var MINPUTACCELKEY_STYLE::MINPUTACCELKEY_S_NOTIFY_CHANGE
キーが変更された時、MINPUTACCELKEY_N_CHANGE_KEY 通知を送るようにする。

@var MINPUTACCELKEY_NOTIFY::MINPUTACCELKEY_N_CHANGE_KEY
キーが変更された時。\n
装飾キーのみの場合は 0 となる。\n
param1 : キーコード

***********************/


/** 解放処理 */

void mInputAccelKeyDestroyHandle(mWidget *p)
{
	mFree(M_INPUTACCELKEY(p)->ak.text);
}

/** 作成 */

mInputAccelKey *mInputAccelKeyNew(int size,mWidget *parent,uint32_t style)
{
	mInputAccelKey *p;
	
	if(size < sizeof(mInputAccelKey)) size = sizeof(mInputAccelKey);
	
	p = (mInputAccelKey *)mWidgetNew(size, parent);
	if(!p) return NULL;
	
	p->wg.destroy = mInputAccelKeyDestroyHandle;
	p->wg.calcHint = mInputAccelKeyCalcHintHandle;
	p->wg.draw = mInputAccelKeyDrawHandle;
	p->wg.event = mInputAccelKeyEventHandle;
	
	p->wg.fState |= MWIDGET_STATE_TAKE_FOCUS;
	p->wg.fEventFilter |= MWIDGET_EVENTFILTER_POINTER | MWIDGET_EVENTFILTER_KEY;
	p->wg.fAcceptKeys = MWIDGET_ACCEPTKEY_ALL;

	p->ak.style = style;

	return p;
}

/** 作成 */

mInputAccelKey *mInputAccelKeyCreate(mWidget *parent,int id,uint32_t style,uint32_t fLayout,uint32_t marginb4)
{
	mInputAccelKey *p;

	p = mInputAccelKeyNew(0, parent, style);
	if(!p) return NULL;

	p->wg.id = id;
	p->wg.fLayout = fLayout;

	mWidgetSetMargin_b4(M_WIDGET(p), marginb4);

	return p;
}

/** キーを取得 */

uint32_t mInputAccelKey_getKey(mInputAccelKey *p)
{
	return p->ak.key;
}

/** キーをセット
 *
 * @param key 0 でクリア */

void mInputAccelKey_setKey(mInputAccelKey *p,uint32_t key)
{
	if(p->ak.key != key)
	{
		M_FREE_NULL(p->ak.text);

		p->ak.key = key;
		if(key) p->ak.text = mAcceleratorGetKeyText(key);

		mWidgetUpdate(M_WIDGET(p));
	}
}


//========================
// ハンドラ
//========================


/** サイズ計算 */

void mInputAccelKeyCalcHintHandle(mWidget *wg)
{
	wg->hintW = 10;
	wg->hintH = mWidgetGetFontHeight(wg) + _PADDING_Y * 2;
}

/** キー離し時
 *
 * 押し時にセットすると、キーリピートで何度も実行されるので、
 * 押し時にフラグ ON -> 離した時に確定処理。 */

static void _event_keyup(mInputAccelKey *p,mEvent *ev)
{
	uint32_t k;

	if(p->ak.bKeyDown)
	{
		//装飾キーのみの場合はクリア

		k = p->ak.key & MACCKEY_KEYMASK;

		if(k == MKEY_CONTROL || k == MKEY_SHIFT || k == MKEY_ALT)
			mInputAccelKey_setKey(p, 0);

		//通知

		if(p->ak.keyprev != p->ak.key
			&& (p->ak.style & MINPUTACCELKEY_S_NOTIFY_CHANGE))
		{
			mWidgetAppendEvent_notify(NULL, M_WIDGET(p),
				MINPUTACCELKEY_N_CHANGE_KEY, p->ak.key, 0);
		}

		//

		p->ak.bKeyDown = FALSE;
	}
}

/** イベント */

int mInputAccelKeyEventHandle(mWidget *wg,mEvent *ev)
{
	mInputAccelKey *p = M_INPUTACCELKEY(wg);

	switch(ev->type)
	{
		case MEVENT_POINTER:
			//左ボタン押しでフォーカスセット
			if(ev->pt.type == MEVENT_POINTER_TYPE_PRESS
				&& ev->pt.btt == M_BTT_LEFT)
				mWidgetSetFocus_update(wg, FALSE);
			break;

		//キー押し
		case MEVENT_KEYDOWN:
			p->ak.keyprev = p->ak.key;
			p->ak.bKeyDown = TRUE;
			
			mInputAccelKey_setKey(p, mAcceleratorGetKeyFromEvent(ev));
			break;
		//キー離し
		case MEVENT_KEYUP:
			_event_keyup(p, ev);
			break;
		
		case MEVENT_FOCUS:
			//キー押し中にフォーカスアウトした時
			if(ev->focus.bOut)
				_event_keyup(p, ev);
		
			mWidgetUpdate(wg);
			break;
		default:
			return FALSE;
	}

	return TRUE;
}

/** 描画 */

void mInputAccelKeyDrawHandle(mWidget *wg,mPixbuf *pixbuf)
{
	mInputAccelKey *p = M_INPUTACCELKEY(wg);
	mFont *font;
	int tx,tw;
	
	font = mWidgetGetFont(wg);

	//背景

	mPixbufFillBox(pixbuf, 0, 0, wg->w, wg->h, MSYSCOL(FACE_LIGHTEST));

	//枠

	mPixbufBox(pixbuf, 0, 0, wg->w, wg->h,
		(wg->fState & MWIDGET_STATE_FOCUSED)? MSYSCOL(FRAME_FOCUS): MSYSCOL(FRAME_DARK));

	//キーテキスト

	if(!p->ak.text)
		tx = wg->w >> 1, tw = 0;
	else
	{
		mPixbufSetClipBox_d(pixbuf, _PADDING_X, 0, wg->w - _PADDING_X * 2, wg->h);

		tw = mFontGetTextWidth(font, p->ak.text, -1);
		tx = (wg->w - tw) >> 1;

		mFontDrawText(font, pixbuf, tx, _PADDING_Y,
			p->ak.text, -1, MSYSCOL_RGB(TEXT));
	}

	//カーソル

	if(wg->fState & MWIDGET_STATE_FOCUSED)
		mPixbufLineV(pixbuf, tx + tw, _PADDING_Y, font->height, MSYSCOL(TEXT)); 
}

/** @} */
