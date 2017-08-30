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
 * メッセージボックス
 *****************************************/

#include "mDef.h"

#include "mMessageBox.h"

#include "mWidget.h"
#include "mContainer.h"
#include "mWindow.h"
#include "mDialog.h"

#include "mLabel.h"
#include "mButton.h"
#include "mCheckButton.h"

#include "mGui.h"
#include "mStr.h"
#include "mTrans.h"
#include "mEvent.h"
#include "mKeyDef.h"


//------------------------

#define M_MESBOX(p)  ((mMesBox *)(p))

typedef struct
{
	uint32_t fBtts,
		shortcutbtt,
		fRetOR;
}mMesBoxData;

typedef struct _mMesBox
{
	mWidget wg;
	mContainerData ct;
	mWindowData win;
	mDialogData dlg;
	mMesBoxData mb;
}mMesBox;


static int _event_handle(mWidget *wg,mEvent *ev);

//------------------------


//**********************************
// mMesBox ダイアログ
//**********************************


/** ボタン作成 */

static void _createBtt(mWidget *parent,uint32_t id,uint16_t strid,const char *def,char key)
{
	mStr str = MSTR_INIT;
	const char *pc;
	mButton *btt;

	//ラベル

	pc = mTransGetText2Raw(M_TRGROUP_SYS, strid);
	if(pc)
		mStrSetText(&str, pc);
	else
		mStrSetText(&str, def);

	//ショートカットキー

	mStrAppendChar(&str, '(');
	mStrAppendChar(&str, key);
	mStrAppendChar(&str, ')');

	//ボタン

	btt = mButtonCreate(parent, id, 0, 0, 0, str.buf);

	btt->wg.fOption |= MWIDGET_OPTION_ONFOCUS_NORMKEY_TO_WINDOW;

	mStrFree(&str);
}

/** ウィジェット作成 */

static void _createWidget(mWidget *parent,const char *message,uint32_t defbtt)
{
	mWidget *wg;
	uint32_t bt;

	bt = M_MESBOX(parent)->mb.fBtts;

	//メッセージ

	mLabelCreate(parent, 0, MLF_EXPAND_XY | MLF_CENTER_XY, 0, message);

	//"このメッセージを表示しない"

	if(bt & MMESBOX_NOTSHOW)
	{
		mCheckButtonCreate(parent, MMESBOX_NOTSHOW, 0, MLF_CENTER, 0,
			M_TR_T2(M_TRGROUP_SYS, M_TRSYS_NOTSHOW_THISMES), FALSE);
	}

	//ボタン

	wg = mContainerCreate(parent, MCONTAINER_TYPE_HORZ, 0, 4, MLF_EXPAND_X | MLF_CENTER);
	wg->margin.top = 6;

	if(bt & MMESBOX_SAVE) _createBtt(wg, MMESBOX_SAVE, M_TRSYS_SAVE, "Save", 'S');
	if(bt & MMESBOX_SAVENO) _createBtt(wg, MMESBOX_SAVENO, M_TRSYS_SAVENO, "NoSave", 'U');
	if(bt & MMESBOX_YES) _createBtt(wg, MMESBOX_YES, M_TRSYS_YES, "Yes", 'Y');
	if(bt & MMESBOX_NO) _createBtt(wg, MMESBOX_NO, M_TRSYS_NO, "No", 'N');
	if(bt & MMESBOX_OK) _createBtt(wg, MMESBOX_OK, M_TRSYS_OK, "OK", 'O');
	if(bt & MMESBOX_CANCEL) _createBtt(wg, MMESBOX_CANCEL, M_TRSYS_CANCEL, "Cancel", 'C');
	if(bt & MMESBOX_ABORT) _createBtt(wg, MMESBOX_ABORT, M_TRSYS_ABORT, "Abort", 'A');

	//デフォルトボタン

	if(defbtt)
	{
		wg = mWidgetFindByID(wg, defbtt);

		if(wg)
		{
			wg->fState |= MWIDGET_STATE_ENTER_DEFAULT;
			mWidgetSetFocus(wg);
		}
	}
}

/** 作成 */

mMesBox *mMesBoxNew(mWindow *owner,const char *title,const char *message,
	uint32_t btts,uint32_t defbtt)
{
	mMesBox *p;
	
	p = (mMesBox *)mDialogNew(sizeof(mMesBox), owner,
			MWINDOW_S_DIALOG_NORMAL | MWINDOW_S_NO_IM);

	if(!p) return NULL;
	
	p->wg.event = _event_handle;
	p->ct.sepW = 10;
	p->mb.fBtts = btts;

	mContainerSetPadding_one(M_CONTAINER(p), 10);

	mWindowSetTitle(M_WINDOW(p), title);
	mWindowKeepAbove(M_WINDOW(p), 1);

	//ウィジェット作成

	_createWidget(M_WIDGET(p), message, defbtt);

	//表示

	mWindowMoveResizeShow_hintSize(M_WINDOW(p));
	
	return p;
}

/** 閉じるボタン時 */

static void _event_close(mMesBox *p)
{
	int i,ret = 0;
	int btt[3] = {MMESBOX_CANCEL, MMESBOX_NO, MMESBOX_SAVENO};

	for(i = 0; i < 3; i++)
	{
		if(p->mb.fBtts & btt[i])
		{
			ret = btt[i];
			break;
		}
	}

	if(ret == 0) ret = MMESBOX_OK;

	mDialogEnd(M_DIALOG(p), ret | p->mb.fRetOR);
}

/** キー押し時 */

static void _event_keydown(mMesBox *p,mEvent *ev)
{
	int i;
	uint32_t c,btt;
	uint32_t val[] = {
		('O' << 24) | MMESBOX_OK,
		('C' << 24) | MMESBOX_CANCEL,
		('Y' << 24) | MMESBOX_YES,
		('N' << 24) | MMESBOX_NO,
		('S' << 24) | MMESBOX_SAVE,
		('U' << 24) | MMESBOX_SAVENO,
		('A' << 24) | MMESBOX_ABORT
	};
	mWidget *wg;

	c = ev->key.code;

	if(p->mb.shortcutbtt || (c < 'A' || c > 'Z'))
		return;

	//ボタン判定

	for(i = 0, btt = 0; i < 7; i++)
	{
		if(c == (val[i] >> 24))
		{
			btt = val[i] & 0xffffff;
			break;
		}
	}

	//ボタンを押す

	if(p->mb.fBtts & btt)
	{
		p->mb.shortcutbtt = btt;

		wg = mWidgetFindByID(M_WIDGET(p->wg.toplevel), btt);

		mButtonSetPress(M_BUTTON(wg), 1);
	}
}

/** イベント */

int _event_handle(mWidget *wg,mEvent *ev)
{
	mMesBox *p = M_MESBOX(wg);

	switch(ev->type)
	{
		case MEVENT_NOTIFY:
			if(ev->notify.widgetFrom->id == MMESBOX_NOTSHOW)
				p->mb.fRetOR ^= MMESBOX_NOTSHOW;
			else
				mDialogEnd(M_DIALOG(wg), ev->notify.widgetFrom->id | p->mb.fRetOR);
			break;
		//閉じる
		case MEVENT_CLOSE:
			_event_close(p);
			break;
		//キー押し
		case MEVENT_KEYDOWN:
			_event_keydown(p, ev);
			break;
		//キー離し
		/* ショートカットキーが離された場合、終了 */
		case MEVENT_KEYUP:
			if(p->mb.shortcutbtt)
				mDialogEnd(M_DIALOG(wg), p->mb.shortcutbtt | p->mb.fRetOR);
			break;
		default:
			return mDialogEventHandle(wg, ev);
	}

	return TRUE;
}



//**********************************
// mMessageBox 関数
//**********************************


/**
@defgroup messagebox mMessageBox
@brief メッセージボックス

@ingroup group_dialogfunc
@{

@file mMessageBox.h
@enum MMESBOX_BUTTONS
*/

/** メッセージボックス表示
 *
 * @param btts 使用するボタンを OR で複数指定
 * @param defbtt デフォルトのボタンを一つ指定 */

uint32_t mMessageBox(mWindow *owner,const char *title,const char *message,
	uint32_t btts,uint32_t defbtt)
{
	mMesBox *p;

	p = mMesBoxNew(owner, title, message, btts, defbtt);
	if(!p) return 0;

	return mDialogRun(M_DIALOG(p), TRUE);
}

/** メッセージボックス・エラー表示 */

void mMessageBoxErr(mWindow *owner,const char *message)
{
	mMessageBox(owner, "error", message, MMESBOX_OK, MMESBOX_OK);
}

/** メッセージボックス・エラー表示 (翻訳文字列から) */

void mMessageBoxErrTr(mWindow *owner,uint16_t groupid,uint16_t strid)
{
	mMessageBox(owner, "error", M_TR_T2(groupid, strid), MMESBOX_OK, MMESBOX_OK);
}

/** @} */
