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
 * DockColor で使うウィジェット
 *
 * [描画色/背景色] [色マスク]
 *****************************************/

#include "mDef.h"
#include "mWidgetDef.h"
#include "mWidget.h"
#include "mContainer.h"
#include "mEvent.h"
#include "mPixbuf.h"
#include "mSysCol.h"
#include "mMenu.h"
#include "mTrans.h"

#include "defDraw.h"
#include "draw_main.h"

#include "trgroup.h"


//--------------------

//"MASK" 23x7 1bit
static const uint8_t g_img_mask[]={
0x11,0xe1,0x44,0x9b,0x12,0x25,0x55,0x14,0x14,0x55,0xe4,0x0c,
0xd1,0x07,0x15,0x51,0x14,0x25,0x51,0xe4,0x44 };

//"REV" 17x7 1bit
static const unsigned char g_img_rev[]={
0xcf,0x17,0x01,0x51,0x10,0x01,0x51,0x10,0x01,0xcf,0xa3,0x00,
0x45,0xa0,0x00,0x49,0x40,0x00,0xd1,0x47,0x00 };

//--------------------



/**************************************
 * 描画色/背景色
 **************************************/


#define DRAWCOL_SPACE  10


/** 描画 */

static void _drawcol_draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	int w,h;

	w = wg->w, h = wg->h;

	//背景

	mPixbufFillBox(pixbuf, 0, 0, w, h, MSYSCOL(FACE));

	//背景色

	mPixbufBox(pixbuf, DRAWCOL_SPACE, DRAWCOL_SPACE,
		w - DRAWCOL_SPACE, h - DRAWCOL_SPACE, 0);

	mPixbufFillBox(pixbuf, DRAWCOL_SPACE + 1, DRAWCOL_SPACE + 1,
		w - DRAWCOL_SPACE - 2, h - DRAWCOL_SPACE - 2,
		mRGBtoPix(APP_DRAW->col.bkgndcol));

	//描画色

	mPixbufBox(pixbuf, 0, 0, w - DRAWCOL_SPACE, h - DRAWCOL_SPACE, 0);

	mPixbufFillBox(pixbuf, 1, 1, w - DRAWCOL_SPACE - 2, h - DRAWCOL_SPACE - 2,
		mRGBtoPix(APP_DRAW->col.drawcol));
}

/** イベント */

static int _drawcol_event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_POINTER
		&& (ev->pt.type == MEVENT_POINTER_TYPE_PRESS || ev->pt.type == MEVENT_POINTER_TYPE_DBLCLK)
		&& ev->pt.btt == M_BTT_LEFT)
	{
		//左ボタンで色を入れ替え

		drawColor_toggleDrawCol(APP_DRAW);

		mWidgetUpdate(wg);

		mWidgetAppendEvent_notify(NULL, wg, 0, 0, 0);
	}

	return 1;
}

/** 作成 */

mWidget *DockColorDrawCol_new(mWidget *parent,int id)
{
	mWidget *p;

	p = mWidgetNew(0, parent);
	if(!p) return NULL;

	p->id = id;
	p->fEventFilter |= MWIDGET_EVENTFILTER_POINTER;
	p->hintW = p->hintH = 38;
	p->draw = _drawcol_draw_handle;
	p->event = _drawcol_event_handle;

	return p;
}



/**************************************
 * 色マスク
 **************************************/


#define _COLMASK_HEIGHT        37
#define _COLMASK_TYPEBTT_W     35
#define _COLMASK_EDITBTT_SIZE  19
#define _COLMASK_BTT_H         19

enum
{
	_COLMASK_AREA_MASK,
	_COLMASK_AREA_REV,
	_COLMASK_AREA_ADD,
	_COLMASK_AREA_DEL,
	_COLMASK_AREA_COLOR_TOP
};

enum
{
	TRID_COLMASK_SET = 100,
	TRID_COLMASK_ADD,
	TRID_COLMASK_CLEAR,
	TRID_COLMASK_DEL,
	TRID_COLMASK_SPOIT
};


//=======================
// sub
//=======================


/** 色マスクの色を描画色に */

static void _colmask_spoit(mWidget *wg,int area)
{
	drawColor_setDrawColor(APP_DRAW->col.colmask_col[area - _COLMASK_AREA_COLOR_TOP]);

	mWidgetAppendEvent_notify(NULL, wg, 0, 0, 0);
}

/** 色の各幅と終端幅を取得 */

static void _colmask_get_eachw(int w,int *eachw,int *lastw)
{
	int num = APP_DRAW->col.colmask_num;

	w -= _COLMASK_TYPEBTT_W + _COLMASK_EDITBTT_SIZE + 1;

	*eachw = w / num;
	if(lastw) *lastw = w - (*eachw * (num - 1));
}

/** カーソル位置からエリアを取得 */

static int _colmask_get_area_form_pt(mWidget *wg,int x,int y)
{
	int ret,eachw;

	if(x < _COLMASK_TYPEBTT_W)
		//MASK/REV
		ret = (y < _COLMASK_BTT_H)? _COLMASK_AREA_MASK: _COLMASK_AREA_REV;
	else if(x >= wg->w - _COLMASK_EDITBTT_SIZE)
		//+/-
		ret = (y < _COLMASK_BTT_H)? _COLMASK_AREA_ADD: _COLMASK_AREA_DEL;
	else
	{
		//色

		_colmask_get_eachw(wg->w, &eachw, NULL);

		ret = (x - _COLMASK_TYPEBTT_W) / eachw;
		
		if(ret >= APP_DRAW->col.colmask_num)
			ret = APP_DRAW->col.colmask_num - 1;

		ret += _COLMASK_AREA_COLOR_TOP;
	}

	return ret;
}


/** メニュー */

static void _colmask_run_menu(mWidget *wg,int area,int rx,int ry)
{
	mMenu *menu;
	mMenuItemInfo *mi;
	int id,num,update = TRUE;
	uint16_t dat[] = {
		TRID_COLMASK_SET, TRID_COLMASK_ADD, 0xfffe,
		TRID_COLMASK_CLEAR, 0xfffe,
		TRID_COLMASK_DEL, TRID_COLMASK_SPOIT, 0xffff
	};

	num = APP_DRAW->col.colmask_num;

	//メニュー

	M_TR_G(TRGROUP_DOCK_COLOR);

	menu = mMenuNew();

	mMenuAddTrArray16(menu, dat);

	if(num == COLORMASK_MAXNUM)
		mMenuSetEnable(menu, TRID_COLMASK_ADD, 0);

	if(num == 1)
	{
		mMenuSetEnable(menu, TRID_COLMASK_CLEAR, 0);
		mMenuSetEnable(menu, TRID_COLMASK_DEL, 0);
	}

	if(area < _COLMASK_AREA_COLOR_TOP)
	{
		mMenuSetEnable(menu, TRID_COLMASK_DEL, 0);
		mMenuSetEnable(menu, TRID_COLMASK_SPOIT, 0);
	}

	mi = mMenuPopup(menu, NULL, rx, ry, 0);
	id = (mi)? mi->id: -1;

	mMenuDestroy(menu);

	if(id == -1) return;

	//コマンド

	switch(id)
	{
		//描画色をセット
		case TRID_COLMASK_SET:
			drawColorMask_setColor(APP_DRAW, -1);
			break;
		//描画色を追加
		case TRID_COLMASK_ADD:
			update = drawColorMask_addColor(APP_DRAW, -1);
			break;
		//クリア
		case TRID_COLMASK_CLEAR:
			drawColorMask_clear(APP_DRAW);
			break;
		//選択色を削除
		case TRID_COLMASK_DEL:
			drawColorMask_delColor(APP_DRAW, area - _COLMASK_AREA_COLOR_TOP);
			break;
		//選択色をスポイト
		case TRID_COLMASK_SPOIT:
			_colmask_spoit(wg, area);
			update = FALSE;
			break;
	}

	if(update) mWidgetUpdate(wg);
}


//=======================
// ハンドラ
//=======================


/** タイプボタン描画 */

static void _colmask_draw_typebtt(mPixbuf *pixbuf,int type,mBool press)
{
	const uint8_t *pat;
	int y,patw;

	if(type)
		y = _COLMASK_BTT_H, pat = g_img_rev, patw = 17;
	else
		y = 1, pat = g_img_mask, patw = 23;

	if(press)
	{
		//押し時

		mPixbufDraw3DFrame(pixbuf, 1, y, _COLMASK_TYPEBTT_W - 2, _COLMASK_BTT_H - 2,
			mGraytoPix(132), MSYSCOL(WHITE));

		mPixbufFillBox(pixbuf, 2, y + 1, _COLMASK_TYPEBTT_W - 4, _COLMASK_BTT_H - 4,
			mRGBtoPix(0x83aef4));
	}
	else
	{
		//通常時

		mPixbufDraw3DFrame(pixbuf, 1, y, _COLMASK_TYPEBTT_W - 2, _COLMASK_BTT_H - 2,
			MSYSCOL(WHITE), mGraytoPix(132));

		mPixbufFillBox(pixbuf, 2, y + 1, _COLMASK_TYPEBTT_W - 4, _COLMASK_BTT_H - 4,
			mGraytoPix(230));
	}

	//文字

	mPixbufDrawBitPattern(pixbuf, (_COLMASK_TYPEBTT_W - patw) / 2 + press , y + 5 + press,
		pat, patw, 7, (press)? MSYSCOL(WHITE): 0);
}

/** 描画 */

static void _colmask_draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	int w,x,i,eachw,lastw,colw;

	w = wg->w;

	//背景

	mPixbufFillBox(pixbuf, 0, 0, w, _COLMASK_HEIGHT, MSYSCOL(WHITE));

	//----- タイプボタン

	x = APP_DRAW->col.colmask_type;

	//枠

	mPixbufBox(pixbuf, 0, 0, _COLMASK_TYPEBTT_W, _COLMASK_HEIGHT, 0);
	mPixbufLineH(pixbuf, 0, _COLMASK_BTT_H - 1, _COLMASK_TYPEBTT_W, 0);

	//MASK/REV

	_colmask_draw_typebtt(pixbuf, 0, (x == 1));
	_colmask_draw_typebtt(pixbuf, 1, (x == 2));

	//------ マスク色

	_colmask_get_eachw(w, &eachw, &lastw);

	//色

	x = _COLMASK_TYPEBTT_W;

	for(i = 0; i < APP_DRAW->col.colmask_num; i++, x += eachw)
	{
		colw = (i == APP_DRAW->col.colmask_num - 1)? lastw: eachw;
	
		//内枠

		mPixbufBox(pixbuf, x + 1, 1, colw + 1 - 2, _COLMASK_HEIGHT - 2, 0);

		//色

		mPixbufFillBox(pixbuf, x + 2, 2, colw + 1 - 4, _COLMASK_HEIGHT - 4,
			mRGBtoPix(APP_DRAW->col.colmask_col[i]));
	}

	//------ + - ボタン

	x = w - _COLMASK_EDITBTT_SIZE;

	mPixbufBox(pixbuf, x, 0, _COLMASK_EDITBTT_SIZE, _COLMASK_HEIGHT, 0);
	mPixbufLineH(pixbuf, x, _COLMASK_BTT_H - 1, _COLMASK_EDITBTT_SIZE, 0);

	//+

	mPixbufFillBox(pixbuf, x + 4, 8, 11, 3, 0);
	mPixbufFillBox(pixbuf, x + 8, 4, 3, 11, 0);

	//-

	mPixbufFillBox(pixbuf, x + 4, 18 + 8, 11, 3, 0);
}

/** 通常左クリック時 */

static void _colmask_press_left(mWidget *wg,int area)
{
	mBool update = TRUE;

	switch(area)
	{
		//マスク/逆マスク
		case _COLMASK_AREA_MASK:
		case _COLMASK_AREA_REV:
			drawColorMask_setType(APP_DRAW, (area == _COLMASK_AREA_MASK)? -1: -2);
			break;
		//描画色追加
		case _COLMASK_AREA_ADD:
			update = drawColorMask_addColor(APP_DRAW, -1);
			break;
		//最後の色を削除
		case _COLMASK_AREA_DEL:
			update = drawColorMask_delColor(APP_DRAW, -1);
			break;
		//色エリア : 描画色セット
		default:
			drawColorMask_setColor(APP_DRAW, -1);
			break;
	}

	if(update) mWidgetUpdate(wg);
}

/** イベント */

static int _colmask_event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_POINTER
		&& (ev->pt.type == MEVENT_POINTER_TYPE_PRESS || ev->pt.type == MEVENT_POINTER_TYPE_DBLCLK))
	{
		int area;

		area = _colmask_get_area_form_pt(wg, ev->pt.x, ev->pt.y);
	
		if(ev->pt.btt == M_BTT_RIGHT)
			//右ボタン:メニュー
			_colmask_run_menu(wg, area, ev->pt.rootx, ev->pt.rooty);
		else if(ev->pt.btt == M_BTT_LEFT)
		{
			//左ボタン

			if((ev->pt.state & M_MODS_CTRL) && area >= _COLMASK_AREA_COLOR_TOP)
			{
				//Ctrl+左 : 選択色削除

				if(drawColorMask_delColor(APP_DRAW, area - _COLMASK_AREA_COLOR_TOP))
					mWidgetUpdate(wg);
			}
			else if((ev->pt.state & M_MODS_SHIFT) && area >= _COLMASK_AREA_COLOR_TOP)
				//Shift+左 : 選択色スポイト
				_colmask_spoit(wg, area);
			else
				_colmask_press_left(wg, area);
		}
	}
	
	return 1;
}

/** 色マスク 作成 */

mWidget *DockColorColorMask_new(mWidget *parent,int id)
{
	mWidget *p;

	p = mWidgetNew(0, parent);
	if(!p) return NULL;

	p->id = id;
	p->fEventFilter |= MWIDGET_EVENTFILTER_POINTER;
	p->fLayout = MLF_EXPAND_W;
	p->hintW = 120;
	p->hintH = _COLMASK_HEIGHT;
	p->draw = _colmask_draw_handle;
	p->event = _colmask_event_handle;

	return p;
}
