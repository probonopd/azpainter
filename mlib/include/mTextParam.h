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

#ifndef MLIB_TEXTPARAM_H
#define MLIB_TEXTPARAM_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _mTextParam mTextParam;

void mTextParamFree(mTextParam *p);
mTextParam *mTextParamCreate(const char *text,char split,char splitparam);

mBool mTextParamGetInt(mTextParam *p,const char *key,int *dst);
mBool mTextParamGetInt_range(mTextParam *p,const char *key,int *dst,int min,int max);
mBool mTextParamGetDouble(mTextParam *p,const char *key,double *dst);
mBool mTextParamGetDoubleInt(mTextParam *p,const char *key,int *dst,int dig);
mBool mTextParamGetDoubleInt_range(mTextParam *p,const char *key,int *dst,int dig,int min,int max);
mBool mTextParamGetText_raw(mTextParam *p,const char *key,char **dst);
mBool mTextParamGetText_dup(mTextParam *p,const char *key,char **buf);
mBool mTextParamGetStr(mTextParam *p,const char *key,mStr *str);
int mTextParamFindText(mTextParam *p,const char *key,const char *words,mBool bNoCase);

#ifdef __cplusplus
}
#endif

#endif
