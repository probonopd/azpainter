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
 * FilterPrev
 *
 * フィルタのダイアログ内プレビュー
 *****************************************/

#include "mDef.h"
#include "mWidgetDef.h"
#include "mWidget.h"
#include "mPixbuf.h"
#include "mEvent.h"
#include "mSysCol.h"
#include "mRectBox.h"

#include "TileImage.h"
#include "PixbufDraw.h"

#include "FilterPrev.h"


//-------------------------

struct _FilterPrev
{
	mWidget wg;

	mPixbuf *img;	//表示用イメージ

	TileImage *imgsrc;		//元イメージ (ドラッグでの範囲指定時用)
	int fullw,fullh;		//イメージの最大サイズ
	mBool enable_scroll,	//スクロールが有効か
		fpress;
	
	mBox boxview,		//表示している範囲 (イメージ座標)
		boxframe;		//ウィジェット表示範囲内における、イメージ全体の枠の範囲
	mSize sizeViewInFrame;	//枠内の、表示範囲部分のサイズ
};

//-------------------------


//========================
// sub
//========================


/** ドラッグ時、表示範囲位置セット */

static mBool _change_pos(FilterPrev *p,int x,int y)
{
	x = x - 1 - p->boxframe.x - p->sizeViewInFrame.w / 2;
	y = y - 1 - p->boxframe.y - p->sizeViewInFrame.h / 2;

	x = (int)((double)x / p->boxframe.w * p->fullw + 0.5);
	y = (int)((double)y / p->boxframe.h * p->fullh + 0.5);

	//調整

	if(x < 0)
		x = 0;
	else if(x > p->fullw - p->boxview.w)
		x = p->fullw - p->boxview.w;
	
	if(y < 0)
		y = 0;
	else if(y > p->fullh - p->boxview.h)
		y = p->fullh - p->boxview.h;

	//変更

	if(x == p->boxview.x && y == p->boxview.y)
		return FALSE;
	else
	{
		p->boxview.x = x;
		p->boxview.y = y;
		return TRUE;
	}
}

/** ドラッグ中イメージの描画 */

static void _drawimg_in_drag(FilterPrev *p)
{
	int x,y,n;

	//イメージ

	TileImage_drawFilterPreview(p->imgsrc, p->img, &p->boxview);

	//全体の枠

	mPixbufBox(p->img, p->boxframe.x + 1, p->boxframe.y + 1,
		p->boxframe.w, p->boxframe.h, mRGBtoPix(0xff0000));

	//表示範囲の枠

	x = (int)((double)p->boxview.x / p->fullw * p->boxframe.w + p->boxframe.x + 0.5);
	y = (int)((double)p->boxview.y / p->fullh * p->boxframe.h + p->boxframe.y + 0.5);

	n = p->boxframe.x + p->boxframe.w - p->sizeViewInFrame.w;
	if(x > n) x = n;

	n = p->boxframe.y + p->boxframe.h - p->sizeViewInFrame.h;
	if(y > n) y = n;

	pixbufDraw_dashBox_mono(p->img, x + 1, y + 1, p->sizeViewInFrame.w, p->sizeViewInFrame.h);
}


//========================
// イベント
//========================


/** グラブ解除 */

static void _release_grab(FilterPrev *p)
{
	if(p->fpress)
	{
		p->fpress = 0;
		mWidgetUngrabPointer(M_WIDGET(p));

		mWidgetAppendEvent_notify(NULL, M_WIDGET(p), 0, 0, 0);
	}
}

/** イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	FilterPrev *p = (FilterPrev *)wg;

	switch(ev->type)
	{
		case MEVENT_POINTER:
			if(ev->pt.type == MEVENT_POINTER_TYPE_MOTION)
			{
				//移動

				if(p->fpress == 1)
				{
					if(_change_pos(p, ev->pt.x, ev->pt.y))
						mWidgetUpdate(wg);
				}
			}
			else if(ev->pt.type == MEVENT_POINTER_TYPE_PRESS)
			{
				//押し

				if(ev->pt.btt == M_BTT_LEFT && !p->fpress && p->enable_scroll)
				{
					p->fpress = 1;
					mWidgetGrabPointer(M_WIDGET(p));

					_change_pos(p, ev->pt.x, ev->pt.y);

					mWidgetUpdate(wg);
				}
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


//====================


/** 描画 */

static void _draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	FilterPrev *p = (FilterPrev *)wg;

	if(p->fpress)
		_drawimg_in_drag(p);

	mPixbufBlt(pixbuf, 0, 0, p->img, 0, 0, -1, -1);
}

/** 破棄ハンドラ */

static void _destroy_handle(mWidget *wg)
{
	mPixbufFree(((FilterPrev *)wg)->img);
}

/** 作成 */

FilterPrev *FilterPrev_new(mWidget *parent,int id,int w,int h,
	TileImage *imgsrc,int fullw,int fullh)
{
	FilterPrev *p;

	//サイズ調整 (イメージの最大サイズ以下にする)

	if(w - 2 > fullw) w = fullw + 2;
	if(h - 2 > fullh) h = fullh + 2;

	//作成

	p = (FilterPrev *)mWidgetNew(sizeof(FilterPrev), parent);
	if(!p) return NULL;

	p->wg.id = id;
	p->wg.fEventFilter |= MWIDGET_EVENTFILTER_POINTER;
	p->wg.event = _event_handle;
	p->wg.draw = _draw_handle;
	p->wg.destroy = _destroy_handle;
	p->wg.hintW = w;
	p->wg.hintH = h;
	p->wg.margin.right = 12;

	p->imgsrc = imgsrc;
	p->fullw = fullw;
	p->fullh = fullh;

	//スクロールが有効か

	p->enable_scroll = (w - 2 < fullw || h - 2 < fullh);

	//表示範囲

	p->boxview.w = w - 2;
	p->boxview.h = h - 2;

	//イメージ全体の枠の範囲

	p->boxframe.w = fullw;
	p->boxframe.h = fullh;

	mBoxScaleKeepAspect(&p->boxframe, w - 2, h - 2, TRUE);

	//イメージ全体の枠における、表示範囲部分のサイズ

	p->sizeViewInFrame.w = (int)((double)p->boxview.w / fullw * p->boxframe.w + 0.5);
	p->sizeViewInFrame.h = (int)((double)p->boxview.h / fullh * p->boxframe.h + 0.5);

	//イメージ

	p->img = mPixbufCreate(w, h);
	if(!p->img)
	{
		mWidgetDestroy(M_WIDGET(p));
		return NULL;
	}

	mPixbufBox(p->img, 0, 0, w, h, 0);

	return p;
}

/** 描画対象のイメージ範囲取得 */

void FilterPrev_getDrawArea(FilterPrev *p,mRect *rc,mBox *box)
{
	if(p)
	{
		rc->x1 = p->boxview.x;
		rc->y1 = p->boxview.y;
		rc->x2 = rc->x1 + p->boxview.w - 1;
		rc->y2 = rc->y1 + p->boxview.h - 1;

		mBoxSetByRect(box, rc);
	}
}

/** イメージ描画 */

void FilterPrev_drawImage(FilterPrev *p,TileImage *img)
{
	if(p)
	{
		TileImage_drawFilterPreview(img, p->img, &p->boxview);

		mWidgetUpdate(M_WIDGET(p));
	}
}
