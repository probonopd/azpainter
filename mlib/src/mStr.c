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
 * mStr [マルチバイト文字列]
 *****************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <wchar.h>
#include <ctype.h>
#include <stdarg.h>

#include "mDef.h"
#include "mStr.h"
#include "mUtilStr.h"

#ifndef MLIB_NO_PATH_H
#include "mPath.h"
#endif

#ifndef MLIB_NO_UTILCHARCODE_H
#include "mUtilCharCode.h"
#endif

#ifdef MLIB_MEMDEBUG
#include "mMemDebug.h"
#endif


/******************//**

@defgroup str mStr
@brief マルチバイト文字列

- 1文字が可変数バイトから成り、NULL 文字で終わる文字列。
- 基本的に UTF-8 として扱う。
- 未確保の状態でも、文字列のセットや追加などは正しく行われる。
- mStr 構造体はゼロクリアするか、\b MSTR_INIT マクロで初期化しておくこと。

@attention バッファを確保していない場合、buf == NULL の場合がある。

@ingroup group_data
@{

@file mStrDef.h
@file mStr.h
@struct _mStr

@def MSTR_INIT
mStr 構造体の初期化子

*************************/

/*
 * buf : 文字列バッファ (NULL で未確保)
 * len : 文字列長さ (NULL 含まない)
 * allocsize : バッファ確保サイズ (NULL 含む)
 */


//==============================
// sub
//==============================


/** 文字長さから確保サイズ取得
 * 
 * @retval -1 サイズが大きすぎる
 */

static int _str_get_allocsize(int len)
{
	int i;
	
	for(i = 32; i <= len && i < (1<<20); i <<= 1);
	
	return (i >= (1<<20))? -1: i;
}

/** フォーマット文字列追加 */

static void _append_format(mStr *str,const char *format,va_list ap)
{
	const char *pc,*ptext = NULL;
	int len,dig;
	char m[32],m2[16];

	for(pc = format; *pc; pc++)
	{
		if(*pc != '%')
			mStrAppendChar(str, *pc);
		else
		{
			pc++;
			len = -1;

			//'%' の次の文字
			
			if(*pc == 0)
				break;
			else if(*pc == '%')
			{
				mStrAppendChar(str, '%');
				continue;
			}
			else if(*pc == '*')
			{
				//長さ指定
				len = va_arg(ap, int);
				pc++;
			}
			else if(*pc >= '0' && *pc <= '9')
			{
				//桁・長さ 数値指定

				ptext = pc;

				for(len = 0; *pc >= '0' && *pc <= '9'; pc++)
				{
					len *= 10;
					len += *pc - '0';
				}
			}

			//'.' + 数値

			dig = 0;

			if(*pc == '.')
			{
				for(pc++; *pc >= '0' && *pc <= '9'; pc++)
				{
					dig *= 10;
					dig += *pc - '0';
				}
			}

			//型
			
			switch(*pc)
			{
				//int
				case 'd':
					if(len < 1)
						//桁数指定なし
						mStrAppendInt(str, va_arg(ap, int));
					else
					{
						//桁数指定あり (ptext = 桁数の先頭位置)
						
						if(len > 30) len = 30;
						if(*ptext != '0') len = -len;
						
						mIntToStrDig(m, va_arg(ap, int), len);
						mStrAppendText(str, m);
					}
					break;
				//int 浮動小数点
				case 'F':
					mFloatIntToStr(m, va_arg(ap, int), dig);
					mStrAppendText(str, m);
					break;
				//16進数
				case 'X':
				case 'x':
					if(len < 1)
						//桁数指定なし
						snprintf(m2, 16, "%%%c", *pc);
					else
					{
						//桁数指定あり

						if(len > 16) len = 16;

						if(*ptext == '0')
							snprintf(m2, 16, "%%0%d%c", len, *pc);
						else
							snprintf(m2, 16, "%%%d%c", len, *pc);
					}

					snprintf(m, 32, m2, va_arg(ap, int));
					mStrAppendText(str, m);
					break;
				//char
				case 'c':
					mStrAppendChar(str, va_arg(ap, int));
					break;
				//char *
				case 's':
					ptext = va_arg(ap, const char *);
					
					if(len == -1)
						mStrAppendText(str, ptext);
					else
						mStrAppendTextLen(str, ptext, len);
					break;
				//mStr *
				case 't':
					mStrAppendStr(str, va_arg(ap, mStr *));
					break;
			}
		}
	}
}


//==============================


/** 解放 */

void mStrFree(mStr *str)
{
	if(str->buf)
	{
		free(str->buf);
		
		str->buf = NULL;
		str->len = str->allocsize = 0;
	}
}

/** 初期化する */

void mStrInit(mStr *str)
{
	memset(str, 0, sizeof(mStr));
}

/** 初期バッファ確保
 *
 * str の現在値に関係なく強制的に確保されるので注意。 */

mBool mStrAlloc(mStr *str,int size)
{
	str->buf = (char *)malloc(size);
	if(!str->buf) return FALSE;

	str->buf[0] = 0;
	str->len = 0;
	str->allocsize = size;
		
	return TRUE;
}

/** 文字長さから確保サイズを変更 */

mBool mStrResize(mStr *str,int len)
{
	int size;
	char *newbuf;
	
	size = _str_get_allocsize(len);
	if(size == -1) return FALSE;
	
	if(!str->buf)
	{
		//新規確保

		return mStrAlloc(str, size);
	}
	else if(size > str->allocsize)
	{
		//サイズ変更
	
		newbuf = (char *)realloc(str->buf, size);
		if(!newbuf) return FALSE;
		
		str->buf = newbuf;
		str->allocsize = size;
	}

	return TRUE;
}

/** 文字列の長さを再計算 */

void mStrCalcLen(mStr *str)
{
	if(str->buf)
		str->len = strlen(str->buf);
}

/** 文字列が空かどうか */

mBool mStrIsEmpty(mStr *str)
{
	return (!str->buf || !str->buf[0]);
}

/** 文字列を空にする */

void mStrEmpty(mStr *str)
{
	if(str->buf)
	{
		str->buf[0] = 0;
		str->len = 0;
	}
}

/** 指定した長さにセット
 *
 * len の位置に NULL 文字をセットして、文字長さもセット (最大で確保サイズまで) */

void mStrSetLen(mStr *str,int len)
{
	if(len < str->allocsize)
	{
		str->buf[len] = 0;
		str->len = len;
	}
}

/** mStr をコピー (src が空なら dst は未確保のままの場合あり) */

void mStrCopy(mStr *dst,mStr *src)
{
	if(mStrIsEmpty(src))
		mStrEmpty(dst);
	else
	{
		if(mStrResize(dst, src->len))
		{
			memcpy(dst->buf, src->buf, src->len + 1);

			dst->len = src->len;
		}
	}
}

/** dst を初期化後コピー */

void mStrCopy_init(mStr *dst,mStr *src)
{
	mStrInit(dst);
	mStrCopy(dst, src);
}

/** mStr をコピー (src が空の場合でも必ずバッファが確保される) */

void mStrCopy_alloc(mStr *dst,mStr *src)
{
	if(!mStrIsEmpty(src))
		mStrCopy(dst, src);
	else
	{
		//空の場合

		if(dst->buf)
			mStrEmpty(dst);
		else
			mStrAlloc(dst, 32);
	}
}

/** 文字数位置からバイト数を取得 (UTF-8) */

#ifndef MLIB_NO_UTILCHARCODE_H

int mStrCharLenToByte(mStr *str,int len)
{
	int pos;
	
	for(pos = 0; len > 0 && pos < str->len; len--)
		pos += mUTF8CharWidth(str->buf + pos);
	
	if(pos > str->len) pos = str->len;
	
	return pos;
}

/** バイト数を指定して文字数を制限 (UTF-8)
 *
 * @return TRUE で文字列が切り詰められた */

mBool mStrLimitBytes(mStr *str,int bytes)
{
	int pos,n;

	if(str->buf && str->len)
	{
		for(pos = 0; pos < str->len; pos += n)
		{
			n = mUTF8CharWidth(str->buf + pos);

			if(pos + n > bytes)
			{
				str->buf[pos] = 0;
				str->len = pos;
				return TRUE;
			}
		}
	}
	
	return FALSE;
}

#endif


//================================
// 取得
//================================


/** 最後の文字を取得 */

char mStrGetLastChar(mStr *str)
{
	if(str->buf && str->len > 0)
		return str->buf[str->len - 1];
	else
		return 0;
}


//================================
// セット
//================================


/** 1文字セット */

void mStrSetChar(mStr *str,char c)
{
	if(mStrResize(str, 1))
	{
		str->buf[0] = c;
		str->buf[1] = 0;
		str->len = 1;
	}
}

/** 文字列をセット
 * 
 * @param text NULL で空文字列にする */

void mStrSetText(mStr *str,const char *text)
{
	int len;

	if(!text)
		mStrEmpty(str);
	else
	{
		len = strlen(text);
		
		if(mStrResize(str, len))
		{
			memcpy(str->buf, text, len + 1);
			str->len = len;
		}
	}
}

/** 長さを指定して文字列をセット */

void mStrSetTextLen(mStr *str,const char *text,int len)
{
	if(!text)
		mStrEmpty(str);
	else if(mStrResize(str, len))
	{
		memcpy(str->buf, text, len);
		
		str->buf[len] = 0;
		str->len = len;
	}
}

/** int 値を文字列に変換してセット */

void mStrSetInt(mStr *str,int val)
{
	char m[20];
	
	mIntToStr(m, val);
	mStrSetText(str, m);
}

/** double 値を文字列に変換してセット
 *
 * 小数点以下が 0 なら、整数のみとする。
 *
 * @param dig 小数点以下の桁数 */

void mStrSetDouble(mStr *str,double d,int dig)
{
	char m[64];

	if(d - (int)d == 0)
		//小数点以下が0
		snprintf(m, 64, "%d", (int)d);
	else
		snprintf(m, 64, "%.*f", dig, d);

	mStrSetText(str, m);
}

/** ワイド文字列 (wchar_t *) からセット
 *
 * @param len 負の値で自動 */

#ifndef MLIB_NO_UTILCHARCODE_H

void mStrSetTextWide(mStr *str,const void *text,int len)
{
	const wchar_t *src = (const wchar_t *)text;
	int utf8len;

	if(!text)
		mStrEmpty(str);
	else
	{
		utf8len = mWideToUTF8(src, len, NULL, 0);

		if(mStrResize(str, utf8len))
		{
			mWideToUTF8(src, len, str->buf, str->allocsize);

			str->len = utf8len;
		}
	}
}

/** ロケールの文字列からセット
 *
 * @param len 負の値で自動 */

void mStrSetTextLocal(mStr *str,const char *text,int len)
{
	char *utf8;
	int ulen;
	
	if(!text)
		mStrEmpty(str);
	else
	{
		utf8 = mLocalToUTF8_alloc(text, len, &ulen);
		if(!utf8) return;
		
		mStrSetTextLen(str, utf8, ulen);
		
		mFree(utf8);
	}
}

/** UCS-4 文字列からセット */

void mStrSetTextUCS4(mStr *str,const uint32_t *text,int len)
{
	int utf8len;
	
	if(!text)
		mStrEmpty(str);
	else
	{
		utf8len = mUCS4ToUTF8(text, len, NULL, 0);
		
		if(mStrResize(str, utf8len))
		{
			mUCS4ToUTF8(text, len, str->buf, str->allocsize);
			
			str->len = utf8len;
		}
	}
}

#endif

/** フォーマット文字列からセット

| フォーマット | 説明 |
| --- | --- |
| \%\% | '\%' |
| \%d | int |
| \%.{dig}F | int 浮動小数点数 |
| \%x, \%X | 16進数 |
| \%c | char |
| \%s | char * |
| \%\*s | char *。前の引数で文字数を指定 |
| \%t | mStr * |
*/

void mStrSetFormat(mStr *str,const char *format,...)
{
	va_list ap;

	mStrEmpty(str);

	va_start(ap, format);

	_append_format(str, format, ap);

	va_end(ap);
}

/** 半角英数字以外を \%XX にした文字列をセット */

void mStrSetPercentEncoding(mStr *str,const char *text)
{
	char c,m[6];
	
	mStrEmpty(str);

	if(!text) return;

	for(; (c = *text); text++)
	{
		if(isalnum(c))
			mStrAppendChar(str, c);
		else
		{
			snprintf(m, 6, "%%%02X", (uint8_t)c);
			mStrAppendText(str, m);
		}
	}
}

/** \%XX をデコードした文字列をセット */

void mStrDecodePercentEncoding(mStr *str,const char *text)
{
	char c,m[3] = {0,0,0};

	mStrEmpty(str);

	if(!text) return;

	for(; (c = *text); text++)
	{
		if(c != '%')
			mStrAppendChar(str, c);
		else
		{
			if(text[1] && text[2])
			{
				m[0] = text[1];
				m[1] = text[2];

				c = (char)strtoul(m, NULL, 16);
				if(c == 0) break;
				
				mStrAppendChar(str, c);

				text += 2;
			}
		}
	}
}

/** \%XX をデコードした文字列をセット ('+' はスペースに変換) */

void mStrDecodePercentEncoding_plus(mStr *str,const char *text)
{
	char c,m[3] = {0,0,0};

	mStrEmpty(str);

	if(!text) return;

	for(; (c = *text); text++)
	{
		if(c == '+')
			mStrAppendChar(str, ' ');
		else if(c != '%')
			mStrAppendChar(str, c);
		else
		{
			if(text[1] && text[2])
			{
				m[0] = text[1];
				m[1] = text[2];

				c = (char)strtoul(m, NULL, 16);
				if(c == 0) break;
				
				mStrAppendChar(str, c);

				text += 2;
			}
		}
	}
}


//============================
// URL
//============================


#ifndef MLIB_NO_UTILCHARCODE_H

/** text/uri-list の文字列を変換してセット
 *
 * 改行は \\t に変換し、"%xx" は文字に変換する。
 *
 * @param uri 各 URI は改行で区切られている。ロケールの文字列。
 * @param localfile "file://" のファイルのみ取得。かつ "file://" を除去
 * @return セットされた URI の数 */

int mStrSetURIList(mStr *str,const char *uri,mBool localfile)
{
	char *buf,*top,*next,*ps,*pd;
	char m[3] = {0,0,0};
	int num = 0;

	mStrEmpty(str);

	//URI コピー

	buf = mStrdup(uri);
	if(!buf) return 0;

	//

	for(top = buf; top; top = next)
	{
		next = mGetStrNextLine(top, TRUE);
		if(!next) break;
	
		//"file://" のみ

		if(localfile)
		{
			if(*top == 'f' && strncmp(top, "file://", 7) == 0)
				top += 7;
			else
				continue;
		}

		//"%xx" を変換

		for(ps = pd = top; *ps; pd++)
		{
			if(*ps == '%' && ps[1] && ps[2])
			{
				m[0] = ps[1];
				m[1] = ps[2];

				*pd = (char)strtoul(m, NULL, 16);
				ps += 3;
			}
			else
			{
				if(ps != pd) *pd = *ps;
				ps++;
			}
		}

		*pd = 0;

		//追加

		if(*top)
		{
			mStrAppendTextLocal(str, top, -1);
			mStrAppendChar(str, '\t');

			num++;
		}
	}

	mFree(buf);

	return num;
}

#endif

/** URL エンコード文字列に変換
 *
 * 英数字、'-' '.' '_' '~' '/' 以外は \%XX でエンコード */

void mStrSetURLEncode(mStr *str,const char *text)
{
	char c,flag,m[6];
	
	mStrEmpty(str);

	if(!text) return;

	for(; (c = *text); text++)
	{
		//flag = [0] %XX [1] そのまま

		if(c < 0)
			flag = 0;
		else if(c == '-' || c == '.' || c == '_' || c == '~' || c == '/' || isalnum(c))
			flag = 1;
		else
			flag = 0;

		//

		if(flag)
			mStrAppendChar(str, c);
		else
		{
			snprintf(m, 6, "%%%02X", (uint8_t)c);
			mStrAppendText(str, m);
		}
	}
}


//============================
// 追加
//============================


/** 1文字追加 */

void mStrAppendChar(mStr *str,char c)
{
	if(mStrResize(str, str->len + 1))
	{
		str->buf[str->len] = c;
		str->buf[str->len + 1] = 0;
		str->len++;
	}
}

/** UCS-4 1文字を追加 */

#ifndef MLIB_NO_UTILCHARCODE_H

void mStrAppendCharUCS4(mStr *str,uint32_t ucs)
{
	int len;
	
	len = mUCS4ToUTF8Char(ucs, NULL);
	
	if(len && mStrResize(str, str->len + len))
	{
		mUCS4ToUTF8Char(ucs, str->buf + str->len);
		
		str->len += len;
		str->buf[str->len] = 0;
	}
}

#endif

/** 文字列を追加 */

void mStrAppendText(mStr *str,const char *text)
{
	int len;

	if(text)
	{
		len = strlen(text);
		
		if(mStrResize(str, str->len + len))
		{
			memcpy(str->buf + str->len, text, len + 1);
			str->len += len;
		}
	}
}

/** 長さを指定して文字列を追加 */

void mStrAppendTextLen(mStr *str,const char *text,int len)
{
	if(text && len > 0 && mStrResize(str, str->len + len))
	{
		memcpy(str->buf + str->len, text, len);
		str->len += len;

		str->buf[str->len] = 0;
	}
}

/** int 値を文字列に変換して追加 */

void mStrAppendInt(mStr *str,int val)
{
	char m[20];

	mIntToStr(m, val);
	mStrAppendText(str, m);
}

/** double 値を文字列に変換して追加
 *
 * @param dig 小数点以下の桁数 */

void mStrAppendDouble(mStr *str,double d,int dig)
{
	char m[64];

	if(d - (int)d == 0)
		snprintf(m, 64, "%d", (int)d);
	else
		snprintf(m, 64, "%.*f", dig, d);

	mStrAppendText(str, m);
}

/** ロケールの文字列を追加 */

#ifndef MLIB_NO_UTILCHARCODE_H

void mStrAppendTextLocal(mStr *str,const char *text,int len)
{
	char *utf8;
	int ulen;

	if(len < 0) len = strlen(text);
	
	if(text && mStrResize(str, str->len + len))
	{
		utf8 = mLocalToUTF8_alloc(text, len, &ulen);
		if(utf8)
		{
			mStrAppendTextLen(str, utf8, ulen);
			mFree(utf8);
		}
	}
}

#endif

/** mStr 文字列を追加 */

void mStrAppendStr(mStr *dst,mStr *src)
{
	if(src->buf && mStrResize(dst, dst->len + src->len))
	{
		memcpy(dst->buf + dst->len, src->buf, src->len + 1);
		dst->len += src->len;
	}
}

/** フォーマット文字列を追加 */

void mStrAppendFormat(mStr *str,const char *format,...)
{
	va_list ap;
	
	va_start(ap, format);

	_append_format(str, format, ap);

	va_end(ap);
}

/** 先頭に文字列を挿入 */

void mStrPrependText(mStr *str,const char *text)
{
	int len;

	if(text)
	{
		len = strlen(text);
		
		if(mStrResize(str, str->len + len))
		{
			memmove(str->buf + len, str->buf, str->len + 1);
			memcpy(str->buf, text, len);
			
			str->len += len;
		}
	}
}


//==================================
// 取得
//==================================


/** src の指定位置から指定文字数分を取り出してセット
 *
 * @param len 取り出す文字数。負の値で pos〜最後 まで */

void mStrMid(mStr *dst,mStr *src,int pos,int len)
{
	int maxlen;

	if(pos >= src->len)
	{
		mStrEmpty(dst);
		return;
	}

	maxlen = src->len - pos;

	if(len < 0)
		len = maxlen;
	else if(len > maxlen)
		len = maxlen;

	mStrSetTextLen(dst, src->buf + pos, len);
}

/** 指定文字で区切られた中の指定位置の文字列を取得
 *
 * 区切り文字が複数連続している場合は空文字とする。 */

void mStrGetSplitText(mStr *dst,const char *text,char split,int index)
{
	const char *pc,*pend;
	int cnt;

	mStrEmpty(dst);

	if(!text) return;

	for(pc = text, cnt = 0; *pc; cnt++)
	{
		//終端位置

		pend = strchr(pc, split);
		if(!pend) pend = strchr(pc, 0);

		//セット

		if(cnt == index)
		{
			mStrSetTextLen(dst, pc, pend - pc);
			break;
		}

		//次へ

		pc = (*pend)? pend + 1: pend;
	}
}


//==================================
// 変換
//==================================


/** アルファベットを小文字に変換 */

void mStrLower(mStr *str)
{
	char *p;

	if(!str->buf) return;
	
	for(p = str->buf; *p; p++)
	{
		if(*p >= 'A' && *p <= 'Z') *p += 0x20;
	}
}

/** アルファベットを大文字に変換 */

void mStrUpper(mStr *str)
{
	char *p;
	
	if(!str->buf) return;

	for(p = str->buf; *p; p++)
	{
		if(*p >= 'a' && *p <= 'z') *p -= 0x20;
	}
}

/** int 値に変換 */

int mStrToInt(mStr *str)
{
	if(str->buf)
		return atoi(str->buf);
	else
		return 0;
}

/** double 値に変換 */

double mStrToDouble(mStr *str)
{
	if(str->buf)
		return strtod(str->buf, NULL);
	else
		return 0;
}

/** 文字で区切られた複数数値を配列で取得
 *
 * @param ch  区切り文字。0 で数字以外の文字すべて。
 * @return 取得した数値の個数 */

int mStrToIntArray(mStr *str,int *dst,int maxnum,char ch)
{
	char *p = str->buf;
	int num = 0;

	if(!p) return 0;

	while(*p)
	{
		//数値取得

		if(num >= maxnum) break;

		*(dst++) = atoi(p);
		num++;

		//次の数値へ

		if(ch)
		{
			for(; *p && *p != ch; p++);
		}
		else
		{
			for(; *p && *p >= '0' && *p <= '9'; p++);
		}

		if(*p) p++;
	}

	return num;
}

/** 複数数値を配列で取得 (最小値、最大値判定あり) */

int mStrToIntArray_range(mStr *str,int *dst,int maxnum,char ch,int min,int max)
{
	int i,num,n;

	num = mStrToIntArray(str, dst, maxnum, ch);

	for(i = 0; i < num; i++)
	{
		n = dst[i];
		if(n < min) n = min;
		else if(n > max) n = max;

		dst[i] = n;
	}

	return num;
}

/** 文字を置き換え */

void mStrReplaceChar(mStr *str,char ch,char chnew)
{
	char *p;
	
	if(!str->buf) return;

	for(p = str->buf; *p; p++)
	{
		if(*p == ch) *p = chnew;
	}
}

/** 指定文字で区切られた文字列の指定位置のテキストを置換え */

void mStrReplaceSplitText(mStr *str,char split,int index,const char *text)
{
	mStr copy = MSTR_INIT;
	int cnt;
	char *pc,*pend;

	mStrCopy_alloc(&copy, str);
	mStrEmpty(str);

	for(pc = copy.buf, cnt = 0; 1; cnt++)
	{
		//終端位置

		if(*pc)
		{
			pend = strchr(pc, split);
			if(!pend) pend = strchr(pc, 0);
		}
		else
			pend = pc;

		//セット

		if(cnt == index)
			mStrAppendText(str, text);
		else
			mStrAppendTextLen(str, pc, pend - pc);

		//終了

		if(cnt >= index && !(*pend)) break;

		//区切り文字追加

		mStrAppendChar(str, split);

		//次へ

		pc = (*pend)? pend + 1: pend;
	}

	mStrFree(&copy);
}

/** \%f など、指定文字＋1文字の部分を任意の文字列に置き換える
 *
 * @param paramch '\%' など、置換え対象となる先頭文字。この文字が2つ続くとその文字1つに置換。
 * @param reparray 置換文字列の配列。先頭の1文字は paramch に続く文字。 */

void mStrReplaceParams(mStr *str,char paramch,mStr *reparray,int arraynum)
{
	mStr copy = MSTR_INIT;
	char *ps,c;
	int i;

	if(mStrIsEmpty(str)) return;

	mStrCopy(&copy, str);
	mStrEmpty(str);

	for(ps = copy.buf; *ps; )
	{
		c = *(ps++);
	
		if(c != paramch)
			//通常文字
			mStrAppendChar(str, c);
		else
		{
			c = *(ps++);

			if(!c)
				//'%'で終わっている場合
				break;
			else if(c == paramch)
				mStrAppendChar(str, c);
			else
			{
				//置換え対象検索
				
				for(i = 0; i < arraynum; i++)
				{
					if(reparray[i].buf[0] == c) break;
				}

				if(i == arraynum)
					ps--;
				else
					mStrAppendText(str, reparray[i].buf + 1);
			}
		}
	}

	mStrFree(&copy);
}


//==================================
// 検索
//==================================


/** 文字を検索 */

int mStrFindChar(mStr *str,char ch)
{
	char *p;

	if(!str->buf) return -1;

	p = strchr(str->buf, ch);

	return (p)? p - str->buf: -1;
}

/** 終端から文字を検索
 *
 * @return 文字の位置。-1 で見つからなかった。 */

int mStrFindCharRev(mStr *str,char ch)
{
	char *p;

	if(!str->buf) return -1;

	p = strrchr(str->buf, ch);

	return (p)? p - str->buf: -1;
}

/** 指定文字を検索し、その位置で文字列を終わらせる */

void mStrFindCharToEnd(mStr *str,char ch)
{
	int pos = mStrFindChar(str, ch);

	if(pos != -1) mStrSetLen(str, pos);
}


//==================================
// 比較
//==================================


/** 文字列比較 (等しいか) */

mBool mStrCompareEq(mStr *str,const char *text)
{
	if(str->buf && text)
		return (strcmp(str->buf, text) == 0);
	else
		//どちらかが NULL の場合
		return ((!str->buf || !*(str->buf)) && (!text || !*text));
}

/** 文字列比較 (大/小文字区別しない。等しいか) */

mBool mStrCompareCaseEq(mStr *str,const char *text)
{
	if(str->buf && text)
		return (strcasecmp(str->buf, text) == 0);
	else
		return ((!str->buf || !*(str->buf)) && (!text || !*text));
}


//==================================
// パス
//==================================


#ifndef MLIB_NO_PATH_H

/** パスがトップの状態かどうか (親がないかどうか) */

mBool mStrPathIsTop(mStr *str)
{
	return mPathIsTop(str->buf);
}

/** ホームディレクトリパスをセット */

void mStrPathSetHomeDir(mStr *str)
{
	char *path;

	path = mGetHomePath();

	mStrSetText(str, path);

	mFree(path);
}

/** ホームディレクトリ+指定パスをセット */

void mStrPathSetHomeDir_add(mStr *str,const char *path)
{
	mStrPathSetHomeDir(str);
	mStrPathAdd(str, path);
}

/** 終端のパス区切り文字を除去する */

void mStrPathRemoveBottomPathSplit(mStr *str)
{
	int pos;

	pos = mPathGetBottomSplitCharPos(str->buf, str->len);

	if(pos != -1)
		mStrSetLen(str, pos);
}

/** ファイル名として無効な文字を置き換える */

void mStrPathReplaceDisableChar(mStr *str,char rep)
{
	char *pc;

	for(pc = str->buf; *pc; pc++)
	{
		if(mPathIsDisableFilenameChar(*pc))
			*pc = rep;
	}
}

/** ファイル名を除いてディレクトリ名のみにする */

void mStrPathRemoveFileName(mStr *str)
{
	int pos = mPathGetSplitCharPos(str->buf, TRUE);

	if(pos == -1)
		mStrEmpty(str);
	else if(pos == 0)
		//先頭がパス区切りの場合、その文字だけ残す
		mStrSetLen(str, 1);
	else
		mStrSetLen(str, pos);
}

/** パスからディレクトリのみを取得 */

void mStrPathGetDir(mStr *dst,const char *path)
{
	int pos = mPathGetSplitCharPos(path, TRUE);

	if(pos == -1)
		mStrEmpty(dst);
	else
		mStrSetTextLen(dst, path, pos);
}

/** ディレクトリ名を除いたファイル名を取得 */

void mStrPathGetFileName(mStr *dst,const char *path)
{
	int pos = mPathGetSplitCharPos(path, TRUE);

	if(pos == -1)
		mStrSetText(dst, path);
	else
		mStrSetText(dst, path + pos + 1);
}

/** 拡張子を除いたファイル名のみ取得してセット */

void mStrPathGetFileNameNoExt(mStr *dst,const char *path)
{
	int pos;

	mStrPathGetFileName(dst, path);

	//拡張子除去 (先頭が '.' の場合は除く)

	pos = mStrFindCharRev(dst, '.');

	if(pos > 0)
		mStrSetLen(dst, pos);
}

/** 拡張子を取得 */

void mStrPathGetExt(mStr *dst,const char *path)
{
	char *pc;
	int top;

	top = mPathGetSplitCharPos(path, TRUE);
	if(top == -1) top = 0; else top++;

	//最後の '.' を検索

	pc = strrchr(path + top, '.');

	if(!pc || pc == path + top)
		mStrEmpty(dst);
	else
		mStrSetText(dst, pc + 1);
}

/** パスをディレクトリとファイル名に分割
 *
 * @param dst1 ディレクトリ部分 (最後のパス区切り文字も含む)*/

void mStrPathSplitByDir(mStr *dst1,mStr *dst2,const char *path)
{
	int pos;

	pos = mPathGetSplitCharPos(path, TRUE);
	if(pos == -1)
	{
		mStrEmpty(dst1);
		mStrSetText(dst2, path);
	}
	else
	{
		mStrSetTextLen(dst1, path, pos + 1);
		mStrSetText(dst2, path + pos + 1);
	}
}

/** パスを拡張子の前と後に分割する
 *
 * @param dst2 拡張子部分。'.' も含む */

void mStrPathSplitByExt(mStr *dst1,mStr *dst2,const char *path)
{
	int pos;
	const char *pc;

	//ファイル名の開始位置

	pos = mPathGetSplitCharPos(path, TRUE);
	if(pos == -1)
		pos = 0;
	else
		pos++;

	//'.' 検索

	pc = strrchr(path + pos, '.');

	if(!pc || pc == path + pos)
	{
		//拡張子なし
		mStrSetText(dst1, path);
		mStrEmpty(dst2);
	}
	else
	{
		mStrSetTextLen(dst1, path, pc - path);
		mStrSetText(dst2, pc);
	}
}

/** 現在のパスにパス文字を追加後、src を追加
 *
 * dst が空の場合や、パス区切り文字で終わっている場合は、パス文字は追加しない。*/

void mStrPathAdd(mStr *dst,const char *path)
{
	if(dst->len > 0 && dst->buf[dst->len - 1] != mPathGetSplitChar())
		mStrAppendChar(dst, mPathGetSplitChar());

	mStrAppendText(dst, path);
}

/** 拡張子をセットする
 *
 * すでに同じ拡張子がある場合はそのまま。なければ '.' + ext を追加する。 */

void mStrPathSetExt(mStr *path,const char *ext)
{
	if(!mStrPathCompareExtEq(path, ext))
	{
		mStrAppendChar(path, '.');
		mStrAppendText(path, ext);
	}
}

/** ディレクトリ、ファイル名、拡張子を結合したパスを取得
 *
 * @param ext NULL で拡張子なし */

void mStrPathCombine(mStr *dst,const char *dir,const char *fname,const char *ext)
{
	mStrSetText(dst, dir);
	mStrPathAdd(dst, fname);

	if(ext)
	{
		mStrAppendChar(dst, '.');
		mStrAppendText(dst, ext);
	}
}

/** 入力ファイル名を元に、出力ファイル名を取得
 *
 * @param outdir 出力先ディレクトリ。NULL または空で入力先と同じ場所。
 * @param outext 出力ファイル名の拡張子。NULL でそのまま。*/

void mStrPathGetOutputFile(mStr *dst,const char *infile,const char *outdir,const char *outext)
{
	mStr fname = MSTR_INIT;

	//出力先ディレクトリ

	if(outdir && *outdir)
		mStrSetText(dst, outdir);
	else
		mStrPathGetDir(dst, infile);

	//ファイル名

	if(!outext)
		mStrPathGetFileName(&fname, infile);
	else
	{
		mStrPathGetFileNameNoExt(&fname, infile);
		mStrAppendChar(&fname, '.');
		mStrAppendText(&fname, outext);
	}

	//結合

	mStrPathAdd(dst, fname.buf);

	mStrFree(&fname);
}

/** パスが同じか比較 */

mBool mStrPathCompareEq(mStr *str,const char *path)
{
	if(str->buf && path)
		return mPathCompareEq(str->buf, path);
	else
		return FALSE;
}

/** パスの拡張子を比較
 *
 * str の終端が指定文字列と同じかどうか。大文字/小文字は比較しない。 */

mBool mStrPathCompareExtEq(mStr *path,const char *ext)
{
	mStr str = MSTR_INIT;
	mBool ret;

	if(!path->buf || !ext) return FALSE;

	//比較文字列

	mStrSetChar(&str, '.');
	mStrAppendText(&str, ext);

	//比較

	if(path->len < str.len)
		ret = FALSE;
	else
		ret = (strcasecmp(path->buf + path->len - str.len, str.buf) == 0);

	mStrFree(&str);

	return ret;
}

/** パスの拡張子を複数比較
 *
 * @param exts ':' で区切る。拡張子区切りはなくて良い。
 * @return 該当するものがあれば TRUE */

mBool mStrPathCompareExts(mStr *path,const char *exts)
{
	mStr str = MSTR_INIT;
	const char *ptop,*pend;
	mBool ret = FALSE;

	if(!path->buf || !exts) return FALSE;

	ptop = exts;

	while(mGetStrNextSplit(&ptop, &pend, ':'))
	{
		//比較文字列

		mStrSetChar(&str, '.');
		mStrAppendTextLen(&str, ptop, pend - ptop);

		//比較

		if(path->len >= str.len)
		{
			if(strcasecmp(path->buf + path->len - str.len, str.buf) == 0)
			{
				ret = TRUE;
				break;
			}
		}

		ptop = pend;
	}

	mStrFree(&str);

	return ret;
}

/** path のファイル名を除いたディレクトリが同じか比較 */

mBool mStrPathCompareDir(mStr *path,const char *dir)
{
	mStr str = MSTR_INIT;
	mBool ret;

	mStrPathGetDir(&str, path->buf);

	ret = mPathCompareEq(str.buf, dir);

	mStrFree(&str);

	return ret;
}

/** ファイルダイアログの複数選択文字列から各ファイルのフルパスを取り出す
 *
 * @param param 作業用 (開始時は 0 を入れておくこと)
 * @return FALSE で終了 */

mBool mStrPathExtractMultiFiles(mStr *dst,const char *text,int *param)
{
	const char *ptop,*pend,*dirend;

	//ディレクトリ

	dirend = strchr(text, '\t');
	if(!dirend) return FALSE;

	//次のファイル

	ptop = (*param == 0)? dirend + 1: text + *param;
	if(*ptop == 0) return FALSE;

	pend = strchr(ptop, '\t');
	if(!pend) return FALSE;

	//パス

	mStrSetTextLen(dst, text, dirend - text);

	if(dst->len > 0 && dst->buf[dst->len - 1] != mPathGetSplitChar())
		mStrAppendChar(dst, mPathGetSplitChar());

	mStrAppendTextLen(dst, ptop, pend - ptop);

	//

	*param = pend + 1 - text;

	return TRUE;
}

#endif //MLIB_NO_PATH_H


//=============================
// 配列
//=============================


/** mStr の配列をすべて解放 */

void mStrArrayFree(mStr *array,int num)
{
	int i;

	for(i = 0; i < num; i++)
		mStrFree(array + i);
}

/** mStr 配列を初期化 */

void mStrArrayInit(mStr *array,int num)
{
	memset(array, 0, sizeof(mStr) * num);
}

/** mStr 配列の文字列をすべてコピー */

void mStrArrayCopy(mStr *dst,mStr *src,int num)
{
	int i;

	for(i = 0; i < num; i++)
		mStrCopy(dst + i, src + i);
}

/** mStr 配列の内容を上方向にシフトする
 * 
 * start を削除して詰める。end の位置は空になる。 */

void mStrArrayShiftUp(mStr *array,int start,int end)
{
	int i;

	mStrFree(array + start);

	for(i = start; i < end; i++)
		array[i] = array[i + 1];

	/* 上記で end の位置のデータをずらしてコピーしているので、
	 * end - 1 と end のデータは同じになっている。
	 * end は解放してはいけないので、値を初期化して空にする。 */
	
	mStrInit(array + end);
}

/** 文字列の履歴配列にテキストをセット、または配列内移動
 *
 * 同じ項目かどうかの判定を独自で行った後、配列内にセットしたい時に使う。\n
 * text が array 内の文字列でも問題ない。
 *
 * @param index 負の値で新規挿入。正の値でその位置のデータを先頭へ移動して、先頭に新しい文字列セット */

void mStrArraySetRecent(mStr *array,int arraynum,int index,const char *text)
{
	mStr str = MSTR_INIT;
	int i;

	//新規時は終端

	if(index < 0)
		index = arraynum - 1;
	
	//セットする文字列をコピー
	//(text が array 内の文字列の場合があるので)

	mStrSetText(&str, text);

	//削除

	mStrFree(array + index);

	//先頭を空ける

	for(i = index; i > 0; i--)
		array[i] = array[i - 1];

	//先頭にセット (str をそのままセット)

	array[0] = str;
}

#ifndef MLIB_NO_PATH_H

/** 文字列の履歴配列に新しい文字列を追加
 *
 * 指定文字列が一番先頭に来る。\n
 * 配列内に同じ文字列がある場合、それを先頭へ移動する。\n
 * text が array 内の文字列でも問題ない。
 *
 * @param bPath パス名かどうか */

void mStrArrayAddRecent(mStr *array,int arraynum,const char *text,mBool bPath)
{
	int i,find;
	mBool ret;

	//array 内に同じ文字列があるか

	for(i = 0, find = -1; i < arraynum; i++)
	{
		if(bPath)
			ret = mStrPathCompareEq(array + i, text);
		else
			ret = mStrCompareEq(array + i, text);
	
		if(ret)
		{
			find = i;
			break;
		}
	}

	//セット

	mStrArraySetRecent(array, arraynum, find, text);
}

#endif

/** @} */
