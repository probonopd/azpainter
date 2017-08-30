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

#ifndef MLIB_UTIL_H
#define MLIB_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

int mDPMtoDPI(int dpm);
int mDPItoDPM(int dpi);

int mGetBitOnPos(uint32_t val);
int mGetBitOffPos(uint32_t val);

mBool mIsChangeState(int type,int current_on);
mBool mGetChangeState(int type,int current_on,int *ret);

uint16_t mGetBuf16BE(const void *buf);
uint32_t mGetBuf32BE(const void *buf);
uint16_t mGetBuf16LE(const void *buf);
uint32_t mGetBuf32LE(const void *buf);
void mSetBuf16BE(uint8_t *buf,uint16_t val);
void mSetBuf32BE(uint8_t *buf,uint32_t val);
void mConvertEndianBuf(void *buf,int endian,const char *pattern);
void mSetBufLE_args(void *buf,const char *format,...);
void mSetBufBE_args(void *buf,const char *format,...);

int mBase64GetEncodeSize(int size);
int mBase64Encode(void *dst,int bufsize,const void *src,int size);
int mBase64Decode(void *dst,int bufsize,const char *src,int len);

#ifdef __cplusplus
}
#endif

#endif
