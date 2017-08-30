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

#ifndef MLIB_ZLIB_H
#define MLIB_ZLIB_H

#include "mFile.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _mZlibEncode mZlibEncode;
typedef struct _mZlibDecode mZlibDecode;

#define MZLIBDEC_WINDOWBITS_ZLIB     15
#define MZLIBDEC_WINDOWBITS_NOHEADER -15

enum
{
	MZLIBDEC_READ_OK,
	MZLIBDEC_READ_ZLIB,
	MZLIBDEC_READ_INDATA
};


void mZlibEncodeFree(mZlibEncode *p);
mZlibEncode *mZlibEncodeNew(int bufsize,int level,int windowbits,int memlevel,int strategy);
mZlibEncode *mZlibEncodeNew_simple(int bufsize,int level);
void mZlibEncodeSetIO_stdio(mZlibEncode *p,void *fp);
mBool mZlibEncodeSend(mZlibEncode *p,void *buf,int size);
mBool mZlibEncodeFlushEnd(mZlibEncode *p);
mBool mZlibEncodeReset(mZlibEncode *p);
uint32_t mZlibEncodeGetWriteSize(mZlibEncode *p);

void mZlibDecodeFree(mZlibDecode *p);
mZlibDecode *mZlibDecodeNew(int bufsize,int windowbits);
void mZlibDecodeSetIO_stdio(mZlibDecode *p,void *fp);
void mZlibDecodeSetInSize(mZlibDecode *p,uint32_t size);
void mZlibDecodeReset(mZlibDecode *p);
int mZlibDecodeReadOnce(mZlibDecode *p,void *buf,int bufsize,uint32_t insize);
int mZlibDecodeRead(mZlibDecode *p,void *buf,int size);
int mZlibDecodeReadEnd(mZlibDecode *p);

mBool mZlibDecodeFILE(mFile file,void *dstbuf,uint32_t dstsize,
	uint32_t compsize,uint32_t bufsize,int windowbits);

#ifdef __cplusplus
}
#endif

#endif
