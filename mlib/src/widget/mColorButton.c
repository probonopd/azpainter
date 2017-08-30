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
 * mColorButton
 *****************************************/

#include "mDef.h"

#include "mColorButton.h"

#include "mWidget.h"
#include "mPixbuf.h"
#include "mSysCol.h"
#include "mSysDialog.h"


/********************//**

@defgroup colorbutton mColorButton
@brief 色指定ボタン
@ingroup group_widget

<h3>継承</h3>
mWidget @> mButton @> mColorButton

@{

@file mColorButton.h
@def M_COLORBUTTON(p)
@struct mColorButtonData
@struct mColorButton
@enum MCOLORBUTTON_STYLE
@enum MCOLORBUTTON_NOTIFY

@var MCOLORBUTTON_STYLE::MCOLORBUTTON_S_DIALOG
ボタンが押された時、色選択ダイアログを実行する

@var MCOLORBUTTON_NOTIFY::MCOLORBUTTON_N_PRESS
ボタンが押された。 \n
\b MCOLORBUTTON_S_DIALOG がある場合は、色が変更された後に来る。 \n
param1 : 現在色

************************/



/** ボタン押し時 */

static void _onpressed(mButton *btt)
{
	mColorButton *p = M_COLORBUTTON(btt);

	//色選択ダイアログ

	if(p->cb.style & MCOLORBUTTON_S_DIALOG)
	{
		if(!mSysDlgSelectColor(p->wg.toplevel, &p->cb.col))
			return;
	}

	//通知
	
	mWidgetAppendEvent_notify(NULL, M_WIDGET(p),
		MCOLORBUTTON_N_PRESS, p->cb.col, 0);
}

/** 作成 */

mColorButton *mColorButtonNew(int size,mWidget *parent,uint32_t style)
{
	mColorButton *p;
	
	if(size < sizeof(mColorButton)) size = sizeof(mColorButton);
	
	p = (mColorButton *)mButtonNew(size, parent, 0);
	if(!p) return NULL;
	
	p->wg.draw = mColorButtonDrawHandle;
	p->wg.calcHint = NULL;
	p->wg.hintW = 60;
	p->wg.hintH = 23;

	p->btt.onPressed = _onpressed;

	p->cb.style = style;
	
	return p;
}

/** 作成 */

mColorButton *mColorButtonCreate(mWidget *parent,int id,uint32_t style,
	uint32_t fLayout,uint32_t marginB4,mRgbCol col)
{
	mColorButton *p;

	p = mColorButtonNew(0, parent, style);
	if(!p) return NULL;

	p->wg.id = id;
	p->wg.fLayout = fLayout;
	p->cb.col = col & 0xffffff;

	mWidgetSetMargin_b4(M_WIDGET(p), marginB4);

	return p;
}

/** 色を取得 */

mRgbCol mColorButtonGetColor(mColorButton *p)
{
	return p->cb.col;
}

/** 色をセット */

void mColorButtonSetColor(mColorButton *p,mRgbCol col)
{
	col &= 0xffffff;

	if(p->cb.col != col)
	{
		p->cb.col = col;

		mWidgetUpdate(M_WIDGET(p));
	}
}


/** 描画 */

void mColorButtonDrawHandle(mWidget *wg,mPixbuf *pixbuf)
{
	mColorButton *p = M_COLORBUTTON(wg);
	int down;

	down = mButtonIsPressed(M_BUTTON(p));

	//ボタン

	mButtonDrawBase(M_BUTTON(p), pixbuf, down);

	//色

	if(wg->fState & MWIDGET_STATE_ENABLED)
	{
		mPixbufBox(pixbuf, 4 + down, 4 + down, wg->w - 8, wg->h - 8, 0);

		mPixbufFillBox(pixbuf, 5 + down, 5 + down, wg->w - 10, wg->h - 10,
			mRGBtoPix(p->cb.col));
	}
	else
		mPixbufBox(pixbuf, 4 + down, 4 + down, wg->w - 8, wg->h - 8, MSYSCOL(FRAME_LIGHT));
}

/** @} */
