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

#ifndef MLIB_X11_CLIPBOARD_H
#define MLIB_X11_CLIPBOARD_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _mX11Clipboard mX11Clipboard;

typedef struct
{
	void *buf;
	uint32_t size;
	Window winid;
	Atom prop,type;
	mBool multiple;
}mX11Clipboard_cb;

enum
{
	MX11CLIPB_DATTYPE_NONE,
	MX11CLIPB_DATTYPE_TEXT
};


mX11Clipboard *mX11ClipboardNew(void);
void mX11ClipboardDestroy(mX11Clipboard *p);
void mX11ClipboardFreeDat(mX11Clipboard *p);

mBool mX11ClipboardSetDat(mX11Clipboard *p,
	mWindow *win,int dattype,const void *datbuf,uint32_t datsize,
	Atom *atom_list,int atom_num,
	int (*callback)(mX11Clipboard *,mX11Clipboard_cb *));

char *mX11ClipboardGetText(mX11Clipboard *p,mWindow *win);

mBool mX11ClipboardSaveManager(mX11Clipboard *p,mWindow *win,int dattype);

void mX11ClipboardSelectionRequest(mX11Clipboard *p,XSelectionRequestEvent *ev);

#ifdef __cplusplus
}
#endif

#endif
