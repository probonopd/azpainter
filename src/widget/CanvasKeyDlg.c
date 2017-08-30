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
 * キャンバスキー設定
 *****************************************/

#include "mDef.h"
#include "mWidget.h"
#include "mDialog.h"
#include "mContainer.h"
#include "mWindow.h"
#include "mLabel.h"
#include "mButton.h"
#include "mListView.h"
#include "mStr.h"
#include "mTrans.h"
#include "mEvent.h"
#include "mMessageBox.h"
#include "mGui.h"
#include "mKeyDef.h"

#include "defConfig.h"
#include "defTool.h"
#include "defCanvasKeyID.h"
#include "AppErr.h"

#include "trgroup.h"


//----------------------

typedef struct
{
	mListViewItem li;

	const char *text;	//ラベル名
	uint8_t cmdid;		//コマンドID。0 でグループの項目
	short key;			//キー番号。-1 でなし
}_listitem;

//----------------------

typedef struct
{
	mWidget wg;
	mContainerData ct;
	mWindowData win;
	mDialogData dlg;

	mListView *list;
}_canvaskey_dlg;

//----------------------

enum
{
	TRID_HELP = 1,
	TRID_CLEAR,
	
	TRID_GROUP_TOOL = 100,
	TRID_GROUP_DRAWTYPE,
	TRID_GROUP_COMMAND,
	TRID_GROUP_OP_TOOL,
	TRID_GROUP_OP_DRAWTYPE,
	TRID_GROUP_OP_OTHER,
	TRID_GROUP_OP_SELECT,

	TRID_COMMAND_TOP = 1000,
	TRID_OP_OTHER_TOP = 1100,
	TRID_OP_SELECT_TOP = 1200
};

#define WID_BTT_CLEAR 100
#define _KEY_NONE  -1

//----------------------


//=======================
// リスト作成
//=======================


/** グループ項目追加 */

static void _add_group(_canvaskey_dlg *p,uint16_t trid,mStr *str)
{
	mListViewAddItem_ex(p->list, sizeof(_listitem),
		M_TR_T2(TRGROUP_DLG_CANVASKEY_OPT, trid),
		-1, MLISTVIEW_ITEM_F_HEADER, 0);
}

/** 項目追加 */

static void _add_item(_canvaskey_dlg *p,uint16_t trid,int cmdid,mStr *str)
{
	int i,key;
	uint8_t *pkey;
	char m[32];
	_listitem *pi;
	const char *trtext;

	trtext = M_TR_T(trid);

	mStrSetChar(str, ' ');
	mStrAppendText(str, trtext);

	//キーを検索

	key = _KEY_NONE;

	pkey = APP_CONF->canvaskey;

	for(i = 0; i < 256; i++, pkey++)
	{
		if(*pkey == cmdid)
		{
			key = i;
			
			if(mRawKeyCodeToName(i, m, 32) == 0)
			{
				m[0] = '?';
				m[1] = 0;
			}

			mStrAppendText(str, " = ");
			mStrAppendText(str, m);
			break;
		}
	}

	//追加
	
	pi = (_listitem *)mListViewAddItem_ex(p->list, sizeof(_listitem),
		str->buf, -1, 0, 0);

	pi->text = trtext;
	pi->cmdid = cmdid;
	pi->key = key;
}

/** グループ + 項目を追加 */

static void _add_groupitem(_canvaskey_dlg *p,uint16_t trid_group,
	uint16_t trgroup_item,uint16_t trid_item_top,uint16_t cmdid_top,int num,mStr *str)
{
	int i;
	
	_add_group(p, trid_group, str);

	M_TR_G(trgroup_item);

	for(i = 0; i < num; i++)
		_add_item(p, trid_item_top + i, cmdid_top + i, str);
}

/** 初期リストセット */

static void _set_list(_canvaskey_dlg *p)
{
	mStr str = MSTR_INIT;
	int i;

	//ツール変更

	_add_groupitem(p, TRID_GROUP_TOOL, TRGROUP_TOOL,
		0, CANVASKEYID_CMD_TOOL, TOOL_NUM, &str);

	//描画タイプ変更

	_add_groupitem(p, TRID_GROUP_DRAWTYPE, TRGROUP_TOOL_SUB,
		0, CANVASKEYID_CMD_DRAWTYPE, TOOLSUB_DRAW_NUM, &str);

	//他コマンド

	_add_groupitem(p, TRID_GROUP_COMMAND, TRGROUP_DLG_CANVASKEY_OPT,
		TRID_COMMAND_TOP, CANVASKEYID_CMD_OTHER, CANVASKEY_CMD_OTHER_NUM, &str);

	//+キーでツール動作 (一部ツールは除く)

	_add_group(p, TRID_GROUP_OP_TOOL, &str);

	M_TR_G(TRGROUP_TOOL);

	for(i = 0; i < TOOL_NUM; i++)
	{
		if(i == TOOL_BOXEDIT || i == TOOL_TEXT || i == TOOL_STAMP)
			continue;
	
		_add_item(p, i, CANVASKEYID_OP_TOOL + i, &str);
	}

	//+キーで描画タイプ動作

	_add_groupitem(p, TRID_GROUP_OP_DRAWTYPE, TRGROUP_TOOL_SUB,
		0, CANVASKEYID_OP_BRUSHDRAW, TOOLSUB_DRAW_NUM, &str);

	//+キーで選択範囲ツール動作

	_add_groupitem(p, TRID_GROUP_OP_SELECT, TRGROUP_DLG_CANVASKEY_OPT,
		TRID_OP_SELECT_TOP, CANVASKEYID_OP_SELECT, TOOLSUB_SEL_NUM, &str);

	//+キーで他動作

	_add_groupitem(p, TRID_GROUP_OP_OTHER, TRGROUP_DLG_CANVASKEY_OPT,
		TRID_OP_OTHER_TOP, CANVASKEYID_OP_OTHER, CANVASKEY_OP_OTHER_NUM, &str);

	mStrFree(&str);
}


//=======================
//
//=======================


/** 項目テキスト取得 */

static void _get_item_text(_listitem *pi,mStr *str)
{
	char m[32];

	mStrSetChar(str, ' ');
	mStrAppendText(str, pi->text);

	if(pi->key != _KEY_NONE)
	{
		if(mRawKeyCodeToName(pi->key, m, 32) == 0)
		{
			m[0] = '?';
			m[1] = 0;
		}

		mStrAppendText(str, " = ");
		mStrAppendText(str, m);
	}
}

/** 指定キーが使われているか */

static mBool _find_key(_canvaskey_dlg *p,int key)
{
	_listitem *pi;

	pi = (_listitem *)mListViewGetTopItem(p->list);

	for(; pi; pi = (_listitem *)pi->li.i.next)
	{
		if(pi->cmdid && pi->key == key)
			return TRUE;
	}

	return FALSE;
}

/** リストビューのキー押し時
 *
 * key は生のキーコード (ハードウェアの各ボタンに対する番号) */

static void _list_keydown(_canvaskey_dlg *p,int rawkey)
{
	_listitem *pi;
	uint8_t exkey[] = {
		MKEY_SPACE, MKEY_SHIFT, MKEY_CONTROL, MKEY_ALT, MKEY_SUPER, MKEY_MENU,
		MKEY_NUM_LOCK, MKEY_SCROLL_LOCK, MKEY_CAPS_LOCK, 0
	};
	int i;
	uint32_t code;
	mStr str = MSTR_INIT;

	//選択アイテム

	pi = (_listitem *)mListViewGetFocusItem(p->list);

	if(!pi || pi->cmdid == 0)
		return;

	//除外対象のキーの場合はクリア

	code = mKeyRawToCode(rawkey);

	if(code)
	{
		for(i = 0; exkey[i]; i++)
		{
			if(exkey[i] == code)
			{
				rawkey = _KEY_NONE;
				break;
			}
		}
	}

	//キーが変更された

	if(rawkey != pi->key)
	{
		//すでに使われているキーか

		if(rawkey != _KEY_NONE && _find_key(p, rawkey))
		{
			mMessageBoxErrTr(M_WINDOW(p), TRGROUP_APPERR, APPERR_USED_KEY);
			return;
		}
	
		//
		
		pi->key = rawkey;

		//リストテキスト

		_get_item_text(pi, &str);
		
		mListViewSetItemText(p->list, M_LISTVIEWITEM(pi), str.buf);

		mStrFree(&str);
	}
}

/** すべてクリア */

static void _cmd_clear_all(_canvaskey_dlg *p)
{
	_listitem *pi;
	mStr str = MSTR_INIT;

	pi = (_listitem *)mListViewGetTopItem(p->list);

	for(; pi; pi = (_listitem *)pi->li.i.next)
	{
		if(pi->cmdid)
		{
			pi->key = _KEY_NONE;

			_get_item_text(pi, &str);
			
			mListViewSetItemText(p->list, M_LISTVIEWITEM(pi), str.buf);
		}
	}

	mStrFree(&str);
}


//=========================
//
//=========================


/** リストビューのイベント */

static int _list_event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_KEYDOWN)
	{
		if(ev->key.rawcode < 256)
			_list_keydown((_canvaskey_dlg *)wg->toplevel, ev->key.rawcode);

		return 1;
	}

	return mListViewEventHandle(wg, ev);
}

/** イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY && ev->notify.id == WID_BTT_CLEAR)
	{
		//すべてクリア

		_cmd_clear_all((_canvaskey_dlg *)wg);
		return 1;
	}

	return mDialogEventHandle_okcancel(wg, ev);
}


//===================


/** 作成 */

static _canvaskey_dlg *_create_dlg(mWindow *owner)
{
	_canvaskey_dlg *p;
	mWidget *ct;
	mListView *list;

	p = (_canvaskey_dlg *)mDialogNew(sizeof(_canvaskey_dlg), owner, MWINDOW_S_DIALOG_NORMAL);
	if(!p) return NULL;

	p->wg.event = _event_handle;
	p->ct.sepW = 5;

	//

	mContainerSetPadding_one(M_CONTAINER(p), 8);

	M_TR_G(TRGROUP_DLG_CANVASKEY_OPT);

	mWindowSetTitle(M_WINDOW(p), M_TR_T(0));

	//------- ウィジェット

	//リスト

	list = p->list = mListViewNew(0, M_WIDGET(p), 0,
		MSCROLLVIEW_S_HORZVERT | MSCROLLVIEW_S_FIX_VERT | MSCROLLVIEW_S_FRAME);

	list->wg.fLayout = MLF_EXPAND_WH;
	list->wg.event = _list_event_handle;
	list->wg.fAcceptKeys = MWIDGET_ACCEPTKEY_ENTER;

	mWidgetSetInitSize_fontHeight(M_WIDGET(list), 24, 18);

	//ヘルプ

	mLabelCreate(M_WIDGET(p), MLABEL_S_BORDER, MLF_EXPAND_W, 0, M_TR_T(TRID_HELP));

	//クリア/OK/cancel

	ct = mContainerCreate(M_WIDGET(p), MCONTAINER_TYPE_HORZ, 0, 3, MLF_RIGHT);
	ct->margin.top = 12;

	mButtonCreate(ct, WID_BTT_CLEAR, 0, 0, M_MAKE_DW4(0,0,10,0), M_TR_T(TRID_CLEAR));
	mButtonCreate(ct, M_WID_OK, 0, 0, 0, M_TR_T2(M_TRGROUP_SYS, M_TRSYS_OK));
	mButtonCreate(ct, M_WID_CANCEL, 0, 0, 0, M_TR_T2(M_TRGROUP_SYS, M_TRSYS_CANCEL));

	//リストセット

	_set_list(p);

	mListViewSetWidthAuto(list, FALSE);

	return p;
}

/** データセット */

static void _set_data(_canvaskey_dlg *p)
{
	_listitem *pi;

	mMemzero(APP_CONF->canvaskey, 256);

	//

	pi = (_listitem *)mListViewGetTopItem(p->list);

	for(; pi; pi = (_listitem *)pi->li.i.next)
	{
		if(pi->cmdid && pi->key != _KEY_NONE)
			APP_CONF->canvaskey[pi->key] = pi->cmdid;
	}
}

/** キャンバスキーダイアログ実行 */

mBool CanvasKeyDlg_run(mWindow *owner)
{
	_canvaskey_dlg *p;
	int ret;

	p = _create_dlg(owner);
	if(!p) return FALSE;

	mWindowMoveResizeShow_hintSize(M_WINDOW(p));

	ret = mDialogRun(M_DIALOG(p), FALSE);

	if(ret)
		_set_data(p);

	mWidgetDestroy(M_WIDGET(p));

	return ret;
}
