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
 * キャンバス関連ダイアログ
 *
 * [キャンバスサイズ変更]
 * [キャンバス拡大縮小]
 *****************************************/

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
#include "mEvent.h"

#include "defConfig.h"
#include "defMacros.h"
#include "defScalingType.h"

#include "trgroup.h"
#include "trid_word.h"

#include "CanvasDialogs.h"


//*****************************************
// キャンバスサイズ変更
//*****************************************


//----------------------

typedef struct
{
	mWidget wg;
	mContainerData ct;
	mWindowData win;
	mDialogData dlg;

	mLineEdit *edit[2];
	mCheckButton *ck_align,
		*ck_crop;
}_resize_dlg;

//----------------------

enum
{
	WID_RESIZE_EDITW = 100,
	WID_RESIZE_EDITH,
	WID_RESIZE_ALIGN_TOP,
	WID_RESIZE_CROP = 200,

	TRID_RESIZE_ALIGN = 1,
	TRID_RESIZE_CROP
};

//----------------------


/** 作成 */

static _resize_dlg *_resizedlg_create(mWindow *owner,int initw,int inith)
{
	_resize_dlg *p;
	mWidget *ct,*ct2;
	mCheckButton *ckbtt;
	int i;

	p = (_resize_dlg *)mDialogNew(sizeof(_resize_dlg), owner, MWINDOW_S_DIALOG_NORMAL);
	if(!p) return NULL;

	p->wg.event = mDialogEventHandle_okcancel;

	//

	mContainerSetPadding_one(M_CONTAINER(p), 8);
	p->ct.sepW = 18;

	M_TR_G(TRGROUP_DLG_CANVAS_RESIZE);

	mWindowSetTitle(M_WINDOW(p), M_TR_T(0));

	//------ ウィジェット

	ct = mContainerCreateGrid(M_WIDGET(p), 2, 10, 8, 0);

	//幅、高さ

	for(i = 0; i < 2; i++)
	{
		mLabelCreate(ct, 0, MLF_RIGHT | MLF_MIDDLE, 0, M_TR_T2(TRGROUP_WORD, TRID_WORD_WIDTH + i));

		p->edit[i] = mLineEditCreate(ct, WID_RESIZE_EDITW + i, MLINEEDIT_S_SPIN, 0, 0);

		mLineEditSetWidthByLen(p->edit[i], 7);
		mLineEditSetNumStatus(p->edit[i], 1, IMAGE_SIZE_MAX, 0);
		mLineEditSetNum(p->edit[i], (i == 0)? initw: inith);
	}

	//配置

	mLabelCreate(ct, 0, MLF_RIGHT, 0, M_TR_T(TRID_RESIZE_ALIGN));

	ct2 = mContainerCreateGrid(ct, 3, 0, 0, 0);

	for(i = 0; i < 9; i++)
	{
		ckbtt = mCheckButtonCreate(ct2, WID_RESIZE_ALIGN_TOP + i,
			MCHECKBUTTON_S_RADIO, 0, 0, NULL, (i == 0));

		if(i == 0) p->ck_align = ckbtt;
	}

	//範囲外を切り取る

	mWidgetNew(0, ct);

	p->ck_crop = mCheckButtonCreate(ct, WID_RESIZE_CROP, 0, 0, 0,
		M_TR_T(TRID_RESIZE_CROP),
		APP_CONF->canvas_resize_flags & CONFIG_CANVAS_RESIZE_F_CROP);

	//OK/cancel

	mContainerCreateOkCancelButton(M_WIDGET(p));

	return p;
}

/** キャンバスサイズ変更ダイアログ
 *
 * w,h には現在の値を入れておく。 */

mBool CanvasResizeDlg_run(mWindow *owner,CanvasResizeInfo *info)
{
	_resize_dlg *p;
	mBool ret;

	p = _resizedlg_create(owner, info->w, info->h);
	if(!p) return FALSE;

	mWindowMoveResizeShow_hintSize(M_WINDOW(p));

	ret = mDialogRun(M_DIALOG(p), FALSE);

	if(ret)
	{
		info->w = mLineEditGetNum(p->edit[0]);
		info->h = mLineEditGetNum(p->edit[1]);
		info->align = mCheckButtonGetGroupSelIndex(p->ck_align);
		info->crop = mCheckButtonIsChecked(p->ck_crop);

		APP_CONF->canvas_resize_flags = (info->crop)? CONFIG_CANVAS_RESIZE_F_CROP: 0;
	}

	mWidgetDestroy(M_WIDGET(p));

	return ret;
}


//*****************************************
// キャンバス拡大縮小
//*****************************************


//----------------------

typedef struct
{
	mWidget wg;
	mContainerData ct;
	mWindowData win;
	mDialogData dlg;

	mLineEdit *edit[4],
		*edit_dpi;
	mCheckButton *ck_aspect,
		*ck_dpi;
	mComboBox *cb_type;

	CanvasScaleInfo *info;

	int sizeval[4];		//[0]幅 [1]高さ [2]%幅 [3]%高さ
	mBool keep_aspect;
}_scale_dlg;

//----------------------

static const char *g_wb_scale_canvas =
"ct#h:sep=8;"
  "gb:cts=g2:tr=1:sep=6,7;"
    "lb:lf=mr:TR=0,0;"
    "le#cs:id=100:wlen=6;"
    "lb:lf=mr:TR=0,1;"
    "le#cs:id=101:wlen=6;"
    "-;"
  "gb:cts=g2:tr=2:sep=6,7;"
    "lb:lf=mr:TR=0,0;"
    "le#cs:id=102:wlen=6;"
    "lb:lf=mr:TR=0,1;"
    "le#cs:id=103:wlen=6;"
    "-;"
  "-;"
"ck:id=104:lf=r:tr=3:mg=t8;"
"ct#g2:sep=8,7:mg=t10;"
  "ck:id=105:lf=m:tr=4;"
  "le#cs:id=106:wlen=6;"
  "lb:lf=mr:tr=5;"
  "cb:id=107;"
;

//----------------------

enum
{
	WID_SCALE_EDIT_PX_W = 100,
	WID_SCALE_EDIT_PX_H,
	WID_SCALE_EDIT_PERS_W,
	WID_SCALE_EDIT_PERS_H,
	WID_SCALE_CK_KEEP_ASPECT,
	WID_SCALE_CK_CHANGE_DPI,
	WID_SCALE_EDIT_DPI,
	WID_SCALE_CB_TYPE
};

//----------------------


/** サイズの値セット
 *
 * @param no  0-3 で指定値以外。それ以外ですべて。 */

static void _scale_set_sizeval(_scale_dlg *p,int no)
{
	int i;

	for(i = 0; i < 4; i++)
	{
		if(no != i)
			mLineEditSetNum(p->edit[i], p->sizeval[i]);
	}
}

/** サイズのエディット値変更時 */

static void _scale_change_sizeval(_scale_dlg *p,int no)
{
	int n1,n2,w1,w2;

	if(no == 0 || no == 2)
		w1 = p->info->w, w2 = p->info->h;
	else
		w1 = p->info->h, w2 = p->info->w;

	//

	if(no < 2)
	{
		//px

		n1 = no;
		n2 = 1 - no;

		p->sizeval[n1]     = mLineEditGetNum(p->edit[n1]);
		p->sizeval[2 + n1] = (int)((double)p->sizeval[n1] / w1 * 1000.0 + 0.5);

		if(p->keep_aspect)
		{
			p->sizeval[n2]     = (int)((double)p->sizeval[n1] / w1 * w2 + 0.5);
			p->sizeval[2 + n2] = p->sizeval[2 + n1];
		}
	}
	else
	{
		//%

		n1 = no;
		n2 = 5 - no;

		p->sizeval[n1]     = mLineEditGetNum(p->edit[n1]);
		p->sizeval[n1 - 2] = (int)((double)p->sizeval[n1] / 1000.0 * w1 + 0.5);

		if(p->keep_aspect)
		{
			p->sizeval[n2]     = p->sizeval[n1];
			p->sizeval[n2 - 2] = (int)((double)p->sizeval[n2] / 1000.0 * w2 + 0.5);
		}
	}

	_scale_set_sizeval(p, no);
}

/** DPI値変更時 */

static void _scale_change_dpival(_scale_dlg *p)
{
	double d;

	d = (double)mLineEditGetNum(p->edit_dpi) / p->info->dpi;

	p->sizeval[0] = (int)(p->info->w * d + 0.5);
	p->sizeval[1] = (int)(p->info->h * d + 0.5);
	p->sizeval[2] = p->sizeval[3] = (int)(d * 1000 + 0.5);

	_scale_set_sizeval(p, -1);
}

/** DPI変更のチェック変更時 */

static void _scale_toggle_change_dpi(_scale_dlg *p)
{
	int i,f;

	f = !mCheckButtonIsChecked(p->ck_dpi);

	for(i = 0; i < 4; i++)
		mWidgetEnable(M_WIDGET(p->edit[i]), f);

	mWidgetEnable(M_WIDGET(p->edit_dpi), !f);

	if(!f) _scale_change_dpival(p);
}

/** イベント */

static int _scale_event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY)
	{
		_scale_dlg *p = (_scale_dlg *)wg;
	
		switch(ev->notify.id)
		{
			//サイズ
			case WID_SCALE_EDIT_PX_W:
			case WID_SCALE_EDIT_PX_H:
			case WID_SCALE_EDIT_PERS_W:
			case WID_SCALE_EDIT_PERS_H:
				if(ev->notify.type == MLINEEDIT_N_CHANGE)
					_scale_change_sizeval(p, ev->notify.id - WID_SCALE_EDIT_PX_W);
				break;
			
			//縦横比維持
			case WID_SCALE_CK_KEEP_ASPECT:
				p->keep_aspect ^= 1;
				break;
			//DPI変更
			case WID_SCALE_CK_CHANGE_DPI:
				_scale_toggle_change_dpi(p);
				break;
			//DPI値
			case WID_SCALE_EDIT_DPI:
				_scale_change_dpival(p);
				break;
		}
	}
	
	return mDialogEventHandle_okcancel(wg, ev);
}

/** 作成 */

static _scale_dlg *_scaledlg_create(mWindow *owner,CanvasScaleInfo *info)
{
	_scale_dlg *p;
	mWidget *wg;
	int i;

	//作成

	p = (_scale_dlg *)mDialogNew(sizeof(_scale_dlg), owner, MWINDOW_S_DIALOG_NORMAL);
	if(!p) return NULL;

	p->info = info;
	p->wg.event = _scale_event_handle;

	p->sizeval[0] = info->w;
	p->sizeval[1] = info->h;
	p->sizeval[2] = 1000;
	p->sizeval[3] = 1000;

	//

	mContainerSetPadding_one(M_CONTAINER(p), 8);

	M_TR_G(TRGROUP_DLG_CANVAS_SCALE);

	mWindowSetTitle(M_WINDOW(p), M_TR_T(0));

	//------ ウィジェット

	mWidgetBuilderCreateFromText(M_WIDGET(p), g_wb_scale_canvas);

	//サイズ、%

	for(i = 0; i < 4; i++)
	{
		p->edit[i] = (mLineEdit *)mWidgetFindByID(M_WIDGET(p), WID_SCALE_EDIT_PX_W + i);

		if(i < 2)
			mLineEditSetNumStatus(p->edit[i], 1, IMAGE_SIZE_MAX, 0);
		else
		{
			mLineEditSetNumStatus(p->edit[i], 1, 100000, 1);
			mLineEditSetNum(p->edit[i], 1000);
		}
	}

	mLineEditSetNum(p->edit[0], info->w);
	mLineEditSetNum(p->edit[1], info->h);

	//縦横比維持

	p->ck_aspect = (mCheckButton *)mWidgetFindByID(M_WIDGET(p), WID_SCALE_CK_KEEP_ASPECT);
	p->keep_aspect = 1;

	mCheckButtonSetState(p->ck_aspect, 1);

	//DPI 変更

	p->ck_dpi = (mCheckButton *)mWidgetFindByID(M_WIDGET(p), WID_SCALE_CK_CHANGE_DPI);
	p->edit_dpi = (mLineEdit *)mWidgetFindByID(M_WIDGET(p), WID_SCALE_EDIT_DPI);

	if(info->have_dpi)
	{
		mWidgetEnable(M_WIDGET(p->edit_dpi), FALSE);
		mLineEditSetNumStatus(p->edit_dpi, 1, 10000, 0);
		mLineEditSetNum(p->edit_dpi, info->dpi);
	}
	else
	{
		mWidgetEnable(M_WIDGET(p->ck_dpi), FALSE);
		mWidgetEnable(M_WIDGET(p->edit_dpi), FALSE);
	}

	//補間方法

	p->cb_type = (mComboBox *)mWidgetFindByID(M_WIDGET(p), WID_SCALE_CB_TYPE);

	M_TR_G(TRGROUP_SCALE_TYPE);

	mComboBoxAddTrItems(p->cb_type, SCALING_TYPE_NUM, 0, 0);
	mComboBoxSetWidthAuto(p->cb_type);
	mComboBoxSetSel_index(p->cb_type, APP_CONF->canvas_scale_type);

	//OK/cancel

	wg = mContainerCreateOkCancelButton(M_WIDGET(p));
	wg->margin.top = 18;

	return p;
}

/** キャンバス拡大縮小ダイアログ
 *
 * w,h,dpi には現在の値を入れておく。 */

mBool CanvasScaleDlg_run(mWindow *owner,CanvasScaleInfo *info)
{
	_scale_dlg *p;
	mBool ret;

	p = _scaledlg_create(owner, info);
	if(!p) return FALSE;

	mWindowMoveResizeShow_hintSize(M_WINDOW(p));

	ret = mDialogRun(M_DIALOG(p), FALSE);

	if(ret)
	{
		info->w = mLineEditGetNum(p->edit[0]);
		info->h = mLineEditGetNum(p->edit[1]);
		info->type = mComboBoxGetSelItemIndex(p->cb_type);

		if(info->have_dpi)
			info->dpi = (mCheckButtonIsChecked(p->ck_dpi))? mLineEditGetNum(p->edit_dpi): -1;

		APP_CONF->canvas_scale_type = info->type;
	}

	mWidgetDestroy(M_WIDGET(p));

	return ret;
}
