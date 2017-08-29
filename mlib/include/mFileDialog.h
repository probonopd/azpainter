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

#ifndef MLIB_FILEDIALOG_H
#define MLIB_FILEDIALOG_H

#include "mDialog.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_FILEDIALOG(p)  ((mFileDialog *)(p))

typedef struct _mFileDialog mFileDialog;
typedef struct _mFileListView mFileListView;
typedef struct _mLineEdit mLineEdit;
typedef struct _mComboBox mComboBox;

typedef struct
{
	uint32_t style;
	int type;
	mStr *strdst;
	int *filtertypeDst;
	
	char *filter;

	void (*onSelectFile)(mFileDialog *,const char *filename);
	void (*onChangeType)(mFileDialog *,int type);
	mBool (*onOkCancel)(mFileDialog *,mBool bOK,const char *path);

	mFileListView *flist;
	mLineEdit *editdir,
		*editname;
	mComboBox *cbtype;
}mFileDialogData;

struct _mFileDialog
{
	mWidget wg;
	mContainerData ct;
	mWindowData win;
	mDialogData dlg;
	mFileDialogData fdlg;
};


enum MFILEDIALOG_STYLE
{
	MFILEDIALOG_S_MULTI_SEL = 1<<0,
	MFILEDIALOG_S_NO_OVERWRITE_MES = 1<<1
};

enum MFILEDIALOG_TYPE
{
	MFILEDIALOG_TYPE_OPENFILE,
	MFILEDIALOG_TYPE_SAVEFILE,
	MFILEDIALOG_TYPE_DIR
};


void mFileDialogDestroyHandle(mWidget *p);
int mFileDialogEventHandle(mWidget *wg,mEvent *ev);

mFileDialog *mFileDialogNew(int size,mWindow *owner,uint32_t style,int type);

void mFileDialogInit(mFileDialog *p,const char *filter,int deftype,
	const char *initdir,mStr *strdst);

void mFileDialogShow(mFileDialog *p);

#ifdef __cplusplus
}
#endif

#endif
