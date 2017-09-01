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
 * BMP 保存
 *****************************************/

#include <stdio.h>

#include "mDef.h"
#include "mSaveImage.h"
#include "mUtilStdio.h"
#include "mUtil.h"


//--------------------

typedef struct
{
	mSaveImage *param;

	FILE *fp;
	uint8_t *rowbuf;

	int bits,
		pitch,			//4byte 境界の余白分あり
		pitch_nopad,	//余白なし
		infoheader_size;	//INFOHEADER サイズ
	uint8_t bottomup;	//ボトムアップ
}SAVEBMP;

//--------------------



//===========================
// 書き込み
//===========================


/** ファイルヘッダ書き込み */

static void _write_fileheader(SAVEBMP *p)
{
	uint8_t d[14];
	int headsize;

	headsize = 14 + p->infoheader_size;
	if(p->bits <= 8) headsize += p->param->palette_num << 2;

	mSetBufLE_args(d, "s444",
		"BM",
		headsize + p->pitch * p->param->height, //イメージサイズ
		0,
		headsize);	//イメージまでのオフセット

	fwrite(d, 1, 14, p->fp);
}

/** 情報ヘッダ書き込み */

static void _write_infoheader(SAVEBMP *p,mSaveImage *param)
{
	uint8_t d[40],
		bitfields[16] = {0,0,0xff,0, 0,0xff,0,0, 0xff,0,0,0, 0,0,0,0xff};
	int resoh,resov,height;

	//解像度 (dpm)

	if(!mSaveImage_getResolution_dpm(param, &resoh, &resov))
		resoh = resov = 0;

	//基本情報

	height = param->height;
	if(!p->bottomup) height = -height;

	mSetBufLE_args(d, "44422444444",
		p->infoheader_size,
		param->width,
		height,
		1,			//plane
		p->bits,	//ビット数
		(p->bits == 32)? 3: 0,		//圧縮形式 (アルファ値ありなら BI_BITFIELDS)
		p->pitch * param->height,	//イメージサイズ
		resoh,
		resov,
		(p->bits <= 8)? param->palette_num: 0,	//パレット数
		0);		//重要な色数

	fwrite(d, 1, 40, p->fp);

	//アルファチャンネルありなら ver4

	if(p->bits == 32)
	{
		//ビットフィールド (R,G,B,A)

		fwrite(bitfields, 1, 16, p->fp);

		//ほかは 0

		mFILEwriteZero(p->fp, 108 - 40 - 16);
	}
}

/** パレット書き込み */

static int _write_palette(SAVEBMP *p)
{
	int i,num;
	uint8_t *buf,*ps,*pd;

	num = p->param->palette_num;

	buf = (uint8_t *)mMalloc(num << 2, FALSE);
	if(!buf) return MSAVEIMAGE_ERR_ALLOC;

	//RGBA => BGRA に変換

	ps = p->param->palette_buf;
	pd = buf;

	for(i = num; i > 0; i--, ps += 4, pd += 4)
	{
		pd[0] = ps[2];
		pd[1] = ps[1];
		pd[2] = ps[0];
		pd[3] = 0;
	}

	fwrite(buf, 1, num << 2, p->fp);

	mFree(buf);

	return 0;
}


//===========================
// main
//===========================


/** RGB/RGBA のイメージ変換 */

static void _convert_image(SAVEBMP *p)
{
	uint8_t *pd = p->rowbuf,r,b;
	int i,bytes;

	bytes = p->bits >> 3;

	for(i = p->param->width; i > 0; i--, pd += bytes)
	{
		r = pd[0], b = pd[2];
		pd[0] = b, pd[2] = r;
	}
}

/** 初期化 */

static int _init(SAVEBMP *p,mSaveImage *param)
{
	//8bit 以外は非対応

	if(param->sample_bits != 8)
		return MSAVEIMAGE_ERR_FORMAT;

	//BMP ビット数

	if(param->coltype == MSAVEIMAGE_COLTYPE_PALETTE)
		p->bits = param->bits;
	else
		p->bits = (param->coltype == MSAVEIMAGE_COLTYPE_RGB)? 24: 32;

	//INFOHEADER サイズ (アルファ値ありなら ver 4)

	p->infoheader_size = (p->bits == 32)? 108: 40;

	//Y1列バイト数

	p->pitch_nopad = (param->width * p->bits + 7) >> 3;
	p->pitch = (p->pitch_nopad + 3) & (~3);

	//Y1行バッファ
	
	p->rowbuf = (uint8_t *)mMalloc(p->pitch, TRUE);
	if(!p->rowbuf) return MSAVEIMAGE_ERR_ALLOC;

	return 0;
}

/** メイン処理 */

static int _main_proc(SAVEBMP *p)
{
	mSaveImage *param = p->param;
	int ret,i,convert;

	//初期化

	if((ret = _init(p, param)))
		return ret;

	//開く

	p->fp = mSaveImage_openFILE(&param->output);
	if(!p->fp) return MSAVEIMAGE_ERR_OPENFILE;

	//設定

	if(p->param->set_option)
	{
		if((ret = (p->param->set_option)(p->param)))
			return ret;
	}

	//ヘッダ書き込み

	_write_fileheader(p);
	_write_infoheader(p, param);

	if(p->bits <= 8)
	{
		if((ret = _write_palette(p)))
			return ret;
	}

	//イメージ書き込み

	convert = (!param->send_raw_format && p->bits >= 24);

	for(i = param->height; i > 0; i--)
	{
		//取得
		
		ret = (param->send_row)(param, p->rowbuf, p->pitch_nopad);
		if(ret) return ret;

		//変換 (RGB/RGBA の場合は R,B の値を交換)

		if(convert)
			_convert_image(p);

		//書き込み

		fwrite(p->rowbuf, 1, p->pitch, p->fp);
	}

	return 0;
}


/** BMP ファイルに保存
 *
 * デフォルトでトップダウン。
 * 
 * @ingroup saveimage */

mBool mSaveImageBMP(mSaveImage *param)
{
	SAVEBMP *p;
	int ret;

	//確保

	p = (SAVEBMP *)mMalloc(sizeof(SAVEBMP), TRUE);
	if(!p) return FALSE;

	p->param = param;

	param->internal = p;

	//処理

	ret = _main_proc(p);

	if(ret)
	{
		if(ret > 0)
			mSaveImage_setMessage_errno(param, ret);
	}

	//解放

	if(p->fp)
	{
		if(param->output.type == MSAVEIMAGE_OUTPUT_TYPE_PATH)
			fclose(p->fp);
		else
			fflush(p->fp);
	}

	mFree(p->rowbuf);
	mFree(p);

	return (ret == 0);
}

/** Y の書き込み順をボトムアップにする
 *
 * send_row() に渡す順番も下→上にすること。 */

void mSaveImageBMP_setBottomUp(mSaveImage *p)
{
	((SAVEBMP *)p->internal)->bottomup = TRUE;
}
