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
 * PNG 読み込み関数
 *****************************************/

#include <string.h>
#include <png.h>
/* setjmp.h はインクルードしない。コンパイルエラーが出る場合あり */

#include "mDef.h"
#include "mLoadImage.h"
#include "mIOFileBuf.h"


//--------------------

typedef struct
{
	mLoadImage *param;

	png_struct *png;
	png_info *pnginfo;

	mIOFileBuf *io;
	uint8_t *rowbuf,	//Y1行バッファ
		*imgbuf,		//インターレース時の画像バッファ
		*palbuf;		//パレットバッファ (生読み込み時のみ)

	int palnum;

	mBool interlace,
		start_image;
}_LOADPNG;

//--------------------

#define RET_OK        0
#define RET_ERR_NOMES -1

//--------------------



//=======================
// sub
//=======================


/** PNG エラー関数 */

static void _png_error_func(png_struct *png,png_const_charp mes)
{
	mLoadImage_setMessage((mLoadImage *)png_get_error_ptr(png), mes);

	longjmp(png_jmpbuf(png), 1);
}

/** PNG 警告関数 */

static void _png_warn_func(png_struct *png,png_const_charp mes)
{

}

/** 読み込みコールバック */

static void _png_read_func(png_struct *png,png_byte *data,png_size_t size)
{
	_LOADPNG *p = (_LOADPNG *)png_get_io_ptr(png);

	if(!mIOFileBuf_readSize(p->io, data, size))
		png_error(png, mLoadImage_getErrMessage(MLOADIMAGE_ERR_CORRUPT));
}

/** 解像度取得 */

static void _get_resolution(_LOADPNG *p,mLoadImageInfo *info)
{
	png_uint_32 h,v;
	int type;

	info->resolution_unit = MLOADIMAGE_RESOLITION_UNIT_NONE;

	if(png_get_pHYs(p->png, p->pnginfo, &h, &v, &type))
	{
		if(type == PNG_RESOLUTION_METER)
			info->resolution_unit = MLOADIMAGE_RESOLITION_UNIT_DPM;
		else
			info->resolution_unit = MLOADIMAGE_RESOLITION_UNIT_UNKNOWN;

		info->resolution_horz = h;
		info->resolution_vert = v;
	}
}

/** 透過色取得 */

static int _get_transparent(_LOADPNG *p,int coltype)
{
	png_bytep buf;
	png_color_16p rgb;
	png_color *palbuf;
	int i,num,palnum,trans_index;
	uint16_t r,g,b;

	if(png_get_tRNS(p->png, p->pnginfo, &buf, &num, &rgb) == 0)
		return -1;
	else if(buf && num)
	{
		//パレット
		/* 最初にアルファ値が 0 になる色。
		 * 0,255 以外の値があった時、0 が複数あったときは透過色とみなさない。 */

		trans_index = -1;

		for(i = 0; i < num; i++, buf++)
		{
			if(*buf != 0 && *buf != 255)
				return -1;
			
			if(*buf == 0)
			{
				if(trans_index != -1) return -1;
				trans_index = i;
			}
		}

		if(trans_index != -1
			&& png_get_PLTE(p->png, p->pnginfo, &palbuf, &palnum)
			&& trans_index < palnum)
		{
			palbuf += trans_index;
			
			return (palbuf->red << 16) | (palbuf->green << 8) | palbuf->blue;
		}
	}
	else if(rgb)
	{
		//グレイスケール or RGB

		if(coltype == PNG_COLOR_TYPE_GRAY)
			r = g = b = rgb->gray;
		else
			r = rgb->red, g = rgb->green, b = rgb->blue;

		r >>= 8;
		g >>= 8;
		b >>= 8;

		return (r << 16) | (g << 8) | b;
	}

	return -1;
}

/** パレット読み込み */

static int _get_palette(_LOADPNG *p)
{
	int i,num;
	uint8_t *ps,*pd;
	png_color *srcbuf;
	
	if(png_get_PLTE(p->png, p->pnginfo, &srcbuf, &num) == 0)
	{
		mLoadImage_setMessage(p->param, "can not get palette");
		return RET_ERR_NOMES;
	}

	//確保

	p->palbuf = (uint8_t *)mMalloc(num << 2, FALSE);
	if(!p->palbuf) return MLOADIMAGE_ERR_ALLOC;

	p->palnum = num;

	//変換 (RGB => RGBA)

	ps = (uint8_t *)srcbuf;
	pd = p->palbuf;

	for(i = num; i > 0; i--, ps += 3, pd += 4)
	{
		pd[0] = ps[0];
		pd[1] = ps[1];
		pd[2] = ps[2];
		pd[3] = 255;
	}

	return RET_OK;
}

/** インターレース時、イメージ読み込み */

static int _read_interlace_image(_LOADPNG *p,mLoadImageInfo *info)
{
	uint8_t **pprowbuf,**pp,*prow;
	int i,pitch;

	pitch = info->raw_pitch;

	//バッファ確保

	p->imgbuf = (uint8_t *)mMalloc(pitch * info->height, FALSE);
	if(!p->imgbuf) return MLOADIMAGE_ERR_ALLOC;

	//ポインタ配列確保

	pprowbuf = (uint8_t **)mMalloc(sizeof(void*) * info->height, FALSE);
	if(!pprowbuf) return MLOADIMAGE_ERR_ALLOC;
	
	//読み込み位置セット

	pp = pprowbuf;
	prow = p->imgbuf;

	for(i = info->height; i > 0; i--, prow += pitch)
		*(pp++) = prow;

	//読み込み

	png_read_image(p->png, pprowbuf);
	png_read_end(p->png, NULL);

	mFree(pprowbuf);

	p->start_image = FALSE;

	return RET_OK;
}


//=======================
// 処理
//=======================


/** 読み込み設定 */

static int _setinfo(_LOADPNG *p,mLoadImageInfo *info)
{
	png_struct *png;
	png_info *pnginfo;
	png_uint_32 width,height,tRNS;
	int ret,depth,coltype,interlace;

	png = p->png;
	pnginfo = p->pnginfo;

	//----- 情報取得

	//IHDR チャンク取得

	if(!png_get_IHDR(png, pnginfo, &width, &height,
			&depth, &coltype, &interlace, NULL, NULL))
		return MLOADIMAGE_ERR_INVALID_VALUE;

	//最大サイズ

	if((p->param->max_width > 0 && width > p->param->max_width)
		|| (p->param->max_height > 0 && height > p->param->max_height))
		return MLOADIMAGE_ERR_LARGE_SIZE;

	//tRNS チャンクデータがあるか (なければ 0)

	tRNS = png_get_valid(png, pnginfo, PNG_INFO_tRNS);

	//透過色取得

	info->transparent_col = _get_transparent(p, coltype);

	//パレット

	if(p->param->format == MLOADIMAGE_FORMAT_RAW
		&& png_get_valid(png, pnginfo, PNG_INFO_PLTE))
	{
		ret = _get_palette(p);
		if(ret) return ret;
	}

	//----- 情報セット

	info->raw_sample_bits = (depth == 16)? 16: 8;

	//8bit 時のビット数

	if(depth <= 8)
	{
		if(coltype == PNG_COLOR_TYPE_RGB_ALPHA)
			ret = 32;
		else if(coltype == PNG_COLOR_TYPE_RGB)
			ret = 24;
		else
			ret = depth;

		info->raw_bits = ret;
	}

	//カラータイプ

	if(coltype == PNG_COLOR_TYPE_RGB)
		ret = MLOADIMAGE_COLTYPE_RGB;
	else if(coltype == PNG_COLOR_TYPE_RGB_ALPHA)
		ret = MLOADIMAGE_COLTYPE_RGBA;
	else if(coltype == PNG_COLOR_TYPE_GRAY)
		ret = MLOADIMAGE_COLTYPE_GRAY;
	else if(coltype == PNG_COLOR_TYPE_GRAY_ALPHA)
		ret = MLOADIMAGE_COLTYPE_GRAY_ALPHA;
	else
		ret = MLOADIMAGE_COLTYPE_PALETTE;

	info->raw_coltype = ret;
	
	//----- イメージ展開方法セット

	if(p->param->format != MLOADIMAGE_FORMAT_RAW)
	{
		/* パレットカラー => RGB/RGBA
		 * 8bit 未満のグレイスケール => 8bit グレイスケール
		 * 透過色をアルファ値に
		 */

		if(coltype == PNG_COLOR_TYPE_PALETTE
			|| (coltype == PNG_COLOR_TYPE_GRAY && depth < 8)
			|| tRNS)
			png_set_expand(png);
					
		//グレイスケール => RGB/RGBA
			
		if(coltype == PNG_COLOR_TYPE_GRAY
			|| coltype == PNG_COLOR_TYPE_GRAY_ALPHA)
			png_set_gray_to_rgb(png);

		//ビット深度

		if(depth <= 8)
			//8bit 以下は 8bit に
			depth = 8;
		else if(depth == 16 && !(p->param->flags & MLOADIMAGE_FLAGS_ALLOW_16BIT))
		{
			//16bit => 8bit
			png_set_strip_16(png);
			depth = 8;
		}

		//アルファ値

		if(p->param->format == MLOADIMAGE_FORMAT_RGBA)
		{
			if(info->transparent_col != -1
				&& (p->param->flags & MLOADIMAGE_FLAGS_LEAVE_TRANSPARENT))
				//1色での透過色ありで、透過色を透明にしない場合
			{
				png_set_strip_alpha(png);
				png_set_add_alpha(png, 0xff, PNG_FILLER_AFTER);
			}
			else if(!(coltype & PNG_COLOR_MASK_ALPHA) && !tRNS)
			{
				//アルファ値がない場合は追加
				
				png_set_add_alpha(png,
					(depth == 16)? 0xffff: 0xff, PNG_FILLER_AFTER);
			}
		}
		else
		{
			//RGB で出力 (アルファ値削除)

			if((coltype & PNG_COLOR_MASK_ALPHA) || tRNS)
				png_set_strip_alpha(png);
		}
	}

	//インターレース処理

	if(interlace == PNG_INTERLACE_ADAM7)
		png_set_interlace_handling(png);

	//----- 反映させる

	png_read_update_info(png, pnginfo);

	//----- 情報セット

	info->width  = width;
	info->height = height;
	info->sample_bits = (depth <= 8)? 8: 16;
	info->raw_pitch = png_get_rowbytes(png, pnginfo);

	p->interlace = (interlace == PNG_INTERLACE_ADAM7);

	//----- インターレースの場合、全体を読み込む

	if(p->interlace)
		return _read_interlace_image(p, info);
	else
		return RET_OK;
}

/** イメージ部分まで読み込み */

static int _read_headers(_LOADPNG *p)
{
	png_struct *png;
	png_info *pnginfo;
	uint8_t sig[8];

	//------- libpng 初期化

	//png_struct 作成

	png = png_create_read_struct(PNG_LIBPNG_VER_STRING,
			p->param, _png_error_func, _png_warn_func);

	if(!png) return MLOADIMAGE_ERR_ALLOC;

	p->png = png;

	//png_info 作成

	pnginfo = png_create_info_struct(png);
	if(!pnginfo) return MLOADIMAGE_ERR_ALLOC;

	p->pnginfo = pnginfo;

	//setjmp

	if(setjmp(png_jmpbuf(png)))
		return RET_ERR_NOMES;

	//入力

	p->io = mLoadImage_openIOFileBuf(&p->param->src);
	if(!p->io) return MLOADIMAGE_ERR_OPENFILE;

	png_set_read_fn(png, p, _png_read_func);

	//CRC エラーの対処
	/* デフォルトだとクラッシュするので、
	 * 重要なチャンクでは無視してそのまま使う。 */

	png_set_crc_action(png, PNG_CRC_QUIET_USE, PNG_CRC_DEFAULT);

	//----- ヘッダチェック

	if(!mIOFileBuf_readSize(p->io, sig, 8) || png_sig_cmp(sig, 0, 8))
	{
		mLoadImage_setMessage(p->param, "This is not PNG");
		return RET_ERR_NOMES;
	}

	png_set_sig_bytes(png, 8);

	//---- 情報読み込み

	png_read_info(png, pnginfo);

	p->start_image = TRUE;

	//解像度取得

	_get_resolution(p, &p->param->info);

	//---- 読み込み設定

	return _setinfo(p, &p->param->info);
}

/** メイン処理 */

static int _main_proc(_LOADPNG *p)
{
	mLoadImage *param = p->param;
	uint8_t *ps;
	int ret,i,pitch;

	//ヘッダ部分読み込み

	if((ret = _read_headers(p)))
		return ret;

	//Y1行バッファ確保

	pitch = param->info.raw_pitch;

	p->rowbuf = (uint8_t *)mMalloc(pitch, FALSE);
	if(!p->rowbuf) return MLOADIMAGE_ERR_ALLOC;

	//getinfo()

	if(param->getinfo)
	{
		ret = (param->getinfo)(param, &param->info);
		if(ret)
			return (ret < 0)? RET_ERR_NOMES: ret;
	}

	//イメージ

	ps = p->imgbuf;

	for(i = param->info.height; i > 0; i--)
	{
		//p->rowbuf に読み込み
		
		if(p->interlace)
		{
			//インターレース

			memcpy(p->rowbuf, ps, pitch);
			ps += pitch;
		}
		else
			png_read_row(p->png, (png_bytep)p->rowbuf, NULL);

		//getrow()

		ret = (param->getrow)(param, p->rowbuf, pitch);
		if(ret)
			return (ret < 0)? RET_ERR_NOMES: ret;
	}

	return RET_OK;
}


//========================
// main
//========================


/** 解放 */

static void _png_free(_LOADPNG *p)
{
	//libpng

	if(p->png)
	{
		//画像読み込み終了

		if(p->start_image)
			png_read_end(p->png, NULL);

		//png_struct,png_info 破棄

		png_destroy_read_struct(&p->png,
			(p->pnginfo)? &p->pnginfo: NULL, NULL);
	}

	mIOFileBuf_close(p->io);

	mFree(p->rowbuf);
	mFree(p->imgbuf);
	mFree(p->palbuf);
	mFree(p);
}

/** PNG 読み込み
 *
 * @ingroup loadimage */

mBool mLoadImagePNG(mLoadImage *param)
{
	_LOADPNG *p;
	int ret;

	//確保

	p = (_LOADPNG *)mMalloc(sizeof(_LOADPNG), TRUE);
	if(!p) return FALSE;

	p->param = param;

	//処理

	ret = _main_proc(p);

	if(ret != RET_OK)
	{
		if(ret == RET_ERR_NOMES)
		{
			if(!param->message)
				mLoadImage_setMessage(param, "error");
		}
		else
			mLoadImage_setMessage_errno(param, ret);
	}

	//解放

	_png_free(p);

	return (ret == RET_OK);
}

