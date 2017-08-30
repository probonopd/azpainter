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
 * mMenuBar [メニューバー]
 *****************************************/


#include "mDef.h"

#include "mMenuBar.h"
#include "mMenu.h"
#include "mWidget.h"
#include "mFont.h"
#include "mPixbuf.h"
#include "mEvent.h"
#include "mSysCol.h"
#include "mTrans.h"

#include "mMenuWindow.h"
#include "mMenu_pv.h"


//------------------

#define _PADDING_H   6
#define _PADDING_V   4

#define _MENUITEM_TOP(p)  M_MENUITEM((p)->mb.menu->list.top)

static int _event_handle(mWidget *,mEvent *);
static void _draw_handle(mWidget *,mPixbuf *);

//------------------


/***************//**

@defgroup menubar mMenuBar
@brief メニューバー

ウィジェット破棄時は、セットされているメニューデータも削除される。

@ingroup group_widget
@{

@file mMenuBar.h
@def M_MENUBAR(p)
@struct mMenuBar
@struct mMenuBarData
@enum MMENUBAR_ARRAY16

*****************/


//=======================
// sub
//=======================


/** 破棄ハンドラ */

static void _destroy_handle(mWidget *wg)
{
	//メニューデータ
	
	if(M_MENUBAR(wg)->mb.menu)
		mMenuDestroy(M_MENUBAR(wg)->mb.menu);

	//トップレベルとの関連付けをクリア
	
	(wg->toplevel)->win.menubar = NULL;
}

/** サイズ計算 */

static void _calchint_handle(mWidget *wg)
{
	mFont *font;
	
	font = mWidgetGetFont(wg);

	wg->hintW = 1;
	wg->hintH = font->height + _PADDING_V * 2;

	//初期化 (ラベルサイズ計算)
	
	if(M_MENUBAR(wg)->mb.menu)
		__mMenuInit(M_MENUBAR(wg)->mb.menu, font);
}

/** 16bit 配列値からメニュー項目作成
 *
 * @param radio  TRUE で 0x4000 はラジオ */

static void _setmenu_array16(mMenuBar *bar,const uint16_t *buf,int idtop,mBool radio)
{
	int val,id;
	uint32_t flags;
	mMenu *menutop,*menucur = NULL,*menuparent = NULL;
	mMenuItemInfo *pi;

	if(bar->mb.menu) return;

	//メニューバーのメニュー

	menutop = mMenuNew();
	if(!menutop) return;

	bar->mb.menu = menutop;

	//項目追加

	while(*buf != MMENUBAR_ARRAY16_END)
	{
		val = *(buf++);

		if(val < idtop)
		{
			//メニューバーの項目

			menucur = mMenuNew();

			mMenuAddSubmenu(menutop, val, M_TR_T(val), menucur);
		}
		else if(val == MMENUBAR_ARRAY16_SUBMENU)
		{
			//サブメニュー開始/終了

			if(menucur)
			{
				if(menuparent)
				{
					//親に戻る

					menucur = menuparent;
					menuparent = NULL;
				}
				else
				{
					//開始

					pi = mMenuGetLastItemInfo(menucur);

					if(pi)
					{
						menuparent = menucur;
						menucur = mMenuNew();

						pi->submenu = menucur;
					}
				}
			}
		}
		else if(menucur)
		{
			//項目

			if(val == MMENUBAR_ARRAY16_SEP)
				mMenuAddSep(menucur);
			else
			{
				//ID,フラグ

				flags = 0;
				id = val & 0x7fff;

				if(val & MMENUBAR_ARRAY16_AUTOCHECK)
					flags |= MMENUITEM_F_AUTOCHECK;

				if(radio)
				{
					if(val & MMENUBAR_ARRAY16_RADIO)
						flags |= MMENUITEM_F_RADIO;

					id &= 0x3fff;
				}
			
				//
			
				pi = mMenuAddText_static(menucur, id, M_TR_T(id));

				if(pi) pi->flags = flags;
			}
		}
	}
}


//=======================
// main
//=======================


/** メニューバー作成
 * 
 * 作成と同時にトップウィンドウに関連付けられる。 @n
 * MLF_EXPAND_W はデフォルト。 */

mMenuBar *mMenuBarNew(int size,mWidget *parent,uint32_t style)
{
	mMenuBar *p;
	
	if(size < sizeof(mMenuBar)) size = sizeof(mMenuBar);
	
	p = (mMenuBar *)mWidgetNew(size, parent);
	if(!p) return NULL;
	
	p->wg.destroy = _destroy_handle;
	p->wg.calcHint = _calchint_handle;
	p->wg.draw = _draw_handle;
	p->wg.event = _event_handle;
	
	p->wg.fEventFilter |= MWIDGET_EVENTFILTER_POINTER;
	p->wg.fLayout |= MLF_EXPAND_W;
	
	//トップウィンドウと関連付け
	
	(p->wg.toplevel)->win.menubar = p;
	
	return p;
}

/** メニューデータ取得 */

mMenu *mMenuBarGetMenu(mMenuBar *bar)
{
	return bar->mb.menu;
}

/** メニューデータをセット
 *
 * メニューバーの破棄時は、セットされたデータも破棄される。 */

void mMenuBarSetMenu(mMenuBar *bar,mMenu *menu)
{
	bar->mb.menu = menu;
}

/** 指定IDのアイテムにサブメニューをセット */

void mMenuBarSetItemSubmenu(mMenuBar *bar,int id,mMenu *submenu)
{
	mMenuSetSubmenu(bar->mb.menu, id, submenu);
}

/** 文字列 ID などを定義したデータからメニューを作成
 *
 * サブメニューは1階層のみ。
 *
 * @param idtop この値未満の場合はメニューバーのトップ項目とする */

void mMenuBarCreateMenuTrArray16(mMenuBar *bar,const uint16_t *buf,int idtop)
{
	_setmenu_array16(bar, buf, idtop, FALSE);
}

/** 文字列 ID などを定義したデータからメニューを作成
 *
 * 0x4000 はラジオにする。 */

void mMenuBarCreateMenuTrArray16_radio(mMenuBar *bar,const uint16_t *buf,int idtop)
{
	_setmenu_array16(bar, buf, idtop, TRUE);
}


//=======================
// sub
//=======================


/** カーソル位置からアイテム取得 */

static mMenuItem *_getItemByPos(mMenuBar *mbar,int x)
{
	mMenuItem *pi;
	int xx = 0;
	
	for(pi = _MENUITEM_TOP(mbar); pi; pi = M_MENUITEM(pi->i.next))
	{
		if(xx > x) break;
		if(xx <= x && x < xx + pi->labelLeftW + _PADDING_H * 2)
			return pi;
		
		xx += pi->labelLeftW + _PADDING_H * 2;
	}
	
	return NULL;
}

/** アイテムのサブメニュー表示位置取得 (ルート座標) */

static void _getItemSubmenuPos(mMenuBar *mbar,mMenuItem *item,mPoint *pt)
{
	mMenuItem *pi;
	int x = 0;

	for(pi = _MENUITEM_TOP(mbar); pi && pi != item; pi = M_MENUITEM(pi->i.next))
		x += pi->labelLeftW + _PADDING_H * 2;
	
	pt->x = x;
	pt->y = mWidgetGetFontHeight(M_WIDGET(mbar)) + _PADDING_V * 2;
	
	mWidgetMapPoint(M_WIDGET(mbar), NULL, pt);
}

/** サブメニュー表示 */

static void _showSubmenu(mMenuBar *mbar,mMenuItem *item)
{
	mMenuWindow *menuwin;
	mPoint pt;

	/* メニュー表示中に、カーソル移動によって表示サブメニューを
	 * 変更する場合があるので、繰り返す */

	do
	{
		//カレントアイテム
	
		mbar->mb.itemCur = item;
		mWidgetUpdate(M_WIDGET(mbar));

		//ポップアップ表示
		
		_getItemSubmenuPos(mbar, item, &pt);
		
		menuwin = mMenuWindowNew(item->item.submenu);
		if(!menuwin) break;
		
		item = mMenuWindowShowMenuBar(menuwin, mbar,
					pt.x, pt.y, item->item.id);
		
	}while(item);
	
	//終了
	
	mbar->mb.itemCur = NULL;
	mWidgetUpdate(M_WIDGET(mbar));
}

/** ポップアップ表示中のカーソル移動時
 *
 * (mMenuWindow から呼ばれる)
 * 
 * @return 新たに表示させるアイテム。NULLで変更なし。 */

mMenuItem *__mMenuBar_motion(mMenuBar *mbar,mMenuWindow *topmenu,int x,int y)
{
	mMenuItem *pi;
	mPoint pt;

	//メニューバーの座標に変換
	
	pt.x = x, pt.y = y;
	mWidgetMapPoint(M_WIDGET(topmenu), M_WIDGET(mbar), &pt);

	//アイテム取得
	
	if(mWidgetIsContain(M_WIDGET(mbar), pt.x, pt.y))
	{
		pi = _getItemByPos(mbar, pt.x);
		
		if(pi && pi != mbar->mb.itemCur && __mMenuItemIsEnableSubmenu(pi))
			return pi;
	}
	
	return NULL;
}

/** Alt+ のホットキー押しによる表示 */

mBool __mMenuBar_showhotkey(mMenuBar *mbar,uint32_t key,uint32_t state,int press)
{
	mMenuItem *item;
	
	if((state & M_MODS_MASK_KEY) != M_MODS_ALT) return FALSE;
	
	if(!((key >= '0' && key <= '9') || (key >= 'A' && key <= 'Z')))
		return FALSE;
	
	item = __mMenuFindByHotkey(mbar->mb.menu, (char)key);
	
	if(!item || !__mMenuItemIsEnableSubmenu(item))
		return FALSE;
	
	if(press)
		_showSubmenu(mbar, item);
	
	return TRUE;
}


//=======================
// ハンドラ
//=======================


/** イベント */

int _event_handle(mWidget *wg,mEvent *ev)
{
	switch(ev->type)
	{
		case MEVENT_POINTER:
			//左ボタン押し時
			if(ev->pt.type == MEVENT_POINTER_TYPE_PRESS
				&& ev->pt.btt == M_BTT_LEFT)
			{
				mMenuItem *item;
				
				item = _getItemByPos(M_MENUBAR(wg), ev->pt.x);
				
				if(item && __mMenuItemIsEnableSubmenu(item))
					_showSubmenu(M_MENUBAR(wg), item);
			}
			break;
	}
	
	return 1;
}

/** 描画 */

void _draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	mMenuBar *mbar = M_MENUBAR(wg);
	mFont *font;
	mMenuItem *pi;
	int x;
	uint32_t col;
		
	//背景
	
	mPixbufFillBox(pixbuf, 0, 0, wg->w, wg->h, MSYSCOL(MENU_FACE));
	
	//項目

	if(!mbar->mb.menu) return;
	
	font = mWidgetGetFont(wg);
	x = 0;
	
	for(pi = _MENUITEM_TOP(mbar); pi; pi = M_MENUITEM(pi->i.next))
	{
		if(pi == mbar->mb.itemCur)
		{
			//選択時
			
			mPixbufFillBox(pixbuf, x, 0,
				pi->labelLeftW + _PADDING_H * 2, font->height + _PADDING_V * 2,
				MSYSCOL(MENU_SELECT));
			
			col = MSYSCOL_RGB(MENU_TEXT);
		}
		else
		{
			//通常時
			
			col = (pi->item.flags & MMENUITEM_F_DISABLE)
					? MSYSCOL_RGB(MENU_TEXT_DISABLE): MSYSCOL_RGB(MENU_TEXT);
		}
		
		//ラベル
		
		mFontDrawTextHotkey(font, pixbuf, x + _PADDING_H, _PADDING_V,
			pi->labelsrc, -1, col);
		
		
		x += pi->labelLeftW + _PADDING_H * 2;
	}
}

/** @} */
