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
 * mBitImgButton
 *****************************************/

#include "mDef.h"

#include "mBitImgButton.h"

#include "mWidget.h"
#include "mPixbuf.h"
#include "mSysCol.h"


//-------------

#define _FRAMEW  3

//-------------


/********************//**

@defgroup bitimgbutton mBitImgButton
@brief ビットイメージのボタン

<h2>継承</h2>
mWidget \> mButton \> mBitImgButton

@ingroup group_widget
@{

@file mBitImgButton.h
@def M_BITIMGBUTTON(p)
@struct mBitImgButtonData
@struct mBitImgButton
@enum MBITIMGBUTTON_TYPE

***********************/


/** 作成 */

mBitImgButton *mBitImgButtonNew(int size,mWidget *parent,uint32_t style)
{
	mBitImgButton *p;
	
	if(size < sizeof(mBitImgButton)) size = sizeof(mBitImgButton);
	
	p = (mBitImgButton *)mButtonNew(size, parent, 0);
	if(!p) return NULL;
	
	p->wg.calcHint = mBitImgButtonCalcHintHandle;
	p->wg.draw = mBitImgButtonDrawHandle;

	return p;
}

/** 作成 */

mBitImgButton *mBitImgButtonCreate(mWidget *parent,int id,uint32_t style,uint32_t fLayout,uint32_t marginB4)
{
	mBitImgButton *p;

	p = mBitImgButtonNew(0, parent, style);
	if(!p) return NULL;

	p->wg.id = id;
	p->wg.fLayout = fLayout;

	mWidgetSetMargin_b4(M_WIDGET(p), marginB4);

	return p;
}

/** イメージセット */

void mBitImgButtonSetImage(mBitImgButton *p,int type,const uint8_t *imgpat,int width,int height)
{
	p->bib.type = type;
	p->bib.imgpat = imgpat;
	p->bib.width = width;
	p->bib.height = height;
}


//========================
// ハンドラ
//========================


/** サイズ計算 */

void mBitImgButtonCalcHintHandle(mWidget *wg)
{
	mBitImgButton *p = M_BITIMGBUTTON(wg);

	p->wg.hintW = p->bib.width + _FRAMEW * 2;
	p->wg.hintH = p->bib.height + _FRAMEW * 2;
}

/** 描画 */

void mBitImgButtonDrawHandle(mWidget *wg,mPixbuf *pixbuf)
{
	mBitImgButton *p = M_BITIMGBUTTON(wg);
	mBool press;
	int x,y;
	mRgbCol colf;
	mPixCol col[4];

	press = mButtonIsPressed(M_BUTTON(wg));

	//ボタンのベース描画

	mButtonDrawBase(M_BUTTON(wg), pixbuf, press);

	//イメージ描画

	if(p->bib.imgpat)
	{
		x = _FRAMEW + press;
		y = _FRAMEW + press;

		if(wg->fState & MWIDGET_STATE_ENABLED)
		{
			col[1] = 0;
			col[2] = MSYSCOL(WHITE);
		}
		else
		{
			//無効時は FACE 色と合成

			colf = (wg->fState & MWIDGET_STATE_FOCUSED)? MSYSCOL_RGB(FACE_FOCUS): MSYSCOL_RGB(FACE_DARK);

			col[1] = mGraytoPix(mBlendRGB_256(0, colf, 64));
			col[2] = mGraytoPix(mBlendRGB_256(0xffffff, colf, 64));
		}
	
		switch(p->bib.type)
		{
			//1bit (白黒)
			case MBITIMGBUTTON_TYPE_1BIT_BLACK_WHITE:
				mPixbufBltFrom1bit(pixbuf, x, y,
					p->bib.imgpat, 0, 0, p->bib.width, p->bib.height, p->bib.width,
					col[1], col[2]);
				break;
			//1bit (透過 + 黒)
			case MBITIMGBUTTON_TYPE_1BIT_TP_BLACK:
				mPixbufDrawBitPattern(pixbuf, x, y,
					p->bib.imgpat, p->bib.width, p->bib.height, col[1]);
				break;
			//2bit (透過 + 黒 + 白)
			case MBITIMGBUTTON_TYPE_2BIT_TP_BLACK_WHITE:
				col[0] = (mPixCol)-1;

				mPixbufDraw2bitPattern(pixbuf, x, y,
					p->bib.imgpat, p->bib.width, p->bib.height, col);
				break;
		}
	}
}

/* @} */
