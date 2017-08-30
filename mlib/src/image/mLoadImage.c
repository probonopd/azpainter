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
 * mLoadImage
 *****************************************/

#include <string.h>

#include "mDef.h"
#include "mLoadImage.h"
#include "mIOFileBuf.h"
#include "mUtil.h"


//------------------------

static const char *g_err_message[] = {
	"open file error",
	"data is corrupted",
	"unsupported format",
	"invalid value",
	"memory allocation error",
	"decompress error",
	"image size is large",
	"none image"
};

//------------------------


/******************//**

@defgroup loadimage mLoadImage
@brief 画像読み込み

デフォルトで、常に 8bit イメージで読み込む。\n
読み込み時は、R-G-B-[A] のバイト順でバッファに並ぶ。\n

@ingroup group_image
@{

@file mLoadImage.h

@enum MLOADIMAGE_FORMAT
@enum MLOADIMAGE_FLAGS
@enum MLOADIMAGE_SOURCE_TYPE
@enum MLOADIMAGE_RESOLITION_UNIT
@enum MLOADIMAGE_COLTYPE
@enum MLOADIMAGE_CHECKFORMAT_FLAGS
@enum MLOADIMAGE_ERR

@struct mLoadImageSource
@struct mLoadImageInfo
@struct _mLoadImage

@var MLOADIMAGE_FLAGS::MLOADIMAGE_FLAGS_ALLOW_16BIT
1サンプルが 16bit のイメージの場合は 16bit で読み込む

**************************/


/** mLoadImage 確保
 *
 * @param size  sizeof(mLoadImage) 以下の場合は常に最低サイズ */

mLoadImage *mLoadImage_create(int size)
{
	mLoadImage *p;

	if(size < sizeof(mLoadImage))
		size = sizeof(mLoadImage);
	
	p = (mLoadImage *)mMalloc(size, TRUE);
	if(!p) return NULL;

	p->info.transparent_col = -1;

	return p;
}

/** mLoadImage 解放 */

void mLoadImage_free(mLoadImage *p)
{
	if(p)
	{
		mFree(p->message);
		mFree(p);
	}
}

/** エラーメッセージ文字列セット */

void mLoadImage_setMessage(mLoadImage *p,const char *mes)
{
	mFree(p->message);
	
	p->message = mStrdup(mes);
}

/** 共通エラーの番号からメッセージ文字列セット */

void mLoadImage_setMessage_errno(mLoadImage *p,int err)
{
	if(err > 0 && err < MLOADIMAGE_ERR_MAX)
		mLoadImage_setMessage(p, g_err_message[err - 1]);
}

/** エラー番号から文字列取得 */

const char *mLoadImage_getErrMessage(int err)
{
	return g_err_message[err - 1];
}

/** 出力フォーマットでの Y1行のバイト数を取得 */

int mLoadImage_getPitch(mLoadImage *p)
{
	switch(p->format)
	{
		case MLOADIMAGE_FORMAT_RGB:
			return p->info.width * 3;
		case MLOADIMAGE_FORMAT_RGBA:
			return p->info.width << 2;
		case MLOADIMAGE_FORMAT_RAW:
			return p->info.raw_pitch;
	}

	return 0;
}

/** 解像度を DPI 単位で取得
 *
 * @return DPI で取得できない場合 FALSE */

mBool mLoadImage_getResolution_dpi(mLoadImage *p,int *phorz,int *pvert)
{
	int h,v;

	h = p->info.resolution_horz;
	v = p->info.resolution_vert;

	switch(p->info.resolution_unit)
	{
		//dot/inch
		case MLOADIMAGE_RESOLITION_UNIT_DPI:
			*phorz = h;
			*pvert = v;
			return TRUE;
		//dot/meter
		case MLOADIMAGE_RESOLITION_UNIT_DPM:
			*phorz = mDPMtoDPI(h);
			*pvert = mDPMtoDPI(v);
			return TRUE;
		//dot/centimeter
		case MLOADIMAGE_RESOLITION_UNIT_DPCM:
			*phorz = mDPMtoDPI(h * 100);
			*pvert = mDPMtoDPI(v * 100);
			return TRUE;
		default:
			return FALSE;
	}
}

/** mLoadImage の情報から mIOFileBuf を読み込み用に開く */

mIOFileBuf *mLoadImage_openIOFileBuf(mLoadImageSource *src)
{
	mIOFileBuf *io = NULL;

	switch(src->type)
	{
		case MLOADIMAGE_SOURCE_TYPE_PATH:
			io = mIOFileBuf_openRead_filename(src->filename);
			break;
		case MLOADIMAGE_SOURCE_TYPE_BUF:
			io = mIOFileBuf_openRead_buf(src->buf, src->bufsize);
			break;
		case MLOADIMAGE_SOURCE_TYPE_STDIO:
			io = mIOFileBuf_openRead_FILE(src->fp);
			break;
	}

	return io;
}

/** ヘッダからフォーマットを判定して読み込み関数を取得
 *
 * @param flags 許可するフォーマットのフラグ
 * @return FALSE で非対応のフォーマット、または読み込みに失敗 */

mBool mLoadImage_checkFormat(mLoadImageSource *src,mLoadImageFunc *pfunc,uint32_t flags)
{
	mIOFileBuf *io;
	uint8_t d[8],png[8] = {0x89,0x50,0x4e,0x47,0x0d,0x0a,0x1a,0x0a};
	int size;
	mLoadImageFunc func = NULL;

	//ヘッダ読み込み

	io = mLoadImage_openIOFileBuf(src);
	if(!io) return FALSE;

	size = mIOFileBuf_read(io, d, 8);

	mIOFileBuf_close(io);

	//判定

	if((flags & MLOADIMAGE_CHECKFORMAT_F_BMP) && size >= 2 && d[0] == 'B' && d[1] == 'M')
		//BMP
		func = mLoadImageBMP;

	else if((flags & MLOADIMAGE_CHECKFORMAT_F_PNG) && size >= 8 && memcmp(d, png, 8) == 0)
		//PNG
		func = mLoadImagePNG;

	else if((flags & MLOADIMAGE_CHECKFORMAT_F_JPEG) && size >= 2 && d[0] == 0xff && d[1] == 0xd8)
		//JPEG
		func = mLoadImageJPEG;

#ifndef MLIB_NO_GIF_DECODE
	else if((flags & MLOADIMAGE_CHECKFORMAT_F_GIF) && size >= 3
		&& d[0] == 'G' && d[1] == 'I' && d[2] == 'F')
		//GIF
		func = mLoadImageGIF;
#endif

	*pfunc = func;

	return (func != NULL);
}

/** @} */
