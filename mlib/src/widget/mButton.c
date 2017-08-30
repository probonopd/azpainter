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
 * mButton  [ボタン]
 *****************************************/

#include <string.h>

#include "mDef.h"

#include "mButton.h"
#include "mWidget.h"
#include "mFont.h"
#include "mPixbuf.h"
#include "mEvent.h"
#include "mSysCol.h"
#include "mKeyDef.h"


//-----------------------

#define BTT_DEFW  64
#define BTT_DEFH  21

enum MBUTTON_FLAGS
{
	MBUTTON_FLAGS_PRESSED = 1<<0,
	MBUTTON_FLAGS_GRAB    = 1<<1
};

//-----------------------


/******************//**

@defgroup button mButton 
@brief ボタン

<h3>継承</h3>
mWidget \> mButton
 
@ingroup group_widget

@{

@file mButton.h
@struct _mButton
@struct mButtonData
@enum MBUTTON_STYLE
@enum MBUTTON_NOTIFY

@var MBUTTON_NOTIFY::MBUTTON_N_PRESS
ボタンが押された

********************/


//==========================


/** グラブ状態解除 */

static void _grab_release(mButton *p,mBool pressed)
{
	//押し状態解除

	if(p->btt.flags & MBUTTON_FLAGS_GRAB)
	{
		mWidgetUngrabPointer(M_WIDGET(p));

		p->btt.flags &= ~MBUTTON_FLAGS_GRAB;
		p->btt.press_key = 0;

		mButtonSetPress(p, FALSE);
	}
	
	//ハンドラ実行
	
	if(pressed)
		(p->btt.onPressed)(p);
}

/** ボタン/キー押し時 */

static void _pressed(mButton  *p)
{
	p->btt.flags |= MBUTTON_FLAGS_GRAB;
	
	mButtonSetPress(p, TRUE);
}

/** onPress() デフォルト */

static void _handle_pressed(mButton  *p)
{
	mWidgetAppendEvent_notify(NULL, M_WIDGET(p), MBUTTON_N_PRESS, 0, 0);
}


//==========================


/** 解放処理 */

void mButtonDestroyHandle(mWidget *p)
{
	mButton *btt = M_BUTTON(p);

	M_FREE_NULL(btt->btt.text);
}

/** ボタン作成 */

mButton *mButtonCreate(mWidget *parent,int id,uint32_t style,
	uint32_t fLayout,uint32_t marginB4,const char *text)
{
	mButton *p;

	p = mButtonNew(0, parent, style);
	if(!p) return NULL;

	p->wg.id = id;
	p->wg.fLayout = fLayout;

	mWidgetSetMargin_b4(M_WIDGET(p), marginB4);

	mButtonSetText(p, text);

	return p;
}

/** ボタン作成 */

mButton *mButtonNew(int size,mWidget *parent,uint32_t style)
{
	mButton *p;
	
	if(size < sizeof(mButton)) size = sizeof(mButton);
	
	p = (mButton *)mWidgetNew(size, parent);
	if(!p) return NULL;
	
	p->wg.destroy = mButtonDestroyHandle;
	p->wg.calcHint = mButtonCalcHintHandle;
	p->wg.draw = mButtonDrawHandle;
	p->wg.event = mButtonEventHandle;
	
	p->wg.fState |= MWIDGET_STATE_TAKE_FOCUS;
	p->wg.fEventFilter |= MWIDGET_EVENTFILTER_POINTER | MWIDGET_EVENTFILTER_KEY;
	p->wg.fAcceptKeys = MWIDGET_ACCEPTKEY_ENTER;

	p->btt.style = style;
	p->btt.onPressed = _handle_pressed;
	
	return p;
}

/** テキストセット */

void mButtonSetText(mButton *p,const char *text)
{
	mFree(p->btt.text);

	p->btt.text = mStrdup(text);
	
	mWidgetCalcHintSize(M_WIDGET(p));
	mWidgetUpdate(M_WIDGET(p));
}

/** 押し状態変更 */

void mButtonSetPress(mButton *p,mBool press)
{
	int now;
	
	now = ((p->btt.flags & MBUTTON_FLAGS_PRESSED) != 0);
	
	if(!now != !press)
	{
		if(press)
			p->btt.flags |= MBUTTON_FLAGS_PRESSED;
		else
			p->btt.flags &= ~MBUTTON_FLAGS_PRESSED;
		
		mWidgetUpdate(M_WIDGET(p));
	}
}

/** 押された状態か */

mBool mButtonIsPressed(mButton *p)
{
	return ((p->btt.flags & MBUTTON_FLAGS_PRESSED) != 0);
}

/** ボタンのベースを描画 */

void mButtonDrawBase(mButton *p,mPixbuf *pixbuf,mBool pressed)
{
	uint32_t flags = 0;

	if(pressed) flags |= MPIXBUF_DRAWBTT_PRESS;
	if(p->wg.fState & MWIDGET_STATE_FOCUSED) flags |= MPIXBUF_DRAWBTT_FOCUS;
	if(!(p->wg.fState & MWIDGET_STATE_ENABLED)) flags |= MPIXBUF_DRAWBTT_DISABLE;
	if(p->wg.fState & MWIDGET_STATE_ENTER_DEFAULT) flags |= MPIXBUF_DRAWBTT_DEFAULT_BUTTON;
	
	mPixbufDrawButton(pixbuf, 0, 0, p->wg.w, p->wg.h, flags);
}


//========================
// ハンドラ
//========================


/** サイズ計算 */

void mButtonCalcHintHandle(mWidget *p)
{
	mButton *btt = M_BUTTON(p);
	mFont *font = mWidgetGetFont(p);
	int w,h;

	//幅

	w = mFontGetTextWidth(font, btt->btt.text, -1);
	
	btt->btt.textW = w;
	
	if(btt->btt.style & MBUTTON_S_REAL_W)
		w += 8;
	else
	{
		w += 10;

		if(w < BTT_DEFW)
		{
			w = BTT_DEFW;
			if((w - btt->btt.textW) & 1) w++;
		}
	}

	//高さ
	
	h = font->height;
		
	if(btt->btt.style & MBUTTON_S_REAL_H)
		h += 6;
	else
	{
		h += 8;
	
		if(h < BTT_DEFH)
		{
			h = BTT_DEFH;
			if((h - font->height) & 1) h++;
		}
	}

	//REAL_W 時は高さが最小

	if((btt->btt.style & MBUTTON_S_REAL_W) && w < h)
		w = h;
	
	p->hintW = w;
	p->hintH = h;
}

/** イベント */

int mButtonEventHandle(mWidget *wg,mEvent *ev)
{
	mButton *btt = M_BUTTON(wg);

	switch(ev->type)
	{
		case MEVENT_POINTER:
			if(ev->pt.type == MEVENT_POINTER_TYPE_PRESS
				|| ev->pt.type == MEVENT_POINTER_TYPE_DBLCLK)
			{
				//押し
			
				if(ev->pt.btt == M_BTT_LEFT
					&& !(btt->btt.flags & MBUTTON_FLAGS_GRAB))
				{
					mWidgetSetFocus_update(wg, FALSE);
					mWidgetGrabPointer(wg);
					
					_pressed(btt);
				}
			}
			else if(ev->pt.type == MEVENT_POINTER_TYPE_RELEASE)
			{
				//離し
			
				if(ev->pt.btt == M_BTT_LEFT
					&& (btt->btt.flags & MBUTTON_FLAGS_GRAB)
					&& btt->btt.press_key == 0)
					_grab_release(btt, TRUE);
			}
			return TRUE;

		//キー
		case MEVENT_KEYDOWN:
			if((ev->key.code == MKEY_SPACE || ev->key.code == MKEY_ENTER)
				&& !(btt->btt.flags & MBUTTON_FLAGS_GRAB))
			{
				btt->btt.press_key = ev->key.code;
				
				_pressed(btt);

				return TRUE;
			}
			break;
		case MEVENT_KEYUP:
			if((btt->btt.flags & MBUTTON_FLAGS_GRAB)
				&& btt->btt.press_key == ev->key.code)
			{
				_grab_release(btt, TRUE);
				return TRUE;
			}
			break;
		
		case MEVENT_FOCUS:
			//フォーカスアウト時、グラブ状態解除
			if(ev->focus.bOut)
				_grab_release(btt, FALSE);
					
			mWidgetUpdate(wg);
			return TRUE;
	}

	return FALSE;
}

/** 描画 */

void mButtonDrawHandle(mWidget *p,mPixbuf *pixbuf)
{
	mButton *btt = M_BUTTON(p);
	mFont *font;
	int tx,ty,press;

	font = mWidgetGetFont(p);
	
	press = ((btt->btt.flags & MBUTTON_FLAGS_PRESSED) != 0);

	//テキスト位置
	
	tx = (p->w - btt->btt.textW) >> 1;
	ty = (p->h - font->height) >> 1;
	
	if(press) tx++, ty++;
	
	//ボタン
	
	mButtonDrawBase(btt, pixbuf, press);
	
	//テキスト
	
	mFontDrawText(font, pixbuf, tx, ty, btt->btt.text, -1,
		(p->fState & MWIDGET_STATE_ENABLED)? MSYSCOL_RGB(TEXT): MSYSCOL_RGB(TEXT_DISABLE));
}

/** @} */
