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

#ifndef MLIB_INIREAD_H
#define MLIB_INIREAD_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _mIniRead mIniRead;

void mIniReadEnd(mIniRead *p);
mIniRead *mIniReadLoadFile(const char *filename);
mIniRead *mIniReadLoadFile2(const char *path,const char *filename);

void mIniReadEmpty(mIniRead *p);

mBool mIniReadSetGroup(mIniRead *p,const char *name);
mBool mIniReadSetGroup_int(mIniRead *p,int no);
const char *mIniReadSetFirstGroup(mIniRead *p);
const char *mIniReadSetNextGroup(mIniRead *p);
int mIniReadGetGroupItemNum(mIniRead *p);
mBool mIniReadIsHaveKey(mIniRead *p,const char *key);

mBool mIniReadGetNextItem(mIniRead *p,const char **key,const char **param);
mBool mIniReadGetNextItem_nonum32(mIniRead *p,int *keyno,void *buf,mBool hex);

int mIniReadInt(mIniRead *p,const char *key,int def);
uint32_t mIniReadHex(mIniRead *p,const char *key,uint32_t def);
const char *mIniReadText(mIniRead *p,const char *key,const char *def);
int mIniReadTextBuf(mIniRead *p,const char *key,char *buf,int size,const char *def);
mBool mIniReadStr(mIniRead *p,const char *key,mStr *str,const char *def);
mBool mIniReadNoStr(mIniRead *p,int keyno,mStr *str,const char *def);
void mIniReadNoStrs(mIniRead *p,int keytop,mStr *array,int maxnum);
int mIniReadNums(mIniRead *p,const char *key,void *buf,int bufnum,int bytes,mBool hex);
void *mIniReadNums_alloc(mIniRead *p,const char *key,int bytes,mBool hex,int append_bytes,int *psize);
mBool mIniReadPoint(mIniRead *p,const char *key,mPoint *pt,int defx,int defy);
mBool mIniReadSize(mIniRead *p,const char *key,mSize *size,int defw,int defh);
mBool mIniReadBox(mIniRead *p,const char *key,mBox *box,int defx,int defy,int defw,int defh);
void *mIniReadBase64(mIniRead *p,const char *key,int *psize);
mBool mIniReadFileDialogConfig(mIniRead *p,const char *key,void *val);

#ifdef __cplusplus
}
#endif

#endif
