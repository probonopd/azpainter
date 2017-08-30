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

#ifndef MLIB_WIDGET_PV_H
#define MLIB_WIDGET_PV_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _mWidgetLabelTextRowInfo
{
	int pos,len,width;
}mWidgetLabelTextRowInfo;


/*---- mWidget ----*/

mWidget *__mWidgetGetTreeNext(mWidget *p);
mWidget *__mWidgetGetTreeNextPass(mWidget *p);
mWidget *__mWidgetGetTreeNextPass_root(mWidget *p,mWidget *root);
mWidget *__mWidgetGetTreeNext_root(mWidget *p,mWidget *root);

mWidget *__mWidgetGetTreeNext_follow_ui(mWidget *p,uint32_t follow_mask,uint32_t run_mask);
mWidget *__mWidgetGetTreeNext_follow_uidraw(mWidget *p);

void __mWidgetSetTreeUpper_ui(mWidget *p,uint32_t flags);

mWidget *__mWidgetGetTreeNext_state(mWidget *p,mWidget *root,uint32_t mask);

mBool __mWidgetGetClipRect(mWidget *wg,mRect *rcdst);

void __mWidgetOnResize(mWidget *p);
void __mWidgetCalcHint(mWidget *p);

int __mWidgetGetLayoutW(mWidget *p);
int __mWidgetGetLayoutH(mWidget *p);
int __mWidgetGetLayoutW_space(mWidget *p);
int __mWidgetGetLayoutH_space(mWidget *p);
void __mWidgetGetLayoutCalcSize(mWidget *p,mSize *hint,mSize *init);

void __mWidgetSetFocus(mWidget *p,int by);
void __mWidgetKillFocus(mWidget *p,int by);
void __mWidgetRemoveFocus(mWidget *p);
void __mWidgetRemoveFocus_byDisable(mWidget *p);

mWidgetLabelTextRowInfo *__mWidgetGetLabelTextRowInfo(const char *text);
void __mWidgetGetLabelTextSize(mWidget *p,const char *text,mWidgetLabelTextRowInfo *buf,mSize *dstsize);

/*---- mWindow ----*/

mWidget *__mWindowGetDefaultFocusWidget(mWindow *win);
mBool __mWindowSetFocus(mWindow *win,mWidget *focus,int by);
mBool __mWindowMoveNextFocus(mWindow *win);

#ifdef __cplusplus
}
#endif

#endif
