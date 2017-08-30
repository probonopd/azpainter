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
 * 環境設定ダイアログ
 *****************************************/

#include <string.h>

#include "mDef.h"
#include "mWidget.h"
#include "mDialog.h"
#include "mContainer.h"
#include "mContainerView.h"
#include "mWindow.h"
#include "mListView.h"
#include "mTrans.h"
#include "mEvent.h"
#include "mStr.h"
#include "mMessageBox.h"
#include "mUtilFile.h"

#include "defConfig.h"

#include "Undo.h"

#include "defDrawGlobal.h"
#include "draw_main.h"

#include "trgroup.h"

#include "EnvOptDlg.h"
#include "EnvOptDlg_pv.h"


//----------------------

typedef struct
{
	mWidget wg;
	mContainerData ct;
	mWindowData win;
	mDialogData dlg;

	mContainerView *ctview;	//内容の親
	mWidget *contents;		//項目のコンテナ (param に値取得関数のポインタ)
	int tabno;			//現在のタブ番号

	EnvOptDlgCurData dat;
}_dialog;

//----------------------

#define WID_LIST  100

//----------------------

enum
{
	TABNO_OPT1,
	TABNO_OPT2,
	TABNO_STEP,
	TABNO_BTT,
	TABNO_SYSTEM,
	TABNO_CURSOR,
	TABNO_INTERFACE,

	TAB_NUM
};

//----------------------

mWidget *EnvOptDlg_create_option1(mWidget *parent,EnvOptDlgCurData *dat);
mWidget *EnvOptDlg_create_option2(mWidget *parent,EnvOptDlgCurData *dat);
mWidget *EnvOptDlg_create_step(mWidget *parent,EnvOptDlgCurData *dat);
mWidget *EnvOptDlg_create_interface(mWidget *parent,EnvOptDlgCurData *dat);
mWidget *EnvOptDlg_create_system(mWidget *parent,EnvOptDlgCurData *dat);
mWidget *EnvOptDlg_create_cursor(mWidget *parent,EnvOptDlgCurData *dat);

mWidget *EnvOptDlg_create_btt(mWidget *parent,EnvOptDlgCurData *dat);

//----------------------


//=============================
// EnvOptDlgCurData
//=============================


/** 設定データから作業用データにセット */

static void _config_to_curdata(EnvOptDlgCurData *pd,ConfigData *ps)
{
	//設定1

	pd->canvcol[0] = ps->colCanvasBkgnd;
	pd->canvcol[1] = ps->colBkgndPlaid[0];
	pd->canvcol[2] = ps->colBkgndPlaid[1];

	pd->initw = ps->init_imgw;
	pd->inith = ps->init_imgh;
	pd->init_dpi = ps->init_dpi;
	pd->undo_maxnum = ps->undo_maxnum;
	pd->undo_maxbufsize = ps->undo_maxbufsize;

	//設定2

	pd->optflags = ps->optflags;

	//増減幅

	pd->step[0] = ps->canvasZoomStep_low;
	pd->step[1] = ps->canvasZoomStep_hi;
	pd->step[2] = ps->canvasAngleStep;
	pd->step[3] = ps->dragBrushSize_step;

	//操作

	memcpy(pd->default_btt_cmd, ps->default_btt_cmd, CONFIG_POINTERBTT_NUM);
	memcpy(pd->pentab_btt_cmd, ps->pentab_btt_cmd, CONFIG_POINTERBTT_NUM);

	//インターフェイス

	mStrCopy(&pd->strThemeFile, &ps->strThemeFile);

	pd->iconsize[0] = ps->iconsize_toolbar;
	pd->iconsize[1] = ps->iconsize_layer;
	pd->iconsize[2] = ps->iconsize_other;

	pd->toolbar_btts = mMemdup(ps->toolbar_btts, ps->toolbar_btts_size);
	pd->toolbar_btts_size = ps->toolbar_btts_size;
	
	//システム

	mStrCopy(pd->strSysDir, &ps->strTempDir);
	mStrCopy(pd->strSysDir + 1, &ps->strUserBrushDir);
	mStrCopy(pd->strSysDir + 2, &ps->strUserTextureDir);
	
	mStrCopy(pd->strFontStyle, &ps->strFontStyle_gui);
	mStrCopy(pd->strFontStyle + 1, &ps->strFontStyle_dock);

	//カーソル

	pd->cursor_buf = mMemdup(ps->cursor_buf, ps->cursor_bufsize);
	pd->cursor_bufsize = ps->cursor_bufsize;
}

/** 作業用データから設定データにセット
 *
 * @return 更新フラグ */

static int _curdata_to_config(ConfigData *cf,EnvOptDlgCurData *ps)
{
	int ret = 0;

	//------- 値の変化

	//キャンバス背景色

	if(cf->colCanvasBkgnd != ps->canvcol[0])
		ret |= ENVOPTDLG_F_UPDATE_CANVAS;

	//チェック柄背景色変更による更新

	if((cf->fView & CONFIG_VIEW_F_BKGND_PLAID)
		&& (cf->colBkgndPlaid[0] != ps->canvcol[1]
			|| cf->colBkgndPlaid[1] != ps->canvcol[2]))
		ret |= ENVOPTDLG_F_UPDATE_ALL;

	//アイコンサイズ

	if(cf->iconsize_toolbar != ps->iconsize[0]
		|| cf->iconsize_layer != ps->iconsize[1]
		|| cf->iconsize_other != ps->iconsize[2])
		ret |= ENVOPTDLG_F_ICONSIZE;

	//テーマ

	if(!mStrCompareEq(&cf->strThemeFile, ps->strThemeFile.buf))
		ret |= ENVOPTDLG_F_THEME;

	//--------- 値のセット

	//設定1

	cf->colCanvasBkgnd = ps->canvcol[0];
	cf->colBkgndPlaid[0] = ps->canvcol[1];
	cf->colBkgndPlaid[1] = ps->canvcol[2];

	cf->init_imgw = ps->initw;
	cf->init_imgh = ps->inith;
	cf->init_dpi = ps->init_dpi;
	cf->undo_maxnum = ps->undo_maxnum;
	cf->undo_maxbufsize = ps->undo_maxbufsize;

	//設定2

	cf->optflags = ps->optflags;

	//増減幅

	cf->canvasZoomStep_low = ps->step[0];
	cf->canvasZoomStep_hi = ps->step[1];
	cf->canvasAngleStep = ps->step[2];
	cf->dragBrushSize_step = ps->step[3];

	//操作

	memcpy(cf->default_btt_cmd, ps->default_btt_cmd, CONFIG_POINTERBTT_NUM);
	memcpy(cf->pentab_btt_cmd, ps->pentab_btt_cmd, CONFIG_POINTERBTT_NUM);

	//インターフェイス

	mStrCopy(&cf->strThemeFile, &ps->strThemeFile);

	cf->iconsize_toolbar = ps->iconsize[0];
	cf->iconsize_layer = ps->iconsize[1];
	cf->iconsize_other = ps->iconsize[2];

	if(ps->fchange & ENVOPTDLG_CHANGE_F_TOOLBAR_BTTS)
	{
		mFree(cf->toolbar_btts);

		cf->toolbar_btts = mMemdup(ps->toolbar_btts, ps->toolbar_btts_size);
		cf->toolbar_btts_size = ps->toolbar_btts_size;

		ret |= ENVOPTDLG_F_TOOLBAR_BTT;
	}

	//システム

	mStrCopy(&cf->strTempDir, ps->strSysDir);
	mStrCopy(&cf->strUserBrushDir, ps->strSysDir + 1);
	mStrCopy(&cf->strUserTextureDir, ps->strSysDir + 2);

	mStrCopy(&cf->strFontStyle_gui, ps->strFontStyle);
	mStrCopy(&cf->strFontStyle_dock, ps->strFontStyle + 1);

	//カーソル

	if(ps->fchange & ENVOPTDLG_CHANGE_F_CURSOR)
	{
		mFree(cf->cursor_buf);

		cf->cursor_buf = mMemdup(ps->cursor_buf, ps->cursor_bufsize);
		cf->cursor_bufsize = ps->cursor_bufsize;

		ret |= ENVOPTDLG_F_CURSOR;
	}

	//-------- 値変更によるセット

	//チェック柄色変更 (RGBAFix15 に変換する必要あり)

	drawConfig_changeCanvasColor(APP_DRAW);

	//アンドゥ回数セット

	Undo_setMaxNum(cf->undo_maxnum);

	return ret;
}


//=============================
// sub
//=============================


/** 内容の基本形を作成 (デフォルトで垂直コンテナ) */

mWidget *EnvOptDlg_createContentsBase(mWidget *parent,int size,EnvOptDlg_funcGetValue func)
{
	mContainer *p;
	
	p = mContainerNew(size, parent);

	p->wg.notifyTarget = MWIDGET_NOTIFYTARGET_SELF;
	p->wg.param = (intptr_t)func;
	p->ct.sepW = 6;
	p->ct.padding.right = 4;

	return (mWidget *)p;
}


//=============================
// main
//=============================


/** 内容ウィジェットからデータ取得 */

static void _getdat_current(_dialog *p)
{
	if(p->contents->param)
		((EnvOptDlg_funcGetValue)p->contents->param)(p->contents, &p->dat);
}

/** 内容ウィジェット作成
 *
 * @param init  初期作成時 TRUE */

static void _create_contents(_dialog *p,mBool init)
{
	mWidget *(*create[])(mWidget *,EnvOptDlgCurData *) = {
		EnvOptDlg_create_option1, EnvOptDlg_create_option2,
		EnvOptDlg_create_step, EnvOptDlg_create_btt,
		EnvOptDlg_create_system, EnvOptDlg_create_cursor,
		EnvOptDlg_create_interface
	};

	//現在のウィジェットを削除

	if(p->contents)
	{
		//値取得

		_getdat_current(p);
	
		mWidgetDestroy(p->contents);
		p->contents = NULL;
	}

	//作成

	M_TR_G(TRGROUP_DLG_ENV_OPTION);

	p->contents = (create[p->tabno])(M_WIDGET(p->ctview), &p->dat);

	p->ctview->cv.area = p->contents;

	//再レイアウト

	if(!init)
	{
		mWidgetReLayout(M_WIDGET(p));

		if(p->wg.w < p->wg.hintW)
			mWidgetResize(M_WIDGET(p), p->wg.hintW, p->wg.h);
	}
}

/** イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY)
	{
		_dialog *p = (_dialog *)wg;
		
		switch(ev->notify.id)
		{
			//リスト選択変更 -> 項目ウィジェット作成
			case WID_LIST:
				if(ev->notify.type == MLISTVIEW_N_CHANGE_FOCUS)
				{
					p->tabno = ev->notify.param2;
					_create_contents(p, FALSE);
				}
				break;
			
			//OK
			case M_WID_OK:
				//現在の内容データを取得

				_getdat_current(p);

				//作業ディレクトリが存在するか

				if(!mIsFileExist(p->dat.strSysDir[0].buf, TRUE))
				{
					mMessageBoxErrTr(M_WINDOW(wg), TRGROUP_DLG_ENV_OPTION, TRID_MES_TEMPDIR);
					break;
				}
				
				mDialogEnd(M_DIALOG(wg), _curdata_to_config(APP_CONF, &p->dat));
				break;
			//キャンセル
			case M_WID_CANCEL:
				mDialogEnd(M_DIALOG(wg), 0);
				break;
		}
	}

	return mDialogEventHandle(wg, ev);
}

/** 破棄ハンドラ */

static void _destroy_handle(mWidget *wg)
{
	EnvOptDlgCurData *p = &((_dialog *)wg)->dat;

	mStrArrayFree(p->strSysDir, 3);

	mStrFree(&p->strThemeFile);
	mStrFree(&p->strFontStyle[0]);
	mStrFree(&p->strFontStyle[1]);
	
	mFree(p->cursor_buf);
	mFree(p->toolbar_btts);
}

/** ダイアログ作成 */

static _dialog *_create_dlg(mWindow *owner)
{
	_dialog *p;
	mWidget *ct;
	mListView *list;
	int i;
	uint8_t tabno[] = {
		TABNO_OPT1, TABNO_OPT2, TABNO_STEP, TABNO_BTT,
		TABNO_INTERFACE, TABNO_SYSTEM, TABNO_CURSOR
	};

	p = (_dialog *)mDialogNew(sizeof(_dialog), owner, MWINDOW_S_DIALOG_NORMAL);
	if(!p) return NULL;

	p->wg.event = _event_handle;
	p->wg.destroy = _destroy_handle;

	//設定コピー

	_config_to_curdata(&p->dat, APP_CONF);

	//

	mContainerSetPadding_one(M_CONTAINER(p), 8);

	p->ct.sepW = 16;

	M_TR_G(TRGROUP_DLG_ENV_OPTION);

	mWindowSetTitle(M_WINDOW(p), M_TR_T(0));

	//----- ウィジェット

	ct = mContainerCreate(M_WIDGET(p), MCONTAINER_TYPE_HORZ, 0, 12, MLF_EXPAND_WH);

	//リスト

	list = mListViewNew(0, ct, 0, MSCROLLVIEW_S_VERT | MSCROLLVIEW_S_FRAME);

	list->wg.id = WID_LIST;
	list->wg.fLayout = MLF_EXPAND_H;
	list->lv.itemHeight = mWidgetGetFontHeight(M_WIDGET(list)) + 6;

	for(i = 0; i < TAB_NUM; i++)
		mListViewAddItem(list, M_TR_T(1 + tabno[i]), -1, MLISTVIEW_ITEM_F_STATIC_TEXT, tabno[i]);

	mListViewSetWidthAuto(list, TRUE);
	mListViewSetFocusItem_index(list, 0);

	//初期高さ
	
	mWidgetSetInitSize_fontHeight(M_WIDGET(list), 0, 22);

	//内容の親

	p->ctview = mContainerViewNew(0, ct, 0);

	p->ctview->wg.fLayout = MLF_EXPAND_WH;

	//タブ内容作成

	_create_contents(p, TRUE);

	//OK/cancel

	mContainerCreateOkCancelButton(M_WIDGET(p));

	return p;
}

/** 環境設定ダイアログ
 *
 * @return 更新フラグ */

int EnvOptionDlg_run(mWindow *owner)
{
	_dialog *p;

	p = _create_dlg(owner);
	if(!p) return 0;

	mWindowMoveResizeShow_hintSize(M_WINDOW(p));

	return mDialogRun(M_DIALOG(p), TRUE);
}
