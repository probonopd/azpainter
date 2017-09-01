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
 * PNG 保存
 *****************************************/

#include <stdio.h>
#include <string.h>
#include <png.h>

#include "mDef.h"
#include "mSaveImage.h"


//--------------------

typedef struct
{
	mSaveImage *param;

	png_struct *png;
	png_info *pnginfo;

	FILE *fp;
	uint8_t *rowbuf,
		*palbuf;

	mBool start_image;
}SAVEPNG;

//--------------------



//=======================
// sub
//=======================


/** PNG エラー関数 */

static void _png_error_func(png_struct *png,png_const_charp mes)
{
	mSaveImage_setMessage((mSaveImage *)png_get_error_ptr(png), mes);

	longjmp(png_jmpbuf(png), 1);
}

/** PNG 警告関数 */

static void _png_warn_func(png_struct *png,png_const_charp mes)
{

}


//===========================
// 書き込みセット
//===========================


/** ヘッダ */

static void _set_header(SAVEPNG *p,mSaveImage *param)
{
	int type,bits;

	bits = param->sample_bits;

	//カラータイプ

	switch(param->coltype)
	{
		case MSAVEIMAGE_COLTYPE_PALETTE:
			type = PNG_COLOR_TYPE_PALETTE;
			bits = param->bits;
			break;
		case MSAVEIMAGE_COLTYPE_RGBA:
			type = PNG_COLOR_TYPE_RGB_ALPHA;
			break;
		case MSAVEIMAGE_COLTYPE_GRAY:
			type = PNG_COLOR_TYPE_GRAY;
			break;
		case MSAVEIMAGE_COLTYPE_GRAY_ALPHA:
			type = PNG_COLOR_TYPE_GRAY_ALPHA;
			break;
		default:
			type = PNG_COLOR_TYPE_RGB;
			break;
	}

	png_set_IHDR(p->png, p->pnginfo,
		param->width, param->height, bits, type,
		PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
}

/** パレット */

static int _set_palette(SAVEPNG *p)
{
	p->palbuf = mSaveImage_convertPalette_RGB(p->param);
	if(!p->palbuf) return MSAVEIMAGE_ERR_ALLOC;

	png_set_PLTE(p->png, p->pnginfo, (png_colorp)p->palbuf, p->param->palette_num);

	return 0;
}


//===========================
// main
//===========================


/** 初期化 */

static int _init(SAVEPNG *p)
{
	png_struct *png;
	png_info *pnginfo;

	//png_struct

	png = png_create_write_struct(PNG_LIBPNG_VER_STRING,
		p->param, _png_error_func, _png_warn_func);
	if(!png) return MSAVEIMAGE_ERR_ALLOC;

	p->png = png;

	//png_info

	pnginfo = png_create_info_struct(png);
	if(!pnginfo) return MSAVEIMAGE_ERR_ALLOC;

	p->pnginfo = pnginfo;

	//エラー時

	if(setjmp(png_jmpbuf(png)))
		return -1;

	//出力

	p->fp = mSaveImage_openFILE(&p->param->output);
	if(!p->fp) return MSAVEIMAGE_ERR_OPENFILE;

	png_init_io(png, p->fp);

	return 0;
}

/** イメージ前処理 */

static int _before_image(SAVEPNG *p)
{
	int ret,h,v;
	
	//IHDR
	
	_set_header(p, p->param);

	//パレット

	if(p->param->coltype == MSAVEIMAGE_COLTYPE_PALETTE)
	{
		if((ret = _set_palette(p)))
			return ret;
	}

	//解像度 (dpi or dpm)

	if(mSaveImage_getResolution_dpm(p->param, &h, &v))
		png_set_pHYs(p->png, p->pnginfo, h, v, PNG_RESOLUTION_METER);

	//他設定

	if(p->param->set_option)
	{
		if((ret = (p->param->set_option)(p->param)))
			return ret;
	}

	//書き込み

	png_write_info(p->png, p->pnginfo);

	return 0;
}

/** メイン処理 */

static int _main_proc(SAVEPNG *p)
{
	mSaveImage *param = p->param;
	int ret,i,pitch;

	//初期化

	if((ret = _init(p)))
		return ret;

	//イメージ前の処理

	if((ret = _before_image(p)))
		return ret;

	//Y1行バッファ確保

	pitch = png_get_rowbytes(p->png, p->pnginfo);

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

		png_write_row(p->png, (png_bytep)p->rowbuf);
	}

	return 0;
}

/** 解放 */

static void _png_free(SAVEPNG *p)
{
	if(p->png)
	{
		//イメージ書き込み終了
	
		if(p->start_image)
			png_write_end(p->png, NULL);
	
		//png_struct,png_info 削除

		png_destroy_write_struct(&p->png, (p->pnginfo)? &p->pnginfo: NULL);
	}

	if(p->fp)
	{
		if(p->param->output.type == MSAVEIMAGE_OUTPUT_TYPE_PATH)
			fclose(p->fp);
		else
			fflush(p->fp);
	}

	mFree(p->rowbuf);
	mFree(p->palbuf);
	mFree(p);
}


//===================


/**

@addtogroup saveimage
@{

*/


/** PNG ファイルに保存 */

mBool mSaveImagePNG(mSaveImage *param)
{
	SAVEPNG *p;
	int ret;

	//確保

	p = (SAVEPNG *)mMalloc(sizeof(SAVEPNG), TRUE);
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

	_png_free(p);

	return (ret == 0);
}

/** PNG 圧縮レベル設定 */

void mSaveImagePNG_setCompressionLevel(mSaveImage *p,int level)
{
	png_set_compression_level(((SAVEPNG *)p->internal)->png, level);
}

/** PNG 透過色セット
 *
 * パレットカラー、グレイスケール、RGB [8bit] 用 */

void mSaveImagePNG_setTransparent_8bit(mSaveImage *p,uint32_t col)
{
	SAVEPNG *pp = (SAVEPNG *)p->internal;
	uint8_t r,g,b,*trbuf,*ps;
	png_color_16 pngcol;
	int i,pnum;

	r = M_GET_R(col);
	g = M_GET_G(col);
	b = M_GET_B(col);

	switch(pp->param->coltype)
	{
		//RGB
		case MSAVEIMAGE_COLTYPE_RGB:
			pngcol.red = r;
			pngcol.green = g;
			pngcol.blue = b;
			
			png_set_tRNS(pp->png, pp->pnginfo, NULL, 0, &pngcol);
			break;
		//グレイスケール
		case MSAVEIMAGE_COLTYPE_GRAY:
			pngcol.gray = r;
			png_set_tRNS(pp->png, pp->pnginfo, NULL, 0, &pngcol);
			break;
		//パレット
		case MSAVEIMAGE_COLTYPE_PALETTE:
			pnum = pp->param->palette_num;
			
			trbuf = (uint8_t *)mMalloc(pnum, FALSE);
			if(!trbuf) return;

			memset(trbuf, 255, pnum);
			
			ps = pp->param->palette_buf;

			for(i = 0; i < pnum && !(ps[0] == r && ps[1] == g && ps[2] == b); i++, ps += 4);

			if(i < pnum)
			{
				trbuf[i] = 0;
				png_set_tRNS(pp->png, pp->pnginfo, trbuf, i + 1, 0);
			}

			mFree(trbuf);
			break;
	}
}

/* @} */
