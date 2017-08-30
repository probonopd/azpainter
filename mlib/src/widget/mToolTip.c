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
 * mToolTip
 *****************************************/

#include "mDef.h"

#include "mWindowDef.h"
#include "mWindow.h"
#include "mWidget.h"
#include "mFont.h"
#include "mPixbuf.h"
#include "mUtilStr.h"
#include "mSysCol.h"


//----------------------

typedef struct _mToolTip
{
	mWidget wg;
	mContainerData ct;
	mWindowData win;

	char *text;
}mToolTip;

#define M_TOOLTIP(p)  ((mToolTip *)(p))

#define _PADDING_X  4
#define _PADDING_Y  3

//----------------------


//******************************


/** 解放 */

static void _handle_destroy(mWidget *wg)
{
	mFree(M_TOOLTIP(wg)->text);
}

/** 描画 */

static void _handle_draw(mWidget *wg,mPixbuf *pixbuf)
{
	mFont *font;
	char *pc,*pc2;
	int y;

	font = mWidgetGetFont(wg);

	//枠

	mPixbufBox(pixbuf, 0, 0, wg->w, wg->h, MSYSCOL(MENU_FRAME));

	//背景

	mPixbufFillBox(pixbuf, 1, 1, wg->w - 2, wg->h - 2, MSYSCOL(MENU_FACE));

	//テキスト

	y = _PADDING_Y;

	for(pc = M_TOOLTIP(wg)->text; *pc; )
	{
		pc2 = mStrchr_end(pc, '\n');

		if(pc != pc2)
		{
			mFontDrawText(font, pixbuf,
				_PADDING_X, y,
				pc, pc2 - pc, MSYSCOL_RGB(MENU_TEXT));
		}

		y += font->lineheight;

		pc = pc2;
		if(*pc) pc++;
	}
}

/** ツールチップウィンドウ作成 */

mToolTip *_create_tooltip()
{
	mToolTip *p;

	p = (mToolTip *)mWindowNew(sizeof(mToolTip), NULL, MWINDOW_S_POPUP | MWINDOW_S_NO_IM);
	if(!p) return NULL;

	p->wg.destroy = _handle_destroy;
	p->wg.draw = _handle_draw;

	return p;
}

/** ウィンドウサイズセット */

static void _set_window_size(mToolTip *p)
{
	mFont *font;
	int maxh,maxw,w;
	char *pc,*pc2;

	font = mWidgetGetFont(M_WIDGET(p));

	maxw = maxh = 0;

	for(pc = p->text; *pc; )
	{
		pc2 = mStrchr_end(pc, '\n');

		if(pc != pc2)
		{
			w = mFontGetTextWidth(font, pc, pc2 - pc);

			if(maxw < w) maxw = w;
		}

		maxh += (pc == p->text)? font->height: font->lineheight;

		pc = pc2;
		if(*pc) pc++;
	}

	mWidgetResize(M_WIDGET(p), maxw + _PADDING_X * 2, maxh + _PADDING_Y * 2);
}


//******************************


/********************//**

@defgroup tooltip mToolTip
@brief ツールチップ

mWidget \> mContainer \> mWindow
 
@ingroup group_window
@{

@file mToolTip.h

**************************/


/** ツールチップ表示
 *
 * テキストが空の場合、非表示になる。\n
 * ツールチップが必要なくなった場合、 mWidgetDestroy() で削除すること。
 *
 * @param p 同じツールチップを使ってテキストを変える場合は指定。NULL で新規作成される。
 * @return 新規作成された場合は、そのポインタが返る。 */

mToolTip *mToolTipShow(mToolTip *p,int rootx,int rooty,const char *text)
{
	//空テキスト

	if(!text || *text == 0)
	{
		if(p) mWidgetShow(M_WIDGET(p), 0);
		
		return p;
	}

	//未作成の場合は、作成
	
	if(!p)
	{
		p = _create_tooltip();
		if(!p) return NULL;
	}

	//テキストセット

	mFree(p->text);

	p->text = mStrdup(text);

	//表示

	_set_window_size(p);

	mWindowMoveAdjust(M_WINDOW(p), rootx, rooty, FALSE);

	mWidgetShow(M_WIDGET(p), 1);
	mWindowKeepAbove(M_WINDOW(p), 1);

	mWidgetUpdate(M_WIDGET(p));

	return p;
}

/** @} */
