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

#ifndef MLIB_FREETYPE_H
#define MLIB_FREETYPE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _FcPattern mFcPattern;
typedef struct _mFontInfo mFontInfo;

typedef struct _mFreeTypeInfo
{
	uint32_t flags,fLoadGlyph;
	FT_Render_Mode nRenderMode;
	int nLCDFilter;
	double dpi,size;
	FT_Matrix matrix;
}mFreeTypeInfo;

typedef struct
{
	int height,
		lineheight,
		baseline,
		underline;
}mFreeTypeMetricsInfo;


#define MFTINFO_F_SUBPIXEL_BGR 1
#define MFTINFO_F_EMBOLDEN     2
#define MFTINFO_F_MATRIX       4

enum MFT_HINTING
{
	MFT_HINTING_NONE,
	MFT_HINTING_DEFAULT,
	MFT_HINTING_MAX
};

/*------*/

void mFreeTypeGetInfoByFontConfig(mFreeTypeInfo *dst,mFcPattern *pat,mFontInfo *info);
void mFreeTypeSetInfo_hinting(mFreeTypeInfo *dst,int type);

void mFreeTypeGetMetricsInfo(FT_Library lib,FT_Face face,mFreeTypeInfo *info,
	mFreeTypeMetricsInfo *dst);

int mFreeTypeGetHeightFromGlyph(FT_Library lib,FT_Face face,
	mFreeTypeInfo *info,int ascender,uint32_t code);

FT_BitmapGlyph mFreeTypeGetBitmapGlyph(FT_Library lib,FT_Face face,mFreeTypeInfo *info,uint32_t code);

void *mFreeTypeGetGSUB(FT_Face face);

mRgbCol mFreeTypeBlendColorGray(mRgbCol bgcol,mRgbCol fgcol,int a);
mRgbCol mFreeTypeBlendColorLCD(mRgbCol bgcol,mRgbCol fgcol,int ra,int ga,int ba);

#ifdef __cplusplus
}
#endif

#endif
