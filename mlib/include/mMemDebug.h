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

#ifdef MLIB_MEMDEBUG

#ifndef MLIB_MEMDEBUG_H
#define MLIB_MEMDEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

void *mMemDebug_malloc(size_t,const char *,int);
void *mMemDebug_realloc(void *,size_t,const char *,int);
void mMemDebug_free(void *);
char *mMemDebug_strdup(const char *,const char *,int);
char *mMemDebug_strndup(const char *,size_t,const char *,int);

#undef strdup
#undef strndup

#define malloc(sz)        mMemDebug_malloc(sz, __FILE__, __LINE__)
#define realloc(ptr,sz)   mMemDebug_realloc(ptr, sz, __FILE__, __LINE__)
#define free(ptr)         mMemDebug_free(ptr)
#define strdup(str)       mMemDebug_strdup(str, __FILE__, __LINE__)
#define strndup(str,len)  mMemDebug_strndup(str, len, __FILE__, __LINE__)

#ifdef __cplusplus
}
#endif

#endif

#endif
