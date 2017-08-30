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

#ifndef MLIB_UTIL_STDIO_H
#define MLIB_UTIL_STDIO_H

#ifdef __cplusplus
extern "C" {
#endif

FILE *mFILEopenUTF8(const char *filename,const char *mode);

mBool mFILEreadByte(FILE *fp,void *buf);
mBool mFILEread16LE(FILE *fp,void *buf);
mBool mFILEread32LE(FILE *fp,void *buf);
mBool mFILEread16BE(FILE *fp,void *buf);
mBool mFILEread32BE(FILE *fp,void *buf);
uint16_t mFILEget16LE(FILE *fp);
uint32_t mFILEget32LE(FILE *fp);
mBool mFILEreadCompareStr(FILE *fp,const char *text);
int mFILEreadStr_variableLen(FILE *fp,char **ppbuf);
int mFILEreadStr_len16BE(FILE *fp,char **ppbuf);
int mFILEreadArray16BE(FILE *fp,void *buf,int num);
int mFILEreadArray32BE(FILE *fp,void *buf,int num);
mBool mFILEreadArgsLE(FILE *fp,const char *format,...);
mBool mFILEreadArgsBE(FILE *fp,const char *format,...);

void mFILEwriteByte(FILE *fp,uint8_t val);
void mFILEwrite16LE(FILE *fp,uint16_t val);
void mFILEwrite32LE(FILE *fp,uint32_t val);
void mFILEwrite16BE(FILE *fp,uint16_t val);
void mFILEwrite32BE(FILE *fp,uint32_t val);
void mFILEwriteZero(FILE *fp,int size);
int mFILEwriteStr_variableLen(FILE *fp,const char *text,int len);
int mFILEwriteStr_len16BE(FILE *fp,const char *text,int len);
void mFILEwriteArray16BE(FILE *fp,void *buf,int num);
void mFILEwriteArray32BE(FILE *fp,void *buf,int num);

#ifdef __cplusplus
}
#endif

#endif
