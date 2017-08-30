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
 * [文字コード関連ユーティリティ]
 **********************************************/

#include <string.h>
#include <stdlib.h>
#include <wchar.h>

#include "mDef.h"
#include "mUtilCharCode.h"


//-------------------------

static const uint8_t g_utf8_charwidth[128] =
{
 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
 2,2,2,2, 2,2,2,2, 2,2,2,2, 2,2,2,2,
 3,3,3,3, 3,3,3,3, 4,4,4,4, 5,5,6,1
};

//-------------------------


/**
@defgroup util_charcode mUtilCharCode
@brief 文字コード関連ユーティリティ

@ingroup group_util
 
@{
@file mUtilCharCode.h
*/


//=============================
// UTF-8
//=============================


/** UTF-8 の１文字のバイト数取得 */

int mUTF8CharWidth(const char *p)
{
	return g_utf8_charwidth[(*(uint8_t *)p) >> 1];
}

/** UTF-8 から UCS-4 へ１文字変換
 * 
 * @param maxlen src の１文字の最大長さ (負の値で自動)
 * @param ppnext src の次の文字の位置が格納される (NULL 可)
 * @retval 0 成功
 * @retval 1 範囲外の文字など、処理は行わずスキップする文字
 * @retval -1 エラー */

int mUTF8ToUCS4Char(const char *src,int maxlen,uint32_t *dst,const char **ppnext)
{
	const uint8_t *ps = (const uint8_t *)src;
	uint32_t c;
	int len,ret = 0;
	
	if(maxlen < 0) maxlen = 4;
	
	c = *ps;

	if(c <= 0x7f)
	{
		if(maxlen < 1) return -1;
		len = 1;
	}
	else if((c & 0xe0) == 0xc0)
	{
		//2byte : U+0080 - U+07FF

		if(maxlen < 2) return -1;
		if((ps[1] & 0xc0) != 0x80) return -1;
		
		c = ((c & 0x1f) << 6) | (ps[1] & 0x3f);
		
		if(c < 0x80) ret = 1;
	
		len = 2;
	}
	else if((c & 0xf0) == 0xe0)
	{
		//3byte : U+0800 - U+FFFF (サロゲートエリア U+D800-U+DFFF は除く)
		
		if(maxlen < 3) return -1;
		if((ps[1] & 0xc0) != 0x80 || (ps[2] & 0xc0) != 0x80)
			return -1;
		
		c = ((c & 0x0f) << 12) | ((ps[1] & 0x3f) << 6) | (ps[2] & 0x3f);
		
		if(c < 0x800 || (c >= 0xd800 && c <= 0xdfff)) ret = 1;
		if(c == 0xfeff) ret = 1; //BOM
		
		len = 3;
	}
	else if((c & 0xf8) == 0xf0)
	{
		//4byte : U+10000 - U+1FFFFF
		
		if(maxlen < 4) return -1;
		if((ps[1] & 0xc0) != 0x80 || (ps[2] & 0xc0) != 0x80 || (ps[3] & 0xc0) != 0x80)
			return -1;
		
		c = ((c & 0x07) << 18) | ((ps[1] & 0x3f) << 12) | ((ps[2] & 0x3f) << 6) | (ps[3] & 0x3f);
		
		if(c < 0x10000 || c >= 0x10ffff) ret = 1;
		
		len = 4;
	}
	else
		return -1;
	
	//
	
	*dst = c;
	
	if(ppnext) *ppnext = src + len;
	
	return ret;
}

/** UTF-8 文字列を UCS-4 文字列に変換
 * 
 * NULL 文字が追加できる場合は、最後に NULL 文字を追加する。
 * 
 * @param srclen 負の値で NULL 文字まで
 * @param dst    NULL で変換後の文字数のみ計算
 * @param dstlen NULL 文字も含む長さ。dst が NULL の場合は無視。
 * @return 変換した文字数 (NULL 文字は含まない)。-1 でエラー */

int mUTF8ToUCS4(const char *src,int srclen,uint32_t *dst,int dstlen)
{
	const char *ps,*psend;
	uint32_t uc;
	int reslen = 0,ret;

	if(srclen < 0) srclen = strlen(src);
	
	ps = src;
	psend = src + srclen;
		
	while(*ps && ps - src < srclen)
	{
		if(dst && reslen >= dstlen) break;
	
		ret = mUTF8ToUCS4Char(ps, psend - ps, &uc, &ps);
		
		if(ret < 0) return -1;
		if(ret > 0) continue;
		
		if(dst) *(dst++) = uc;
		
		reslen++;
	}
	
	if(dst && reslen < dstlen) *dst = 0;
	
	return reslen;
}

/** UTF-8 文字列を UCS-4 文字列に変換 (メモリ確保)
 * 
 * @param retlen NULL でなければ、結果の文字数が返る
 * @return NULL でエラー */

uint32_t *mUTF8ToUCS4_alloc(const char *src,int srclen,int *retlen)
{
	int len;
	uint32_t *buf;
	
	len = mUTF8ToUCS4(src, srclen, NULL, 0);
	if(len < 0) return NULL;
	
	buf = (uint32_t *)mMalloc((len + 1) << 2, FALSE);
	if(!buf) return NULL;
	
	mUTF8ToUCS4(src, srclen, buf, len + 1);
	
	if(retlen) *retlen = len;
	
	return buf;
}

/** UTF-8 文字列をワイド文字列に変換
 * 
 * NULL 文字が追加できる場合は、最後に NULL 文字を追加する。
 * 
 * @param srclen 負の値で NULL 文字まで
 * @param dst    NULL で変換後の文字数のみ計算
 * @param dstlen NULL 文字も含む長さ。dst が NULL の場合は無視。
 * @return 変換した文字数 (NULL 文字は含まない)。-1 でエラー */

int mUTF8ToWide(const char *src,int srclen,wchar_t *dst,int dstlen)
{
	const char *ps,*psend;
	uint32_t wc;
	int reslen = 0,ret;

	if(srclen < 0) srclen = strlen(src);
	
	ps = src;
	psend = src + srclen;
		
	while(*ps && ps - src < srclen)
	{
		if(dst && reslen >= dstlen) break;
	
		ret = mUTF8ToUCS4Char(ps, psend - ps, &wc, &ps);
		
		if(ret < 0) return -1;
		if(ret > 0) continue;
		
		if(wc > WCHAR_MAX) continue;
		
		if(dst) *(dst++) = wc;
		
		reslen++;
	}
	
	if(dst && reslen < dstlen) *dst = 0;
	
	return reslen;
}

/** UTF-8 文字列をワイド文字列に変換 (メモリ確保)
 * 
 * @param retlen NULL でなければ、結果の文字数が返る
 * @return NULL でエラー */

wchar_t *mUTF8ToWide_alloc(const char *src,int srclen,int *retlen)
{
	int len;
	wchar_t *buf;
	
	len = mUTF8ToWide(src, srclen, NULL, 0);
	if(len < 0) return NULL;
	
	buf = (wchar_t *)mMalloc(sizeof(wchar_t) * (len + 1), FALSE);
	if(!buf) return NULL;
	
	mUTF8ToWide(src, srclen, buf, len + 1);
	
	if(retlen) *retlen = len;
	
	return buf;
}

/** UTF-8 文字列をロケール文字列に変換
 * 
 * NULL 文字が追加できる場合は、最後に NULL 文字を追加する。
 * 
 * @param srclen 負の値で NULL 文字まで
 * @param dst    NULL で変換後のサイズのみ計算
 * @param dstlen NULL 文字も含むサイズ。dst が NULL の場合は無視。
 * @return 変換後のバッファサイズ (NULL 文字は含まない)。-1 でエラー */

int mUTF8ToLocal(const char *src,int srclen,char *dst,int dstlen)
{
	const char *ps,*psEnd;
	uint32_t wc;
	char mb[MB_CUR_MAX];
	mbstate_t state;
	int reslen = 0;
	size_t ret;

	if(srclen < 0) srclen = strlen(src);
	
	ps = src;
	psEnd = src + srclen;

	memset(&state, 0, sizeof(mbstate_t));
	
	while(*ps && ps - src < srclen)
	{
		//UTF8 -> UCS4
	
		ret = mUTF8ToUCS4Char(ps, psEnd - ps, &wc, &ps);
		if(ret < 0) return -1;
		if(ret > 0) continue;
		if(wc > WCHAR_MAX) continue;

		//wchar_t -> local

		ret = wcrtomb(mb, wc, &state);
		if(ret == -1) continue;

		//セット
		
		if(dst)
		{
			if(reslen + ret > dstlen) break;
		
			memcpy(dst, mb, ret);
			dst += ret;
		}
		
		reslen += ret;
	}
	
	if(dst && reslen < dstlen) *dst = 0;
	
	return reslen;
}

/** UTF-8 文字列をロケール文字列に変換 (メモリ確保) */

char *mUTF8ToLocal_alloc(const char *src,int srclen,int *retlen)
{
	char *buf;
	int len;
	
	len = mUTF8ToLocal(src, srclen, NULL, 0);
	if(len < 0) return NULL;
	
	buf = (char *)mMalloc(len + 1, FALSE);
	if(!buf) return NULL;
	
	mUTF8ToLocal(src, srclen, buf, len + 1);
	
	if(retlen) *retlen = len;
	
	return buf;
}


//=============================
// ワイド文字列
//=============================


/** ワイド文字列を UTF-8 文字列に変換
 * 
 * NULL 文字を付加する余裕がある場合、最後に NULL 文字を追加する。 @n
 * 変換できない文字はスキップされる。
 * 
 * @param srclen 負の値で NULL 文字まで
 * @param dst    NULL で変換後の文字数のみ計算
 * @param dstlen NULL 文字も含む長さ。dst が NULL の場合は無視。
 * @return 変換した UTF-8 文字列のバイト数 (NULL 文字は含まない) */

int mWideToUTF8(const wchar_t *src,int srclen,char *dst,int dstlen)
{
	const wchar_t *ps;
	char *pd;
	int reslen = 0,len;

	if(srclen < 0) srclen = wcslen(src);
	
	ps = src;
	pd = dst;
		
	for(; *ps && srclen > 0; srclen--, ps++)
	{
		if(pd && reslen >= dstlen) break;
	
		len = mUCS4ToUTF8Char(*ps, pd);
		
		reslen += len;
		
		if(pd) pd += len;
	}
	
	if(pd && reslen < dstlen) *pd = 0;
	
	return reslen;
}

/** ワイド文字列を UTF-8 文字列に変換 (メモリを確保する)
 * 
 * @param retlen NULL でなければ、結果の文字数が返る
 * @return NULL でエラー */

char *mWideToUTF8_alloc(const wchar_t *src,int srclen,int *retlen)
{
	int len;
	char *buf;
	
	len = mWideToUTF8(src, srclen, NULL, 0);
	
	buf = (char *)mMalloc(len + 1, FALSE);
	if(!buf) return NULL;
	
	mWideToUTF8(src, srclen, buf, len + 1);
	
	if(retlen) *retlen = len;
	
	return buf;
}


//=============================
// ロケール文字列
//=============================


/** ロケール文字列からワイド文字列に変換 */

int mLocalToWide(const char *src,int srclen,wchar_t *dst,int dstlen)
{
	const char *ps = src;
	wchar_t *pd = dst;
	mbstate_t state;
	size_t ret;
	wchar_t wc;
	int len = 0;
	
	if(srclen < 0) srclen = strlen(src);
	
	memset(&state, 0, sizeof(mbstate_t));
	
	while(*ps && ps - src < srclen)
	{
		if(pd && len >= dstlen) break;
	
		ret = mbsrtowcs(&wc, &ps, 1, &state);
		if(ret == 0 || ret == (size_t)-1) break;
		
		if(pd) *(pd++) = wc;
		
		len++;
	}
	
	if(pd && len < dstlen) *pd = 0;
	
	return len;
}

/** ロケール文字列からワイド文字列に変換 (メモリ確保) */

wchar_t *mLocalToWide_alloc(const char *src,int srclen,int *retlen)
{
	int len;
	wchar_t *buf;
	
	len = mLocalToWide(src, srclen, NULL, 0);
	if(len < 0) return NULL;
	
	buf = (wchar_t *)mMalloc(sizeof(wchar_t) * (len + 1), FALSE);
	if(!buf) return NULL;
	
	mLocalToWide(src, srclen, buf, len + 1);
	
	if(retlen) *retlen = len;
	
	return buf;
}

/** ロケール文字列から UTF-8 文字列に変換 (メモリ確保) */

char *mLocalToUTF8_alloc(const char *src,int srclen,int *retlen)
{
	wchar_t *pwc;
	char *utf8;
	int len;
	
	pwc = mLocalToWide_alloc(src, srclen, &len);
	if(!pwc) return NULL;
	
	utf8 = mWideToUTF8_alloc(pwc, len, retlen);
	
	mFree(pwc);
	
	return utf8;
}


//=============================
// UCS-4
//=============================


/** UCS-4 文字数取得 */

int mUCS4Len(const uint32_t *p)
{
	int i;
	
	for(i = 0; *p; p++, i++);
	
	return i;
}

/** UCS-4 文字列を複製 */

uint32_t *mUCS4StrDup(const uint32_t *src)
{
	if(!src)
		return NULL;
	else
	{
		uint32_t *buf;
		int size;

		size = 4 * (mUCS4Len(src) + 1);

		buf = (uint32_t *)mMalloc(size, FALSE);
		if(buf)
			memcpy(buf, src, size);

		return buf;
	}
}

/** UCS-4 -> UTF-8 1文字変換
 * 
 * @param dst NULL で必要なバイト数のみ取得。NULL 文字は追加されない。
 * @return UTF-8 文字のバイト数。0 で変換なし。 */

int mUCS4ToUTF8Char(uint32_t ucs,char *dst)
{
	uint8_t *pd = (uint8_t *)dst;
	
	if(ucs < 0x80)
	{
		if(pd) *pd = ucs;
		
		return 1;
	}
	else if(ucs <= 0x7ff)
	{
		if(pd)
		{
			pd[0] = 0xc0 | (ucs >> 6);
			pd[1] = 0x80 | (ucs & 0x3f);
		}
		
		return 2;
	}
	else if(ucs <= 0xffff)
	{
		if(pd)
		{
			pd[0] = 0xe0 | (ucs >> 12);
			pd[1] = 0x80 | ((ucs >> 6) & 0x3f);
			pd[2] = 0x80 | (ucs & 0x3f);
		}
		
		return 3;
	}
	else if(ucs <= 0x1fffff)
	{
		if(pd)
		{
			pd[0] = 0xf0 | (ucs >> 18);
			pd[1] = 0x80 | ((ucs >> 12) & 0x3f);
			pd[2] = 0x80 | ((ucs >> 6) & 0x3f);
			pd[3] = 0x80 | (ucs & 0x3f);
		}
		
		return 4;
	}
	else
		return 0;
}

/** UCS-4 文字列から UTF-8 文字列に変換
 *
 * UCS-4 の変換できない文字はスキップされる。 @n
 * NULL 文字を付加する余裕がある場合、NULL 文字が追加される。 */

int mUCS4ToUTF8(const uint32_t *src,int srclen,char *dst,int dstlen)
{
	char *pd = dst,m[6];
	int len = 0,clen;

	if(srclen < 0) srclen = mUCS4Len(src);
	
	for(; *src && srclen > 0; src++, srclen--)
	{
		clen = mUCS4ToUTF8Char(*src, m);
		
		if(pd && len + clen > dstlen) break;
		
		if(pd)
		{
			memcpy(pd, m, clen);
			pd += clen;
		}
		
		len += clen;
	}
	
	if(pd && len < dstlen) *pd = 0;
	
	return len;
}

/** UCS-4 文字列から UTF-8 文字列に変換 (メモリ確保) */

char *mUCS4ToUTF8_alloc(const uint32_t *ucs,int srclen,int *retlen)
{
	int i,len,dstlen = 0;
	char *utf8,*pd;

	if(!ucs) return NULL;

	if(srclen < 0) srclen = mUCS4Len(ucs);

	//サイズ
	
	for(i = 0; *ucs && i < srclen; i++)
		dstlen += mUCS4ToUTF8Char(ucs[i], NULL);
	
	//確保
	
	utf8 = (char *)mMalloc(dstlen + 1, FALSE);
	if(!utf8) return NULL;
	
	//変換
	
	pd = utf8;
	
	for(i = 0; *ucs && i < srclen; i++)
	{
		len = mUCS4ToUTF8Char(ucs[i], pd);
		pd += len;
	}
	
	*pd = 0;
	
	if(retlen) *retlen = dstlen;
	
	return utf8;
}

/** UCS-4 文字列から wchar_t 文字列に変換 (メモリ確保) */

wchar_t *mUCS4ToWide_alloc(const uint32_t *src,int srclen,int *retlen)
{
	const uint32_t *ps;
	wchar_t *pwc;
	int dlen;
	
	if(srclen < 0) srclen = mUCS4Len(src);
	
	pwc = (wchar_t *)mMalloc(sizeof(wchar_t) * (srclen + 1), FALSE);
	if(!pwc) return NULL;
	
	for(ps = src, dlen = 0; *ps && ps - src < srclen; ps++)
	{
		if(*ps <= WCHAR_MAX)
			pwc[dlen++] = *ps;
	}
	
	pwc[dlen] = 0;
	
	if(retlen) *retlen = dlen;
	
	return pwc;
}

/** UCS-4 文字列を数値に変換
 * 
 * 小数点以下は dig の桁数まで取得。
 * dig = 1 なら 1.0 = 10 */

int mUCS4ToFloatInt(const uint32_t *text,int dig)
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
	
	if(*text == '-') n = -n;
	
	return n;
}

/** UCS-4 文字列比較 */

int mUCS4Compare(const uint32_t *text1,const uint32_t *text2)
{
	//ポインタが NULL の時

	if(!text1 && !text2)
		return 0;
	else if(!text1)
		return -1;
	else if(!text2)
		return 1;

	//比較

	for(; *text1 && *text2 && *text1 == *text2; text1++, text2++);

	if(*text1 == 0 && *text2 == 0)
		return 0;
	else
		return (*text1 < *text2)? -1: 1;
}


//=============================
// UTF16
//=============================


/** UTF-16 文字列の長さ取得 */

int mUTF16Len(const uint16_t *p)
{
	int len = 0;
	
	for(; *p; p++, len++);

	return len;
}

/** UTF-16 から UCS-4 １文字取得
 *
 * @retval 0 成功
 * @retval 1 無効な文字
 * @retval -1 エラー */

int mUTF16ToUCS4Char(const uint16_t *src,uint32_t *dst,const uint16_t **ppnext)
{
	uint32_t c,c2;
	int ret = 0;

	c = *(src++);

	if((c & 0xfc00) == 0xdc00)
		ret = -1;
	else if(c == 0xfeff) //BOM
		ret = 1;
	else if((c & 0xfc00) == 0xd800)
	{
		//32bit

		c2 = *(src++);

		if((c2 & 0xfc00) != 0xdc00)
			ret = -1;
		else
			c = (((c & 0x03c0) + 0x40) << 10) | ((c & 0x3f) << 10) | (c2 & 0x3ff);
	}

	*dst = c;

	if(ppnext) *ppnext = src;

	return ret;
}

/** UTF-16 文字列を UTF-8 文字列に変換 */

int mUTF16ToUTF8(const uint16_t *src,int srclen,char *dst,int dstlen)
{
	const uint16_t *next;
	char *pd;
	uint32_t ucs;
	int reslen = 0,len;

	if(srclen < 0) srclen = mUTF16Len(src);
	
	pd = dst;
		
	while(*src && srclen > 0)
	{
		if(pd && reslen >= dstlen) break;

		//UTF16 => UCS4

		if(mUTF16ToUCS4Char(src, &ucs, &next)) break;

		srclen -= next - src;
		src = next;

		//UCS4 => UTF8
	
		len = mUCS4ToUTF8Char(ucs, pd);
		
		reslen += len;
		if(pd) pd += len;
	}
	
	if(pd && reslen < dstlen) *pd = 0;
	
	return reslen;
}

/** UTF-16 文字列を UTF-8 文字列に変換 (確保) */

char *mUTF16ToUTF8_alloc(const uint16_t *src,int srclen,int *retlen)
{
	int len;
	char *buf;
	
	len = mUTF16ToUTF8(src, srclen, NULL, 0);
	
	buf = (char *)mMalloc(len + 1, FALSE);
	if(!buf) return NULL;
	
	mUTF16ToUTF8(src, srclen, buf, len + 1);
	
	if(retlen) *retlen = len;
	
	return buf;
}

/** @} */
