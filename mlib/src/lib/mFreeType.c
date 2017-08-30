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
 * mFreeType
 *****************************************/

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H
#include FT_BITMAP_H
#include FT_LCD_FILTER_H
#include FT_OPENTYPE_VALIDATE_H

#include "mDef.h"
#include "mFreeType.h"
#include "mFontConfig.h"
#include "mFont.h"


//------------------

#define _FT_UINT_TO_PIX(v,s)      (FT_MulFix((v),(s)) >> 6)
#define _FT_UINT_TO_PIX_CEIL(v,s) ((FT_MulFix((v),(s)) + 32) >> 6)

//------------------


/**
@defgroup freetype mFreeType
@brief FreeType 用関数

@ingroup group_lib

@{
@file mFreeType.h

@struct mFreeTypeInfo
@struct mFreeTypeMetricsInfo
*/


/** FcPattern と mFontInfo から描画用情報取得 */

void mFreeTypeGetInfoByFontConfig(mFreeTypeInfo *dst,mFcPattern *pat,mFontInfo *info)
{
	uint32_t f;
	int n1,n2,hintstyle;
	double matrix[4],size;
	
	dst->flags = 0;

	hintstyle = mFontConfigGetInt(pat, "hintstyle", 1);

	//dpi

	dst->dpi = mFontConfigGetDouble(pat, "dpi", 96);

	//サイズ

	if(mFontConfigGetDouble2(pat, "size", &size))
		dst->size = size;
	else if(mFontConfigGetDouble2(pat, "pixelsize", &size))
		dst->size = -size;
	else
		dst->size = 10;
		
	//FT_Get_Glyph() 時のフラグ
	
	f = 0;

	if(!mFontConfigGetBool(pat, "hinting", TRUE)) f |= FT_LOAD_NO_HINTING;
	if(!mFontConfigGetBool(pat, "embeddedbitmap", TRUE)) f |= FT_LOAD_NO_BITMAP;
	if(mFontConfigGetBool(pat, "force_autohint", FALSE)) f |= FT_LOAD_FORCE_AUTOHINT;
	if(!mFontConfigGetBool(pat, "autohint", TRUE)) f |= FT_LOAD_NO_AUTOHINT;
	if(hintstyle == 1) f |= FT_LOAD_TARGET_LIGHT;  /* hintstyle:1 -> FC_HINT_SLIGHT */

	dst->fLoadGlyph = f;
	
	//FT_Glyph_To_Bitmap() 時のモード + RGB/BGR

	if((info->mask & MFONTINFO_MASK_RENDER)
		&& info->render != MFONTINFO_RENDER_DEFAULT)
	{
		//info 指定
		
		n1 = info->render;

		//MONO 以外なら埋め込みビットマップを無効に

		if(n1 != MFONTINFO_RENDER_MONO)
			dst->fLoadGlyph |= FT_LOAD_NO_BITMAP;
	}
	else
	{
		//デフォルト

		n1 = mFontConfigGetBool(pat, "antialias", TRUE);
		n2 = mFontConfigGetInt(pat, "rgba", 0);

		if(n1 == 0)
			n1 = MFONTINFO_RENDER_MONO;
		else
			n1 = MFONTINFO_RENDER_GRAY + n2;
	}

	switch(n1)
	{
		case MFONTINFO_RENDER_MONO:
			dst->nRenderMode = FT_RENDER_MODE_MONO;
			break;
		case MFONTINFO_RENDER_LCD_RGB:
			dst->nRenderMode = FT_RENDER_MODE_LCD;
			break;
		case MFONTINFO_RENDER_LCD_BGR:
			dst->nRenderMode = FT_RENDER_MODE_LCD;
			dst->flags |= MFTINFO_F_SUBPIXEL_BGR;
			break;
		case MFONTINFO_RENDER_LCD_VRGB:
			dst->nRenderMode = FT_RENDER_MODE_LCD_V;
			break;
		case MFONTINFO_RENDER_LCD_VBGR:
			dst->nRenderMode = FT_RENDER_MODE_LCD_V;
			dst->flags |= MFTINFO_F_SUBPIXEL_BGR;
			break;
		default:
			dst->nRenderMode = FT_RENDER_MODE_NORMAL;
			break;
	}
		
	//LCD Filter
	
	n1 = mFontConfigGetInt(pat, "lcdfilter", 1);
	
	if(n1 == 1)
		dst->nLCDFilter = FT_LCD_FILTER_DEFAULT;
	else if(n1 == 2)
		dst->nLCDFilter = FT_LCD_FILTER_LIGHT;
	else if(n1 == 3)
		dst->nLCDFilter = FT_LCD_FILTER_LEGACY;
	else
		dst->nLCDFilter = FT_LCD_FILTER_NONE;
	
	//太字
	
	if(mFontConfigGetBool(pat, "embolden", FALSE))
		dst->flags |= MFTINFO_F_EMBOLDEN;
	
	//斜体 (matrix)
	
	if(mFontConfigGetMatrix(pat, "matrix", matrix))
	{
		dst->flags |= MFTINFO_F_MATRIX;
		dst->matrix.xx = (int)(matrix[0] * (1<<16) + 0.5);
		dst->matrix.xy = (int)(matrix[1] * (1<<16) + 0.5);
		dst->matrix.yx = (int)(matrix[2] * (1<<16) + 0.5);
		dst->matrix.yy = (int)(matrix[3] * (1<<16) + 0.5);
	}
}

/** ヒンティングのタイプをセット */

void mFreeTypeSetInfo_hinting(mFreeTypeInfo *dst,int type)
{
	uint32_t f;

	f = dst->fLoadGlyph &
		(~(FT_LOAD_NO_HINTING | FT_LOAD_FORCE_AUTOHINT | FT_LOAD_NO_AUTOHINT | FT_LOAD_TARGET_LIGHT));

	switch(type)
	{
		case MFT_HINTING_NONE:
			f |= FT_LOAD_NO_HINTING;
			break;
		case MFT_HINTING_DEFAULT:
			f |= FT_LOAD_TARGET_NORMAL;
			break;
		case MFT_HINTING_MAX:
			f |= FT_LOAD_TARGET_LIGHT;
			break;
	}

	dst->fLoadGlyph = f;
}

/** FT_Face から文字高さなどの情報を取得
 *
 * height:文字高さ、lineheight:行の高さ、underline:ベースラインからの下線位置 */

void mFreeTypeGetMetricsInfo(FT_Library lib,FT_Face face,
	mFreeTypeInfo *info,mFreeTypeMetricsInfo *dst)
{
	FT_Size_Metrics *pm;
	FT_Fixed scale;
	int asc,desc,h;

	pm = &face->size->metrics;

	if(FT_IS_SCALABLE(face))
	{
		//-------- スケーラブル
	
		scale = pm->y_scale;

		asc = _FT_UINT_TO_PIX_CEIL(face->ascender, scale);
		desc = _FT_UINT_TO_PIX_CEIL(-(face->descender), scale);

		dst->height = asc + desc;
		dst->lineheight = _FT_UINT_TO_PIX(face->height, scale);
		dst->baseline = asc;
		dst->underline = _FT_UINT_TO_PIX(-(face->underline_position), scale);

		if(dst->underline > desc) dst->underline = 0;

		/* IPAフォントなど、実際の描画時よりも高さが1px少ない場合があるので、
		 * 下端までピクセルがある 'g' などのグリフを取得して比較する */

		h = mFreeTypeGetHeightFromGlyph(lib, face, info, asc, 'g');
		if(h != -1)
		{
			if(h > dst->height) dst->height = h;
			if(h > dst->lineheight) dst->lineheight = h;
		}
	}
	else if(pm->height)
	{
		//ビットマップフォント (metrics の値がある場合)
	
		dst->height = (pm->ascender - pm->descender) >> 6;
		dst->lineheight = pm->height >> 6;
		dst->baseline = pm->ascender >> 6;
		dst->underline = 1;
	}
	else if(face->available_sizes)
	{
		dst->height = face->available_sizes->y_ppem >> 6;
		dst->lineheight = face->available_sizes->height;
		dst->baseline = dst->height - 3;
		dst->underline = 1;
	}
}

/** グリフから文字高さ取得
 *
 * @return -1 で失敗 */

int mFreeTypeGetHeightFromGlyph(FT_Library lib,FT_Face face,
	mFreeTypeInfo *info,int ascender,uint32_t code)
{
	FT_BitmapGlyph glyph;
	int h;

	glyph = mFreeTypeGetBitmapGlyph(lib, face, info, code);
	if(!glyph) return -1;

	//高さ (LCD_V の場合は1/3)

	h = glyph->bitmap.rows;

	if(glyph->bitmap.pixel_mode == FT_PIXEL_MODE_LCD_V)
		h /= 3;

	//

	h += ascender - glyph->top;

	FT_Done_Glyph((FT_Glyph)glyph);

	return h; 
}

/** 文字コードからビットマップグリフ取得
 *
 * @return NULL で失敗 */

FT_BitmapGlyph mFreeTypeGetBitmapGlyph(FT_Library lib,
	FT_Face face,mFreeTypeInfo *info,uint32_t code)
{
	FT_UInt gindex;
	FT_Glyph glyph;
	
	//コードからグリフインデックス取得
	
	gindex = FT_Get_Char_Index(face, code);
	if(gindex == 0) return NULL;
	
	//グリフスロットにロード
	
	if(FT_Load_Glyph(face, gindex, info->fLoadGlyph))
		return NULL;
	
	//グリフのコピー取得
	
	if(FT_Get_Glyph(face->glyph, &glyph))
		return NULL;
	
	//太字変換
	
	if(info->flags & MFTINFO_F_EMBOLDEN)
	{
		if(glyph->format == FT_GLYPH_FORMAT_OUTLINE)
			//アウトライン
			FT_Outline_Embolden(&((FT_OutlineGlyph)glyph)->outline, 1<<5);
		else if(glyph->format == FT_GLYPH_FORMAT_BITMAP)
			//ビットマップ
			FT_Bitmap_Embolden(lib, &((FT_BitmapGlyph)glyph)->bitmap, 1<<6, 0);
	}
	
	//斜体変換
	
	if((info->flags & MFTINFO_F_MATRIX) && glyph->format == FT_GLYPH_FORMAT_OUTLINE)
		FT_Outline_Transform(&((FT_OutlineGlyph)glyph)->outline, &info->matrix);	
	
	//ビットマップに変換
	
	if(glyph->format != FT_GLYPH_FORMAT_BITMAP)
	{
		if(FT_Glyph_To_Bitmap(&glyph, info->nRenderMode, NULL, TRUE))
		{
			FT_Done_Glyph(glyph);
			return NULL;
		}
	}
	
	return (FT_BitmapGlyph)glyph;
}

/** GSUB テーブル取得
 *
 * @return <code>FT_OpenType_Free(face, (FT_Bytes)gsub)</code> で解放すること */

void *mFreeTypeGetGSUB(FT_Face face)
{
	FT_Bytes base,gdef,gpos,gsub = 0,jstf;

	if(FT_OpenType_Validate(face, FT_VALIDATE_GSUB, &base,&gdef,&gpos,&gsub,&jstf) == 0)
		return (void *)gsub;
	else
		return NULL;
}

/** グレイスケール用アルファブレンド */

mRgbCol mFreeTypeBlendColorGray(mRgbCol bgcol,mRgbCol fgcol,int a)
{
	int r,g,b;
	
	r = M_GET_R(bgcol);
	g = M_GET_G(bgcol);
	b = M_GET_B(bgcol);
	
	r = ((M_GET_R(fgcol) - r) * a >> 8) + r;
	g = ((M_GET_G(fgcol) - g) * a >> 8) + g;
	b = ((M_GET_B(fgcol) - b) * a >> 8) + b;

	return M_RGB(r, g, b);
}

/** LCD 用アルファブレンド */

mRgbCol mFreeTypeBlendColorLCD(mRgbCol bgcol,mRgbCol fgcol,int ra,int ga,int ba)
{
	int r,g,b;
	
	r = M_GET_R(bgcol);
	g = M_GET_G(bgcol);
	b = M_GET_B(bgcol);
	
	r = ((M_GET_R(fgcol) - r) * ra >> 8) + r;
	g = ((M_GET_G(fgcol) - g) * ga >> 8) + g;
	b = ((M_GET_B(fgcol) - b) * ba >> 8) + b;

	return M_RGB(r,g,b);
}

/** @} */
