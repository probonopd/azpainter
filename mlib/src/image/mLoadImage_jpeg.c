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
 * JPEG 読み込み関数
 *****************************************/

#include <stdio.h>
#include <string.h>
#include <setjmp.h>

#include <jpeglib.h>

#include "mDef.h"
#include "mLoadImage.h"
#include "mUtilStdio.h"


//---------------------------
/* メモリからの読み込み用 */

typedef struct
{
	struct jpeg_source_mgr pub;
	const uint8_t *buf;
	uint32_t size;
	uint8_t eof_buf[2];  //0xff, 0xd9
}_JPEG_MEM;

//--------------------
/* エラー用 */

typedef struct
{
	struct jpeg_error_mgr jerr;
	jmp_buf jmpbuf;
	struct __LOADJPEG *loadjpg;
}_JPEG_ERR;

//--------------------

typedef struct __LOADJPEG
{
	mLoadImage *param;

	struct jpeg_decompress_struct jpg;
	_JPEG_ERR jpgerr;

	FILE *fp;
	uint8_t *rowbuf;

	uint8_t start_decompress,
		need_fp_close;
}_LOADJPEG;

//--------------------

#define RET_OK        0
#define RET_ERR_NOMES -1

//--------------------


//===============================
// メモリからの読み込み用
//===============================


static void _mem_dummy(j_decompress_ptr p)
{

}

/** バッファを埋める */

static boolean _mem_fill_input_buffer(j_decompress_ptr p)
{
	p->src->next_input_byte = ((_JPEG_MEM *)p->src)->eof_buf;
	p->src->bytes_in_buffer = 2;

	return TRUE;
}

/** 指定バイト移動 */

static void _mem_skip_input_data(j_decompress_ptr p,long size)
{
	_JPEG_MEM *mem = (_JPEG_MEM *)p->src;

	if(mem->pub.bytes_in_buffer < size)
		//データが足りない
		_mem_fill_input_buffer(p);
	else if(size > 0)
	{
		mem->pub.next_input_byte += size;
		mem->pub.bytes_in_buffer -= size;
	}
}

/** ソースをメモリにセット */

static void _jpeg_memory_src(j_decompress_ptr cinfo,const void *buf,uint32_t size)
{
	_JPEG_MEM *p;

	//_JPEG_MEM 確保

	if(!cinfo->src)
	{
		cinfo->src = (struct jpeg_source_mgr *)
			(*cinfo->mem->alloc_small)((j_common_ptr)cinfo, JPOOL_PERMANENT, sizeof(_JPEG_MEM));
	}

	//セット

	p = (_JPEG_MEM *)cinfo->src;

	p->pub.init_source = _mem_dummy;
	p->pub.fill_input_buffer = _mem_fill_input_buffer;
	p->pub.skip_input_data = _mem_skip_input_data;
	p->pub.resync_to_restart = jpeg_resync_to_restart;
	p->pub.term_source = _mem_dummy;

	p->pub.next_input_byte = (JOCTET *)buf;
	p->pub.bytes_in_buffer = size;

	p->buf = buf;
	p->size = size;
	p->eof_buf[0] = 0xff;
	p->eof_buf[1] = JPEG_EOI;
}


//=======================
// sub
//=======================


/** エラー終了時のハンドラ */

static void _jpeg_error_exit(j_common_ptr p)
{
	_JPEG_ERR *err = (_JPEG_ERR *)p->err;
	char msg[JMSG_LENGTH_MAX];

	//エラー文字列を取得

	(err->jerr.format_message)(p, msg);

	mLoadImage_setMessage(err->loadjpg->param, msg);

	//setjmp の位置に飛ぶ

	longjmp(err->jmpbuf, 1);
}

/** メッセージ表示ハンドラ */

static void _jpeg_output_message(j_common_ptr p)
{

}

/** 解像度単位を取得 */

static int _get_resolution_unit(int unit)
{
	if(unit == 0)
		return MLOADIMAGE_RESOLITION_UNIT_ASPECT;
	else if(unit == 1)
		return MLOADIMAGE_RESOLITION_UNIT_DPI;
	else if(unit == 2)
		return MLOADIMAGE_RESOLITION_UNIT_DPCM;
	else
		return MLOADIMAGE_RESOLITION_UNIT_NONE;
}

/** EXIF 値取得 */

static uint32_t _get_exif_value(uint8_t **pp,int size,uint8_t bigendian)
{
	uint8_t *p = *pp;

	*pp += size;

	if(bigendian)
	{
		if(size == 2)
			return (p[0] << 8) | p[1];
		else
			return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
	}
	else
	{
		if(size == 2)
			return (p[1] << 8) | p[0];
		else
			return (p[3] << 24) | (p[2] << 16) | (p[1] << 8) | p[0];
	}
}

/** EXIF から解像度取得 */

static void _get_exif_resolution(_LOADJPEG *p,mLoadImageInfo *info)
{
	struct jpeg_marker_struct *plist;
	uint8_t *ps,*ps2,*tifftop,endian,flags = 0;
	int fnum,tag,dattype,unit;
	uint32_t pos,horz,vert;

	for(plist = p->jpg.marker_list; plist; plist = plist->next)
	{
		if(plist->marker == 0xe1 && plist->data_length >= 16)
		{
			ps = (uint8_t *)plist->data;
			tifftop = ps + 6;

			if(strcmp((char *)ps, "Exif") != 0) return;

			endian = (ps[6] == 'M');  //0=little-endian 1=big-endian

			ps += 14;

			//IFD-0th

			fnum = _get_exif_value(&ps, 2, endian);

			for(; fnum > 0 && flags != 7; fnum--)
			{
				tag = _get_exif_value(&ps, 2, endian);
				dattype = _get_exif_value(&ps, 2, endian);
				ps += 4;
				pos = _get_exif_value(&ps, 4, endian);

				if(tag == 0x011a && dattype == 5)
				{
					//解像度水平

					ps2 = tifftop + pos;
					horz = _get_exif_value(&ps2, 4, endian);
					flags |= 1;
				}
				else if(tag == 0x011b && dattype == 5)
				{
					//解像度垂直

					ps2 = tifftop + pos;
					vert = _get_exif_value(&ps2, 4, endian);
					flags |= 2;
				}
				else if(tag == 0x0128 && dattype == 3)
				{
					//解像度単位 (short)
					/* 1:none 2:dpi 3:dpcm */

					unit = pos;
					flags |= 4;
				}
			}

			//終了

			if(flags == 7 && (unit == 2 || unit == 3))
			{
				info->resolution_unit = _get_resolution_unit(unit - 1);
				info->resolution_horz = horz;
				info->resolution_vert = vert;
			}

			break;
		}
	}
}

/** イメージ部分まで処理 */

static int _read_headers(_LOADJPEG *p)
{
	j_decompress_ptr jpg;
	mLoadImageInfo *info = &p->param->info;
	int colspace;

	jpg = &p->jpg;

	//エラー設定
	/* デフォルトではエラー時にプロセスが終了するので、longjmp を行うようにする */

	jpg->err = jpeg_std_error(&p->jpgerr.jerr);

	p->jpgerr.jerr.error_exit = _jpeg_error_exit;
	p->jpgerr.jerr.output_message = _jpeg_output_message;
	p->jpgerr.loadjpg = p;

	//エラー時

	if(setjmp(p->jpgerr.jmpbuf))
		return RET_ERR_NOMES;

	//構造体初期化

	jpeg_create_decompress(jpg);

	//------ 各タイプ別の初期化

	switch(p->param->src.type)
	{
		//ファイル
		case MLOADIMAGE_SOURCE_TYPE_PATH:
			p->fp = mFILEopenUTF8(p->param->src.filename, "rb");
			if(!p->fp) return MLOADIMAGE_ERR_OPENFILE;

			jpeg_stdio_src(jpg, p->fp);
			p->need_fp_close = TRUE;
			break;
		//バッファ
		case MLOADIMAGE_SOURCE_TYPE_BUF:
			_jpeg_memory_src(jpg, p->param->src.buf, p->param->src.bufsize);
			break;
		//FILE*
		case MLOADIMAGE_SOURCE_TYPE_STDIO:
			jpeg_stdio_src(jpg, (FILE *)p->param->src.fp);
			break;
	}

	//-------- 情報

	//保存するマーカー

	jpeg_save_markers(jpg, 0xe1, 0xffff);  //EXIF

	//ヘッダ読み込み

	if(jpeg_read_header(jpg, TRUE) != JPEG_HEADER_OK)
		return MLOADIMAGE_ERR_NONE_IMAGE;

	//最大サイズ判定

	if((p->param->max_width > 0 && jpg->image_width > p->param->max_width)
		|| (p->param->max_height > 0 && jpg->image_height > p->param->max_height))
		return MLOADIMAGE_ERR_LARGE_SIZE;

	//情報セット

	colspace = jpg->jpeg_color_space;

	info->width  = jpg->image_width;
	info->height = jpg->image_height;
	info->sample_bits = 8;
	info->raw_sample_bits = 8;
	info->raw_pitch = info->width * ((colspace == JCS_GRAYSCALE)? 1: 3);

	if(colspace == JCS_GRAYSCALE)
	{
		info->raw_bits = 8;
		info->raw_coltype = MLOADIMAGE_COLTYPE_GRAY;
	}
	else
	{
		info->raw_bits = 24;
		info->raw_coltype = MLOADIMAGE_COLTYPE_RGB;
	}

	//------- 開始

	//出力フォーマット

	if(p->param->format != MLOADIMAGE_FORMAT_RAW)
	{
		if(colspace == JCS_CMYK || colspace == JCS_YCCK)
			jpg->out_color_space = JCS_CMYK;
		else
			jpg->out_color_space = (p->param->format == MLOADIMAGE_FORMAT_RGBA)? JCS_EXT_RGBA: JCS_RGB;
	}

	//展開開始

	if(!jpeg_start_decompress(jpg))
		return MLOADIMAGE_ERR_ALLOC;

	p->start_decompress = TRUE;

	//----- マーカー情報

	//解像度

	if(jpg->saw_JFIF_marker)
	{
		//JFIF
		p->param->info.resolution_horz = jpg->X_density;
		p->param->info.resolution_vert = jpg->Y_density;
		p->param->info.resolution_unit = _get_resolution_unit(jpg->density_unit);
	}
	else
		//EXIF
		_get_exif_resolution(p, &p->param->info);

	return RET_OK;
}

/** 出力用のY1行サイズ取得 */

static int _get_out_pitch(_LOADJPEG *p)
{
	int cs,bytes;

	cs = p->jpg.out_color_space;

	if(cs == JCS_GRAYSCALE)
		bytes = 1;
	else if(cs == JCS_CMYK)
		bytes = 4;
	else
		bytes = (p->param->format == MLOADIMAGE_FORMAT_RGBA)? 4: 3;

	return p->jpg.image_width * bytes;
}


//=======================
// main
//=======================


/** イメージ変換 (CMYK -> RGB/RGBA) */

static void _convert_CMYK(_LOADJPEG *p)
{
	int i,c,m,y,k;
	uint8_t *ps,*pd,alpha;

	alpha = (p->param->format == MLOADIMAGE_FORMAT_RGBA);

	ps = pd = p->rowbuf;

	for(i = p->jpg.image_width; i > 0; i--)
	{
		c = ps[0];
		m = ps[1];
		y = ps[2];
		k = ps[3];

		/* [基本の計算式]
		 * R,G,B = 1 - min(1, [C,M,Y] * (1 - K) + K)
		 *
		 * CMYK JPEG の場合、値は常に反転されている。
		 * 
		 * [値の反転時]
		 * 1 - ((1 - C) * (1 - (1 - K)) + 1 - K)
		 * = 1 - ((1 - C) * K + 1 - K)
		 * = 1 - (K + C * K + 1 - K)
		 * = C * K */

		pd[0] = (c * k + 127) / 255;
		pd[1] = (m * k + 127) / 255;
		pd[2] = (y * k + 127) / 255;

		if(alpha)
		{
			pd[3] = 255;
			pd += 4;
		}
		else
			pd += 3;
	
		ps += 4;
	}
}

/** メイン処理 */

static int _main_proc(_LOADJPEG *p)
{
	mLoadImage *param = p->param;
	int ret,pitch,bconvert;
	
	//イメージ部分まで処理

	if((ret = _read_headers(p)))
		return ret;

	//Y1行バッファ確保

	pitch = _get_out_pitch(p);

	p->rowbuf = (uint8_t *)mMalloc(pitch, FALSE);
	if(!p->rowbuf) return MLOADIMAGE_ERR_ALLOC;

	//getinfo()

	if(param->getinfo)
	{
		ret = (param->getinfo)(param, &param->info);
		if(ret)
			return (ret < 0)? RET_ERR_NOMES: RET_OK;
	}

	//変換するか

	bconvert = (p->jpg.out_color_space == JCS_CMYK
		&& p->param->format != MLOADIMAGE_FORMAT_RAW);

	//イメージ

	while(p->jpg.output_scanline < p->jpg.output_height)
	{
		//読み込み

		if(jpeg_read_scanlines(&p->jpg, (JSAMPARRAY)&p->rowbuf, 1) != 1)
			return MLOADIMAGE_ERR_CORRUPT;

		//変換

		if(bconvert)
			_convert_CMYK(p);

		//getrow()

		ret = (param->getrow)(param, p->rowbuf, pitch);
		if(ret)
			return (ret < 0)? RET_ERR_NOMES: RET_OK;
	}

	return RET_OK;
}

/** 解放 */

static void _jpeg_free(_LOADJPEG *p)
{
	//jpeglib

	if(p->start_decompress)
		jpeg_finish_decompress(&p->jpg);
	
	jpeg_destroy_decompress(&p->jpg);

	//ファイル閉じる

	if(p->fp && p->need_fp_close)
		fclose(p->fp);

	//

	mFree(p->rowbuf);
	mFree(p);
}

/** JPEG 読み込み
 *
 * @ingroup loadimage */

mBool mLoadImageJPEG(mLoadImage *param)
{
	_LOADJPEG *p;
	int ret;

	//確保

	p = (_LOADJPEG *)mMalloc(sizeof(_LOADJPEG), TRUE);
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

	_jpeg_free(p);

	return (ret == RET_OK);
}

