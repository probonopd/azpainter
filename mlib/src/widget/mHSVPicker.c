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
 * mHSVPicker
 *****************************************/

#include "mDef.h"

#include "mHSVPicker.h"

#include "mWidget.h"
#include "mPixbuf.h"
#include "mEvent.h"
#include "mSysCol.h"
#include "mColorConv.h"


//-------------------

#define _SPACE_W   5
#define _HUE_W     13

#define _BTTF_H    1
#define _BTTF_SV   2

//-------------------


/*************************//**

@defgroup hsvpicker mHSVPicker
@brief HSV 色ピッカー

<h3>継承</h3>
mWidget \> mHSVPicker

@ingroup group_widget
@{

@file mHSVPicker.h
@def M_HSVPICKER(p)
@struct mHSVPickerData
@struct mHSVPicker
@enum MHSVPICKER_NOTIFY

@var MHSVPICKER_NOTIFY::MHSVPICKER_N_CHANGE_H
色相値が変化した。\n
param1 : RGB色

@var MHSVPICKER_NOTIFY::MHSVPICKER_N_CHANGE_SV
SV 値が変化した。\n
param1 : RGB色

*************************/


//============================
// 描画
//============================


/** SV 部分描画 */

static void _drawSV(mHSVPicker *p)
{
	int x,y,h,hue,div;
	mPixbuf *img = p->hsv.img;

	h = p->wg.h;
	hue = p->hsv.curH * 360 / h;
	div = h - 1;

	for(y = 0; y < h; y++)
	{
		for(x = 0; x < h; x++)
		{
			mPixbufSetPixelRGB(img, x, y,
				mHSVtoRGB_fast(hue, x * 255 / div, 255 - y * 255 / div));
		}
	}
}

/** H カーソル描画 */

static void _drawHCur(mHSVPicker *p)
{
	int x,y;

	x = p->wg.h + _SPACE_W;
	y = p->hsv.curH;

	mPixbufLine(p->hsv.img, x    , y    , x + 4, y - 4, MPIXBUF_COL_XOR);
	mPixbufLine(p->hsv.img, x + 1, y + 1, x + 4, y + 4, MPIXBUF_COL_XOR);
	mPixbufLine(p->hsv.img, x + 5, y - 5, x + 5, y + 5, MPIXBUF_COL_XOR);
}

/** SV カーソル描画 */

static void _drawSVCur(mHSVPicker *p)
{
	mPixbufBoxSlow(p->hsv.img, p->hsv.curX - 3, p->hsv.curY - 3, 7, 7, MPIXBUF_COL_XOR);
}


//============================
// sub
//============================


/** イメージ初期化 */

static void _init_image(mHSVPicker *p)
{
	int i,h;
	uint8_t *pd;
	mPixbuf *img;
	uint32_t col;

	img = p->hsv.img;
	h = p->wg.h;

	//SV

	_drawSV(p);

	//間の空白

	mPixbufFillBox(img, h, 0, _SPACE_W, h, MSYSCOL(FACE));

	//色相 (H)

	pd = mPixbufGetBufPt(img, h + _SPACE_W, 0);

	for(i = 0; i < h; i++, pd += img->pitch_dir)
	{
		col = mHSVtoRGB_fast(i * 360 / h, 255, 255);
		mPixbufRawLineH(img, pd, _HUE_W, mRGBtoPix(col));
	}

	//カーソル

	_drawHCur(p);
	_drawSVCur(p);
}

/** 現在のカーソル位置から色計算 */

static void _calcCol(mHSVPicker *p)
{
	int h = p->wg.h;

	p->hsv.col = mHSVtoRGB_pac((double)p->hsv.curH / h,
		(double)p->hsv.curX / (h - 1),
		1.0 - (double)p->hsv.curY / (h - 1));
}

/** H 変更時 */

static void _changeH(mHSVPicker *p,int y,mBool notify)
{
	if(y < 0) y = 0;
	else if(y >= p->wg.h) y = p->wg.h - 1;

	if(p->hsv.curH == y) return;

	//描画

	_drawHCur(p);
	_drawSVCur(p);

	p->hsv.curH = y;

	_drawSV(p);

	_drawHCur(p);
	_drawSVCur(p);

	mWidgetUpdate(M_WIDGET(p));

	//色

	_calcCol(p);

	//通知

	if(notify)
		mWidgetAppendEvent_notify(NULL, M_WIDGET(p), MHSVPICKER_N_CHANGE_H, p->hsv.col, 0);
}

/** SV 位置変更時 */

static void _changeSV(mHSVPicker *p,int x,int y,mBool notify)
{
	if(x < 0) x = 0;
	else if(x >= p->wg.h) x = p->wg.h - 1;

	if(y < 0) y = 0;
	else if(y >= p->wg.h) y = p->wg.h - 1;

	if(x == p->hsv.curX && y == p->hsv.curY) return;

	//SV

	_drawSVCur(p);

	p->hsv.curX = x;
	p->hsv.curY = y;

	_drawSVCur(p);

	mWidgetUpdate(M_WIDGET(p));

	//色

	_calcCol(p);

	//通知

	if(notify)
		mWidgetAppendEvent_notify(NULL, M_WIDGET(p), MHSVPICKER_N_CHANGE_SV, p->hsv.col, 0);
}


//============================
//
//============================


/** 解放処理 */

void mHSVPickerDestroyHandle(mWidget *p)
{
	mPixbufFree(M_HSVPICKER(p)->hsv.img);
}

/** 作成 */

mHSVPicker *mHSVPickerCreate(mWidget *parent,int id,int height,uint32_t marginB4)
{
	mHSVPicker *p;

	p = mHSVPickerNew(0, parent, 0, height);
	if(!p) return NULL;

	p->wg.id = id;

	mWidgetSetMargin_b4(M_WIDGET(p), marginB4);

	return p;
}

/** 作成 */

mHSVPicker *mHSVPickerNew(int size,mWidget *parent,uint32_t style,int height)
{
	mHSVPicker *p;
	
	if(size < sizeof(mHSVPicker)) size = sizeof(mHSVPicker);
	
	p = (mHSVPicker *)mWidgetNew(size, parent);
	if(!p) return NULL;
	
	p->wg.destroy = mHSVPickerDestroyHandle;
	p->wg.draw = mHSVPickerDrawHandle;
	p->wg.event = mHSVPickerEventHandle;

	p->wg.fLayout |= MLF_FIX_WH;
	p->wg.fEventFilter |= MWIDGET_EVENTFILTER_POINTER;

	p->wg.w = height + _SPACE_W + _HUE_W;
	p->wg.h = height;

	p->hsv.col = 0xffffff;

	p->hsv.img = mPixbufCreate(p->wg.w, p->wg.h);

	_init_image(p);

	return p;
}

/** RGB 色取得 */

mRgbCol mHSVPickerGetRGBColor(mHSVPicker *p)
{
	return p->hsv.col;
}

/** 現在の HSV 値取得
 *
 * @param dst HSV の順で３つ。0.0-1.0 */

void mHSVPickerGetHSVColor(mHSVPicker *p,double *dst)
{
	int h = p->wg.h;

	dst[0] = (double)p->hsv.curH / h;
	dst[1] = (double)p->hsv.curX / (h - 1);
	dst[2] = 1.0 - (double)p->hsv.curY / (h - 1);
}

/** 色相値変更 */

void mHSVPickerSetHue(mHSVPicker *p,int hue)
{
	if(hue < 0) hue = 0;
	else if(hue > 359) hue %= 360;

	_changeH(p, hue * p->wg.h / 360, FALSE);
}

/** SV 値変更 */

void mHSVPickerSetSV(mHSVPicker *p,double s,double v)
{
	_changeSV(p,
		(int)(s * (p->wg.h - 1) + 0.5),
		(int)((1.0 - v) * (p->wg.h - 1) + 0.5),
		FALSE);
}

/** HSV 値をセット
 *
 * @param h,s,v  0.0-1.0 */

void mHSVPickerSetHSVColor(mHSVPicker *p,double h,double s,double v)
{
	int size = p->wg.h;

	//HSV -> RGB

	p->hsv.col = mHSVtoRGB_pac(h, s, v);

	//カーソル消去

	_drawHCur(p);
	_drawSVCur(p);

	//HSV

	p->hsv.curH = (int)(h * size + 0.5);
	p->hsv.curX = (int)(s * (size - 1) + 0.5);
	p->hsv.curY = (int)((1.0 - v) * (size - 1) + 0.5);

	if(p->hsv.curH >= size) p->hsv.curH = size - 1;

	//描画

	_drawSV(p);
	_drawHCur(p);
	_drawSVCur(p);

	mWidgetUpdate(M_WIDGET(p));
}

/** RGB 値をセット */

void mHSVPickerSetRGBColor(mHSVPicker *p,mRgbCol col)
{
	double hsv[3];
	int h = p->wg.h;

	col &= 0xffffff;

	p->hsv.col = col;

	//カーソル消去

	_drawHCur(p);
	_drawSVCur(p);

	//RGB -> HSV

	mRGBtoHSV_pac(col, hsv);

	p->hsv.curH = (int)(hsv[0] * h + 0.5);
	p->hsv.curX = (int)(hsv[1] * (h - 1) + 0.5);
	p->hsv.curY = (int)((1.0 - hsv[2]) * (h - 1) + 0.5);

	if(p->hsv.curH >= h) p->hsv.curH = h - 1;

	//描画

	_drawSV(p);
	_drawHCur(p);
	_drawSVCur(p);

	mWidgetUpdate(M_WIDGET(p));
}

/** 表示用イメージを再描画 */

void mHSVPickerRedraw(mHSVPicker *p)
{
	_init_image(p);
}


//========================
// ハンドラ
//========================


/** イベント */

int mHSVPickerEventHandle(mWidget *wg,mEvent *ev)
{
	mHSVPicker *p = M_HSVPICKER(wg);

	switch(ev->type)
	{
		case MEVENT_POINTER:
			if(ev->pt.type == MEVENT_POINTER_TYPE_MOTION)
			{
				//移動
				
				if(p->hsv.fbtt == _BTTF_H)
					_changeH(p, ev->pt.y, TRUE);
				else if(p->hsv.fbtt == _BTTF_SV)
					_changeSV(p, ev->pt.x, ev->pt.y, TRUE);
			}
			else if(ev->pt.type == MEVENT_POINTER_TYPE_PRESS)
			{
				//押し

				if(ev->pt.btt == M_BTT_LEFT && p->hsv.fbtt == 0)
				{
					if(ev->pt.x < p->wg.h)
					{
						//SV

						p->hsv.fbtt = _BTTF_SV;
						_changeSV(p, ev->pt.x, ev->pt.y, TRUE);
					}
					else if(ev->pt.x >= p->wg.h + _SPACE_W)
					{
						//H

						p->hsv.fbtt = _BTTF_H;
						_changeH(p, ev->pt.y, TRUE);
					}

					//grab

					if(p->hsv.fbtt)
						mWidgetGrabPointer(wg);
				}
			}
			else if(ev->pt.type == MEVENT_POINTER_TYPE_RELEASE)
			{
				//離し

				if(ev->pt.btt == M_BTT_LEFT && p->hsv.fbtt)
				{
					p->hsv.fbtt = 0;
					mWidgetUngrabPointer(wg);
				}
			}
			break;

		//フォーカスアウト
		case MEVENT_FOCUS:
			if(ev->focus.bOut && p->hsv.fbtt)
			{
				p->hsv.fbtt = 0;
				mWidgetUngrabPointer(wg);
			}
			break;
		default:
			return FALSE;
	}

	return TRUE;
}

/** 描画 */

void mHSVPickerDrawHandle(mWidget *wg,mPixbuf *pixbuf)
{
	mPixbufBlt(pixbuf, 0, 0, M_HSVPICKER(wg)->hsv.img, 0, 0, -1, -1);
}

/** @} */
