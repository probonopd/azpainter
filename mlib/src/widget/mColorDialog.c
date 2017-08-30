/*$
Copyright (c) 2017, Azel
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERSAND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
$*/

/*****************************************
 * mColorDialog
 *****************************************/

#include "mDef.h"

#include "mWidget.h"
#include "mContainer.h"
#include "mWindow.h"
#include "mDialog.h"
#include "mEvent.h"
#include "mTrans.h"
#include "mColorConv.h"

#include "mLabel.h"
#include "mColorPreview.h"
#include "mHSVPicker.h"
#include "mSliderBar.h"
#include "mLineEdit.h"


//-------------------------

#define M_COLORDIALOG(p)  ((mColorDialog *)(p))

typedef struct _mHSVPicker mHSVPicker;
typedef struct _mColorPreview mColorPreview;
typedef struct _mSliderBar mSliderBar;
typedef struct _mLineEdit mLineEdit;

typedef struct
{
	mRgbCol *retcol,curcol;

	mHSVPicker *hsv;
	mColorPreview *prev;
	mSliderBar *slider[6];
	mLineEdit *edit[6];
}mColorDialogData;

typedef struct _mColorDialog
{
	mWidget wg;
	mContainerData ct;
	mWindowData win;
	mDialogData dlg;
	mColorDialogData cdlg;
}mColorDialog;


int mColorDialogEventHandle(mWidget *wg,mEvent *ev);
mColorDialog *mColorDialogNew(int size,mWindow *owner,mRgbCol *retcol);
void mColorDialogSetColor(mColorDialog *p,mRgbCol col);

//-------------------------

enum
{
	WID_HSV    = 200,
	WID_SLIDER = 300,
	WID_EDIT   = 400
};

#define _SET_HSV 1
#define _SET_RGB 2

//-------------------------


//*******************************
// mColorDialog
//*******************************


//========================
// sub
//========================


/** ウィジェット作成 */

static void _create_widget(mColorDialog *p)
{
	mWidget *cth,*ct;
	int i,max;
	char label[6] = {'H','S','V','R','G','B'},m[2] = {0,0};
	uint32_t margin;

	//トップレイアウト

	mContainerSetPadding_one(M_CONTAINER(p), 10);

	//水平レイアウト

	cth = mContainerCreate(M_WIDGET(p), MCONTAINER_TYPE_HORZ, 0, 20, MLF_EXPAND_W);

	//------ 色

	ct = mContainerCreate(cth, MCONTAINER_TYPE_VERT, 0, 20, 0);

	//色プレビュー

	p->cdlg.prev = mColorPreviewCreate(ct, MCOLORPREVIEW_S_FRAME, 60, 50, 0);

	//HSV

	p->cdlg.hsv = mHSVPickerCreate(ct, WID_HSV, 120, 0);

	//------ 色値

	ct = mContainerCreate(cth, MCONTAINER_TYPE_GRID, 3, 0, MLF_EXPAND_W);

	M_CONTAINER(ct)->ct.gridSepCol = 5;
	M_CONTAINER(ct)->ct.gridSepRow = 3;

	for(i = 0; i < 6; i++)
	{
		if(i < 3)
			max = (i == 0)? 359: 100;
		else
			max = 255;

		margin = (i == 2)? M_MAKE_DW4(0,0,0,15): 0;

		//ラベル

		m[0] = label[i];
		mLabelCreate(ct, 0, MLF_MIDDLE, margin, m);

		//スライダー

		p->cdlg.slider[i] = mSliderBarCreate(ct, WID_SLIDER + i, 0,
			MLF_EXPAND_W | MLF_MIDDLE, margin);

		(p->cdlg.slider[i])->wg.initW = 140;

		mSliderBarSetStatus(p->cdlg.slider[i], 0, max, 0);

		//エディット

		p->cdlg.edit[i] = mLineEditCreate(ct, WID_EDIT + i,
			MLINEEDIT_S_SPIN | MLINEEDIT_S_NOTIFY_CHANGE,
			MLF_MIDDLE, margin);

		mLineEditSetWidthByLen(p->cdlg.edit[i], 4);
		mLineEditSetNumStatus(p->cdlg.edit[i], 0, max, 0);
	}

	//------ OK/CANCEL

	ct = mContainerCreateOkCancelButton(M_WIDGET(p));

	M_CONTAINER(ct)->ct.padding.top = 15;
}

/** スライダーとエディットの数値セット */

static void _setColorNum_one(mColorDialog *p,int no,int num)
{
	mSliderBarSetPos(p->cdlg.slider[no], num);
	mLineEditSetNum(p->cdlg.edit[no], num);
}

/** HSV/RGB 値から数値セット */

static void _setColorNum(mColorDialog *p,double *hsv,mRgbCol rgb,int flag)
{
	int i,n;

	//HSV

	if(flag & _SET_HSV)
	{
		for(i = 0; i < 3; i++)
		{
			if(i == 0)
				n = (int)(hsv[0] * 360 + 0.5);
			else
				n = (int)(hsv[i] * 100 + 0.5);

			_setColorNum_one(p, i, n);
		}
	}

	//RGB

	if(flag & _SET_RGB)
	{
		for(i = 0; i < 3; i++)
			_setColorNum_one(p, i + 3, (rgb >> ((2 - i) << 3)) & 255);
	}
}


//====================


/** 作成 */

mColorDialog *mColorDialogNew(int size,mWindow *owner,mRgbCol *retcol)
{
	mColorDialog *p;
	
	if(size < sizeof(mColorDialog)) size = sizeof(mColorDialog);
	
	p = (mColorDialog *)mDialogNew(size, owner, MWINDOW_S_DIALOG_NORMAL);
	if(!p) return NULL;
	
	p->wg.event = mColorDialogEventHandle;

	p->cdlg.retcol = retcol;

	//タイトル

	mWindowSetTitle(M_WINDOW(p), M_TR_T2(M_TRGROUP_SYS, M_TRSYS_TITLE_SELECTCOLOR));

	//ウィジェット

	_create_widget(p);
		
	return p;
}

/** 色をセット */

void mColorDialogSetColor(mColorDialog *p,mRgbCol col)
{
	double d[3];

	p->cdlg.curcol = col;

	//プレビュー
	
	mColorPreviewSetColor(p->cdlg.prev, col);

	//HSV ピッカー

	mHSVPickerSetRGBColor(p->cdlg.hsv, col);

	//数値

	mRGBtoHSV_pac(col, d);

	_setColorNum(p, d, col, _SET_HSV | _SET_RGB);
}

/** 色数値が変更された時 */

static void _changeNum(mColorDialog *p,int no)
{
	int i,n[3];
	double d[3];
	uint32_t col;

	if(no < 3)
	{
		//------ HSV

		//HSV 値

		for(i = 0; i < 3; i++)
			n[i] = (p->cdlg.slider[i])->sl.pos;

		//HSV -> RGB

		col = mHSVtoRGB_pac(n[0] / 360.0, n[1] / 100.0, n[2] / 100.0);

		//RGB 数値

		_setColorNum(p, NULL, col, _SET_RGB);
	}
	else
	{
		//------ RGB

		//RGB 値

		for(i = 0; i < 3; i++)
			n[i] = (p->cdlg.slider[i + 3])->sl.pos;

		//色

		col = M_RGB(n[0], n[1], n[2]);

		//HSV 数値

		mRGBtoHSV_pac(col, d);

		_setColorNum(p, d, 0, _SET_HSV);
	}

	//プレビュー

	mColorPreviewSetColor(p->cdlg.prev, col);

	//現在色セット

	p->cdlg.curcol = col;

	//HSV ピッカー連動

	if(no == 0)
		//H
		mHSVPickerSetHue(p->cdlg.hsv, (p->cdlg.slider[0])->sl.pos);
	else if(no < 3)
	{
		//SV
		mHSVPickerSetSV(p->cdlg.hsv,
			(p->cdlg.slider[1])->sl.pos / 100.0,
			(p->cdlg.slider[2])->sl.pos / 100.0);
	}
	else
		//RGB
		mHSVPickerSetRGBColor(p->cdlg.hsv, p->cdlg.curcol);
}



//========================
// イベント
//========================


/** 通知イベント */

static void _event_notify(mColorDialog *p,mEvent *ev)
{
	int id,type,n;
	double d[3];

	id   = ev->notify.widgetFrom->id;
	type = ev->notify.type;

	if(id == WID_HSV)
	{
		//------ HSV ピッカー操作時

		p->cdlg.curcol = ev->notify.param1;

		//プレビュー

		mColorPreviewSetColor(p->cdlg.prev, p->cdlg.curcol);
		
		//数値

		mHSVPickerGetHSVColor(p->cdlg.hsv, d);

		_setColorNum(p, d, p->cdlg.curcol, _SET_HSV | _SET_RGB);
	}
	else if(id >= WID_SLIDER && id < WID_SLIDER + 6)
	{
		//------ スライダー操作時

		if(type == MSLIDERBAR_N_HANDLE
			&& (ev->notify.param2 & MSLIDERBAR_HANDLE_F_CHANGE))
		{
			n = id - WID_SLIDER;

			mLineEditSetNum(p->cdlg.edit[n], ev->notify.param1);

			_changeNum(p, n);
		}
	}
	else if(id >= WID_EDIT && id < WID_EDIT + 6)
	{
		//------ エディット数値変更時

		if(type == MLINEEDIT_N_CHANGE)
		{
			n = id - WID_EDIT;

			mSliderBarSetPos(p->cdlg.slider[n], mLineEditGetNum(p->cdlg.edit[n]));

			_changeNum(p, n);
		}
	}
	else if(id == M_WID_OK)
	{
		//OK

		*(p->cdlg.retcol) = p->cdlg.curcol;

		mDialogEnd(M_DIALOG(p), TRUE);
	}
	else if(id == M_WID_CANCEL)
	{
		//キャンセル

		mDialogEnd(M_DIALOG(p), FALSE);
	}
}

/** イベント */

int mColorDialogEventHandle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY)
	{
		_event_notify(M_COLORDIALOG(wg), ev);
		return TRUE;
	}

	return mDialogEventHandle(wg, ev);
}


//*******************************
// 関数
//*******************************


/** 色選択ダイアログ
 *
 * @param pcol デフォルト色。終了時は結果の色が入る。
 * @ingroup sysdialog */

mBool mSysDlgSelectColor(mWindow *owner,mRgbCol *pcol)
{
	mColorDialog *p;

	p = mColorDialogNew(0, owner, pcol);
	if(!p) return FALSE;

	mColorDialogSetColor(p, *pcol);

	mWindowMoveResizeShow_hintSize(M_WINDOW(p));

	return mDialogRun(M_DIALOG(p), TRUE);
}

