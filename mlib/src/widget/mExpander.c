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
 * mExpander
 *****************************************/

#include "mDef.h"

#include "mExpander.h"

#include "mWidget.h"
#include "mContainer.h"
#include "mPixbuf.h"
#include "mFont.h"
#include "mEvent.h"
#include "mSysCol.h"
#include "mGui.h"
#include "mRectBox.h"
#include "mUtil.h"


/***************//**

@defgroup expander mExpander
@brief 閉じることができる、ヘッダ付きのコンテナ

- 閉じるときは、このウィジェットのサイズ (高さ) を変更して子ウィジェットを隠す。
- 高さは推奨サイズを使うようにすること (MLF_EXPAND_H にしない)。
- MEXPANDER_N_TOGGLE 通知が来た時、影響のある最上位ウィジェットを再レイアウト (mWidgetReLayout) すること。

<h2>継承</h2>
mWidget \> mContainer \> mExpander

@ingroup group_widget
@{

@file mExpander.h
@def M_EXPANDER(p)
@struct mExpanderData
@struct mExpander
@enum MEXPANDER_STYLE
@enum MEXPANDER_NOTIFY

@var MEXPANDER_STYLE::MEXPANDER_S_BORDER_TOP
ヘッダの上に境界線を描画
@var MEXPANDER_STYLE::MEXPANDER_S_HEADER_DARK
ヘッダの背景を暗い色にする

@var MEXPANDER_NOTIFY::MEXPANDER_N_TOGGLE
展開状態が変化した時 (手動変化時も含む)。\n
param1 : 展開されているか

********************/


//=============================
// サブ
//=============================


/** 展開状態の変更時 */

static void _change_expand(mExpander *p)
{
	if(p->wg.fUI & MWIDGET_UI_LAYOUTED)
	{
		//自身の推奨サイズを再計算 (フラグ)

		mWidgetCalcHintSize(M_WIDGET(p)); 

		//通知

		mWidgetAppendEvent_notify(NULL, M_WIDGET(p), MEXPANDER_N_TOGGLE, p->exp.expand, 0);
	}
}


//=============================


/** 破棄 */

void mExpanderDestroyHandle(mWidget *p)
{
	mFree(M_EXPANDER(p)->exp.text);
}

/** 作成 */

mExpander *mExpanderNew(int size,mWidget *parent,uint32_t style)
{
	mExpander *p;
	
	if(size < sizeof(mExpander)) size = sizeof(mExpander);
	
	p = (mExpander *)mContainerNew(size, parent);
	if(!p) return NULL;

	p->wg.destroy = mExpanderDestroyHandle;
	p->wg.calcHint = mExpanderCalcHintHandle;
	p->wg.draw = mExpanderDrawHandle;
	p->wg.event = mExpanderEventHandle;
	
	p->wg.fEventFilter |= MWIDGET_EVENTFILTER_POINTER;

	p->exp.style = style;
	
	return p;
}

/** ボタン作成 */

mExpander *mExpanderCreate(mWidget *parent,int id,uint32_t style,
	uint32_t fLayout,uint32_t marginB4,const char *text)
{
	mExpander *p;

	p = mExpanderNew(0, parent, style);
	if(p)
	{
		p->wg.id = id;
		p->wg.fLayout = fLayout;

		mWidgetSetMargin_b4(M_WIDGET(p), marginB4);

		mExpanderSetText(p, text);
	}

	return p;
}

/** ヘッダテキストセット */

void mExpanderSetText(mExpander *p,const char *text)
{
	mStrdup_ptr(&p->exp.text, text);
}

/** 内部余白を byte x 4 でセット */

void mExpanderSetPadding_b4(mExpander *p,uint32_t val)
{
	mRectSetByPack(&p->exp.padding, val);
}

/** 展開状態をセット */

void mExpanderSetExpand(mExpander *p,int type)
{
	if(mIsChangeState(type, p->exp.expand))
	{
		p->exp.expand ^= 1;

		_change_expand(p);
	}
}


//========================
// ハンドラ
//========================


/** サイズ計算 */

void mExpanderCalcHintHandle(mWidget *wg)
{
	mExpander *p = M_EXPANDER(wg);
	mFont *font = mWidgetGetFont(wg);
	mWidgetSpacing *pad;
	int w;

	pad = &p->ct.padding;

	//ヘッダ高さ

	p->exp.headerH = (font->height < 7)? 7: font->height;

	//コンテナ余白 (共通)

	mMemzero(pad, sizeof(mWidgetSpacing));

	pad->top = p->exp.headerH;

	if(p->exp.style & MEXPANDER_S_BORDER_TOP)
		pad->top += 2;

	//内容

	if(p->exp.expand)
	{
		//展開時
		/* 計算時、コンテナの余白はサイズに含まれるので注意 */
		
		pad->left   = p->exp.padding.x1;
		pad->top    += p->exp.padding.y1;
		pad->right  = p->exp.padding.x2;
		pad->bottom = p->exp.padding.y2;

		(p->ct.calcHint)(wg);
	}
	else
	{
		//閉じている時

		wg->hintW = 1;
		wg->hintH = pad->top;
	}

	//ヘッダテキスト幅

	w = mFontGetTextWidth(font, p->exp.text, -1) + 14;

	if(w > wg->hintW) wg->hintW = w;
}

/** イベント */

int mExpanderEventHandle(mWidget *wg,mEvent *ev)
{
	mExpander *p = M_EXPANDER(wg);

	switch(ev->type)
	{
		//ヘッダ上をボタン押し時
		case MEVENT_POINTER:
			if((ev->pt.type == MEVENT_POINTER_TYPE_PRESS || ev->pt.type == MEVENT_POINTER_TYPE_DBLCLK)
				&& ev->pt.btt == M_BTT_LEFT && ev->pt.y < p->exp.headerH)
			{
				p->exp.expand ^= 1;

				_change_expand(p);
			}
			break;
		
		default:
			return FALSE;
	}

	return TRUE;
}

/** 描画 */

void mExpanderDrawHandle(mWidget *wg,mPixbuf *pixbuf)
{
	mExpander *p = M_EXPANDER(wg);
	mFont *font = mWidgetGetFont(wg);
	int y = 0;
	mRgbCol coltext;

	//背景

	mWidgetDrawBkgnd(wg, NULL);

	//上の境界線

	if(p->exp.style & MEXPANDER_S_BORDER_TOP)
	{
		mPixbufLineH(pixbuf, 0, 0, wg->w, MSYSCOL(FRAME));
		y = 2;
	}

	//ヘッダ背景

	if(p->exp.style & MEXPANDER_S_HEADER_DARK)
	{
		mPixbufFillBox(pixbuf, 0, y, wg->w, p->exp.headerH, MSYSCOL(FACE_DARKER));
		coltext = MSYSCOL_RGB(TEXT_REVERSE);
	}
	else
		coltext = MSYSCOL_RGB(TEXT);

	//ヘッダテキスト

	mFontDrawText(font, pixbuf, 14, y, p->exp.text, -1, coltext);

	//矢印

	y += (p->exp.headerH - 7) >> 1;

	if(p->exp.expand)
		mPixbufDrawArrowDown_7x4(pixbuf, 3, y + 2, coltext);
	else
		mPixbufDrawArrowRight_4x7(pixbuf, 5, y, coltext);
}

/* @} */
