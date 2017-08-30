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
 * mAboutDlg
 *****************************************/

#include "mDef.h"
#include "mDialog.h"
#include "mWidget.h"
#include "mContainer.h"
#include "mWindow.h"
#include "mLabel.h"
#include "mButton.h"
#include "mEvent.h"
#include "mTrans.h"
#include "mMessageBox.h"



//******************************
// mAboutDlg
//******************************

enum
{
	WID_BTT_LICENSE = 100
};


/** イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY)
	{
		switch(ev->notify.id)
		{
			//ライセンス
			case WID_BTT_LICENSE:
				mMessageBox(M_WINDOW(wg), "License", (const char *)wg->param, MMESBOX_OK, MMESBOX_OK);
				break;
		}
	}

	return mDialogEventHandle_okcancel(wg, ev);
}

/** ダイアログ作成 */

static mDialog *_create_dialog(mWindow *owner,const char *copying,const char *license)
{
	mDialog *p;
	mButton *btt;
	mWidget *ct;
	
	p = (mDialog *)mDialogNew(0, owner, MWINDOW_S_DIALOG_NORMAL);
	if(!p) return NULL;
	
	p->wg.event = _event_handle;
	p->wg.param = (intptr_t)license;

	mContainerSetPadding_one(M_CONTAINER(p), 10);
	p->ct.sepW = 16;

	mWindowSetTitle(M_WINDOW(p), "about");

	//ラベル

	mLabelCreate(M_WIDGET(p), MLABEL_S_CENTER, MLF_EXPAND_W, 0, copying);

	//----- ボタン

	ct = mContainerCreate(M_WIDGET(p), MCONTAINER_TYPE_HORZ, 0, 3, MLF_CENTER | MLF_EXPAND_X);

	//OK

	btt = mButtonCreate(ct, M_WID_OK, 0, 0, 0, M_TR_T2(M_TRGROUP_SYS, M_TRSYS_OK));

	btt->wg.fState |= MWIDGET_STATE_ENTER_DEFAULT;

	//ライセンス

	if(license)
		mButtonCreate(ct, WID_BTT_LICENSE, 0, 0, 0, "License");
	
	return p;
}


//******************************
// 関数
//******************************


/** @addtogroup sysdialog
 * @{ */

/** ソフト情報ダイアログ */

void mSysDlgAbout(mWindow *owner,const char *label)
{
	mDialog *p;

	p = _create_dialog(owner, label, NULL);
	if(!p) return;

	mWindowMoveResizeShow_hintSize(M_WINDOW(p));

	mDialogRun(M_DIALOG(p), TRUE);
}

/** ソフト情報ダイアログ (著作権とライセンスボタン) */

void mSysDlgAbout_license(mWindow *owner,const char *copying,const char *license)
{
	mDialog *p;

	p = _create_dialog(owner, copying, license);
	if(!p) return;

	mWindowMoveResizeShow_hintSize(M_WINDOW(p));

	mDialogRun(M_DIALOG(p), TRUE);
}

/* @} */
