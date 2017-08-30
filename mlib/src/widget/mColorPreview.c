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
 * mColorPreview
 *****************************************/

#include "mDef.h"

#include "mColorPreview.h"

#include "mWidget.h"
#include "mPixbuf.h"
#include "mSysCol.h"


/**
@defgroup colorpreview mColorPreview
@brief カラープレビュー
@ingroup group_widget

<h3>継承</h3>
mWidget \> mColorPreview

@{

@file mColorPreview.h
@def M_COLORPREVIEW(p)
@struct mColorPreviewData
@struct mColorPreview
@enum MCOLORPREVIEW_STYLE

@var MCOLORPREVIEW_STYLE::MCOLORPREVIEW_S_FRAME
枠を付ける
*/


/** 作成 */

mColorPreview *mColorPreviewCreate(mWidget *parent,uint32_t style,int w,int h,uint32_t marginB4)
{
	mColorPreview *p;

	p = mColorPreviewNew(0, parent, style);
	if(!p) return NULL;

	p->wg.hintW = w;
	p->wg.hintH = h;

	mWidgetSetMargin_b4(M_WIDGET(p), marginB4);

	return p;
}

/** 作成 */

mColorPreview *mColorPreviewNew(int size,mWidget *parent,uint32_t style)
{
	mColorPreview *p;
	
	if(size < sizeof(mColorPreview)) size = sizeof(mColorPreview);
	
	p = (mColorPreview *)mWidgetNew(size, parent);
	if(!p) return NULL;
	
	p->wg.draw = mColorPreviewDrawHandle;
	
	p->cp.style = style;
	p->cp.colrgb = 0xffffff;
	
	return p;
}

/** 色セット */

void mColorPreviewSetColor(mColorPreview *p,mRgbCol col)
{
	col &= 0xffffff;

	if(col != p->cp.colrgb)
	{
		p->cp.colrgb = col;

		mWidgetUpdate(M_WIDGET(p));
	}
}

/** 描画 */

void mColorPreviewDrawHandle(mWidget *wg,mPixbuf *pixbuf)
{
	mColorPreview *p = M_COLORPREVIEW(wg);
	uint32_t col;

	col = mRGBtoPix(p->cp.colrgb);

	if(p->cp.style & MCOLORPREVIEW_S_FRAME)
	{
		//枠つき
		
		mPixbufBox(pixbuf, 0, 0, wg->w, wg->h, 0);
		mPixbufFillBox(pixbuf, 1, 1, wg->w - 2, wg->h - 2, col);
	}
	else
		mPixbufFillBox(pixbuf, 0, 0, wg->w, wg->h, col);
}

/** @} */
