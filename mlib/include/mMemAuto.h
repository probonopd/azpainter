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

#ifndef MLIB_MEMAUTO_H
#define MLIB_MEMAUTO_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _mMemAuto
{
	uint8_t *buf;
	uintptr_t allocsize,curpos,appendsize;
}mMemAuto;


void mMemAutoInit(mMemAuto *p);
void mMemAutoFree(mMemAuto *p);
mBool mMemAutoAlloc(mMemAuto *p,uintptr_t initsize,uintptr_t appendsize);

void mMemAutoReset(mMemAuto *p);
void mMemAutoBack(mMemAuto *p,int size);
uintptr_t mMemAutoGetRemain(mMemAuto *p);
uint8_t *mMemAutoGetBottom(mMemAuto *p);

void mMemAutoCutCurrent(mMemAuto *p);

mBool mMemAutoAppend(mMemAuto *p,void *dat,uintptr_t size);
mBool mMemAutoAppendByte(mMemAuto *p,uint8_t dat);
mBool mMemAutoAppendZero(mMemAuto *p,int size);

#ifdef __cplusplus
}
#endif

#endif
