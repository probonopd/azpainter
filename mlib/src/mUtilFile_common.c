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
 * <共通> ファイル関連ユーティリティ
 *****************************************/

#include "mDef.h"

#include "mFile.h"
#include "mUtilFile.h"


/****************//**

@addtogroup util_file mUtilFile
@brief ファイル関連ユーティリティ

@ingroup group_util
@{

@file mUtilFile.h

**********************/


//============================
// 読み込み
//============================


/** ファイルの内容を、確保したメモリにすべて読み込む
 *
 * ファイルが空の場合、デフォルトでエラーになる。
 *
 * @param flags
 * - @b MREADFILEFULL_ADD_NULL : 最後に NULL 文字を追加
 * - @b MREADFILEFULL_ACCEPT_EMPTY : 内容が空の場合は確保サイズ=１で 0 のみ格納する */

mBool mReadFileFull(const char *filename,uint8_t flags,mBuf *dstbuf)
{
	mFile file;
	uintptr_t size,allocsize;
	char *buf = NULL;

	if(!mFileOpenRead(&file, filename,
		(flags & MREADFILEFULL_FILENAME_LOCALE)? MFILEOPEN_FILENAME_LOCALE: 0))
		return FALSE;

	//ファイルサイズ

	size = mFileGetSize(file);
	if(size == 0 && !(flags & MREADFILEFULL_ACCEPT_EMPTY)) goto ERR;

	//確保サイズ

	allocsize = size;

	if(size == 0 || (flags & MREADFILEFULL_ADD_NULL))
		allocsize++;

	//確保&読み込み

	buf = (char *)mMalloc(allocsize, FALSE);
	if(!buf) goto ERR;

	if(!mFileReadSize(file, buf, size)) goto ERR;

	//NULL 追加

	if(size != allocsize)
		*(buf + size) = 0;

	mFileClose(file);

	//

	dstbuf->buf  = buf;
	dstbuf->size = size;

	return TRUE;

ERR:
	if(buf) mFree(buf);
	mFileClose(file);
	return FALSE;
}

/** ファイルの先頭バイトを読み込み */

mBool mReadFileHead(const char *filename,void *buf,int size)
{
	mFile file;
	mBool ret;

	if(!mFileOpenRead(&file, filename, 0))
		return FALSE;

	ret = mFileReadSize(file, buf, size);

	mFileClose(file);

	return ret;
}

/** ファイルをコピー
 *
 * @param bufsize バッファサイズ。0 以下でデフォルト。 */

mBool mCopyFile(const char *src,const char *dst,int bufsize)
{
	mFile filesrc = MFILE_NONE, filedst = MFILE_NONE;
	uint8_t *buf;
	uint64_t remain;
	int size;
	mBool ret = FALSE;

	if(bufsize <= 0) bufsize = 8192;

	//バッファ確保

	buf = (uint8_t *)mMalloc(bufsize, FALSE);
	if(!buf) return FALSE;

	//開く

	if(!mFileOpenRead(&filesrc, src, 0)) goto ERR;

	if(!mFileOpenWrite(&filedst, dst, 0)) goto ERR;

	//コピー

	remain = mFileGetSizeLong(filesrc);

	while(remain)
	{
		size = mFileRead(filesrc, buf, bufsize);
		if(size == 0) break;
	
		if(!mFileWriteSize(filedst, buf, size))
			goto ERR;

		remain -= size;
	}

	//

	ret = TRUE;

ERR:
	mFileClose(filesrc);
	mFileClose(filedst);
	mFree(buf);

	return ret;
}

/* @} */
