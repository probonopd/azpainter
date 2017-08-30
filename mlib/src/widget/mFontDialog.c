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
 * mFontDialog
 *****************************************/

#include <string.h>

#include "mDef.h"

#include "mWidget.h"
#include "mContainer.h"
#include "mWindow.h"
#include "mDialog.h"
#include "mLabel.h"
#include "mButton.h"
#include "mComboBox.h"
#include "mLineEdit.h"
#include "mListView.h"

#include "mStr.h"
#include "mFont.h"
#include "mTrans.h"
#include "mEvent.h"
#include "mEnumFont.h"
#include "mUtilStr.h"


//----------------------

typedef struct _mFontDialog
{
	mWidget wg;
	mContainerData ct;
	mWindowData win;
	mDialogData dlg;
	//

	mFontInfo *info;
	uint32_t mask;  //取得する項目

	mListView *lvfont,
		*lvstyle;
	mLineEdit *editsize;
	mComboBox *cbstyle,
		*cbweight,
		*cbslant,
		*cbrender;
}mFontDialog;

//----------------------

enum
{
	WID_LIST = 100,
	WID_BTT_PREV,
};

static int _event_handle(mWidget *wg,mEvent *ev);

//----------------------


//******************************
// プレビュー
//******************************


/** プレビューダイアログ実行 */

static void _run_previewdlg(mWindow *owner,mFontInfo *info)
{
	mDialog *p;
	mLineEdit *edit;
	mButton *btt;
	mFont *font;

	//------- 作成

	p = (mDialog *)mDialogNew(0, owner, MWINDOW_S_DIALOG_NORMAL);
	if(!p) return;

	p->wg.event = mDialogEventHandle_okcancel;

	M_TR_G(M_TRGROUP_SYS);

	mWindowSetTitle(M_WINDOW(p), M_TR_T(M_TRSYS_PREVIEW));

	mContainerSetPadding_one(M_CONTAINER(p), 8);

	p->ct.sepW = 12;

	//フォント

	font = mFontCreate(info);

	//エディット

	edit = mLineEditCreate(M_WIDGET(p), 0, 0, MLF_EXPAND_W, 0);

	edit->wg.initW = 300;
	edit->wg.font = font;

	mLineEditSetText(edit, M_TR_T(M_TRSYS_FONTPREVIEWTEXT));

	//OK

	btt = mButtonCreate(M_WIDGET(p), M_WID_OK, 0, MLF_RIGHT, 0, M_TR_T(M_TRSYS_OK));

	btt->wg.fState |= MWIDGET_STATE_ENTER_DEFAULT;

	//------ 実行

	mWindowMoveResizeShow_hintSize(M_WINDOW(p));

	mDialogRun(p, FALSE);

	mFontFree(font);

	mWidgetDestroy(M_WIDGET(p));
}


//******************************
// mFontDialog
//******************************


/** フォントリストセット */

static void _set_fontlist(mFontDialog *p)
{
	char **buf,**pp;
	mListViewItem *pi,*focus = NULL;
	mStr *strdef = &p->info->strFamily;

	//デフォルト

	pi = mListViewAddItem_textparam(p->lvfont, "(default)", -1);

	if(!(p->info->mask & MFONTINFO_MASK_FAMILY) || mStrIsEmpty(strdef))
		focus = pi;

	//リスト

	buf = mEnumFontFamily();
	if(!buf) return;

	for(pp = buf; *pp; pp++)
	{
		pi = mListViewAddItem_textparam(p->lvfont, *pp, 0);

		if(!focus && mStrCompareEq(strdef, *pp))
			focus = pi;
	}

	mFreeStrsBuf(buf);

	//幅

	mListViewSetWidthAuto(p->lvfont, TRUE);

	//フォーカス

	if(focus)
	{
		mListViewSetFocusItem(p->lvfont, focus);
		mListViewScrollToItem(p->lvfont, focus, 0);
	}
}

/** スタイルリストセット */

static void _set_stylelist(mFontDialog *p,mBool bInit)
{
	mListViewItem *pi,*focus = NULL;
	char **buf,**pp;
	mStr *strdef = &p->info->strStyle;

	if(!p->lvstyle) return;

	//クリア

	mListViewDeleteAllItem(p->lvstyle);

	//指定なし

	mListViewAddItem_textparam(p->lvstyle, "----", -1);

	//現在の選択フォントのスタイル

	pi = mListViewGetFocusItem(p->lvfont);

	if(pi && pi->param == 0)
	{
		buf = mEnumFontStyle(pi->text);

		if(buf)
		{
			for(pp = buf; *pp; pp++)
			{
				pi = mListViewAddItem_textparam(p->lvstyle, *pp, 0);

				if(bInit && !focus && mStrCompareEq(strdef, *pp))
					focus = pi;
			}
		}

		mFreeStrsBuf(buf);
	}

	//選択 (初期表示時のみ、指定スタイルにセット)

	mListViewSetFocusItem(p->lvstyle,
		(bInit && focus)? focus: mListViewGetTopItem(p->lvstyle));
}

/** ウィジェット作成 */

static void _create_widget(mFontDialog *p)
{
	mWidget *cth,*ct;
	uint32_t mask = p->mask;

	//水平コンテナ

	cth = mContainerCreate(M_WIDGET(p), MCONTAINER_TYPE_HORZ, 0, 10, MLF_EXPAND_WH);

	//---- リスト・プレビュー

	ct = mContainerCreate(cth, MCONTAINER_TYPE_VERT, 0, 6, MLF_EXPAND_H);

	//リスト

	p->lvfont = mListViewNew(0, ct,
		0, MSCROLLVIEW_S_VERT | MSCROLLVIEW_S_FRAME);

	p->lvfont->wg.id = WID_LIST;
	p->lvfont->wg.fLayout = MLF_EXPAND_H;
	p->lvfont->wg.initH = 300;

	_set_fontlist(p);

	//プレビュー

	mButtonCreate(ct, WID_BTT_PREV, 0, MLF_EXPAND_W, 0, M_TR_T(M_TRSYS_PREVIEW));

	//------ 各項目

	ct = mContainerCreate(cth, MCONTAINER_TYPE_GRID, 2, 0, MLF_EXPAND_WH);

	M_CONTAINER(ct)->ct.gridSepCol = 5;
	M_CONTAINER(ct)->ct.gridSepRow = 6;

	//スタイル

	if(mask & MFONTINFO_MASK_STYLE)
	{
		mLabelCreate(ct, 0, MLF_RIGHT, 0, M_TR_T(M_TRSYS_FONTSTYLE));

		p->lvstyle = mListViewNew(0, ct,
			0, MSCROLLVIEW_S_VERT | MSCROLLVIEW_S_FRAME);

		p->lvstyle->wg.fLayout = MLF_EXPAND_WH;
		p->lvstyle->wg.initW = 180;

		_set_stylelist(p, TRUE);
	}

	//サイズ

	if(mask & MFONTINFO_MASK_SIZE)
	{
		mLabelCreate(ct, 0, MLF_RIGHT | MLF_MIDDLE, 0, M_TR_T(M_TRSYS_FONTSIZE));

		p->editsize = mLineEditCreate(ct, 0, 0, MLF_EXPAND_W|MLF_MIDDLE, 0);

		p->editsize->wg.initW = 180;

		mLineEditSetDouble(p->editsize,
			(p->info->mask & MFONTINFO_MASK_SIZE)? p->info->size: 10, 1);
	}

	//太さ

	if(mask & MFONTINFO_MASK_WEIGHT)
	{
		mLabelCreate(ct, 0, MLF_RIGHT | MLF_MIDDLE, 0, M_TR_T(M_TRSYS_FONTWEIGHT));

		p->cbweight = mComboBoxCreate(ct, 0, 0, MLF_EXPAND_W|MLF_MIDDLE, 0);

		mComboBoxAddItem(p->cbweight, "----", -1);
		mComboBoxAddItem(p->cbweight, "Normal", MFONTINFO_WEIGHT_NORMAL);
		mComboBoxAddItem(p->cbweight, "Bold", MFONTINFO_WEIGHT_BOLD);

		mComboBoxSetWidthAuto(p->cbweight);

		if(p->info->mask & MFONTINFO_MASK_WEIGHT)
			mComboBoxSetSel_findParam_notfind(p->cbweight, p->info->weight, 0);
		else
			mComboBoxSetSel_index(p->cbweight, 0);
	}

	//傾き

	if(mask & MFONTINFO_MASK_SLANT)
	{
		mLabelCreate(ct, 0, MLF_RIGHT | MLF_MIDDLE, 0, M_TR_T(M_TRSYS_FONTSLANT));

		p->cbslant = mComboBoxCreate(ct, 0, 0, MLF_EXPAND_W|MLF_MIDDLE, 0);

		mComboBoxAddItem(p->cbslant, "----", -1);
		mComboBoxAddItem(p->cbslant, "Roman", MFONTINFO_SLANT_ROMAN);
		mComboBoxAddItem(p->cbslant, "Italic", MFONTINFO_SLANT_ITALIC);
		mComboBoxAddItem(p->cbslant, "Oblique", MFONTINFO_SLANT_OBLIQUE);

		mComboBoxSetWidthAuto(p->cbslant);

		if(p->info->mask & MFONTINFO_MASK_SLANT)
			mComboBoxSetSel_findParam_notfind(p->cbslant, p->info->slant, 0);
		else
			mComboBoxSetSel_index(p->cbslant, 0);
	}

	//レンダリング

	if(mask & MFONTINFO_MASK_RENDER)
	{
		mLabelCreate(ct, 0, MLF_RIGHT | MLF_MIDDLE, 0, M_TR_T(M_TRSYS_FONTRENDER));

		p->cbrender = mComboBoxCreate(ct, 0, 0, MLF_EXPAND_W|MLF_MIDDLE, 0);

		mComboBoxAddItems(p->cbrender,
			"Default\tMono\tGray\tLCD (RGB)\tLCD (BGR)\tLCD (V RGB)\tLCD (V BGR)",
			0);

		mComboBoxSetWidthAuto(p->cbrender);

		mComboBoxSetSel_index(p->cbrender,
			(p->info->mask & MFONTINFO_MASK_RENDER)? p->info->render: 0);
	}
}

/** 各値を取得 */

static void _get_param(mFontDialog *p,mFontInfo *pd)
{
	mListViewItem *pi;
	uint32_t mask = 0;

	//ファミリ名

	pi = mListViewGetFocusItem(p->lvfont);

	if(pi)
	{
		if(pi->param == 0)
		{
			//選択された名前
			mask |= MFONTINFO_MASK_FAMILY;
			mStrSetText(&pd->strFamily, pi->text);
		}
		else
		{
			//default の場合はファミリ名なし
			mStrEmpty(&pd->strFamily);
		}
	}
	else if(!mStrIsEmpty(&pd->strFamily))
		//選択なしの場合は、元の名前が空でなければそのままに
		mask |= MFONTINFO_MASK_FAMILY;

	//スタイル

	if(p->lvstyle)
	{
		pi = mListViewGetFocusItem(p->lvstyle);

		if(pi && pi->param == 0)
		{
			mask |= MFONTINFO_MASK_STYLE;
			mStrSetText(&pd->strStyle, pi->text);
		}
	}

	//サイズ

	if(p->editsize)
	{
		mask |= MFONTINFO_MASK_SIZE;
		pd->size = mLineEditGetDouble(p->editsize);
	}

	//太さ

	if(p->cbweight)
	{
		pi = mComboBoxGetSelItem(p->cbweight);

		if(pi && pi->param != -1)
		{
			mask |= MFONTINFO_MASK_WEIGHT;
			pd->weight = pi->param;
		}
	}

	//傾き

	if(p->cbslant)
	{
		pi = mComboBoxGetSelItem(p->cbslant);

		if(pi && pi->param != -1)
		{
			mask |= MFONTINFO_MASK_SLANT;
			pd->slant = pi->param;
		}
	}

	//レンダリング

	if(p->cbrender)
	{
		pi = mComboBoxGetSelItem(p->cbrender);
		if(pi)
		{
			mask |= MFONTINFO_MASK_RENDER;
			pd->render = pi->param;
		}
	}

	//マスク

	pd->mask = mask;
}


//====================


/** 作成 */

mFontDialog *mFontDialogNew(mWindow *owner,mFontInfo *info,uint32_t mask)
{
	mFontDialog *p;
	
	p = (mFontDialog *)mDialogNew(sizeof(mFontDialog), owner, MWINDOW_S_DIALOG_NORMAL);
	if(!p) return NULL;
	
	p->wg.event = _event_handle;

	p->info = info;
	p->mask = mask;

	//

	M_TR_G(M_TRGROUP_SYS);

	mWindowSetTitle(M_WINDOW(p), M_TR_T(M_TRSYS_TITLE_SELECTFONT));

	//

	mContainerSetPadding_one(M_CONTAINER(p), 8);
	p->ct.sepW = 15;

	//ウィジェット

	_create_widget(p);

	//OK/Cancel

	mContainerCreateOkCancelButton(M_WIDGET(p));
	
	return p;
}

/** プレビュー実行 */

static void _run_preview(mFontDialog *p)
{
	mFontInfo info;

	memset(&info, 0, sizeof(mFontInfo));

	_get_param(p, &info);

	_run_previewdlg(M_WINDOW(p), &info);

	mFontInfoFree(&info);
}

/** イベントハンドラ */

int _event_handle(mWidget *wg,mEvent *ev)
{
	mFontDialog *p = (mFontDialog *)wg;

	if(ev->type == MEVENT_NOTIFY)
	{
		switch(ev->notify.widgetFrom->id)
		{
			//フォントリスト
			case WID_LIST:
				if(ev->notify.type == MLISTVIEW_N_CHANGE_FOCUS)
					_set_stylelist(p, FALSE);
				break;
			//プレビュー
			case WID_BTT_PREV:
				_run_preview(p);
				break;
		
			//OK
			case M_WID_OK:
				_get_param(p, p->info);
			
				mDialogEnd(M_DIALOG(wg), TRUE);
				break;
			case M_WID_CANCEL:
				mDialogEnd(M_DIALOG(wg), FALSE);
				break;
		}
	
		return 1;
	}
	else
		return mDialogEventHandle(wg, ev);
}



//*******************************
// 関数
//*******************************


/** フォント選択
 *
 * @param mask MFONTINFO_MASK_*。設定を取得したい項目。
 * @ingroup sysdialog */

mBool mSysDlgSelectFont(mWindow *owner,mFontInfo *info,uint32_t mask)
{
	mFontDialog *p;

	p = mFontDialogNew(owner, info, mask);
	if(!p) return FALSE;

	mWindowMoveResizeShow_hintSize(M_WINDOW(p));

	return mDialogRun(M_DIALOG(p), TRUE);
}

/** フォント選択 (文字列フォーマットで)
 *
 * @param mask MFONTINFO_MASK_*。設定を取得したい項目。
 * @ingroup sysdialog */

mBool mSysDlgSelectFont_format(mWindow *owner,mStr *strformat,uint32_t mask)
{
	mFontDialog *p;
	mFontInfo info;
	mBool ret = FALSE;

	//mFontInfo

	mMemzero(&info, sizeof(mFontInfo));

	mFontFormatToInfo(&info, strformat->buf);

	//ダイアログ

	p = mFontDialogNew(owner, &info, mask);

	if(p)
	{
		mWindowMoveResizeShow_hintSize(M_WINDOW(p));

		ret = mDialogRun(M_DIALOG(p), TRUE);

		if(ret)
			mFontInfoToFormat(strformat, &info);
	}

	mFontInfoFree(&info);

	return ret;
}

