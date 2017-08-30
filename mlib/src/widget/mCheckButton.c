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
 * mCheckButton [チェックボタン]
 *****************************************/

#include "mDef.h"

#include "mCheckButton.h"
#include "mWidget.h"
#include "mFont.h"
#include "mPixbuf.h"
#include "mEvent.h"
#include "mSysCol.h"
#include "mKeyDef.h"

#include "mWidget_pv.h"


//-----------------------

#define _CHECKBOXSIZE 13
#define _TEXTSPACE    5		//チェックボックスとテキストの間隔
#define _PADDINGX     2
#define _PADDINGY     2


enum MCHECKBTT_FLAGS
{
	MCHECKBTT_FLAGS_CHECKED  = 1<<0,
	MCHECKBTT_FLAGS_GRAB_PT  = 1<<1,
	MCHECKBTT_FLAGS_GRAB_KEY = 1<<2
};

//-----------------------


/*************************//**

@defgroup checkbutton mCheckButton
@brief チェックボタン

テキスト無しも可。\n
'\\n' で改行可能。

<h3>継承</h3>
mWidget \> mCheckButton

@ingroup group_widget
@{

@file mCheckButton.h

@def M_CHECKBUTTON(p)
@struct mCheckButtonData
@struct mCheckButton
@enum MCHECKBUTTON_STYLE
@enum MCHECKBUTTON_NOTIFY

@var MCHECKBUTTON_STYLE::MCHECKBUTTON_S_RADIO_GROUP
複数のグループが連続して並んでいる場合に、各ラジオグループの先頭に付ける。\n
グループの間に他のウィジェットが存在する場合は必要ない。

@var MCHECKBUTTON_NOTIFY::MCHECKBUTTON_N_PRESS
ボタンが押された。\n
param1 : チェックされているか

*****************************/


//==========================


/** グラブ状態解除 */

static void _grab_release(mCheckButton *p,mBool notify)
{
	if(p->ck.flags & (MCHECKBTT_FLAGS_GRAB_PT | MCHECKBTT_FLAGS_GRAB_KEY))
	{
		mWidgetUngrabPointer(M_WIDGET(p));

		p->ck.flags &= ~(MCHECKBTT_FLAGS_GRAB_PT | MCHECKBTT_FLAGS_GRAB_KEY);
	}
	
	//通知
	
	if(notify)
	{
		mWidgetAppendEvent_notify(NULL, M_WIDGET(p),
			MCHECKBUTTON_N_PRESS, mCheckButtonIsChecked(p), 0);
	}
}


//==========================


/** 解放処理 */

void mCheckButtonDestroyHandle(mWidget *wg)
{
	mCheckButton *p = M_CHECKBUTTON(wg);

	M_FREE_NULL(p->ck.text);
	M_FREE_NULL(p->ck.rowbuf);
}

/** チェックボタン作成 */

mCheckButton *mCheckButtonCreate(mWidget *parent,int id,
	uint32_t style,uint32_t fLayout,uint32_t marginB4,const char *text,int checked)
{
	mCheckButton *p;

	p = mCheckButtonNew(0, parent, style);
	if(!p) return NULL;

	p->wg.id = id;
	p->wg.fLayout = fLayout;

	mWidgetSetMargin_b4(M_WIDGET(p), marginB4);

	mCheckButtonSetText(p, text);

	if(checked)
		p->ck.flags |= MCHECKBTT_FLAGS_CHECKED;

	return p;
}

/** チェックボタン作成 */

mCheckButton *mCheckButtonNew(int size,mWidget *parent,uint32_t style)
{
	mCheckButton *p;
	
	if(size < sizeof(mCheckButton)) size = sizeof(mCheckButton);
	
	p = (mCheckButton *)mWidgetNew(size, parent);
	if(!p) return NULL;
	
	p->wg.destroy = mCheckButtonDestroyHandle;
	p->wg.calcHint = mCheckButtonCalcHintHandle;
	p->wg.draw = mCheckButtonDrawHandle;
	p->wg.event = mCheckButtonEventHandle;
	
	p->wg.fState |= MWIDGET_STATE_TAKE_FOCUS;
	p->wg.fType |= MWIDGET_TYPE_CHECKBUTTON;
	p->wg.fEventFilter |= MWIDGET_EVENTFILTER_POINTER | MWIDGET_EVENTFILTER_KEY;

	p->ck.style = style;
	
	return p;
}

/** テキストセット */

void mCheckButtonSetText(mCheckButton *p,const char *text)
{
	mCheckButtonDestroyHandle(M_WIDGET(p));

	if(text)
	{
		p->ck.text = mStrdup(text);
		p->ck.rowbuf = __mWidgetGetLabelTextRowInfo(text);
	}
	
	mWidgetCalcHintSize(M_WIDGET(p));
	mWidgetUpdate(M_WIDGET(p));
}

/** 状態変更
 * 
 * @param type [0]OFF [正]ON [負]反転 */

void mCheckButtonSetState(mCheckButton *p,int type)
{
	int f;
	mCheckButton *sel;
	
	if(p->ck.style & MCHECKBUTTON_S_RADIO)
	{
		//ラジオの場合、現在の選択を OFF に
	
		sel = mCheckButtonGetRadioInfo(p, NULL, NULL);
		
		if(type && sel)
		{
			if(sel == p) return;
		
			sel->ck.flags &= ~MCHECKBTT_FLAGS_CHECKED;
			mWidgetUpdate(M_WIDGET(sel));
		}
	}
	else
	{
		f = ((p->ck.flags & MCHECKBTT_FLAGS_CHECKED) != 0);

		if(type < 0) type = !f;

		if(!f == !type) return;
	}
	
	//
	
	if(type)
		p->ck.flags |= MCHECKBTT_FLAGS_CHECKED;
	else
		p->ck.flags &= ~MCHECKBTT_FLAGS_CHECKED;
	
	mWidgetUpdate(M_WIDGET(p));
}

/** ラジオボタンの選択をセット
 *
 * @param p グループ内のどのウィジェットでもOK */

void mCheckButtonSetRadioSel(mCheckButton *p,int no)
{
	mCheckButton *top,*end;
	int i;

	mCheckButtonGetRadioInfo(p, &top, &end);

	if(top && end)
	{
		for(i = 0, p = top; p; i++, p = M_CHECKBUTTON(p->wg.next))
		{
			if(i == no)
			{
				mCheckButtonSetState(p, 1);
				break;
			}
		
			if(p == end) break;
		}
	}
}

/** チェックされているか */

mBool mCheckButtonIsChecked(mCheckButton *p)
{
	return ((p->ck.flags & MCHECKBTT_FLAGS_CHECKED) != 0);
}

/** ラジオグループの範囲と現在の選択取得
 * 
 * @return 現在の選択 */

mCheckButton *mCheckButtonGetRadioInfo(mCheckButton *start,
	mCheckButton **ppTop,mCheckButton **ppEnd)
{
	mWidget *p,*p2,*top,*end;
	
	//先頭を検索
	
	for(p = M_WIDGET(start); p; p = p2)
	{
		p2 = p->prev;
	
		if(!p2 || !(p2->fType & MWIDGET_TYPE_CHECKBUTTON)
			|| (M_CHECKBUTTON(p)->ck.style & MCHECKBUTTON_S_RADIO_GROUP)
			|| !(M_CHECKBUTTON(p2)->ck.style & MCHECKBUTTON_S_RADIO))
			break;
	}
	
	top = p;
	
	if(ppTop) *ppTop = M_CHECKBUTTON(top);

	//終端を検索

	for(p = M_WIDGET(start); p; p = p2)
	{
		p2 = p->next;
	
		if(!p2 || !(p2->fType & MWIDGET_TYPE_CHECKBUTTON)
			|| (M_CHECKBUTTON(p2)->ck.style & MCHECKBUTTON_S_RADIO_GROUP)
			|| !(M_CHECKBUTTON(p2)->ck.style & MCHECKBUTTON_S_RADIO))
			break;
	}
	
	end = p;

	if(ppEnd) *ppEnd = M_CHECKBUTTON(end);
	
	//選択
	
	for(p = top, p2 = NULL; 1; p = p->next)
	{
		if(M_CHECKBUTTON(p)->ck.flags & MCHECKBTT_FLAGS_CHECKED)
		{
			p2 = p;
			break;
		}
		
		if(p == end) break;
	}
	
	return (mCheckButton *)p2;
}

/** 現在選択されているラジオのインデックス番号取得
 * 
 * @return 0〜。-1 で選択なし */

int mCheckButtonGetGroupSelIndex(mCheckButton *p)
{
	mCheckButton *sel,*top;
	int i;
	
	if(p->ck.style & MCHECKBUTTON_S_RADIO)
	{
		sel = mCheckButtonGetRadioInfo(p, &top, NULL);
		
		if(!sel)
			return -1;
		else
		{
			for(i = 0; top && top != sel; i++, top = M_CHECKBUTTON(top->wg.next));
			return i;
		}
	}
	
	return -1;
}


//========================
// ハンドラ
//========================


/** サイズ計算 */

void mCheckButtonCalcHintHandle(mWidget *wg)
{
	mCheckButton *p = M_CHECKBUTTON(wg);
	mSize size;

	__mWidgetGetLabelTextSize(wg, p->ck.text, p->ck.rowbuf, &size);

	p->ck.sztext = size;

	//

	if(size.w) size.w += _TEXTSPACE;
	size.w += _CHECKBOXSIZE + _PADDINGX * 2;

	if(size.h < _CHECKBOXSIZE) size.h = _CHECKBOXSIZE;
	size.h += _PADDINGY * 2;
	
	wg->hintW = size.w;
	wg->hintH = size.h;
}

/** イベント */

int mCheckButtonEventHandle(mWidget *wg,mEvent *ev)
{
	mCheckButton *p = M_CHECKBUTTON(wg);

	switch(ev->type)
	{
		case MEVENT_POINTER:
			if(ev->pt.type == MEVENT_POINTER_TYPE_PRESS
				|| ev->pt.type == MEVENT_POINTER_TYPE_DBLCLK)
			{
				//押し
			
				if(ev->pt.btt == M_BTT_LEFT
					&& !(p->ck.flags & (MCHECKBTT_FLAGS_GRAB_PT | MCHECKBTT_FLAGS_GRAB_KEY)))
				{
					p->ck.flags |= MCHECKBTT_FLAGS_GRAB_PT;
				
					mWidgetSetFocus_update(wg, FALSE);
					mWidgetGrabPointer(wg);
					
					mCheckButtonSetState(p, -1);
				}
			}
			else if(ev->pt.type == MEVENT_POINTER_TYPE_RELEASE)
			{
				//離し
			
				if(ev->pt.btt == M_BTT_LEFT
					&& (p->ck.flags & MCHECKBTT_FLAGS_GRAB_PT))
					_grab_release(p, TRUE);
			}
			break;

		//キー
		case MEVENT_KEYDOWN:
			if(ev->key.code == MKEY_SPACE
				&& !(p->ck.flags & (MCHECKBTT_FLAGS_GRAB_PT | MCHECKBTT_FLAGS_GRAB_KEY)))
			{
				p->ck.flags |= MCHECKBTT_FLAGS_GRAB_KEY;
				
				mCheckButtonSetState(p, -1);
			}
			break;
		case MEVENT_KEYUP:
			if(ev->key.code == MKEY_SPACE
				&& (p->ck.flags & MCHECKBTT_FLAGS_GRAB_KEY))
				_grab_release(p, TRUE);
			break;
		
		case MEVENT_FOCUS:
			//フォーカスアウト時、グラブ状態解除
			if(ev->focus.bOut)
				_grab_release(p, FALSE);
					
			mWidgetUpdate(wg);
			break;
		default:
			return FALSE;
	}

	return TRUE;
}

/** 描画 */

void mCheckButtonDrawHandle(mWidget *wg,mPixbuf *pixbuf)
{
	mCheckButton *p = M_CHECKBUTTON(wg);
	mFont *font;
	mWidgetLabelTextRowInfo *buf;
	char *text;
	int disable,y;
	uint32_t flags,txtcol;

	font = mWidgetGetFont(wg);
	
	disable = !(wg->fState & MWIDGET_STATE_ENABLED);
	
	//背景
	
	mWidgetDrawBkgnd(wg, NULL);
	
	//フォーカス
	
	if(wg->fState & MWIDGET_STATE_FOCUSED)
		mPixbufBoxDash(pixbuf, 0, 0, wg->w, wg->h, MSYSCOL(TEXT));
	
	//チェックボックス
	
	flags = 0;
	if(p->ck.flags & MCHECKBTT_FLAGS_CHECKED) flags |= MPIXBUF_DRAWCKBOX_CHECKED;
	if(p->ck.style & MCHECKBUTTON_S_RADIO) flags |= MPIXBUF_DRAWCKBOX_RADIO;
	if(disable) flags |= MPIXBUF_DRAWCKBOX_DISABLE;

	y = (font->height - _CHECKBOXSIZE) >> 1;
	if(y < 0) y = 0;
	
	mPixbufDrawCheckBox(pixbuf, _PADDINGX, _PADDINGY + y, flags);
	
	//テキスト

	text = p->ck.text;
	buf = p->ck.rowbuf;

	txtcol = (disable)? MSYSCOL_RGB(TEXT_DISABLE): MSYSCOL_RGB(TEXT);

	if(!buf)
	{
		//---- 単一行

		if(text)
		{
			y = (_CHECKBOXSIZE - font->height) >> 1;
			if(y < 0) y = 0;
		
			mFontDrawText(font, pixbuf,
				_PADDINGX + _CHECKBOXSIZE + _TEXTSPACE, _PADDINGY + y,
				text, -1, txtcol);
		}
	}
	else
	{
		//---- 複数行

		for(y = _PADDINGY; buf->pos >= 0; buf++)
		{
			mFontDrawText(font, pixbuf,
				_PADDINGX + _CHECKBOXSIZE + _TEXTSPACE, y,
				text + buf->pos, buf->len, txtcol);

			y += font->lineheight;
		}
	}
}

/** @} */
