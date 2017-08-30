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
 * ブラシ詳細設定ダイアログ
 ************************************/

#include "mDef.h"
#include "mWidget.h"
#include "mDialog.h"
#include "mContainer.h"
#include "mWindow.h"
#include "mWidgetBuilder.h"
#include "mLabel.h"
#include "mLineEdit.h"
#include "mCheckButton.h"
#include "mStr.h"
#include "mTrans.h"

#include "BrushItem.h"

#include "trgroup.h"


//----------------------

typedef struct
{
	mWidget wg;
	mContainerData ct;
	mWindowData win;
	mDialogData dlg;

	mCheckButton *ck_autosave;
	mLineEdit *le_radius[2];
}_dialog;

//----------------------

enum
{
	WID_CK_AUTOSAVE = 100,
	WID_EDIT_RADIUS_MIN,
	WID_EDIT_RADIUS_MAX,
	
	WID_LABEL_MIN = 200
};

//----------------------

static const char *g_build =
//自動保存
"ck:id=100:tr=1;"
//半径設定
"gb:cts=g3:sep=5,6:tr=2;"  
  //最小値
  "lb:lf=m:tr=3;"
  "le#s:id=101:wlen=6;"
  "lb:id=200:lf=m;"
  //最大値
  "lb:lf=m:tr=4;"
  "le#s:id=102:wlen=6;"
;

//----------------------



/** 作成 */

static _dialog *_dlg_create(mWindow *owner,BrushItem *item)
{
	_dialog *p;
	mLineEdit *le;
	mWidget *wg;
	int i;
	mStr str = MSTR_INIT;

	p = (_dialog *)mDialogNew(sizeof(_dialog), owner, MWINDOW_S_DIALOG_NORMAL);
	if(!p) return NULL;

	p->wg.event = mDialogEventHandle_okcancel;

	//

	mContainerSetPadding_one(M_CONTAINER(p), 8);
	p->ct.sepW = 7;

	M_TR_G(TRGROUP_DLG_BRUSH_DETAIL);

	mWindowSetTitle(M_WINDOW(p), M_TR_T(0));

	//ウィジェット

	mWidgetBuilderCreateFromText(M_WIDGET(p), g_build);

	//自動保存

	p->ck_autosave = (mCheckButton *)mWidgetFindByID(M_WIDGET(p), WID_CK_AUTOSAVE);

	mCheckButtonSetState(p->ck_autosave, item->flags & BRUSHITEM_F_AUTO_SAVE);

	//半径

	for(i = 0; i < 2; i++)
	{
		p->le_radius[i] = le = (mLineEdit *)mWidgetFindByID(M_WIDGET(p), WID_EDIT_RADIUS_MIN + i);

		mLineEditSetNumStatus(le, BRUSHITEM_RADIUS_MIN, BRUSHITEM_RADIUS_MAX, 1);
	}

	if(item->type == BRUSHITEM_TYPE_FINGER)
	{
		for(i = 0; i < 2; i++)
			mWidgetEnable(M_WIDGET(p->le_radius[i]), FALSE);
	}
	else
	{
		mLineEditSetNum(p->le_radius[0], item->radius_min);
		mLineEditSetNum(p->le_radius[1], item->radius_max);
	}

	//範囲のラベル

	mStrSetFormat(&str, "[%.1F-%.1F]", BRUSHITEM_RADIUS_MIN, BRUSHITEM_RADIUS_MAX);

	mLabelSetText((mLabel *)mWidgetFindByID(M_WIDGET(p), WID_LABEL_MIN), str.buf);

	mStrFree(&str);

	//OK/cancel

	wg = mContainerCreateOkCancelButton(M_WIDGET(p));
	wg->margin.top = 10;

	return p;
}

/** OK 時、値セット */

static void _set_value(_dialog *p,BrushItem *item)
{
	int min,max,n;

	//自動保存

	if(mCheckButtonIsChecked(p->ck_autosave))
		item->flags |= BRUSHITEM_F_AUTO_SAVE;
	else
		item->flags &= ~BRUSHITEM_F_AUTO_SAVE;

	//------- 半径

	if(item->type != BRUSHITEM_TYPE_FINGER)
	{
		min = mLineEditGetNum(p->le_radius[0]);
		max = mLineEditGetNum(p->le_radius[1]);

		//min < max になるように

		if(min > max)
			min = BRUSHITEM_RADIUS_MIN;

		if(min == max)
		{
			if(min == BRUSHITEM_RADIUS_MIN)
				max++;
			else
				min--;
		}

		//セット

		item->radius_min = min;
		item->radius_max = max;

		//半径値を調整

		n = item->radius;

		if(n < min) n = min;
		else if(n > max) n = max;

		item->radius = n;
	}
}

/** ブラシ詳細設定ダイアログ */

mBool BrushDetailDlg_run(mWindow *owner,BrushItem *item)
{
	_dialog *p;
	mBool ret;

	p = _dlg_create(owner, item);
	if(!p) return FALSE;

	mWindowMoveResizeShow_hintSize(M_WINDOW(p));

	ret = mDialogRun(M_DIALOG(p), FALSE);

	if(ret)
		_set_value(p, item);

	mWidgetDestroy(M_WIDGET(p));

	return ret;
}
