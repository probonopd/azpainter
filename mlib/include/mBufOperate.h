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

#ifndef MLIB_BUFOPERATE_H
#define MLIB_BUFOPERATE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _mBufOperate
{
	uint8_t *cur,*top;
	int32_t size;
	uint8_t endian;
}mBufOperate;

enum MBUFOPERATE_ENDIAN
{
	MBUFOPERATE_ENDIAN_SYSTEM,
	MBUFOPERATE_ENDIAN_LITTLE,
	MBUFOPERATE_ENDIAN_BIG
};


void mBufOpInit(mBufOperate *p,void *buf,int32_t size,uint8_t endian);

int32_t mBufOpGetRemain(mBufOperate *p);
mBool mBufOpIsRemain(mBufOperate *p,int32_t size);

mBool mBufOpSeek(mBufOperate *p,int32_t size);
mBool mBufOpSetPos(mBufOperate *p,int32_t pos);
int32_t mBufOpSetPosRet(mBufOperate *p,int32_t pos);

mBool mBufOpRead(mBufOperate *p,void *buf,int32_t size);
mBool mBufOpReadByte(mBufOperate *p,void *buf);
mBool mBufOpRead16(mBufOperate *p,void *buf);
mBool mBufOpRead32(mBufOperate *p,void *buf);
uint16_t mBufOpGet16(mBufOperate *p);
uint32_t mBufOpGet32(mBufOperate *p);

void mBufOpWrite(mBufOperate *p,const void *buf,int32_t size);
void mBufOpWrite16(mBufOperate *p,void *buf);
void mBufOpWrite32(mBufOperate *p,void *buf);
void mBufOpSet16(mBufOperate *p,uint16_t val);
void mBufOpSet32(mBufOperate *p,uint32_t val);

#ifdef __cplusplus
}
#endif

#endif
