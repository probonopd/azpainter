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
 * FileDialog
 * 
 * ファイル選択ダイアログ
 *****************************************/

#include "mDef.h"
#include "mFileDialog.h"
#include "mWidget.h"
#include "mWindow.h"
#include "mContainer.h"
#include "mCheckButton.h"
#include "mTrans.h"

#include "FileDialog.h"

#include "trgroup.h"


//-------------------------

typedef struct
{
	mWidget wg;
	mContainerData ct;
	mWindowData win;
	mDialogData dlg;
	mFileDialogData fdlg;

	mCheckButton *ck_ignore;
}_openfile;

//-------------------------

enum
{
	TRID_IGNORE_ALPHA = 1,

	WID_CK_IGNORE_ALPHA = 100
};

//-------------------------



//*****************************
// 画像を開くダイアログ
//*****************************


/** 拡張ウィジェット作成 */

static void _open_create_widget(_openfile *p)
{
	mWidget *ctv;

	M_TR_G(TRGROUP_FILEDIALOG);

	ctv = mContainerCreate(M_WIDGET(p), MCONTAINER_TYPE_VERT, 0, 4, 0);
	ctv->margin.top = 8;

	//アルファチャンネル無視

	p->ck_ignore = mCheckButtonCreate(ctv, WID_CK_IGNORE_ALPHA, 0, 0, 0, M_TR_T(TRID_IGNORE_ALPHA), FALSE);
}

/** 画像をレイヤとして開くダイアログ
 *
 * @return -1 でキャンセル */

int FileDialog_openLayerImage(mWindow *owner,const char *filter,
	const char *initdir,mStr *strfname)
{
	_openfile *p;
	int ret = -1;

	//作成

	p = (_openfile *)mFileDialogNew(sizeof(_openfile),
			owner, 0, MFILEDIALOG_TYPE_OPENFILE);
	if(!p) return FALSE;

	mFileDialogInit(M_FILEDIALOG(p), filter, 0, initdir, strfname);

	_open_create_widget(p);

	mFileDialogShow((mFileDialog *)p);

	//実行

	if(mDialogRun(M_DIALOG(p), FALSE))
	{
		ret = 0;
		
		if(mCheckButtonIsChecked(p->ck_ignore))
			ret |= 1;
	}

	mWidgetDestroy(M_WIDGET(p));

	return ret;
}

