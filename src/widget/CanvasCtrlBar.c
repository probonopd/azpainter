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
 * CanvasCtrlBar
 *
 * DockCanvasCtrl で使う、表示倍率・回転角度バーのウィジェット
 *****************************************/
/*
 * <通知>
 * 
 * - CANVASCTRLBAR_N_BAR (バーが操作された時)
 *     param1 : CANVASCTRLBAR_N_BAR_TYPE_*
 *     param2 : 位置
 *
 * - CANVASCTRLBAR_N_BUTTON (ボタンが押された時)
 *     param1 : [0]- [1]+ [2]reset
 * 
 */


#include "mDef.h"
#include "mWidgetDef.h"
#include "mWidget.h"
#include "mPixbuf.h"
#include "mEvent.h"
#include "mSysCol.h"
#include "mUtilStr.h"

#include "CanvasCtrlBar.h"
#include "defMacros.h"
#include "dataImagePattern.h"


//----------------------

struct _CanvasCtrlBar
{
	mWidget wg;

	int type,min,max,pos,fpress;
};

//----------------------

#define _CCB(p)  ((CanvasCtrlBar *)(p))

#define _HEIGHT          13
#define _SPACE_BAR_LEFT  2
#define _SPACE_BAR_RIGHT 4
#define _BTT_ALLW        ((_HEIGHT + 1) * 3 - 1)

#define _PRESS_F_BAR 1
#define _PRESS_F_BTT 2

static int _event_handle(mWidget *wg,mEvent *ev);
static void _draw_handle(mWidget *wg,mPixbuf *pixbuf);

//----------------------


/** 作成 */

CanvasCtrlBar *CanvasCtrlBar_new(mWidget *parent,int type)
{
	CanvasCtrlBar *p;
	
	p = (CanvasCtrlBar *)mWidgetNew(sizeof(CanvasCtrlBar), parent);
	if(!p) return NULL;
	
	p->wg.draw = _draw_handle;
	p->wg.event = _event_handle;
	p->wg.fEventFilter |= MWIDGET_EVENTFILTER_POINTER;
	p->wg.fLayout |= MLF_EXPAND_W;
	p->wg.hintW = _SPACE_BAR_LEFT + _SPACE_BAR_RIGHT + _BTT_ALLW + 5;
	p->wg.hintH = _HEIGHT;

	p->type = type;

	if(type == CANVASCTRLBAR_TYPE_ZOOM)
	{
		p->min = CANVAS_ZOOM_MIN;
		p->max = CANVAS_ZOOM_MAX;
		p->pos = 1000;
	}
	else
	{
		p->min = -1800;
		p->max = 1800;
	}
	
	return p;
}

/** 位置セット */

void CanvasCtrlBar_setPos(CanvasCtrlBar *p,int pos)
{
	if(pos != p->pos)
	{
		p->pos = pos;
		mWidgetUpdate(M_WIDGET(p));
	}
}

/** 位置変更 */

static mBool _change_pos(CanvasCtrlBar *p,int x)
{
	int barw,ct,pos;

	x -= _SPACE_BAR_LEFT;

	barw = p->wg.w - (_SPACE_BAR_LEFT + _SPACE_BAR_RIGHT + _BTT_ALLW);
	ct = barw >> 1;

	if(x < 0) x = 0;
	else if(x >= barw) x = barw - 1;

	//位置

	if(p->type == CANVASCTRLBAR_TYPE_ANGLE)
	{
		if(x >= ct)
			pos = (int)((x - ct) * 1800.0 / (barw - 1 - ct) + 0.5);
		else
			pos = (int)((x - ct) * 1800.0 / ct - 0.5);
	}
	else
	{
		//100% 以上の場合は 10% 単位

		if(x >= ct)
		{
			pos = (int)((double)(x - ct) * ((p->max - 1000) / 100) / (barw - 1 - ct) + 0.5);
			pos = pos * 100 + 1000;
		}
		else
			pos = (int)((double)x * (1000 - p->min) / ct + 0.5) + p->min;
	}

	//セット

	if(pos != p->pos)
	{
		p->pos = pos;

		mWidgetUpdate(M_WIDGET(p));
		
		return TRUE;
	}

	return FALSE;
}

/** バー操作の通知 */

static void _send_notify_bar(CanvasCtrlBar *p,int type)
{
	mWidgetAppendEvent_notify(NULL, M_WIDGET(p),
		CANVASCTRLBAR_N_BAR, type, p->pos);
}

/** ボタン押し時 */

static void _event_press(CanvasCtrlBar *p,mEvent *ev)
{
	int i,x;

	x = p->wg.w - _BTT_ALLW;

	if(ev->pt.x >= x)
	{
		//ボタン

		for(i = 0; i < 3; i++, x += _HEIGHT + 1)
		{
			if(x <= ev->pt.x && ev->pt.x < x + _HEIGHT)
			{
				p->fpress = _PRESS_F_BTT + i;

				mWidgetGrabPointer(M_WIDGET(p));
				mWidgetUpdate(M_WIDGET(p));
			
				break;
			}
		}
	}
	else
	{
		//バー

		p->fpress = _PRESS_F_BAR;

		mWidgetGrabPointer(M_WIDGET(p));

		_change_pos(p, ev->pt.x);
		_send_notify_bar(p, CANVASCTRLBAR_N_BAR_TYPE_PRESS);
	}
}

/** グラブ解放 */

static void _release_grab(CanvasCtrlBar *p,mBool send)
{
	if(p->fpress)
	{
		mWidgetUngrabPointer(M_WIDGET(p));

		if(p->fpress == _PRESS_F_BAR)
		{
			//バー

			_send_notify_bar(p, CANVASCTRLBAR_N_BAR_TYPE_RELEASE);
		}
		else
		{
			//ボタン
			
			mWidgetUpdate(M_WIDGET(p));

			if(send)
			{
				mWidgetAppendEvent_notify(NULL, M_WIDGET(p),
					CANVASCTRLBAR_N_BUTTON, p->fpress - _PRESS_F_BTT, 0);
			}
		}

		p->fpress = 0;
	}
}

/** イベント */

int _event_handle(mWidget *wg,mEvent *ev)
{
	CanvasCtrlBar *p = _CCB(wg);

	switch(ev->type)
	{
		case MEVENT_POINTER:
			if(ev->pt.type == MEVENT_POINTER_TYPE_MOTION)
			{
				//移動
				
				if(p->fpress == _PRESS_F_BAR)
				{
					if(_change_pos(p, ev->pt.x))
						_send_notify_bar(p, CANVASCTRLBAR_N_BAR_TYPE_MOTION);
				}
			}
			else if(ev->pt.type == MEVENT_POINTER_TYPE_PRESS
				|| ev->pt.type == MEVENT_POINTER_TYPE_DBLCLK)
			{
				//押し
				
				if(ev->pt.btt == M_BTT_LEFT && p->fpress == 0)
					_event_press(p, ev);
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

/** 描画 */

void _draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	CanvasCtrlBar *p = _CCB(wg);
	int i,n,barw,center;
	char m[16],*pc;
	mBool bAngle,bPress;
	uint8_t pat_btt[3][7] = {
		{0x10,0x18,0x1c,0x1e,0x1c,0x18,0x10},
		{0x04,0x0c,0x1c,0x3c,0x1c,0x0c,0x04},
		{0x00,0x00,0x7f,0x7f,0x7f,0x00,0x00}
	};

	bAngle = (p->type == CANVASCTRLBAR_TYPE_ANGLE);

	barw = wg->w - (_SPACE_BAR_LEFT + _SPACE_BAR_RIGHT + _BTT_ALLW);
	center = barw >> 1;

	//背景

	mPixbufFillBox(pixbuf, 0, 0, wg->w, wg->h, MSYSCOL(FACE));

	//バー

	mPixbufBox(pixbuf, _SPACE_BAR_LEFT, 0, barw, 2, MSYSCOL(FRAME));

	//中央線

	mPixbufLineV(pixbuf, _SPACE_BAR_LEFT + center, 3, wg->h - 3, MSYSCOL(FRAME));

	//カーソル

	if(bAngle)
	{
		n = (p->pos >= 0)? barw - 1 - center: center;
		n = (int)(center + p->pos * n / 1800.0 + 0.5);
	}
	else
	{
		if(p->pos >= 1000)
			n = (int)(center + (double)(p->pos - 1000) * (barw - 1 - center) / (p->max - 1000) + 0.5);
		else
			n = (int)((double)(p->pos - p->min) * center / (1000 - p->min) + 0.5);
	}

	mPixbufDrawArrowUp(pixbuf, _SPACE_BAR_LEFT + n, 4, MSYSCOL(TEXT));

	//数値

	mFloatIntToStr(m, p->pos, 1);

	for(pc = m; *pc; pc++)
	{
		if(*pc == '-')
			*pc = 10;
		else if(*pc == '.')
			*pc = 11;
		else
			*pc = *pc - '0';
	}

	*(pc++) = 12 + p->type;
	*pc = 255;

	if((bAngle && p->pos >= 0) || (!bAngle && p->pos >= 1000))
		n = _SPACE_BAR_LEFT;
	else
		n = _SPACE_BAR_LEFT + barw - (pc - m) * 5;

	mPixbufDrawBitPatternSum(pixbuf, n, _HEIGHT - 9,
		g_imgpat_number_5x9, IMGPAT_NUMBER_5x9_PATW, 9, 5,
		(uint8_t *)m, MSYSCOL(TEXT));

	//ボタン

	n = wg->w - _BTT_ALLW;

	for(i = 0; i < 3; i++)
	{
		bPress = (p->fpress == _PRESS_F_BTT + i);
	
		mPixbufBox(pixbuf, n, 0, _HEIGHT, _HEIGHT, MSYSCOL(FRAME));
		
		mPixbufFillBox(pixbuf, n + 1, 1, _HEIGHT - 2, _HEIGHT - 2,
			(bPress)? MSYSCOL(FACE_DARKER): MSYSCOL(FACE_DARK));
		
		mPixbufDrawBitPattern(pixbuf, n + 3 + bPress, 3 + bPress,
			pat_btt[i], 7, 7, MSYSCOL(TEXT));

		n += _HEIGHT + 1; 
	}
}
