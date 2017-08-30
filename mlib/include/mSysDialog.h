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

#ifndef MLIB_SYSTEMDIALOG_H
#define MLIB_SYSTEMDIALOG_H

typedef struct _mFontInfo mFontInfo;

#define MSYSDLG_OPENFILE_F_MULTI_SEL        1
#define MSYSDLG_SAVEFILE_F_NO_OVERWRITE_MES 2

#define MSYSDLG_INPUTTEXT_F_NOEMPTY  1

#define MSYSDLG_INPUTNUM_F_DEFAULT  1


#ifdef __cplusplus
extern "C" {
#endif

mBool mSysDlgOpenFile(mWindow *owner,const char *filter,int deftype,
	const char *initdir,uint32_t flags,mStr *strdst);

mBool mSysDlgSaveFile(mWindow *owner,const char *filter,int deftype,
	const char *initdir,uint32_t flags,mStr *strdst,int *filtertype);

mBool mSysDlgSelectDir(mWindow *owner,const char *initdir,uint32_t flags,mStr *strdst);

mBool mSysDlgSelectColor(mWindow *owner,mRgbCol *pcol);

mBool mSysDlgSelectFont(mWindow *owner,mFontInfo *info,uint32_t mask);
mBool mSysDlgSelectFont_format(mWindow *owner,mStr *strformat,uint32_t mask);

void mSysDlgAbout(mWindow *owner,const char *label);
void mSysDlgAbout_license(mWindow *owner,const char *copying,const char *license);

mBool mSysDlgInputText(mWindow *owner,
	const char *title,const char *message,mStr *str,uint32_t flags);

mBool mSysDlgInputNum(mWindow *owner,
	const char *title,const char *message,int *dst,int min,int max,uint32_t flags);

#ifdef __cplusplus
}
#endif

#endif
