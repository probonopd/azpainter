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
 * mIOFileBuf
 *****************************************/

#include <stdio.h>
#include <string.h>

#include "mDef.h"
#include "mIOFileBuf.h"
#include "mUtilStdio.h"


//----------------

struct _mIOFileBuf
{
	FILE *fp;
	const uint8_t *buftop,	//先頭バッファ位置
		*bufcur;			//現在のバッファ位置
	size_t bufsize,			//バッファサイズ
		bufremain,			//残りバッファサイズ
		readsize;			//読み込んだサイズ
	int need_close,  		//fp を閉じるか (fp を指定した場合は行わない)
		endian;
};

//----------------


//=====================

static mIOFileBuf *_create()
{
	mIOFileBuf *p;
	
	p = (mIOFileBuf *)mMalloc(sizeof(mIOFileBuf), TRUE);
	if(p)
		p->endian = MIOFILEBUF_ENDIAN_SYSTEM;

	return p;
}

//=====================


/**************//**

@defgroup iofilebuf mIOFileBuf
@brief ファイルまたはバッファを対象とした入出力
@ingroup group_etc
 
@{
@file mIOFileBuf.h

********************/


/** 閉じる */

void mIOFileBuf_close(mIOFileBuf *p)
{
	if(p)
	{
		if(p->fp && p->need_close)
			fclose(p->fp);
	
		mFree(p);
	}
}

/** 開く (ファイル名から) */

mIOFileBuf *mIOFileBuf_openRead_filename(const char *filename)
{
	mIOFileBuf *p;

	if(!filename) return NULL;

	if((p = _create()))
	{
		p->fp = mFILEopenUTF8(filename, "rb");
		if(!p->fp)
		{
			mFree(p);
			return NULL;
		}

		p->need_close = 1;
	}

	return p;
}

/** 開く (バッファから) */

mIOFileBuf *mIOFileBuf_openRead_buf(const void *buf,uint32_t bufsize)
{
	mIOFileBuf *p;

	if((p = _create()))
	{
		p->buftop = p->bufcur = (const uint8_t *)buf;
		p->bufsize = p->bufremain = bufsize;
	}

	return p;
}

/** 開く (FILE* から)
 *
 * fp は開いたままとなる。 */

mIOFileBuf *mIOFileBuf_openRead_FILE(void *fp)
{
	mIOFileBuf *p;

	if((p = _create()))
		p->fp = (FILE *)fp;

	return p;
}

/** エンディアンタイプセット */

void mIOFileBuf_setEndian(mIOFileBuf *p,int type)
{
	p->endian = type;
}

/** 読み込み
 *
 * @return 読み込んだバイト数 */

int mIOFileBuf_read(mIOFileBuf *p,void *buf,int size)
{
	if(p->fp)
		return fread(buf, 1, size, p->fp);
	else
	{
		if(p->bufremain < size)
			size = p->bufremain;

		memcpy(buf, p->bufcur, size);

		p->bufcur += size;
		p->bufremain -= size;

		return size;
	}
}

/** 指定サイズ分を読み込んで捨てる
 *
 * パイプでつながった標準入力の場合はシークができないので、読み込む。 */

mBool mIOFileBuf_readEmpty(mIOFileBuf *p,int size)
{
	uint8_t d[128];
	int n;

	while(size > 0)
	{
		n = (size < 128)? size: 128;

		if(mIOFileBuf_read(p, d, n) < n)
			return FALSE;

		size -= n;
	}

	return TRUE;
}

/** 読み込み
 *
 * @return サイズ分を読み込めなかった場合、FALSE */

mBool mIOFileBuf_readSize(mIOFileBuf *p,void *buf,int size)
{
	return (mIOFileBuf_read(p, buf, size) == size);
}

/** 1バイト読み込み */

mBool mIOFileBuf_readByte(mIOFileBuf *p,void *buf)
{
	return (mIOFileBuf_read(p, buf, 1) == 1);
}

/** 16bit 数値読み込み */

mBool mIOFileBuf_read16(mIOFileBuf *p,void *buf)
{
	uint8_t d[2];

	if(!mIOFileBuf_readSize(p, d, 2))
		return FALSE;

	switch(p->endian)
	{
		case MIOFILEBUF_ENDIAN_LITTLE:
			*((uint16_t *)buf) = (d[1] << 8) | d[0];
			break;
		case MIOFILEBUF_ENDIAN_BIG:
			*((uint16_t *)buf) = (d[0] << 8) | d[1];
			break;
		default:
			*((uint16_t *)buf) = *((uint16_t *)d);
			break;
	}

	return TRUE;
}

/** 32bit 数値読み込み */

mBool mIOFileBuf_read32(mIOFileBuf *p,void *buf)
{
	uint8_t d[4];

	if(!mIOFileBuf_readSize(p, d, 4))
		return FALSE;

	switch(p->endian)
	{
		case MIOFILEBUF_ENDIAN_LITTLE:
			*((uint32_t *)buf) = ((uint32_t)d[3] << 24) | (d[2] << 16) | (d[1] << 8) | d[0];
			break;
		case MIOFILEBUF_ENDIAN_BIG:
			*((uint32_t *)buf) = ((uint32_t)d[0] << 24) | (d[1] << 16) | (d[2] << 8) | d[3];
			break;
		default:
			*((uint32_t *)buf) = *((uint32_t *)d);
			break;
	}

	return TRUE;
}

/** 位置をセット */

void mIOFileBuf_setPos(mIOFileBuf *p,uint32_t pos)
{
	if(p->fp)
		fseek(p->fp, pos, SEEK_SET);
	else
	{
		if(pos > p->bufsize) pos = p->bufsize;

		p->bufcur = p->buftop + pos;
		p->bufremain = p->bufsize - pos;
	}
}

/** 現在位置から移動 */

mBool mIOFileBuf_seekCur(mIOFileBuf *p,int seek)
{
	if(p->fp)
	{
		return (fseek(p->fp, seek, SEEK_CUR) == 0);
	}
	else
	{
		size_t pos;

		pos = p->bufcur - p->buftop;

		if(seek < 0 && pos < -seek)
			pos = 0;
		else if(seek > 0 && p->bufremain < seek)
			pos += p->bufremain;
		else
			pos += seek;

		p->bufcur = p->buftop + pos;
		p->bufremain = p->bufsize - pos;

		return TRUE;
	}
}

/** @} */
