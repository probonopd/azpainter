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

/*****************************************
 * <X11> mCursor
 *****************************************/

#include "mSysX11.h"
#include "mCursor.h"


/**
@defgroup cursor mCursor
@brief カーソル

@ingroup group_system
@{

@file mCursor.h
@enum MCURSOR_DEF
*/


/** 解放 */

void mCursorFree(mCursor cur)
{
	if(cur)
		XFreeCursor(XDISP, cur);
}

/** データから作成(白黒)
 *
 * イメージデータは、下位ビットから上位ビットへの方向で。 @n
 *
 * [0byte] 幅 @n
 * [1byte] 高さ @n
 * [2byte] hotx @n
 * [3byte] hoty @n
 * [4~]    イメージデータ(1bit) 0=白 1=黒 @n
 * [4+(w+7)/8*h] マスクデータ(1bit) 0=透明 1=不透明
 */

mCursor mCursorCreateMono(const uint8_t *buf)
{
	int w,h;
	Pixmap img,mask;
	XColor col[2];
	Cursor curid = 0;

	w = buf[0];
	h = buf[1];

	//Pixmap 作成

	img  = XCreateBitmapFromData(XDISP, MAPP_SYS->root_window,
				(char *)buf + 4, w, h);

	mask = XCreateBitmapFromData(XDISP, MAPP_SYS->root_window,
				(char *)buf + 4 + ((w + 7) >> 3) * h, w, h);

	if(!img || !mask) goto END;

	//カーソル作成

	col[0].pixel = XBlackPixel(XDISP, MAPP_SYS->screen);
	col[0].flags = DoRed | DoGreen | DoBlue;
	col[0].red = col[0].green = col[0].blue = 0;

	col[1].pixel = XWhitePixel(XDISP, MAPP_SYS->screen);
	col[1].flags = DoRed | DoGreen | DoBlue;
	col[1].red = col[1].green = col[1]. blue = 0xffff;

	curid = XCreatePixmapCursor(XDISP, img, mask, col, col + 1, buf[2], buf[3]);

END:
	if(img) XFreePixmap(XDISP, img);
	if(mask) XFreePixmap(XDISP, mask);

	return curid;
}

/** 標準カーソル取得 */

mCursor mCursorGetDefault(int type)
{
	mCursor cur;

	switch(type)
	{
		case MCURSOR_DEF_HSPLIT:
			cur = MAPP_SYS->cursor_hsplit;
			break;
		case MCURSOR_DEF_VSPLIT:
			cur = MAPP_SYS->cursor_vsplit;
			break;
		default:
			cur = 0;
			break;
	}

	return cur;
}

/** @} */
