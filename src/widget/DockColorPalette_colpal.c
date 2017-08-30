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
 * カラーパレット
 *****************************************/

#include "mDef.h"
#include "mContainerDef.h"
#include "mWidget.h"
#include "mContainer.h"
#include "mScrollBar.h"
#include "mSysCol.h"
#include "mPixbuf.h"
#include "mEvent.h"
#include "mRectBox.h"
#include "mMenu.h"
#include "mTrans.h"

#include "defDraw.h"
#include "ColorPalette.h"

#include "trgroup.h"


//---------------------

typedef struct
{
	mWidget wg;

	mScrollBar *scr;
	
	int fpress,
		xnum,		//横の個数
		press_no;		//押し時のパレット番号
	mPoint press_pt,	//押し時のパレット位置
		grad_pt,		//グラデーション時の始点
		last_pt;		//前回の位置
}ColPalArea;

typedef struct
{
	mWidget wg;
	mContainerData ct;

	ColPalArea *area;
	mScrollBar *scr;
}ColPal;

//---------------------

#define _COL_BKGND  0x999999
#define _COL_PALSEL 0xff0000

#define _CELLW      (APP_DRAW->col.colpal_cellw)
#define _CELLH      (APP_DRAW->col.colpal_cellh)
#define _PAL_SELDAT (APP_COLPAL->pal + APP_DRAW->col.colpal_sel)
#define _PALNUM     (APP_COLPAL->pal[APP_DRAW->col.colpal_sel].num)

enum
{
	_PRESS_SPOIT = 1,
	_PRESS_SETCOL,
	_PRESS_GRADATION
};

enum
{
	TRID_MENU_SETCOL = 1000,
	TRID_MENU_GETCOL
};

//---------------------

void DockColorPalette_tabPalette_dnd_load(mWidget *area,const char *filename);

//---------------------



/************************************
 * ColPalArea (領域部分)
 ************************************/


//========================
// sub
//========================


/** 横表示数セット */

static void _area_set_column(ColPalArea *p)
{
	int n,max;

	max = APP_DRAW->col.colpal_max_column;

	n = p->wg.w / _CELLW;

	if(max && n > max)
		n = max;
	
	if(n < 1) n = 1;

	p->xnum = n;
}

/** スクロール情報セット */

static void _area_set_scroll(ColPalArea *p)
{
	int page;

	page = p->wg.h / _CELLH;
	if(page <= 0) page = 1;

	mScrollBarSetStatus(p->scr, 0, (_PALNUM + p->xnum - 1) / p->xnum, page);
}

/** カーソル位置からパレット番号取得
 *
 * @param  pt  NULL 以外で X Y 位置がセットされる */

static int _get_palno_at_pt(ColPalArea *p,int x,int y,mPoint *pt)
{
	int no;

	if(x < 0 || y < 0) return -1;

	x = x / _CELLW;
	y = y / _CELLH + p->scr->sb.pos;

	if(x >= p->xnum) return -1;

	no = x + y * p->xnum;

	if(no >= _PALNUM) return -1;

	if(pt)
	{
		pt->x = x * _CELLW;
		pt->y = (y - p->scr->sb.pos) * _CELLH;
	}

	return no;
}


//========================
// 直接描画
//========================


/** カーソルを直接描画 */

static void _draw_cursor(ColPalArea *p,mPoint *pt,mBool erase)
{
	mPixbuf *pixbuf;
	mBox box;

	pixbuf = mWidgetBeginDirectDraw(M_WIDGET(p));
	if(pixbuf)
	{
		box.x = pt->x;
		box.y = pt->y;
		box.w = _CELLW + 1;
		box.h = _CELLH + 1;
	
		mPixbufBox(pixbuf, box.x, box.y, box.w, box.h,
			mRGBtoPix(erase? _COL_BKGND: _COL_PALSEL));
	
		mWidgetEndDirectDraw(M_WIDGET(p), pixbuf);

		mWidgetUpdateBox_box(M_WIDGET(p), &box);
	}
}

/** 色とカーソルを直接描画 */

static void _draw_color_and_cursor(ColPalArea *p,mPoint *pt,uint32_t col)
{
	mPixbuf *pixbuf;
	mBox box;

	pixbuf = mWidgetBeginDirectDraw(M_WIDGET(p));
	if(pixbuf)
	{
		box.x = pt->x;
		box.y = pt->y;
		box.w = _CELLW + 1;
		box.h = _CELLH + 1;
	
		mPixbufBox(pixbuf, box.x, box.y, box.w, box.h, mRGBtoPix(_COL_PALSEL));

		mPixbufFillBox(pixbuf, box.x + 1, box.y + 1, box.w - 2, box.h - 2,
			mRGBtoPix(col));
	
		mWidgetEndDirectDraw(M_WIDGET(p), pixbuf);

		mWidgetUpdateBox_box(M_WIDGET(p), &box);
	}
}

/** グラデーション線描画
 *
 * @param pt2  NULL で開始時 */

static void _draw_grad_line(ColPalArea *p,mPoint *pt1,mPoint *pt2)
{
	mPixbuf *pixbuf;
	mBox box,box2;

	pixbuf = mWidgetBeginDirectDraw(M_WIDGET(p));
	if(pixbuf)
	{
		if(!pt2)
		{
			mPixbufSetPixel(pixbuf, pt1->x, pt1->y, MPIXBUF_COL_XOR);

			box.x = pt1->x, box.y = pt1->y;
			box.w = box.h = 1;
		}
		else
		{
			mPixbufLine(pixbuf, pt1->x, pt1->y, p->last_pt.x, p->last_pt.y, MPIXBUF_COL_XOR);
			mPixbufLine(pixbuf, pt1->x, pt1->y, pt2->x, pt2->y, MPIXBUF_COL_XOR);

			mBoxSetByPoint(&box, pt1, &p->last_pt);
			mBoxSetByPoint(&box2, pt1, pt2);
			mBoxUnion(&box, &box2);
		}
	
		mWidgetEndDirectDraw(M_WIDGET(p), pixbuf);

		mWidgetUpdateBox_box(M_WIDGET(p), &box);
	}
}


//========================
// イベント
//========================


/** メニュー */

static void _run_menu(ColPalArea *p,int palno,int rx,int ry,mPoint *pt)
{
	mMenu *menu;
	mMenuItemInfo *mi;
	int id;
	uint16_t dat[] = { TRID_MENU_SETCOL, TRID_MENU_GETCOL, 0xffff };

	//カーソル

	_draw_cursor(p, pt, FALSE);

	//メニュー

	M_TR_G(TRGROUP_DOCK_COLOR_PALETTE);

	menu = mMenuNew();

	mMenuAddTrArray16(menu, dat);

	mi = mMenuPopup(menu, NULL, rx, ry, 0);
	id = (mi)? mi->id: -1;

	mMenuDestroy(menu);

	//カーソル消去

	_draw_cursor(p, pt, TRUE);

	//コマンド

	if(id == -1) return;

	switch(id)
	{
		//色をセット
		case TRID_MENU_SETCOL:
			ColorPaletteDat_setColor(_PAL_SELDAT, palno, APP_DRAW->col.drawcol);

			mWidgetUpdate(M_WIDGET(p));
			break;
		//色を取得
		case TRID_MENU_GETCOL:
			mWidgetAppendEvent_notify(NULL, p->wg.parent, 0,
				ColorPaletteDat_getColor(_PAL_SELDAT, palno), 0);
			break;
	}
}

/** 押し時 */

static void _event_press(ColPalArea *p,mEvent *ev)
{
	int no,type = -1;
	mPoint pt;

	//パレット位置

	no = _get_palno_at_pt(p, ev->pt.x, ev->pt.y, &pt);
	if(no == -1) return;

	//ボタン

	if(ev->pt.btt == M_BTT_RIGHT)
		//右ボタン : メニュー
		_run_menu(p, no, ev->pt.rootx, ev->pt.rooty, &pt);
	else if(ev->pt.btt == M_BTT_LEFT)
	{
		//------ 左ボタン

		if(ev->pt.state & M_MODS_SHIFT)
		{
			//+Shift : 色登録

			ColorPaletteDat_setColor(_PAL_SELDAT, no, APP_DRAW->col.drawcol);

			type = _PRESS_SETCOL;

			_draw_color_and_cursor(p, &pt, APP_DRAW->col.drawcol);
		}
		else if(ev->pt.state & M_MODS_CTRL)
		{
			//+Ctrl : グラデーション

			type = _PRESS_GRADATION;

			p->grad_pt.x = ev->pt.x;
			p->grad_pt.y = ev->pt.y;
			p->last_pt = p->grad_pt;

			_draw_grad_line(p, &p->grad_pt, NULL);
		}
		else
		{
			//色取得

			mWidgetAppendEvent_notify(NULL, p->wg.parent, 0,
				ColorPaletteDat_getColor(_PAL_SELDAT, no), 0);

			type = _PRESS_SPOIT;

			_draw_cursor(p, &pt, FALSE);
		}
	}

	//グラブ

	if(type != -1)
	{
		p->fpress = type;
		p->press_no = no;
		p->press_pt = pt;

		mWidgetGrabPointer(M_WIDGET(p));
	}
}

/** グラブ解除
 *
 * @param ev  NULL でフォーカスアウト時 */

static void _grab_release(ColPalArea *p,mEvent *ev)
{
	int no;

	if(p->fpress)
	{
		switch(p->fpress)
		{
			case _PRESS_SPOIT:
			case _PRESS_SETCOL:
				_draw_cursor(p, &p->press_pt, TRUE);
				break;
			//グラデーション
			case _PRESS_GRADATION:
				if(ev)
				{
					no = _get_palno_at_pt(p, ev->pt.x, ev->pt.y, NULL);
					if(no != -1)
						ColorPaletteDat_gradation(_PAL_SELDAT, p->press_no, no);
				}
				
				mWidgetUpdate(M_WIDGET(p));
				break;
		}
	
		p->fpress = 0;
		mWidgetUngrabPointer(M_WIDGET(p));
	}
}

/** イベント */

static int _area_event_handle(mWidget *wg,mEvent *ev)
{
	ColPalArea *p = (ColPalArea *)wg;

	switch(ev->type)
	{
		case MEVENT_POINTER:
			if(ev->pt.type == MEVENT_POINTER_TYPE_MOTION)
			{
				//移動

				if(p->fpress == _PRESS_GRADATION)
				{
					//グラデーション

					mPoint pt;

					pt.x = ev->pt.x;
					pt.y = ev->pt.y;

					_draw_grad_line(p, &p->grad_pt, &pt);

					p->last_pt = pt;
				}
			}
			else if(ev->pt.type == MEVENT_POINTER_TYPE_PRESS)
			{
				//押し
				
				if(!p->fpress)
					_event_press(p, ev);
			}
			else if(ev->pt.type == MEVENT_POINTER_TYPE_RELEASE)
			{
				//離し

				if(ev->pt.btt == M_BTT_LEFT)
					_grab_release(p, ev);
			}
			break;

		case MEVENT_FOCUS:
			if(ev->focus.bOut)
				_grab_release(p, NULL);
			break;
	}

	return 1;
}


//========================


/** D&D (パレットファイル読み込み) */

static int _area_on_dnd(mWidget *wg,char **files)
{
	DockColorPalette_tabPalette_dnd_load(wg, *files);

	return 1;
}

/** サイズ変更時 */

static void _area_on_size(mWidget *wg)
{
	ColPalArea *p = (ColPalArea *)wg;

	_area_set_column(p);
	_area_set_scroll(p);
}

/** 描画 */

static void _area_draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	ColPalArea *p = (ColPalArea *)wg;
	ColorPaletteDat *paldat;
	uint8_t *buf;
	int no,num,ix,x,y,h,cellw,cellh;

	h = wg->h;

	//背景

	mPixbufFillBox(pixbuf, 0, 0, wg->w, h, mRGBtoPix(_COL_BKGND));

	//パレット

	paldat = _PAL_SELDAT;

	no = p->scr->sb.pos * p->xnum;
	buf = paldat->buf + no * 3;
	num = paldat->num;
	cellw = _CELLW;
	cellh = _CELLH;

	for(y = 1; y < h; y += cellh)
	{
		for(ix = p->xnum, x = 1; ix > 0; ix--, x += cellw, buf += 3, no++)
		{
			if(no >= num) return;

			mPixbufFillBox(pixbuf, x, y, cellw - 1, cellh - 1, mRGBtoPix2(buf[0], buf[1], buf[2]));
		}
	}
}

/** 作成 */

static ColPalArea *_create_area(mWidget *parent)
{
	ColPalArea *p;

	p = (ColPalArea *)mWidgetNew(sizeof(ColPalArea), parent);

	p->wg.fLayout = MLF_EXPAND_WH;
	p->wg.fEventFilter |= MWIDGET_EVENTFILTER_POINTER;
	p->wg.fState |= MWIDGET_STATE_ENABLE_DROP;
	p->wg.onSize = _area_on_size;
	p->wg.draw = _area_draw_handle;
	p->wg.event = _area_event_handle;
	p->wg.onDND = _area_on_dnd;

	return p;
}


/************************************
 * ColPal (コンテナ)
 ************************************/


/** スクロールバー ハンドラ */

static void _scrollbar_handle(mScrollBar *bar,int pos,int flags)
{
	if(flags & MSCROLLBAR_N_HANDLE_F_CHANGE)
		mWidgetUpdate((mWidget *)bar->wg.param);
}

/** 作成 */

mWidget *DockColorPalette_ColorPalette_new(mWidget *parent,int id)
{
	ColPal *p;
	mScrollBar *sb;

	p = (ColPal *)mContainerNew(sizeof(ColPal), parent);

	mContainerSetType(M_CONTAINER(p), MCONTAINER_TYPE_HORZ, 0);

	p->wg.id = id;
	p->wg.fLayout = MLF_EXPAND_WH;
	p->wg.draw = NULL;

	//ウィジェット

	p->area = _create_area(M_WIDGET(p));

	sb = p->scr = mScrollBarNew(0, M_WIDGET(p), MSCROLLBAR_S_VERT);
	sb->wg.fLayout = MLF_EXPAND_H;
	sb->wg.param = (intptr_t)p->area;
	sb->sb.handle = _scrollbar_handle;

	p->area->scr = sb;
	
	return (mWidget *)p;
}

/** 更新 */

void DockColorPalette_ColorPalette_update(mWidget *wg)
{
	ColPalArea *p = ((ColPal *)wg)->area;

	_area_set_column(p);
	_area_set_scroll(p);

	mWidgetUpdate(M_WIDGET(p));
}
