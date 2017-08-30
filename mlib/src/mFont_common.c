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
 * mFont - 共通
 *****************************************/

#include <string.h>

#include "mDef.h"

#include "mFont.h"
#include "mTextParam.h"
#include "mStr.h"



/**
@addtogroup font

@{
*/


/** フォント情報の文字列を解放 */

void mFontInfoFree(mFontInfo *p)
{
	mStrFree(&p->strFamily);
	mStrFree(&p->strStyle);
}

/** 文字列フォーマットからフォント作成 */

mFont *mFontCreateFromFormat(const char *format)
{
	mFontInfo info;
	mFont *font;

	mMemzero(&info, sizeof(mFontInfo));

	mFontFormatToInfo(&info, format);

	font = mFontCreate(&info);

	mFontInfoFree(&info);

	return font;
}

/** 指定フォーマット文字列のサイズのみ変更したものから作成 */

mFont *mFontCreateFromFormat_size(const char *format,double size)
{
	mFontInfo info;
	mFont *font;

	mMemzero(&info, sizeof(mFontInfo));

	mFontFormatToInfo(&info, format);

	info.mask |= MFONTINFO_MASK_SIZE;
	info.size = size;

	font = mFontCreate(&info);

	mFontInfoFree(&info);

	return font;
}

/** フォント情報を記述した文字列から mFontInfo 取得
 *
 * @attention p は初期化しておくこと。 @n
 * 使用後は mFontInfoFree() で解放すること。
 *
 * "type=value;..." @n
 * @n
 * family=[text] : ファミリ名 @n
 * style=[text]  : スタイル @n
 * size=[double] : フォントサイズ(pt) @n
 * weight=[text] : 太さ [normal,bold] @n
 * slant=[text]  : 傾き [roman,italic,oblique] @n
 * render=[text] : 描画タイプ [default,mono,gray,rgb,bgr,vrgb,vbgr]
 */

void mFontFormatToInfo(mFontInfo *p,const char *format)
{
	mTextParam *tp;
	int no;

	p->mask = 0;

	if(!format) return;

	//

	tp = mTextParamCreate(format, ';', -1);
	if(!tp) return;

	//ファミリ

	if(mTextParamGetStr(tp, "family", &p->strFamily))
		p->mask |= MFONTINFO_MASK_FAMILY;

	//スタイル

	if(mTextParamGetStr(tp, "style", &p->strStyle))
		p->mask |= MFONTINFO_MASK_STYLE;

	//サイズ

	if(mTextParamGetDouble(tp, "size", &p->size))
		p->mask |= MFONTINFO_MASK_SIZE;

	//太さ

	no = mTextParamFindText(tp, "weight", "normal\0bold\0", TRUE);

	if(no != -1)
	{
		p->mask |= MFONTINFO_MASK_WEIGHT;

		if(no == 0)
			p->weight = MFONTINFO_WEIGHT_NORMAL;
		else
			p->weight = MFONTINFO_WEIGHT_BOLD;
	}

	//傾き

	no = mTextParamFindText(tp, "slant", "roman\0italic\0oblique\0", TRUE);

	if(no != -1)
	{
		p->mask |= MFONTINFO_MASK_SLANT;
		p->slant = no;
	}

	//レンダリング

	no = mTextParamFindText(tp, "render",
			"default\0mono\0gray\0rgb\0bgr\0vrgb\0vbgr\0", TRUE);

	if(no != -1)
	{
		p->mask |= MFONTINFO_MASK_RENDER;
		p->render = no;
	}

	mTextParamFree(tp);
}

/** mFontInfo の情報を文字列フォーマットに変換 */

void mFontInfoToFormat(mStr *str,mFontInfo *info)
{
	const char *name,
		*slantname[] = {"roman","italic","oblique"},
		*rendername[] = {"default","mono","gray","rgb","bgr","vrgb","vbgr"};

	mStrEmpty(str);

	//ファミリ

	if((info->mask & MFONTINFO_MASK_FAMILY) && !mStrIsEmpty(&info->strFamily))
		mStrAppendFormat(str, "family=%t;", &info->strFamily);

	//スタイル

	if((info->mask & MFONTINFO_MASK_STYLE) && !mStrIsEmpty(&info->strStyle))
		mStrAppendFormat(str, "style=%t;", &info->strStyle);

	//サイズ

	if(info->mask & MFONTINFO_MASK_SIZE)
	{
		mStrAppendText(str, "size=");
		mStrAppendDouble(str, info->size, 1);
		mStrAppendChar(str, ';');
	}

	//太さ

	if(info->mask & MFONTINFO_MASK_WEIGHT)
	{
		switch(info->weight)
		{
			case MFONTINFO_WEIGHT_NORMAL:
				name = "normal";
				break;
			case MFONTINFO_WEIGHT_BOLD:
				name = "bold";
				break;
			default:
				name = NULL;
				break;
		}

		if(name)
			mStrAppendFormat(str, "weight=%s;", name);
	}

	//傾き

	if((info->mask & MFONTINFO_MASK_SLANT)
		&& info->slant >= MFONTINFO_SLANT_ROMAN && info->slant <= MFONTINFO_SLANT_OBLIQUE)
		mStrAppendFormat(str, "slant=%s;", slantname[info->slant]);

	//レンダリング

	if((info->mask & MFONTINFO_MASK_RENDER)
		&& info->render >= MFONTINFO_RENDER_DEFAULT && info->render <= MFONTINFO_RENDER_LCD_VBGR)
		mStrAppendFormat(str, "render=%s;", rendername[info->render]);
}

/** @} */
