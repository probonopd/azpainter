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
 * DockBrushSize
 *
 * ブラシのサイズリスト
 *****************************************/
/*
 * - サイズが選択されると、通知イベント発生。
 *   通知元は DockBrushSize で、param1 にサイズ値。
 */

#include "mDef.h"
#include "mWidget.h"
#include "mContainerDef.h"
#include "mContainer.h"
#include "mScrollBar.h"
#include "mEvent.h"
#include "mPixbuf.h"
#include "mUtilStr.h"
#include "mSysCol.h"
#include "mMenu.h"
#include "mTrans.h"
#include "mSysDialog.h"
#include "mStr.h"

#include "defConfig.h"

#include "BrushSizeList.h"

#include "trgroup.h"


//---------------------

typedef struct
{
	mWidget wg;
	mScrollBar *scr;

	int fbtt,
		hnum,		//水平方向の表示個数
		press_no;	//押し時のデータ位置
}DockBrushSizeArea;

//---------------------

typedef struct _DockBrushSize
{
	mWidget wg;
	mContainerData ct;

	DockBrushSizeArea *area;
	mScrollBar *scr;
}DockBrushSize;

//---------------------

#define _AREA(p)   ((DockBrushSizeArea *)(p))

#define EACHSIZE   27			//1個のサイズ

#define COL_SELBOX 0xff0000
#define COL_NUMBER 0x0000ff
#define COL_FRAME  0x999999

#define _BTTF_NORMAL 1
#define _BTTF_DELETE 2

enum
{
	TRID_MENU_ADD,
	TRID_MENU_DEL,
	TRID_ADDDLG_TITLE = 100,
	TRID_ADDDLG_MESSAGE
};

//---------------------



//**********************************
// DockBrushSizeArea
//**********************************


//=============================
// sub
//=============================


/** スクロール情報セット */

static void _area_set_scrollinfo(DockBrushSizeArea *p)
{
	int max,page;

	max = BrushSizeList_getNum();

	max = (max + p->hnum - 1) / p->hnum;

	page = p->wg.h / EACHSIZE;
	if(page <= 0) page = 1;

	mScrollBarSetStatus(p->scr, 0, max, page);
}

/** 更新 */

static void _area_update(DockBrushSizeArea *p)
{
	_area_set_scrollinfo(p);
	mWidgetUpdate(M_WIDGET(p));
}

/** ポインタ位置からデータインデックス取得
 *
 * @return -1 で範囲外 */

static int _area_getindex_atpos(DockBrushSizeArea *p,int x,int y)
{
	int no,num;

	num = BrushSizeList_getNum();

	x /= EACHSIZE;
	y /= EACHSIZE;

	if(x >= p->hnum) return -1;

	no = (y + (p->scr)->sb.pos) * p->hnum + x;

	return (no < num)? no: -1;
}


//=============================
// 描画
//=============================


/** 選択枠描画 (直接描画) */

static void _draw_selbox(DockBrushSizeArea *p,mBool erase)
{
	mPixbuf *pixbuf;
	int x,y;

	if(p->press_no == -1) return;

	x = (p->press_no % p->hnum) * EACHSIZE;
	y = (p->press_no / p->hnum - (p->scr)->sb.pos) * EACHSIZE; 

	//描画

	pixbuf = mWidgetBeginDirectDraw(M_WIDGET(p));
	if(pixbuf)
	{
		mPixbufBox(pixbuf, x, y, EACHSIZE + 1, EACHSIZE + 1,
			mRGBtoPix(erase? COL_FRAME: COL_SELBOX));

		mWidgetEndDirectDraw(M_WIDGET(p), pixbuf);

		mWidgetUpdateBox_d(M_WIDGET(p), x, y, EACHSIZE + 1, EACHSIZE + 1);
	}
}

/** ブラシプレビュー描画
 *
 * @param radius 1.0 = 10 */

static void _draw_brushprev(mPixbuf *pixbuf,
	int x,int y,int w,int h,int radius)
{
	uint8_t *pd;
	mBox box;
	int bpp,pitch,ix,iy,jx,jy,xtbl[4],ytbl[4],f,fcx,fcy,frr,val;

	if(!mPixbufGetClipBox_d(pixbuf, &box, x, y, w, h)) return;

	pd = mPixbufGetBufPtFast(pixbuf, box.x, box.y);
	bpp = pixbuf->bpp;
	pitch = pixbuf->pitch_dir - bpp * box.w;

	/* 位置は2bit固定小数点数で計算 */

	fcx = w << 1; //w * 4 / 2
	fcy = h << 1;

	frr = (radius << 2) / 10;
	frr *= frr;

	//box.x, box.y を相対位置に

	box.x -= x;
	box.y -= y;

	//4x4 オーバーサンプリング

	for(iy = 0; iy < box.h; iy++, pd += pitch)
	{
		//y テーブル

		f = ((box.y + iy) << 2) - fcy;

		for(jy = 0; jy < 4; jy++, f++)
			ytbl[jy] = f * f;
	
		//
	
		for(ix = 0; ix < box.w; ix++, pd += bpp)
		{
			//x テーブル

			f = ((box.x + ix) << 2) - fcx;

			for(jx = 0; jx < 4; jx++, f++)
				xtbl[jx] = f * f;

			//4x4

			val = 0;

			for(jy = 0; jy < 4; jy++)
			{
				for(jx = 0; jx < 4; jx++)
				{
					if(xtbl[jx] + ytbl[jy] < frr)
						val += 255;
				}
			}

			val >>= 4;

			//文字にかかる部分は色を薄くする

			if(box.y + iy >= EACHSIZE - 11) val >>= 3;

			//セット

			(pixbuf->setbuf)(pd, mGraytoPix(255 - val));
		}
	}
}

/** 描画 */

static void _area_draw(mWidget *wg,mPixbuf *pixbuf)
{
	DockBrushSizeArea *p = _AREA(wg);
	uint16_t *pbuf;
	int num,pos,ix,iy,xx,size;
	char m[8];
	mPixCol col_frame,col_num;

	//背景

	mPixbufFillBox(pixbuf, 0, 0, wg->w, wg->h, MSYSCOL(FACE_DARKER));

	//

	col_frame = mRGBtoPix(COL_FRAME);
	col_num = mRGBtoPix(COL_NUMBER);

	pos = (p->scr)->sb.pos * p->hnum;
	num = BrushSizeList_getNum();
	pbuf = BrushSizeList_getBuf() + pos;

	for(iy = 0; iy < wg->h; iy += EACHSIZE)
	{
		for(ix = 0, xx = 0; ix < p->hnum; ix++, pos++, pbuf++, xx += EACHSIZE)
		{
			if(pos >= num) return;

			size = *pbuf;

			mPixbufBox(pixbuf, xx, iy, EACHSIZE + 1, EACHSIZE + 1, col_frame);
			mPixbufBox(pixbuf, xx + 1, iy + 1, EACHSIZE - 1, EACHSIZE - 1, MSYSCOL(WHITE));

			_draw_brushprev(pixbuf,
				xx + 2, iy + 2, EACHSIZE - 3, EACHSIZE - 3, (size > 120)? 120: size);

			//数値

			mFloatIntToStr(m, size, 1);

			mPixbufDrawNumber_5x7(pixbuf, xx + 2, iy + EACHSIZE - 8, m, col_num);
		}
	}
}


//=============================
// コマンド
//=============================


/** 追加 */

static void _cmd_add(DockBrushSizeArea *p)
{
	mStr str = MSTR_INIT;

	/* メニューの直後なので、翻訳グループはそのまま */

	if(mSysDlgInputText(p->wg.toplevel,
		M_TR_T(TRID_ADDDLG_TITLE), M_TR_T(TRID_ADDDLG_MESSAGE),
		&str, MSYSDLG_INPUTTEXT_F_NOEMPTY))
	{
		BrushSizeList_addFromText(str.buf);
		
		mStrFree(&str);

		_area_update(p);
	}
}

/** 削除 */

static void _cmd_del(DockBrushSizeArea *p,int no)
{
	BrushSizeList_deleteAtPos(no);
	_area_update(p);
}

/** メニュー実行 */

static void _run_menu(DockBrushSizeArea *p,int x,int y,int no)
{
	mMenu *menu;
	mMenuItemInfo *mi;
	int id;

	p->press_no = no;

	_draw_selbox(p, FALSE);

	//メニュー

	M_TR_G(TRGROUP_DOCK_BRUSH_SIZE);

	menu = mMenuNew();

	mMenuAddTrTexts(menu, TRID_MENU_ADD, 2);

	if(no == -1) mMenuSetEnable(menu, TRID_MENU_DEL, 0);

	mi = mMenuPopup(menu, NULL, x, y, 0);
	id = (mi)? mi->id: -1;

	mMenuDestroy(menu);

	//選択枠消去

	_draw_selbox(p, TRUE);

	//処理

	if(id == -1) return;

	switch(id)
	{
		//追加
		case TRID_MENU_ADD:
			_cmd_add(p);
			break;
		//削除
		case TRID_MENU_DEL:
			_cmd_del(p, no);
			break;
	}
}


//=============================
// イベント
//=============================


/** ボタン押し時 */

static void _event_press(DockBrushSizeArea *p,mEvent *ev)
{
	int no;

	no = _area_getindex_atpos(p, ev->pt.x, ev->pt.y);

	if(ev->pt.btt == M_BTT_RIGHT)
	{
		//右クリックメニュー

		_run_menu(p, ev->pt.rootx, ev->pt.rooty, no);
	}
	else if(ev->pt.btt == M_BTT_LEFT && no != -1)
	{
		//----- 左ボタン

		p->fbtt = (ev->pt.state & M_MODS_SHIFT)? _BTTF_DELETE: _BTTF_NORMAL;
		p->press_no = no;

		//通知

		if(p->fbtt == _BTTF_NORMAL)
			mWidgetAppendEvent_notify(NULL, p->wg.parent, 0, BrushSizeList_getValue(no), 0);

		_draw_selbox(p, FALSE);

		mWidgetGrabPointer(M_WIDGET(p));
	}
}

/** グラブ解放 */

static void _release_grab(DockBrushSizeArea *p)
{
	if(p->fbtt)
	{
		if(p->fbtt == _BTTF_DELETE)
			_cmd_del(p, p->press_no);
		else
			_draw_selbox(p, TRUE);
	
		mWidgetUngrabPointer(M_WIDGET(p));
		p->fbtt = 0;
	}
}

/** イベント */

static int _area_event_handle(mWidget *wg,mEvent *ev)
{
	DockBrushSizeArea *p = _AREA(wg);
	
	switch(ev->type)
	{
		case MEVENT_POINTER:
			if(ev->pt.type == MEVENT_POINTER_TYPE_PRESS)
			{
				if(!p->fbtt)
					_event_press(p, ev);
			}
			else if(ev->pt.type == MEVENT_POINTER_TYPE_RELEASE)
			{
				if(ev->pt.btt == M_BTT_LEFT)
					_release_grab(p);
			}
			break;
		case MEVENT_FOCUS:
			if(ev->focus.bOut)
				_release_grab(p);
			break;
	}

	return 1;
}


//=============================


/** サイズ変更時 */

static void _area_onsize_handle(mWidget *wg)
{
	int n;

	//高さ保存
	//DockBrush は非表示時は破棄されるので、常に記録しておく

	APP_CONF->dockbrush_height[0] = wg->h;
	
	//水平方向の個数

	n = (wg->w - 1) / EACHSIZE;
	if(n < 1) n = 1;

	_AREA(wg)->hnum = n;

	//スクロール

	_area_set_scrollinfo(_AREA(wg));
}

/** 作成 */

static DockBrushSizeArea *_area_new(mWidget *parent)
{
	DockBrushSizeArea *p;

	p = (DockBrushSizeArea *)mWidgetNew(sizeof(DockBrushSizeArea), parent);

	p->wg.fLayout = MLF_EXPAND_WH;
	p->wg.event = _area_event_handle;
	p->wg.draw = _area_draw;
	p->wg.onSize = _area_onsize_handle;
	p->wg.fEventFilter |= MWIDGET_EVENTFILTER_POINTER;

	p->hnum = 1;

	return p;
}


//**********************************
// DockBrushSize
//**********************************


/** 描画 */

static void _draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	//枠描画
	
	mPixbufBox(pixbuf, 0, 0, wg->w, wg->h, MSYSCOL(FRAME));
}

/** スクロールバー ハンドル */

static void _scroll_handle(mScrollBar *p,int pos,int flags)
{
	if(flags & MSCROLLBAR_N_HANDLE_F_CHANGE)
		mWidgetUpdate(M_WIDGET(p->wg.param));
}

/** 作成 */

DockBrushSize *DockBrushSize_new(mWidget *parent)
{
	DockBrushSize *p;

	p = (DockBrushSize *)mContainerNew(sizeof(DockBrushSize), parent);

	mContainerSetType(M_CONTAINER(p), MCONTAINER_TYPE_HORZ, 0);
	mContainerSetPadding_one(M_CONTAINER(p), 1);

	p->wg.h = APP_CONF->dockbrush_height[0] + 2;
	p->wg.fLayout = MLF_EXPAND_W | MLF_FIX_H;
	p->wg.draw = _draw_handle;

	//エリア

	p->area = _area_new(M_WIDGET(p));

	//スクロールバー

	p->scr = mScrollBarNew(0, M_WIDGET(p), MSCROLLBAR_S_VERT);

	p->scr->wg.fLayout = MLF_EXPAND_H;
	p->scr->wg.param = (intptr_t)p->area;
	p->scr->sb.handle = _scroll_handle;

	//

	p->area->scr = p->scr;

	return p;
}
