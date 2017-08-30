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
 * ショートカットキー設定
 *****************************************/
/*
 * - 起動時に設定ファイルから読み込み、
 *   メニューにショートカットキーがセットされる。
 * - 設定ダイアログでは、メニューのデータからショートカットキーを
 *   取得し、作業用としてツリーデータを作成する。
 * - 確定時、ツリーデータからメニューにショートカットキーを設定し、
 *   設定ファイルに保存する。
 */

#include <stdio.h>

#include "mDef.h"
#include "mWidget.h"
#include "mDialog.h"
#include "mContainer.h"
#include "mWindow.h"
#include "mButton.h"
#include "mListView.h"
#include "mComboBox.h"
#include "mLineEdit.h"
#include "mInputAccelKey.h"
#include "mWidgetBuilder.h"
#include "mTree.h"
#include "mMenu.h"
#include "mStr.h"
#include "mAccelerator.h"
#include "mTrans.h"
#include "mEvent.h"
#include "mIniWrite.h"
#include "mAppDef.h"
#include "mMessageBox.h"

#include "defMacros.h"
#include "defWidgets.h"
#include "defMainWindow.h"
#include "AppErr.h"

#include "trgroup.h"
#include "trid_mainmenu.h"


//-------------------------
/* ツリーアイテムデータ */

typedef struct
{
	mTreeItem i;
	const char *text;	//項目名 (メニューのテキストと同じポインタを使う)
	uint16_t cmdid,		//メニューのコマンドID
		key,			//現在のキー
		key_old;		//元のキー (ダイアログを開いた時点の値)
	uint8_t submenu;	//サブメニューのタイプ
}_treeitem;

//----------------------

typedef struct
{
	mWidget wg;
	mContainerData ct;
	mWindowData win;
	mDialogData dlg;

	mTree tree;
	_treeitem *seltop;	//選択されているトップ項目

	mComboBox *cb_top;
	mListView *list;
	mLineEdit *edit_cmd;
	mInputAccelKey *acckey;
}_sckey_dlg;

//----------------------

static const char *g_build =
"cb:id=100:lf=w;"
"lv#,hvVf:lf=wh:id=101;"
"ct#g2:lf=w:sep=5,7;"
  "lb:lf=mr:tr=1;"
  "le#r:lf=w:id=102;"
  "lb:lf=mr:tr=2;"
  "ct#h:lf=w:sep=4;"
    "iak#c:lf=w:id=103;"
    "bt:id=104:tr=3;"
;

//----------------------

enum
{
	WID_CB_TOP = 100,
	WID_LIST,
	WID_EDIT_CMD,
	WID_ACCKEY,
	WID_BTT_CLEAR,
	WID_BTT_CLEAR_ALL
};

#define _SUBMENU_TOP   1	//サブメニューの親アイテム
#define _SUBMENU_ITEM  2	//サブメニューの子アイテム

//設定可能な項目か
#define _IS_TREEITEM_NORM(pi)  (pi->i.parent && pi->submenu != _SUBMENU_TOP)

//----------------------


//==========================
// ツリーデータ
//==========================


/** メニューアイテムからツリーアイテム追加
 *
 * @param pptreeitem 作成されたツリーアイテムのポインタが入る
 * @param ppsubmenu  サブメニューがある場合、そのポインタが入る
 * @return FALSE で作成に失敗 */

static mBool _append_treeitem(_sckey_dlg *p,_treeitem *parent,mMenuItem *mi,
	_treeitem **pptreeitem,mMenu **ppsubmenu)
{
	_treeitem *pi;
	mMenuItemInfo *pinfo;

	pinfo = mMenuGetInfoInItem(mi);

	//リストに追加しないもの (セパレータ、最近使ったファイル)

	if((pinfo->flags & MMENUITEM_F_SEP)
		|| pinfo->id == TRMENU_FILE_RECENTFILE)
	{
		if(ppsubmenu) *ppsubmenu = NULL;
		return TRUE;
	}

	//追加

	pi = (_treeitem *)mTreeAppendNew(&p->tree, M_TREEITEM(parent), sizeof(_treeitem), NULL);
	if(!pi) return FALSE;
	
	pi->cmdid = pinfo->id;
	pi->key = pi->key_old = pinfo->shortcutkey;
	pi->text = pinfo->label;

	if(parent)
	{
		if(pinfo->submenu)
			pi->submenu = _SUBMENU_TOP;
		else if(parent->submenu)
			pi->submenu = _SUBMENU_ITEM;
	}

	if(pptreeitem) *pptreeitem = pi;
	if(ppsubmenu) *ppsubmenu = pinfo->submenu;

	return TRUE;
}

/** メニューからコマンドのツリーデータ作成 */

static void _create_treedat(_sckey_dlg *p)
{
	mMenuItem *mi_top,*mi,*mi2;
	_treeitem *ti_top,*ti;
	mMenu *submenu,*submenu2;

	mi_top = mMenuGetTopItem(APP_WIDGETS->mainwin->menu_main);

	for(; mi_top; mi_top = mMenuGetNextItem(mi_top))
	{
		//トップ
		
		if(!_append_treeitem(p, NULL, mi_top, &ti_top, &submenu))
			return;

		//メイン項目

		for(mi = mMenuGetTopItem(submenu); mi; mi = mMenuGetNextItem(mi))
		{
			if(!_append_treeitem(p, ti_top, mi, &ti, &submenu2))
				return;

			//サブメニュー

			if(submenu2)
			{
				for(mi2 = mMenuGetTopItem(submenu2); mi2; mi2 = mMenuGetNextItem(mi2))
				{
					if(!_append_treeitem(p, ti, mi2, NULL, NULL))
						return;
				}
			}
		}
	}
}


//==========================
// sub
//==========================


/** (mComboBox) トップ項目のリストをセット */

static void _set_topmenu(_sckey_dlg *p)
{
	_treeitem *pi;

	for(pi = (_treeitem *)p->tree.top; pi; pi = (_treeitem *)pi->i.next)
		mComboBoxAddItem(p->cb_top, pi->text, (intptr_t)pi);

	mComboBoxSetSel_index(p->cb_top, 0);

	p->seltop = (_treeitem *)p->tree.top;
}

/** リストの項目テキスト取得 */

static void _get_listitem_text(_treeitem *pi,mStr *str)
{
	char *actext;

	//サブメニューインデント

	if(pi->submenu == _SUBMENU_ITEM)
		mStrSetText(str, "   ");
	else
		mStrEmpty(str);

	//テキスト

	mStrAppendText(str, pi->text);

	//キー名

	if(pi->key)
	{
		actext = mAcceleratorGetKeyText(pi->key);
		if(actext)
		{
			mStrAppendFormat(str, " = %s", actext);
			mFree(actext);
		}
	}
}

/** リスト項目追加 */

static void _append_listitem(_sckey_dlg *p,_treeitem *pi,mStr *str)
{
	_get_listitem_text(pi, str);

	mListViewAddItem(p->list, str->buf, -1,
		(pi->submenu == _SUBMENU_TOP)? MLISTVIEW_ITEM_F_HEADER: 0, (intptr_t)pi);
}

/** リスト項目をセット */

static void _set_list(_sckey_dlg *p)
{
	_treeitem *pi,*pisub;
	mStr str = MSTR_INIT;

	mListViewDeleteAllItem(p->list);

	for(pi = (_treeitem *)p->seltop->i.first; pi; pi = (_treeitem *)pi->i.next)
	{
		_append_listitem(p, pi, &str);

		if(pi->i.first)
		{
			for(pisub = (_treeitem *)pi->i.first; pisub; pisub = (_treeitem *)pisub->i.next)
				_append_listitem(p, pisub, &str);
		}
	}

	mStrFree(&str);

	mListViewSetWidthAuto(p->list, FALSE);
}

/** 指定キーがツリーデータ内に存在するか */

static mBool _find_key(_sckey_dlg *p,int key)
{
	_treeitem *pi;

	for(pi = (_treeitem *)p->tree.top; pi; pi = (_treeitem *)mTreeItemGetNext(M_TREEITEM(pi)))
	{
		if(pi->key == key)
			return TRUE;
	}

	return FALSE;
}

/** すべてクリア */

static void _cmd_clear_all(_sckey_dlg *p)
{
	_treeitem *pi;

	//キーをすべて 0 に

	for(pi = (_treeitem *)p->tree.top; pi; pi = (_treeitem *)mTreeItemGetNext(M_TREEITEM(pi)))
		pi->key = 0;

	//リスト

	_set_list(p);
}


//==========================
//
//==========================


/** リスト選択変更時
 *
 * @param pi  NULL でクリア */

static void _change_listsel(_sckey_dlg *p,_treeitem *pi)
{
	if(!pi || pi->submenu == _SUBMENU_TOP)
	{
		//キーなし
		
		mLineEditSetText(p->edit_cmd, NULL);
		mInputAccelKey_setKey(p->acckey, 0);
	}
	else
	{
		mLineEditSetText(p->edit_cmd, pi->text);
		mInputAccelKey_setKey(p->acckey, pi->key);

		mWidgetSetFocus(M_WIDGET(p->acckey));
	}
}

/** キーを変更 */

static void _set_key(_sckey_dlg *p,mBool clear)
{
	mListViewItem *lvi;
	_treeitem *ti;
	uint32_t key;
	mStr str = MSTR_INIT;

	lvi = mListViewGetFocusItem(p->list);
	if(!lvi) return;

	ti = (_treeitem *)lvi->param;
	if(ti->submenu == _SUBMENU_TOP) return;

	//キー

	if(clear)
		key = 0;
	else
		key = mInputAccelKey_getKey(p->acckey);

	//変更

	if(key != ti->key)
	{
		//すでに使用されているキーか

		if(key && _find_key(p, key))
		{
			mMessageBoxErrTr(M_WINDOW(p), TRGROUP_APPERR, APPERR_USED_KEY);
			return;
		}
	
		//
		
		ti->key = key;

		//リストテキスト

		_get_listitem_text(ti, &str);

		mListViewSetItemText(p->list, lvi, str.buf);

		mStrFree(&str);

		//ボタンからのクリア時

		if(clear)
			mInputAccelKey_setKey(p->acckey, 0);
	}
}

/** イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY)
	{
		_sckey_dlg *p = (_sckey_dlg *)wg;
		
		switch(ev->notify.id)
		{
			//キー
			case WID_ACCKEY:
				if(ev->notify.type == MINPUTACCELKEY_N_CHANGE_KEY)
					_set_key(p, FALSE);
				break;
			//リスト
			case WID_LIST:
				if(ev->notify.type == MLISTVIEW_N_CHANGE_FOCUS)
					_change_listsel(p, (_treeitem *)ev->notify.param2);
				break;

			//トップメニュー
			case WID_CB_TOP:
				if(ev->notify.type == MCOMBOBOX_N_CHANGESEL)
				{
					p->seltop = (_treeitem *)ev->notify.param2;

					_set_list(p);
					_change_listsel(p, NULL);
				}
				break;
			//クリア
			case WID_BTT_CLEAR:
				_set_key(p, TRUE);
				break;
			//すべてクリア
			case WID_BTT_CLEAR_ALL:
				_cmd_clear_all(p);
				break;
		}
	}

	return mDialogEventHandle_okcancel(wg, ev);
}


//=====================


/** 作成 */

static _sckey_dlg *_create_dlg(mWindow *owner)
{
	_sckey_dlg *p;
	mWidget *ct;

	p = (_sckey_dlg *)mDialogNew(sizeof(_sckey_dlg), owner, MWINDOW_S_DIALOG_NORMAL);
	if(!p) return NULL;

	p->wg.event = _event_handle;
	p->ct.sepW = 7;

	//

	mContainerSetPadding_one(M_CONTAINER(p), 8);

	M_TR_G(TRGROUP_DLG_SHORTCUTKEY_OPT);

	mWindowSetTitle(M_WINDOW(p), M_TR_T(0));

	//------- ウィジェット

	mWidgetBuilderCreateFromText(M_WIDGET(p), g_build);

	p->cb_top = (mComboBox *)mWidgetFindByID(M_WIDGET(p), WID_CB_TOP);
	p->list = (mListView *)mWidgetFindByID(M_WIDGET(p), WID_LIST);
	p->edit_cmd = (mLineEdit *)mWidgetFindByID(M_WIDGET(p), WID_EDIT_CMD);
	p->acckey = (mInputAccelKey *)mWidgetFindByID(M_WIDGET(p), WID_ACCKEY);

	//クリア/OK/cancel

	ct = mContainerCreate(M_WIDGET(p), MCONTAINER_TYPE_HORZ, 0, 3, MLF_EXPAND_W);
	ct->margin.top = 12;

	mButtonCreate(ct, WID_BTT_CLEAR_ALL, 0, MLF_EXPAND_X, M_MAKE_DW4(0,0,10,0), M_TR_T(4));
	mButtonCreate(ct, M_WID_OK, 0, 0, 0, M_TR_T2(M_TRGROUP_SYS, M_TRSYS_OK));
	mButtonCreate(ct, M_WID_CANCEL, 0, 0, 0, M_TR_T2(M_TRGROUP_SYS, M_TRSYS_CANCEL));

	//------- 初期化

	_create_treedat(p);

	//トップメニュー

	_set_topmenu(p);

	//リスト

	_set_list(p);

	mWidgetSetInitSize_fontHeight(M_WIDGET(p->list), 24, 18);

	return p;
}

/** アクセラレータ/メニューにセット */

static void _set_acckey(mTree *tree)
{
	mAccelerator *accel;
	mMenu *mainmenu;
	_treeitem *pi;

	accel = APP_WIDGETS->mainwin->win.accelerator;
	mainmenu = APP_WIDGETS->mainwin->menu_main;

	mAcceleratorDeleteAll(accel);

	for(pi = (_treeitem *)tree->top; pi; pi = (_treeitem *)mTreeItemGetNext(M_TREEITEM(pi)))
	{
		//トップメニュー、またはサブメニューのトップは除く
		
		if(_IS_TREEITEM_NORM(pi))
		{
			//アクセラレータ
			
			if(pi->key)
				mAcceleratorAdd(accel, pi->cmdid, pi->key, NULL);

			//メニュー (キーが変更された場合のみ)

			if(pi->key != pi->key_old)
				mMenuSetShortcutKey(mainmenu, pi->cmdid, pi->key);
		}
	}
}

/** 設定ファイルに書き込み */

static void _write_config(mTree *tree)
{
	FILE *fp;
	_treeitem *pi;

	fp = mIniWriteOpenFile2(MAPP->pathConfig, CONFIG_FILENAME_SHORTCUTKEY);
	if(!fp) return;

	mIniWriteGroup(fp, "sckey");

	for(pi = (_treeitem *)tree->top; pi; pi = (_treeitem *)mTreeItemGetNext(M_TREEITEM(pi)))
	{
		if(_IS_TREEITEM_NORM(pi) && pi->key)
			mIniWriteNoHex(fp, pi->cmdid, pi->key);
	}

	fclose(fp);
}

/** 実行 */

mBool ShortcutKeyDlg_run(mWindow *owner)
{
	_sckey_dlg *p;
	int ret;

	p = _create_dlg(owner);
	if(!p) return FALSE;

	mWindowMoveResizeShow_hintSize(M_WINDOW(p));

	ret = mDialogRun(M_DIALOG(p), FALSE);

	if(ret)
	{
		_set_acckey(&p->tree);

		_write_config(&p->tree);
	}

	mTreeDeleteAll(&p->tree);

	mWidgetDestroy(M_WIDGET(p));

	return ret;
}
