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
 * DockBrushList
 *
 * (Dock)ブラシのリスト
 *****************************************/

#include <stdlib.h>	//abs
#include <string.h>	//strcmp

#include "mDef.h"
#include "mWidget.h"
#include "mContainerDef.h"
#include "mContainer.h"
#include "mWindow.h"
#include "mDialog.h"
#include "mLabel.h"
#include "mLineEdit.h"
#include "mScrollBar.h"
#include "mEvent.h"
#include "mPixbuf.h"
#include "mUtilStr.h"
#include "mSysCol.h"
#include "mMenu.h"
#include "mTrans.h"
#include "mSysDialog.h"
#include "mStr.h"
#include "mFont.h"
#include "mMessageBox.h"
#include "mClipboard.h"

#include "defConfig.h"
#include "defDrawGlobal.h"
#include "defTool.h"

#include "BrushList.h"
#include "BrushItem.h"
#include "AppCursor.h"

#include "draw_main.h"

#include "trgroup.h"
#include "trid_word.h"


//---------------------
//ボタンが押された時の情報

typedef struct
{
	BrushGroupItem *group;
	BrushItem *item;
	int w,		//アイテムの場合、アイテムの表示幅
		x,y,
		/* アイテムの場合、アイテムの左上位置 (ウィジェット座標)。
		 * グループメニューボタンの場合、メニューを表示するルート位置。 */
		header_y;	//グループヘッダ内の場合、ヘッダの y 位置 (ウィジェット座標)
}_POINTINFO;

//---------------------
/** DockBrushListArea */

typedef struct
{
	mWidget wg;

	mScrollBar *scr;

	int itemh,
		headerh,
		fbtt;
	mPoint ptPress;			//D&D 押した時の位置
	_POINTINFO info_dst;	//D&D先の情報
	BrushItem *dnd_src;		//D&D元のアイテム
}DockBrushListArea;

//---------------------
/** DockBrushList */

typedef struct _DockBrushList
{
	mWidget wg;
	mContainerData ct;

	DockBrushListArea *area;
	mScrollBar *scr;
}DockBrushList;

//---------------------

#define _AREA(p)   ((DockBrushListArea *)(p))

#define COL_FRAME        0x999999   //枠の色
#define COL_GROUP_HEAD   0xa7c7fb   //グループヘッダの背景
#define COL_SELECT       0xffdcdc	//選択ブラシの背景色
#define COL_SELECT_FRAME 0xcc0000	//選択ブラシの枠色
#define COL_SEL_TEXT     0x880000	//選択ブラシの文字色
#define COL_REG_FRAME    0x0000ff   //登録ブラシ枠
#define COL_BRUSH_SELBOX 0xfd8d00   //ブラシ右クリック時の選択枠色

#define _GROUP_ARROW_X    3			//グループ展開矢印のX
#define _GROUP_ARROW_SIZE 7			//展開矢印のサイズ
#define _GROUP_NAME_X     15		//グループ名のX位置
#define _GROUP_MENU_PADDING 3		//メニューボタン、右端の余白

#define _ITEMW_MIN   10

enum
{
	_BTT_F_DND_FIRST = 1,
	_BTT_F_DND
};

enum
{
	POINTINFO_NONE,			//なにもない
	POINTINFO_BRUSHITEM,	//ブラシアイテムの上
	POINTINFO_GROUP_HEADER,	//グループのヘッダの上 [!] 以降の値はグループヘッダ部分
	POINTINFO_GROUP_ARROW,	//グループの展開矢印の上
	POINTINFO_GROUP_MENU	//グループのメニューボタンの上
};

enum
{
	//グループメニュー
	TRID_GMENU_NEW_BRUSH = 0,
	TRID_GMENU_NEW_PASTE,
	TRID_GMENU_NEW_GROUP,
	TRID_GMENU_GROUP_OPTION,
	TRID_GMENU_EDIT,
	TRID_GMENU_NEW_COPY,

	//ブラシメニュー
	TRID_BMENU_RENAME = 100,
	TRID_BMENU_COPY,
	TRID_BMENU_PASTE,
	TRID_BMENU_DELETE,
	TRID_BMENU_SET_REG,
	TRID_BMENU_RELEASE_REG,

	//グループ設定ダイアログ
	TRID_GROUPOPTDLG_TITLE_NEW = 200,
	TRID_GROUPOPTDLG_TITLE_OPTION,
	TRID_GROUPOPTDLG_COLNUM,

	//ブラシ名変更
	TRID_BRUSHITEM_RENAME_TITLE,

	//メッセージ
	TRID_MES_TEXT_NOT_BRUSH_DATA = 1000
};

//---------------------

mBool GroupOptionDlg_run(mWindow *owner,int type,mStr *strname,int *colnum);
void BrushListEditDlg_run(mWindow *owner);

//---------------------



//**********************************
// DockBrushListArea
//**********************************


//=============================
// sub
//=============================


/** スクロール情報セット */

static void _area_set_scrollinfo(DockBrushListArea *p)
{
	BrushGroupItem *gi;
	int h = 0,itemh,headerh;

	itemh = p->itemh;
	headerh = p->headerh;

	//全体の高さ

	for(gi = BrushList_getTopGroup(); gi; gi = BRUSHGROUPITEM(gi->i.next))
	{
		h += headerh;

		if(gi->expand)
			h += BrushGroup_getViewVertNum(gi) * itemh;
	}

	mScrollBarSetStatus(p->scr, 0, h, p->wg.h);
}

/** 通知 (値のウィジェットを更新する必要がある場合) */

static void _notify_changevalue(DockBrushListArea *p)
{
	mWidgetAppendEvent_notify(NULL, p->wg.parent, 0, 0, 0);
}

/** 更新 (スクロール情報も再セット) */

static void _update(DockBrushListArea *p)
{
	_area_set_scrollinfo(p);
	
	mWidgetUpdate(M_WIDGET(p));
}

/** ポインタ位置から情報取得 */

static int _get_pointinfo(DockBrushListArea *p,int px,int py,_POINTINFO *info)
{
	BrushGroupItem *gi;
	BrushItem *item;
	mPoint pt;
	int n,w,y,eachw,colnum,xno,yno,itemh,headerh,menu_left,menu_size;

	//ドラッグ中にカーソルがウィジェット外の場合は範囲外

	if(p->fbtt && (px < 0 || px >= p->wg.w || py < 0 || py >= p->wg.h))
		return POINTINFO_NONE;

	//

	info->group = NULL;
	info->item  = NULL;

	w = p->wg.w;
	y = -((p->scr)->sb.pos);	//先頭グループの y 位置

	itemh = p->itemh;
	headerh = p->headerh;

	menu_size = headerh - 3;
	menu_left = w - menu_size - _GROUP_MENU_PADDING;

	//各グループ判定

	for(gi = BrushList_getTopGroup(); gi && y < p->wg.h; gi = BRUSHGROUPITEM(gi->i.next))
	{
		//----- グループのヘッダ内

		if(y <= py && py < y + headerh)
		{
			info->group = gi;
			info->header_y = y;

			py -= y;
		
			if(px < _GROUP_ARROW_X + _GROUP_ARROW_SIZE)
				//展開矢印
				return POINTINFO_GROUP_ARROW;
			else if(menu_left <= px && px < menu_left + menu_size
				&& 1 <= py && py < 1 + menu_size)
			{
				//メニューボタン (x,y はメニューのルート位置)

				pt.x = menu_left;
				pt.y = y + 1 + menu_size;

				mWidgetMapPoint(M_WIDGET(p), NULL, &pt);

				info->x = pt.x;
				info->y = pt.y;
				
				return POINTINFO_GROUP_MENU;
			}
			else
				//他部分
				return POINTINFO_GROUP_HEADER;
		}

		//

		y += headerh;

		//グループが閉じている

		if(!gi->expand) continue;

		//py がこのグループのアイテム全体の範囲内にあるか

		n = BrushGroup_getViewVertNum(gi) * itemh;

		if(py < y || py >= y + n)
		{
			y += n;
			continue;
		}

		//------ アイテム判定

		colnum = gi->colnum;

		//1つの幅

		eachw = w / colnum;
		if(eachw < _ITEMW_MIN) eachw = _ITEMW_MIN;

		//アイテム取得

		xno = px / eachw;
		yno = (py - y) / itemh;
		if(xno >= colnum) xno = colnum - 1;

		item = BrushGroup_getItemAtIndex(gi, yno * colnum + xno);

		if(!item)
			return POINTINFO_NONE;
		else
		{
			//セット
		
			info->item = item;
			info->x = xno * eachw;
			info->y = y + yno * itemh;
			info->w = (xno == colnum - 1)? w - 1 - eachw * (colnum - 1): eachw;

			return POINTINFO_BRUSHITEM;
		}
	}

	return POINTINFO_NONE;
}


//=============================
// 描画
//=============================


/** D&D 先描画 (sub) */

static void _draw_dnd_frame_sub(DockBrushListArea *p,mPixbuf *pixbuf,_POINTINFO *info,mBool erase)
{
	if(info->group)
	{
		//グループヘッダ

		mPixbufLineH(pixbuf, 0, info->header_y + p->headerh - 1, p->wg.w,
			(erase)? mRGBtoPix(COL_FRAME): mRGBtoPix(0xff0000));

		mWidgetUpdateBox_d(M_WIDGET(p), 0, info->header_y + p->headerh - 1, p->wg.w, 1);
	}
	else
	{
		//アイテム

		mPixbufLineV(pixbuf, info->x, info->y - 1, p->itemh + 1,
			(erase)? mRGBtoPix(COL_FRAME): mRGBtoPix(0xff0000));

		mWidgetUpdateBox_d(M_WIDGET(p), info->x, info->y - 1, 1, p->itemh + 1);
	}
}

/** D&D 先描画 (直接描画) */

static void _draw_dnd_frame(DockBrushListArea *p,_POINTINFO *info)
{
	mPixbuf *pixbuf;

	pixbuf = mWidgetBeginDirectDraw(M_WIDGET(p));
	if(pixbuf)
	{
		//消去
		_draw_dnd_frame_sub(p, pixbuf, &p->info_dst, TRUE);

		//新規
		_draw_dnd_frame_sub(p, pixbuf, info, FALSE);
	
		mWidgetEndDirectDraw(M_WIDGET(p), pixbuf);
	}
}

/** ブラシアイテム選択枠描画 (直接描画) */

static void _draw_brush_sel(DockBrushListArea *p,int x,int y,int w,mBool erase)
{
	mPixbuf *pixbuf;

	pixbuf = mWidgetBeginDirectDraw(M_WIDGET(p));
	if(pixbuf)
	{
		mPixbufBox(pixbuf, x, y - 1, w + 1, p->itemh + 1,
			mRGBtoPix(erase? COL_FRAME: COL_BRUSH_SELBOX));
	
		mWidgetEndDirectDraw(M_WIDGET(p), pixbuf);

		mWidgetUpdateBox_d(M_WIDGET(p), x, y - 1, w + 1, p->itemh + 1);
	}
}

/** グループヘッダ描画 */

static void _draw_group_header(DockBrushListArea *p,mPixbuf *pixbuf,
	BrushGroupItem *gi,int y,int w,mFont *font)
{
	int h,x,msize;
	const uint8_t pat_arrow_close[] = {0x01,0x03,0x07,0x0f,0x07,0x03,0x01},
		pat_arrow_expand[] = {0x7f,0x3e,0x1c,0x08},
		pat_menu_arrow[] = { 0x1f,0x0e,0x04 };

	h = p->headerh - 1;
	msize = h - 2;

	//背景

	mPixbufFillBox(pixbuf, 0, y, w, h, mRGBtoPix(COL_GROUP_HEAD));

	//枠

	mPixbufLineH(pixbuf, 0, y + h, w, mRGBtoPix(COL_FRAME));

	//展開矢印

	if(gi->expand)
	{
		mPixbufDrawBitPattern(pixbuf,
			_GROUP_ARROW_X, y + (h - 4) / 2,
			pat_arrow_expand, 7, 4, 0);
	}
	else
	{
		mPixbufDrawBitPattern(pixbuf,
			_GROUP_ARROW_X + 3, y + (h - 7) / 2,
			pat_arrow_close, 4, 7, 0);
	}

	//名前

	x = w - _GROUP_NAME_X - 3 - _GROUP_MENU_PADDING - msize;

	if(mPixbufSetClipBox_d(pixbuf, _GROUP_NAME_X, y, x, h))
		mFontDrawText(font, pixbuf, _GROUP_NAME_X, y + 1, gi->name, -1, 0);

	mPixbufClipNone(pixbuf);

	//メニューボタン

	x = w - msize - _GROUP_MENU_PADDING;

	mPixbufBox(pixbuf, x, y + 1, msize, msize, 0);
	mPixbufDrawBitPattern(pixbuf, x + msize / 2 - 2, y + 1 + msize / 2 - 1,
		pat_menu_arrow, 5, 3, 0);
}

/** 描画 */

static void _area_draw(mWidget *wg,mPixbuf *pixbuf)
{
	DockBrushListArea *p = _AREA(wg);
	BrushGroupItem *gi;
	BrushItem *item,*selitem,*regitem;
	mFont *font;
	int x,y,hcnt,eachw,lastw,itemw,itemh,headerh;
	mBool select;
	mPixCol col_frame;

	font = mWidgetGetFont(wg);
	itemh = p->itemh;
	headerh = p->headerh;

	selitem = BrushList_getSelItem();
	regitem = BrushList_getRegisteredItem();

	col_frame = mRGBtoPix(COL_FRAME);

	//背景

	mPixbufFillBox(pixbuf, 0, 0, wg->w, wg->h, MSYSCOL(FACE_DARKER));

	//グループ

	y = -((p->scr)->sb.pos);

	for(gi = BrushList_getTopGroup(); gi && y < wg->h; gi = BRUSHGROUPITEM(gi->i.next))
	{
		//グループヘッダ

		if(y > -headerh)
			_draw_group_header(p, pixbuf, gi, y, wg->w, font);

		//

		y += headerh;

		if(!gi->expand) continue;

		//アイテム全体が隠れている場合、位置を進める

		hcnt = BrushGroup_getViewVertNum(gi) * itemh;

		if(y + hcnt <= 0)
		{
			y += hcnt;
			continue;
		}

		//------- 各アイテム

		x = hcnt = 0;

		//1つの幅

		eachw = wg->w / gi->colnum;
		if(eachw < _ITEMW_MIN) eachw = _ITEMW_MIN;

		//右端のアイテムの幅 (右に 1px 余白を作る)

		lastw = wg->w - 1 - eachw * (gi->colnum - 1);

		//

		for(item = BrushGroup_getTopItem(gi); item && y < wg->h; item = BRUSHITEM(item->i.next))
		{
			itemw = (hcnt == gi->colnum - 1)? lastw: eachw;
			select = (item == selitem);

			//枠

			mPixbufBox(pixbuf, x, y, itemw + 1, itemh, col_frame);
			
			//背景

			mPixbufFillBox(pixbuf, x + 1, y, itemw - 1, itemh - 1,
				(select)? mRGBtoPix(COL_SELECT): MSYSCOL(WHITE));

			//選択枠

			if(select)
				mPixbufBox(pixbuf, x + 1, y, itemw - 1, itemh - 1, mRGBtoPix(COL_SELECT_FRAME));

			//登録ブラシ枠

			if(item == regitem)
				mPixbufBox(pixbuf, x + 2, y + 1, itemw - 3, itemh - 3, mRGBtoPix(COL_REG_FRAME));

			//名前

			if(mPixbufSetClipBox_d(pixbuf, x + 3, y, itemw - 5, itemh))
			{
				mFontDrawText(font, pixbuf, x + 3, y + 3, item->name, -1,
					(select)? COL_SEL_TEXT: 0);
			}

			mPixbufClipNone(pixbuf);
		
			//次の行

			hcnt++;
			x += itemw;
			
			if(hcnt >= gi->colnum)
			{
				hcnt = x = 0;
				y += itemh;
			}
		}

		//次のグループへ
		//(hcnt == 0 ならすでに折り返し済み)

		if(hcnt)
			y += itemh;
	}
}


//=============================
// コマンド
//=============================


/** 新規グループ or グループの設定
 *
 * @param group  NULL で新規グループ */

static void _cmd_newgroup_groupoption(DockBrushListArea *p,BrushGroupItem *group)
{
	mStr str = MSTR_INIT;
	int colnum;

	if(group)
	{
		mStrSetText(&str, group->name);
		colnum = group->colnum;
	}

	if(GroupOptionDlg_run(p->wg.toplevel, (group != 0), &str, &colnum))
	{
		if(!group)
			//新規
			BrushList_newGroup(str.buf, colnum);
		else
		{
			//変更
			mStrdup_ptr(&group->name, str.buf);
			group->colnum = colnum;
		}

		_update(p);
	}

	mStrFree(&str);
}

/** ブラシ名変更 */

static void _cmd_brush_rename(DockBrushListArea *p,BrushItem *item)
{
	mStr str = MSTR_INIT;

	mStrSetText(&str, item->name);

	if(mSysDlgInputText(p->wg.toplevel,
		M_TR_T2(TRGROUP_DOCK_BRUSH_LIST, TRID_BRUSHITEM_RENAME_TITLE),
		M_TR_T2(TRGROUP_WORD, TRID_WORD_NAME),
		&str, 0))
	{
		//選択ブラシの場合はパラメータ部分を更新
	
		if(BrushList_setBrushName(item, str.buf))
			_notify_changevalue(p);
	
		mWidgetUpdate(M_WIDGET(p));
	}

	mStrFree(&str);
}

/** ブラシコピー */

static void _cmd_brush_copy(DockBrushListArea *p,BrushItem *item)
{
	mStr str = MSTR_INIT;

	BrushItem_getTextFormat(item, &str);

	mClipboardSetText(M_WIDGET(p), str.buf, str.len + 1);

	mStrFree(&str);
}

/** 貼り付け
 *
 * @param group  NULL 以外で、新規貼り付け
 * @param item   NULL 以外で、上書き貼り付け */

static void _paste_brush(DockBrushListArea *p,BrushGroupItem *group,BrushItem *item)
{
	char *buf = mClipboardGetText(M_WIDGET(p));

	if(buf)
	{
		if(strncmp(buf, "azpainter-v2-brush;", 19))
			mMessageBoxErr(p->wg.toplevel, M_TR_T2(TRGROUP_DOCK_BRUSH_LIST, TRID_MES_TEXT_NOT_BRUSH_DATA));
		else
		{
			if(BrushList_pasteItem_text(group, item, buf))
			{
				_update(p);
				_notify_changevalue(p);
			}
		}
		
		mFree(buf);
	}
}

/** リスト編集 */

static void _cmd_brushlist_edit(DockBrushListArea *p)
{
	BrushListEditDlg_run(p->wg.toplevel);

	BrushList_afterBrushListEdit();

	//更新

	_update(p);
	_notify_changevalue(p);
}


//=============================
// メニュー
//=============================


/** グループメニュー */

static void _run_group_menu(DockBrushListArea *p,int x,int y,BrushGroupItem *group)
{
	mMenu *menu;
	mMenuItemInfo *pi;
	BrushItem *item;
	int id;
	uint16_t menuid[] = {
		TRID_GMENU_NEW_BRUSH, TRID_GMENU_NEW_COPY, TRID_GMENU_NEW_PASTE,
		TRID_GMENU_NEW_GROUP, 0xfffe,
		TRID_GMENU_GROUP_OPTION, 0xfffe, TRID_GMENU_EDIT,
		0xffff
	};

	M_TR_G(TRGROUP_DOCK_BRUSH_LIST);

	//----- メニュー

	menu = mMenuNew();

	mMenuAddTrArray16(menu, menuid);

	if(!BrushList_getSelItem())
		mMenuSetEnable(menu, TRID_GMENU_NEW_COPY, FALSE);

	pi = mMenuPopup(menu, NULL, x, y, 0);
	id = (pi)? pi->id: -1;

	mMenuDestroy(menu);

	if(id == -1) return;

	//------ コマンド

	switch(id)
	{
		//新規ブラシ
		case TRID_GMENU_NEW_BRUSH:
		case TRID_GMENU_NEW_COPY:
			item = BrushList_newBrush(group);

			if(id == TRID_GMENU_NEW_COPY)
				BrushItem_copy(item, BrushList_getEditItem());

			BrushList_setSelItem(item);

			_update(p);
			_notify_changevalue(p);
			break;
		//ブラシ貼り付け
		case TRID_GMENU_NEW_PASTE:
			_paste_brush(p, group, NULL);
			break;
		//グループの設定
		case TRID_GMENU_GROUP_OPTION:
			_cmd_newgroup_groupoption(p, group);
			break;
		//新規グループ
		case TRID_GMENU_NEW_GROUP:
			_cmd_newgroup_groupoption(p, NULL);
			break;
		//編集
		case TRID_GMENU_EDIT:
			_cmd_brushlist_edit(p);
			break;
	}
}

/** ブラシメニュー */

static void _run_brush_menu(DockBrushListArea *p,int x,int y,_POINTINFO *info)
{
	mMenu *menu;
	mMenuItemInfo *pi;
	int id;
	uint16_t menuid[] = {
		TRID_BMENU_RENAME, 0xfffe,
		TRID_BMENU_COPY, TRID_BMENU_PASTE, 0xfffe,
		TRID_BMENU_DELETE, 0xfffe,
		TRID_BMENU_SET_REG, TRID_BMENU_RELEASE_REG, 0xffff
	};

	//選択枠

	_draw_brush_sel(p, info->x, info->y, info->w, FALSE);

	//----- メニュー

	M_TR_G(TRGROUP_DOCK_BRUSH_LIST);

	menu = mMenuNew();

	mMenuAddTrArray16(menu, menuid);

	if(!BrushList_getRegisteredItem())
		mMenuSetEnable(menu, TRID_BMENU_RELEASE_REG, FALSE);

	pi = mMenuPopup(menu, NULL, x, y, 0);
	id = (pi)? pi->id: -1;

	mMenuDestroy(menu);

	//

	_draw_brush_sel(p, info->x, info->y, info->w, TRUE);

	if(id == -1) return;

	//------ コマンド

	switch(id)
	{
		//名前の変更
		case TRID_BMENU_RENAME:
			_cmd_brush_rename(p, info->item);
			break;
		//コピー
		case TRID_BMENU_COPY:
			_cmd_brush_copy(p, info->item);
			break;
		//貼り付け
		case TRID_BMENU_PASTE:
			_paste_brush(p, NULL, info->item);
			break;
		//削除
		case TRID_BMENU_DELETE:
			if(BrushList_deleteItem(info->item))
				_notify_changevalue(p);

			_update(p);
			break;
		//登録ブラシ指定
		case TRID_BMENU_SET_REG:
			BrushList_setRegisteredItem(info->item);
			mWidgetUpdate(M_WIDGET(p));
			break;
		//登録ブラシ解除
		case TRID_BMENU_RELEASE_REG:
			BrushList_setRegisteredItem(NULL);
			mWidgetUpdate(M_WIDGET(p));
			break;
	}
}


//=============================
// イベント
//=============================


/** ボタン押し */

static void _event_press(DockBrushListArea *p,mEvent *ev)
{
	_POINTINFO info;
	int area;

	area = _get_pointinfo(p, ev->pt.x, ev->pt.y, &info);

	if(ev->pt.btt == M_BTT_RIGHT)
	{
		//------ 右クリック

		if(area == POINTINFO_BRUSHITEM)
			//ブラシ
			_run_brush_menu(p, ev->pt.rootx, ev->pt.rooty, &info);
		else if(area >= POINTINFO_GROUP_HEADER)
			//グループヘッダ内
			_run_group_menu(p, ev->pt.rootx, ev->pt.rooty, info.group);
	}
	else if(ev->pt.btt == M_BTT_LEFT)
	{
		//------- 左クリック

		switch(area)
		{
			//ブラシ
			case POINTINFO_BRUSHITEM:
				if(ev->pt.type == MEVENT_POINTER_TYPE_DBLCLK)
				{
					//ダブルクリック => 名前変更

					_cmd_brush_rename(p, info.item);
				}
				else
				{
					//選択変更
					
					if(BrushList_setSelItem(info.item))
					{
						mWidgetUpdate(M_WIDGET(p));
						_notify_changevalue(p);

						//ツールがブラシでないなら変更

						drawTool_setTool(APP_DRAW, TOOL_BRUSH);
					}

					//D&D 開始

					p->fbtt = _BTT_F_DND_FIRST;
					p->ptPress.x = ev->pt.x;
					p->ptPress.y = ev->pt.y;
					p->dnd_src = info.item;
					p->info_dst = info;

					mWidgetGrabPointer(M_WIDGET(p));
				}
				break;
			//グループ展開 (矢印部分)
			case POINTINFO_GROUP_ARROW:
				info.group->expand ^= 1;
				_update(p);
				break;
			//グループメニュー
			case POINTINFO_GROUP_MENU:
				_run_group_menu(p, info.x, info.y, info.group);
				break;
		}
	}
}

/** ポインタ移動 */

static void _event_motion(DockBrushListArea *p,mEvent *ev)
{
	int area;
	_POINTINFO info;

	//押し位置から少し離れた場合、D&D 開始

	if(p->fbtt == _BTT_F_DND_FIRST
		&& (abs(ev->pt.x - p->ptPress.x) > 10 || abs(ev->pt.y - p->ptPress.y) > 10))
	{
		p->fbtt = _BTT_F_DND;

		mWidgetSetCursor(M_WIDGET(p), AppCursor_getForDrag(APP_CURSOR_ITEM_MOVE));
	}

	//D&D 中

	if(p->fbtt == _BTT_F_DND)
	{
		area = _get_pointinfo(p, ev->pt.x, ev->pt.y, &info);

		//移動先変更

		if((area == POINTINFO_BRUSHITEM && info.item != p->info_dst.item)
			|| (area >= POINTINFO_GROUP_HEADER && info.group != p->info_dst.group))
		{
			//挿入線

			_draw_dnd_frame(p, &info);

			p->info_dst = info;
		}
	}
}

/** グラブ解放 */

static void _release_grab(DockBrushListArea *p)
{
	if(p->fbtt)
	{
		//D&D 実行

		if(p->fbtt == _BTT_F_DND)
		{
			BrushList_moveItem(p->dnd_src, p->info_dst.group, p->info_dst.item);
		
			mWidgetSetCursor(M_WIDGET(p), 0);

			_update(p);
		}

		//

		p->fbtt = 0;
		mWidgetUngrabPointer(M_WIDGET(p));
	}
}

/** イベント */

static int _area_event_handle(mWidget *wg,mEvent *ev)
{
	DockBrushListArea *p = _AREA(wg);
	
	switch(ev->type)
	{
		case MEVENT_POINTER:
			if(ev->pt.type == MEVENT_POINTER_TYPE_MOTION)
			{
				if(p->fbtt)
					_event_motion(p, ev);
			}
			else if(ev->pt.type == MEVENT_POINTER_TYPE_PRESS
				|| ev->pt.type == MEVENT_POINTER_TYPE_DBLCLK)
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

		//ホイール
		case MEVENT_SCROLL:
			if(ev->scr.dir == MEVENT_SCROLL_DIR_UP || ev->scr.dir == MEVENT_SCROLL_DIR_DOWN)
			{
				if(mScrollBarMovePos(p->scr, (ev->scr.dir == MEVENT_SCROLL_DIR_UP)? -(p->itemh): p->itemh))
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


//=============================


/** サイズ変更時 */

static void _area_onsize_handle(mWidget *wg)
{
	//高さ保存

	APP_CONF->dockbrush_height[1] = wg->h;

	//スクロール

	_area_set_scrollinfo(_AREA(wg));
}

/** 作成 */

static DockBrushListArea *_area_new(mWidget *parent)
{
	DockBrushListArea *p;

	p = (DockBrushListArea *)mWidgetNew(sizeof(DockBrushListArea), parent);

	p->wg.fLayout = MLF_EXPAND_WH;
	p->wg.event = _area_event_handle;
	p->wg.draw = _area_draw;
	p->wg.onSize = _area_onsize_handle;
	p->wg.fEventFilter |= MWIDGET_EVENTFILTER_POINTER | MWIDGET_EVENTFILTER_SCROLL;

	return p;
}


//**********************************
// DockBrushList
//**********************************


/** 描画 */

static void _draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	//枠描画
	mPixbufBox(pixbuf, 0, 0, wg->w, wg->h, MSYSCOL(FRAME));
}

/** スクロールハンドル */

static void _scroll_handle(mScrollBar *p,int pos,int flags)
{
	if(flags & MSCROLLBAR_N_HANDLE_F_CHANGE)
		mWidgetUpdate(M_WIDGET(p->wg.param));
}

/** 作成 */

DockBrushList *DockBrushList_new(mWidget *parent,mFont *font)
{
	DockBrushList *p;
	int n;

	p = (DockBrushList *)mContainerNew(sizeof(DockBrushList), parent);

	mContainerSetType(M_CONTAINER(p), MCONTAINER_TYPE_HORZ, 0);
	mContainerSetPadding_one(M_CONTAINER(p), 1);

	p->wg.h = APP_CONF->dockbrush_height[1] + 2;
	p->wg.fLayout = MLF_EXPAND_W | MLF_FIX_H;
	p->wg.draw = _draw_handle;

	//エリア

	p->area = _area_new(M_WIDGET(p));

	p->area->wg.font = font;
	p->area->itemh = font->height + 8;

	n = font->height + 3;	//fonth + padding(2px) + bottom(1px)
	if(n < 10) n = 10;		//矢印ボタンを含めた最小サイズ
	p->area->headerh = n;

	//スクロールバー

	p->scr = mScrollBarNew(0, M_WIDGET(p), MSCROLLBAR_S_VERT);

	p->scr->wg.fLayout = MLF_EXPAND_H;
	p->scr->wg.param = (intptr_t)p->area;
	p->scr->sb.handle = _scroll_handle;

	//

	p->area->scr = p->scr;

	return p;
}

/** 更新 */

void DockBrushList_update(DockBrushList *p)
{
	mWidgetUpdate(M_WIDGET(p->area));
}



//**********************************
// グループ新規/設定ダイアログ
//**********************************


/** ダイアログ実行
 *
 * @param type [0]新規グループ [1]グループ設定(変更) */

mBool GroupOptionDlg_run(mWindow *owner,int type,mStr *strname,int *colnum)
{
	mDialog *p;
	mBool ret;
	mWidget *ct;
	mLineEdit *pname,*pcol;

	p = (mDialog *)mDialogNew(0, owner, MWINDOW_S_DIALOG_NORMAL);
	if(!p) return FALSE;

	p->wg.event = mDialogEventHandle_okcancel;

	//

	M_TR_G(TRGROUP_DOCK_BRUSH_LIST);

	mWindowSetTitle(M_WINDOW(p),
		M_TR_T(type == 0? TRID_GROUPOPTDLG_TITLE_NEW: TRID_GROUPOPTDLG_TITLE_OPTION));

	mContainerSetPadding_one(M_CONTAINER(p), 8);

	p->ct.sepW = 15;

	//----- ウィジェット

	ct = mContainerCreateGrid(M_WIDGET(p), 2, 7, 7, MLF_EXPAND_W);

	//名前

	mLabelCreate(ct, 0, MLF_RIGHT | MLF_MIDDLE, 0, M_TR_T2(TRGROUP_WORD, TRID_WORD_NAME));

	pname = mLineEditCreate(ct, 0, 0, MLF_EXPAND_W, 0);
	pname->wg.initW = mWidgetGetFontHeight(M_WIDGET(pname)) * 14;

	mLineEditSetText(pname, (type == 0)? "group": strname->buf);

	mWidgetSetFocus(M_WIDGET(pname));
	mLineEditSelectAll(pname);

	//横に並べる数

	mLabelCreate(ct, 0, MLF_RIGHT | MLF_MIDDLE, 0, M_TR_T(TRID_GROUPOPTDLG_COLNUM));

	pcol = mLineEditCreate(ct, 0, MLINEEDIT_S_SPIN, 0, 0);

	mLineEditSetWidthByLen(pcol, 4);
	mLineEditSetNumStatus(pcol, 1, 10, 0);

	mLineEditSetNum(pcol, (type == 0)? 2: *colnum);

	//OK/cancel

	mContainerCreateOkCancelButton(M_WIDGET(p));

	//----- 実行

	mWindowMoveResizeShow_hintSize(M_WINDOW(p));

	ret = mDialogRun(p, FALSE);

	if(ret)
	{
		mLineEditGetTextStr(pname, strname);

		*colnum = mLineEditGetNum(pcol);
	}

	mWidgetDestroy(M_WIDGET(p));

	return ret;
}
