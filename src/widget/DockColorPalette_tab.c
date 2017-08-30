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
 * [dock] カラーパレットのタブ内容
 *****************************************/

#include <stdio.h>

#include "mDef.h"
#include "mWidget.h"
#include "mContainer.h"
#include "mWindow.h"
#include "mDialog.h"
#include "mLabel.h"
#include "mLineEdit.h"
#include "mButton.h"
#include "mArrowButton.h"
#include "mTab.h"
#include "mEvent.h"
#include "mTrans.h"
#include "mSysDialog.h"
#include "mMessageBox.h"
#include "mMenu.h"
#include "mStr.h"

#include "defMacros.h"
#include "defDraw.h"
#include "defWidgets.h"
#include "AppErr.h"

#include "DockObject.h"
#include "ColorPalette.h"
#include "ImageBuf24.h"
#include "MainWindow.h"

#include "trgroup.h"
#include "trid_word.h"


//--------------------

/* DockColorPalete_hlspal.c */
mWidget *DockColorPalette_HLSPalette_new(mWidget *parent);
void DockColorPalette_HLSPalette_changeTheme(mWidget *wg);

/* DockColorPalete_gradbar.c */
mWidget *DockColorPalette_GradationBar_new(mWidget *parent,int id,uint32_t colL,uint32_t colR,int step);
void DockColorPalette_GradationBar_setStep(mWidget *wg,int step);

/* DockColorPalete_colpal.c */
mWidget *DockColorPalette_ColorPalette_new(mWidget *parent,int id);
void DockColorPalette_ColorPalette_update(mWidget *wg);

/* DockColorPalette_editpaldlg.c */
mBool DockColorPalette_runEditPalette(mWindow *owner);

//--------------------

#define _OWNER_WIN   DockObject_getOwnerWindow(APP_WIDGETS->dockobj[DOCKWIDGET_COLOR_PALETTE])

enum
{
	TRID_GRAD_SETSTEP = 100,
	TRID_GRAD_SETSTEP_MES
};

static mBool _paloptdlg_run(mWindow *owner);

//--------------------




//*****************************
// 共通関数
//*****************************


/** タブのメインコンテナ作成
 *
 * MLF_EXPAND_W、垂直コンテナ、境界余白は 5 */

static mContainer *_create_main_container(int size,mWidget *parent,
	int (*event)(mWidget *,mEvent *))
{
	mContainer *ct;

	ct = mContainerNew(size, parent);

	mContainerSetPadding_b4(ct, M_MAKE_DW4(3,0,3,0));

	ct->wg.fLayout = MLF_EXPAND_W;
	ct->wg.notifyTargetInterrupt = MWIDGET_NOTIFYTARGET_INT_SELF; //子の通知は自身で受け取る
	ct->ct.sepW = 5;
	if(event) ct->wg.event = event;

	return ct;
}


/***************************************
 * HLS パレット
 ***************************************/

typedef struct
{
	mWidget wg;
	mContainerData ct;

	mWidget *hlspal;
}_tabct_hls;


/** タブ内容作成 */

mWidget *DockColorPalette_tabHLS_create(mWidget *parent,int id)
{
	_tabct_hls *p;

	p = (_tabct_hls *)_create_main_container(sizeof(_tabct_hls), parent, NULL);
	p->wg.id = id;

	p->hlspal = DockColorPalette_HLSPalette_new(M_WIDGET(p));

	return (mWidget *)p;
}

/** テーマ変更時 */

void DockColorPalette_tabHLS_changeTheme(mWidget *wg)
{
	DockColorPalette_HLSPalette_changeTheme(((_tabct_hls *)wg)->hlspal);
}


/***************************************
 * 中間色
 ***************************************/


typedef struct
{
	mWidget wg;
	mContainerData ct;

	mWidget *bar[COLPAL_GRADNUM],
		*button;
}_tabct_grad;

enum
{
	WID_GRAD_BAR_TOP = 1,
	WID_GRAD_BUTTON = 100
};


/** 段階数の設定 */

static void _grad_set_step(_tabct_grad *p)
{
	mMenu *menu;
	mMenuItemInfo *mi;
	int i,step;
	mBox box;
	char m[16];

	//メニュー

	mWidgetGetRootBox(p->button, &box);

	menu = mMenuNew();

	for(i = 0; i < COLPAL_GRADNUM; i++)
	{
		snprintf(m, 16, "no.%d", i + 1);
		mMenuAddText_copy(menu, i, m);
	}

	mi = mMenuPopup(menu, NULL, box.x + box.w, box.y + box.h, MMENU_POPUP_F_RIGHT);
	i = (mi)? mi->id: -1;

	mMenuDestroy(menu);

	if(i < 0) return;

	//段階数取得

	M_TR_G(TRGROUP_DOCK_COLOR_PALETTE);

	step = APP_DRAW->col.gradcol[i][0] >> 24;

	if(mSysDlgInputNum(_OWNER_WIN, M_TR_T(TRID_GRAD_SETSTEP), M_TR_T(TRID_GRAD_SETSTEP_MES),
		&step, 0, 64, MSYSDLG_INPUTNUM_F_DEFAULT))
	{
		if(step != 0 && step < 3) step = 3;

		APP_DRAW->col.gradcol[i][0] &= 0xffffff;
		APP_DRAW->col.gradcol[i][0] |= step << 24;

		DockColorPalette_GradationBar_setStep(p->bar[i], step);
	}
}

/** イベント */

static int _grad_event_handle(mWidget *wg,mEvent *ev)
{
	int n;

	if(ev->type == MEVENT_NOTIFY)
	{
		if(ev->notify.id == WID_GRAD_BUTTON)
			//段階数の設定
			_grad_set_step((_tabct_grad *)wg);
		else
		{
			//グラデーションバー

			if(ev->notify.type == 0)
				//色選択 (そのまま送る)
				mWidgetAppendEvent_notify(MWIDGET_NOTIFYWIDGET_RAW, wg, 0, ev->notify.param1, 0);
			else
			{
				//左右の色変更
				/* type: 1-左 2-右 */

				n = ev->notify.id - WID_GRAD_BAR_TOP;

				APP_DRAW->col.gradcol[n][ev->notify.type - 1] &= 0xff000000;
				APP_DRAW->col.gradcol[n][ev->notify.type - 1] |= ev->notify.param1;
			}
		}
	}

	return 1;
}

/** タブ内容作成 */

mWidget *DockColorPalette_tabGrad_create(mWidget *parent,int id)
{
	_tabct_grad *p;
	int i;

	p = (_tabct_grad *)_create_main_container(sizeof(_tabct_grad), parent, _grad_event_handle);

	p->wg.id = id;
	p->ct.sepW = 7;

	//グラデーションバー

	for(i = 0; i < COLPAL_GRADNUM; i++)
	{
		p->bar[i] = DockColorPalette_GradationBar_new(M_WIDGET(p), WID_GRAD_BAR_TOP + i,
				APP_DRAW->col.gradcol[i][0] & 0xffffff, APP_DRAW->col.gradcol[i][1],
				APP_DRAW->col.gradcol[i][0] >> 24);
	}

	//ボタン

	p->button = (mWidget *)mButtonCreate(M_WIDGET(p), WID_GRAD_BUTTON,
		0, MLF_RIGHT, M_MAKE_DW4(0,2,0,0),
		M_TR_T2(TRGROUP_DOCK_COLOR_PALETTE, TRID_GRAD_SETSTEP));

	return (mWidget *)p;
}



/***************************************
 * パレット
 ***************************************/


typedef struct
{
	mWidget wg;
	mContainerData ct;

	mWidget *colpal,
		*menubtt;
	mTab *tab;

	mStr strdir;
}_tabct_pal;


#define _PAL_SELDAT (APP_COLPAL->pal + APP_DRAW->col.colpal_sel)

enum
{
	WID_PAL_COLPAL = 100,
	WID_PAL_MENUBTT,
	WID_PAL_TAB
};

enum
{
	TRID_PAL_EDIT = 1100,
	TRID_PAL_RESIZE,
	TRID_PAL_GETPAL_FROM_IMAGE,
	TRID_PAL_FILL_WHITE,
	TRID_PAL_LOAD_FILE,
	TRID_PAL_SAVE_FILE,
	TRID_PAL_OPTION,

	TRID_PAL_RESIZE_TITLE = 1200,
	TRID_PAL_RESIZE_MES,
	TRID_PAL_FILL_WHITE_MES,
	TRID_PAL_OPTION_TITLE,
	TRID_PAL_OPTION_MAX_COLUMN
};



//===========================
// コマンド
//===========================


/** 個数変更 */

static void _pal_cmd_resize(_tabct_pal *p)
{
	mStr str = MSTR_INIT;
	int num;

	//[!] 文字列グループはそのままで良い

	mStrSetFormat(&str, "%s (%d - %d)",
		M_TR_T(TRID_PAL_RESIZE_MES), 1, COLORPALETTEDAT_MAXNUM);

	num = _PAL_SELDAT->num;

	if(mSysDlgInputNum(_OWNER_WIN, M_TR_T(TRID_PAL_RESIZE_TITLE), str.buf, &num,
		1, COLORPALETTEDAT_MAXNUM, MSYSDLG_INPUTNUM_F_DEFAULT))
	{
		ColorPaletteDat_resize(_PAL_SELDAT, num);

		DockColorPalette_ColorPalette_update(p->colpal);
	}

	mStrFree(&str);
}

/** 画像からパレット取得 */

static void _pal_cmd_getpal_from_image(_tabct_pal *p)
{
	mStr str = MSTR_INIT;
	ImageBuf24 *img;

	if(!mSysDlgOpenFile(_OWNER_WIN,
		FILEFILTER_NORMAL_IMAGE,
		0, p->strdir.buf, 0, &str))
		return;

	//ディレクトリ保存

	mStrPathGetDir(&p->strdir, str.buf);

	//画像読み込み

	img = ImageBuf24_loadFile(str.buf);

	if(!img)
		MainWindow_apperr(APPERR_LOAD, NULL);
	else
	{
		ColorPaletteDat_createFromImage(_PAL_SELDAT, img->buf, img->w, img->h);

		DockColorPalette_ColorPalette_update(p->colpal);
	}

	ImageBuf24_free(img);
	mStrFree(&str);
}

/** ファイル読み込み */

static void _pal_cmd_loadfile(_tabct_pal *p)
{
	mStr str = MSTR_INIT;

	if(!mSysDlgOpenFile(_OWNER_WIN,
		"Palette Files (APL/ACO)\t*.apl;*.aco\t"
		"AzPainter Palette (APL)\t*.apl\t"
		"Adobe Color File (ACO)\t*.aco\t"
		"All Files (*)\t*\t",
		0, p->strdir.buf, 0, &str))
		return;

	//ディレクトリ保存

	mStrPathGetDir(&p->strdir, str.buf);

	//読み込み

	if(!ColorPaletteDat_loadFile(_PAL_SELDAT, str.buf))
		MainWindow_apperr(APPERR_LOAD, NULL);
	else
		DockColorPalette_ColorPalette_update(p->colpal);

	mStrFree(&str);
}

/** ファイル保存 */

static void _pal_cmd_savefile(_tabct_pal *p)
{
	mStr str = MSTR_INIT;
	int type;
	mBool ret;

	if(!mSysDlgSaveFile(_OWNER_WIN,
		"AzPainter Palette (APL)\t*.apl\t"
		"Adobe Color File (ACO)\t*.aco\t",
		0, p->strdir.buf, 0, &str, &type))
		return;

	//ディレクトリ保存

	mStrPathGetDir(&p->strdir, str.buf);

	//保存

	mStrPathSetExt(&str, (type == 0)? "apl": "aco");

	if(type == 0)
		ret = ColorPaletteDat_saveFile_apl(_PAL_SELDAT, str.buf);
	else
		ret = ColorPaletteDat_saveFile_aco(_PAL_SELDAT, str.buf);

	if(!ret)
		MainWindow_apperr(APPERR_FAILED, NULL);

	mStrFree(&str);
}


//===========================
//
//===========================


/** メニュー */

static void _pal_run_menu(_tabct_pal *p)
{
	mMenu *menu;
	mMenuItemInfo *mi;
	int id;
	mBox box;
	uint16_t dat[] = {
		TRID_PAL_EDIT, TRID_PAL_RESIZE, 0xfffe,
		TRID_PAL_GETPAL_FROM_IMAGE, TRID_PAL_FILL_WHITE, 0xfffe,
		TRID_PAL_LOAD_FILE, TRID_PAL_SAVE_FILE, 0xfffe,
		TRID_PAL_OPTION, 0xffff
	};

	//メニュー

	M_TR_G(TRGROUP_DOCK_COLOR_PALETTE);

	menu = mMenuNew();

	mMenuAddTrArray16(menu, dat);

	mWidgetGetRootBox(p->menubtt, &box);

	mi = mMenuPopup(menu, NULL, box.x + box.w, box.y + box.h, MMENU_POPUP_F_RIGHT);
	id = (mi)? mi->id: -1;

	mMenuDestroy(menu);

	if(id == -1) return;

	//コマンド

	switch(id)
	{
		//編集
		case TRID_PAL_EDIT:
			if(DockColorPalette_runEditPalette((mWindow *)APP_WIDGETS->mainwin))
			{
				DockColorPalette_ColorPalette_update(p->colpal);
			}
			break;
		//個数変更
		case TRID_PAL_RESIZE:
			_pal_cmd_resize(p);
			break;
		//画像からパレット取得
		case TRID_PAL_GETPAL_FROM_IMAGE:
			_pal_cmd_getpal_from_image(p);
			break;
		//すべて白に
		case TRID_PAL_FILL_WHITE:
			if(mMessageBox(NULL, NULL, M_TR_T(TRID_PAL_FILL_WHITE_MES),
				MMESBOX_YESNO, MMESBOX_YES) == MMESBOX_YES)
			{
				ColorPaletteDat_fillWhite(_PAL_SELDAT);
				DockColorPalette_ColorPalette_update(p->colpal);

				APP_COLPAL->exit_save = TRUE;
			}
			break;
		//ファイル読込
		case TRID_PAL_LOAD_FILE:
			_pal_cmd_loadfile(p);
			break;
		//ファイル保存
		case TRID_PAL_SAVE_FILE:
			_pal_cmd_savefile(p);
			break;
		//設定
		case TRID_PAL_OPTION:
			if(_paloptdlg_run(p->wg.toplevel))
				DockColorPalette_ColorPalette_update(p->colpal);
			break;
	}
}

/** イベント */

static int _pal_event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY)
	{
		_tabct_pal *p = (_tabct_pal *)wg;
		
		switch(ev->notify.id)
		{
			//パレット (色を通知)
			case WID_PAL_COLPAL:
				mWidgetAppendEvent_notify(MWIDGET_NOTIFYWIDGET_RAW, wg, 0, ev->notify.param1, 0);
				break;
			//メニュー
			case WID_PAL_MENUBTT:
				_pal_run_menu(p);
				break;
			//タブ選択変更
			case WID_PAL_TAB:
				if(ev->notify.type == MTAB_N_CHANGESEL)
				{
					APP_DRAW->col.colpal_sel = ev->notify.param1;

					DockColorPalette_ColorPalette_update(p->colpal);
				}
				break;
		}
	}

	return 1;
}

/** 破棄ハンドラ */

static void _destroy_handle(mWidget *wg)
{
	mStrFree(&((_tabct_pal *)wg)->strdir);
}

/** タブ内容作成 */

mWidget *DockColorPalette_tabPalette_create(mWidget *parent,int id)
{
	_tabct_pal *p;
	mWidget *ct;
	int i;
	char m[3] = {' ',0,0};

	p = (_tabct_pal *)_create_main_container(sizeof(_tabct_pal), parent, _pal_event_handle);

	p->wg.id = id;
	p->wg.destroy = _destroy_handle;

	mContainerSetType(M_CONTAINER(p), MCONTAINER_TYPE_HORZ, 0);
	p->ct.sepW = 0;
	p->wg.fLayout = MLF_EXPAND_WH;

	//カラーパレット

	p->colpal = DockColorPalette_ColorPalette_new(M_WIDGET(p), WID_PAL_COLPAL);

	//------ 右側

	ct = mContainerCreate(M_WIDGET(p), MCONTAINER_TYPE_VERT, 0, 0, MLF_EXPAND_H);

	//メニューボタン

	p->menubtt = (mWidget *)mArrowButtonCreate(ct, WID_PAL_MENUBTT, MARROWBUTTON_S_DOWN,
		0, M_MAKE_DW4(3,0,0,8));

	//タブ

	p->tab = mTabCreate(ct, WID_PAL_TAB, MTAB_S_RIGHT, MLF_EXPAND_H, 0);

	for(i = 0; i < COLORPALETTE_NUM; i++)
	{
		m[1] = '1' + i;
		mTabAddItemText(p->tab, m);
	}

	mTabSetSel_index(p->tab, APP_DRAW->col.colpal_sel);

	return (mWidget *)p;
}

/** カラーパレットエリア内にファイルが D&D された時、エリアから呼び出す関数 */

void DockColorPalette_tabPalette_dnd_load(mWidget *area,const char *filename)
{
	_tabct_pal *p = (_tabct_pal *)area->parent->parent;

	if(!ColorPaletteDat_loadFile(_PAL_SELDAT, filename))
		MainWindow_apperr(APPERR_LOAD, NULL);
	else
		DockColorPalette_ColorPalette_update(p->colpal);
}


/***************************************
 * パレット表示設定ダイアログ
 ***************************************/


typedef struct
{
	mWidget wg;
	mContainerData ct;
	mWindowData win;
	mDialogData dlg;

	mLineEdit *edit[3];
}_paloptdlg;


/** 作成 */

static _paloptdlg *_paloptdlg_create(mWindow *owner)
{
	_paloptdlg *p;
	mWidget *ct;
	mLineEdit *le;
	const char *label;
	int i,min,max,val[3];

	p = (_paloptdlg *)mDialogNew(sizeof(_paloptdlg), owner, MWINDOW_S_DIALOG_NORMAL);
	if(!p) return NULL;

	p->wg.event = mDialogEventHandle_okcancel;

	//

	mContainerSetPadding_one(M_CONTAINER(p), 8);

	p->ct.sepW = 18;

	M_TR_G(TRGROUP_DOCK_COLOR_PALETTE);

	mWindowSetTitle(M_WINDOW(p), M_TR_T(TRID_PAL_OPTION_TITLE));

	//ウィジェット

	val[0] = APP_DRAW->col.colpal_cellw;
	val[1] = APP_DRAW->col.colpal_cellh;
	val[2] = APP_DRAW->col.colpal_max_column;

	ct = mContainerCreateGrid(M_WIDGET(p), 2, 6, 8, 0);

	for(i = 0; i < 3; i++)
	{
		//ラベル

		if(i == 0)
			label = M_TR_T2(TRGROUP_WORD, TRID_WORD_WIDTH);
		else if(i == 1)
			label = M_TR_T2(TRGROUP_WORD, TRID_WORD_HEIGHT);
		else
			label = M_TR_T(TRID_PAL_OPTION_MAX_COLUMN);
	
		mLabelCreate(ct, 0, MLF_MIDDLE, 0, label);

		//エディット

		p->edit[i] = le = mLineEditCreate(ct, 0, MLINEEDIT_S_SPIN, 0, 0);

		if(i < 2)
			min = 6, max = 32;
		else
			min = 0, max = 500;

		mLineEditSetWidthByLen(le, 6);
		mLineEditSetNumStatus(le, min, max, 0);
		mLineEditSetNum(le, val[i]);
	}

	//OK/cancel

	mContainerCreateOkCancelButton(M_WIDGET(p));

	return p;
}

/** ダイアログ実行 */

mBool _paloptdlg_run(mWindow *owner)
{
	_paloptdlg *p;
	int ret;

	p = _paloptdlg_create(owner);

	mWindowMoveResizeShow_hintSize(M_WINDOW(p));

	ret = mDialogRun(M_DIALOG(p), FALSE);

	if(ret)
	{
		APP_DRAW->col.colpal_cellw = mLineEditGetNum(p->edit[0]);
		APP_DRAW->col.colpal_cellh = mLineEditGetNum(p->edit[1]);
		APP_DRAW->col.colpal_max_column = mLineEditGetNum(p->edit[2]);
	}

	mWidgetDestroy(M_WIDGET(p));

	return ret;
}
