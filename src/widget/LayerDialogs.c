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
 * レイヤ関連ダイアログ
 *
 * [新規作成] [設定] [レイヤタイプ変更]
 * [複数レイヤ結合]
 *****************************************/

#include "mDef.h"
#include "mWidget.h"
#include "mDialog.h"
#include "mContainer.h"
#include "mWindow.h"
#include "mWidgetBuilder.h"
#include "mCheckButton.h"
#include "mColorButton.h"
#include "mLineEdit.h"
#include "mComboBox.h"
#include "mMessageBox.h"
#include "mMenu.h"
#include "mSysDialog.h"
#include "mTrans.h"
#include "mEvent.h"
#include "mStr.h"
#include "mUtilStr.h"

#include "LayerDialogs.h"

#include "defConfig.h"
#include "defBlendMode.h"

#include "trgroup.h"
#include "trid_word.h"
#include "trid_layer_dialogs.h"




//************************************
// 共通処理
//************************************


/** レイヤ名初期化 */

static mLineEdit *_initwg_layername(mWidget *root,int id,const char *defname)
{
	mLineEdit *le;

	le = (mLineEdit *)mWidgetFindByID(root, id);

	le->wg.initW = mWidgetGetFontHeight(M_WIDGET(le)) * 15;

	mWidgetSetFocus(M_WIDGET(le));

	mLineEditSetText(le, defname);
	mLineEditSelectAll(le);

	return le;
}

/** 合成モード初期化 */

static mComboBox *_initwg_blendmode(mWidget *root,int id,int def)
{
	mComboBox *cb;

	M_TR_G(TRGROUP_BLENDMODE);

	cb = (mComboBox *)mWidgetFindByID(root, id);

	mComboBoxAddTrItems(cb, BLENDMODE_NUM, 0, 0);
	mComboBoxSetWidthAuto(cb);
	mComboBoxSetSel_index(cb, def);

	return cb;
}

/** 線の色ボタン初期化 */

static mColorButton *_initwg_colorbtt(mWidget *root,int id,uint32_t col,mBool enable)
{
	mColorButton *btt;

	btt = (mColorButton *)mWidgetFindByID(root, id);

	mColorButtonSetColor(btt, col);
	if(!enable) mWidgetEnable(M_WIDGET(btt), 0);

	return btt;
}

/** 色ボタンが押された時 */

static void _onpress_colorbtt(mWindow *owner,mColorButton *btt)
{
	uint32_t col = mColorButtonGetColor(btt);
	
	if(LayerColorDlg_run(owner, &col))
		mColorButtonSetColor(btt, col);
}

/** 名前リストボタンが押された時 */

static void _onpress_namelistbtt(mWindow *win,mWidget *wgbtt,mLineEdit *ledit)
{
	mMenu *menu;
	mMenuItemInfo mi,*pmi;
	int no = 0;
	mBox box;
	mStr str = MSTR_INIT;
	const char *pctop,*pcend;

	M_TR_G(TRGROUP_DLG_LAYER);

	//メニュー作成

	menu = mMenuNew();

	mMemzero(&mi, sizeof(mMenuItemInfo));

	pctop = APP_CONF->strLayerNameList.buf;

	while(mGetStrNextSplit(&pctop, &pcend, ';') && no < 100)
	{
		if(no < 10)
			//0-9
			mStrSetFormat(&str, "&%d %*s", no, pcend - pctop, pctop);
		else if(no < 10 + 26)
			//a-z
			mStrSetFormat(&str, "&%c %*s", no - 10 + 'A', pcend - pctop, pctop);
		else
			mStrSetTextLen(&str, pctop, pcend - pctop);

		mi.id = no;
		mi.label = str.buf;
		mi.flags = MMENUITEM_F_LABEL_COPY;
		mi.param1 = (intptr_t)pctop;
		mi.param2 = pcend - pctop;

		mMenuAdd(menu, &mi);
	
		pctop = pcend;
		no++;
	}

	if(no != 0) mMenuAddSep(menu);
	mMenuAddText_static(menu, 1000, M_TR_T(TRID_LAYERDLGS_EDITNAMELIST_MENU));

	//メニュー実行

	mWidgetGetRootBox(wgbtt, &box);

	pmi = mMenuPopup(menu, NULL, box.x + box.w, box.y + box.h, MMENU_POPUP_F_RIGHT);

	mMenuDestroy(menu);

	//処理

	if(pmi)
	{
		if(pmi->id == 1000)
		{
			//編集

			M_TR_G(TRGROUP_DLG_LAYER);

			mSysDlgInputText(win, M_TR_T(TRID_LAYERDLGS_EDITNAMELIST_TITLE),
				M_TR_T(TRID_LAYERDLGS_EDITNAMELIST_MES), &APP_CONF->strLayerNameList, 0);
		}
		else
		{
			//呼び出し

			mStrSetTextLen(&str, (const char *)pmi->param1, pmi->param2);

			mLineEditSetText(ledit, str.buf);
		}
	}

	mStrFree(&str);
}


//************************************
// 新規作成
//************************************


//----------------------

typedef struct
{
	mWidget wg;
	mContainerData ct;
	mWindowData win;
	mDialogData dlg;

	mLineEdit *le_name;
	mCheckButton *ck_type;
	mComboBox *cb_blend;
	mColorButton *colbt;
	mWidget *wg_namebtt;
}_dlg_new;

//----------------------

static const char *g_build_new =
"ct#g2:lf=w:sep=7,7;"
  //名前
  "lb:lf=mr:TR=0,8;"
  "ct#h:lf=w:sep=5;"
    "le:lf=wm:id=100;"
    "arrbt#d:lf=m:id=104;"
  "-;"
  //タイプ
  "lb:lf=r:tr=100;"
  "ct#v:sep=3:id=101;"
  "-;"
  //合成モード
  "lb:lf=rm:tr=101;"
  "cb:id=102;"
  //線の色
  "lb:lf=mr:tr=102;"
  "colbt:id=103;"
;

//----------------------

enum
{
	WID_NEW_EDIT_NAME = 100,
	WID_NEW_CT_TYPE,
	WID_NEW_CB_BLEND,
	WID_NEW_COLBTT,
	WID_NEW_BTT_NAMELIST,
	WID_NEW_CK_TYPE_TOP = 200
};

//----------------------


/** イベント */

static int _newdlg_event_handle(mWidget *wg,mEvent *ev)
{
	_dlg_new *p = (_dlg_new *)wg;
	int id;

	if(ev->type == MEVENT_NOTIFY)
	{
		id = ev->notify.id;

		if(id >= WID_NEW_CK_TYPE_TOP && id < WID_NEW_CK_TYPE_TOP + 5)
		{
			//タイプ選択変更 => 合成モードと色を有効/無効

			id -= WID_NEW_CK_TYPE_TOP;

			mWidgetEnable(M_WIDGET(p->cb_blend), (id != 4));
			mWidgetEnable(M_WIDGET(p->colbt), (id == 2 || id == 3));
		}
		else if(id == WID_NEW_COLBTT
			&& ev->notify.type == MCOLORBUTTON_N_PRESS)
		{
			//色ボタン => 色選択

			_onpress_colorbtt(M_WINDOW(p), p->colbt);
		}
		else if(id == WID_NEW_BTT_NAMELIST)
		{
			//名前リスト

			_onpress_namelistbtt(M_WINDOW(p), p->wg_namebtt, p->le_name);
		}
	}

	return mDialogEventHandle_okcancel(wg, ev);
}

/** 作成 */

static _dlg_new *_newdlg_create(mWindow *owner,LayerNewDlgInfo *info)
{
	_dlg_new *p;
	mWidget *ct;
	mCheckButton *ck;
	int i;

	p = (_dlg_new *)mDialogNew(sizeof(_dlg_new), owner, MWINDOW_S_DIALOG_NORMAL);
	if(!p) return NULL;

	p->wg.event = _newdlg_event_handle;

	//

	mContainerSetPadding_one(M_CONTAINER(p), 8);

	p->ct.sepW = 18;

	M_TR_G(TRGROUP_DLG_LAYER);

	mWindowSetTitle(M_WINDOW(p), M_TR_T(TRID_LAYERDLGS_TITLE_NEW));

	//ウィジェット

	mWidgetBuilderCreateFromText(M_WIDGET(p), g_build_new);

	p->le_name = _initwg_layername(M_WIDGET(p), WID_NEW_EDIT_NAME, info->strname.buf);
	p->cb_blend = _initwg_blendmode(M_WIDGET(p), WID_NEW_CB_BLEND, 0);
	p->colbt = _initwg_colorbtt(M_WIDGET(p), WID_NEW_COLBTT, 0, FALSE);
	p->wg_namebtt = mWidgetFindByID(M_WIDGET(p), WID_NEW_BTT_NAMELIST);

	//カラータイプ

	M_TR_G(TRGROUP_LAYERTYPE);

	ct = mWidgetFindByID(M_WIDGET(p), WID_NEW_CT_TYPE);

	for(i = 0; i < 4; i++)
	{
		ck = mCheckButtonCreate(ct, WID_NEW_CK_TYPE_TOP + i, MCHECKBUTTON_S_RADIO, 0, 0, M_TR_T(i), FALSE);

		if(i == 0) p->ck_type = ck;
	}

	mCheckButtonCreate(ct, WID_NEW_CK_TYPE_TOP + 4, MCHECKBUTTON_S_RADIO, 0, 0,
		M_TR_T2(TRGROUP_WORD, TRID_WORD_FOLDER), FALSE);

	mCheckButtonSetRadioSel(p->ck_type, 0);

	//OK/cancel

	mContainerCreateOkCancelButton(M_WIDGET(p));

	return p;
}

/** レイヤ新規作成ダイアログ
 *
 * strname にデフォルトのレイヤ名を入れておく。
 * coltype は -1 でフォルダ。 */

mBool LayerNewDlg_run(mWindow *owner,LayerNewDlgInfo *info)
{
	_dlg_new *p;
	int ret;

	p = _newdlg_create(owner, info);
	if(!p) return FALSE;

	mWindowMoveResizeShow_hintSize(M_WINDOW(p));

	ret = mDialogRun(M_DIALOG(p), FALSE);

	if(ret)
	{
		mLineEditGetTextStr(p->le_name, &info->strname);

		info->blendmode = mComboBoxGetSelItemIndex(p->cb_blend);
		info->col = mColorButtonGetColor(p->colbt);

		info->coltype = mCheckButtonGetGroupSelIndex(p->ck_type);
		if(info->coltype == 4) info->coltype = -1;
	}

	mWidgetDestroy(M_WIDGET(p));

	return ret;
}


//************************************
// レイヤ設定
//************************************


//----------------------

typedef struct
{
	mWidget wg;
	mContainerData ct;
	mWindowData win;
	mDialogData dlg;

	mLineEdit *le_name;
	mComboBox *cb_blend;
	mColorButton *colbt;
	mWidget *wg_namebtt;
}_dlg_option;

//----------------------

static const char *g_build_option =
"ct#v:lf=w:sep=10;"
  "ct#g2:lf=w:sep=7,7;"
	//名前
    "lb:lf=mr:TR=0,8;"
    "ct#h:lf=w:sep=5;"
      "le:lf=wm:id=100;"
      "arrbt#d:lf=m:id=104;"
    "-;"
    //合成モード
	"lb:lf=mr:tr=101;"
	"cb:id=101;"
    //色
    "lb:lf=mr:tr=102;"
    "ct#h:sep=10;"
      "colbt:id=102;"
      "bt#wh:lf=m:id=103:t=?;"
;

//----------------------

enum
{
	WID_OPT_EDIT_NAME = 100,
	WID_OPT_CB_BLEND,
	WID_OPT_COLBT,
	WID_OPT_COLOR_HELP,
	WID_OPT_BTT_NAMELIST
};

//----------------------


/** イベント */

static int _optdlg_event_handle(mWidget *wg,mEvent *ev)
{
	_dlg_option *p = (_dlg_option *)wg;

	if(ev->type == MEVENT_NOTIFY)
	{
		switch(ev->notify.id)
		{
			//色ボタン
			case WID_OPT_COLBT:
				if(ev->notify.type == MCOLORBUTTON_N_PRESS)
					_onpress_colorbtt(M_WINDOW(p), p->colbt);
				break;
			//ヘルプ
			case WID_OPT_COLOR_HELP:
				mMessageBox(M_WINDOW(p), "tips",
					M_TR_T2(TRGROUP_DLG_LAYER, TRID_LAYERDLGS_LAYERCOLOR_TIPS),
					MMESBOX_OK, MMESBOX_OK);
				break;
			//レイヤ名リスト
			case WID_OPT_BTT_NAMELIST:
				_onpress_namelistbtt(M_WINDOW(p), p->wg_namebtt, p->le_name);
				break;
		}
	}

	return mDialogEventHandle_okcancel(wg, ev);
}

/** 作成 */

static _dlg_option *_optdlg_create(mWindow *owner,
	const char *name,int *blendmode,uint32_t *col)
{
	_dlg_option *p;

	p = (_dlg_option *)mDialogNew(sizeof(_dlg_option), owner, MWINDOW_S_DIALOG_NORMAL);
	if(!p) return NULL;

	p->wg.event = _optdlg_event_handle;

	//

	mContainerSetPadding_one(M_CONTAINER(p), 8);

	p->ct.sepW = 18;

	M_TR_G(TRGROUP_DLG_LAYER);

	mWindowSetTitle(M_WINDOW(p), M_TR_T(TRID_LAYERDLGS_TITLE_OPTION));

	//ウィジェット

	mWidgetBuilderCreateFromText(M_WIDGET(p), g_build_option);

	p->le_name = _initwg_layername(M_WIDGET(p), WID_OPT_EDIT_NAME, name);
	p->cb_blend = _initwg_blendmode(M_WIDGET(p), WID_OPT_CB_BLEND, (blendmode)? *blendmode: 0);
	p->colbt = _initwg_colorbtt(M_WIDGET(p), WID_OPT_COLBT, (col)? *col: 0, (col != 0));
	p->wg_namebtt = mWidgetFindByID(M_WIDGET(p), WID_NEW_BTT_NAMELIST);

	if(!blendmode)
		mWidgetEnable(M_WIDGET(p->cb_blend), 0);

	//OK/cancel

	mContainerCreateOkCancelButton(M_WIDGET(p));

	return p;
}

/** レイヤ設定ダイアログ
 *
 * @param blendmode NULL でフォルダ
 * @param col   NULL で色なし */

mBool LayerOptionDlg_run(mWindow *owner,mStr *strname,int *blendmode,uint32_t *col)
{
	_dlg_option *p;
	int ret;

	p = _optdlg_create(owner, strname->buf, blendmode, col);
	if(!p) return FALSE;

	mWindowMoveResizeShow_hintSize(M_WINDOW(p));

	ret = mDialogRun(M_DIALOG(p), FALSE);

	if(ret)
	{
		mLineEditGetTextStr(p->le_name, strname);

		if(blendmode)
			*blendmode = mComboBoxGetSelItemIndex(p->cb_blend);

		if(col)
			*col = mColorButtonGetColor(p->colbt);
	}

	mWidgetDestroy(M_WIDGET(p));

	return ret;
}


//************************************
// レイヤタイプ変更
//************************************


//----------------------

typedef struct
{
	mWidget wg;
	mContainerData ct;
	mWindowData win;
	mDialogData dlg;

	mBool have_color;	//現在のカラータイプが色を持つタイプか

	mCheckButton *ck_type,
		*ck_lumtoa;
}_dlg_type;

//----------------------

#define WID_TYPE_CK_TYPE_TOP 100

//----------------------


/** イベント */

static int _typedlg_event_handle(mWidget *wg,mEvent *ev)
{
	_dlg_type *p = (_dlg_type *)wg;

	if(ev->type == MEVENT_NOTIFY
		&& ev->notify.id >= WID_TYPE_CK_TYPE_TOP && ev->notify.id < WID_TYPE_CK_TYPE_TOP + 4)
	{
		//タイプ選択変更
		//フルカラー/GRAY => A16/A1 へ変更する場合のみ有効

		mWidgetEnable(M_WIDGET(p->ck_lumtoa),
			(p->have_color && ev->notify.id - WID_TYPE_CK_TYPE_TOP >= 2) );
	}

	return mDialogEventHandle_okcancel(wg, ev);
}

/** 作成 */

static _dlg_type *_typedlg_create(mWindow *owner,int curtype)
{
	_dlg_type *p;
	mWidget *ct;
	mCheckButton *ckbtt;
	int i;

	p = (_dlg_type *)mDialogNew(sizeof(_dlg_type), owner, MWINDOW_S_DIALOG_NORMAL);
	if(!p) return NULL;

	p->have_color = (curtype < 2);

	p->wg.event = _typedlg_event_handle;
	p->ct.sepW = 5;

	mContainerSetPadding_one(M_CONTAINER(p), 8);

	//--------

	//タイプ

	M_TR_G(TRGROUP_LAYERTYPE);

	for(i = 0; i < 4; i++)
	{
		ckbtt = mCheckButtonCreate(M_WIDGET(p),
			WID_TYPE_CK_TYPE_TOP + i,
			MCHECKBUTTON_S_RADIO, 0, 0, M_TR_T(i), FALSE);

		//現在のタイプは選択不可
		
		if(i == curtype)
			mWidgetEnable(M_WIDGET(ckbtt), 0);

		if(i == 0) p->ck_type = ckbtt;
	}

	//初期選択は、現在がフルカラーならGRAY、それ以外はフルカラー
	mCheckButtonSetRadioSel(p->ck_type, (curtype == 0));

	//輝度をアルファ値に

	M_TR_G(TRGROUP_DLG_LAYER);

	p->ck_lumtoa = mCheckButtonCreate(M_WIDGET(p), 0,
		0, 0, M_MAKE_DW4(0,3,0,0),
		M_TR_T(TRID_LAYERDLGS_LUM_TO_ALPHA), FALSE);

	mWidgetEnable(M_WIDGET(p->ck_lumtoa), 0);

	mWindowSetTitle(M_WINDOW(p), M_TR_T(TRID_LAYERDLGS_TITLE_SETTYPE));

	//OK/cancel

	ct = mContainerCreateOkCancelButton(M_WIDGET(p));
	ct->margin.top = 8;

	return p;
}

/** レイヤタイプ変更ダイアログ
 *
 * @return -1 でキャンセル、下位8ビットがタイプ、9bit が色から変換 */

int LayerTypeDlg_run(mWindow *owner,int curtype)
{
	_dlg_type *p;
	int ret = -1;

	p = _typedlg_create(owner, curtype);
	if(!p) return -1;

	mWindowMoveResizeShow_hintSize(M_WINDOW(p));

	if(mDialogRun(M_DIALOG(p), FALSE))
	{
		ret = mCheckButtonGetGroupSelIndex(p->ck_type);

		if(mCheckButtonIsChecked(p->ck_lumtoa))
			ret |= 1<<9;
	}

	mWidgetDestroy(M_WIDGET(p));

	return ret;
}


//************************************
// 複数レイヤ結合
//************************************


//----------------------

typedef struct
{
	mWidget wg;
	mContainerData ct;
	mWindowData win;
	mDialogData dlg;

	mBool have_checked;

	mCheckButton *ck_target[3],*ck_new;
	mComboBox *cb_type;
}_dlg_combine;

//----------------------

static const char *g_build_combine =
"ct#v:sep=4;"
  "ck#r:id=100:tr=200;"
  "ck#r:id=101:tr=201;"
  "ck#r:id=102:tr=202;"
  "ck:id=103:tr=106:mg=t5;"
  "ct#h:sep=7:mg=t8;"
    "lb:lf=m:tr=107;"
    "cb:id=104;"
  "-;"
  "lb#B:tr=108:mg=t10;";

//----------------------

enum
{
	WID_COMBINE_TARGET_ALL = 100,
	WID_COMBINE_TARGET_FOLDER,
	WID_COMBINE_TARGET_CHECKED,
	WID_COMBINE_NEWLAYER,
	WID_COMBINE_CB_TYPE
};

//----------------------


/** イベント */

static int _combinedlg_event_handle(mWidget *wg,mEvent *ev)
{
	_dlg_combine *p = (_dlg_combine *)wg;

	if(ev->type == MEVENT_NOTIFY)
	{
		switch(ev->notify.id)
		{
			//新規レイヤ結合
			case WID_COMBINE_NEWLAYER:
				if(p->have_checked)
				{
					mWidgetEnable(M_WIDGET(p->ck_target[2]), ev->notify.param1);

					//新規レイヤが OFF で、チェックされたレイヤが ON の場合
					
					if(!ev->notify.param1 && mCheckButtonIsChecked(p->ck_target[2]))
						mCheckButtonSetState(p->ck_target[0], 1);
				}
				break;
		}
	}

	return mDialogEventHandle_okcancel(wg, ev);
}

/** 作成 */

static _dlg_combine *_combinedlg_create(mWindow *owner,mBool cur_folder)
{
	_dlg_combine *p;
	int i;

	p = (_dlg_combine *)mDialogNew(sizeof(_dlg_combine), owner, MWINDOW_S_DIALOG_NORMAL);
	if(!p) return NULL;

	p->wg.event = _combinedlg_event_handle;

	//

	mContainerSetPadding_one(M_CONTAINER(p), 8);

	p->ct.sepW = 18;

	M_TR_G(TRGROUP_DLG_LAYER);

	mWindowSetTitle(M_WINDOW(p), M_TR_T(TRID_LAYERDLGS_TITLE_COMBINE));

	//----- ウィジェット

	mWidgetBuilderCreateFromText(M_WIDGET(p), g_build_combine);

	//対象

	for(i = 0; i < 3; i++)
		p->ck_target[i] = (mCheckButton *)mWidgetFindByID(M_WIDGET(p), WID_COMBINE_TARGET_ALL + i);

	if(!cur_folder)
		mWidgetEnable(M_WIDGET(p->ck_target[1]), 0);

	mWidgetEnable(M_WIDGET(p->ck_target[2]), 0);

	mCheckButtonSetState(p->ck_target[0], 1);

	//

	p->ck_new = (mCheckButton *)mWidgetFindByID(M_WIDGET(p), WID_COMBINE_NEWLAYER);

	//カラータイプ
	
	p->cb_type = (mComboBox *)mWidgetFindByID(M_WIDGET(p), WID_COMBINE_CB_TYPE);

	M_TR_G(TRGROUP_LAYERTYPE);

	mComboBoxAddTrItems(p->cb_type, 4, 0, 0);
	mComboBoxSetWidthAuto(p->cb_type);
	mComboBoxSetSel_index(p->cb_type, 0);

	//OK/cancel

	mContainerCreateOkCancelButton(M_WIDGET(p));

	return p;
}

/** 複数レイヤ結合ダイアログ
 *
 * @param have_checked チェックされたレイヤがあるか
 * @return -1 でキャンセル。下位2bit:対象、1bit:新規レイヤ、2bit:レイヤタイプ */

int LayerCombineDlg_run(mWindow *owner,mBool cur_folder,mBool have_checked)
{
	_dlg_combine *p;
	int ret = -1;

	p = _combinedlg_create(owner, cur_folder);
	if(!p) return FALSE;

	p->have_checked = have_checked;

	mWindowMoveResizeShow_hintSize(M_WINDOW(p));

	if(mDialogRun(M_DIALOG(p), FALSE))
	{
		ret = mCheckButtonGetGroupSelIndex(p->ck_target[0]);

		if(mCheckButtonIsChecked(p->ck_new)) ret |= 1<<2;

		ret |= mComboBoxGetSelItemIndex(p->cb_type) << 3;
	}

	mWidgetDestroy(M_WIDGET(p));

	return ret;
}
