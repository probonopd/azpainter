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
 * DockLayerArea
 *
 * [dock]レイヤの一覧表示エリア
 *****************************************/

#include <stdlib.h>

#include "mDef.h"
#include "mRectBox.h"
#include "mWidget.h"
#include "mScrollView.h"
#include "mScrollViewArea.h"
#include "mScrollBar.h"
#include "mSysCol.h"
#include "mPixbuf.h"
#include "mEvent.h"
#include "mFont.h"
#include "mUtilStr.h"
#include "mTrans.h"
#include "mMenu.h"

#include "defWidgets.h"
#include "defDraw.h"
#include "AppCursor.h"

#include "TileImage.h"
#include "LayerList.h"
#include "LayerItem.h"

#include "MainWindow.h"
#include "Docks_external.h"

#include "draw_main.h"
#include "draw_layer.h"

#include "trgroup.h"

#include "img_layer_info.h"
#include "dataImagePattern.h"


//----------------------

typedef struct _DockLayerArea
{
	mWidget wg;
	mScrollViewAreaData sva;

	LayerItem *item;	//D&D 現在の移動先 (_MOVEDEST_INSERT で NULL の場合は終端へ)
	int fpress,
		dnd_type,		//D&D 現在の移動先のタイプ
		dnd_pos;		//D&D Y位置 (判定中:押し時, D&D中:マーカー表示位置)
	mPoint drag_ptpos;
}DockLayerArea;

//----------------------

#define _AREA(p)  ((DockLayerArea *)(p))

#define _EACH_H           40	//各レイヤの高さ
#define _MIN_WIDTH        150	//最小幅
#define _DEPTH_W          12	//深さ1の余白幅
#define _VISIBLE_BOX_SIZE 13	//表示ボックスのサイズ
#define _PREV_H           (_EACH_H - 7)	//プレビューの高さ
#define _PREV_W           _PREV_H
#define _FLAGS_BOX_SIZE   14	//フラグボックスの各サイズ (枠も含む)
#define _FLAGS_BOX_W      ((_FLAGS_BOX_SIZE - 1) * 4 + 1)	//フラグボックスの全体幅

#define _UPPER_Y        4			//上段のY位置
#define _LOWER_Y        (_EACH_H - 1 - 3 - _FLAGS_BOX_SIZE)	//下段のY位置
#define _PREV_X         3
#define _PREV_Y         3
#define _INFO_X         (_PREV_X + _PREV_W + 3)	//表示ボックス、フラグボックスのX位置
#define _INFO_NAME_X    (_INFO_X + _VISIBLE_BOX_SIZE + 4)	//レイヤ名のX位置

#define _FOLDER_ARROW_W  9
#define _FOLDER_ICON_X   5
#define _FOLDER_ARROW_X  (_INFO_X - 5 - _FOLDER_ARROW_W)

#define _COL_SEL_BKGND    0xffc2b8
#define _COL_SEL_FRAME    0xff0000
#define _COL_SEL_NAME     0xc80000
#define _COL_VISIBLE_BOX  0x002080

#define _DRAWONE_F_UPDATE     1
#define _DRAWONE_F_ONLY_INFO  2

#define _PRESS_MOVE_BEGIN 1
#define _PRESS_MOVE       2

/* 左ボタン押し時の処理 */

enum
{
	_PRESSLEFT_DONE,			//処理済み (カレント変更なし)
	_PRESSLEFT_SETCURRENT,		//通常カレント変更 (ボタンなどの処理なし)
	_PRESSLEFT_SETCURRENT_DONE,	//処理済み＋カレント変更
	_PRESSLEFT_OPTION,			//レイヤ設定
	_PRESSLEFT_SETCOLOR			//線の色設定
};

/* D&D 移動先のタイプ (負の値で移動しない) */

enum
{
	_MOVEDEST_NONE   = -1,
	_MOVEDEST_SCROLL = -2,
	_MOVEDEST_INSERT = 0,
	_MOVEDEST_FOLDER = 1
};

/* 右クリックメニュー */

enum
{
	TRID_MENU_OPTION = 1000,
	TRID_MENU_SETCOLOR,
	TRID_MENU_SETTYPE,
	TRID_MENU_PALETTE1,
	TRID_MENU_PALETTE2,
	TRID_MENU_F_LOCK,
	TRID_MENU_F_FILL_REF,
	TRID_MENU_F_MASKLAYER,
	TRID_MENU_F_MASKLAYER_UNDER,
	TRID_MENU_F_CHECKED,
	TRID_MENU_NEWLAYER_ABOVE,
	TRID_MENU_MOVE_FOLER_CHECKED,

	MENUID_PALETTE_TOP = 2000
};

//----------------------

static void _draw_one(DockLayerArea *area,mPixbuf *pixbuf,
	LayerItem *pi,int xtop,int ytop,uint8_t flags);
static void _draw_dnd(DockLayerArea *p,mBool erase);

//----------------------

static const char *g_blend_name[] = {
	"NORM", "MUL", "ADD", "SUB", "SCRN", "OVERL", "HARDL", "SOFTL",
	"DODGE", "BURN", "LBURN", "VIVID", "LINRL", "PINL",
	"DARK", "LIGHT", "DIFF"
};

//----------------------



//==============================
// sub
//==============================


/** スクロール情報セット */

void DockLayerArea_setScrollInfo(DockLayerArea *p)
{
	int num,depth,w;

	num = LayerList_getScrollInfo(APP_DRAW->layerlist, &depth);

	//水平

	w = depth * _DEPTH_W + _MIN_WIDTH;

	mScrollBarSetStatus(
		mScrollViewAreaGetScrollBar(M_SCROLLVIEWAREA(p), FALSE),
		0, (p->wg.w > w)? p->wg.w: w, p->wg.w);

	//垂直
	
	mScrollBarSetStatus(
		mScrollViewAreaGetScrollBar(M_SCROLLVIEWAREA(p), TRUE),
		0, num * _EACH_H, p->wg.h); 
}

/** 先頭レイヤと、先頭レイヤの表示位置取得 */

static LayerItem *_getTopItemInfo(DockLayerArea *p,int *xtop,int *ytop)
{
	mPoint pt;

	mScrollViewAreaGetScrollPos(M_SCROLLVIEWAREA(p), &pt);

	*xtop = -pt.x;
	*ytop = -pt.y;

	return LayerList_getItem_top(APP_DRAW->layerlist);
}

/** 指定レイヤの表示X位置と幅を取得
 *
 * @param item  NULL で x = 0, w = 全体 */

static int _getItem_left_width(DockLayerArea *p,LayerItem *item,int *ret_width)
{
	int depw,w;

	depw = (item)? LayerItem_getTreeDepth(item) * _DEPTH_W: 0;

	w = p->wg.w - depw;
	if(w < _MIN_WIDTH) w = _MIN_WIDTH;

	*ret_width = w;

	return depw;
}

/** 指定レイヤの表示位置取得 (ウィジェット座標)
 *
 * @return 閉じられているか、Yが表示範囲外の場合は FALSE */

static mBool _getItemXY(DockLayerArea *p,LayerItem *item,int *xret,int *yret)
{
	LayerItem *pi;
	int xtop,y;

	pi = _getTopItemInfo(p, &xtop, &y);

	for(; pi; pi = LayerItem_getNextExpand(pi), y += _EACH_H)
	{
		//範囲外 (下)
		
		if(y >= p->wg.h) return FALSE;

		//

		if(pi == item)
		{
			*xret = xtop;
			*yret = y;

			//上が範囲外で FALSE

			return (y > -_EACH_H);
		}
	}

	return FALSE;
}

/** 指定レイヤの表示Y位置取得 (先頭レイヤを0とした絶対位置)
 *
 * @return item のレイヤ全体が一覧上に見えているか */

static mBool _getItemY_abs(DockLayerArea *p,LayerItem *item,int *yret)
{
	LayerItem *pi;
	int xtop,y,scry;

	pi = _getTopItemInfo(p, &xtop, &y);

	scry = -y;

	for(; pi && pi != item; pi = LayerItem_getNextExpand(pi), y += _EACH_H);

	*yret = scry + y;

	return (y >= 0 && y + _EACH_H <= p->wg.h);
}

/** ポインタ位置からレイヤアイテム取得
 *
 * @param pttop NULL でなければ、表示位置が返る */

static LayerItem *_get_item_at_pt(DockLayerArea *p,int y,mPoint *pttop)
{
	LayerItem *pi;
	int ytop,xtop;

	pi = _getTopItemInfo(p, &xtop, &ytop);

	for(; pi; pi = LayerItem_getNextExpand(pi), ytop += _EACH_H)
	{
		if(ytop <= y && y < ytop + _EACH_H)
		{
			if(pttop)
			{
				pttop->x = xtop;
				pttop->y = ytop;
			}
			return pi;
		}
	}

	return NULL;
}

/** 指定アイテムのみ直接描画 (内部用) */

static void _draw_one_direct(DockLayerArea *p,
	LayerItem *pi,int xtop,int ytop,uint8_t flags)
{
	mPixbuf *pixbuf;

	if((pixbuf = mWidgetBeginDirectDraw(M_WIDGET(p))))
	{
		_draw_one(p, pixbuf, pi, xtop, ytop, flags);
	
		mWidgetEndDirectDraw(M_WIDGET(p), pixbuf);
	}
}

/** 指定アイテムのみ直接描画 */

void DockLayerArea_drawLayer(DockLayerArea *p,LayerItem *pi)
{
	int x,y;

	if(_getItemXY(p, pi, &x, &y))
		_draw_one_direct(p, pi, x, y, _DRAWONE_F_UPDATE);
}

/** カレント変更時の更新 (カレントが一覧上で見えるように) */

void DockLayerArea_changeCurrent_visible(DockLayerArea *p,LayerItem *lastitem,mBool update_all)
{
	int y;

	//カレントが表示範囲外ならスクロール

	if(!_getItemY_abs(p, APP_DRAW->curlayer, &y))
	{
		y -= p->wg.h / 2 - _EACH_H / 2;

		mScrollBarSetPos(mScrollViewAreaGetScrollBar(M_SCROLLVIEWAREA(p), TRUE), y);

		update_all = TRUE;
	}

	//更新

	if(update_all)
		mWidgetUpdate(M_WIDGET(p));
	else
	{
		//スクロールも全体更新もない場合

		if(lastitem) DockLayerArea_drawLayer(p, lastitem);
		DockLayerArea_drawLayer(p, APP_DRAW->curlayer);
	}
}

/** メニュー描画 */

static void _menu_draw_handle(mPixbuf *pixbuf,mMenuItemInfo *pi,mMenuItemDraw *draw)
{
	mBox *box = &draw->box;

	//背景

	if(draw->flags & MMENUITEM_DRAW_F_SELECTED)
		mPixbufFillBox(pixbuf, box->x, box->y, box->w, box->h, MSYSCOL(MENU_SELECT));

	//枠

	mPixbufBox(pixbuf, box->x + 4, box->y + 2, box->w - 8, box->h - 4, 0);

	//色

	mPixbufFillBox(pixbuf,
		box->x + 5, box->y + 3, box->w - 10, box->h - 6, pi->param1);
}

/** 右クリックメニュー */

static void _run_rbtt_menu(DockLayerArea *p,LayerItem *pi,int rx,int ry)
{
	mMenu *menu,*sub[2];
	mMenuItemInfo *pmi,mi;
	int i,id;
	mBool iscol,isimg;
	uint16_t dat[] = {
		TRID_MENU_OPTION, TRID_MENU_SETCOLOR, TRID_MENU_SETTYPE, 0xfffe,
		TRID_MENU_PALETTE1, TRID_MENU_PALETTE2, 0xfffe,
		TRID_MENU_F_LOCK, TRID_MENU_F_FILL_REF,
		TRID_MENU_F_MASKLAYER, TRID_MENU_F_MASKLAYER_UNDER,
		TRID_MENU_F_CHECKED, 0xffff
	};
	uint16_t dat_folder[] = {
		TRID_MENU_NEWLAYER_ABOVE, TRID_MENU_MOVE_FOLER_CHECKED, 0xfffe, 0xffff
	};

	M_TR_G(TRGROUP_DOCK_LAYER);

	//----- メニュー

	iscol = LayerItem_isType_layercolor(pi);
	isimg = LAYERITEM_IS_IMAGE(pi);

	menu = mMenuNew();

	//追加

	if(!isimg)
		mMenuAddTrArray16(menu, dat_folder);
	
	mMenuAddTrArray16(menu, dat);

	//

	mMenuSetEnable(menu, TRID_MENU_SETCOLOR, iscol);
	mMenuSetEnable(menu, TRID_MENU_SETTYPE, isimg);
	mMenuSetEnable(menu, TRID_MENU_PALETTE1, iscol);
	mMenuSetEnable(menu, TRID_MENU_PALETTE2, iscol);

	//パレット

	if(iscol)
	{
		mMemzero(&mi, sizeof(mMenuItemInfo));

		sub[0] = mMenuNew();
		sub[1] = mMenuNew();

		for(i = 0, id = 0; i < LAYERCOLPAL_NUM; i++)
		{
			if(i == LAYERCOLPAL_NUM / 2) id = 1;
		
			mi.id = MENUID_PALETTE_TOP + i;
			mi.width = 48;
			mi.height = 16;
			mi.param1 = mRGBtoPix(APP_DRAW->col.layercolpal[i]);
			mi.draw = _menu_draw_handle;

			mMenuAdd(sub[id], &mi);
		}

		mMenuSetSubmenu(menu, TRID_MENU_PALETTE1, sub[0]);
		mMenuSetSubmenu(menu, TRID_MENU_PALETTE2, sub[1]);
	}

	//フラグ

	mMenuSetCheck(menu, TRID_MENU_F_LOCK, LAYERITEM_IS_LOCK(pi));

	if(isimg)
	{
		mMenuSetCheck(menu, TRID_MENU_F_FILL_REF, LAYERITEM_IS_FILLREF(pi));
		mMenuSetCheck(menu, TRID_MENU_F_MASKLAYER, (APP_DRAW->masklayer == pi));
		mMenuSetCheck(menu, TRID_MENU_F_MASKLAYER_UNDER, LAYERITEM_IS_MASK_UNDER(pi));
		mMenuSetCheck(menu, TRID_MENU_F_CHECKED, LAYERITEM_IS_CHECKED(pi));
	}
	else
	{
		mMenuSetEnable(menu, TRID_MENU_F_FILL_REF, 0);
		mMenuSetEnable(menu, TRID_MENU_F_MASKLAYER, 0);
		mMenuSetEnable(menu, TRID_MENU_F_MASKLAYER_UNDER, 0);
		mMenuSetEnable(menu, TRID_MENU_F_CHECKED, 0);
	}

	//

	pmi = mMenuPopup(menu, NULL, rx, ry, 0);
	id = (pmi)? pmi->id: -1;

	mMenuDestroy(menu);

	//------- コマンド

	if(id == -1) return;

	if(id >= MENUID_PALETTE_TOP)
	{
		//パレットから色セット
		
		drawLayer_setLayerColor(APP_DRAW, pi,
			APP_DRAW->col.layercolpal[id - MENUID_PALETTE_TOP]);
		return;
	}

	switch(id)
	{
		//フォルダの上に新規レイヤ
		case TRID_MENU_NEWLAYER_ABOVE:
			MainWindow_layer_new_dialog(APP_WIDGETS->mainwin, pi);
			break;
		//チェックレイヤをフォルダに移動
		case TRID_MENU_MOVE_FOLER_CHECKED:
			drawLayer_moveCheckedToFolder(APP_DRAW, pi);
			break;
		//設定
		case TRID_MENU_OPTION:
			MainWindow_layer_setoption(APP_WIDGETS->mainwin, pi);
			break;
		//線の色変更
		case TRID_MENU_SETCOLOR:
			MainWindow_layer_setcolor(APP_WIDGETS->mainwin, pi);
			break;
		//カラータイプ変更
		case TRID_MENU_SETTYPE:
			MainWindow_layer_settype(APP_WIDGETS->mainwin, pi);
			break;
		//ロック
		case TRID_MENU_F_LOCK:
			drawLayer_revLock(APP_DRAW, pi);
			break;
		//塗りつぶし判定元
		case TRID_MENU_F_FILL_REF:
			drawLayer_revFillRef(APP_DRAW, pi);
			break;
		//マスクレイヤに指定
		case TRID_MENU_F_MASKLAYER:
			drawLayer_setLayerMask(APP_DRAW, pi, -1);
			break;
		//下レイヤマスク
		case TRID_MENU_F_MASKLAYER_UNDER:
			drawLayer_setLayerMask(APP_DRAW, pi, -2);
			break;
		//チェック
		case TRID_MENU_F_CHECKED:
			drawLayer_revChecked(APP_DRAW, pi);
			break;
	}
}


//==============================
// イベント
//==============================


/** 左ボタン押し時の処理 */

static LayerItem *_press_left(DockLayerArea *p,int x,int y,
	mBool bDblClk,uint32_t state,int *retproc)
{
	LayerItem *pi;
	mPoint ptpos;
	int proc,n;
	mBool bFolder;

	//レイヤアイテム

	pi = _get_item_at_pt(p, y, &ptpos);
	if(!pi) return NULL;

	//

	x -= ptpos.x + LayerItem_getTreeDepth(pi) * _DEPTH_W;
	y -= ptpos.y;

	bFolder = LAYERITEM_IS_FOLDER(pi);

	proc = _PRESSLEFT_SETCURRENT;

	//

	if(_UPPER_Y <= y && y < _UPPER_Y + _VISIBLE_BOX_SIZE)
	{
		//------ 上段
	
		if(_INFO_X <= x && x < _INFO_X + _VISIBLE_BOX_SIZE)
		{
			//[表示/非表示ボックス]

			drawLayer_revVisible(APP_DRAW, pi);
			_draw_one_direct(p, pi, ptpos.x, ptpos.y, _DRAWONE_F_UPDATE);

			proc = _PRESSLEFT_DONE;
		}
		else if(bFolder && _FOLDER_ARROW_X <= x && x < _FOLDER_ARROW_X + _FOLDER_ARROW_W)
		{
			//[フォルダ展開/閉じる]

			/* [!] この時、カレントレイヤは常に変更しなければならない。
			 * 指定されたレイヤのフォルダを閉じる時、
			 * 現在のカレントレイヤがそのフォルダの子である場合、
			 * 一覧上にカレントレイヤが表示されなくなるため。 */

			drawLayer_revFolderExpand(APP_DRAW, pi);

			proc = _PRESSLEFT_SETCURRENT_DONE;
		}
	}
	else if(_LOWER_Y <= y && y < _LOWER_Y + _FLAGS_BOX_SIZE
		&& _INFO_X <= x && x < _INFO_X + _FLAGS_BOX_W)
	{
		//------ フラグボックス

		n = (x - _INFO_X) / (_FLAGS_BOX_SIZE - 1);
		if(n > 3) n = 3;
		
		//フォルダはロックのみ

		if(!bFolder || (bFolder && n == 0))
		{
			proc = _PRESSLEFT_DONE;

			switch(n)
			{
				//ロック
				case 0:
					drawLayer_revLock(APP_DRAW, pi);
					break;
				//塗りつぶし判定元
				case 1:
					drawLayer_revFillRef(APP_DRAW, pi);
					break;
				//レイヤマスク
				case 2:
					drawLayer_setLayerMask(APP_DRAW, pi,
						(bDblClk || (state & M_MODS_SHIFT))? -2: -1);
					break;
				//チェック
				case 3:
					drawLayer_revChecked(APP_DRAW, pi);
					break;
			}
		}
	}

	//他の領域

	if(proc == _PRESSLEFT_SETCURRENT)
	{
		if(x >= _PREV_X && x < _PREV_X + _PREV_W
			&& y >= _PREV_Y && y < _PREV_Y + _PREV_H)
		{
			//プレビュー部分 (A16/A1 時)
			/* ダブルクリックで色設定ダイアログ。
			 * +Shift で描画色をレイヤ色にセット。 */

			if(LayerItem_isType_layercolor(pi))
			{
				if(bDblClk)
					proc = _PRESSLEFT_SETCOLOR;
				else if(state & M_MODS_SHIFT)
				{
					drawLayer_setLayerColor(APP_DRAW, pi, APP_DRAW->col.drawcol);
					proc = _PRESSLEFT_DONE;
				}
			}
		}
		else if(bDblClk)
		{
			//プレビュー部分以外でダブルクリック => レイヤ設定

			proc = _PRESSLEFT_OPTION;
		}
	}

	//

	*retproc = proc;

	return pi;
}

/** 押し時 */

static void _event_press(DockLayerArea *p,mEvent *ev)
{
	LayerItem *pi;
	int proc;

	if(ev->pt.btt == M_BTT_RIGHT)
	{
		//----- 右ボタン

		pi = _get_item_at_pt(p, ev->pt.y, NULL);

		if(pi)
			_run_rbtt_menu(p, pi, ev->pt.rootx, ev->pt.rooty);
	}
	else if(ev->pt.btt == M_BTT_LEFT)
	{
		//----- 左ボタン

		pi = _press_left(p, ev->pt.x, ev->pt.y,
				(ev->pt.type == MEVENT_POINTER_TYPE_DBLCLK), ev->pt.state, &proc);

		//範囲外、または各処理済みでカレント変更しない場合

		if(!pi || proc == _PRESSLEFT_DONE)
			return;


		if(proc == _PRESSLEFT_OPTION)
		{
			//ダブルクリックでレイヤ設定
			
			MainWindow_layer_setoption(APP_WIDGETS->mainwin, pi);
		}
		else if(proc == _PRESSLEFT_SETCOLOR)
		{
			//プレビュー部分ダブルクリックで線の色変更

			MainWindow_layer_setcolor(APP_WIDGETS->mainwin, pi);
		}
		else
		{
			//カレントレイヤ変更
			/* カレントレイヤ上をクリックした (カレントが変化しない) 場合は、
			 * プレビュー更新 */

			if(!drawLayer_setCurrent(APP_DRAW, pi))
				DockLayer_update_curlayer(FALSE);

			//+Ctrl でカレントレイヤのみ表示

			if(proc == _PRESSLEFT_SETCURRENT && (ev->pt.state & M_MODS_CTRL))
				drawLayer_showAll(APP_DRAW, 2);

			//レイヤ移動 D&D 判定開始

			if(proc == _PRESSLEFT_SETCURRENT)
			{
				p->fpress = _PRESS_MOVE_BEGIN;
				p->dnd_pos = ev->pt.y;

				mWidgetGrabPointer(M_WIDGET(p));
			}
		}
	}
}

/** 移動時 (レイヤ移動) */

static void _event_motion_movelayer(DockLayerArea *p,mEvent *ev)
{
	LayerItem *pi;
	int y,type;
	mPoint ptpos;

	y = ev->pt.y;

	//----- ドラッグ開始判定 (押し位置から一定距離離れたらドラッグ開始)

	if(p->fpress == _PRESS_MOVE_BEGIN)
	{
		if(abs(y - p->dnd_pos) <= _EACH_H / 2)
			return;

		//以降、ドラッグ開始

		mWidgetSetCursor(M_WIDGET(p), AppCursor_getForDrag(APP_CURSOR_ITEM_MOVE));

		p->fpress = _PRESS_MOVE;
		p->item = NULL;
		p->dnd_type = _MOVEDEST_NONE;
	}

	//------ ドラッグ中

	//エリアの範囲外の場合

	if(y < 0 || y >= p->wg.h)
	{
		//スクロール開始
	
		if(p->dnd_type != _MOVEDEST_SCROLL)
		{
			mWidgetTimerAdd(M_WIDGET(p), 0, 200, (y >= p->wg.h));

			_draw_dnd(p, TRUE);
			
			p->dnd_type = _MOVEDEST_SCROLL;
		}

		return;
	}

	//エリア範囲内のため、スクロール解除

	if(p->dnd_type == _MOVEDEST_SCROLL)
		mWidgetTimerDelete(M_WIDGET(p), 0);

	//移動先

	pi = _get_item_at_pt(p, y, &ptpos);

	type = _MOVEDEST_INSERT;

	if(pi == APP_DRAW->curlayer)
		//カレントの場合は移動なし
		type = _MOVEDEST_NONE;
	else if(!pi)
	{
		//一覧の一番下の余白部分なら終端へ

		mScrollBar *sb = mScrollViewAreaGetScrollBar(M_SCROLLVIEWAREA(p), TRUE);

		ptpos.y = sb->sb.max;
	}
	else if(LayerItem_isChildItem(pi, APP_DRAW->curlayer))
		//移動元(カレントレイヤ)がフォルダで、移動先がそのフォルダ下の場合は無効
		type = _MOVEDEST_NONE;
	else if(LAYERITEM_IS_FOLDER(pi))
		//移動先がフォルダの場合、境界から一定範囲内は挿入、それ以外はフォルダ内への移動
		type = (y - ptpos.y >= 8)? _MOVEDEST_FOLDER: _MOVEDEST_INSERT;

	//移動先変更

	if(pi != p->item || type != p->dnd_type)
	{
		_draw_dnd(p, TRUE);
	
		p->item = pi;
		p->dnd_type = type;
		p->dnd_pos = ptpos.y;

		_draw_dnd(p, FALSE);
	}
}

/** グラブ解放 */

static void _release_grab(DockLayerArea *p)
{
	if(p->fpress)
	{
		//D&D 移動

		if(p->fpress == _PRESS_MOVE)
		{
			mWidgetTimerDelete(M_WIDGET(p), 0);

			mWidgetSetCursor(M_WIDGET(p), 0);

			_draw_dnd(p, TRUE);

			if(p->dnd_type >= 0)
				drawLayer_moveDND(APP_DRAW, p->item, p->dnd_type);
		}
	
		p->fpress = 0;
		mWidgetUngrabPointer(M_WIDGET(p));
	}
}


//==============================
// ハンドラ
//==============================


/** イベント */

static int _area_event_handle(mWidget *wg,mEvent *ev)
{
	DockLayerArea *p = _AREA(wg);

	switch(ev->type)
	{
		case MEVENT_POINTER:
			if(ev->pt.type == MEVENT_POINTER_TYPE_MOTION)
			{
				//移動
				
				if(p->fpress == _PRESS_MOVE_BEGIN || p->fpress == _PRESS_MOVE)
					_event_motion_movelayer(p, ev);
			}
			else if(ev->pt.type == MEVENT_POINTER_TYPE_PRESS
				|| ev->pt.type == MEVENT_POINTER_TYPE_DBLCLK)
			{
				//押し

				if(!p->fpress)
					_event_press(p, ev);
			}
			else if(ev->pt.type == MEVENT_POINTER_TYPE_RELEASE)
			{
				//離し

				if(ev->pt.btt == M_BTT_LEFT)
					_release_grab(p);
			}
			break;

		//スクロール
		case MEVENT_NOTIFY:
			if(ev->notify.widgetFrom == wg->parent
				&& (ev->notify.type == MSCROLLVIEWAREA_N_SCROLL_VERT
					|| ev->notify.type == MSCROLLVIEWAREA_N_SCROLL_HORZ)
				&& (ev->notify.param2 & MSCROLLBAR_N_HANDLE_F_CHANGE))
			{
				mWidgetUpdate(wg);
			}
			break;

		//タイマー
		case MEVENT_TIMER:
			if(mScrollBarMovePos(
				mScrollViewAreaGetScrollBar(M_SCROLLVIEWAREA(wg), TRUE),
				(ev->timer.param)? _EACH_H: -_EACH_H))
				mWidgetUpdate(wg);
			break;

		//ホイールによるスクロール
		case MEVENT_SCROLL:
			if(p->fpress == 0
				&& (ev->scr.dir == MEVENT_SCROLL_DIR_UP ||
					ev->scr.dir == MEVENT_SCROLL_DIR_DOWN))
			{
				if(mScrollBarMovePos(
					mScrollViewAreaGetScrollBar(M_SCROLLVIEWAREA(wg), TRUE),
					(ev->scr.dir == MEVENT_SCROLL_DIR_UP)? -_EACH_H: _EACH_H))
					mWidgetUpdate(wg);
			}
			break;

		case MEVENT_FOCUS:
			if(ev->focus.bOut)
				_release_grab(p);
			break;
	}
	
	return 1;
}

/** 描画 */

static void _area_draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	LayerItem *pi;
	int y;
	mPoint ptscr;

	//レイヤ

	mScrollViewAreaGetScrollPos(M_SCROLLVIEWAREA(wg), &ptscr);

	pi = LayerList_getItem_top(APP_DRAW->layerlist);

	for(y = -ptscr.y; pi && y < wg->h; pi = LayerItem_getNextExpand(pi), y += _EACH_H)
	{
		if(y > -_EACH_H)
			_draw_one((DockLayerArea *)wg, pixbuf, pi, -ptscr.x, y, 0);
	}

	//残りの背景

	if(y < wg->h)
		mPixbufFillBox(pixbuf, 0, y, wg->w, wg->h - y, MSYSCOL(FACE_DARKER));
}

/** サイズ変更時 */

static void _area_onsize_handle(mWidget *wg)
{
	DockLayerArea_setScrollInfo((DockLayerArea *)wg);
}

/** D&D */

static int _area_ondnd_handle(mWidget *wg,char **files)
{
	mRect rc;

	mRectEmpty(&rc);

	//読み込み

	for(; *files; files++)
		drawLayer_newLayer_file(APP_DRAW, *files, FALSE, &rc);

	//まとめて更新

	drawUpdate_rect_imgcanvas_canvasview_fromRect(APP_DRAW, &rc);
	DockLayer_update_all();

	return 1;
}


//==============================
// 作成
//==============================


/** エリア作成 */

static mScrollViewArea *_area_new(mWidget *parent)
{
	DockLayerArea *p;

	p = (DockLayerArea *)mScrollViewAreaNew(sizeof(DockLayerArea), parent);
	if(!p) return NULL;

	p->wg.fEventFilter |= MWIDGET_EVENTFILTER_POINTER | MWIDGET_EVENTFILTER_SCROLL;
	p->wg.fState |= MWIDGET_STATE_ENABLE_DROP;
	p->wg.draw = _area_draw_handle;
	p->wg.event = _area_event_handle;
	p->wg.onSize = _area_onsize_handle;
	p->wg.onDND = _area_ondnd_handle;

	return (mScrollViewArea *)p;
}

/** スクロールビュー作成
 *
 * @return エリアのウィジェットを返す */

DockLayerArea *DockLayerArea_new(mWidget *parent)
{
	mScrollView *p;

	p = mScrollViewNew(0, parent, MSCROLLVIEW_S_HORZVERT_FRAME | MSCROLLVIEW_S_FIX_HORZVERT);

	p->wg.fLayout = MLF_EXPAND_WH;

	p->sv.area = _area_new(M_WIDGET(p));

	return (DockLayerArea *)p->sv.area;
}


//==============================
// 描画
//==============================


/** レイヤを一つ描画 */

void _draw_one(DockLayerArea *area,mPixbuf *pixbuf,
	LayerItem *pi,int xtop,int ytop,uint8_t flags)
{
	int width,right,x,y,i,n;
	mBool bSel,bFolder;
	uint8_t m[8];
	const char *pc;
	mPixCol coltmp[4],*pcol;
	uint32_t colbkgnd,col[5];

	//

	n = LayerItem_getTreeDepth(pi) * _DEPTH_W;

	width = area->wg.w - n;
	if(width < _MIN_WIDTH) width = _MIN_WIDTH;

	xtop += n;

	right = xtop + width;

	//

	bSel = (pi == APP_DRAW->curlayer);
	bFolder = LAYERITEM_IS_FOLDER(pi);

	colbkgnd = (bSel)? mRGBtoPix(_COL_SEL_BKGND): MSYSCOL(WHITE);

	col[0] = 0;
	col[1] = MSYSCOL(WHITE);
	col[2] = mRGBtoPix(_COL_VISIBLE_BOX);
	col[3] = mRGBtoPix(0xb0b0b0);		//フラグ OFF 時の色
	col[4] = mRGBtoPix(0xFF42DD);		//フラグ ON 時の背景色

	//----- 背景など基本部分

	if(!(flags & _DRAWONE_F_ONLY_INFO))
	{
		//左の余白 (フォルダの深さ分)

		if(xtop > 0)
			mPixbufFillBox(pixbuf, 0, ytop, xtop, _EACH_H, MSYSCOL(FACE_DARKER));
	
		//背景

		mPixbufFillBox(pixbuf, xtop, ytop, right - xtop, _EACH_H - 1, colbkgnd);

		//右の余白

		if(right < area->wg.w)
			mPixbufFillBox(pixbuf, right, ytop, area->wg.w - right, _EACH_H, MSYSCOL(FACE_DARKER));

		//下の境界線

		mPixbufLineH(pixbuf, 0, ytop + _EACH_H - 1, area->wg.w, 0);

		//選択枠

		if(bSel)
			mPixbufBox(pixbuf, xtop, ytop, width, _EACH_H - 1, mRGBtoPix(_COL_SEL_FRAME));
	}

	//------- プレビュー

	if(!bFolder && !(flags & _DRAWONE_F_ONLY_INFO))
	{
		x = xtop + _PREV_X;
		y = ytop + _PREV_Y;
	
		mPixbufBox(pixbuf, x, y, _PREV_W, _PREV_H, 0);

		TileImage_drawPreview(pi->img, pixbuf,
			x + 1, y + 1, _PREV_W - 2, _PREV_H - 2,
			APP_DRAW->imgw, APP_DRAW->imgh);

		//カラータイプ別

		x += _PREV_W - 8;
		y += _PREV_H - 8;

		if(pi->coltype == TILEIMAGE_COLTYPE_GRAY)
		{
			//グレイスケール
			
			mPixbufBltFromGray(pixbuf, x, y,
				g_img_coltype_gray, 0, 0, 8, 8, 8);
		}
		else if(pi->coltype == TILEIMAGE_COLTYPE_ALPHA16
			|| pi->coltype == TILEIMAGE_COLTYPE_ALPHA1)
		{
			//A16/A1
		
			coltmp[0] = 0;
			coltmp[1] = MSYSCOL(WHITE);
			coltmp[2] = mRGBtoPix(pi->col);

			mPixbufBltFrom2bit(pixbuf, x, y,
				(pi->coltype == TILEIMAGE_COLTYPE_ALPHA16)? g_img_coltype_16bit: g_img_coltype_1bit,
				0, 0, 8, 8, 8, coltmp);
		}
	}

	//------ フォルダ

	if(bFolder)
	{
		//フォルダアイコン

		mPixbufDrawBitPattern(pixbuf,
			xtop + _FOLDER_ICON_X, ytop + _UPPER_Y + 2,
			g_img_folder_icon, 10, 9, 0);

		//矢印

		if(LAYERITEM_IS_EXPAND(pi))
		{
			mPixbufDrawBitPattern(pixbuf,
				xtop + _FOLDER_ARROW_X, ytop + _UPPER_Y + 4,
				g_img_arrow_down, 9, 5, 0);
		}
		else
		{
			mPixbufDrawBitPattern(pixbuf,
				xtop + _FOLDER_ARROW_X + 2, ytop + _UPPER_Y + 2,
				g_img_arrow_right, 5, 9, 0);
		}
	}

	//------ 共通部分

	//表示/非表示ボックス

	mPixbufBltFrom2bit(pixbuf,
		xtop + _INFO_X, ytop + _UPPER_Y,
		g_img_visible_2bit, (LAYERITEM_IS_VISIBLE(pi))? 13: 0, 0, 13, 13, 13 * 2, col);

	//名前

	if(!(flags & _DRAWONE_F_ONLY_INFO))
	{
		mPixbufSetClipBox_d(pixbuf,
			xtop + _INFO_NAME_X, ytop + _UPPER_Y,
			right - (xtop + _INFO_NAME_X) - 4, 15);
	
		mFontDrawText(mWidgetGetFont(M_WIDGET(area)), pixbuf,
			xtop + _INFO_NAME_X, ytop + _UPPER_Y,
			pi->name, -1,
			(bSel)? _COL_SEL_NAME: 0);

		mPixbufClipNone(pixbuf);
	}

	//フラグボックス (枠)

	x = xtop + _INFO_X;
	y = ytop + _LOWER_Y;

	if(bFolder)
		//フォルダ時はロックのみ
		mPixbufBox(pixbuf, x, y, _FLAGS_BOX_SIZE, _FLAGS_BOX_SIZE, 0);
	else
	{
		//通常レイヤ時は4つ
		
		mPixbufBox(pixbuf, x, y, _FLAGS_BOX_W, _FLAGS_BOX_SIZE, 0);

		for(i = 0, x += _FLAGS_BOX_SIZE - 1; i < 3; i++, x += _FLAGS_BOX_SIZE - 1)
			mPixbufLineV(pixbuf, x, y + 1, _FLAGS_BOX_SIZE - 2, 0);
	}

	//

	coltmp[0] = col[3];
	coltmp[1] = col[1];
	coltmp[2] = col[1];
	coltmp[3] = col[4];

	x = xtop + _INFO_X + 1;
	y++;

	//ロック

	pcol = coltmp + ((pi->flags & LAYERITEM_F_LOCK)? 2: 0);

	mPixbufBltFrom1bit(pixbuf, x, y,
		g_img_flags, 0, 0, _FLAGS_BOX_SIZE - 2, _FLAGS_BOX_SIZE - 2, (_FLAGS_BOX_SIZE - 2) * 5,
		pcol[0], pcol[1]);

	x += _FLAGS_BOX_SIZE - 1;

	//フラグボックス (通常レイヤ時)

	if(!bFolder)
	{
		//塗りつぶし判定元

		pcol = coltmp + ((pi->flags & LAYERITEM_F_FILLREF)? 2: 0);

		mPixbufBltFrom1bit(pixbuf, x, y,
			g_img_flags, (_FLAGS_BOX_SIZE - 2) * 1, 0, _FLAGS_BOX_SIZE - 2, _FLAGS_BOX_SIZE - 2, (_FLAGS_BOX_SIZE - 2) * 5,
			pcol[0], pcol[1]);

		x += _FLAGS_BOX_SIZE - 1;

		//レイヤマスク

		if(pi->flags & LAYERITEM_F_MASK_UNDER)
			n = 3, pcol = coltmp + 2;
		else if(APP_DRAW->masklayer == pi)
			n = 2, pcol = coltmp + 2;
		else
			n = 2, pcol = coltmp;

		mPixbufBltFrom1bit(pixbuf, x, y,
			g_img_flags, (_FLAGS_BOX_SIZE - 2) * n, 0, _FLAGS_BOX_SIZE - 2, _FLAGS_BOX_SIZE - 2, (_FLAGS_BOX_SIZE - 2) * 5,
			pcol[0], pcol[1]);

		x += _FLAGS_BOX_SIZE - 1;

		//チェック

		pcol = coltmp + ((pi->flags & LAYERITEM_F_CHECKED)? 2: 0);

		mPixbufBltFrom1bit(pixbuf, x, y,
			g_img_flags, (_FLAGS_BOX_SIZE - 2) * 4, 0, _FLAGS_BOX_SIZE - 2, _FLAGS_BOX_SIZE - 2, (_FLAGS_BOX_SIZE - 2) * 5,
			pcol[0], pcol[1]);
	}

	//合成モード

	if(!bFolder)
	{
		pc = g_blend_name[pi->blendmode];

		for(i = 0; *pc ;i++, pc++)
			m[i] = *pc - 'A';

		m[i] = 255;

		mPixbufDrawBitPatternSum(pixbuf,
			xtop + _INFO_X + 6 + _FLAGS_BOX_W, ytop + _EACH_H - 7 - 7,
			g_img_alphabet, 156, 7, 6, m, mRGBtoPix(0x000080));
	}

	//不透明度

	n = mIntToStr((char *)m, (int)(pi->opacity / 128.0 * 100.0 + 0.5));

	for(i = 0; i < n; i++)
		m[i] -= '0';

	m[n] = 255;

	mPixbufDrawBitPatternSum(pixbuf,
		xtop + width - 5 * n - 4, ytop + _EACH_H - 9 - 6,
		g_imgpat_number_5x9, IMGPAT_NUMBER_5x9_PATW, 9, 5,
		m, 0);

	//------ 更新

	if(flags & _DRAWONE_F_UPDATE)
		mWidgetUpdateBox_d(M_WIDGET(area), 0, ytop, area->wg.w, _EACH_H);
}

/** D&D 移動時の描画 */

void _draw_dnd(DockLayerArea *p,mBool erase)
{
	mPixbuf *pixbuf;
	uint32_t col;
	int x,y,w;

	//移動先なし

	if(p->dnd_type < 0) return;

	//色

	if(erase)
	{
		if(p->item)
			col = MSYSCOL(WHITE);
		else
			col = MSYSCOL(FACE_DARKER);
	}
	else
		col = mRGBtoPix(0xff0000);

	//描画

	if((pixbuf = mWidgetBeginDirectDraw(M_WIDGET(p))))
	{
		x = -mScrollViewAreaGetHorzScrollPos(M_SCROLLVIEWAREA(p));
		x += _getItem_left_width(p, p->item, &w);
		y = p->dnd_pos;
	
		if(p->dnd_type == _MOVEDEST_FOLDER)
		{
			//フォルダ

			mPixbufBox(pixbuf, x, y, w, _EACH_H - 1, col);
			mPixbufBox(pixbuf, x + 1, y + 1, w - 2, _EACH_H - 3, col);

			mWidgetUpdateBox_d(M_WIDGET(p), x, y, w, _EACH_H - 1);
		}
		else
		{
			//挿入

			mPixbufFillBox(pixbuf, x, y, w, 2, col);

			mWidgetUpdateBox_d(M_WIDGET(p), x, y, w, 2);
		}

		mWidgetEndDirectDraw(M_WIDGET(p), pixbuf);
	}
}
