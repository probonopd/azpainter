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

#ifndef MLIB_EDITTEXTBUF_H
#define MLIB_EDITTEXTBUF_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _mEditTextBuf
{
	mWidget *widget;
	uint32_t *text;
	uint16_t *widthbuf;
	int textLen,
		allocLen,
		curPos,
		curLine,
		selTop,
		selEnd,
		maxwidth,
		maxlines,
		multi_curx,
		multi_curline;
	mBool bMulti;
}mEditTextBuf;

enum
{
	MEDITTEXTBUF_KEYPROC_CURSOR = 1,
	MEDITTEXTBUF_KEYPROC_TEXT,
	MEDITTEXTBUF_KEYPROC_UPDATE
};


mEditTextBuf *mEditTextBuf_new(mWidget *widget,mBool multiline);
void mEditTextBuf_free(mEditTextBuf *p);

mBool mEditTextBuf_resize(mEditTextBuf *p,int len);
void mEditTextBuf_empty(mEditTextBuf *p);
mBool mEditTextBuf_isEmpty(mEditTextBuf *p);

mBool mEditTextBuf_isSelectAtPos(mEditTextBuf *p,int pos);
void mEditTextBuf_cursorToBottom(mEditTextBuf *p);
int mEditTextBuf_getPosAtPixel_forMulti(mEditTextBuf *p,int x,int y,int lineh);

void mEditTextBuf_setText(mEditTextBuf *p,const char *text);
mBool mEditTextBuf_insertText(mEditTextBuf *p,const char *text,char ch);

int mEditTextBuf_keyProc(mEditTextBuf *p,uint32_t key,uint32_t state,int editok);

mBool mEditTextBuf_selectAll(mEditTextBuf *p);
mBool mEditTextBuf_deleteSelText(mEditTextBuf *p);
void mEditTextBuf_deleteText(mEditTextBuf *p,int pos,int len);
void mEditTextBuf_expandSelect(mEditTextBuf *p,int pos);
void mEditTextBuf_moveCursorPos(mEditTextBuf *p,int pos,int select);
void mEditTextBuf_dragExpandSelect(mEditTextBuf *p,int pos);

mBool mEditTextBuf_paste(mEditTextBuf *p);
void mEditTextBuf_copy(mEditTextBuf *p);
mBool mEditTextBuf_cut(mEditTextBuf *p);

#ifdef __cplusplus
}
#endif

#endif
