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
 * mSliderBar [スライダー]
 *****************************************/

#include "mDef.h"

#include "mSliderBar.h"

#include "mWidget.h"
#include "mPixbuf.h"
#include "mEvent.h"
#include "mKeyDef.h"
#include "mSysCol.h"


/**************//**

@defgroup sliderbar mSliderBar
@brief スライダーバー

通知の処理はスクロールバーと同じ。

<h3>継承</h3>
mWidget \> mSliderBar

<h3>キー操作</h3>
上下 or 左右キーで -1 or +1。\n
HOME/END キーで先頭/終端。

@ingroup group_widget
@{

@file mSliderBar.h
@def M_SLIDERBAR(p)
@struct _mSliderBar
@struct mSliderBarData
@enum MSLIDERBAR_STYLE
@enum MSLIDERBAR_NOTIFY
@enum MSLIDERBAR_HANDLE_FLAGS

@var MSLIDERBAR_NOTIFY::MSLIDERBAR_N_HANDLE
つまみが操作された時。\n
param1 : 現在位置\n
param2 : フラグ (MSLIDERBAR_HANDLE_FLAGS)

******************/


//=============================
// sub
//=============================


/** 内部での位置変更時 */

static void _changePos(mSliderBar *p,int pos,int flags)
{
	//位置変更

	if(mSliderBarSetPos(p, pos))
		flags |= MSLIDERBAR_HANDLE_F_CHANGE;
	
	//キー操作/ドラッグ中時は、位置が変わった時のみ
	
	if((flags & (MSLIDERBAR_HANDLE_F_KEY | MSLIDERBAR_HANDLE_F_MOTION))
		&& !(flags & MSLIDERBAR_HANDLE_F_CHANGE))
		return;
	
	//通知

	if(p->sl.handle)
		(p->sl.handle)(p, pos, flags);
}

/** ハンドラデフォルト */

static void _handle_default(mSliderBar *p,int pos,int flags)
{
	mWidgetAppendEvent_notify(NULL, M_WIDGET(p),
		MSLIDERBAR_N_HANDLE, pos, flags);
}


//=============================
//
//=============================


/** 作成 */

mSliderBar *mSliderBarCreate(mWidget *parent,int id,uint32_t style,
	uint32_t fLayout,uint32_t marginB4)
{
	mSliderBar *p;

	p = mSliderBarNew(0, parent, style);
	if(!p) return NULL;

	p->wg.id = id;
	p->wg.fLayout = fLayout;

	mWidgetSetMargin_b4(M_WIDGET(p), marginB4);

	return p;
}

/** 作成 */

mSliderBar *mSliderBarNew(int size,mWidget *parent,uint32_t style)
{
	mSliderBar *p;
	int n;
	
	if(size < sizeof(mSliderBar)) size = sizeof(mSliderBar);
	
	p = (mSliderBar *)mWidgetNew(size, parent);
	if(!p) return NULL;
	
	p->wg.draw = mSliderBarDrawHandle;
	p->wg.event = mSliderBarEventHandle;
	
	p->wg.fState |= MWIDGET_STATE_TAKE_FOCUS;
	p->wg.fEventFilter |= MWIDGET_EVENTFILTER_POINTER | MWIDGET_EVENTFILTER_KEY;

	p->sl.style = style;
	p->sl.barw_hf = (style & MSLIDERBAR_S_SMALL)? 3: 4;
	p->sl.handle = _handle_default;
	p->sl.max = 100;
	
	//サイズ
	
	n = (style & MSLIDERBAR_S_SMALL)? 11: 15;
	
	if(style & MSLIDERBAR_S_VERT)
	{
		p->wg.hintW = n;
		p->wg.hintH = (p->sl.barw_hf * 2 + 1) + 4;
	}
	else
	{
		p->wg.hintW = (p->sl.barw_hf * 2 + 1) + 4;
		p->wg.hintH = n;
	}
	
	return p;
}

/** 位置を取得 */

int mSliderBarGetPos(mSliderBar *p)
{
	return p->sl.pos;
}

/** ステータスセット */

void mSliderBarSetStatus(mSliderBar *p,int min,int max,int pos)
{
	if(min >= max) max = min + 1;
	
	if(pos < min) pos = min;
	else if(pos > max) pos = max;
	
	p->sl.min = min;
	p->sl.max = max;
	p->sl.pos = pos;
	
	mWidgetUpdate(M_WIDGET(p));
}

/** 範囲をセット */

void mSliderBarSetRange(mSliderBar *p,int min,int max)
{
	if(min >= max) max = min + 1;
	
	p->sl.min = min;
	p->sl.max = max;

	if(p->sl.pos < min)
		p->sl.pos = min;
	else if(p->sl.pos > max)
		p->sl.pos = max;
	
	mWidgetUpdate(M_WIDGET(p));
}

/** 位置セット */

mBool mSliderBarSetPos(mSliderBar *p,int pos)
{
	if(pos < p->sl.min) pos = p->sl.min;
	else if(pos > p->sl.max) pos = p->sl.max;
	
	if(pos == p->sl.pos)
		return FALSE;
	else
	{
		p->sl.pos = pos;
		mWidgetUpdate(M_WIDGET(p));
		return TRUE;
	}
}


//========================
// ハンドラ
//========================


/** ドラッグ時 */

static mBool _ondrag(mSliderBar *p,int x,int y,int flags)
{
	int w,pos,n;
	
	if(p->sl.style & MSLIDERBAR_S_VERT)
		w = p->wg.h, n = y;
	else
		w = p->wg.w, n = x;

	//端の余白部分を除く
	
	w -= p->sl.barw_hf * 2 + 1;
	if(w <= 0) return FALSE;

	//位置
	
	n -= p->sl.barw_hf;
	
	if(n < 0) n = 0;
	else if(n > w) n = w;
	
	pos = (int)((double)n / w * (p->sl.max - p->sl.min) + p->sl.min + 0.5);
	
	_changePos(p, pos, flags);
	
	return TRUE;
}

/** キー押し時 */

static int _event_keydown(mSliderBar *p,uint32_t key)
{
	int pos,vert;

	if(p->sl.fpress) return 0;
	
	pos = p->sl.pos;
	vert = (p->sl.style & MSLIDERBAR_S_VERT);
	
	switch(key)
	{
		case MKEY_LEFT:
			if(!vert) pos--;
			break;
		case MKEY_RIGHT:
			if(!vert) pos++;
			break;
		case MKEY_UP:
			if(vert) pos--;
			break;
		case MKEY_DOWN:
			if(vert) pos++;
			break;
		case MKEY_HOME:
			pos = p->sl.min;
			break;
		case MKEY_END:
			pos = p->sl.max;
			break;
		default:
			return 0;
	}
	
	_changePos(p, pos, MSLIDERBAR_HANDLE_F_KEY);
	
	return 1;
}

/** ドラッグ終了 */

static void _endDrag(mSliderBar *p)
{
	if(p->sl.fpress)
	{
		p->sl.fpress = 0;
		mWidgetUngrabPointer(M_WIDGET(p));
		
		_changePos(p, p->sl.pos, MSLIDERBAR_HANDLE_F_RELEASE);
	}
}

/** イベント */

int mSliderBarEventHandle(mWidget *wg,mEvent *ev)
{
	mSliderBar *p = M_SLIDERBAR(wg);

	switch(ev->type)
	{
		case MEVENT_POINTER:
			if(ev->pt.type == MEVENT_POINTER_TYPE_MOTION)
			{
				//移動
				
				if(p->sl.fpress)
					_ondrag(p, ev->pt.x, ev->pt.y, MSLIDERBAR_HANDLE_F_MOTION);
			}
			else if(ev->pt.type == MEVENT_POINTER_TYPE_PRESS)
			{
				//押し
				
				if(ev->pt.btt == M_BTT_LEFT && p->sl.fpress == 0)
				{
					if(_ondrag(p, ev->pt.x, ev->pt.y, MSLIDERBAR_HANDLE_F_PRESS))
					{
						mWidgetSetFocus_update(wg, FALSE);
						mWidgetGrabPointer(wg);
						
						p->sl.fpress = 1;
					}
				}
			}
			else if(ev->pt.type == MEVENT_POINTER_TYPE_RELEASE)
			{
				//離し

				if(ev->pt.btt == M_BTT_LEFT)
					_endDrag(p);
			}
			break;

		//キー
		case MEVENT_KEYDOWN:
			return _event_keydown(p, ev->key.code);
		
		case MEVENT_FOCUS:
			if(ev->focus.bOut)
				_endDrag(p);
		
			mWidgetUpdate(wg);
			break;
		default:
			return FALSE;
	}

	return TRUE;
}


//========================
// 描画
//========================


/** 枠色取得 */

static uint32_t _getFrameCol(mSliderBar *p)
{
	if(p->wg.fState & MWIDGET_STATE_FOCUSED)
		return MSYSCOL(FRAME_FOCUS);
	else if(p->wg.fState & MWIDGET_STATE_ENABLED)
		return MSYSCOL(FRAME);
	else
		return MSYSCOL(FRAME_LIGHT);
}

/** 水平 */

static void _draw_horz(mSliderBar *p,mPixbuf *pixbuf)
{
	int hf,x;
	uint32_t col;
	
	hf = p->sl.barw_hf;

	col = _getFrameCol(p);
	
	//バー
	
	mPixbufBox(pixbuf, hf, p->wg.h / 2 - 1, p->wg.w - hf * 2, 3, col);
	
	//つまみ
	
	x = (int)((double)(p->wg.w - hf * 2 - 1) / (p->sl.max - p->sl.min)
				* (p->sl.pos - p->sl.min) + 0.5);

	mPixbufBox(pixbuf, x, 0, hf * 2 + 1, p->wg.h, col);
		
	mPixbufFillBox(pixbuf, x + 1, 1, hf * 2 + 1 - 2, p->wg.h - 2, MSYSCOL(FACE));

	mPixbufLineV(pixbuf, x + hf, 2, p->wg.h - 4, MSYSCOL(FRAME));
}

/** 垂直 */

static void _draw_vert(mSliderBar *p,mPixbuf *pixbuf)
{
	int hf,y;
	uint32_t col;
	
	hf = p->sl.barw_hf;
	
	col = _getFrameCol(p);

	//バー
	
	mPixbufBox(pixbuf, p->wg.w / 2 - 1, hf, 3, p->wg.h - hf * 2, col);
	
	//つまみ
	
	y = (int)((double)(p->wg.h - hf * 2 - 1) / (p->sl.max - p->sl.min)
				* (p->sl.pos - p->sl.min) + 0.5);
	
	mPixbufBox(pixbuf, 0, y, p->wg.w, hf * 2 + 1, col);
		
	mPixbufFillBox(pixbuf, 1, y + 1, p->wg.w - 2, hf * 2 + 1 - 2, MSYSCOL(FACE));

	mPixbufLineH(pixbuf, 2, y + hf, p->wg.w - 4, MSYSCOL(FRAME));
}

/** 描画 */

void mSliderBarDrawHandle(mWidget *wg,mPixbuf *pixbuf)
{
	mSliderBar *p = M_SLIDERBAR(wg);
	
	//背景
	
	mWidgetDrawBkgnd(wg, NULL);
	
	//バー
	
	if(p->sl.style & MSLIDERBAR_S_VERT)
		_draw_vert(p, pixbuf);
	else
		_draw_horz(p, pixbuf);
}

/** @} */
