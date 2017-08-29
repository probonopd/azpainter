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

#ifndef MLIB_FILE_H
#define MLIB_FILE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef int mFile;

#define MFILE_NONE  (-1)

enum MFILEOPEN_FLAGS
{
	MFILEOPEN_FILENAME_LOCALE = 1<<0
};

/*------------*/

mBool mFileClose(mFile file);
mBool mFileOpenRead(mFile *file,const char *filename,uint32_t flags);
mBool mFileOpenWrite(mFile *file,const char *filename,uint32_t flags);

uintptr_t mFileGetSize(mFile file);
uint64_t mFileGetSizeLong(mFile file);

void mFileSetPos(mFile file,uintptr_t pos);
void mFileSeekCur(mFile file,int seek);
void mFileSeekEnd(mFile file,int seek);

int mFileRead(mFile file,void *buf,int size);
mBool mFileReadSize(mFile file,void *buf,int size);
uint32_t mFileGet32LE(mFile file);
uint32_t mFileGet32BE(mFile file);
mBool mFileReadCompareText(mFile file,const char *text);

int mFileWrite(mFile file,const void *buf,int size);
mBool mFileWriteSize(mFile file,const void *buf,int size);

#ifdef __cplusplus
}
#endif

#endif
