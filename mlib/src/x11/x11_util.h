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

#ifndef MLIB_X11_UTIL_H
#define MLIB_X11_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

Atom mX11GetAtom(const char *name);

void mX11SetProperty32(Window id,const char *name,Atom proptype,void *val,int num);
void mX11SetPropertyCARDINAL(Window id,Atom prop,void *val,int num);
void mX11SetPropertyAtom(Window id,Atom prop,Atom *atoms,int num);
void mX11SetProperty8(Window id,Atom prop,Atom type,
	const void *buf,long size,mBool append);
void mX11SetPropertyCompoundText(Window id,Atom prop,const char *utf8,int len);

mBool mX11GetProperty32Array(Window id,Atom prop,Atom proptype,void *buf,int num);
void *mX11GetProperty32(Window id,Atom prop,Atom proptype,int *resnum);
void *mX11GetProperty8(Window id,Atom prop,mBool append_null,uint32_t *ret_size);

void mX11SetEventClientMessage(XEvent *ev,Window win,Atom mestype);
void mX11SendClientMessageToRoot(Window id,Atom mestype,void *data,int num);
void mX11SendClientMessageToRoot_string(const char *mestype,const char *mestype_begin,const char *sendstr);
void mX11SendStartupNotify_complete();
int mX11GetEventTimeout(int evtype,int timeoutms,XEvent *ev);

void mX11Send_NET_WM_STATE(Window id,int action,Atom data1,Atom data2);

mBool mX11SelectionConvert(Window id,Atom selection,Atom target,XEvent *ev);
Atom mX11GetSelectionTargetType(Window id,
	Atom atom_selection,Atom *target_list,int target_num);
void *mX11GetSelectionDat(Window id,Atom selection,Atom target,mBool append_null,uint32_t *ret_size);
char *mX11GetSelectionCompoundText(Window id,Atom selection);

Window mX11GetFrameWindow(Window id);

#ifdef __cplusplus
}
#endif

#endif
