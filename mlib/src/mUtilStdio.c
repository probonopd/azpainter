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
 * mUtilStdio
 *****************************************/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "mDef.h"

#include "mUtilCharCode.h"


/**************//**

@defgroup util_stdio mUtilStdio
@brief stdio のユーティリティ関数

@ingroup group_util
@{

@file mUtilStdio.h

******************/


/** UTF-8 ファイル名で開く */

FILE *mFILEopenUTF8(const char *filename,const char *mode)
{
	char *fname;
	FILE *fp;

	fname = mUTF8ToLocal_alloc(filename, -1, NULL);
	if(!fname) return FALSE;

	fp = fopen(fname, mode);

	mFree(fname);

	return fp;
}



//===========================
// 読み込み
//===========================


/** 1バイト読み込み */

mBool mFILEreadByte(FILE *fp,void *buf)
{
	return (fread(buf, 1, 1, fp) == 1);
}

/** 16bit値 (LE) 読み込み */

mBool mFILEread16LE(FILE *fp,void *buf)
{
	uint8_t v[2];

	if(fread(v, 1, 2, fp) < 2)
		return FALSE;
	else
	{
		*((uint16_t *)buf) = (v[1] << 8) | v[0];
		return TRUE;
	}
}

/** 32bit値 (LE) 読み込み */

mBool mFILEread32LE(FILE *fp,void *buf)
{
	uint8_t v[4];

	if(fread(v, 1, 4, fp) < 4)
		return FALSE;
	else
	{
		*((uint32_t *)buf) = ((uint32_t)v[3] << 24) | (v[2] << 16) | (v[1] << 8) | v[0];
		return TRUE;
	}
}

/** 16bit値 (BE) 読み込み */

mBool mFILEread16BE(FILE *fp,void *buf)
{
	uint8_t v[2];

	if(fread(v, 1, 2, fp) < 2)
		return FALSE;
	else
	{
		*((uint16_t *)buf) = (v[0] << 8) | v[1];
		return TRUE;
	}
}

/** 32bit値 (BE) 読み込み */

mBool mFILEread32BE(FILE *fp,void *buf)
{
	uint8_t v[4];

	if(fread(v, 1, 4, fp) < 4)
		return FALSE;
	else
	{
		*((uint32_t *)buf) = ((uint32_t)v[0] << 24) | (v[1] << 16) | (v[2] << 8) | v[3];
		return TRUE;
	}
}

/** 16bit値 (LE) 読み込み
 *
 * @return データが足りない場合 0 */

uint16_t mFILEget16LE(FILE *fp)
{
	uint16_t v = 0;

	mFILEread16LE(fp, &v);
	return v;
}

/** 32bit値 (LE) 読み込み
 *
 * @return データが足りない場合 0 */

uint32_t mFILEget32LE(FILE *fp)
{
	uint32_t v = 0;

	mFILEread32LE(fp, &v);
	return v;
}

/** 指定文字列数分を読み込み、文字列比較
 *
 * @return TRUE で一致 */

mBool mFILEreadCompareStr(FILE *fp,const char *text)
{
	int len = strlen(text);
	char *buf;
	mBool ret = FALSE;

	buf = (char *)mMalloc(len, FALSE);
	if(!buf) return FALSE;

	if(fread(buf, 1, len, fp) == len)
		ret = (strncmp(buf, text, len) == 0);

	mFree(buf);

	return ret;
}

/** 長さ(可変)+文字列から文字列を確保して読み込み
 *
 * @param ppbuf  確保された文字列ポインタが入る。NULL で空文字列。
 * @return 文字列の長さ。-1 でエラー、0 で空文字列。 */

int mFILEreadStr_variableLen(FILE *fp,char **ppbuf)
{
	int len,shift;
	uint8_t b;
	char *buf;

	*ppbuf = NULL;

	//長さ

	for(len = shift = 0; 1; shift += 7)
	{
		if(!mFILEreadByte(fp, &b)) return -1;

		len |= (b & 0x7f) << shift;

		if(b < 128) break;
	}

	//文字列

	if(len)
	{
		buf = (char *)mMalloc(len + 1, FALSE);
		if(!buf) return -1;

		if(fread(buf, 1, len, fp) != len)
		{
			mFree(buf);
			return -1;
		}

		buf[len] = 0;

		*ppbuf = buf;
	}

	return len;
}

/** 文字列を確保して読み込み (16bit BE 長さ + 文字列)
 *
 * @param ppbuf  確保された文字列ポインタが入る。NULL で空文字列。
 * @return 文字列の長さ。-1 でエラー、0 で空文字列。 */

int mFILEreadStr_len16BE(FILE *fp,char **ppbuf)
{
	uint16_t len;
	char *pc;

	//長さ

	if(!mFILEread16BE(fp, &len)) return -1;

	//文字列

	if(len == 0)
		*ppbuf = NULL;
	else
	{
		pc = (char *)mMalloc(len + 1, FALSE);
		if(!pc) return -1;

		if(fread(pc, 1, len, fp) != len)
		{
			mFree(pc);
			return -1;
		}

		pc[len] = 0;

		*ppbuf = pc;
	}

	return len;
}

/** 16bit 配列読み込み (BE)
 *
 * @return 実際に読み込んだ数 */

int mFILEreadArray16BE(FILE *fp,void *buf,int num)
{
	int size,i;
	uint16_t *pw = (uint16_t *)buf;
	uint8_t *pb = (uint8_t *)buf;

	//全体を読み込む

	size = fread(buf, 1, num << 1, fp);

	//システムのエンディアンに直す

	for(i = size >> 1; i; i--, pb += 2)
		*(pw++) = (pb[0] << 8) | pb[1];

	return size >> 1;
}

/** 32bit 配列読み込み (BE)
 *
 * @return 実際に読み込んだ数 */

int mFILEreadArray32BE(FILE *fp,void *buf,int num)
{
	uint32_t *pdw = (uint32_t *)buf;
	uint8_t *pb = (uint8_t *)buf;
	int size,i;

	//全体を読み込む

	size = fread(buf, 1, num << 2, fp);

	//BE -> システムのエンディアンに

	for(i = size >> 2; i; i--, pb += 4)
		*(pdw++) = ((uint32_t)pb[0] << 24) | (pb[1] << 16) | (pb[2] << 8) | pb[3];

	return size >> 2;
}

/** 可変引数で連続値を読み込み (LE)
 *
 * @param format バイト数
 * @return すべて読み込めたか */

mBool mFILEreadArgsLE(FILE *fp,const char *format,...)
{
	va_list ap;
	char c;
	void *buf;
	mBool ret = TRUE;

	va_start(ap, format);

	for(; (c = *format); format++)
	{
		buf = va_arg(ap, void *);
		
		if(c == '4')
			ret = mFILEread32LE(fp, buf);
		else if(c == '2')
			ret = mFILEread16LE(fp, buf);
		else
			ret = mFILEreadByte(fp, buf);

		if(!ret) break;
	}

	va_end(ap);

	return ret;
}

/** 可変引数で連続値を読み込み (BE)
 *
 * @param format バイト数
 * @return すべて読み込めたか */

mBool mFILEreadArgsBE(FILE *fp,const char *format,...)
{
	va_list ap;
	char c;
	void *buf;
	mBool ret = TRUE;

	va_start(ap, format);

	for(; (c = *format); format++)
	{
		buf = va_arg(ap, void *);
		
		if(c == '4')
			ret = mFILEread32BE(fp, buf);
		else if(c == '2')
			ret = mFILEread16BE(fp, buf);
		else
			ret = mFILEreadByte(fp, buf);

		if(!ret) break;
	}

	va_end(ap);

	return ret;
}


//===========================
// 書き込み
//===========================


/** 1バイト書き込み */

void mFILEwriteByte(FILE *fp,uint8_t val)
{
	fwrite(&val, 1, 1, fp);
}

/** 16bit 数値書き込み (LE) */

void mFILEwrite16LE(FILE *fp,uint16_t val)
{
	uint8_t b[2];

	b[0] = (uint8_t)val;
	b[1] = (uint8_t)(val >> 8);

	fwrite(b, 1, 2, fp);
}

/** 32bit 数値書き込み (LE) */

void mFILEwrite32LE(FILE *fp,uint32_t val)
{
	uint8_t b[4];

	b[0] = (uint8_t)val;
	b[1] = (uint8_t)(val >> 8);
	b[2] = (uint8_t)(val >> 16);
	b[3] = (uint8_t)(val >> 24);

	fwrite(b, 1, 4, fp);
}

/** 16bit 数値書き込み (BE) */

void mFILEwrite16BE(FILE *fp,uint16_t val)
{
	uint8_t b[2];

	b[0] = (uint8_t)(val >> 8);
	b[1] = (uint8_t)val;

	fwrite(b, 1, 2, fp);
}

/** 32bit 数値書き込み (BE) */

void mFILEwrite32BE(FILE *fp,uint32_t val)
{
	uint8_t b[4];

	b[0] = (uint8_t)(val >> 24);
	b[1] = (uint8_t)(val >> 16);
	b[2] = (uint8_t)(val >> 8);
	b[3] = (uint8_t)val;

	fwrite(b, 1, 4, fp);
}

/** ゼロを指定サイズ分書き込み */

void mFILEwriteZero(FILE *fp,int size)
{
	uint8_t buf[32];

	memset(buf, 0, 32);

	while(size >= 32)
	{
		fwrite(buf, 1, 32, fp);
		size -= 32;
	}

	if(size)
		fwrite(buf, 1, size, fp);
}

/** 文字列を、長さ(可変)＋文字列で出力
 *
 * 長さは、7bit が ON で次のバイトに続く
 *
 * @param len  負の値で自動
 * @return 書き込んだバイト数 */

int mFILEwriteStr_variableLen(FILE *fp,const char *text,int len)
{
	int val,cnt;

	if(!text)
		len = 0;
	else if(len < 0)
		len = strlen(text);

	if(len == 0)
	{
		//長さ = 0

		mFILEwriteByte(fp, 0);
		return 1;
	}
	else
	{
		for(val = len, cnt = 0; val; val >>= 7, cnt++)
			mFILEwriteByte(fp, (val < 128)? val: (val & 0x7f) | 0x80);

		fwrite(text, 1, len, fp);

		return cnt + len;
	}
}

/** 文字列書き込み (16bit BE 長さ + 文字列)
 *
 * @param len  負の値で自動
 * @return 書き込んだバイト数 */

int mFILEwriteStr_len16BE(FILE *fp,const char *text,int len)
{
	//長さ (2byte)

	if(!text)
		len = 0;
	else if(len < 0)
		len = strlen(text);

	if(len > 0xffff) len = 0xffff;

	mFILEwrite16BE(fp, len);

	//文字列

	if(len) fwrite(text, 1, len, fp);

	return 2 + len;
}

/** 16bit の配列書き込み (BE) */

void mFILEwriteArray16BE(FILE *fp,void *buf,int num)
{
	uint16_t *ps = (uint16_t *)buf;

	for(; num > 0; num--)
		mFILEwrite16BE(fp, *(ps++));
}

/** 32bit 配列書き込み (BE) */

void mFILEwriteArray32BE(FILE *fp,void *buf,int num)
{
	uint32_t *ps = (uint32_t *)buf;

	for(; num > 0; num--)
		mFILEwrite32BE(fp, *(ps++));
}

/** @} */
