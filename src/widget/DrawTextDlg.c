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

/*************************************
 * テキスト描画ダイアログ
 *************************************/

#include <string.h>

#include "mDef.h"
#include "mStr.h"
#include "mWidget.h"
#include "mDialog.h"
#include "mContainer.h"
#include "mWindow.h"
#include "mLabel.h"
#include "mButton.h"
#include "mCheckButton.h"
#include "mLineEdit.h"
#include "mComboBox.h"
#include "mMultiEdit.h"
#include "mContainerView.h"
#include "mEvent.h"
#include "mTrans.h"
#include "mEnumFont.h"
#include "mUtilStr.h"
#include "mKeyDef.h"
#include "mMessageBox.h"

#include "defConfig.h"
#include "defDraw.h"

#include "draw_text.h"

#include "trgroup.h"


//--------------------

typedef struct
{
	mWidget wg;
	mContainerData ct;
	mWindowData win;
	mDialogData dlg;

	mMultiEdit *medit;
	mLineEdit *edit_size,
		*edit_charsp,
		*edit_linesp;
	mComboBox *cb_family,
		*cb_style,
		*cb_weight,
		*cb_slant,
		*cb_hinting;
}_drawtext_dlg;

//--------------------

enum
{
	WID_CB_FONTNAME = 100,
	WID_CB_STYLE,
	WID_CB_WEIGHT,
	WID_CB_SLANT,
	WID_EDIT_SIZE,
	WID_CB_SIZE_UNIT,
	WID_CB_HINTING,
	WID_EDIT_CHARSP,
	WID_EDIT_LINESP,
	WID_CK_VERT,
	WID_CK_ANTIALIAS,

	WID_MEDIT = 200,
	WID_CK_PREV,
	WID_BTT_HELP
};

enum
{
	TRID_PREVIEW = 1,
	TRID_HELP,
	TRID_STYLE,
	TRID_WEIGHT,
	TRID_SLANT,
	TRID_SIZE,
	TRID_HINTING,
	TRID_CHAR_SPACE,
	TRID_LINE_SPACE,
	TRID_VERT,
	TRID_ANTIALIAS,

	TRID_HINTING_TOP = 100,
	TRID_WEIGHT_TOP = 110,
	TRID_SLANT_TOP = 120
};

//--------------------

#define _UPDATE_CREATEFONT  1
#define _UPDATE_TIMER       2

//--------------------


//============================
// エディタ
//============================


/** 複数行エディタハンドラ */

static int _multiedit_event_handle(mWidget *wg,mEvent *ev)
{
	//Ctrl+上下左右で 1px 移動

	if(ev->type == MEVENT_KEYDOWN
		&& (ev->key.state & M_MODS_MASK_KEY) == M_MODS_CTRL)
	{
		uint8_t flag = 1;
	
		switch(ev->key.code)
		{
			case MKEY_LEFT:
				APP_DRAW->w.pttmp[0].x--;
				break;
			case MKEY_RIGHT:
				APP_DRAW->w.pttmp[0].x++;
				break;
			case MKEY_UP:
				APP_DRAW->w.pttmp[0].y--;
				break;
			case MKEY_DOWN:
				APP_DRAW->w.pttmp[0].y++;
				break;
			default:
				flag = 0;
				break;
		}

		if(flag)
		{
			//プレビュー更新
			
			if(APP_DRAW->drawtext.flags & DRAW_DRAWTEXT_F_PREVIEW)
				drawText_drawPreview(APP_DRAW);
			
			return 1;
		}
	}

	return mMultiEditEventHandle(wg, ev);
}


//============================
// sub
//============================


/** フォント名のリストをセット */

static void _set_family_list(_drawtext_dlg *p,DrawTextData *pdat)
{
	mComboBox *cb;
	mListViewItem *pi,*pi_sel = NULL;
	char **names,**pp,*selname;

	cb = p->cb_family;

	names = mEnumFontFamily();

	if(names)
	{
		//追加

		selname = (mStrIsEmpty(&pdat->strName))? NULL: pdat->strName.buf;
	
		for(pp = names, pi_sel = NULL; *pp; pp++)
		{
			pi = mComboBoxAddItem(cb, *pp, 0);

			if(selname && !pi_sel && strcmp(*pp, selname) == 0)
				pi_sel = pi;
		}

		mFreeStrsBuf(names);

		//選択

		if(pi_sel)
			mComboBoxSetSelItem(cb, pi_sel);
		else
			mComboBoxSetSel_index(cb, 0);

		//

		mComboBoxSetWidthAuto(cb);
	}
}

/** 現在のフォントのスタイルのリストをセット */

static void _set_style_list(_drawtext_dlg *p,DrawTextData *pdat)
{
	mListViewItem *pi,*pi_sel = NULL;
	char **names,**pp,*selname;

	//削除

	pi = mComboBoxGetItemByIndex(p->cb_style, 1);

	for(; pi; pi = M_LISTVIEWITEM(pi->i.next))
		mComboBoxDeleteItem(p->cb_style, pi);

	//追加

	pi = mComboBoxGetSelItem(p->cb_family);

	selname = (mStrIsEmpty(&pdat->strStyle))? NULL: pdat->strStyle.buf;

	names = mEnumFontStyle(pi->text);

	if(names)
	{
		for(pp = names; *pp; pp++)
		{
			pi = mComboBoxAddItem(p->cb_style, *pp, 0);

			if(!pi_sel && selname && strcmp(*pp, selname) == 0)
				pi_sel = pi;
		}

		mFreeStrsBuf(names);
	}

	//選択

	if(pi_sel)
		mComboBoxSetSelItem(p->cb_style, pi_sel);
	else
	{
		mStrEmpty(&pdat->strStyle);

		mComboBoxSetSel_index(p->cb_style, 0);
	}
}

/** 太さと斜体の有効/無効 */

static void _enable_weight_and_slant(_drawtext_dlg *p,mBool enable)
{
	mWidgetEnable(M_WIDGET(p->cb_weight), enable);
	mWidgetEnable(M_WIDGET(p->cb_slant), enable);
}

/** 値変更時の更新 */

static void _update(_drawtext_dlg *p,uint8_t flags)
{
	//フォント作成
	
	if(flags & _UPDATE_CREATEFONT)
		drawText_createFont();

	//プレビュー

	if(APP_DRAW->drawtext.flags & DRAW_DRAWTEXT_F_PREVIEW)
	{
		if(flags & _UPDATE_TIMER)
			//タイマーを使う
			mWidgetTimerAdd(M_WIDGET(p), 0, 100, 0);
		else
			//直ちに更新
			drawText_drawPreview(APP_DRAW);
	}
}


//============================
// ハンドラ
//============================


/** 通知イベント */

static void _event_notify(_drawtext_dlg *p,mEvent *ev)
{
	DrawTextData *pdat = &APP_DRAW->drawtext;

	switch(ev->notify.id)
	{
		//エディット
		case WID_MEDIT:
			if(ev->notify.type == MMULTIEDIT_N_CHANGE)
			{
				mMultiEditGetTextStr(p->medit, &pdat->strText);

				_update(p, _UPDATE_TIMER);
			}
			break;
	
		//フォント名
		case WID_CB_FONTNAME:
			if(ev->notify.type == MCOMBOBOX_N_CHANGESEL)
			{
				mStrSetText(&pdat->strName, M_LISTVIEWITEM(ev->notify.param1)->text);
			
				_set_style_list(p, pdat);

				//スタイルが変更される場合があるので、太さと斜体セット
				_enable_weight_and_slant(p, mStrIsEmpty(&pdat->strStyle));

				_update(p, _UPDATE_CREATEFONT);
			}
			break;
		//スタイル
		case WID_CB_STYLE:
			if(ev->notify.type == MCOMBOBOX_N_CHANGESEL)
			{
				//param2 => 0:スタイルあり 1:スタイルなし
			
				if(ev->notify.param2)
					mStrEmpty(&pdat->strStyle);
				else
					mStrSetText(&pdat->strStyle, M_LISTVIEWITEM(ev->notify.param1)->text);

				//太さと斜体
				_enable_weight_and_slant(p, ev->notify.param2);

				_update(p, _UPDATE_CREATEFONT);
			}
			break;
		//太さ
		case WID_CB_WEIGHT:
			if(ev->notify.type == MCOMBOBOX_N_CHANGESEL)
			{
				pdat->weight = ev->notify.param2;

				_update(p, _UPDATE_CREATEFONT);
			}
			break;
		//斜体
		case WID_CB_SLANT:
			if(ev->notify.type == MCOMBOBOX_N_CHANGESEL)
			{
				pdat->slant = ev->notify.param2;

				_update(p, _UPDATE_CREATEFONT);
			}
			break;
		//サイズ
		case WID_EDIT_SIZE:
			if(ev->notify.type == MLINEEDIT_N_CHANGE)
			{
				pdat->size = mLineEditGetNum(p->edit_size);

				_update(p, _UPDATE_CREATEFONT);
			}
			break;
		//サイズ単位
		case WID_CB_SIZE_UNIT:
			if(ev->notify.type == MCOMBOBOX_N_CHANGESEL)
			{
				if(ev->notify.param2 == 0)
					pdat->flags &= ~DRAW_DRAWTEXT_F_SIZE_PIXEL;
				else
					pdat->flags |= DRAW_DRAWTEXT_F_SIZE_PIXEL;

				_update(p, _UPDATE_CREATEFONT);
			}
			break;
		//ヒンティング
		case WID_CB_HINTING:
			if(ev->notify.type == MCOMBOBOX_N_CHANGESEL)
			{
				pdat->hinting = ev->notify.param2;

				drawText_setHinting(APP_DRAW);

				_update(p, 0);
			}
			break;
		//字間
		case WID_EDIT_CHARSP:
			if(ev->notify.type == MLINEEDIT_N_CHANGE)
			{
				pdat->char_space = mLineEditGetNum(p->edit_charsp);

				_update(p, _UPDATE_TIMER);
			}
			break;
		//行間
		case WID_EDIT_LINESP:
			if(ev->notify.type == MLINEEDIT_N_CHANGE)
			{
				pdat->line_space = mLineEditGetNum(p->edit_linesp);

				_update(p, _UPDATE_TIMER);
			}
			break;
		//縦書き
		case WID_CK_VERT:
			pdat->flags ^= DRAW_DRAWTEXT_F_VERT;

			_update(p, 0);
			break;
		//アンチエイリアス
		case WID_CK_ANTIALIAS:
			pdat->flags ^= DRAW_DRAWTEXT_F_ANTIALIAS;

			_update(p, _UPDATE_CREATEFONT);
			break;

		//プレビュー
		case WID_CK_PREV:
			pdat->flags ^= DRAW_DRAWTEXT_F_PREVIEW;

			drawText_drawPreview(APP_DRAW);
			break;
		//ヘルプ
		case WID_BTT_HELP:
			mMessageBox(M_WINDOW(p), "help", M_TR_T2(TRGROUP_DLG_DRAWTEXT, TRID_HELP),
				MMESBOX_OK, MMESBOX_OK);
			break;
	}
}

/** イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	switch(ev->type)
	{
		case MEVENT_NOTIFY:
			_event_notify((_drawtext_dlg *)wg, ev);
			break;

		//タイマー (プレビュー更新)
		case MEVENT_TIMER:
			mWidgetTimerDelete(wg, 0);

			drawText_drawPreview(APP_DRAW);
			return 1;
	}

	return mDialogEventHandle_okcancel(wg, ev);
}


//==============================
// 作成
//==============================


/** ラベル作成 */

static void _create_label(mWidget *parent,uint16_t trid)
{
	mLabelCreate(parent, 0, MLF_MIDDLE, 0, M_TR_T(trid));
}

/** ラベル+コンボボックス作成 */

static mComboBox *_create_combo(mWidget *parent,uint16_t label_trid,int wid)
{
	_create_label(parent, label_trid);

	return mComboBoxCreate(parent, wid, 0, MLF_EXPAND_W, 0);
}

/** 字間 or 行間作成 */

static mLineEdit *_create_edit_space(mWidget *parent,uint16_t label_trid,int wid,int val)
{
	mLineEdit *edit;

	_create_label(parent, label_trid);

	edit = mLineEditCreate(parent, wid,
		MLINEEDIT_S_SPIN | MLINEEDIT_S_NOTIFY_CHANGE, 0, 0);

	mLineEditSetWidthByLen(edit, 4);
	mLineEditSetNumStatus(edit, -1000, 1000, 0);
	mLineEditSetNum(edit, val);

	return edit;
}

/** 設定項目のウィジェットを作成 */

static mWidget *_create_options_widget(_drawtext_dlg *p,mWidget *parent)
{
	DrawTextData *pdat;
	mWidget *ctv,*ctg,*ct;
	mComboBox *cb;

	pdat = &APP_DRAW->drawtext;

	//垂直コンテナ

	ctv = mContainerCreate(parent, MCONTAINER_TYPE_VERT, 0, 6, MLF_EXPAND_W);

	M_CONTAINER(ctv)->ct.padding.right = 5;

	//フォント名

	p->cb_family = mComboBoxCreate(ctv, WID_CB_FONTNAME, 0, MLF_EXPAND_W, 0);

	_set_family_list(p, pdat);

	//------

	//グリッドコンテナ

	ctg = mContainerCreateGrid(ctv, 2, 5, 6, MLF_EXPAND_W);

	//スタイル

	p->cb_style = _create_combo(ctg, TRID_STYLE, WID_CB_STYLE);

	mComboBoxAddItem(p->cb_style, "----", 1);

	_set_style_list(p, pdat);

	//太さ

	p->cb_weight = cb = _create_combo(ctg, TRID_WEIGHT, WID_CB_WEIGHT);

	mComboBoxAddTrItems(cb, 2, TRID_WEIGHT_TOP, 0);
	mComboBoxSetSel_index(cb, pdat->weight);

	//斜体

	p->cb_slant = cb = _create_combo(ctg, TRID_SLANT, WID_CB_SLANT);

	mComboBoxAddTrItems(cb, 3, TRID_SLANT_TOP, 0);
	mComboBoxSetSel_index(cb, pdat->slant);

	//

	_enable_weight_and_slant(p, mStrIsEmpty(&pdat->strStyle));

	//サイズ

	_create_label(ctg, TRID_SIZE);

	ct = mContainerCreate(ctg, MCONTAINER_TYPE_HORZ, 0, 5, MLF_EXPAND_W);

	p->edit_size = mLineEditCreate(ct, WID_EDIT_SIZE,
		MLINEEDIT_S_SPIN | MLINEEDIT_S_NOTIFY_CHANGE, MLF_EXPAND_W, 0);

	mLineEditSetNumStatus(p->edit_size, 1, 1000, 0);
	mLineEditSetInt(p->edit_size, pdat->size);

	cb = mComboBoxCreate(ct, WID_CB_SIZE_UNIT, 0, 0, 0);

	mComboBoxAddItems(cb, "pt\tpx", 0);
	mComboBoxSetWidthAuto(cb);
	
	mComboBoxSetSel_index(cb,
		((pdat->flags & DRAW_DRAWTEXT_F_SIZE_PIXEL) != 0));

	//ヒンティング

	p->cb_hinting = cb = _create_combo(ctg, TRID_HINTING, WID_CB_HINTING);

	mComboBoxAddTrItems(cb, 4, TRID_HINTING_TOP, 0);
	mComboBoxSetWidthAuto(cb);
	mComboBoxSetSel_index(cb, pdat->hinting);

	//字間

	p->edit_charsp = _create_edit_space(ctg, TRID_CHAR_SPACE, WID_EDIT_CHARSP, pdat->char_space);

	//行間

	p->edit_linesp = _create_edit_space(ctg, TRID_LINE_SPACE, WID_EDIT_LINESP, pdat->line_space);

	//縦書き

	mCheckButtonCreate(ctv, WID_CK_VERT, 0, 0, 0, M_TR_T(TRID_VERT),
		pdat->flags & DRAW_DRAWTEXT_F_VERT);

	//アンチエイリアス

	mCheckButtonCreate(ctv, WID_CK_ANTIALIAS, 0, 0, 0, M_TR_T(TRID_ANTIALIAS),
		pdat->flags & DRAW_DRAWTEXT_F_ANTIALIAS);

	return ctv;
}

/** 作成 */

static _drawtext_dlg *_create_dlg(mWindow *owner)
{
	_drawtext_dlg *p;
	mWidget *cth,*ct,*ct2,*wg;
	mContainerView *ctview;

	p = (_drawtext_dlg *)mDialogNew(sizeof(_drawtext_dlg), owner, MWINDOW_S_DIALOG_NORMAL);
	if(!p) return NULL;

	p->wg.event = _event_handle;

	//

	mContainerSetPadding_one(M_CONTAINER(p), 8);
	p->ct.sepW = 4;

	M_TR_G(TRGROUP_DLG_DRAWTEXT);

	mWindowSetTitle(M_WINDOW(p), M_TR_T(0));

	//======= ウィジェット

	cth = mContainerCreate(M_WIDGET(p), MCONTAINER_TYPE_HORZ, 0, 8, MLF_EXPAND_WH);

	//------ 右側

	ct = mContainerCreate(cth, MCONTAINER_TYPE_VERT, 0, 4, MLF_EXPAND_WH);

	//エディット

	p->medit = mMultiEditCreate(ct, WID_MEDIT, MMULTIEDIT_S_NOTIFY_CHANGE, MLF_EXPAND_WH, 0);

	p->medit->wg.event = _multiedit_event_handle;

	//ボタン

	ct2 = mContainerCreate(ct, MCONTAINER_TYPE_HORZ, 0, 4, MLF_EXPAND_W);

	wg = (mWidget *)mButtonCreate(ct2, M_WID_OK, 0, 0, 0, M_TR_T2(M_TRGROUP_SYS, M_TRSYS_OK));
	wg->fState |= MWIDGET_STATE_ENTER_DEFAULT;
	
	mButtonCreate(ct2, M_WID_CANCEL, 0, 0, 0, M_TR_T2(M_TRGROUP_SYS, M_TRSYS_CANCEL));
	mButtonCreate(ct2, WID_BTT_HELP, MBUTTON_S_REAL_W, 0, 0, "?");

	//プレビュー

	mCheckButtonCreate(ct2, WID_CK_PREV, 0, MLF_EXPAND_X | MLF_RIGHT | MLF_MIDDLE, 0, M_TR_T(TRID_PREVIEW),
		APP_DRAW->drawtext.flags & DRAW_DRAWTEXT_F_PREVIEW);

	//----- 左側 (項目)

	ctview = mContainerViewNew(0, cth, 0);
	ctview->wg.fLayout = MLF_EXPAND_H;

	ctview->cv.area = _create_options_widget(p, M_WIDGET(ctview));

	//=========

	//フォントが未作成の場合は、作成

	if(!APP_DRAW->drawtext.font)
		drawText_createFont();

	//前回のテキストがある場合

	if(!mStrIsEmpty(&APP_DRAW->drawtext.strText))
	{
		mWidgetSetFocus(M_WIDGET(p->medit));

		mMultiEditSetText(p->medit, APP_DRAW->drawtext.strText.buf);
		mMultiEditSelectAll(p->medit);

		_update(p, 0);
	}

	return p;
}


//==============================


/** テキストダイアログ */

mBool DrawTextDlg_run(mWindow *owner)
{
	_drawtext_dlg *p;
	mBox box;
	mBool ret;

	p = _create_dlg(owner);
	if(!p) return FALSE;

	//ウィンドウ状態セット

	box = APP_CONF->box_textdlg;

	if(box.w == 0 || box.h == 0)
	{
		mWidgetSetInitSize_fontHeight(M_WIDGET(p->medit), 20, 16);
		mWindowMoveResizeShow_hintSize(M_WINDOW(p));
	}
	else
	{
		mWidgetMoveResize(M_WIDGET(p), box.x, box.y, box.w, box.h);
		mWidgetShow(M_WIDGET(p), 1);
	}

	//実行

	ret = mDialogRun(M_DIALOG(p), FALSE);

	//ウィンドウ状態保存

	mWindowGetSaveBox(M_WINDOW(p), &APP_CONF->box_textdlg);

	mWidgetDestroy(M_WIDGET(p));

	return ret;
}

