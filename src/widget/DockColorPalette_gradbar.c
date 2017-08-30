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
 * 中間色グラデーションバー
 *****************************************/
/*
 * [notify]
 * type   : 0-色選択  1-左の色変更 2-右の色変更
 * param1 : 色
 */

#include "mDef.h"
#include "mWidgetDef.h"
#include "mWidget.h"
#include "mSysCol.h"
#include "mPixbuf.h"
#include "mEvent.h"

#include "defDraw.h"


//---------------------

typedef struct
{
	mWidget wg;

	uint32_t col[2];
	int rgb[2][3],			//左右の RGB 値
		rgb_diff[3],		//左右の RGB 色差
		rgb_diff_step[3],	//左右の RGB 色差 (段階有効時)
		step,				//段階数
		fdrag,
		curx,			//ドラッグ時のカーソル位置
		last_gradx;		//ドラッグ時、前回のグラデーション部分X位置 (-1 で最初)
}GradBar;

//---------------------

#define _BAR_H    13
#define _BETWEEN  4		//端の色とグラデーションの間

//---------------------


//========================
// sub
//========================


/** 色から RGB 値取得してセット */

static void _set_rgb(GradBar *p)
{
	int i,j,sf;
	uint32_t c;

	//R,G,B

	for(i = 0; i < 2; i++)
	{
		c = p->col[i];
		
		for(j = 0, sf = 16; j < 3; j++, sf -= 8)
			p->rgb[i][j] = (c >> sf) & 255;
	}

	//色差

	for(i = 0; i < 3; i++)
	{
		j = p->rgb[1][i] - p->rgb[0][i];

		p->rgb_diff[i] = j;

		if((j & 1) && j > 0) j++;

		p->rgb_diff_step[i] = j;
	}
}

/** ドラッグ移動時、色を取得して通知 */

static void _on_motion(GradBar *p,int curx)
{
	int i,x,w,c[3],step,stepno;
	double d;

	x = curx - (_BAR_H + _BETWEEN + 1);  //グラデーション部分を先頭とした位置
	w = p->wg.w - (_BAR_H + _BETWEEN) * 2 - 2;

	if(x < 0) x = 0;
	else if(x >= w) x = w - 1;

	//前回位置と比較

	if(p->last_gradx == x) return;

	p->last_gradx = x;
	p->curx = x + _BAR_H + _BETWEEN + 1;

	//色取得

	step = p->step;

	if(step < 3)
	{
		//通常
		
		d = (double)x / (w - 1);

		for(i = 0; i < 3; i++)
			c[i] = (int)(p->rgb_diff[i] * d + p->rgb[0][i] + 0.5);
	}
	else
	{
		//段階あり

		stepno = x * step / w;

		if(stepno == step - 1)
		{
			//終端の色
			
			for(i = 0; i < 3; i++)
				c[i] = p->rgb[1][i];
		}
		else
		{
			for(i = 0; i < 3; i++)
				c[i] = stepno * p->rgb_diff_step[i] / (step - 1) + p->rgb[0][i];
		}
	}

	//通知

	mWidgetAppendEvent_notify(NULL, M_WIDGET(p), 0, M_RGB(c[0], c[1], c[2]), 0);

	mWidgetUpdate(M_WIDGET(p));
}


//========================
// ハンドラ
//========================


/** 押し時 */

static void _event_press(GradBar *p,int x)
{
	int w = p->wg.w,no;

	if(x < _BAR_H || x >= w - _BAR_H)
	{
		//左右の色 : 描画色セット

		no = (x >= w - _BAR_H);

		p->col[no] = APP_DRAW->col.drawcol;
		_set_rgb(p);

		mWidgetUpdate(M_WIDGET(p));

		mWidgetAppendEvent_notify(NULL, M_WIDGET(p), 1 + no, APP_DRAW->col.drawcol, 0);
	}
	else if(x >= _BAR_H + _BETWEEN + 1 && x < w - _BAR_H - _BETWEEN)
	{
		//グラデーション部分 (枠は含まない)

		p->fdrag = 1;
		p->last_gradx = -1;

		_on_motion(p, x);

		mWidgetGrabPointer(M_WIDGET(p));
	}
}

/** グラブ解除 */

static void _grab_release(GradBar *p)
{
	if(p->fdrag)
	{
		p->fdrag = 0;

		mWidgetUngrabPointer(M_WIDGET(p));
		mWidgetUpdate(M_WIDGET(p));
	}
}

/** イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	GradBar *p = (GradBar *)wg;

	switch(ev->type)
	{
		case MEVENT_POINTER:
			if(ev->pt.type == MEVENT_POINTER_TYPE_MOTION)
			{
				//移動

				if(p->fdrag)
					_on_motion(p, ev->pt.x);
			}
			else if(ev->pt.type == MEVENT_POINTER_TYPE_PRESS)
			{
				//押し
				
				if(ev->pt.btt == M_BTT_LEFT && !p->fdrag)
					_event_press(p, ev->pt.x);
			}
			else if(ev->pt.type == MEVENT_POINTER_TYPE_RELEASE)
			{
				//離し

				if(ev->pt.btt == M_BTT_LEFT)
					_grab_release(p);
			}
			break;

		case MEVENT_FOCUS:
			if(ev->focus.bOut)
				_grab_release(p);
			break;
	}

	return 1;
}

/** 描画 */

static void _draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	GradBar *p = (GradBar *)wg;
	int i,j,x,w,c[3],step,stepno;
	uint32_t col;
	uint8_t *pd,*pd_y;
	mBox box;

	w = wg->w;

	//背景色

	mPixbufFillBox(pixbuf, 0, 0, wg->w, _BAR_H, MSYSCOL(FACE));

	//端の色

	for(i = 0, x = 0; i < 2; i++)
	{
		mPixbufBox(pixbuf, x, 0, _BAR_H, _BAR_H, 0);
		mPixbufFillBox(pixbuf, x + 1, 1, _BAR_H - 2, _BAR_H - 2, mRGBtoPix(p->col[i]));

		x = w - _BAR_H;
	}

	//グラデーション

	w -= (_BAR_H + _BETWEEN) * 2;

	mPixbufBox(pixbuf, _BAR_H + _BETWEEN, 0, w, _BAR_H, 0);


	if(mPixbufGetClipBox_d(pixbuf, &box, _BAR_H + _BETWEEN + 1, 1, w - 2, _BAR_H - 2))
	{
		pd = mPixbufGetBufPtFast(pixbuf, box.x, box.y);

		x = box.x - (_BAR_H + _BETWEEN + 1);
		w -= 2;

		step = p->step;
		if(step < 3) w--;

		for(i = 0; i < box.w; i++, x++, pd += pixbuf->bpp)
		{
			if(step < 3)
			{
				//通常

				for(j = 0; j < 3; j++)
					c[j] = p->rgb_diff[j] * x / w + p->rgb[0][j];
			}
			else
			{
				//段階あり
				
				stepno = x * step / w;

				if(stepno == step - 1)
				{
					//終端
					
					for(j = 0; j < 3; j++)
						c[j] = p->rgb[1][j];
				}
				else
				{
					for(j = 0; j < 3; j++)
						c[j] = stepno * p->rgb_diff_step[j] / (step - 1) + p->rgb[0][j];
				}
			}

			col = mRGBtoPix2(c[0], c[1], c[2]);

			//Y を同じ色で埋める

			for(j = _BAR_H - 2, pd_y = pd; j; j--, pd_y += pixbuf->pitch_dir)
				(pixbuf->setbuf)(pd_y, col);
		}
	}

	//カーソル

	if(p->fdrag)
		mPixbufLineV(pixbuf, p->curx, 0, _BAR_H, MPIXBUF_COL_XOR);
}


//========================


/** 作成 */

mWidget *DockColorPalette_GradationBar_new(mWidget *parent,
	int id,uint32_t colL,uint32_t colR,int step)
{
	GradBar *p;

	p = (GradBar *)mWidgetNew(sizeof(GradBar), parent);

	p->col[0] = colL;
	p->col[1] = colR;
	p->step = step;

	p->wg.id = id;
	p->wg.fEventFilter |= MWIDGET_EVENTFILTER_POINTER;
	p->wg.fLayout = MLF_EXPAND_W;
	p->wg.hintH = _BAR_H;
	p->wg.hintW = 80;
	p->wg.draw = _draw_handle;
	p->wg.event = _event_handle;

	_set_rgb(p);

	return (mWidget *)p;
}

/** 段階数変更 */

void DockColorPalette_GradationBar_setStep(mWidget *wg,int step)
{
	((GradBar *)wg)->step = step;

	mWidgetUpdate(wg);
}
