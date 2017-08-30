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
 * SelMaterial
 *
 * 素材画像の選択バーウィジェット
 *****************************************/
/*
 * - 選択が変更された時、NOTIFY イベントが送られる。
 *   param1 = パス名 (char *)。なしの場合 NULL
 */


#include <string.h>

#include "mDef.h"
#include "mWidgetDef.h"
#include "mWidget.h"
#include "mPixbuf.h"
#include "mSysCol.h"
#include "mFont.h"
#include "mEvent.h"
#include "mStr.h"
#include "mTrans.h"
#include "mMenu.h"

#include "SelMaterial.h"

#include "trgroup.h"


//----------------------

struct _SelMaterial
{
	mWidget wg;

	int type;
	mStr strText,	//表示用テキスト
		strPath;	//パス名
};

//----------------------

#define SELMATERIAL(p)  ((SelMaterial *)(p))

enum
{
	TRID_NONE,
	TRID_NONE_FORCE,
	TRID_OPT_TEXTURE,
	TRID_SHAPE_NORMAL,

	TRID_SELECT = 100,
	TRID_SELECT_TEXTURE
};

mBool SelMaterialDlg_run(mWindow *owner,mStr *strdst,mBool bTexture);

//----------------------


/** 通知 */

static void _notify(SelMaterial *p)
{
	mWidgetAppendEvent_notify(NULL, M_WIDGET(p),
		0, (intptr_t)(mStrIsEmpty(&p->strPath)? 0: p->strPath.buf), 0);
}

/** ダイアログ実行 */

static void _run_dialog(SelMaterial *p)
{
	mStr str = MSTR_INIT;

	mStrCopy(&str, &p->strPath);

	if(SelMaterialDlg_run(p->wg.toplevel, &str, (p->type != SELMATERIAL_TYPE_BRUSH_SHAPE)))
	{
		mStrCopy(&p->strText, &str);
		mStrCopy(&p->strPath, &str);

		mWidgetUpdate(M_WIDGET(p));

		_notify(p);
	}

	mStrFree(&str);
}

/** メニュー実行 */

static void _run_menu(SelMaterial *p)
{
	mMenu *menu;
	mMenuItemInfo *pi;
	mBox box;
	int id;
	uint16_t trid[3][4] = {
		{TRID_NONE, TRID_SELECT, 0xffff, 0},
		{TRID_OPT_TEXTURE, TRID_NONE_FORCE, TRID_SELECT, 0xffff},
		{TRID_SHAPE_NORMAL, TRID_SELECT, 0xffff, 0}
	};

	//メニュー

	M_TR_G(TRGROUP_SELMATERIAL);

	menu = mMenuNew();

	if(p->type == SELMATERIAL_TYPE_TEXTURE)
	{
		//レイヤテクスチャの場合

		trid[SELMATERIAL_TYPE_TEXTURE][1] = TRID_SELECT_TEXTURE;
	}

	mMenuAddTrArray16(menu, trid[p->type]);

	mWidgetGetRootBox(M_WIDGET(p), &box);

	pi = mMenuPopup(menu, NULL, box.x + box.w, box.y + box.h, MMENU_POPUP_F_RIGHT);

	id = (pi)? pi->id: -1;

	mMenuDestroy(menu);

	//セット

	if(id == -1) return;

	if(id == TRID_SELECT || id == TRID_SELECT_TEXTURE)
		_run_dialog(p);
	else
	{
		//「画像選択」以外
		/* なし => NULL
		 * オプションテクスチャを使う => "?" */
		
		SelMaterial_setName(p, (id == TRID_OPT_TEXTURE)? "?": NULL);

		_notify(p);
	}
}

/** イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_POINTER
		&& ev->pt.type == MEVENT_POINTER_TYPE_PRESS)
	{
		SelMaterial *p = SELMATERIAL(wg);
	
		//------ 押し時
	
		if(ev->pt.btt == M_BTT_RIGHT
			|| (ev->pt.btt == M_BTT_LEFT && (ev->pt.state & M_MODS_CTRL)))
		{
			//右ボタン/Ctrl+左でリセット

			if(!mStrIsEmpty(&p->strPath))
			{
				SelMaterial_setName(p, NULL);

				_notify(p);
			}
		}
		else if(ev->pt.btt == M_BTT_LEFT)
		{
			//左ボタン

			if(ev->pt.x >= wg->w - 2 - (wg->h - 4))
				_run_menu(p);
			else
				_run_dialog(p);
		}
	}
	
	return 1;
}

/** 描画 */

static void _draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	int bw;
	mBool enable;

	enable = ((wg->fState & MWIDGET_STATE_ENABLED) != 0);

	bw = wg->h - 4;

	//枠

	mPixbufBox(pixbuf, 0, 0, wg->w, wg->h,
		(enable)? MSYSCOL(FRAME_DARK): MSYSCOL(FRAME));

	//背景

	mPixbufFillBox(pixbuf, 1, 1, wg->w - 2, wg->h - 2,
		(enable)? MSYSCOL(FACE_LIGHTEST): MSYSCOL(FACE_DARK));

	//ボタン

	mPixbufFillBox(pixbuf, wg->w - 2 - bw, 2, bw, bw, MSYSCOL(FRAME_DARK));

	mPixbufDrawArrowDown(pixbuf, wg->w - 2 - bw + bw / 2, 2 + bw / 2, MSYSCOL(FACE_LIGHTEST));

	//テキスト

	if(enable)
	{
		mPixbufSetClipBox_d(pixbuf, 3, 0, wg->w - bw - 3 - 5, wg->h);

		mFontDrawText(mWidgetGetFont(wg), pixbuf,
			3, 2, SELMATERIAL(wg)->strText.buf, -1, MSYSCOL_RGB(TEXT));
	}
}

/** 推奨サイズ計算 */

static void _calchint_handle(mWidget *wg)
{
	wg->hintH = mWidgetGetFont(wg)->height + 4;
	wg->hintW = wg->hintH + 12;
}

/** 破棄 */

static void _destroy_handle(mWidget *wg)
{
	SelMaterial *p = SELMATERIAL(wg);

	mStrFree(&p->strText);
	mStrFree(&p->strPath);
}

/** 作成 */

SelMaterial *SelMaterial_new(mWidget *parent,int id,int type)
{
	SelMaterial *p;

	p = (SelMaterial *)mWidgetNew(sizeof(SelMaterial), parent);
	if(!p) return NULL;

	p->wg.id = id;
	p->wg.fLayout = MLF_EXPAND_W;
	p->wg.fEventFilter |= MWIDGET_EVENTFILTER_POINTER;

	p->wg.destroy = _destroy_handle;
	p->wg.draw = _draw_handle;
	p->wg.event = _event_handle;
	p->wg.calcHint = _calchint_handle;
	
	p->type = type;

	return p;
}

/** 名前取得 */

void SelMaterial_getName(SelMaterial *p,mStr *str)
{
	mStrCopy(str, &p->strPath);
}

/** 名前セット
 *
 * @param name  NULL または空文字列でなし */

void SelMaterial_setName(SelMaterial *p,const char *name)
{
	int id = -1;
	mBool empty;

	mStrSetText(&p->strPath, name);

	//表示名

	empty = (!name || !name[0]);

	M_TR_G(TRGROUP_SELMATERIAL);

	switch(p->type)
	{
		//テクスチャ
		case SELMATERIAL_TYPE_TEXTURE:
			if(empty) id = TRID_NONE;
			break;
		//ブラシテクスチャ
		case SELMATERIAL_TYPE_BRUSH_TEXTURE:
			if(empty)
				id = TRID_NONE_FORCE;
			else if(strcmp(name, "?") == 0)
				id = TRID_OPT_TEXTURE;
			break;
		//ブラシ形状
		case SELMATERIAL_TYPE_BRUSH_SHAPE:
			if(empty)
				id = TRID_SHAPE_NORMAL;
			break;
	}

	if(id == -1)
		mStrSetText(&p->strText, name);
	else
		mStrSetText(&p->strText, M_TR_T(id));

	//

	mWidgetUpdate(M_WIDGET(p));
}
