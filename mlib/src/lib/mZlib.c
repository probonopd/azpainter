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
 * mZlib
 *****************************************/

#include <stdio.h>
#include <string.h>
#include <zlib.h>

#include "mDef.h"
#include "mFile.h"
#include "mZlib.h"


//-----------------------

typedef struct _mZlibEncode
{
	z_stream z;
	FILE *fp;
	uint8_t *buf;
	int bufsize;
	uint32_t writesize;
}mZlibEncode;

typedef struct _mZlibDecode
{
	z_stream z;
	FILE *fp;
	uint8_t *buf;
	int bufsize;
	uint32_t insize;
}mZlibDecode;

//-----------------------



/**
@defgroup zlib mZlib
@brief zlib ユーティリティ関数

@ingroup group_lib
 
@{
@file mZlib.h
*/


//===========================
// sub
//===========================


/** 圧縮初期化
 *
 * @param memlevel 0 以下で deflateInit() を使う */

static mBool _init_encode(uint8_t **ppbuf,int bufsize,z_stream *z,
	int level,int windowbits,int memlevel,int strategy)
{
	int ret;

	//作業用バッファ

	*ppbuf = (uint8_t *)mMalloc(bufsize, FALSE);
	if(!(*ppbuf)) return FALSE;

	//zlib 初期化

	memset(z, 0, sizeof(z_stream));

	if(memlevel <= 0)
		ret = deflateInit(z, level);
	else
		ret = deflateInit2(z, level, Z_DEFLATED, windowbits, memlevel, strategy);

	if(ret == Z_OK)
		return TRUE;
	else
	{
		mFree(*ppbuf);
		return FALSE;
	}
}

/** 展開初期化 */

static mBool _init_decode(uint8_t **ppbuf,int bufsize,z_stream *z,int windowbits)
{
	int ret;

	//作業用バッファ

	*ppbuf = (uint8_t *)mMalloc(bufsize, FALSE);
	if(!(*ppbuf)) return FALSE;

	//zlib 初期化

	memset(z, 0, sizeof(z_stream));

	if(windowbits)
		ret = inflateInit2(z, windowbits);
	else
		ret = inflateInit(z);

	if(ret == Z_OK)
		return TRUE;
	else
	{
		mFree(*ppbuf);
		return FALSE;
	}
}


//=================================
// mZlibEncode : 圧縮
//=================================



/** 書き込み処理 */

static mBool _encode_write(mZlibEncode *p,int size)
{
	if(p->fp)
		return (fwrite(p->buf, 1, size, p->fp) == size);
	else
		return FALSE;
}

/** 解放 */

void mZlibEncodeFree(mZlibEncode *p)
{
	if(p)
	{
		deflateEnd(&p->z);
	
		mFree(p->buf);
		mFree(p);
	}
}

/** 作成
 *
 * @param bufsize 圧縮用バッファのサイズ (0 以下で自動)
 * @param level   圧縮率 (0-9)
 * @param windowbits [8-15] zlib 形式 [+16] gzip 形式 [-8...-15] ヘッダなし zlib
 * @param memlevel  1-9 (default: 8)
 * @param strategy  圧縮タイプ (default: 0) */

mZlibEncode *mZlibEncodeNew(int bufsize,
	int level,int windowbits,int memlevel,int strategy)
{
	mZlibEncode *p;

	if(bufsize <= 0) bufsize = 8 * 1024;

	p = (mZlibEncode *)mMalloc(sizeof(mZlibEncode), TRUE);
	if(!p) return NULL;

	if(!_init_encode(&p->buf, bufsize, &p->z, level, windowbits, memlevel, strategy))
	{
		mFree(p);
		return NULL;
	}

	//

	p->bufsize = bufsize;

	p->z.next_out  = p->buf;
	p->z.avail_out = bufsize;

	return p;
}

/** 作成 (シンプル) */

mZlibEncode *mZlibEncodeNew_simple(int bufsize,int level)
{
	return mZlibEncodeNew(bufsize, level, 0, -1, 0);
}

/** ファイルポインタ (FILE *) をセット */

void mZlibEncodeSetIO_stdio(mZlibEncode *p,void *fp)
{
	p->fp = (FILE *)fp;
}

/** 書き込まれたサイズを取得 */

uint32_t mZlibEncodeGetWriteSize(mZlibEncode *p)
{
	return p->writesize;
}

/** リセットする */

mBool mZlibEncodeReset(mZlibEncode *p)
{
	p->z.next_out  = p->buf;
	p->z.avail_out = p->bufsize;
	p->writesize = 0;

	return (deflateReset(&p->z) == Z_OK);
}

/** データを送る */

mBool mZlibEncodeSend(mZlibEncode *p,void *buf,int size)
{
	z_stream *z = &p->z;
	int ret,bufsize;

	bufsize = p->bufsize;

	z->next_in  = (unsigned char *)buf;
	z->avail_in = size;

	while(z->avail_in)
	{
		ret = deflate(z, Z_NO_FLUSH);
		if(ret != Z_OK && ret != Z_STREAM_END) return FALSE;

		//バッファが一杯なので書き出し

		if(z->avail_out == 0)
		{
			if(!_encode_write(p, bufsize)) return FALSE;

			z->next_out  = p->buf;
			z->avail_out = bufsize;

			p->writesize += bufsize;
		}
	}

	return TRUE;
}

/** 残りのデータを処理する */

mBool mZlibEncodeFlushEnd(mZlibEncode *p)
{
	z_stream *z = &p->z;
	int ret,bufsize;

	bufsize = p->bufsize;

	//残りデータの圧縮

	while(1)
	{
		ret = deflate(z, Z_FINISH);
		
		if(ret == Z_STREAM_END)
			break;
		else if(ret != Z_OK)
			return FALSE;

		//書き出し

		if(z->avail_out == 0)
		{
			if(!_encode_write(p, bufsize))
				return FALSE;

			z->next_out  = p->buf;
			z->avail_out = bufsize;

			p->writesize += bufsize;
		}
	}

	//バッファ残りの書き込み

	ret = bufsize - z->avail_out;

	if(ret)
	{
		if(!_encode_write(p, ret)) return FALSE;

		p->writesize += ret;
	}

	return TRUE;
}


//===========================
// mZlibDecode : 展開
//===========================


/** バッファサイズ分を読み込み */

static int _decode_read(mZlibDecode *p)
{
	int size;

	if(p->fp)
		size = fread(p->buf, 1, (p->insize < p->bufsize)? p->insize: p->bufsize, p->fp);
	else
		size = 0;

	if(size > 0)
		p->insize -= size;

	return size;
}

/** 解放 */

void mZlibDecodeFree(mZlibDecode *p)
{
	if(p)
	{
		inflateEnd(&p->z);
	
		mFree(p->buf);
		mFree(p);
	}
}

/** 作成
 *
 * @param windowbits 圧縮時と同じ (default: 15) */

mZlibDecode *mZlibDecodeNew(int bufsize,int windowbits)
{
	mZlibDecode *p;

	if(bufsize <= 0) bufsize = 8 * 1024;

	p = (mZlibDecode *)mMalloc(sizeof(mZlibDecode), TRUE);
	if(!p) return NULL;

	if(!_init_decode(&p->buf, bufsize, &p->z, windowbits))
	{
		mFree(p);
		return NULL;
	}

	p->bufsize = bufsize;

	return p;
}

/** ファイルポインタ (FILE *) をセット */

void mZlibDecodeSetIO_stdio(mZlibDecode *p,void *fp)
{
	p->fp = fp;
}

/** 入力データ (圧縮後のサイズ) をセット */

void mZlibDecodeSetInSize(mZlibDecode *p,uint32_t size)
{
	p->insize = size;
}

/** リセット */

void mZlibDecodeReset(mZlibDecode *p)
{
	inflateReset(&p->z);

	p->insize = 0;
}

/** 一回で最後まですべて展開
 *
 * @param insize 入力サイズ
 * @return エラーコード */

int mZlibDecodeReadOnce(mZlibDecode *p,void *buf,int bufsize,uint32_t insize)
{
	z_stream *z = &p->z;
	int n;

	p->insize = insize;

	z->next_out  = (unsigned char *)buf;
	z->avail_out = bufsize;

	while(1)
	{
		//ファイルからバッファへ読み込み

		if(z->avail_in == 0)
		{
			if(p->insize == 0) return MZLIBDEC_READ_INDATA;

			n = _decode_read(p);
			if(n <= 0) return MZLIBDEC_READ_INDATA;

			z->next_in  = p->buf;
			z->avail_in = n;
		}

		//展開

		n = inflate(z, Z_NO_FLUSH);

		if(n == Z_STREAM_END)
			break;
		else if(n != Z_OK)
			return MZLIBDEC_READ_ZLIB;
	}

	//念の為入力データを最後まで読み込む

	while(p->insize)
	{
		if(_decode_read(p) <= 0)
			return MZLIBDEC_READ_INDATA;
	}

	return MZLIBDEC_READ_OK;
}

/** 指定サイズ分を読み込み
 *
 * @return エラーコード */

int mZlibDecodeRead(mZlibDecode *p,void *buf,int size)
{
	z_stream *z = &p->z;
	int n;

	z->next_out  = (unsigned char *)buf;
	z->avail_out = size;

	while(z->avail_out)
	{
		//ファイルからバッファへ読み込み

		if(z->avail_in == 0)
		{
			if(p->insize == 0) return MZLIBDEC_READ_INDATA;

			n = _decode_read(p);
			if(n <= 0) return MZLIBDEC_READ_INDATA;

			z->next_in  = p->buf;
			z->avail_in = n;
		}

		//展開

		n = inflate(z, Z_NO_FLUSH);

		if(n == Z_STREAM_END)
			break;
		else if(n != Z_OK)
			return MZLIBDEC_READ_ZLIB;
	}

	return MZLIBDEC_READ_OK;
}

/** 読み込み終了
 *
 * mZlibDecodeRead() で複数回に分けて読み込んだ場合、実行すること。
 * 
 * @return エラーコード */

int mZlibDecodeReadEnd(mZlibDecode *p)
{
	//入力データが残っている場合、すべて読み込む
	/* Z_STREAM_END が来ていない場合、すべてのデータを展開済みでも、
	 * 数バイト残っている場合がある。 */

	while(p->insize)
	{
		if(_decode_read(p) <= 0)
			return MZLIBDEC_READ_INDATA;
	}

	return MZLIBDEC_READ_OK;
}


//===========================
// 単体関数
//===========================


/** ファイルから展開
 *
 * @param compsize 圧縮後のサイズ
 * @param bufsize  作業用バッファサイズ (0 で自動)
 * @param windowbits 0 で zlib 形式。-15 で生データ */

mBool mZlibDecodeFILE(mFile file,void *dstbuf,uint32_t dstsize,
	uint32_t compsize,uint32_t bufsize,int windowbits)
{
	z_stream z;
	uint8_t *buf;
	uint32_t remain;
	int ret;
	mBool result = FALSE;

	if(!bufsize) bufsize = 8 * 1024;

	//初期化

	if(!_init_decode(&buf, bufsize, &z, windowbits))
		return FALSE;

	//

	z.next_out  = (uint8_t *)dstbuf;
	z.avail_out = dstsize;

	remain = compsize;

	while(1)
	{
		//ファイルから読み込み

		if(z.avail_in == 0)
		{
			ret = mFileRead(file, buf, (remain < bufsize)? remain: bufsize);
			if(ret == 0) break;

			z.next_in  = buf;
			z.avail_in = ret;

			remain -= ret;
		}

		//展開

		ret = inflate(&z, Z_NO_FLUSH);

		if(ret == Z_STREAM_END)
		{
			result = TRUE;
			break;
		}
		else if(ret != Z_OK)
			break;
	}

	//終了

	inflateEnd(&z);

	mFree(buf);

	return result;
}


/** @} */
