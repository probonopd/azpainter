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

#ifndef MLIB_X11_EVENT_SUB_H
#define MLIB_X11_EVENT_SUB_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	XEvent *xev;
	mEvent *ev;
	mWindow *win;
	mAppRunDat *rundat;
	int modalskip;
}_X11_EVENT;

#define _IS_BLOCK_USERACTION  (MAPP->flags & MAPP_FLAGS_BLOCK_USER_ACTION)


mWindow *mX11Event_getWindow(Window id);
void mX11Event_setTime(XEvent *p);
uint32_t mX11Event_convertState(unsigned int state);
void mX11Event_change_NET_WM_STATE(_X11_EVENT *p);
void mX11Event_onMap(mWindow *p);

mWidget *mX11Event_procButton_PressRelease(_X11_EVENT *p,
	int x,int y,int rootx,int rooty,int btt,uint32_t state,Time time,mBool press,int *evtype);

mWidget *mX11Event_procMotion(_X11_EVENT *p,int x,int y,int rootx,int rooty,uint32_t state);

#ifdef __cplusplus
}
#endif

#endif
