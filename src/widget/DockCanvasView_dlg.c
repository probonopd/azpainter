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

/***********************************************
 * [dock] キャンバスビュー/イメージビューアの
 *  操作設定ダイアログ (共通)
 ***********************************************/

#include "mDef.h"
#include "mWidget.h"
#include "mWindow.h"
#include "mDialog.h"
#include "mContainer.h"
#include "mLabel.h"
#include "mComboBox.h"
#include "mTrans.h"

#include "defConfig.h"

#include "trgroup.h"


//----------------------

#define _BUTTON_NUM  5

typedef struct
{
	mWidget wg;
	mContainerData ct;
	mWindowData win;
	mDialogData dlg;

	mComboBox *cb[_BUTTON_NUM];
}_dialog;

//----------------------

enum
{
	TRID_TITLE = 0,
	TRID_LABEL_TOP,
	TRID_CB_CANVASVIEW_TOP = 100,
	TRID_CB_IMAGEVIEWER_TOP = 200,

	CB_CANVASVIEW_NUM  = 3,
	CB_IMAGEVIEWER_NUM = 4
};

//----------------------


/** 作成 */

static _dialog *_create_dlg(mWindow *owner,mBool imageviewer)
{
	_dialog *p;
	mWidget *ct;
	mComboBox *cb;
	int i,j,num,top,sel;

	//作成
	
	p = (_dialog *)mDialogNew(sizeof(_dialog), owner, MWINDOW_S_DIALOG_NORMAL);
	if(!p) return NULL;

	p->wg.event = mDialogEventHandle_okcancel;

	//

	M_TR_G(TRGROUP_DLG_CANVASVIEW_OPT);

	mContainerSetPadding_one(M_CONTAINER(p), 8);
	p->ct.sepW = 18;

	mWindowSetTitle(M_WINDOW(p), M_TR_T(TRID_TITLE));

	//ウィジェット

	ct = mContainerCreateGrid(M_WIDGET(p), 2, 6, 7, 0);

	for(i = 0; i < _BUTTON_NUM; i++)
	{
		//ラベル

		mLabelCreate(ct, 0, MLF_RIGHT | MLF_MIDDLE, 0, M_TR_T(TRID_LABEL_TOP + i));
	
		//コンボボックス
	
		if(imageviewer)
		{
			num = CB_IMAGEVIEWER_NUM;
			top = TRID_CB_IMAGEVIEWER_TOP;
			sel = APP_CONF->imageviewer_btt[i];
		}
		else
		{
			num = CB_CANVASVIEW_NUM;
			top = TRID_CB_CANVASVIEW_TOP;
			sel = APP_CONF->canvasview_btt[i];
		}

		p->cb[i] = cb = mComboBoxNew(0, ct, 0);

		for(j = 0; j < num; j++)
			mComboBoxAddItem(cb, M_TR_T(top + j), 0);

		if(i == 0)
			mComboBoxSetWidthAuto(cb);
		else
			cb->wg.hintOverW = p->cb[0]->wg.hintOverW;
		
		mComboBoxSetSel_index(cb, sel);
	}

	//OK/Cancel

	mContainerCreateOkCancelButton(M_WIDGET(p));
	
	return p;
}

/** 設定ダイアログ実行
 *
 * @param imageviewer TRUE でイメージビューア、FALSE でキャンバスビュー */

void DockCanvasView_runOptionDialog(mWindow *owner,mBool imageviewer)
{
	_dialog *p;
	int i;
	uint8_t *pdst;

	p = _create_dlg(owner, imageviewer);
	if(!p) return;

	mWindowMoveResizeShow_hintSize(M_WINDOW(p));

	if(mDialogRun(M_DIALOG(p), FALSE))
	{
		//セット
		
		pdst = (imageviewer)? APP_CONF->imageviewer_btt: APP_CONF->canvasview_btt;
	
		for(i = 0; i < _BUTTON_NUM; i++)
			pdst[i] = mComboBoxGetSelItemIndex(p->cb[i]);
	}

	mWidgetDestroy(M_WIDGET(p));
}
