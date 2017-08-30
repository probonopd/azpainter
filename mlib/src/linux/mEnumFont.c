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
 * <Linux> フォント列挙
 *****************************************/

#include <string.h>
#include <fontconfig/fontconfig.h>

#include "mDef.h"

#include "mFontConfig.h"


//----------------

typedef struct
{
	int weight,slant;
}WeightSlantDat;

//----------------


/**
@defgroup enumfont mEnumFont
@brief フォント列挙

@ingroup group_system

@{
@file mEnumFont.h
*/


//=========================


/** FcFontSet 取得 */

static FcFontSet *_get_fontset(const char *family)
{
	FcObjectSet *os;
	FcPattern *pat;
 	FcFontSet *set;

 	if(family)
 	{
		//スタイルリスト
		os = FcObjectSetBuild(FC_STYLE, FC_WEIGHT, FC_SLANT, (void *)0);
		pat = FcPatternBuild(0, FC_FAMILY, FcTypeString, family, (void *)0);
 	}
 	else
 	{
		os = FcObjectSetBuild(FC_FAMILY, (void *)0);
		pat = FcPatternCreate();
 	}

	set = FcFontList(0, pat, os);

	FcPatternDestroy(pat);
	FcObjectSetDestroy(os);

	return set;
}

/** 文字列列挙バッファ確保＆セット */

static char **_get_strlist(FcFontSet *set,const char *object)
{
	char **buf,**pd,*pc;
	int i;

	if(set->nfont == 0) return NULL;

	//確保

	buf = (char **)mMalloc(sizeof(char *) * (set->nfont + 1), TRUE);
	if(!buf) return NULL;

	//セット

	for(i = 0, pd = buf; i < set->nfont; i++)
	{
		pc = mFontConfigGetText(set->fonts[i], object);

		if(pc)
			*(pd++) = mStrdup(pc);
	}

	return buf;
}

/** フォント名を並べ替え＆同名を削除 */

static void _sort_family(char **buf)
{
	char **p1,**p2,**p3,*tmp;
	int ret;

	for(p1 = buf; *p1; p1++)
	{
		for(p2 = p1 + 1; *p2; p2++)
		{
			ret = strcmp(*p2, *p1);

			if(ret == 0)
			{
				//削除

				mFree(*p2);
				
				for(p3 = p2; *p3; p3++)
				{
					*p3 = *(p3 + 1);
					if(*p3 == 0) break;
				}

				p2--;
			}
			else if(ret < 0)
			{
				tmp = *p1;
				*p1 = *p2;
				*p2 = tmp;
			}
		}
	}
}

/** スタイルを太さ順に並べ替え */

static void _sort_style(FcFontSet *set)
{
	int i,j,minpos,f;
	FcPattern *pat;
	WeightSlantDat *buf,min;

	//太さと斜体の値を取得

	buf = (WeightSlantDat *)mMalloc(sizeof(WeightSlantDat) * set->nfont, FALSE);
	if(!buf) return;

	for(i = 0; i < set->nfont; i++)
	{
		buf[i].weight = mFontConfigGetInt(set->fonts[i], FC_WEIGHT, 0);
		buf[i].slant = mFontConfigGetInt(set->fonts[i], FC_SLANT, 0);
	}

	//ソート

	for(i = 0; i < set->nfont - 1; i++)
	{
		//最小値
	
		min = buf[i];
		minpos = i;
		
		for(j = i + 1; j < set->nfont; j++)
		{
			f = 0;
		
			if(buf[j].weight < min.weight)
				f = 1;
			else if(buf[j].weight == min.weight && buf[j].slant < min.slant)
				f = 1;

			if(f) min = buf[j], minpos = j;
		}

		//入れ替え

		if(i != minpos)
		{
			pat = set->fonts[i];
			set->fonts[i] = set->fonts[minpos];
			set->fonts[minpos] = pat;

			min = buf[i];
			buf[i] = buf[minpos];
			buf[minpos] = min;
		}
	}

	mFree(buf);
}


//=========================


/** フォント名の列挙を取得
 *
 * @return char* の配列が確保されて返る。ポインタが NULL で終了。mFreeStrsBuf() で解放すると良い。 */

char **mEnumFontFamily()
{
	FcFontSet *set;
	char **buf;

	set = _get_fontset(NULL);
	if(!set) return NULL;

	buf = _get_strlist(set, FC_FAMILY);

	FcFontSetDestroy(set);

	if(buf) _sort_family(buf);

	return buf;
}

/** 指定フォントのスタイルの列挙を取得 */

char **mEnumFontStyle(const char *family)
{
	FcFontSet *set;
	char **buf;

	set = _get_fontset(family);
	if(!set) return NULL;

	_sort_style(set);

	buf = _get_strlist(set, FC_STYLE);

	FcFontSetDestroy(set);

	return buf;
}

/** @} */
