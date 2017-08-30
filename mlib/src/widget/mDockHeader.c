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
 * mDockHeader
 *****************************************/

#include "mDef.h"

#include "mDockHeader.h"

#include "mWidget.h"
#include "mPixbuf.h"
#include "mFont.h"
#include "mEvent.h"
#include "mSysCol.h"
#include "mUtil.h"


//-------------

#define _MIN_TEXT_HEIGHT  11
#define _PADDING_VERT  1

#define _PRESSF_CLOSE  1
#define _PRESSF_SWITCH 2
#define _PRESSF_FOLD   3

//-------------

#define _TITLE_CH_W  6
#define _TITLE_CH_H  8

//-------------


/** 作成 */

mDockHeader *mDockHeaderNew(int size,mWidget *parent,uint32_t style)
{
	mDockHeader *p;
	
	if(size < sizeof(mDockHeader)) size = sizeof(mDockHeader);
	
	p = (mDockHeader *)mWidgetNew(size, parent);
	if(!p) return NULL;
	
	p->wg.draw = mDockHeaderDrawHandle;
	p->wg.event = mDockHeaderEventHandle;
	p->wg.calcHint = mDockHeaderCalcHintHandle;
	
	p->wg.fEventFilter |= MWIDGET_EVENTFILTER_POINTER;
	p->wg.fLayout |= MLF_EXPAND_W;

	p->dh.style = style;
	
	return p;
}

/** 推奨サイズセットハンドラ */

void mDockHeaderCalcHintHandle(mWidget *wg)
{
	int h;

	h = mWidgetGetFontHeight(wg);
	if(h < _MIN_TEXT_HEIGHT) h = _MIN_TEXT_HEIGHT;

	wg->hintH = h + _PADDING_VERT * 2;
}

/** タイトルのセット
 *
 * @param title 静的文字列 */

void mDockHeaderSetTitle(mDockHeader *p,const char *title)
{
	p->dh.title = title;
}

/** 折りたたみ状態の変更 */

void mDockHeaderSetFolded(mDockHeader *p,int type)
{
	if((p->dh.style & MDOCKHEADER_S_HAVE_FOLD)
		&& mIsChangeState(type, (p->dh.style & MDOCKHEADER_S_FOLDED)))
	{
		p->dh.style ^= MDOCKHEADER_S_FOLDED;

		mWidgetUpdate(M_WIDGET(p));
	}
}


//========================
// ハンドラ
//========================


/** ボタンの範囲にあるか */

static mBool _isInButtonArea(mDockHeader *p,int x,int bx)
{
	return (bx <= x && x < bx + p->wg.h - _PADDING_VERT * 2);
}

/** 位置からボタン取得 */

static int _getButtonByPos(mDockHeader *p,int x,int y)
{
	int bx,bttsize;

	if(y >= _PADDING_VERT && y < p->wg.h - _PADDING_VERT)
	{
		//折りたたみ
		
		if((p->dh.style & MDOCKHEADER_S_HAVE_FOLD)
			&& _isInButtonArea(p, x, 3))
			return _PRESSF_FOLD;
		
		//閉じる

		bttsize = p->wg.h - _PADDING_VERT * 2;

		bx = p->wg.w - 3 - bttsize;

		if(p->dh.style & MDOCKHEADER_S_HAVE_CLOSE)
		{
			if(_isInButtonArea(p, x, bx)) return _PRESSF_CLOSE;

			bx -= bttsize + 3;
		}

		//切り替え

		if((p->dh.style & MDOCKHEADER_S_HAVE_SWITCH) && _isInButtonArea(p, x, bx))
			return _PRESSF_SWITCH;
	}

	return 0;
}

/** グラブ解除 */

static void _release_grab(mDockHeader *p,mBool send)
{
	if(p->dh.fpress)
	{
		if(send)
		{
			mWidgetAppendEvent_notify(NULL, M_WIDGET(p),
				MDOCKHEADER_N_BUTTON, p->dh.fpress, 0);
		}
	
		mWidgetUngrabPointer(M_WIDGET(p));
		mWidgetUpdate(M_WIDGET(p));

		p->dh.fpress = 0;
	}
}

/** イベント */

int mDockHeaderEventHandle(mWidget *wg,mEvent *ev)
{
	mDockHeader *p = M_DOCKHEADER(wg);

	switch(ev->type)
	{
		case MEVENT_POINTER:
			if(ev->pt.type == MEVENT_POINTER_TYPE_PRESS
				|| ev->pt.type == MEVENT_POINTER_TYPE_DBLCLK)
			{
				//押し
			
				if(ev->pt.btt == M_BTT_LEFT && p->dh.fpress == 0)
				{
					p->dh.fpress = _getButtonByPos(p, ev->pt.x, ev->pt.y);

					if(p->dh.fpress)
					{
						mWidgetGrabPointer(wg);
						mWidgetUpdate(wg);
					}
				}
			}
			else if(ev->pt.type == MEVENT_POINTER_TYPE_RELEASE)
			{
				//離し
			
				if(ev->pt.btt == M_BTT_LEFT)
					_release_grab(p, TRUE);
			}
			break;

		case MEVENT_FOCUS:
			if(ev->focus.bOut)
				_release_grab(p, FALSE);
			break;

		default:
			return FALSE;
	}

	return TRUE;
}


//========================
// 描画
//========================


/** ボタン描画 */

static void _draw_button(mPixbuf *pixbuf,
	int x,int y,uint8_t *pat,int bttsize,mBool press)
{
	mPixbufBox(pixbuf, x, y, bttsize, bttsize, MSYSCOL(FRAME_DARK));

	mPixbufFillBox(pixbuf, x + 1, y + 1, bttsize - 2, bttsize - 2,
		(press)? MSYSCOL(FACE_DARK): MSYSCOL(FACE_LIGHTEST));

	mPixbufDrawBitPattern(pixbuf,
		x + (bttsize - 7) / 2 + press, y + (bttsize - 7) / 2 + press,
		pat, 7, 7, MSYSCOL(TEXT));
}

/** 描画 */

void mDockHeaderDrawHandle(mWidget *wg,mPixbuf *pixbuf)
{
	mDockHeader *p = M_DOCKHEADER(wg);
	int x,x2,bttsize;
	uint8_t pat_fold_on[7] = {0x00,0x04,0x0c,0x1c,0x0c,0x04,0x00},
		pat_fold_off[7] = {0x00,0x00,0x3e,0x1c,0x08,0x00,0x00},
		pat_close[7] = {0x22,0x77,0x3e,0x1c,0x3e,0x77,0x22},
		pat_sw_up[7] = {0x08,0x1c,0x36,0x6b,0x1c,0x36,0x63},
		pat_sw_down[7] = {0x63,0x36,0x1c,0x6b,0x36,0x1c,0x08};

	bttsize = wg->h - _PADDING_VERT * 2;

	//背景

	mPixbufFillBox(pixbuf, 0, 0, wg->w, wg->h, MSYSCOL(FACE_DARKER));

	//タイトル

	if(p->dh.title)
	{
		x = (p->dh.style & MDOCKHEADER_S_HAVE_FOLD)? 3 + bttsize + 3: 3;

		x2 = p->wg.w - 3;
		if(p->dh.style & MDOCKHEADER_S_HAVE_CLOSE) x2 -= bttsize + 3;
		if(p->dh.style & MDOCKHEADER_S_HAVE_SWITCH) x2 -= bttsize + 3;

		if(mPixbufSetClipBox_d(pixbuf, x, _PADDING_VERT, x2 - x, bttsize))
			mFontDrawText(mWidgetGetFont(wg), pixbuf, x, _PADDING_VERT, p->dh.title, -1, MSYSCOL_RGB(TEXT));

		mPixbufClipNone(pixbuf);
	}

	//折りたたみボタン

	if(p->dh.style & MDOCKHEADER_S_HAVE_FOLD)
	{
		_draw_button(pixbuf, 3, _PADDING_VERT,
			(p->dh.style & MDOCKHEADER_S_FOLDED)? pat_fold_on: pat_fold_off,
			bttsize,
			(p->dh.fpress == _PRESSF_FOLD));
	}

	x = wg->w - 3 - bttsize;

	//閉じるボタン

	if(p->dh.style & MDOCKHEADER_S_HAVE_CLOSE)
	{
		_draw_button(pixbuf, x, 1, pat_close, bttsize, (p->dh.fpress == _PRESSF_CLOSE));

		x -= bttsize + 3;
	}

	//切り替えボタン

	if(p->dh.style & MDOCKHEADER_S_HAVE_SWITCH)
	{
		_draw_button(pixbuf, x, 1,
			(p->dh.style & MDOCKHEADER_S_SWITCH_DOWN)? pat_sw_down: pat_sw_up,
			bttsize,
			(p->dh.fpress == _PRESSF_SWITCH));
	}
}
