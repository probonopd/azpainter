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
 * mGroupBox [グループボックス]
 *****************************************/

#include "mDef.h"

#include "mGroupBox.h"

#include "mWidget.h"
#include "mContainer.h"
#include "mFont.h"
#include "mPixbuf.h"
#include "mSysCol.h"


//-----------------------------

#define _PADDING_W  6
#define _LABEL_X    6
#define _LABEL_PADDING_W 2

static void _draw_handle(mWidget *wg,mPixbuf *pixbuf);

//-----------------------------


/*****************//**

@defgroup groupbox mGroupBox
@brief グループボックス
@ingroup group_widget

<h3>継承</h3>
mWidget \> mContainer \> mGroupBox

@{

@file mGroupBox.h
@def M_GROUPBOX(p)
@struct mGroupBoxData
@struct mGroupBox

************************/


/** 解放処理 */

static void _destroy_handle(mWidget *wg)
{
	mGroupBox *p = M_GROUPBOX(wg);

	M_FREE_NULL(p->gb.label);
}

/** サイズ計算 */

static void _calchint_handle(mWidget *wg)
{
	mGroupBox *p = M_GROUPBOX(wg);
	mFont *font = mWidgetGetFont(wg);
	int w,h;
	
	w = mFontGetTextWidth(font, p->gb.label, -1);
	h = (w)? font->height: 0;

	p->gb.labelW = w;
	p->gb.labelH = h;

	//余白
	
	if(w) w += 6 + _LABEL_PADDING_W * 2;

	//padding
	
	p->ct.padding.left = _PADDING_W;
	p->ct.padding.top = h + _PADDING_W;
	p->ct.padding.right = _PADDING_W;
	p->ct.padding.bottom = _PADDING_W;

	//レイアウトサイズ
	
	(p->ct.calcHint)(wg);

	if(wg->hintW < w) wg->hintW = w;
	if(wg->initW < w) wg->initW = w;
}


//=======================


/** グループボックス作成 */

mGroupBox *mGroupBoxNew(int size,mWidget *parent,uint32_t style)
{
	mGroupBox *p;
	
	if(size < sizeof(mGroupBox)) size = sizeof(mGroupBox);
	
	p = (mGroupBox *)mContainerNew(size, parent);
	if(!p) return NULL;
	
	p->wg.destroy = _destroy_handle;
	p->wg.calcHint = _calchint_handle;
	p->wg.draw = _draw_handle;
	
	return p;
}

/** グループボックス作成 */

mGroupBox *mGroupBoxCreate(mWidget *parent,uint32_t style,
	uint32_t fLayout,uint32_t marginB4,const char *label)
{
	mGroupBox *p;

	p = mGroupBoxNew(0, parent, style);
	if(p)
	{
		p->wg.fLayout = fLayout;
		mWidgetSetMargin_b4(M_WIDGET(p), marginB4);

		mGroupBoxSetLabel(p, label);
	}

	return p;
}

/** ラベルセット */

void mGroupBoxSetLabel(mGroupBox *p,const char *text)
{
	mFree(p->gb.label);

	p->gb.label = mStrdup(text);
	
	mWidgetCalcHintSize(M_WIDGET(p));
	mWidgetUpdate(M_WIDGET(p));
}

/* @} */


/** 描画 */

void _draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	mGroupBox *p = M_GROUPBOX(wg);
	mFont *font;
	mBox box;
	int tx,fy;

	font = mWidgetGetFont(wg);

	//背景
	
	mWidgetDrawBkgnd(wg, NULL);
	
	//枠
	
	fy = p->gb.labelH >> 1;
	
	mPixbufBox(pixbuf, 0, fy, wg->w, wg->h - fy, MSYSCOL(FRAME));
	
	//ラベル
	
	if(p->gb.label)
	{
		tx = (_LABEL_X + p->gb.labelW + _LABEL_PADDING_W * 2 >= wg->w)?
				(wg->w - p->gb.labelW - _LABEL_PADDING_W * 2) >> 1: _LABEL_X;
	
		//背景 (枠を消す)
		
		box.x = tx;
		box.y = fy;
		box.w = p->gb.labelW + _LABEL_PADDING_W * 2;
		box.h = 1;
		
		mWidgetDrawBkgnd_force(wg, &box);
		
		//テキスト
		
		mFontDrawText(font, pixbuf,
			tx + _LABEL_PADDING_W, 0,
			p->gb.label, -1, MSYSCOL_RGB(TEXT));
	}
}

