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
 * mTextParam
 *****************************************/

#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "mDef.h"

#include "mTextParam.h"

#include "mStr.h"
#include "mList.h"
#include "mUtilStr.h"


//----------------------

struct _mTextParam
{
	char *buf;   //文字列バッファ
	mList list;  //キーと値のリスト
};

typedef struct
{
	mListItem i;

	char *key,*param;  //バッファの位置
	uint32_t hash;     //キーのハッシュ値
}mTextParamItem;

//----------------------


/**
@defgroup textparam mTextParam
@brief 文字列から各パラメータを取得

\c "key1=param1;key2=param2;..." などのように指定文字で区切られた形式の文字列から、各値を取得する。\n
キー名は、大文字小文字を区別しない。

@ingroup group_etc
@{

@file mTextParam.h
*/


//==============================
// sub
//==============================


/** ハッシュ値取得 */

static uint32_t _get_hash(const char *pc)
{
	uint32_t h = 0;

	for(; *pc; pc++)
		h = h * 37 + (*pc);

	return h;
}

/** 文字列からリスト作成 */

static void _create_list(mTextParam *p,char split,char splitparam)
{
	char *pc,*end,*keyend;
	mTextParamItem *pi;
	mBool loop = TRUE;

	for(pc = p->buf; loop && *pc; pc = end + 1)
	{
		//終端位置

		end = strchr(pc, split);
		if(!end)
		{
			end = strchr(pc, 0);
			loop = FALSE;
		}

		*end = 0;

		//空の場合は処理しない

		if(!(*pc)) continue;

		//キーと値の区切り文字位置

		keyend = strchr(pc, splitparam);

		if(keyend) *keyend = 0;

		//キーを小文字に

		mToLower(pc);

		//項目追加

		pi = (mTextParamItem *)mListAppendNew(&p->list, sizeof(mTextParamItem), NULL);
		if(!pi) return;

		pi->key = pc;
		pi->param = (keyend)? keyend + 1: end;
		pi->hash = _get_hash(pc);
	}
}

/** キーから検索 */

static char *_find(mTextParam *p,const char *key)
{
	mTextParamItem *pi;
	uint32_t hash;
	char *keycmp,*param = NULL;

	//比較用に、キーを小文字に

	keycmp = mStrdup(key);
	if(!keycmp) return NULL;

	mToLower(keycmp);

	hash = _get_hash(keycmp);

	//キー検索

	for(pi = (mTextParamItem *)p->list.top; pi; pi = (mTextParamItem *)pi->i.next)
	{
		if(hash == pi->hash && strcmp(keycmp, pi->key) == 0)
		{
			param = pi->param;
			break;
		}
	}

	mFree(keycmp);

	return param;
}


//==============================


/** 解放 */

void mTextParamFree(mTextParam *p)
{
	if(p)
	{
		mFree(p->buf);
		mListDeleteAll(&p->list);
		
		mFree(p);
	}
}

/** 文字列から作成
 *
 * @param split ';' など、各値を区切る文字
 * @param splitparam '=' など、キーと値を区切る文字 (-1 で '=') */

mTextParam *mTextParamCreate(const char *text,char split,char splitparam)
{
	mTextParam *p;

	if(splitparam < 0) splitparam = '=';

	p = (mTextParam *)mMalloc(sizeof(mTextParam), TRUE);
	if(!p) return NULL;

	//文字列をコピー

	p->buf = mStrdup(text);
	if(!p->buf)
	{
		mFree(p);
		return NULL;
	}

	//リストを作成

	_create_list(p, split, splitparam);

	return p;
}


/** int 値を取得
 *
 * 16進数 (0x...)、8進数 (0...) にも対応 */

mBool mTextParamGetInt(mTextParam *p,const char *key,int *dst)
{
	char *pc = _find(p, key);

	if(pc)
	{
		*dst = strtol(pc, NULL, 0);
		return TRUE;
	}
	else
		return FALSE;
}

/** int 値を取得 (範囲指定) */

mBool mTextParamGetInt_range(mTextParam *p,const char *key,int *dst,int min,int max)
{
	int n;

	if(!mTextParamGetInt(p, key, &n))
		return FALSE;
	else
	{
		if(n < min) n = min;
		else if(n > max) n = max;

		*dst = n;

		return TRUE;
	}
}

/** double 値を取得 */

mBool mTextParamGetDouble(mTextParam *p,const char *key,double *dst)
{
	char *pc = _find(p, key);

	if(pc)
	{
		*dst = strtod(pc, NULL);
		return TRUE;
	}
	else
		return FALSE;
}

/** double 値を指定桁数の int にして取得 */

mBool mTextParamGetDoubleInt(mTextParam *p,const char *key,int *dst,int dig)
{
	double d;
	int n;

	if(!mTextParamGetDouble(p, key, &d))
		return FALSE;
	else
	{
		for(n = 1; dig > 0; dig--, n *= 10);
	
		*dst = (int)round(d * n);

		return TRUE;
	}
}

/** double 値を指定桁数の int にして取得 (範囲指定) */

mBool mTextParamGetDoubleInt_range(mTextParam *p,const char *key,int *dst,int dig,int min,int max)
{
	int n;

	if(!mTextParamGetDoubleInt(p, key, &n, dig))
		return FALSE;
	else
	{
		if(n < min) n = min;
		else if(n > max) n = max;

		*dst = n;

		return TRUE;
	}
}

/** 文字列をポインタで直接取得
 *
 * ポインタは解放したりしないこと。 */

mBool mTextParamGetText_raw(mTextParam *p,const char *key,char **dst)
{
	char *pc = _find(p, key);

	if(pc)
	{
		*dst = pc;
		return TRUE;
	}
	else
		return FALSE;
}

/** 文字列を複製して取得 */

mBool mTextParamGetText_dup(mTextParam *p,const char *key,char **buf)
{
	char *pc = _find(p, key);

	if(pc)
	{
		*buf = mStrdup(pc);
		return TRUE;
	}
	else
		return FALSE;
}

/** mStr に文字列を取得 */

mBool mTextParamGetStr(mTextParam *p,const char *key,mStr *str)
{
	char *pc = _find(p, key);

	if(pc)
	{
		mStrSetText(str, pc);
		return TRUE;
	}
	else
		return FALSE;
}

/** key の値の文字列を単語リストから探し、そのインデックス番号を取得
 *
 * @param words '\0' で区切った単語リスト。最後は \0\0 で終わること。
 * @return 見つからなかった場合、-1 */

int mTextParamFindText(mTextParam *p,const char *key,const char *words,mBool bNoCase)
{
	char *param = _find(p, key);
	const char *pc;
	int ret,no;

	if(param)
	{
		for(pc = words, no = 0; *pc; pc += strlen(pc) + 1, no++)
		{
			ret = (bNoCase)? strcasecmp(pc, param): strcmp(pc, param);

			if(ret == 0) return no;
		}
	}

	return -1;
}

/** @} */
