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

#ifndef MLIB_DOCKWIDGET_H
#define MLIB_DOCKWIDGET_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _mDockWidget mDockWidget;

typedef mWidget *(*mDockWidgetCallbackCreate)(mDockWidget *,int id,mWidget *parent);
typedef int (*mDockWidgetCallbackArrange)(mDockWidget *,int id1,int id2);


typedef struct _mDockWidgetState
{
	uint32_t flags;
	int dockH;
	mBox boxWin;
}mDockWidgetState;


enum MDOCKWIDGET_STYLE
{
	MDOCKWIDGET_S_HAVE_CLOSE  = 1<<0,
	MDOCKWIDGET_S_HAVE_SWITCH = 1<<1,
	MDOCKWIDGET_S_HAVE_FOLD   = 1<<2,
	MDOCKWIDGET_S_EXPAND      = 1<<3,
	MDOCKWIDGET_S_NO_FOCUS    = 1<<4,

	MDOCKWIDGET_S_ALLBUTTON = MDOCKWIDGET_S_HAVE_CLOSE | MDOCKWIDGET_S_HAVE_SWITCH | MDOCKWIDGET_S_HAVE_FOLD
};

enum MDOCKWIDGET_FLAGS
{
	MDOCKWIDGET_F_EXIST   = 1<<0,
	MDOCKWIDGET_F_VISIBLE = 1<<1,
	MDOCKWIDGET_F_WINDOW  = 1<<2,
	MDOCKWIDGET_F_FOLDED  = 1<<3
};

enum MDOCKWIDGET_NOTIFY
{
	MDOCKWIDGET_NOTIFY_CLOSE = 1,
	MDOCKWIDGET_NOTIFY_TOGGLE_SWITCH,
	MDOCKWIDGET_NOTIFY_TOGGLE_FOLD
};


/*------*/

void mDockWidgetDestroy(mDockWidget *p);
mDockWidget *mDockWidgetNew(int id,uint32_t style);

void mDockWidgetSetCallback_create(mDockWidget *p,mDockWidgetCallbackCreate func);
void mDockWidgetSetCallback_arrange(mDockWidget *p,mDockWidgetCallbackArrange func);

void mDockWidgetSetWindowInfo(mDockWidget *p,mWindow *owner,uint32_t winstyle);
void mDockWidgetSetDockParent(mDockWidget *p,mWidget *parent);
void mDockWidgetSetState(mDockWidget *p,mDockWidgetState *info);
void mDockWidgetSetTitle(mDockWidget *p,const char *title);
void mDockWidgetSetParam(mDockWidget *p,intptr_t param);
void mDockWidgetSetFont(mDockWidget *p,mFont *font);
void mDockWidgetSetNotifyWidget(mDockWidget *p,mWidget *wg,int id);

void mDockWidgetGetState(mDockWidget *p,mDockWidgetState *state);
mBool mDockWidgetIsCreated(mDockWidget *p);
mBool mDockWidgetIsVisibleContents(mDockWidget *p);
mBool mDockWidgetCanTakeFocus(mDockWidget *p);
mBool mDockWidgetIsWindowMode(mDockWidget *p);
mWindow *mDockWidgetGetWindow(mDockWidget *p);
intptr_t mDockWidgetGetParam(mDockWidget *p);

void mDockWidgetCreateWidget(mDockWidget *p);
void mDockWidgetShowWindow(mDockWidget *p);
void mDockWidgetShow(mDockWidget *p,int type);
void mDockWidgetSetVisible(mDockWidget *p,int type);
void mDockWidgetSetWindowMode(mDockWidget *p,int type);
void mDockWidgetRelocate(mDockWidget *p,mBool relayout);

#ifdef __cplusplus
}
#endif

#endif
