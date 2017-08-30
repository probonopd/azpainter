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

#ifndef MLIB_WINDOW_H
#define MLIB_WINDOW_H

#ifdef __cplusplus
extern "C" {
#endif

mWidget *mWindowNew(int size,mWindow *owner,uint32_t style);

mMenu *mWindowGetMenuInMenuBar(mWindow *p);
mWidget *mWindowGetFocusWidget(mWindow *p);

void mWindowActivate(mWindow *p);
void mWindowSetTitle(mWindow *p,const char *title);

void mWindowEnableDND(mWindow *p);
void mWindowEnablePenTablet(mWindow *p);

void mWindowSetIconFromFile(mWindow *p,const char *filename);
void mWindowSetIconFromBufPNG(mWindow *p,const void *buf,uint32_t bufsize);

mBool mWindowIsMinimized(mWindow *p);
mBool mWindowIsMaximized(mWindow *p);

mBool mWindowMinimize(mWindow *p,int type);
mBool mWindowMaximize(mWindow *p,int type);
void mWindowKeepAbove(mWindow *p,int type);
mBool mWindowFullScreen(mWindow *p,int type);

void mWindowMoveAdjust(mWindow *p,int x,int y,mBool workarea);
void mWindowMoveCenter(mWindow *p,mWindow *win);
void mWindowShowInit(mWindow *p,mBox *box,mBox *defbox,int defval,mBool show,mBool maximize);
void mWindowShowInitPos(mWindow *p,mPoint *pt,int defx,int defy,int defval,mBool show,mBool maximize);

void mWindowMoveResizeShow(mWindow *p,int w,int h);
void mWindowMoveResizeShow_hintSize(mWindow *p);

void mWindowAdjustPosDesktop(mWindow *p,mPoint *pt,mBool workarea);

void mWindowGetFrameWidth(mWindow *p,mRect *rc);
void mWindowGetFrameRootPos(mWindow *p,mPoint *pt);
void mWindowGetFullSize(mWindow *p,mSize *size);
void mWindowGetFullBox(mWindow *p,mBox *box);
void mWindowGetSaveBox(mWindow *p,mBox *box);

void mWindowUpdateRect(mWindow *p,mRect *rc);

#ifdef __cplusplus
}
#endif

#endif
