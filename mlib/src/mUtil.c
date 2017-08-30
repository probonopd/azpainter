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
 * [ユーティリティ関数]
 *****************************************/

#include <string.h>
#include <stdarg.h>

#include "mDef.h"


/*******************//**

@defgroup util mUtil
@brief ユーティリティ関数

@ingroup group_util
 
@{
@file mUtil.h

**********************/


/** dots/meter を dots/inch に単位変換 */

int mDPMtoDPI(int dpm)
{
	return (int)(dpm * 0.0254 + 0.5);
}

/** dots/inch を dots/meter に単位変換 */

int mDPItoDPM(int dpi)
{
	return (int)(dpi / 0.0254 + 0.5);
}

/** 最初にビットが ON になる位置
 *
 * @retval -1 すべて 0 */

int mGetBitOnPos(uint32_t val)
{
	uint32_t f;
	int i;

	for(i = 0, f = 1; i < 32; i++, f <<= 1)
	{
		if(val & f) return i;
	}

	return -1;
}

/** 最初にビットが OFF になる位置
 *
 * @retval 32 すべて 1 */

int mGetBitOffPos(uint32_t val)
{
	uint32_t f;
	int i;

	for(i = 0, f = 1; i < 32; i++, f <<= 1)
	{
		if(!(val & f)) return i;
	}

	return 32;
}

/** 状態の変更
 *
 * @param type [0]OFF [正]ON [負]反転
 * @param curret_on  現在の状態が ON かどうか
 * @return TRUE で状態を変更する */

mBool mIsChangeState(int type,int current_on)
{
	if(type < 0) type = !current_on;

	return ((!type) != (!current_on));
}

/** ON/OFF 状態の変更
 *
 * @param type 操作タイプ。0 で OFF、正で ON、負で反転
 * @param current_on 現在の値 (0 で OFF、それ以外で ON)
 * @param ret 結果の値が入る。1 で ON、0 で OFF
 * @return 状態が変化したか */

mBool mGetChangeState(int type,int current_on,int *ret)
{
	if(type < 0) type = !current_on;

	*ret = (type != 0);

	return ((!type) != (!current_on));
}


//======================


/** バッファから 16bit 数値取得 (BE) */

uint16_t mGetBuf16BE(const void *buf)
{
	const uint8_t *ps = (const uint8_t *)buf;

	return (ps[0] << 8) | ps[1];
}

/** バッファから 32bit 数値取得 (BE) */

uint32_t mGetBuf32BE(const void *buf)
{
	const uint8_t *ps = (const uint8_t *)buf;

	return ((uint32_t)ps[0] << 24) | (ps[1] << 16) | (ps[2] << 8) | ps[3];
}

/** バッファから 16bit 数値取得 (LE) */

uint16_t mGetBuf16LE(const void *buf)
{
	const uint8_t *ps = (const uint8_t *)buf;

	return (ps[1] << 8) | ps[0];
}

/** バッファから 32bit 数値取得 (LE) */

uint32_t mGetBuf32LE(const void *buf)
{
	const uint8_t *ps = (const uint8_t *)buf;

	return ((uint32_t)ps[3] << 24) | (ps[2] << 16) | (ps[1] << 8) | ps[0];
}

/** バッファに 16bit 数値セット (BE) */

void mSetBuf16BE(uint8_t *buf,uint16_t val)
{
	buf[0] = (uint8_t)(val >> 8);
	buf[1] = (uint8_t)val;
}

/** バッファに 32bit 数値セット (BE) */

void mSetBuf32BE(uint8_t *buf,uint32_t val)
{
	buf[0] = (uint8_t)(val >> 24);
	buf[1] = (uint8_t)(val >> 16);
	buf[2] = (uint8_t)(val >> 8);
	buf[3] = (uint8_t)val;
}

/** バッファのデータを、指定エンディアンからシステムのエンディアンに変更
 *
 * @param endian  負の値でリトルエンディアン、正の値でビッグエンディアン。
 * @param pattern 各数値のバイト数を文字列として記述。4 or 2 で数値。それ以外はその分スキップ。 */

void mConvertEndianBuf(void *buf,int endian,const char *pattern)
{
	uint8_t *p = (uint8_t *)buf;
	uint16_t test = 0x1234;
	char c;

	//エンディアンが同じか

	if(endian < 0)
	{
		if(*((uint8_t *)&test) == 0x34) return;
	}
	else
	{
		if(*((uint8_t *)&test) == 0x12) return;
	}
	
	//

	while(*pattern)
	{
		c = *(pattern++);
		
		switch(c)
		{
			case '4':
				if(endian < 0)
					*((uint32_t *)p) = ((uint32_t)p[3] << 24) | (p[2] << 16) | (p[1] << 8) | p[0];
				else
					*((uint32_t *)p) = ((uint32_t)p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];

				p += 4;
				break;
			case '2':
				if(endian < 0)
					*((uint16_t *)p) = (p[1] << 8) | p[0];
				else
					*((uint16_t *)p) = (p[0] << 8) | p[1];

				p += 2;
				break;
			default:
				p += c - '0';
				break;
		}
	}
}

/** バッファにリトルエンディアンで複数データ書き込み
 *
 * @param format  "1","2","4","s" (文字列) */

void mSetBufLE_args(void *buf,const char *format,...)
{
	va_list ap;
	uint8_t *pd = (uint8_t *)buf;
	const char *str;
	char c;
	uint32_t u32;

	va_start(ap, format);

	for(; (c = *format); format++)
	{
		if(c == '4')
		{
			u32 = va_arg(ap, uint32_t);
			pd[0] = (uint8_t)u32;
			pd[1] = (uint8_t)(u32 >> 8);
			pd[2] = (uint8_t)(u32 >> 16);
			pd[3] = (uint8_t)(u32 >> 24);
			pd += 4;
		}
		else if(c == '2')
		{
			u32 = va_arg(ap, int);
			pd[0] = (uint8_t)u32;
			pd[1] = (uint8_t)(u32 >> 8);
			pd += 2;
		}
		else if(c == 's')
		{
			str = va_arg(ap, const char *);
			u32 = strlen(str);
			memcpy(pd, str, u32);
			pd += u32;
		}
		else
			*(pd++) = va_arg(ap, int);
	}

	va_end(ap);
}

/** バッファにビッグエンディアンで複数データ書き込み
 *
 * @param format  "1","2","4","s" (文字列) */

void mSetBufBE_args(void *buf,const char *format,...)
{
	va_list ap;
	uint8_t *pd = (uint8_t *)buf;
	const char *str;
	char c;
	uint32_t u32;

	va_start(ap, format);

	for(; (c = *format); format++)
	{
		if(c == '4')
		{
			u32 = va_arg(ap, uint32_t);
			pd[0] = (uint8_t)(u32 >> 24);
			pd[1] = (uint8_t)(u32 >> 16);
			pd[2] = (uint8_t)(u32 >> 8);
			pd[3] = (uint8_t)u32;
			pd += 4;
		}
		else if(c == '2')
		{
			u32 = va_arg(ap, int);
			pd[0] = (uint8_t)(u32 >> 8);
			pd[1] = (uint8_t)u32;
			pd += 2;
		}
		else if(c == 's')
		{
			str = va_arg(ap, const char *);
			u32 = strlen(str);
			memcpy(pd, str, u32);
			pd += u32;
		}
		else
			*(pd++) = va_arg(ap, int);
	}

	va_end(ap);
}


//================================
// Base64
//================================


/** エンコード後のサイズを取得 */

int mBase64GetEncodeSize(int size)
{
	return ((size + 2) / 3) * 4;
}

/** Base64 エンコード
 *
 * bufsize に余裕があれば、最後に 0 を追加する。
 *
 * @return エンコード後のサイズ。-1 でバッファが足りない。 */

int mBase64Encode(void *dst,int bufsize,const void *src,int size)
{
	char *pd;
	const uint8_t *ps;
	int i,shift,val,n,encsize = 0;
	char m[4];

	pd = (char *)dst;
	ps = (const uint8_t *)src;

	for(; size > 0; size -= 3, ps += 3, pd += 4)
	{
		//src 3byte

		if(size >= 3)
			val = (ps[0] << 16) | (ps[1] << 8) | ps[2];
		else if(size == 2)
			val = (ps[0] << 16) | (ps[1] << 8);
		else
			val = ps[0] << 16;

		//3byte => 4文字

		for(i = 0, shift = 18; i < 4; i++, shift -= 6)
		{
			n = (val >> shift) & 63;

			if(n < 26) n += 'A';
			else if(n < 52) n += 'a' - 26;
			else if(n < 62) n += '0' - 52;
			else if(n == 62) n = '+';
			else n = '/';

			m[i] = n;
		}

		//余りを '=' で埋める

		if(size == 1)
			m[2] = m[3] = '=';
		else if(size == 2)
			m[3] = '=';

		//出力

		if(encsize + 4 > bufsize) return -1;

		pd[0] = m[0];
		pd[1] = m[1];
		pd[2] = m[2];
		pd[3] = m[3];

		encsize += 4;
	}

	//0 を追加

	if(encsize < bufsize)
		*pd = 0;

	return encsize;
}

/** Base64 デコード
 *
 * @param len  src の文字数。負の値で NULL 文字まで。
 * @return デコード後のサイズ。-1 でエラー。 */

int mBase64Decode(void *dst,int bufsize,const char *src,int len)
{
	uint8_t *pd;
	int decsize = 0,i,val,shift,n,size;

	if(len < 0) len = strlen(src);

	pd = (uint8_t *)dst;

	while(len)
	{
		//4文字 => 24bit

		for(i = 0, val = 0, shift = 18; i < 4; i++, shift -= 6)
		{
			if(len == 0) return -1;
			
			n = *(src++);
			len--;

			if(n >= 'A' && n <= 'Z') n -= 'A';
			else if(n >= 'a' && n <= 'z') n = n - 'a' + 26;
			else if(n >= '0' && n <= '9') n = n - '0' + 52;
			else if(n == '+') n = 62;
			else if(n == '/') n = 63;
			else if(n == '=') n = 0;
			else return -1;

			val |= n << shift;
		}

		//サイズ
		/* 最後が ??== なら 1byte。???= なら 2byte */

		size = 3;

		if(len == 0 && src[-1] == '=')
			size = (src[-2] == '=')? 1: 2;

		//出力先が一杯

		if(decsize + size > bufsize) return -1;

		//24bit => 1-3byte

		*(pd++) = (uint8_t)(val >> 16);
		if(size >= 2) *(pd++) = (uint8_t)(val >> 8);
		if(size == 3) *(pd++) = (uint8_t)val;

		decsize += size;
	}

	return decsize;
}
  
/* @} */
