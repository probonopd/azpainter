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

/*****************************************
 * <Linux> mFile [ファイル]
 *****************************************/

#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "mDef.h"
#include "mFile.h"

#include "mUtilCharCode.h"



/********************//**

@defgroup file mFile
@brief OS 依存の関数を使ったファイル処理

@ingroup group_system
@{

@file mFile.h
@enum MFILEOPEN_FLAGS

@def MFILE_NONE
ファイルが開けなかった時の mFile の値

*************************/


/** 閉じる */

mBool mFileClose(mFile file)
{
	if(file == MFILE_NONE)
		return TRUE;
	else
		return (close(file) == 0);
}

/** 読み込み用に開く
 *
 * @param filename デフォルトで UTF-8 */

mBool mFileOpenRead(mFile *file,const char *filename,uint32_t flags)
{
	char *fname;
	int fd;

	if(flags & MFILEOPEN_FILENAME_LOCALE)
		fd = open(filename, O_RDONLY);
	else
	{
		fname = mUTF8ToLocal_alloc(filename, -1, NULL);
		if(!fname) return FALSE;

		fd = open(fname, O_RDONLY);

		mFree(fname);
	}

	*file = fd;

	return (fd != -1);
}

/** 書き込み用に開く
 *
 * @param filename UTF-8 */

mBool mFileOpenWrite(mFile *file,const char *filename,uint32_t flags)
{
	char *fname;
	int fd;

	if(flags & MFILEOPEN_FILENAME_LOCALE)
		fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0644);
	else
	{
		fname = mUTF8ToLocal_alloc(filename, -1, NULL);
		if(!fname) return FALSE;

		fd = open(fname, O_CREAT | O_WRONLY | O_TRUNC, 0644);

		mFree(fname);
	}

	*file = fd;

	return (fd != -1);
}

/** サイズ取得 */

uintptr_t mFileGetSize(mFile file)
{
	struct stat st;

	if(fstat(file, &st) == 0)
		return (uintptr_t)st.st_size;
	else
		return 0;
}

/** サイズ取得 (64bit) */

uint64_t mFileGetSizeLong(mFile file)
{
	struct stat st;

	if(fstat(file, &st) == 0)
		return st.st_size;
	else
		return 0;
}

/** 位置をセット */

void mFileSetPos(mFile file,uintptr_t pos)
{
	lseek(file, pos, SEEK_SET);
}

/** カレント位置からシーク */

void mFileSeekCur(mFile file,int seek)
{
	lseek(file, seek, SEEK_CUR);
}

/** 終端位置からシーク */

void mFileSeekEnd(mFile file,int seek)
{
	lseek(file, seek, SEEK_END);
}


//=============================
// 読み込み
//=============================


/** 読み込み
 *
 * 実際に読み込んだサイズは、size 以下の場合あり。
 *
 * @return 実際に読み込んだサイズ */

int mFileRead(mFile file,void *buf,int size)
{
	ssize_t ret;

	do{
		ret = read(file, buf, size);
	}while(ret < 0 && errno == EINTR);

	return (int)ret;
}

/** 読み込み
 *
 * 確実に size 分を読み込む。
 *
 * @return すべて読み込めたか */

mBool mFileReadSize(mFile file,void *buf,int size)
{
	ssize_t ret;
	int readsize = 0;

	while(readsize < size)
	{
		ret = read(file, (uint8_t *)buf + readsize, size - readsize);

		if(ret < 0)
		{
			if(errno == EINTR) continue;
			break;
		}
		else
			readsize += ret;
	}

	return (readsize == size);
}

/** 32bit 値 (LE) 読み込み */

uint32_t mFileGet32LE(mFile file)
{
	uint8_t v[4];

	if(!mFileReadSize(file, v, 4)) return 0;

	return ((uint32_t)v[3] << 24) | (v[2] << 16) | (v[1] << 8) | v[0];
}

/** 32bit 値 (BE) 読み込み */

uint32_t mFileGet32BE(mFile file)
{
	uint8_t v[4];

	if(!mFileReadSize(file, v, 4)) return 0;

	return ((uint32_t)v[0] << 24) | (v[1] << 16) | (v[2] << 8) | v[3];
}

/** 文字列を読み込み、比較 */

mBool mFileReadCompareText(mFile file,const char *text)
{
	char *buf;
	int len;
	mBool ret = FALSE;

	len = strlen(text);

	buf = (char *)mMalloc(len + 1, FALSE);
	if(!buf) return FALSE;

	if(!mFileReadSize(file, buf, len))
		goto END;

	buf[len] = 0;

	ret = (strcmp(buf, text) == 0);
	
END:
	mFree(buf);

	return ret;
}


//=============================
// 書き込み
//=============================


/** 書き込み
 *
 * @return 実際に書き込んだサイズ (size より小さい場合あり) */

int mFileWrite(mFile file,const void *buf,int size)
{
	ssize_t ret;

	do{
		ret = write(file, buf, size);
	}while(ret < 0 && errno == EINTR);

	return (int)ret;
}

/** size 分を確実に書き込み */

mBool mFileWriteSize(mFile file,const void *buf,int size)
{
	ssize_t ret;
	int wsize = 0;

	while(wsize < size)
	{
		ret = write(file, (const uint8_t *)buf + wsize, size - wsize);

		if(ret < 0)
		{
			if(errno == EINTR) continue;
			break;
		}
		else
			wsize += ret;
	}

	return (wsize == size);
}

/** @} */
