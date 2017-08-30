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
 * mMemAuto
 *****************************************/

#include <string.h>

#include "mDef.h"
#include "mMemAuto.h"


/*************//**

@defgroup memauto mMemAuto
@brief 自動拡張メモリバッファ
@ingroup group_data
 
@{

@file mMemAuto.h
@struct mMemAuto

******************/


//========================


/** サイズ追加 */

static mBool _resize_append(mMemAuto *p,uintptr_t size)
{
	uint8_t *newbuf;
	uintptr_t addsize,newsize;

	if(size == 0) return TRUE;
	if(!p->buf) return FALSE;

	//サイズ拡張

	addsize = p->curpos + size;

	if(addsize > p->allocsize)
	{
		newsize = p->allocsize + p->appendsize;

		if(newsize < addsize) newsize = addsize;

		newbuf = (uint8_t *)mRealloc(p->buf, newsize);
		if(!newbuf) return FALSE;

		p->buf = newbuf;
		p->allocsize = newsize;
	}

	return TRUE;
}


//========================


/** 構造体初期化 */

void mMemAutoInit(mMemAuto *p)
{
	p->buf = NULL;
	p->allocsize = p->curpos = 0;
}

/** 解放 */

void mMemAutoFree(mMemAuto *p)
{
	if(p && p->buf)
	{
		mFree(p->buf);

		p->buf = NULL;
		p->allocsize = p->curpos = 0;
	}
}

/** 初期確保
 *
 * @param initsize 初期確保サイズ
 * @param appendsize 自動拡張時に追加するサイズ */

mBool mMemAutoAlloc(mMemAuto *p,uintptr_t initsize,uintptr_t appendsize)
{
	mMemAutoFree(p);

	p->buf = (uint8_t *)mMalloc(initsize, FALSE);
	if(!p->buf) return FALSE;

	p->allocsize = initsize;
	p->curpos = 0;
	p->appendsize = appendsize;

	return TRUE;
}

/** データ追加の現在位置をリセット */

void mMemAutoReset(mMemAuto *p)
{
	p->curpos = 0;
}

/** データ追加位置を指定サイズ前に戻す */

void mMemAutoBack(mMemAuto *p,int size)
{
	if(size < p->curpos)
		p->curpos -= size;
	else
		p->curpos = 0;
}

/** データ追加の残りサイズ取得 */

uintptr_t mMemAutoGetRemain(mMemAuto *p)
{
	return p->allocsize - p->curpos;
}

/** 終端位置のポインタ取得 */

uint8_t *mMemAutoGetBottom(mMemAuto *p)
{
	return p->buf + p->curpos;
}

/** 現在の位置サイズでサイズ変更する */

void mMemAutoCutCurrent(mMemAuto *p)
{
	uint8_t *newbuf;

	if(p->curpos && p->curpos != p->allocsize)
	{
		newbuf = (uint8_t *)mRealloc(p->buf, p->curpos);
		if(!newbuf) return;

		p->buf = newbuf;
		p->allocsize = p->curpos;
	}
}

/** データを追加 */

mBool mMemAutoAppend(mMemAuto *p,void *dat,uintptr_t size)
{
	if(!dat) return TRUE;

	//サイズ拡張

	if(!_resize_append(p, size))
		return FALSE;

	//データ追加

	memcpy(p->buf + p->curpos, dat, size);

	p->curpos += size;

	return TRUE;
}

/** 1byte データ追加 */

mBool mMemAutoAppendByte(mMemAuto *p,uint8_t dat)
{
	return mMemAutoAppend(p, &dat, 1);
}

/** 指定サイズ分のゼロ値を追加 */

mBool mMemAutoAppendZero(mMemAuto *p,int size)
{
	if(!_resize_append(p, size))
		return FALSE;
	else
	{
		memset(p->buf + p->curpos, 0, size);
		p->curpos += size;

		return TRUE;
	}
}

/** @} */
