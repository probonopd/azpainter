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

/************************************
 * 保存設定ダイアログ
 ************************************/

#include "mDef.h"
#include "mWidget.h"
#include "mDialog.h"
#include "mContainer.h"
#include "mWindow.h"
#include "mWidgetBuilder.h"
#include "mLabel.h"
#include "mCheckButton.h"
#include "mLineEdit.h"
#include "mComboBox.h"
#include "mTrans.h"

#include "defConfig.h"
#include "defFileFormat.h"

#include "trgroup.h"


//----------------------

typedef struct __saveopt_dlg _saveopt_dlg;

struct __saveopt_dlg
{
	mWidget wg;
	mContainerData ct;
	mWindowData win;
	mDialogData dlg;

	mLineEdit *edit;
	mComboBox *combo;
	mCheckButton *check[2];

	void (*ok_handle)(_saveopt_dlg *,ConfigSaveOption *);
};

//----------------------

enum
{
	TRID_PNG_COMPRESS = 1,
	TRID_ALPHA_CHANNEL,
	TRID_PNG_HELP_ALPHA,
	TRID_QUALITY,
	TRID_SAMPLING_FACTOR,
	TRID_PROGRESSIVE,
	TRID_UNCOMPRESSED,

	TRID_PSD_TYPE_TOP = 100,
	TRID_JPEG_SAMP_TOP = 110
};

//----------------------

static const char *g_build_png =
"ct#h:sep=6;"
  "lb:lf=m:tr=1;"
  "le#s:id=100:range=0,9:wlen=4;"
  "-;"
"ck:id=101:tr=2;"
"lb#B:tr=3;"
;

//----------------------



//============================
// OK 時
//============================


/** [共通] フラグをセット */

static void _ok_setflags(_saveopt_dlg *p,ConfigSaveOption *dst,int ckno,uint32_t flag)
{
	if(mCheckButtonIsChecked(p->check[ckno]))
		dst->flags |= flag;
	else
		dst->flags &= ~flag;
}

/** PNG */

static void _ok_png(_saveopt_dlg *p,ConfigSaveOption *dst)
{
	dst->png_complevel = mLineEditGetNum(p->edit);

	_ok_setflags(p, dst, 0, CONFIG_SAVEOPTION_F_PNG_ALPHA_CHANNEL);
}

/** JPEG */

static void _ok_jpeg(_saveopt_dlg *p,ConfigSaveOption *dst)
{
	dst->jpeg_quality = mLineEditGetNum(p->edit);
	dst->jpeg_sampling_factor = mComboBoxGetItemParam(p->combo, -1);

	_ok_setflags(p, dst, 0, CONFIG_SAVEOPTION_F_JPEG_PROGRESSIVE);
}

/** PSD */

static void _ok_psd(_saveopt_dlg *p,ConfigSaveOption *dst)
{
	dst->psd_type = mCheckButtonGetGroupSelIndex(p->check[0]);

	_ok_setflags(p, dst, 1, CONFIG_SAVEOPTION_F_PSD_UNCOMPRESSED);
}


//============================
// ウィジェット作成
//============================


/** PNG */

static void _create_png(_saveopt_dlg *p)
{
	mWidgetBuilderCreateFromText(M_WIDGET(p), g_build_png);

	p->edit = (mLineEdit *)mWidgetFindByID(M_WIDGET(p), 100);
	p->check[0] = (mCheckButton *)mWidgetFindByID(M_WIDGET(p), 101);

	//圧縮率
	
	mLineEditSetNum(p->edit, APP_CONF->save.png_complevel);

	//アルファチャンネル

	mCheckButtonSetState(p->check[0], APP_CONF->save.flags & CONFIG_SAVEOPTION_F_PNG_ALPHA_CHANNEL);
}

/** JPEG */

static void _create_jpeg(_saveopt_dlg *p)
{
	mWidget *ct;
	int i,samp[3] = {444,422,420};

	ct = mContainerCreateGrid(M_WIDGET(p), 2, 6, 7, 0);

	//品質

	mLabelCreate(ct, 0, MLF_MIDDLE, 0, M_TR_T(TRID_QUALITY));

	p->edit = mLineEditCreate(ct, 0, MLINEEDIT_S_SPIN, 0, 0);

	mLineEditSetWidthByLen(p->edit, 5);
	mLineEditSetNumStatus(p->edit, 0, 100, 0);
	mLineEditSetNum(p->edit, APP_CONF->save.jpeg_quality);

	//サンプリング比

	mLabelCreate(ct, 0, MLF_MIDDLE, 0, M_TR_T(TRID_SAMPLING_FACTOR));

	p->combo = mComboBoxCreate(ct, 0, 0, 0, 0);

	for(i = 0; i < 3; i++)
		mComboBoxAddItem(p->combo, M_TR_T(TRID_JPEG_SAMP_TOP + i), samp[i]);

	mComboBoxSetWidthAuto(p->combo);
	mComboBoxSetSel_findParam_notfind(p->combo, APP_CONF->save.jpeg_sampling_factor, 0);

	//プログレッシブ

	p->check[0] = mCheckButtonCreate(M_WIDGET(p), 0, 0, 0, 0, M_TR_T(TRID_PROGRESSIVE),
		APP_CONF->save.flags & CONFIG_SAVEOPTION_F_JPEG_PROGRESSIVE);
}

/** PSD */

static void _create_psd(_saveopt_dlg *p)
{
	int i;
	mCheckButton *ck;

	p->ct.sepW = 3;

	//タイプ

	for(i = 0; i < 4; i++)
	{
		ck = mCheckButtonCreate(M_WIDGET(p), 0, MCHECKBUTTON_S_RADIO, 0, 0,
			M_TR_T(TRID_PSD_TYPE_TOP + i),
			(APP_CONF->save.psd_type == i));

		if(i == 0) p->check[0] = ck;
	}

	//無圧縮

	p->check[1] = mCheckButtonCreate(M_WIDGET(p), 0, 0, 0, M_MAKE_DW4(0,3,0,0),
		M_TR_T(TRID_UNCOMPRESSED), APP_CONF->save.flags & CONFIG_SAVEOPTION_F_PSD_UNCOMPRESSED);
}


//============================
// main
//============================


/** 作成 */

static _saveopt_dlg *_dlg_create(mWindow *owner,uint32_t format)
{
	_saveopt_dlg *p;
	mWidget *ct;

	p = (_saveopt_dlg *)mDialogNew(sizeof(_saveopt_dlg), owner, MWINDOW_S_DIALOG_NORMAL);
	if(!p) return NULL;

	p->wg.event = mDialogEventHandle_okcancel;

	//

	mContainerSetPadding_one(M_CONTAINER(p), 8);
	p->ct.sepW = 7;

	M_TR_G(TRGROUP_DLG_SAVE_OPTION);

	mWindowSetTitle(M_WINDOW(p), M_TR_T(0));

	//ウィジェット

	if(format & FILEFORMAT_PNG)
	{
		_create_png(p);
		p->ok_handle = _ok_png;
	}
	else if(format & FILEFORMAT_JPEG)
	{
		_create_jpeg(p);
		p->ok_handle = _ok_jpeg;
	}
	else if(format & FILEFORMAT_PSD)
	{
		_create_psd(p);
		p->ok_handle = _ok_psd;
	}

	//OK/cancel

	ct = mContainerCreateOkCancelButton(M_WIDGET(p));
	ct->margin.top = 10;

	return p;
}

/** 保存設定ダイアログ */

mBool SaveOptionDlg_run(mWindow *owner,uint32_t format)
{
	_saveopt_dlg *p;
	mBool ret;

	p = _dlg_create(owner, format);
	if(!p) return FALSE;

	mWindowMoveResizeShow_hintSize(M_WINDOW(p));

	ret = mDialogRun(M_DIALOG(p), FALSE);

	if(ret)
		(p->ok_handle)(p, &APP_CONF->save);

	mWidgetDestroy(M_WIDGET(p));

	return ret;
}
