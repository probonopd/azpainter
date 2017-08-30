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
 * mProgressBar [プログレスバー]
 *****************************************/

#include <stdio.h>

#include "mDef.h"

#include "mProgressBar.h"

#include "mWidget.h"
#include "mPixbuf.h"
#include "mFont.h"
#include "mSysCol.h"



/**
@defgroup progressbar mProgressBar
@brief プログレスバー

<h3>継承</h3>
mWidget \> mProgressBar

@ingroup group_widget
@{

@file mProgressBar.h
@def M_PROGRESSBAR(p)
@struct mProgressBar
@struct mProgressBarData
@enum MPROGRESSBAR_STYLE

@var MPROGRESSBAR_STYLE::MPROGRESSBAR_S_FRAME
枠を付ける

@var MPROGRESSBAR_STYLE::MPROGRESSBAR_S_TEXT
任意のテキスト付き

@var MPROGRESSBAR_STYLE::MPROGRESSBAR_S_TEXT_PERS
"...％" のテキスト付き
*/


/** 解放処理 */

void mProgressBarDestroyHandle(mWidget *p)
{
	M_FREE_NULL(M_PROGRESSBAR(p)->pb.text);
}

/** 作成 */

mProgressBar *mProgressBarNew(int size,mWidget *parent,uint32_t style)
{
	mProgressBar *p;
	
	if(size < sizeof(mProgressBar)) size = sizeof(mProgressBar);
	
	p = (mProgressBar *)mWidgetNew(size, parent);
	if(!p) return NULL;
	
	p->wg.destroy = mProgressBarDestroyHandle;
	p->wg.calcHint = mProgressBarCalcHintHandle;
	p->wg.draw = mProgressBarDrawHandle;
	
	p->pb.style = style;
	p->pb.max = 100;
	
	return p;
}

/** ステータスセット */

void mProgressBarSetStatus(mProgressBar *p,int min,int max,int pos)
{
	if(min >= max) max = min + 1;
	
	if(pos < min) pos = min;
	else if(pos > max) pos = max;
	
	p->pb.min = min;
	p->pb.max = max;
	p->pb.pos = pos;
	
	mWidgetUpdate(M_WIDGET(p));
}

/** テキストセット */

void mProgressBarSetText(mProgressBar *p,const char *text)
{
	mProgressBarDestroyHandle(M_WIDGET(p));
	
	if(text)
		p->pb.textlen = mStrdup2(text, &p->pb.text);

	mWidgetUpdate(M_WIDGET(p));
}

/** 位置セット */

mBool mProgressBarSetPos(mProgressBar *p,int pos)
{
	if(pos < p->pb.min) pos = p->pb.min;
	else if(pos > p->pb.max) pos = p->pb.max;
	
	if(pos == p->pb.pos)
		return FALSE;
	else
	{
		p->pb.pos = pos;
		mWidgetUpdate(M_WIDGET(p));
		return TRUE;
	}
}

/** 位置を+1 */

void mProgressBarIncPos(mProgressBar *p)
{
	if(p->pb.pos < p->pb.max)
	{
		p->pb.pos++;
		mWidgetUpdate(M_WIDGET(p));
	}
}

/** 現在位置に加算 */

mBool mProgressBarAddPos(mProgressBar *p,int add)
{
	return mProgressBarSetPos(p, p->pb.pos + add);
}

/** サブステップ開始 */

void mProgressBarBeginSubStep(mProgressBar *p,int stepnum,int max)
{
	p->pb.sub_step = stepnum;
	p->pb.sub_max  = max;
	p->pb.sub_toppos = p->pb.pos;
	p->pb.sub_curcnt = 0;
	p->pb.sub_curstep = 0;
	p->pb.sub_nextcnt = (max < stepnum * 2)? -1: max / stepnum;
}

/** サブステップ開始 (１ステップのみ)
 *
 * 最大値を stepnum にセット。 */

void mProgressBarBeginSubStep_onestep(mProgressBar *p,int stepnum,int max)
{
	p->pb.min = p->pb.pos = 0;
	p->pb.max = stepnum;
	
	mProgressBarBeginSubStep(p, stepnum, max);
}

/** サブステップのカウントを +1
 *
 * @return 位置が変化したか */

mBool mProgressBarIncSubStep(mProgressBar *p)
{
	if(p->pb.sub_curcnt > p->pb.sub_max) return FALSE;

	p->pb.sub_curcnt++;

	if(p->pb.sub_curcnt == p->pb.sub_max)
		//最後
		return mProgressBarSetPos(p, p->pb.sub_toppos + p->pb.sub_step);
	else if(p->pb.sub_nextcnt < 0)
	{
		//カウント最大値がステップ数と同じくらいの場合、常に位置を計算
		
		return mProgressBarSetPos(p,
			p->pb.sub_toppos + p->pb.sub_curcnt * p->pb.sub_step / p->pb.sub_max);
	}
	else
	{
		//最大値が大きい値の場合、一つのステップごとに位置をセット

		if(p->pb.sub_curcnt >= p->pb.sub_nextcnt)
		{
			p->pb.sub_curstep++;
			p->pb.sub_nextcnt = (p->pb.sub_curstep + 1) * p->pb.sub_max / p->pb.sub_step;

			return mProgressBarSetPos(p, p->pb.sub_toppos + p->pb.sub_curstep);
		}
		else
			return FALSE;
	}
}


//========================
// ハンドラ
//========================


/** サイズ計算 */

void mProgressBarCalcHintHandle(mWidget *wg)
{
	uint32_t style = M_PROGRESSBAR(wg)->pb.style;

	wg->hintW = 6;

	if(style & (MPROGRESSBAR_S_TEXT | MPROGRESSBAR_S_TEXT_PERS))
		wg->hintH = mWidgetGetFontHeight(wg);
	else
		wg->hintH = 12;

	if(style & MPROGRESSBAR_S_FRAME)
		wg->hintH += 4;
}

/** 描画 */

void mProgressBarDrawHandle(mWidget *wg,mPixbuf *pixbuf)
{
	mProgressBar *p = M_PROGRESSBAR(wg);
	mFont *font;
	int n,range,frame;
	char m[32],*ptext;
	
	range = p->pb.max - p->pb.min;
	frame = (p->pb.style & MPROGRESSBAR_S_FRAME)? 2: 0;
		
	//背景

	mWidgetDrawBkgnd(wg, NULL);
	
	//枠

	if(frame)
		mPixbufBox(pixbuf, 0, 0, wg->w, wg->h, MSYSCOL(FRAME));

	//バー
	
	n = (int)((double)(wg->w - frame * 2) / range * (p->pb.pos - p->pb.min) + 0.5);
	
	if(n)
		mPixbufFillBox(pixbuf, frame, frame, n, wg->h - frame * 2, MSYSCOL(FACE_SELECT_LIGHT));
	
	//テキスト
	
	if(p->pb.style & (MPROGRESSBAR_S_TEXT | MPROGRESSBAR_S_TEXT_PERS))
	{
		if(p->pb.style & MPROGRESSBAR_S_TEXT)
		{
			ptext = p->pb.text;
			n = p->pb.textlen;
		}
		else
		{
			n = (int)((double)(p->pb.pos - p->pb.min) / range * 100.0 + 0.5);
			n = snprintf(m, 32, "%d%%", n);
			
			ptext = m;
		}
		
		//中央寄せ
		
		font = mWidgetGetFont(wg);

		mPixbufSetClipBox_d(pixbuf,
			frame, frame, wg->w - frame * 2, wg->h - frame * 2);
		
		mFontDrawText(font, pixbuf,
			frame + (wg->w - frame * 2 - mFontGetTextWidth(font, ptext, n)) / 2,
			(wg->h - font->height) / 2,
			ptext, -1, MSYSCOL_RGB(TEXT));
	}
}

/** @} */
