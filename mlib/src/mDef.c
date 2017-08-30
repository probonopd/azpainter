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
 * 標準定義関数
 *****************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "mDef.h"

#ifdef MLIB_MEMDEBUG
#include "mMemDebug.h"
#endif


/********************//**

@defgroup default mDef
@brief 標準定義

@ingroup group_main
@{

@file mDef.h
@file mDefGui.h

*************************/


/** stderr にメッセージ表示 */

void mDebug(const char *format,...)
{
	va_list arg;

	va_start(arg, format);
	vfprintf(stderr, format, arg);
	va_end(arg);

	fflush(stderr);
}


//===========================
// メモリ関連 - 通常
//===========================


/** メモリ解放
 * 
 * @param ptr NULL で何もしない */

void mFree(void *ptr)
{
	if(ptr) free(ptr);
}

/** メモリ確保 (非デバッグ)
 * 
 * @param clear TRUE でゼロクリア */

void *__mMalloc(uint32_t size,mBool clear)
{
	void *ptr;
	
	ptr = malloc(size);
	if(!ptr) return NULL;
	
	if(clear) memset(ptr, 0, size);
	
	return ptr;
}

/** メモリサイズ変更 (非デバッグ) */

void *__mRealloc(void *ptr,uint32_t size)
{
	if(ptr)
		return realloc(ptr, size);
	else
		return NULL;
}

/** メモリバッファ複製 */

void *mMemdup(const void *src,uint32_t size)
{
	void *buf;

	if(!src) return NULL;

	buf = mMalloc(size, FALSE);
	if(!buf)
		return NULL;
	else
	{
		memcpy(buf, src, size);
		return buf;
	}
}

/** バッファをゼロクリア */

void mMemzero(void *buf,int size)
{
	if(buf)
		memset(buf, 0, size);
}


//===========================
// メモリデバッグ用
//===========================


#ifdef MLIB_MEMDEBUG

void *__mMalloc_debug(uint32_t size,mBool clear,const char *filename,int line)
{
	void *ptr;
	
	ptr = mMemDebug_malloc(size, filename, line);
	if(!ptr) return NULL;
	
	if(clear) memset(ptr, 0, size);
	
	return ptr;
}

void *__mRealloc_debug(void *ptr,uint32_t size,const char *filename,int line)
{
	return mMemDebug_realloc(ptr, size, filename, line);
}

char *__mStrdup_debug(const char *str,const char *filename,int line)
{
	return mMemDebug_strdup(str, filename, line);
}

char *__mStrndup_debug(const char *str,int len,const char *filename,int line)
{
	return mMemDebug_strndup(str, len, filename, line);
}

#endif


//===========================
// 文字列関連
//===========================


/** 文字列複製 (非デバッグ)
 *
 * @return str が NULL の場合は NULL を返す */

char *__mStrdup(const char *str)
{
	if(str)
		return strdup(str);
	else
		return NULL;
}

/** 文字列複製
 *
 * ptr に複製した文字列を格納し、文字の長さを返す。
 *
 * @param str  NULL の場合、*ptr には NULL、戻り値には 0 を返す
 * @return 文字の長さ */

int mStrdup2(const char *str,char **ptr)
{
	if(str)
	{
		*ptr = mStrdup(str);
		return strlen(str);
	}
	else
	{
		*ptr = NULL;
		return 0;
	}
}

/** 文字列複製
 *
 * *dst が NULL 以外なら、解放してから複製。\n
 * *dst に文字列ポインタが入る。
 *
 * @return *dst の値が返る */

char *mStrdup_ptr(char **dst,const char *src)
{
	mFree(*dst);

	return (*dst = mStrdup(src));
}

/** 文字列を指定長さ分複製 (非デバッグ) */

char *__mStrndup(const char *str,int len)
{
	if(str)
		return strndup(str, len);
	else
		return NULL;
}

/** 文字列を指定サイズに収めてコピー
 *
 * dst は必ず NULL で終わる。
 * 
 * @return コピーした文字数 */

int mStrcpy(char *dst,const char *src,int dstsize)
{
	int len = strlen(src);

	if(dstsize <= 0)
		return 0;
	else if(len < dstsize)
		memcpy(dst, src, len + 1);
	else
	{
		len = dstsize - 1;

		memcpy(dst, src, len);
		dst[len] = 0;
	}

	return len;
}

/** 文字列の長さ取得
 *
 * @param str NULL 可 */

int mStrlen(const char *str)
{
	if(str)
		return strlen(str);
	else
		return 0;
}

/* @} */

