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

#ifndef MLIB_GIFDECODE_H
#define MLIB_GIFDECODE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _mGIFDecode mGIFDecode;
typedef void (*mGIFDecodeErrorFunc)(mGIFDecode *,const char *,void *);

enum MGIFDECODE_FORMAT
{
	MGIFDECODE_FORMAT_RAW,
	MGIFDECODE_FORMAT_RGB,
	MGIFDECODE_FORMAT_RGBA
};


mGIFDecode *mGIFDecode_create();
void mGIFDecode_close(mGIFDecode *p);

mBool mGIFDecode_open_filename(mGIFDecode *p,const char *filename);
mBool mGIFDecode_open_buf(mGIFDecode *p,const void *buf,uint32_t bufsize);
mBool mGIFDecode_open_stdio(mGIFDecode *p,void *fp);

void mGIFDecode_setErrorFunc(mGIFDecode *p,mGIFDecodeErrorFunc func,void *param);

mBool mGIFDecode_readHeader(mGIFDecode *p,mSize *size);
uint8_t *mGIFDecode_getPalette(mGIFDecode *p,int *pnum);
int mGIFDecode_getTransparentColor(mGIFDecode *p);

int mGIFDecode_nextImage(mGIFDecode *p);
mBool mGIFDecode_getNextLine(mGIFDecode *p,void *buf,int format,mBool trans_to_a0);

#ifdef __cplusplus
}
#endif

#endif
