/*$
 Copyright (C) 2013-2017 Azel.

 This file is part of AzPainter.

 AzPainter is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 AzPainter is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
$*/

/*****************************************
 * ValueBar
 *
 * スピンと数値表示付きの値選択バー
 *****************************************/
/*
 * [Ctrl+左右ドラッグ] で相対移動
 *
 * <通知>
 * param1: 位置。
 * param2: 値が確定した時1。押し時やドラッグ中は0。
 * 
 */

#include "mDef.h"
#include "mWidgetDef.h"
#include "mWidget.h"
#include "mPixbuf.h"
#include "mEvent.h"
#include "mSysCol.h"
#include "mUtilStr.h"

#include "ValueBar.h"
#include "dataImagePattern.h"


//-------------------------

struct _ValueBar
{
	mWidget wg;

	int min,max,pos,dig,
		fpress,lastx;
};

//-------------------------


//========================
// sub
//========================


/** 通知 */

static void _notify(ValueBar *p,int flag)
{
	mWidgetAppendEvent_notify(NULL, M_WIDGET(p), 0, p->pos, flag);
}

/** 位置変更 */

static void _change_pos(ValueBar *p,int x)
{
	int pos;

	pos = (int)((double)x * (p->max - p->min) / (p->wg.w - 2 - 8) + 0.5) + p->min;

	if(pos < p->min)
		pos = p->min;
	else if(pos > p->max)
		pos = p->max;

	if(pos != p->pos)
	{
		p->pos = pos;

		mWidgetUpdate(M_WIDGET(p));

		_notify(p, 0);
	}
}

/** 押し時 */

static void _event_press(ValueBar *p,mEvent *ev)
{
	if(ev->pt.x >= p->wg.w - 8)
	{
		//スピン

		if(ValueBar_setPos(p, p->pos + ((ev->pt.y >= p->wg.h / 2)? -1: 1)))
			_notify(p, 1);
	}
	else
	{
		//バー部分

		p->fpress = (ev->pt.state & M_MODS_CTRL)? 2: 1;
		p->lastx = ev->pt.x;

		mWidgetGrabPointer(M_WIDGET(p));

		if(p->fpress == 1)
			_change_pos(p, ev->pt.x);
	}
}

/** グラブ解除 */

static void _release_grab(ValueBar *p)
{
	if(p->fpress)
	{
		p->fpress = 0;
		mWidgetUngrabPointer(M_WIDGET(p));

		_notify(p, 1);
	}
}


//========================


/** イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	ValueBar *p = VALUEBAR(wg);

	switch(ev->type)
	{
		case MEVENT_POINTER:
			if(ev->pt.type == MEVENT_POINTER_TYPE_MOTION)
			{
				//移動

				if(p->fpress == 1)
					_change_pos(p, ev->pt.x);
				else if(p->fpress == 2)
				{
					//+Ctrl 相対移動

					if(ValueBar_setPos(p, ev->pt.x - p->lastx + p->pos))
						_notify(p, 0);

					p->lastx = ev->pt.x;
				}
			}
			else if(ev->pt.type == MEVENT_POINTER_TYPE_PRESS
				|| ev->pt.type == MEVENT_POINTER_TYPE_DBLCLK)
			{
				//押し

				if(ev->pt.btt == M_BTT_LEFT && !p->fpress)
					_event_press(p, ev);
			}
			else if(ev->pt.type == MEVENT_POINTER_TYPE_RELEASE)
			{
				//離し
				
				if(ev->pt.btt == M_BTT_LEFT)
					_release_grab(p);
			}
			break;
		
		case MEVENT_FOCUS:
			if(ev->focus.bOut)
				_release_grab(p);
			break;
		default:
			return FALSE;
	}

	return TRUE;
}

/** 描画 */

static void _draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	ValueBar *p = VALUEBAR(wg);
	int n,n2;
	char m[32],*pc;
	mBool enabled;
	mPixCol colframe;

	enabled = ((wg->fState & MWIDGET_STATE_ENABLED) != 0);

	colframe = (enabled)? MSYSCOL(FRAME_DARK): MSYSCOL(FRAME);

	//枠

	mPixbufBox(pixbuf, 0, 0, wg->w, wg->h, colframe);

	//---- スピン

	//背景

	mPixbufFillBox(pixbuf, wg->w - 8, 1, 7, wg->h - 2, MSYSCOL(FACE_DARKER));

	//縦線

	mPixbufLineV(pixbuf, wg->w - 9, 1, wg->h - 2, colframe);

	//矢印

	mPixbufDrawArrowUp_small(pixbuf, wg->w - 5, 3, MSYSCOL(TEXT));
	mPixbufDrawArrowDown_small(pixbuf, wg->w - 5, wg->h - 5, MSYSCOL(TEXT));

	//---- バー

	if(enabled)
		n = (int)((double)(p->pos - p->min) * (wg->w - 2 - 8) / (p->max - p->min) + 0.5);
	else
		n = 0; //無効時はバー無し

	if(n)
	{
		mPixbufFillBox(pixbuf, 1, 1, n, wg->h - 2,
			(enabled)? MSYSCOL(FACE_FOCUS): MSYSCOL(FACE_DARKER));
	}

	//バー残り

	n2 = wg->w - 2 - 8 - n;

	if(n2 > 0)
	{
		mPixbufFillBox(pixbuf, 1 + n, 1, n2, wg->h - 2,
			(enabled)? MSYSCOL(FACE_LIGHTEST): MSYSCOL(FACE));
	}

	//------ 数値

	mFloatIntToStr(m, p->pos, p->dig);

	for(pc = m; *pc; pc++)
	{
		if(*pc == '-')
			*pc = 10;
		else if(*pc == '.')
			*pc = 11;
		else
			*pc -= '0';
	}

	*pc = 255;

	mPixbufDrawBitPatternSum(pixbuf,
		wg->w - 4 - 8 - (pc - m) * 5, 3,
		g_imgpat_number_5x9, IMGPAT_NUMBER_5x9_PATW, 9, 5,
		(uint8_t *)m, (enabled)? MSYSCOL(TEXT): MSYSCOL(TEXT_DISABLE));
}


//====================


/** 作成 */

ValueBar *ValueBar_new(mWidget *parent,int id,uint32_t fLayout,
	int dig,int min,int max,int pos)
{
	ValueBar *p;

	p = (ValueBar *)mWidgetNew(sizeof(ValueBar), parent);
	if(!p) return NULL;

	p->wg.id = id;
	p->wg.fLayout = fLayout;
	p->wg.fEventFilter |= MWIDGET_EVENTFILTER_POINTER;
	p->wg.event = _event_handle;
	p->wg.draw = _draw_handle;
	p->wg.hintW = 15;
	p->wg.hintH = 14;

	if(pos < min) pos = min;
	else if(pos > max) pos = max;

	p->dig = dig;
	p->min = min;
	p->max = max;
	p->pos = pos;

	return p;
}

/** 位置を取得 */

int ValueBar_getPos(ValueBar *p)
{
	return p->pos;
}

/** 位置をセット */

mBool ValueBar_setPos(ValueBar *p,int pos)
{
	if(pos < p->min)
		pos = p->min;
	else if(pos > p->max)
		pos = p->max;

	if(pos == p->pos)
		return FALSE;
	else
	{
		p->pos = pos;

		mWidgetUpdate(M_WIDGET(p));

		return TRUE;
	}
}

/** 範囲と位置をセット */

void ValueBar_setStatus(ValueBar *p,int min,int max,int pos)
{
	p->min = min;
	p->max = max;
	p->pos = pos;

	mWidgetUpdate(M_WIDGET(p));
}

/** 全ステータスセット */

void ValueBar_setStatus_dig(ValueBar *p,int dig,int min,int max,int pos)
{
	p->dig = dig;
	p->min = min;
	p->max = max;
	p->pos = pos;

	mWidgetUpdate(M_WIDGET(p));
}
