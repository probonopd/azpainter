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

#ifndef MLIB_FONT_H
#define MLIB_FONT_H

#include "mStrDef.h"

#ifdef __cplusplus
extern "C" {
#endif

struct _mFont
{
	int height,lineheight;
};

typedef struct _mFontInfo
{
	uint32_t mask;
	mStr strFamily,strStyle;
	double size;
	int weight,slant,render;
}mFontInfo;


enum MFONTINFO_MASK
{
	MFONTINFO_MASK_FAMILY = 1<<0,
	MFONTINFO_MASK_STYLE  = 1<<1,
	MFONTINFO_MASK_SIZE   = 1<<2,
	MFONTINFO_MASK_WEIGHT = 1<<3,
	MFONTINFO_MASK_SLANT  = 1<<4,
	MFONTINFO_MASK_RENDER = 1<<5,

	MFONTINFO_MASK_ALL = 0xff
};

enum MFONTINFO_WEIGHT
{
	MFONTINFO_WEIGHT_NORMAL = 100,
	MFONTINFO_WEIGHT_BOLD   = 200
};

enum MFONTINFO_SLANT
{
	MFONTINFO_SLANT_ROMAN,
	MFONTINFO_SLANT_ITALIC,
	MFONTINFO_SLANT_OBLIQUE
};

enum MFONTINFO_RENDER
{
	MFONTINFO_RENDER_DEFAULT,
	MFONTINFO_RENDER_MONO,
	MFONTINFO_RENDER_GRAY,
	MFONTINFO_RENDER_LCD_RGB,
	MFONTINFO_RENDER_LCD_BGR,
	MFONTINFO_RENDER_LCD_VRGB,
	MFONTINFO_RENDER_LCD_VBGR
};

/*---------*/

void mFontFree(mFont *font);
mFont *mFontCreate(mFontInfo *info);

void *mFontGetHandle(mFont *font);

void mFontDrawText(mFont *font,mPixbuf *img,int x,int y,
	const char *text,int len,mRgbCol col);
void mFontDrawTextHotkey(mFont *font,mPixbuf *img,int x,int y,
	const char *text,int len,mRgbCol col);
void mFontDrawTextUCS4(mFont *font,mPixbuf *img,int x,int y,
	const uint32_t *text,int len,mRgbCol col);

int mFontGetTextWidth(mFont *font,const char *text,int len);
int mFontGetTextWidthHotkey(mFont *font,const char *text,int len);
int mFontGetTextWidthUCS4(mFont *font,const uint32_t *text,int len);
int mFontGetCharWidthUCS4(mFont *font,uint32_t ucs);

/* common */

void mFontInfoFree(mFontInfo *p);
mFont *mFontCreateFromFormat(const char *format);
mFont *mFontCreateFromFormat_size(const char *format,double size);

void mFontFormatToInfo(mFontInfo *p,const char *format);
void mFontInfoToFormat(mStr *str,mFontInfo *info);

#ifdef __cplusplus
}
#endif

#endif
