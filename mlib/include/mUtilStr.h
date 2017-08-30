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

#ifndef MLIB_UTIL_STR_H
#define MLIB_UTIL_STR_H

#ifdef __cplusplus
extern "C" {
#endif

void mFreeStrsBuf(char **buf);

char *mStrchr_end(const char *text,char ch);
int mStrcmp_endchar(const char *text1,const char *text2,char ch);
int mStrcmp_number(const char *text1,const char *text2);

mBool mGetStrNextSplit(const char **pptop,const char **ppend,char ch);
int mGetStrLines(const char *text);
char *mGetStrNextLine(char *buf,mBool repnull);

char *mGetSplitCharReplaceStr(const char *text,char split);

void mReverseBuf(char *buf,int len);
void mReplaceStrChar(char *text,char find,char rep);

mBool mIsASCIIString(const char *text);
mBool mIsNumString(const char *text);

int mGetStrVariableLenBytes(const char *text);

void mToLower(char *text);

int mIntToStr(char *buf,int num);
int mFloatIntToStr(char *buf,int num,int dig);
int mIntToStrDig(char *buf,int num,int dig);
int mUCS4StrToFloatInt(const uint32_t *text,int dig);

mBool mIsMatchString(const char *text,const char *pattern,mBool bNoCase);
mBool mIsMatchStringSum(const char *text,const char *pattern,char split,mBool bNoCase);
int mGetEqStringIndex(const char *text,const char *enumtext,char split,mBool bNoCase);

char *mGetFormatStrParam(const char *text,const char *key,char split,char paramsplit,mBool bNoCase);

#ifdef __cplusplus
}
#endif

#endif
