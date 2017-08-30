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
 * mFont (freetype)
 *****************************************/

#include <string.h>

#include <ft2build.h>
#include FT_CONFIG_CONFIG_H
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_BITMAP_H
#include FT_LCD_FILTER_H

#include "mDef.h"

#include "mFont.h"
#include "mFontConfig.h"
#include "mFreeType.h"
#include "mPixbuf.h"
#include "mGui.h"
#include "mUtilCharCode.h"


//------------------------

typedef struct
{
	mFont b;

	FT_Face face;
	int baseline,
		underline;

	mFreeTypeInfo info;
}__mFontFt;

#define _FONT(p)  ((__mFontFt *)(p))

//------------------------


/**
@defgroup font mFont
@brief フォント

@ingroup group_system
@{

@file mFont.h
@struct _mFont
@struct mFontInfo
@enum MFONTINFO_MASK
@enum MFONTINFO_WEIGHT
@enum MFONTINFO_SLANT
@enum MFONTINFO_RENDER
*/


//================================


/** 高さなどセット */

static void _set_font_info(__mFontFt *font)
{
	mFreeTypeMetricsInfo info;

	mFreeTypeGetMetricsInfo(mGetFreeTypeLib(), font->face, &font->info, &info);

	font->b.height = info.height;
	font->b.lineheight = info.lineheight;
	font->baseline = info.baseline;
	font->underline = info.underline;
}

/*
//セル高さで指定

static void _ft_set_cell_height(FT_Face face,double size)
{
	FT_Size_RequestRec req;
	
	req.type = FT_SIZE_REQUEST_TYPE_CELL;
	req.width = 0;
	req.height = (int)(size * 64 + 0.5);
	req.horiResolution = 0;
	req.vertResolution = 0;
	
	FT_Request_Size(face, &req);
}
*/


//================================


/** フォント解放 */

void mFontFree(mFont *font)
{
	if(font)
	{
		FT_Done_Face(_FONT(font)->face);
		
		mFree(font);
	}
}

/** フォント作成 */

mFont *mFontCreate(mFontInfo *info)
{
	mFcPattern *pat;
	FT_Face face = NULL;
	char *file;
	int index;
	__mFontFt *font;
	double size;

	//マッチするパターン
	
	pat = mFontConfigMatch(info);
	if(!pat) return NULL;
	
	//フォントファイル読み込み
	/* file は UTF-8 だが、フォントファイル名が ASCII 文字以外の場合は
	 * ほとんどないと思われるのでそのまま使う */
	
	if(mFontConfigGetFileInfo(pat, &file, &index))
		goto ERR;
		
	if(FT_New_Face(mGetFreeTypeLib(), file, index, &face))
		goto ERR;
	
	//mFont 確保
	
	font = (__mFontFt *)mMalloc(sizeof(__mFontFt), TRUE);
	if(!font) goto ERR;
	
	font->face = face;

	//FcPattern から情報取得

	mFreeTypeGetInfoByFontConfig(&font->info, pat, info);

	mFontConfigPatternFree(pat);

	//文字高さセット (FreeType)

	size = font->info.size;
	
	if(size < 0)
		FT_Set_Pixel_Sizes(face, 0, -size);
	else
		FT_Set_Char_Size(face, 0, (int)(size * 64 + 0.5), font->info.dpi, font->info.dpi);

	//ほか情報セット

	_set_font_info(font);

	return (mFont *)font;

ERR:
	if(face) FT_Done_Face(face);
	mFontConfigPatternFree(pat);
	
	return NULL;
}

/** フォントの実体を返す
 *
 * FreeType なら FT_Face */

void *mFontGetHandle(mFont *font)
{
	return (void *)_FONT(font)->face;
}


//===================================
// 文字描画
//===================================


/** 1文字描画 */

static void _draw_char(__mFontFt *font,mPixbuf *img,
	FT_BitmapGlyph glyph,int x,int y,mRgbCol col)
{
	FT_Bitmap *bm;
	uint8_t *pbuf,*pb,r,g,b,fBGR;
	int ix,iy,w,h,pitch,pitch2,f;
	uint32_t dcol;
	
	bm = &glyph->bitmap;
	
	w = bm->width;
	h = bm->rows;
	pbuf  = bm->buffer;
	pitch = bm->pitch;
	
	if(pitch < 0) pbuf += -pitch * (h - 1);
	
	fBGR = ((font->info.flags & MFTINFO_F_SUBPIXEL_BGR) != 0);
	
	//
	
	if(bm->pixel_mode == FT_PIXEL_MODE_MONO)
	{
		//1bit モノクロ

		dcol = mRGBtoPix(col);
		
		for(iy = 0; iy < h; iy++, pbuf += pitch)
		{
			for(ix = 0, f = 0x80, pb = pbuf; ix < w; ix++)
			{
				if(*pb & f)
					mPixbufSetPixel(img, x + ix, y + iy, dcol);
				
				f >>= 1;
				if(!f) { f = 0x80; pb++; }
			}
		}
	}
	else if(bm->pixel_mode == FT_PIXEL_MODE_GRAY)
	{
		//8bit グレイスケール
		
		pitch -= w;

		for(iy = 0; iy < h; iy++, pbuf += pitch)
		{
			for(ix = 0; ix < w; ix++, pbuf++)
			{
				if(*pbuf)
				{
					dcol = mFreeTypeBlendColorGray(
						mPixbufGetPixelRGB(img, x + ix, y + iy), col, *pbuf);

					mPixbufSetPixel(img, x + ix, y + iy, mRGBtoPix(dcol));
				}
			}
		}
	}
	else if(bm->pixel_mode == FT_PIXEL_MODE_LCD)
	{
		//LCD
		
		pitch -= w;
		w /= 3;
		
		for(iy = 0; iy < h; iy++, pbuf += pitch)
		{
			for(ix = 0; ix < w; ix++, pbuf += 3)
			{
				if(fBGR)
					r = pbuf[2], g = pbuf[1], b = pbuf[0];
				else
					r = pbuf[0], g = pbuf[1], b = pbuf[2];
				
				if(r || g || b)
				{
					dcol = mFreeTypeBlendColorLCD(
						mPixbufGetPixelRGB(img, x + ix, y + iy), col, r, g, b);

					mPixbufSetPixel(img, x + ix, y + iy, mRGBtoPix(dcol));
				}
			}
		}
	}
	else if(bm->pixel_mode == FT_PIXEL_MODE_LCD_V)
	{
		//LCD Vertical
		
		pitch2 = pitch * 3 - w;
		h /= 3;
		
		for(iy = 0; iy < h; iy++, pbuf += pitch2)
		{
			for(ix = 0; ix < w; ix++, pbuf++)
			{
				if(fBGR)
					r = pbuf[pitch << 1], g = pbuf[pitch], b = pbuf[0];
				else
					r = pbuf[0], g = pbuf[pitch], b = pbuf[pitch << 1];
				
				if(r || g || b)
				{
					dcol = mFreeTypeBlendColorLCD(
						mPixbufGetPixelRGB(img, x + ix, y + iy), col, r, g, b);

					mPixbufSetPixel(img, x + ix, y + iy, mRGBtoPix(dcol));
				}
			}
		}
	}
}


//===================================
// テキスト描画
//===================================


/** mPixbuf にテキスト描画
 * 
 * @param text UTF-8
 * @param len  text の最大長さ (負の値で NULL 文字まで)
 * @param col  RGB値 */

void mFontDrawText(mFont *font,mPixbuf *img,int x,int y,
	const char *text,int len,mRgbCol col)
{
	__mFontFt *pfont;
	const char *pc;
	int ret;
	uint32_t code;
	FT_Library ftlib;
	FT_BitmapGlyph glyph;
	
	if(!text) return;

	ftlib = mGetFreeTypeLib();
	pfont = _FONT(font);
	
	//LCD Filter
	
#ifdef FT_CONFIG_OPTION_SUBPIXEL_RENDERING
	FT_Library_SetLcdFilter(ftlib, pfont->info.nLCDFilter);
#endif

	//
	
	y += pfont->baseline;
	
	for(pc = text; *pc; )
	{
		if(len >= 0 && pc - text >= len) break;
	
		ret = mUTF8ToUCS4Char(pc, 4, &code, &pc);
		if(ret < 0) break;
		if(ret > 0) continue;
		
		glyph = mFreeTypeGetBitmapGlyph(ftlib, pfont->face, &pfont->info, code);

		if(glyph)
		{
			_draw_char(pfont, img, glyph, x + glyph->left, y - glyph->top, col);
		
			x += glyph->root.advance.x >> 16;
		
			FT_Done_Glyph((FT_Glyph)glyph);
		}
		else
		{
			mPixbufBox(img, x, y - pfont->baseline, 8, pfont->baseline, mRGBtoPix(col));

			x += 8;
		}
	}
}

/** mPixbuf にテキスト描画 (&でホットキー処理)
 * 
 * @param text UTF-8
 * @param len  text の最大長さ (負の値で NULL 文字まで)
 * @param col  RGB値 */

void mFontDrawTextHotkey(mFont *font,mPixbuf *img,int x,int y,
	const char *text,int len,mRgbCol col)
{
	__mFontFt *pfont;
	const char *pc;
	int w,ret;
	uint32_t code;
	mBool bNextHotkey = FALSE;
	FT_Library ftlib;
	FT_BitmapGlyph glyph;
	
	if(!text) return;

	ftlib = mGetFreeTypeLib();
	pfont = _FONT(font);
	
	//LCD Filter
	
#ifdef FT_CONFIG_OPTION_SUBPIXEL_RENDERING
	FT_Library_SetLcdFilter(ftlib, pfont->info.nLCDFilter);
#endif

	//
	
	y += pfont->baseline;
	
	for(pc = text; *pc; )
	{
		if(len >= 0 && pc - text >= len) break;

		if(*pc == '&')
		{
			pc++;
		
			if(bNextHotkey)
			{
				//&&
				code = '&';
				bNextHotkey = FALSE;
			}
			else
			{
				bNextHotkey = TRUE;
				continue;
			}
		}
		else
		{
			ret = mUTF8ToUCS4Char(pc, 4, &code, &pc);
			if(ret < 0) break;
			if(ret > 0) continue;
		}

		//グリフ
		
		glyph = mFreeTypeGetBitmapGlyph(ftlib, pfont->face, &pfont->info, code);

		if(glyph)
		{
			_draw_char(pfont, img, glyph, x + glyph->left, y - glyph->top, col);

			w = glyph->root.advance.x >> 16;
				
			FT_Done_Glyph((FT_Glyph)glyph);
		}
		else
		{
			mPixbufBox(img, x, y - pfont->baseline, 8, pfont->baseline, mRGBtoPix(col));

			w = 8;
		}

		//下線

		if(bNextHotkey)
		{
			mPixbufLineH(img, x, y + pfont->underline, w, mRGBtoPix(col));

			bNextHotkey = FALSE;
		}

		x += w;
	}
}

/** mPixbuf にテキスト描画
 * 
 * @param text UCS-4
 * @param len  text の最大長さ (負の値で NULL 文字まで) */

void mFontDrawTextUCS4(mFont *font,mPixbuf *img,int x,int y,
	const uint32_t *text,int len,mRgbCol col)
{
	__mFontFt *pfont;
	FT_Library ftlib;
	FT_BitmapGlyph glyph;
	int i;
	
	if(!text) return;
	
	ftlib = mGetFreeTypeLib();
	pfont = _FONT(font);
	
	//LCD Filter
	
#ifdef FT_CONFIG_OPTION_SUBPIXEL_RENDERING
	FT_Library_SetLcdFilter(ftlib, pfont->info.nLCDFilter);
#endif

	//
		
	y += pfont->baseline;
	
	for(i = 0; *text; text++, i++)
	{
		if(len >= 0 && i >= len) break;

		glyph = mFreeTypeGetBitmapGlyph(ftlib, pfont->face, &pfont->info, *text);

		if(glyph)
		{
			_draw_char(pfont, img, glyph, x + glyph->left, y - glyph->top, col);
			
			x += glyph->root.advance.x >> 16;
			
			FT_Done_Glyph((FT_Glyph)glyph);
		}
		else
		{
			mPixbufBox(img, x, y - pfont->baseline, 8, pfont->baseline, mRGBtoPix(col));

			x += 8;
		}
	}
}


//============================
// 幅取得
//============================


/** テキストの幅取得 (UTF-8) */

int mFontGetTextWidth(mFont *font,const char *text,int len)
{
	__mFontFt *pfont;
	const char *pc;
	int ret,w = 0;
	uint32_t code;
	FT_Library ftlib;
	FT_BitmapGlyph glyph;
	
	if(!text) return 0;
	
	ftlib = mGetFreeTypeLib();
	pfont = _FONT(font);
		
	for(pc = text; *pc; )
	{
		if(len >= 0 && pc - text >= len) break;
	
		ret = mUTF8ToUCS4Char(pc, 4, &code, &pc);
		if(ret < 0) break;
		if(ret > 0) continue;
		
		glyph = mFreeTypeGetBitmapGlyph(ftlib, pfont->face, &pfont->info, code);

		if(glyph)
		{
			w += glyph->root.advance.x >> 16;
		
			FT_Done_Glyph((FT_Glyph)glyph);
		}
		else
			w += 8;
	}
	
	return w;
}

/** テキストの幅取得
 *
 * ホットキー '&' の処理を行う。 */

int mFontGetTextWidthHotkey(mFont *font,const char *text,int len)
{
	__mFontFt *pfont;
	const char *pc;
	int ret,w = 0;
	uint32_t code;
	mBool bNextHotkey = FALSE;
	FT_Library ftlib;
	FT_BitmapGlyph glyph;
	
	if(!text) return 0;
	
	ftlib = mGetFreeTypeLib();
	pfont = _FONT(font);
		
	for(pc = text; *pc; )
	{
		if(len >= 0 && pc - text >= len) break;

		if(*pc == '&')
		{
			pc++;
		
			if(bNextHotkey)
			{
				//&&
				code = '&';
				bNextHotkey = FALSE;
			}
			else
			{
				bNextHotkey = TRUE;
				continue;
			}
		}
		else
		{
			ret = mUTF8ToUCS4Char(pc, 4, &code, &pc);
			if(ret < 0) break;
			if(ret > 0) continue;
		}

		//
		
		glyph = mFreeTypeGetBitmapGlyph(ftlib, pfont->face, &pfont->info, code);

		if(glyph)
		{
			w += glyph->root.advance.x >> 16;
		
			FT_Done_Glyph((FT_Glyph)glyph);
		}
		else
			w += 8;
	}
	
	return w;
}

/** テキストの幅取得 (UCS-4) */

int mFontGetTextWidthUCS4(mFont *font,const uint32_t *text,int len)
{
	__mFontFt *pfont;
	int i,w = 0;
	FT_Library ftlib;
	FT_BitmapGlyph glyph;
	
	if(!text) return 0;
	
	ftlib = mGetFreeTypeLib();
	pfont = _FONT(font);
	
	for(i = 0; *text; text++, i++)
	{
		if(len >= 0 && i >= len) break;
	
		glyph = mFreeTypeGetBitmapGlyph(ftlib, pfont->face, &pfont->info, *text);

		if(glyph)
		{
			w += glyph->root.advance.x >> 16;
		
			FT_Done_Glyph((FT_Glyph)glyph);
		}
		else
			w += 8;
	}
	
	return w;
}

/** UCS-4 の１文字の幅取得 */

int mFontGetCharWidthUCS4(mFont *font,uint32_t ucs)
{
	FT_BitmapGlyph glyph;
	int w;
	
	glyph = mFreeTypeGetBitmapGlyph(mGetFreeTypeLib(), _FONT(font)->face, &_FONT(font)->info, ucs);

	if(glyph)
	{
		w = glyph->root.advance.x >> 16;
	
		FT_Done_Glyph((FT_Glyph)glyph);
	}
	else
		w = 8;
	
	return w;
}


/** @} */
