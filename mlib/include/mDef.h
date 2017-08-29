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

#ifndef MLIB_DEF_H
#define MLIB_DEF_H

#include <stdint.h>

#include "mConfig.h"

#ifndef MLIB_NO_GUI
#include "mDefGui.h"
#endif

typedef uint8_t mBool;
typedef struct _mStr  mStr;
typedef struct _mList mList;
typedef struct _mFont mFont;
typedef void (*mDefEmptyFunc)();

/* macro */

#undef NULL
#undef TRUE
#undef FALSE

#define NULL  ((void *)0)
#define TRUE  (1)
#define FALSE (0)
#define M_MATH_PI       (3.14159265358979323846)

#define M_MAKE_DW2(hi,low)   ((uint32_t)(hi) << 16) | (low))
#define M_MAKE_DW4(a,b,c,d)  (((uint32_t)(a) << 24) | ((b) << 16) | ((c) << 8) | (d))
#define M_RGB(r,g,b)         (((r) << 16) | ((g) << 8) | (b))
#define M_RGBA(r,g,b,a)      (((uint32_t)(a) << 24) | ((r) << 16) | ((g) << 8) | (b))
#define M_GET_R(c)           (((c) >> 16) & 0xff)
#define M_GET_G(c)           (((c) >> 8) & 0xff)
#define M_GET_B(c)           ((c) & 0xff)
#define M_GET_A(c)           ((c) >> 24)
#define M_FREE_NULL(p)       mFree(p); p = NULL
#define M_BUF_UINT16(p)      (*((uint16_t *)(p)))
#define M_BUF_UINT32(p)      (*((uint32_t *)(p)))
#define M_FLAG_ON(v,f)       v |= f
#define M_FLAG_OFF(v,f)      v &= ~(f)

/* struct */

typedef struct _mPoint
{
	int x,y;
}mPoint;

typedef struct _mDoublePoint
{
	double x,y;
}mDoublePoint;

typedef struct _mSize
{
	int w,h;
}mSize;

typedef struct _mRect
{
	int x1,y1,x2,y2;
}mRect;

typedef struct _mBox
{
	int x,y,w,h;
}mBox;

typedef struct _mBuf
{
	void *buf;
	uintptr_t size;
}mBuf;

/* function */

#ifdef __cplusplus
extern "C" {
#endif

void mDebug(const char *format,...);

void mFree(void *ptr);
void *__mMalloc(uint32_t size,mBool clear);
void *__mRealloc(void *ptr,uint32_t size);
char *__mStrdup(const char *str);
char *__mStrndup(const char *str,int len);
int mStrdup2(const char *str,char **ptr);
char *mStrdup_ptr(char **dst,const char *src);
int mStrcpy(char *dst,const char *src,int dstsize);
int mStrlen(const char *str);
void *mMemdup(const void *src,uint32_t size);
void mMemzero(void *buf,int size);

#ifdef MLIB_MEMDEBUG

void *__mMalloc_debug(uint32_t size,mBool clear,const char *filename,int line);
void *__mRealloc_debug(void *ptr,uint32_t size,const char *filename,int line);
char *__mStrdup_debug(const char *str,const char *filename,int line);
char *__mStrndup_debug(const char *str,int len,const char *filename,int line);

#define mMalloc(size,clear)  __mMalloc_debug(size, clear, __FILE__, __LINE__)
#define mRealloc(ptr,size)   __mRealloc_debug(ptr, size, __FILE__, __LINE__)
#define mStrdup(str)         __mStrdup_debug(str, __FILE__, __LINE__)
#define mStrndup(str,len)    __mStrndup_debug(str, len, __FILE__, __LINE__)

#else

#define mMalloc(size,clear)  __mMalloc(size, clear)
#define mRealloc(ptr,size)   __mRealloc(ptr, size)
#define mStrdup(str)         __mStrdup(str)
#define mStrndup(str,len)    __mStrndup(str, len)

#endif /* MLIB_MEMDEBUG */

#ifdef __cplusplus
}
#endif

#endif
