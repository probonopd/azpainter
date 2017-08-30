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
 * fontconfig
 *****************************************/

#include <fontconfig/fontconfig.h>

#include "mDef.h"

#include "mFontConfig.h"
#include "mFont.h"
#include "mStr.h"


/**
@defgroup fontconfig mFontConfig
@brief fontconfig ユーティリティ

@details
文字列は UTF-8 で指定する。

@ingroup group_lib
 
@{
@file mFontConfig.h

@struct mFontInfo

*/


//================================
// sub
//================================


/** mFontInfo から FcPattern 作成 */

static FcPattern *_create_fc_pattern(mFontInfo *p)
{
	FcPattern *pat;
	int weight,slant;
	
	pat = FcPatternCreate();

	//フォントファミリー

	if((p->mask & MFONTINFO_MASK_FAMILY) && !mStrIsEmpty(&p->strFamily))
		FcPatternAddString(pat, FC_FAMILY, (FcChar8 *)p->strFamily.buf);
	else
		FcPatternAddString(pat, FC_FAMILY, (FcChar8 *)"sans-serif");

	//スタイル

	if((p->mask & MFONTINFO_MASK_STYLE) && !mStrIsEmpty(&p->strStyle))
		FcPatternAddString(pat, FC_STYLE, (FcChar8 *)p->strStyle.buf);
	
	//サイズ

	if(p->mask & MFONTINFO_MASK_SIZE)
	{
		if(p->size < 0)
			FcPatternAddDouble(pat, FC_PIXEL_SIZE, -(p->size));
		else
			FcPatternAddDouble(pat, FC_SIZE, p->size);
	}

	//太さ (指定なしなら通常)

	weight = FC_WEIGHT_NORMAL;
	
	if(p->mask & MFONTINFO_MASK_WEIGHT)
	{
		switch(p->weight)
		{
			case MFONTINFO_WEIGHT_BOLD:
				weight = FC_WEIGHT_BOLD;
				break;
		}
	}

	FcPatternAddInteger(pat, FC_WEIGHT, weight);
	
	//斜体 (指定なしなら通常)

	slant = FC_SLANT_ROMAN;
	
	if(p->mask & MFONTINFO_MASK_SLANT)
	{
		switch(p->slant)
		{
			case MFONTINFO_SLANT_ITALIC:
				slant = FC_SLANT_ITALIC;
				break;
			case MFONTINFO_SLANT_OBLIQUE:
				slant = FC_SLANT_OBLIQUE;
				break;
		}
	}
		
	FcPatternAddInteger(pat, FC_SLANT, slant);
	
	//デフォルトの構成を適用

	if(!FcConfigSubstitute(NULL, pat, FcMatchPattern))
	{
		FcPatternDestroy(pat);
		return NULL;
	}
	
	return pat;
}


//================================
//
//================================


/** fontconfig 初期化 */

mBool mFontConfigInit()
{
	return FcInit();
}

/** fontconfig 解放 */

void mFontConfigEnd()
{
	FcFini();
}

/** パターンを解放 */

void mFontConfigPatternFree(mFcPattern *pat)
{
	if(pat)
		FcPatternDestroy(pat);
}

/** 指定した構成にマッチするパターン取得 */

mFcPattern *mFontConfigMatch(mFontInfo *conf)
{
	FcPattern *pat,*pat_match;
	FcResult ret;
	
	pat = _create_fc_pattern(conf);
	if(!pat) return NULL;
	
	pat_match = FcFontMatch(0, pat, &ret);
	
	FcPatternDestroy(pat);
		
	return pat_match;
}

/** パターンからファイル情報取得
 * 
 * @param file pat の領域内のポインタが返る。pat を解放するまでは有効 */

int mFontConfigGetFileInfo(mFcPattern *pat,char **file,int *index)
{
	*file  = NULL;
	*index = 0;

	if(FcPatternGetString(pat, FC_FILE, 0, (FcChar8 **)file) != FcResultMatch)
		return -1;
	
	FcPatternGetInteger(pat, FC_INDEX, 0, index);
	
	return 0;
}

/** パターンから文字列取得
 *
 * @return ポインタは解放しないこと。NULL で失敗。 */

char *mFontConfigGetText(mFcPattern *pat,const char *object)
{
	FcChar8 *text;

	if(FcPatternGetString(pat, object, 0, &text) == FcResultMatch)
		return (char *)text;
	else
		return NULL;
}

/** パターンから bool 値取得 */

mBool mFontConfigGetBool(mFcPattern *pat,const char *object,mBool def)
{
	FcBool ret;

	if(FcPatternGetBool(pat, object, 0, &ret) == FcResultMatch)
		return ret;
	else
		return def;
}

/** パターンから int 値取得 */

int mFontConfigGetInt(mFcPattern *pat,const char *object,int def)
{
	int ret;

	if(FcPatternGetInteger(pat, object, 0, &ret) == FcResultMatch)
		return ret;
	else
		return def;
}

/** パターンから double 値取得 */

double mFontConfigGetDouble(mFcPattern *pat,const char *object,double def)
{
	double ret;

	if(FcPatternGetDouble(pat, object, 0, &ret) == FcResultMatch)
		return ret;
	else
		return def;
}

/** パターンから double 値取得 */

mBool mFontConfigGetDouble2(mFcPattern *pat,const char *object,double *dst)
{
	return (FcPatternGetDouble(pat, object, 0, dst) == FcResultMatch);
}

/** パターンから matrix 取得 */

mBool mFontConfigGetMatrix(mFcPattern *pat,const char *object,double *matrix)
{
	FcMatrix *mt;
	
	if(FcPatternGetMatrix(pat, object, 0, &mt) != FcResultMatch)
		return FALSE;
	else
	{
		matrix[0] = mt->xx;
		matrix[1] = mt->xy;
		matrix[2] = mt->yx;
		matrix[3] = mt->yy;
		return TRUE;
	}
}

/** @} */
