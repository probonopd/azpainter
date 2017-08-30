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
 * mImageBuf 
 *****************************************/

#include <string.h>

#include "mDef.h"
#include "mImageBuf.h"
#include "mLoadImage.h"


/************//**

@defgroup imagebuf mImageBuf
@brief 24/32bit イメージバッファ

@details
- 「R-G-B-(A)」の順で、「上 -> 下」に向けて並んでいる。

@ingroup group_image
@{

@file mImageBuf.h
@struct _mImageBuf

@def MIMAGEBUF_GETBUFPT(p,x,y)
指定位置のバッファポインタ位置取得

****************/


/** 解放 */

void mImageBufFree(mImageBuf *p)
{
	if(p)
	{
		mFree(p->buf);
		mFree(p);
	}
}

/** 作成
 *
 * @param bpp 3 で RGB、4 で RGBA */

mImageBuf *mImageBufCreate(int w,int h,int bpp)
{
	mImageBuf *p;

	if(bpp != 3 && bpp != 4) return NULL;

	p = (mImageBuf *)mMalloc(sizeof(mImageBuf), TRUE);
	if(!p) return NULL;

	p->w = w;
	p->h = h;
	p->bpp = bpp;
	p->pitch = w * bpp;

	//イメージバッファ確保

	p->buf = (uint8_t *)mMalloc(p->pitch * h, FALSE);
	if(!p->buf)
	{
		mFree(p);
		return NULL;
	}

	return p;
}


//==============================
// 画像読み込み
//==============================


typedef struct
{
	mLoadImage base;
	mImageBuf *img;
	uint8_t *dstbuf;
	int bpp;
}LOADIMAGE_INFO;


/** 情報取得 */

static int _loadimage_getinfo(mLoadImage *load,mLoadImageInfo *info)
{
	LOADIMAGE_INFO *p = (LOADIMAGE_INFO *)load;

	p->img = mImageBufCreate(info->width, info->height, p->bpp);
	if(!p->img) return MLOADIMAGE_ERR_ALLOC;

	p->dstbuf = p->img->buf;
	if(info->bottomup) p->dstbuf += (info->height - 1) * p->img->pitch;

	return MLOADIMAGE_ERR_OK;
}

/** Y1行取得 */

static int _loadimage_getrow(mLoadImage *load,uint8_t *buf,int pitch)
{
	LOADIMAGE_INFO *p = (LOADIMAGE_INFO *)load;

	memcpy(p->dstbuf, buf, pitch);

	p->dstbuf += (load->info.bottomup)? -(p->img->pitch): p->img->pitch;

	return MLOADIMAGE_ERR_OK;
}

/** 画像読み込み
 *
 * @param loadfunc mLoadImageFunc
 * @param errmes   NULL 以外の場合、確保されたエラーメッセージ文字列が入る。 */

mImageBuf *mImageBufLoadImage(mLoadImageSource *src,mDefEmptyFunc loadfunc,int bpp,char **errmes)
{
	LOADIMAGE_INFO *p;
	mImageBuf *img;
	int ret;

	if(bpp != 3 && bpp != 4) return NULL;

	//mLoadImage

	p = (LOADIMAGE_INFO *)mLoadImage_create(sizeof(LOADIMAGE_INFO));
	if(!p) return NULL;

	p->base.format = (bpp == 3)? MLOADIMAGE_FORMAT_RGB: MLOADIMAGE_FORMAT_RGBA;
	p->base.src = *src;
	p->base.getinfo = _loadimage_getinfo;
	p->base.getrow = _loadimage_getrow;

	p->bpp = bpp;

	//読み込み関数

	ret = ((mLoadImageFunc)loadfunc)(M_LOADIMAGE(p));

	//結果

	img = p->img;

	if(!ret)
	{
		mImageBufFree(img);
		img = NULL;

		if(errmes) *errmes = mStrdup(p->base.message);
	}

	mLoadImage_free(M_LOADIMAGE(p));

	return img;
}


/** @} */
