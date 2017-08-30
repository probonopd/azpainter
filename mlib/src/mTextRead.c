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
 * mTextRead
 *****************************************/

#include "mDef.h"

#include "mTextRead.h"
#include "mUtilFile.h"
#include "mUtilStr.h"


//-----------------

struct _mTextRead
{
	char *buf,*cur,*next;
	uint32_t size;
};

//-----------------


/**
@defgroup textread mTextRead
@brief テキストファイル読み込み
@ingroup group_etc
 
@{
@file mTextRead.h
*/


/** 終了 */

void mTextReadEnd(mTextRead *p)
{
	if(p)
	{
		mFree(p->buf);
		mFree(p);
	}
}

/** ファイルから読込 */

mTextRead *mTextRead_readFile(const char *filename)
{
	mTextRead *p;
	mBuf buf;

	//読み込み

	if(!mReadFileFull(filename, MREADFILEFULL_ADD_NULL | MREADFILEFULL_ACCEPT_EMPTY, &buf))
		return NULL;

	//

	p = (mTextRead *)mMalloc(sizeof(mTextRead), TRUE);
	if(!p)
		mFree(buf.buf);
	else
	{
		p->buf = p->cur = p->next = buf.buf;
		p->size = buf.size;
	}

	return p;
}

/** 一行読み込み
 *
 * バッファ内の改行を NULL に変換して、バッファ内のポインタを返す。 @n
 * 先頭から順に一度だけ読み込む場合に使う。 @n
 * 返ったポインタから NULL 文字までの範囲は内容を変更しても構わない。
 * 
 * @return バッファ内のポインタが返る。NULL で終了 */

char *mTextReadGetLine(mTextRead *p)
{
	if(!p->next)
		return NULL;
	else
	{
		p->cur = p->next;
		p->next = mGetStrNextLine(p->cur, TRUE);

		return p->cur;
	}
}

/** 一行読み込み (空行の場合はスキップ) */

char *mTextReadGetLine_skipEmpty(mTextRead *p)
{
	while(p->next)
	{
		p->cur = p->next;
		p->next = mGetStrNextLine(p->cur, TRUE);

		if(*(p->cur)) break;
	}

	return (p->next)? p->cur: NULL;
}

/** @} */
