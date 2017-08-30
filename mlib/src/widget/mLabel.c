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
 * mLabel [ラベル]
 *****************************************/

#include "mDef.h"

#include "mLabel.h"
#include "mWidget.h"
#include "mFont.h"
#include "mPixbuf.h"
#include "mSysCol.h"
#include "mUtilStr.h"

#include "mWidget_pv.h"


//--------------------

#define SPACE_X  3
#define SPACE_Y  2

//--------------------


/*****************//**

@defgroup label mLabel
@brief ラベル

'\\n' で改行して複数行可能。

@ingroup group_widget
 
@{

@file mLabel.h
@def M_LABEL(p)
@struct mLabelData
@struct mLabel
@enum MLABEL_STYLE

*********************/



/** 解放処理 */

void mLabelDestroyHandle(mWidget *wg)
{
	mLabel *p = M_LABEL(wg);

	M_FREE_NULL(p->lb.text);
	M_FREE_NULL(p->lb.rowbuf);
}

/** サイズ計算 */

void mLabelCalcHintHandle(mWidget *wg)
{
	mLabel *p = M_LABEL(wg);
	mSize size;

	__mWidgetGetLabelTextSize(wg, p->lb.text, p->lb.rowbuf, &size);

	p->lb.sztext = size;
		
	wg->hintW = size.w;
	wg->hintH = size.h;
	
	if(p->lb.style & MLABEL_S_BORDER)
	{
		wg->hintW += SPACE_X * 2;
		wg->hintH += SPACE_Y * 2;
	}
}


//==========================


/** ラベル作成 */

mLabel *mLabelCreate(mWidget *parent,uint32_t style,
	uint32_t fLayout,uint32_t marginB4,const char *text)
{
	mLabel *p;

	p = mLabelNew(0, parent, style);
	if(!p) return NULL;

	p->wg.fLayout = fLayout;

	mWidgetSetMargin_b4(M_WIDGET(p), marginB4);

	mLabelSetText(p, text);

	return p;
}


/** ラベル作成 */

mLabel *mLabelNew(int size,mWidget *parent,uint32_t style)
{
	mLabel *p;
	
	if(size < sizeof(mLabel)) size = sizeof(mLabel);
	
	p = (mLabel *)mWidgetNew(size, parent);
	if(!p) return NULL;
	
	p->wg.destroy = mLabelDestroyHandle;
	p->wg.calcHint = mLabelCalcHintHandle;
	p->wg.draw = mLabelDrawHandle;

	p->lb.style = style;
	
	return p;
}

/** テキストセット */

void mLabelSetText(mLabel *p,const char *text)
{
	mLabelDestroyHandle(M_WIDGET(p));

	if(text)
	{
		p->lb.text = mStrdup(text);
		p->lb.rowbuf = __mWidgetGetLabelTextRowInfo(text);
	}

	if(p->wg.fUI & MWIDGET_UI_LAYOUTED)
		mLabelCalcHintHandle(M_WIDGET(p));
	
	mWidgetUpdate(M_WIDGET(p));
}

/** int 値からテキストセット */

void mLabelSetText_int(mLabel *p,int val)
{
	char m[32];

	mIntToStr(m, val);
	mLabelSetText(p, m);
}

/** int 値から浮動小数点数テキストセット */

void mLabelSetText_floatint(mLabel *p,int val,int dig)
{
	char m[32];

	mFloatIntToStr(m, val, dig);
	mLabelSetText(p, m);
}


/** 描画処理 */

void mLabelDrawHandle(mWidget *wg,mPixbuf *pixbuf)
{
	mLabel *p = M_LABEL(wg);
	mFont *font;
	mWidgetLabelTextRowInfo *buf;
	int spx,spy,tx,ty,w,h;
	uint32_t txtcol;

	font = mWidgetGetFont(wg);

	w = wg->w;
	h = wg->h;

	//余白
	
	if(p->lb.style & MLABEL_S_BORDER)
		spx = SPACE_X, spy = SPACE_Y;
	else
		spx = spy = 0;
	
	//テキスト Y 位置
	
	if(p->lb.style & MLABEL_S_BOTTOM)
		ty = h - spy - p->lb.sztext.h;
	else if(p->lb.style & MLABEL_S_MIDDLE)
		ty = spy + (h - spy * 2 - p->lb.sztext.h) / 2;
	else
		ty = spy;
		
	//背景
	
	mWidgetDrawBkgnd(wg, NULL);
	
	//テキスト
	
	buf = p->lb.rowbuf;
	txtcol = MSYSCOL_RGB(TEXT);
	
	if(!buf)
	{
		//---- 単一行

		if(p->lb.text)
		{
			if(p->lb.style & MLABEL_S_RIGHT)
				tx = w - spx - p->lb.sztext.w;
			else if(p->lb.style & MLABEL_S_CENTER)
				tx = spx + (w - spx * 2 - p->lb.sztext.w) / 2;
			else
				tx = spx;

			mFontDrawText(font, pixbuf, tx, ty, p->lb.text, -1, txtcol);
		}
	}
	else
	{
		//---- 複数行
		
		for(; buf->pos >= 0; buf++)
		{
			if(p->lb.style & MLABEL_S_RIGHT)
				tx = w - spx - buf->width;
			else if(p->lb.style & MLABEL_S_CENTER)
				tx = spx + (w - spx * 2 - buf->width) / 2;
			else
				tx = spx;
			
			mFontDrawText(font, pixbuf, tx, ty,
				p->lb.text + buf->pos, buf->len, txtcol);
			
			ty += font->lineheight;
		}
	}
	
	//枠
	
	if(p->lb.style & MLABEL_S_BORDER)
		mPixbufBox(pixbuf, 0, 0, w, h, MSYSCOL(FRAME));
}


/** @} */
