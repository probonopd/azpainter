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
 * 環境設定ダイアログ (ボタン操作)
 *****************************************/

#include "mDef.h"
#include "mWidget.h"
#include "mContainer.h"
#include "mLabel.h"
#include "mButton.h"
#include "mCheckButton.h"
#include "mComboBox.h"
#include "mGroupBox.h"
#include "mListView.h"
#include "mTrans.h"
#include "mEvent.h"
#include "mUtilStr.h"
#include "mStr.h"

#include "defConfig.h"
#include "defCanvasKeyID.h"
#include "defTool.h"

#include "trgroup.h"

#include "EnvOptDlg_pv.h"


//---------------

typedef struct
{
	mWidget wg;
	mContainerData ct;

	mListView *list,
		*list_cmd;
	mComboBox *cb_btt;

	int type;
	uint8_t *buf[2];		//設定先バッファ
	const char *text_eraser;	//"消しゴム側"
}_ct_btt;

//---------------

enum
{
	WID_LIST = 100,
	WID_CK_DEFAULT,
	WID_CK_PENTAB,
	WID_CB_BTT,
	WID_LIST_CMD,
	WID_BTT_SET
};

//---------------

//TRGROUP_DLG_CANVASKEY_OPT
enum
{
	TRID_CANVASKEY_CMD_OTHER_TOP = 1000,
	TRID_CANVASKEY_OP_OTHER_TOP = 1100
};

#define KEYID_HEADER  -1

//----------------------



//=========================
// sub
//=========================


/** コマンドリストからラベル文字列取得 */

static const char *_get_cmd_label(_ct_btt *p,int cmdid)
{
	mListViewItem *pi;

	pi = mListViewGetTopItem(p->list_cmd);

	for(; pi; pi = M_LISTVIEWITEM(pi->i.next))
	{
		if(pi->param == cmdid)
			return pi->text;
	}

	return NULL;
}

/** リスト文字列取得 */

static void _get_listtext(_ct_btt *p,mStr *str,int btt,int cmdid)
{
	const char *pc = _get_cmd_label(p, cmdid);

	if(btt == 0)
		mStrSetFormat(str, "%s = %s", p->text_eraser, pc);
	else
		mStrSetFormat(str, "%d = %s", btt, pc);
}

/** リストセット */

static void _set_list(_ct_btt *p)
{
	int i,cmdid;
	uint8_t *buf;
	mStr str = MSTR_INIT;

	mListViewDeleteAllItem(p->list);

	buf = p->buf[p->type];

	for(i = 0; i < CONFIG_POINTERBTT_NUM; i++)
	{
		cmdid = *(buf++);

		if(cmdid)
		{
			_get_listtext(p, &str, i, cmdid);

			mListViewAddItem(p->list, str.buf, -1, 0, i | (cmdid << 8));
		}
	}

	mStrFree(&str);
}

/** 編集項目にデータセット */

static void _set_edit_value(_ct_btt *p,int val)
{
	//ボタン

	mComboBoxSetSel_index(p->cb_btt, val & 255);

	//コマンド

	mListViewSetFocusItem_findParam(p->list_cmd, val >> 8);
	mListViewScrollToItem(p->list_cmd, NULL, 1);
}

/** セット */

static void _cmd_set(_ct_btt *p)
{
	mListViewItem *pi;
	int btt;

	//コマンド

	pi = mListViewGetFocusItem(p->list_cmd);

	if(!pi || pi->param == KEYID_HEADER) return;

	//ボタン

	btt = mComboBoxGetItemParam(p->cb_btt, -1);

	//セット

	p->buf[p->type][btt] = pi->param;

	//リスト

	_set_list(p);
}


//===================
// イベント
//===================


/** ボタン取得ラベルのイベント */

static int _getbtt_event(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_POINTER
		&& ev->pt.type == MEVENT_POINTER_TYPE_PRESS)
	{
		if(ev->pt.btt <= CONFIG_POINTERBTT_MAXNO)
			mComboBoxSetSel_index((mComboBox *)wg->param, ev->pt.btt);
	}

	return 1;
}

/** イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY)
	{
		_ct_btt *p = (_ct_btt *)wg;
	
		switch(ev->notify.id)
		{
			//リスト
			case WID_LIST:
				if(ev->notify.type == MLISTVIEW_N_CHANGE_FOCUS)
					_set_edit_value(p, ev->notify.param2);
				break;
			//タイプ
			case WID_CK_DEFAULT:
			case WID_CK_PENTAB:
				p->type = ev->notify.id - WID_CK_DEFAULT;
				_set_list(p);
				break;
			//セット
			case WID_BTT_SET:
				_cmd_set(p);
				break;
		}
	}

	return 1;
}


//====================


/** コマンドリストセット */

static void _set_cmdlist(mListView *list)
{
	int i;

	//指定なし

	mListViewAddItem(list, M_TR_T(TRID_BTT_CMD_DEFAULT), -1,
		MLISTVIEW_ITEM_F_STATIC_TEXT, 0);

	//ツール動作

	mListViewAddItem(list, M_TR_T(TRID_BTT_CMD_TOOL), -1,
		MLISTVIEW_ITEM_F_STATIC_TEXT | MLISTVIEW_ITEM_F_HEADER, KEYID_HEADER);

	M_TR_G(TRGROUP_TOOL);

	for(i = 0; i < TOOL_NUM; i++)
	{
		if(i != TOOL_BOXEDIT && i != TOOL_TEXT && i != TOOL_STAMP)
			mListViewAddItem(list, M_TR_T(i), -1, MLISTVIEW_ITEM_F_STATIC_TEXT, CANVASKEYID_OP_TOOL + i);
	}

	//他動作

	mListViewAddItem(list, M_TR_T2(TRGROUP_DLG_ENV_OPTION, TRID_BTT_CMD_OHTER), -1,
		MLISTVIEW_ITEM_F_STATIC_TEXT | MLISTVIEW_ITEM_F_HEADER, KEYID_HEADER);

	M_TR_G(TRGROUP_DLG_CANVASKEY_OPT);

	for(i = 0; i < CANVASKEY_OP_OTHER_NUM; i++)
		mListViewAddItem(list, M_TR_T(TRID_CANVASKEY_OP_OTHER_TOP + i), -1, MLISTVIEW_ITEM_F_STATIC_TEXT, CANVASKEYID_OP_OTHER + i);

	//コマンド

	mListViewAddItem(list, M_TR_T2(TRGROUP_DLG_ENV_OPTION, TRID_BTT_CMD_COMMAND), -1,
		MLISTVIEW_ITEM_F_STATIC_TEXT | MLISTVIEW_ITEM_F_HEADER, KEYID_HEADER);

	for(i = 0; i < CANVASKEY_CMD_OTHER_NUM; i++)
		mListViewAddItem(list, M_TR_T(TRID_CANVASKEY_CMD_OTHER_TOP + i), -1, MLISTVIEW_ITEM_F_STATIC_TEXT, CANVASKEYID_CMD_OTHER + i);

	//

	mListViewSetWidthAuto(list, FALSE);
	mListViewSetFocusItem_index(list, 0);
}

/** 作成 */

mWidget *EnvOptDlg_create_btt(mWidget *parent,EnvOptDlgCurData *dat)
{
	_ct_btt *p;
	mWidget *ct,*ct2,*wg;
	mComboBox *cb;
	mListView *list;
	int i;
	char m[10];

	p = (_ct_btt *)EnvOptDlg_createContentsBase(parent, sizeof(_ct_btt), NULL);

	p->wg.event = _event_handle;

	p->buf[0] = dat->default_btt_cmd;
	p->buf[1] = dat->pentab_btt_cmd;

	p->text_eraser = M_TR_T(TRID_BTT_ERASER);

	//タイプ

	ct = mContainerCreate(M_WIDGET(p), MCONTAINER_TYPE_HORZ, 0, 5, 0);

	for(i = 0; i < 2; i++)
	{
		mCheckButtonCreate(ct, WID_CK_DEFAULT + i, MCHECKBUTTON_S_RADIO,
			0, 0, M_TR_T(TRID_BTT_DEFAULT + i), (i == 0));
	}

	//リスト

	list = p->list = mListViewNew(0, M_WIDGET(p), MLISTVIEW_S_AUTO_WIDTH,
		MSCROLLVIEW_S_HORZVERT_FRAME);

	list->wg.id = WID_LIST;
	list->wg.fLayout = MLF_EXPAND_W;
	list->wg.hintOverH = mWidgetGetFontHeight(M_WIDGET(p->list)) * 7;

	//========= 編集

	ct = (mWidget *)mGroupBoxCreate(M_WIDGET(p), 0, MLF_EXPAND_W, M_MAKE_DW4(0,6,0,0), NULL);

	M_CONTAINER(ct)->ct.sepW = 7;

	//------ ボタン

	ct2 = mContainerCreate(ct, MCONTAINER_TYPE_HORZ, 0, 6, 0);

	mLabelCreate(ct2, 0, MLF_MIDDLE, 0, M_TR_T(TRID_BTT_BUTTON));

	//コンボボックス

	cb = p->cb_btt = mComboBoxCreate(ct2, WID_CB_BTT, 0, 0, 0);

	mComboBoxAddItem(cb, M_TR_T(TRID_BTT_ERASER), 0);

	for(i = 1; i <= CONFIG_POINTERBTT_MAXNO; i++)
	{
		mIntToStr(m, i);
		mComboBoxAddItem(cb, m, i);
	}

	mComboBoxSetWidthAuto(cb);
	mComboBoxSetSel_index(cb, 0);

	//ここでボタンを押す

	wg = (mWidget *)mLabelCreate(ct2, MLABEL_S_BORDER, MLF_MIDDLE, 0, M_TR_T(TRID_BTT_BUTTON_GET));

	wg->event = _getbtt_event;
	wg->fEventFilter |= MWIDGET_EVENTFILTER_POINTER;
	wg->fOption |= MWIDGET_OPTION_DISABLE_SCROLL_EVENT;
	wg->param = (intptr_t)p->cb_btt;

	//----- コマンド

	p->list_cmd = list = mListViewNew(0, ct, 0,
		MSCROLLVIEW_S_HORZVERT | MSCROLLVIEW_S_FIX_VERT | MSCROLLVIEW_S_FRAME);

	list->wg.id = WID_LIST_CMD;
	list->wg.fLayout = MLF_EXPAND_WH;
	list->wg.hintOverH = mWidgetGetFontHeight(M_WIDGET(p->list)) * 7;

	//セット

	mButtonCreate(ct, WID_BTT_SET, 0, MLF_RIGHT, 0, M_TR_T(TRID_BTT_SET));

	//--------

	_set_cmdlist(p->list_cmd);
	_set_list(p);

	return (mWidget *)p;
}
