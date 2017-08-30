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

/*******************************************
 * SelMaterialDlg
 *
 * 素材画像選択ダイアログ
 *******************************************/

#include <string.h>

#include "mDef.h"
#include "mGui.h"
#include "mStr.h"
#include "mDialog.h"
#include "mWidget.h"
#include "mContainer.h"
#include "mWindow.h"
#include "mButton.h"
#include "mListView.h"
#include "mTrans.h"
#include "mEvent.h"
#include "mUtilFile.h"
#include "mDirEntry.h"

#include "defConfig.h"
#include "defMacros.h"

#include "PrevImage8.h"
#include "ImageBuf8.h"
#include "ImageBuf24.h"

#include "trgroup.h"


//-------------------------

typedef struct
{
	mWidget wg;
	mContainerData ct;
	mWindowData win;
	mDialogData dlg;

	mStr *strret,
		strCurDirRel,   //カレントディレクトリ、相対パス
		strCurDirFull;  //カレントディレクトリ、フルパス (空でトップ)
	mBool bTexture,
		bSysDir;

	ImageBuf8 *img8;

	mListView *list;
	PrevImage8 *prev;
}SelMaterialDlg;

//-------------------------

#define WID_LIST  100

enum
{
	_ITEM_TYPE_PARENT,
	_ITEM_TYPE_DIR,
	_ITEM_TYPE_FILE,
	_ITEM_TYPE_TOP_SYSTEM,
	_ITEM_TYPE_TOP_USER
};

enum
{
	TRID_SELECT = 100,
	TRID_SELECT_TEXTURE
};

//-------------------------



//===============================
// リスト
//===============================


/** ソート関数 */

static int _sortfunc_list(mListItem *pi1,mListItem *pi2,intptr_t param)
{
	if(M_LISTVIEWITEM(pi1)->param < M_LISTVIEWITEM(pi2)->param)
		return -1;
	else if(M_LISTVIEWITEM(pi1)->param > M_LISTVIEWITEM(pi2)->param)
		return 1;
	else
		return strcmp(M_LISTVIEWITEM(pi1)->text, M_LISTVIEWITEM(pi2)->text);
}

/** ファイルリストセット */

static void _set_list_file(SelMaterialDlg *p)
{
	mDirEntry *dir;
	mStr str = MSTR_INIT;

	//戻る

	mListViewAddItem(p->list, "../", -1, 0, _ITEM_TYPE_PARENT);

	//ファイル

	dir = mDirEntryOpen(p->strCurDirFull.buf);
	if(!dir) return;

	while(mDirEntryRead(dir))
	{
		if(mDirEntryIsSpecName(dir)) continue;

		mDirEntryGetFileName_str(dir, &str, FALSE);

		if(mDirEntryIsDirectory(dir))
		{
			//ディレクトリ

			mStrAppendChar(&str, '/');
			mListViewAddItem(p->list, str.buf, -1, 0, _ITEM_TYPE_DIR);
		}
		else if(mStrPathCompareExts(&str, "png:jpg:jpeg:gif:bmp"))
			//画像ファイル
			mListViewAddItem(p->list, str.buf, -1, 0, _ITEM_TYPE_FILE);
	}

	mDirEntryClose(dir);

	mStrFree(&str);

	//ソート

	mListViewSortItem(p->list, _sortfunc_list, 0);
}

/** リストセット */

static void _set_list(SelMaterialDlg *p)
{
	mListViewDeleteAllItem(p->list);

	if(!mStrIsEmpty(&p->strCurDirFull))
		_set_list_file(p);
	else
	{
		//トップ

		mListViewAddItem(p->list, "<default>", -1, 0, _ITEM_TYPE_TOP_SYSTEM);
		mListViewAddItem(p->list, "<user dir>", -1, 0, _ITEM_TYPE_TOP_USER);
	}
}


//===============================
// sub
//===============================


/** ルートディレクトリ取得 */

static void _get_root_dir(SelMaterialDlg *p,mStr *str)
{
	if(p->bTexture)
	{
		if(p->bSysDir)
			mAppGetDataPath(str, APP_TEXTURE_PATH);
		else
			mStrCopy(str, &APP_CONF->strUserTextureDir);
	}
	else
	{
		if(p->bSysDir)
			mAppGetDataPath(str, APP_BRUSH_PATH);
		else
			mStrCopy(str, &APP_CONF->strUserBrushDir);
	}
}

/** 初期パスセット */

static void _set_init_path(SelMaterialDlg *p)
{
	mStr str = MSTR_INIT;

	mStrCopy(&str, p->strret);

	if(!mStrIsEmpty(&str) && !mStrCompareEq(&str, "?"))
	{
		//先頭が '/' でシステムディレクトリ

		if(str.buf[0] == '/')
		{
			mStrSetText(&str, p->strret->buf + 1);
			p->bSysDir = TRUE;
		}
		else
			p->bSysDir = FALSE;

		//カレントディレクトリ

		mStrPathGetDir(&p->strCurDirRel, str.buf);

		_get_root_dir(p, &p->strCurDirFull);
		mStrPathAdd(&p->strCurDirFull, p->strCurDirRel.buf);

		//ディレクトリが存在しなければトップ

		if(!mIsFileExist(p->strCurDirFull.buf, TRUE))
		{
			mStrEmpty(&p->strCurDirRel);
			mStrEmpty(&p->strCurDirFull);
		}
	}

	mStrFree(&str);
}

/** プレビュー画像読み込み */

static ImageBuf8 *_load_prev_image(SelMaterialDlg *p,const char *filename)
{
	mStr str = MSTR_INIT;
	ImageBuf24 *img24;
	ImageBuf8 *img8;

	//24bit 読み込み

	mStrCopy(&str, &p->strCurDirFull);
	mStrPathAdd(&str, filename);

	img24 = ImageBuf24_loadFile(str.buf);
	
	mStrFree(&str);

	if(!img24) return NULL;

	//8bit へ変換

	if(p->bTexture)
		img8 = ImageBuf8_createFromImageBuf24(img24);
	else
		img8 = ImageBuf8_createFromImageBuf24_forBrush(img24);

	ImageBuf24_free(img24);

	return img8;
}


//===============================
// イベント
//===============================


/** OK 終了 */

static void _end_ok(SelMaterialDlg *p)
{
	mListViewItem *pi;

	pi = mListViewGetFocusItem(p->list);
	if(!pi || pi->param != _ITEM_TYPE_FILE) return;

	//ファイル名

	if(p->bSysDir)
		mStrSetChar(p->strret, '/');
	else
		mStrEmpty(p->strret);

	mStrAppendStr(p->strret, &p->strCurDirRel);
	mStrPathAdd(p->strret, pi->text);

	//

	mDialogEnd(M_DIALOG(p), TRUE);
}

/** ディレクトリ移動 */

static void _move_dir(SelMaterialDlg *p,const char *text)
{
	mStr str = MSTR_INIT;

	mStrSetText(&str, text);
	mStrSetLen(&str, str.len - 1);

	mStrPathAdd(&p->strCurDirRel, str.buf);
	mStrPathAdd(&p->strCurDirFull, str.buf);

	mStrFree(&str);

	_set_list(p);
}

/** リスト選択変更 */

static void _list_change_sel(SelMaterialDlg *p,mListViewItem *pi)
{
	if(pi->param == _ITEM_TYPE_FILE)
	{
		//イメージ読み込み

		ImageBuf8_free(p->img8);

		p->img8 = _load_prev_image(p, pi->text);

		//セット

		PrevImage8_setImage(p->prev, p->img8, !(p->bTexture));
	}
	else
		PrevImage8_setImage(p->prev, NULL, 0);
}

/** リストアイテムダブルクリック */

static void _list_dblclk(SelMaterialDlg *p,mListViewItem *pi)
{
	switch(pi->param)
	{
		//ファイル
		case _ITEM_TYPE_FILE:
			_end_ok(p);
			break;

		//ディレクトリ
		case _ITEM_TYPE_DIR:
			_move_dir(p, pi->text);
			break;

		//親へ
		case _ITEM_TYPE_PARENT:
			if(mStrIsEmpty(&p->strCurDirRel))
				//システム/ユーザーのトップなら、最初に戻る
				mStrEmpty(&p->strCurDirFull);
			else
			{
				mStrPathRemoveFileName(&p->strCurDirRel);
				mStrPathRemoveFileName(&p->strCurDirFull);
			}

			_set_list(p);
			break;

		//system/user
		case _ITEM_TYPE_TOP_USER:
		case _ITEM_TYPE_TOP_SYSTEM:
			p->bSysDir = (pi->param == _ITEM_TYPE_TOP_SYSTEM);

			mStrEmpty(&p->strCurDirRel);
			_get_root_dir(p, &p->strCurDirFull);

			_set_list(p);
			break;
	}
}


/** イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	SelMaterialDlg *p = (SelMaterialDlg *)wg;

	if(ev->type == MEVENT_NOTIFY)
	{
		switch(ev->notify.id)
		{
			//リスト
			case WID_LIST:
				if(ev->notify.type == MLISTVIEW_N_CHANGE_FOCUS)
					_list_change_sel(p, (mListViewItem *)ev->notify.param1);
				else if(ev->notify.type == MLISTVIEW_N_ITEM_DBLCLK)
					_list_dblclk(p, (mListViewItem *)ev->notify.param1);
				break;
		
			//OK
			case M_WID_OK:
				_end_ok(p);
				break;
			//キャンセル
			case M_WID_CANCEL:
				mDialogEnd(M_DIALOG(wg), FALSE);
				break;
		}
	}

	return mDialogEventHandle(wg, ev);
}


//===============================
//
//===============================


/** 破棄 */

static void _destroy_handle(mWidget *wg)
{
	SelMaterialDlg *p = (SelMaterialDlg *)wg;

	mStrFree(&p->strCurDirFull);
	mStrFree(&p->strCurDirRel);

	ImageBuf8_free(p->img8);
}

/** 作成 */

static SelMaterialDlg *_dlg_create(mWindow *owner,mStr *strdst,mBool bTexture)
{
	SelMaterialDlg *p;
	mWidget *ct;
	mButton *btt;
	mListView *list;

	p = (SelMaterialDlg *)mDialogNew(sizeof(SelMaterialDlg), owner, MWINDOW_S_DIALOG_NORMAL);
	if(!p) return NULL;

	p->bTexture = bTexture;
	p->strret = strdst;

	p->wg.destroy = _destroy_handle;
	p->wg.event = _event_handle;
	p->ct.sepW = 10;

	//

	mContainerSetType(M_CONTAINER(p), MCONTAINER_TYPE_HORZ, 0);
	mContainerSetPadding_one(M_CONTAINER(p), 6);

	M_TR_G(TRGROUP_SELMATERIAL);

	mWindowSetTitle(M_WINDOW(p), M_TR_T(bTexture? TRID_SELECT_TEXTURE: TRID_SELECT));

	//------ ウィジェット

	//リスト

	p->list = list = mListViewNew(0, M_WIDGET(p),
		MLISTVIEW_S_AUTO_WIDTH,
		MSCROLLVIEW_S_HORZVERT_FRAME);

	list->wg.id = WID_LIST;
	list->wg.fLayout = MLF_EXPAND_WH;

	mWidgetSetInitSize_fontHeight(M_WIDGET(list), 16, 20);

	//右側

	ct = mContainerCreate(M_WIDGET(p), MCONTAINER_TYPE_VERT, 0, 0, MLF_EXPAND_H);

	p->prev = PrevImage8_new(ct, 100, 150, MLF_EXPAND_Y);  

	btt = mButtonCreate(ct, M_WID_OK, 0, MLF_EXPAND_W, M_MAKE_DW4(0,16,0,3),
		M_TR_T2(M_TRGROUP_SYS, M_TRSYS_OK));

	btt->wg.fState |= MWIDGET_STATE_ENTER_DEFAULT;

	mButtonCreate(ct, M_WID_CANCEL, 0, MLF_EXPAND_W, 0,
		M_TR_T2(M_TRGROUP_SYS, M_TRSYS_CANCEL));

	//--------

	_set_init_path(p);
	_set_list(p);

	return p;
}

/** ダイアログ実行 */

mBool SelMaterialDlg_run(mWindow *owner,mStr *strdst,mBool bTexture)
{
	SelMaterialDlg *p;

	p = _dlg_create(owner, strdst, bTexture);
	if(!p) return FALSE;

	mWindowMoveResizeShow_hintSize(M_WINDOW(p));

	return mDialogRun(M_DIALOG(p), TRUE);
}
