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

#ifndef MLIB_GUI_H
#define MLIB_GUI_H

#ifdef __cplusplus
extern "C" {
#endif

int mAppInit(int *argc,char **argv);
void mAppEnd(void);
void mAppQuit(void);

mBool mAppInitPenTablet();

void mAppSync(void);
void mAppWakeUpEvent(void);
void mAppMutexLock(void);
void mAppMutexUnlock(void);
void mAppBlockUserAction(mBool on);

void mAppRun(void);
void mAppRunModal(mWindow *modal);
void mAppRunPopup(mWindow *popup);

mWindow *mAppGetCurrentModalWindow();

char *mAppGetFilePath(const char *path);
void mAppSetDataPath(const char *path);
void mAppGetDataPath(mStr *str,const char *pathadd);
void mAppSetConfigPath(const char *path,mBool bHome);
void mAppGetConfigPath(mStr *str,const char *pathadd);
int mAppCreateConfigDir(const char *pathadd);
void mAppCopyFile_dataToConfig(const char *path);

mBool mAppSetDefaultFont(const char *format);
mBool mAppLoadThemeFile(const char *filename);

void mAppLoadTranslation(const void *defdat,const char *lang,const char *path);

void mGuiCalcHintSize(void);
void mGuiDraw(void);
mBool mGuiUpdate(void);
void mGuiUpdateAllWindows();

void *mGetFreeTypeLib(void);

void mGetDesktopWorkBox(mBox *box);
void mGetDesktopBox(mBox *box);

uint32_t mKeyRawToCode(uint32_t key);
int mKeyCodeToName(uint32_t c,char *buf,int bufsize);
int mRawKeyCodeToName(int key,char *buf,int bufsize);

#ifdef __cplusplus
}
#endif

#endif
