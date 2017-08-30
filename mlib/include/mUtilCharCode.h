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

#ifndef MLIB_UTIL_CHARCODE_H
#define MLIB_UTIL_CHARCODE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* UTF-8 */

int mUTF8CharWidth(const char *p);
int mUTF8ToUCS4Char(const char *src,int maxlen,uint32_t *dst,const char **ppnext);
int mUTF8ToUCS4(const char *src,int srclen,uint32_t *dst,int dstlen);
uint32_t *mUTF8ToUCS4_alloc(const char *src,int srclen,int *retlen);
int mUTF8ToWide(const char *src,int srclen,wchar_t *dst,int dstlen);
wchar_t *mUTF8ToWide_alloc(const char *src,int srclen,int *retlen);
int mUTF8ToLocal(const char *src,int srclen,char *dst,int dstlen);
char *mUTF8ToLocal_alloc(const char *src,int srclen,int *retlen);

/* WideChar */

int mWideToUTF8(const wchar_t *src,int srclen,char *dst,int dstlen);
char *mWideToUTF8_alloc(const wchar_t *src,int srclen,int *retlen);

/* Locale */

int mLocalToWide(const char *src,int srclen,wchar_t *dst,int dstlen);
wchar_t *mLocalToWide_alloc(const char *src,int srclen,int *retlen);
char *mLocalToUTF8_alloc(const char *src,int srclen,int *retlen);

/* UCS-4 */

int mUCS4Len(const uint32_t *p);
uint32_t *mUCS4StrDup(const uint32_t *src);
int mUCS4ToUTF8Char(uint32_t ucs,char *dst);
int mUCS4ToUTF8(const uint32_t *src,int srclen,char *dst,int dstlen);
char *mUCS4ToUTF8_alloc(const uint32_t *ucs,int srclen,int *retlen);
wchar_t *mUCS4ToWide_alloc(const uint32_t *src,int srclen,int *retlen);
int mUCS4ToFloatInt(const uint32_t *text,int dig);
int mUCS4Compare(const uint32_t *text1,const uint32_t *text2);

/* UTF-16 */

int mUTF16Len(const uint16_t *p);
int mUTF16ToUCS4Char(const uint16_t *src,uint32_t *dst,const uint16_t **ppnext);
int mUTF16ToUTF8(const uint16_t *src,int srclen,char *dst,int dstlen);
char *mUTF16ToUTF8_alloc(const uint16_t *src,int srclen,int *retlen);

#ifdef __cplusplus
}
#endif

#endif
