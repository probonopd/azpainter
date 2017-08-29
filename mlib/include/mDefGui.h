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

#ifndef MLIB_DEF_GUI_H
#define MLIB_DEF_GUI_H

typedef uint32_t  mRgbCol;
typedef uint32_t  mPixCol;
typedef uintptr_t mCursor;
typedef struct _mWidget    mWidget;
typedef struct _mContainer mContainer;
typedef struct _mWindow    mWindow;
typedef struct _mMenu      mMenu;
typedef struct _mEvent     mEvent;
typedef struct _mPixbuf    mPixbuf;
typedef struct _mImageBuf  mImageBuf;
typedef struct _mImageList mImageList;

#define M_WID_OK		1
#define M_WID_CANCEL	2

#define M_WIDGET(p)    ((mWidget *)(p))
#define M_CONTAINER(p) ((mContainer *)(p))
#define M_WINDOW(p)    ((mWindow *)(p))

#define MACCKEY_SHIFT   0x8000
#define MACCKEY_CTRL    0x4000
#define MACCKEY_ALT     0x2000
#define MACCKEY_KEYMASK 0x0fff
#define MACCKEY_MODMASK 0xe000

enum M_MODSTATE
{
	M_MODS_SHIFT = 1<<0,
	M_MODS_CTRL  = 1<<1,
	M_MODS_ALT   = 1<<2,
	
	M_MODS_MASK_KEY = M_MODS_SHIFT|M_MODS_CTRL|M_MODS_ALT
};

enum M_BUTTON
{
	M_BTT_NONE,
	M_BTT_LEFT,
	M_BTT_MIDDLE,
	M_BTT_RIGHT,
	M_BTT_SCR_UP,
	M_BTT_SCR_DOWN,
	M_BTT_SCR_LEFT,
	M_BTT_SCR_RIGHT
};


typedef struct
{
	uint16_t left,top,right,bottom;
}mWidgetSpacing;


/* function */

#ifdef __cplusplus
extern "C" {
#endif

mPixCol mRGBtoPix(mRgbCol col);
mPixCol mRGBtoPix2(uint8_t r,uint8_t g,uint8_t b);
mPixCol mGraytoPix(uint8_t c);
mRgbCol mPixtoRGB(mPixCol col);
mRgbCol mBlendRGB_256(mRgbCol colsrc,mRgbCol coldst,int a);

#ifdef __cplusplus
}
#endif

#endif
