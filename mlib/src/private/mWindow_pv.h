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

#ifndef MLIB_WINDOW_PV_H
#define MLIB_WINDOW_PV_H

#ifdef __cplusplus
extern "C" {
#endif

int __mWindowNew(mWindow *p);
void __mWindowDestroy(mWindow *p);

void __mWindowShow(mWindow *p);
void __mWindowHide(mWindow *p);
void __mWindowMinimize(mWindow *p,int type);
void __mWindowMaximize(mWindow *p,int type);

void __mWindowActivate(mWindow *p);
void __mWindowMove(mWindow *p,int x,int y);
void __mWindowResize(mWindow *p,int w,int h);

mBool __mWindowGrabPointer(mWindow *p,mCursor cur,int device_id);
void __mWindowUngrabPointer(mWindow *p);
mBool __mWindowGrabKeyboard(mWindow *p);
void __mWindowUngrabKeyboard(mWindow *p);

void __mWindowSetCursor(mWindow *win,mCursor cur);
void __mWindowSetIcon(mWindow *win,mImageBuf *img);

//

void __mWindowOnResize(mWindow *p,int w,int h);

#ifdef __cplusplus
}
#endif

#endif
