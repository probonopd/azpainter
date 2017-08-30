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

/**********************************************
 * [文字列関連ユーティリティ]
 **********************************************/

#include <string.h>
#include <stdlib.h>

#include "mDef.h"
#include "mUtilStr.h"

#ifndef MLIB_NO_UTILCHARCODE_H
#include "mUtilCharCode.h"
#endif


/********************//**

@defgroup util_str mUtilStr
@brief 文字列関連ユーティリティ

@ingroup group_util
@{

@file mUtilStr.h

**************************/


/** char* の配列バッファを解放
 *
 * ポインタが NULL でデータの終了。 */

void mFreeStrsBuf(char **buf)
{
	char **pp;

	if(!buf) return;

	//各文字列を解放

	for(pp = buf; *pp; pp++)
		mFree(*pp);

	//buf を解放

	mFree(buf);
}

/** 文字列内から指定文字の位置を探す
 *
 * @return 見つからなかった場合、終端位置 (NULL 文字の位置) */

char *mStrchr_end(const char *text,char ch)
{
	char *pc = (char *)text;

	for(; *pc && *pc != ch; pc++);

	return pc;
}

/** 文字列比較
 *
 * ch の文字が来た場合は終端とみなす。\n
 * ポインタが NULL の場合は空文字列とみなす。 */

int mStrcmp_endchar(const char *text1,const char *text2,char ch)
{
	const uint8_t *pc1,*pc2;
	uint8_t c1,c2;
	int n;

	pc1 = (const uint8_t *)text1;
	pc2 = (const uint8_t *)text2;

	c1 = (pc1)? *pc1: 0;
	c2 = (pc2)? *pc2: 0;

	while(c1 && c1 == c2 && c1 != ch && c2 != ch)
	{
		c1 = *(pc1++);
		c2 = *(pc2++);
	}

	if(c1 == ch) c1 = 0;
	if(c2 == ch) c2 = 0;

	if(c1 == c2)
		n = 0;
	else
		n = (c1 < c2)? -1: 1;

	return n;
}

/** 文字列比較 (数字部分は数値として比較する)
 *
 * ポインタが NULL の場合は空文字列とみなす。 */

int mStrcmp_number(const char *text1,const char *text2)
{
	const uint8_t *p1,*p2;
	char *pc1,*pc2;
	uint8_t c1,c2;
	int f,v1,v2;

	p1 = (const uint8_t *)text1;
	p2 = (const uint8_t *)text2;

	c1 = (p1)? *p1: 0;
	c2 = (p2)? *p2: 0;

	while(1)
	{
		//数字が見つかるまで通常比較

		f = 0;

		while(c1 && c2)
		{
			if(c1 >= '0' && c1 <= '9'
				&& c2 >= '0' && c2 <= '9')
			{
				f = 1;
				break;
			}

			if(c1 != c2) break;
			
			c1 = *(++p1);
			c2 = *(++p2);
		}

		//数字がなかった場合

		if(f == 0)
		{
			if(c1 == c2)
				return 0;
			else
				return (c1 < c2)? -1: 1;
		}

		//数字が見つかった場合、数値を取り出して比較

		v1 = strtol((const char *)p1, &pc1, 10);
		v2 = strtol((const char *)p2, &pc2, 10);

		if(v1 == v2)
		{
			//数値が同じ場合、数字の後を続けて比較
			
			p1 = (const uint8_t *)pc1;
			p2 = (const uint8_t *)pc2;

			c1 = *p1;
			c2 = *p2;
		}
		else
			return (v1 < v2)? -1: 1;
	}
}

/** ch を区切り文字として、次の文字列を取得
 *
 * @attention 区切り文字が複数連続している部分はスキップされるので、
 *  空文字列を含め順番に取得したい場合には正しく動作しない。
 *
 * 実行後、*ppend の値を *pptop に代入して、繰り返す。
 *
 * @param pptop 先頭位置を入れておく。先頭位置が入る
 * @param ppend 終端位置(ch の位置)が入る
 * @return 文字列があるか */

mBool mGetStrNextSplit(const char **pptop,const char **ppend,char ch)
{
	const char *top;
	char *end;

	if(!(*pptop)) return FALSE;

	//先頭

	for(top = *pptop; *top && *top == ch; top++);

	if(!*top) return FALSE;

	//終端

	end = strchr(top, ch);
	if(!end) end = strchr(top, 0);

	*pptop = top;
	*ppend = end;

	return TRUE;
}

/** 文字列の行数取得
 * 
 * '\\n' で1行。改行で終わっている場合は行数に含めない。 */

int mGetStrLines(const char *text)
{
	const char *pc,*pc2;
	int cnt = 1;
	
	for(pc = text; *pc; )
	{
		pc2 = strchr(pc, '\n');
		if(!pc2) break;
		
		pc = pc2 + 1;
		if(*pc) cnt++;
	}
	
	return cnt;
}

/** 次の行のポインタを取得
 *
 * @param repnull 改行を NULL に置き換えるか
 * @return 次の行のポインタ。NULL で終了 */

char *mGetStrNextLine(char *buf,mBool repnull)
{
	char *pc;

	if(!*buf) return NULL;

	for(pc = buf; *pc && *pc != '\r' && *pc != '\n'; pc++);

	if(*pc == '\r' && pc[1] == '\n')
	{
		if(repnull) *pc = 0, pc[1] = 0;
		pc += 2;
	}
	else if(*pc)
	{
		if(repnull) *pc = 0;
		pc++;
	}

	return pc;
}

/** 区切り文字を NULL に置換えた文字列取得
 *
 * 最後は NULL が２つ連続して終わる。*/

char *mGetSplitCharReplaceStr(const char *text,char split)
{
	char *buf;
	int len;

	len = (text)? strlen(text): 0;

	buf = (char *)mMalloc(len + 2, FALSE);
	if(!buf) return NULL;

	memcpy(buf, text, len);

	buf[len] = 0;
	buf[len + 1] = 0;

	mReplaceStrChar(buf, split, 0);

	return buf;
}

/** バッファの並び順を入れ替える */

void mReverseBuf(char *buf,int len)
{
	int cnt;
	char *p1,*p2;
	char c;

	p1 = buf;
	p2 = buf + len - 1;

	for(cnt = len >> 1; cnt > 0; cnt--, p1++, p2--)
	{
		c = *p1; *p1 = *p2; *p2 = c;
	}
}

/** 文字列内の指定文字を置換え */

void mReplaceStrChar(char *text,char find,char rep)
{
	if(!text) return;

	for(; *text; text++)
	{
		if(*text == find)
			*text = rep;
	}
}

/** 文字列が 0x00-0x7E の文字のみで構成されているか */

mBool mIsASCIIString(const char *text)
{
	for(; *text > 0 && *text < 0x7f; text++);

	return (*text == 0);
}

/** 文字列が半角数字のみで構成されているか */

mBool mIsNumString(const char *text)
{
	for(; *text >= '0' && *text <= '9'; text++);

	return (*text == 0);
}

/** 可変長の長さ+文字列のデータのバイト数を取得 */

int mGetStrVariableLenBytes(const char *text)
{
	int len,bytes;

	if(!text)
		return 1;
	else
	{
		len = strlen(text);

		for(bytes = 1 + len; len > 127; len >>= 7, bytes++);

		return bytes;
	}
}

/** 文字列を小文字に変換 */

void mToLower(char *text)
{
	if(text)
	{
		for(; *text; text++)
		{
			if(*text >= 'A' && *text <= 'Z')
				*text += 0x20;
		}
	}
}

/** 数値を文字列に変換
 * 
 * @return 格納された文字数 */

int mIntToStr(char *buf,int num)
{
	char *pd = buf;
	int n,len = 0;

	n = num;

	if(n < 0)
	{
		n = -n;
		*(pd++) = '-';
		len = 1;
	}

	do
	{
		*(pd++) = '0' + n % 10;
		len++;

		n /= 10;
	}while(n);

	*pd = 0;

	if(num < 0)
		mReverseBuf(buf + 1, len - 1);
	else
		mReverseBuf(buf, len);

	return len;
}

/** 数値を浮動小数点文字列に変換
 * 
 * @param dig 小数点以下の桁数
 * @return 格納された文字数 */

int mFloatIntToStr(char *buf,int num,int dig)
{
	int len = 0,div,i;

	for(i = 0, div = 1; i < dig; i++, div *= 10);

	//符号

	if(num < 0)
	{
		buf[len++] = '-';
		num = -num;
	}

	//整数部分

	len += mIntToStr(buf + len, num / div);

	//小数点部分

	if(dig > 0)
	{
		buf[len++] = '.';
		len += mIntToStrDig(buf + len, num % div, dig);
	}

	return len;
}

/** 桁数を指定して数値を文字列へ
 *
 * @param dig  桁数。正で 0 で埋める。負で空白で埋める。 */

int mIntToStrDig(char *buf,int num,int dig)
{
	char *pd = buf;
	int len = 0,i,div,n;
	uint8_t bspace,nostart=1;

	if(dig >= 0)
		bspace = 0;
	else
		bspace = 1, dig = -dig;

	if(num < 0)
	{
		*(pd++) = '-';
		len = 1;
		num = -num;
	}

	for(i = 0, div = 1; i < dig - 1; i++, div *= 10);

	for(i = 0; i < dig; i++, div /= 10, len++)
	{
		n = (num / div) % 10;
		if(nostart && n) nostart = 0;

		*(pd++) = (bspace && nostart)? ' ': '0' + n;
	}

	*pd = 0;

	return len;
}

/** UCS-4 文字列を数値に変換
 * 
 * 小数点以下は dig の桁数まで取得。
 * dig = 1 なら 1.0 = 10 */

int mUCS4StrToFloatInt(const uint32_t *text,int dig)
{
	const uint32_t *p = text;
	int fcnt = -1,n = 0;
	
	if(*p == '-' || *p == '+') p++;
	
	for(; *p; p++)
	{
		if(*p == '.')
		{
			if(dig <= 0 || fcnt != -1) break;
		
			fcnt = 0;
		}
		else if(*p >= '0' && *p <= '9')
		{
			n *= 10;
			n += *p - '0';
			
			if(fcnt != -1)
			{
				fcnt++;
				if(fcnt >= dig) break;
			}
		}
		else
			break;
	}
	
	//小数点以下の桁数が足りない場合
	
	if(dig > 0 && fcnt < dig)
	{
		if(fcnt == -1) fcnt = 0;
		
		for(; fcnt < dig; fcnt++, n *= 10);
	}

	//符号反転
	
	if(*text == '-') n = -n;
	
	return n;
}

#ifndef MLIB_NO_UTILCHARCODE_H

/** ワイルドカード(*,?) ありの文字列マッチ
 *
 * @param bNoCase TRUE で大文字小文字を区別しない */

mBool mIsMatchString(const char *text,const char *pattern,mBool bNoCase)
{
	uint32_t usrc,upat;

	if(!text || !pattern) return FALSE;

	while(*text && *pattern)
	{
		if(mUTF8ToUCS4Char(text, -1, &usrc, &text) < 0) break;
		if(mUTF8ToUCS4Char(pattern, -1, &upat, &pattern) < 0) break;
	
		if(upat == '*')
		{
			//最後の * なら以降すべてOK

			if(*pattern == 0) return TRUE;

			//

			for(; *text; text += mUTF8CharWidth(text))
			{
				if(mIsMatchString(text, pattern, bNoCase))
					return TRUE;
			}

			return FALSE;
		}
		else if(upat != '?')
		{
			//? は無条件で1文字進める

			//通常文字比較

			if(bNoCase)
			{
				if(usrc >= 'A' && usrc <= 'Z') usrc += 0x20;
				if(upat >= 'A' && upat <= 'Z') upat += 0x20;
			}

			if(usrc != upat) return FALSE;
		}
	}

	//双方最後まで来たらマッチした
	return (*text == 0 && *pattern == 0);
}

/** 指定文字で区切られた複数パターンにマッチするか */

mBool mIsMatchStringSum(const char *text,const char *pattern,char split,mBool bNoCase)
{
	char *pat,*pc;
	mBool ret = FALSE;

	//指定文字を NULL に置換え

	pat = mGetSplitCharReplaceStr(pattern, split);
	if(!pat) return FALSE;

	//比較

	for(pc = pat; *pc; pc += strlen(pc) + 1)
	{
		if(mIsMatchString(text, pc, bNoCase))
		{
			ret = TRUE;
			break;
		}
	}

	mFree(pat);

	return ret;
}

#endif

/** 文字列列挙の中から等しい文字列のインデックス番号を取得
 *
 * @param enumtext split で区切った複数文字列
 * @param split 区切り文字
 * @return -1 で見つからなかった */

int mGetEqStringIndex(const char *text,const char *enumtext,char split,mBool bNoCase)
{
	const char *pc,*pend;
	int ret,no,len;

	len = strlen(text);

	for(pc = enumtext, no = 0; *pc; no++)
	{
		//終端

		pend = strchr(pc, split);
		if(!pend) pend = strchr(pc, 0);

		//文字数が同じ場合比較

		if(pend - pc == len)
		{
			if(bNoCase)
				ret = strncasecmp(text, pc, len);
			else
				ret = strncmp(text, pc, len);

			if(ret == 0) return no;
		}

		//次へ

		pc = (*pend)? pend + 1: pend;
	}

	return -1;
}

/** "key=param;..." の形式の文字列から指定パラメータ値の文字列取得
 *
 * @param split 各値を区切っている文字。-1 で ';'。
 * @param paramsplit キー名と値を区切っている文字。-1 で '='。
 * @return 確保された文字列。NULL でなし */

char *mGetFormatStrParam(const char *text,const char *key,
	char split,char paramsplit,mBool bNoCase)
{
	const char *pc,*pend,*pkeyend;
	int ret;

	if(!text || !key) return NULL;

	if(split == -1) split = ';';
	if(paramsplit == -1) paramsplit = '=';

	for(pc = text; *pc; )
	{
		//終了位置
		
		pend = strchr(pc, split);
		if(!pend) pend = strchr(pc, 0);

		//キー名終了位置

		for(pkeyend = pc; pkeyend != pend && *pkeyend != paramsplit; pkeyend++);

		if(pkeyend != pend)
		{
			//キー名比較

			if(bNoCase)
				ret = strncasecmp(pc, key, pkeyend - pc);
			else
				ret = strncmp(pc, key, pkeyend - pc);

			//等しければ確保して返す

			if(ret == 0)
			{
				pkeyend++;
				
				return mStrndup(pkeyend, pend - pkeyend);
			}
		}

		//次へ

		pc = (*pend)? pend + 1: pend;
	}

	return NULL;
}

/** @} */
