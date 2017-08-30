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
 * mDialog [ダイアログ]
 *****************************************/

#include "mDef.h"

#include "mDialog.h"

#include "mWindow.h"
#include "mWidget.h"
#include "mGui.h"
#include "mEvent.h"
#include "mKeyDef.h"


/*****************//**

@defgroup dialog mDialog
@brief ダイアログ

- \b MWINDOW_S_DIALOG スタイルは常に付く。
- デフォルトで、ESC キーが押されたら閉じる。 \n
- 戻り値のデフォルトは 0。

<h3>継承</h3>
mWidget \> mContainer \> mWindow \> mDialog

@ingroup group_window
@{

@file mDialog.h
@def M_DIALOG(p)
@struct mDialog
@struct mDialogData

*******************/


/** ダイアログ作成 */

mWidget *mDialogNew(int size,mWindow *owner,uint32_t style)
{
	mDialog *p;
	
	if(size < sizeof(mDialog)) size = sizeof(mDialog);
	
	p = (mDialog *)mWindowNew(size, owner, style | MWINDOW_S_DIALOG);
	if(!p) return NULL;
	
	p->wg.event = mDialogEventHandle;
	p->wg.fEventFilter |= MWIDGET_EVENTFILTER_KEY;
	
	return M_WIDGET(p);
}

/** ダイアログ実行
 * 
 * ウィンドウの表示などはあらかじめ行っておくこと。
 * 
 * @param destroy TRUE で終了後自身を削除する */

intptr_t mDialogRun(mDialog *p,mBool destroy)
{
	intptr_t ret;
	
	mAppRunModal(M_WINDOW(p));
	
	ret = p->dlg.retval;

	//削除
	
	if(destroy)
		mWidgetDestroy(M_WIDGET(p));
	
	mAppSync();

	return ret;
}

/** ダイアログ終了 */

void mDialogEnd(mDialog *p,intptr_t ret)
{
	p->dlg.retval = ret;
	
	mAppQuit();
}

/** イベントハンドラ */

int mDialogEventHandle(mWidget *wg,mEvent *ev)
{
	switch(ev->type)
	{
		case MEVENT_CLOSE:
			mAppQuit();
			return TRUE;
		
		case MEVENT_KEYDOWN:
			//ESC で閉じる
			if(!ev->key.bGrab && ev->key.code == MKEY_ESCAPE)
			{
				mWidgetAppendEvent(wg, MEVENT_CLOSE);
				return TRUE;
			}
			break;
	}

	return FALSE;
}

/** OK/キャンセルボタンを処理する汎用イベントハンドラ
 *
 * 通知イベントで M_WID_OK が来た時は TRUE、
 * M_WID_CANCEL が来た時は FALSE を返して終了する。*/

int mDialogEventHandle_okcancel(mWidget *wg,mEvent *ev)
{
	switch(ev->type)
	{
		case MEVENT_NOTIFY:
			if(ev->notify.id == M_WID_OK)
				mDialogEnd(M_DIALOG(wg), TRUE);
			else if(ev->notify.id == M_WID_CANCEL)
				mDialogEnd(M_DIALOG(wg), FALSE);
			break;
		default:
			return mDialogEventHandle(wg, ev);
	}

	return TRUE;
}

/** @} */
