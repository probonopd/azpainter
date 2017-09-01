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
 * JPEG 保存
 *****************************************/

#include <stdio.h>
#include <setjmp.h>
#include <jpeglib.h>

#include "mDef.h"
#include "mSaveImage.h"


//--------------------
/* エラー用 */

typedef struct
{
	struct jpeg_error_mgr jerr;
	jmp_buf jmpbuf;
	struct _SAVEJPEG *savejpg;
}_JPEG_ERR;

//--------------------

typedef struct _SAVEJPEG
{
	mSaveImage *param;

	struct jpeg_compress_struct jpg;
	_JPEG_ERR jpgerr;

	FILE *fp;
	uint8_t *rowbuf;

	mBool start_image;
}SAVEJPEG;

//--------------------


//=======================
// sub
//=======================


/** エラー終了時 */

static void _jpeg_error_exit(j_common_ptr p)
{
	_JPEG_ERR *err = (_JPEG_ERR *)p->err;
	char msg[JMSG_LENGTH_MAX];

	//エラー文字列

	(err->jerr.format_message)(p, msg);

	mSaveImage_setMessage(err->savejpg->param, msg);

	//setjmp の位置に飛ぶ

	longjmp(err->jmpbuf, 1);
}

/** メッセージ表示 */

static void _jpeg_output_message(j_common_ptr p)
{

}


//=======================
// main
//=======================


/** 初期化 */

static int _init(SAVEJPEG *p,mSaveImage *param)
{
	j_compress_ptr jpg;
	mBool grayscale;
	int h,v,ret;

	jpg = &p->jpg;

	//エラー設定
	/* デフォルトではエラー時にプロセスが終了するので、longjmp を行うようにする */

	jpg->err = jpeg_std_error(&p->jpgerr.jerr);

	p->jpgerr.jerr.error_exit = _jpeg_error_exit;
	p->jpgerr.jerr.output_message = _jpeg_output_message;
	p->jpgerr.savejpg = p;

	//エラー時

	if(setjmp(p->jpgerr.jmpbuf))
		return -1;

	//構造体初期化

	jpeg_create_compress(jpg);

	//出力

	p->fp = mSaveImage_openFILE(&param->output);
	if(!p->fp) return MSAVEIMAGE_ERR_OPENFILE;

	jpeg_stdio_dest(jpg, p->fp);

	//----------

	//基本情報

	grayscale = (param->coltype == MSAVEIMAGE_COLTYPE_GRAY);

	jpg->image_width = param->width;
	jpg->image_height = param->height;
	jpg->input_components = (grayscale)? 1: 3;
	jpg->in_color_space = (grayscale)? JCS_GRAYSCALE: JCS_RGB;

	jpeg_set_defaults(jpg);

	//解像度 (JFIF) dpi

	if(mSaveImage_getResolution_dpi(param, &h, &v))
	{
		jpg->density_unit = 1;
		jpg->X_density = h;
		jpg->Y_density = v;
	}

	//ハフマンテーブル最適化

	jpg->optimize_coding = TRUE;

	//他設定

	if(param->set_option)
	{
		if((ret = (param->set_option)(param)))
			return ret;
	}

	//開始

	jpeg_start_compress(jpg, TRUE);

	return 0;
}

/** メイン処理 */

static int _main_proc(SAVEJPEG *p)
{
	mSaveImage *param = p->param;
	int ret,i,pitch;

	//初期化

	if((ret = _init(p, param)))
		return ret;

	//Y1行バッファ確保

	pitch = param->width;
	if(param->coltype == MSAVEIMAGE_COLTYPE_RGB) pitch *= 3;

	p->rowbuf = (uint8_t *)mMalloc(pitch, FALSE);
	if(!p->rowbuf) return MSAVEIMAGE_ERR_ALLOC;

	//イメージ書き込み

	p->start_image = TRUE;

	for(i = param->height; i > 0; i--)
	{
		//取得
		
		ret = (param->send_row)(param, p->rowbuf, pitch);
		if(ret) return ret;

		//書き込み

		jpeg_write_scanlines(&p->jpg, (JSAMPARRAY)&p->rowbuf, 1);
	}

	return 0;
}

/** 解放 */

static void _jpeg_free(SAVEJPEG *p)
{
	if(p->start_image)
		jpeg_finish_compress(&p->jpg);

	jpeg_destroy_compress(&p->jpg);

	if(p->fp)
	{
		if(p->param->output.type == MSAVEIMAGE_OUTPUT_TYPE_PATH)
			fclose(p->fp);
		else
			fflush(p->fp);
	}

	mFree(p->rowbuf);
	mFree(p);
}


//======================


/**

@addtogroup saveimage
@{

*/


/** JPEG ファイルに保存 */

mBool mSaveImageJPEG(mSaveImage *param)
{
	SAVEJPEG *p;
	int ret;

	//確保

	p = (SAVEJPEG *)mMalloc(sizeof(SAVEJPEG), TRUE);
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

	_jpeg_free(p);

	return (ret == 0);
}

/** JPEG 品質セット
 *
 * @param quality 0-100 */

void mSaveImageJPEG_setQuality(mSaveImage *p,int quality)
{
	jpeg_set_quality(&((SAVEJPEG *)p->internal)->jpg, quality, TRUE);
}

/** JPEG プログレッシブにする */

void mSaveImageJPEG_setProgression(mSaveImage *p)
{
	jpeg_simple_progression(&((SAVEJPEG *)p->internal)->jpg);
}

/** サンプリング比セット
 *
 * @param factor 444, 422, 420, 411 */

void mSaveImageJPEG_setSamplingFactor(mSaveImage *p,int factor)
{
	j_compress_ptr jpg = &((SAVEJPEG *)p->internal)->jpg;
	int h,v;

	h = v = 1;

	switch(factor)
	{
		case 422: //Y 2:1
			h = 2;
			break;
		case 420: //Y 2:2
			h = v = 2;
			break;
		case 411: //Y 4:1
			h = 4;
			break;
	}

	jpg->comp_info[0].h_samp_factor = h;
	jpg->comp_info[0].v_samp_factor = v;
}

/* @} */
