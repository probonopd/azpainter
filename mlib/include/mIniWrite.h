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

#ifndef MLIB_INIWRITE_H
#define MLIB_INIWRITE_H

#ifdef __cplusplus
extern "C" {
#endif

FILE *mIniWriteOpenFile(const char *filename);
FILE *mIniWriteOpenFile2(const char *path,const char *filename);

void mIniWriteGroup(FILE *fp,const char *name);
void mIniWriteGroup_int(FILE *fp,int no);

void mIniWriteInt(FILE *fp,const char *key,int num);
void mIniWriteNoInt(FILE *fp,int keyno,int num);
void mIniWriteHex(FILE *fp,const char *key,uint32_t num);
void mIniWriteNoHex(FILE *fp,int keyno,uint32_t num);
void mIniWriteText(FILE *fp,const char *key,const char *text);
void mIniWriteNoText(FILE *fp,int keyno,const char *text);
void mIniWriteStr(FILE *fp,const char *key,mStr *str);
void mIniWriteNoStrs(FILE *fp,int keytop,mStr *array,int strnum);

void mIniWritePoint(FILE *fp,const char *key,mPoint *pt);
void mIniWriteSize(FILE *fp,const char *key,mSize *size);
void mIniWriteBox(FILE *fp,const char *key,mBox *box);

void mIniWriteNums(FILE *fp,const char *key,const void *buf,int numcnt,int bytes,mBool hex);
void mIniWriteBase64(FILE *fp,const char *key,const void *buf,int size);

void mIniWriteFileDialogConfig(FILE *fp,const char *key,const void *buf);

#ifdef __cplusplus
}
#endif

#endif
