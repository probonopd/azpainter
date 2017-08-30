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
 * mPSDLoad
 *****************************************/

#include <stdio.h>
#include <string.h>

#include "mDef.h"
#include "mRectBox.h"
#include "mUtil.h"
#include "mUtilStdio.h"
#include "mUtilCharCode.h"

#include "mPSDLoad.h"


//---------------

typedef struct
{
	mPSDLoadLayerInfo i;

	int channels;
	uint16_t ch_id[5];
	uint32_t ch_size[5],
		ch_fullsize;		//チャンネルデータすべてのサイズ
}_layerinfo;

//---------------

typedef struct 
{
	mPSDLoad i;		//基本情報

	FILE *fp;
	uint8_t *inbuf,			//作業用 + PackBits 展開用バッファ
		*rawlinebuf;		//Y1行生データバッファ
	uint16_t *encsizebuf;	//PackBits 圧縮時の圧縮サイズリストバッファ

	_layerinfo *layerinfobuf,	//レイヤ情報バッファ
		*curlinfo;		//現在のレイヤの情報ポインタ
		
	int curlayerno,		//現在のレイヤ番号
		cury,
		cur_masky,
		read_channel_w,	//現在読込中のチャンネルのイメージ幅
		layer_maxw,		//レイヤイメージの最大幅
		layer_maxh,		//レイヤイメージの最大高さ
		compress,		//圧縮フラグ
		inbuf_size,
		inbuf_pos,
		remain_read,	//残りの読み込みサイズ
		layermask_expand_left,
		layermask_expand_right,
		layermask_mask_w;
	uint32_t fsize_resource;	//リソースデータサイズ
	long fpos_resource,	//リソースデータ位置
		fpos_layertop,	//レイヤデータの先頭位置
		fpos_image,		//一枚絵イメージの位置
		fpos_tmp1,
		fpos_tmp2;
}_psdload;

//---------------

#define _INBUFSIZE  (8 * 1024)
#define _MAKE_SIG(a,b,c,d)   ((a << 24) | (b << 16) | (c << 8) | d)

//---------------

static const char *g_error_str[] = {
	NULL,
	"memory allocation error",
	"open file error",
	"this is not PSD",
	"data is corrupted",
	"invalid value",
	"unsupported format"
};

//---------------


//=========================
// PackBits 展開
//=========================


/** inbuf に読み込み */

static int _read_inbuf(_psdload *p)
{
	int size;

	size = p->remain_read;

	if(size > _INBUFSIZE - p->inbuf_size)
		size = _INBUFSIZE - p->inbuf_size;

	if(fread(p->inbuf + p->inbuf_size, 1, size, p->fp) != size)
		return MPSDLOAD_ERR_CORRUPTED;

	p->remain_read -= size;
	p->inbuf_size += size;

	return MPSDLOAD_ERR_OK;
}

/** inbuf に必要分を読み込み */

static mBool _read_inbuf_need(_psdload *p,int need)
{
	int remain;

	//必要分が存在する

	remain = p->inbuf_size - p->inbuf_pos;
	if(need <= remain) return TRUE;

	//要求サイズが残りサイズより大きい

	if(need > remain + p->remain_read) return FALSE;

	//残りを先頭に移動

	memcpy(p->inbuf, p->inbuf + p->inbuf_pos, remain);

	p->inbuf_size = remain;
	p->inbuf_pos = 0;

	//読み込み

	return (_read_inbuf(p) == MPSDLOAD_ERR_OK);
}

/** PackBits 展開 */

static int _read_packbits(_psdload *p,uint8_t *dstbuf,int rawsize,int encsize)
{
	uint8_t *inbuf = p->inbuf;
	int len;

	while(rawsize)
	{
		//長さ

		if(!_read_inbuf_need(p, 1)) return MPSDLOAD_ERR_CORRUPTED;

		len = *((char *)(inbuf + p->inbuf_pos));
		p->inbuf_pos++;

		//

		if(len == 128)
			continue;
		else if(len <= 0)
		{
			//連続データ

			len = -len + 1;

			if(len > rawsize || !_read_inbuf_need(p, 1))
				return MPSDLOAD_ERR_CORRUPTED;

			memset(dstbuf, inbuf[p->inbuf_pos], len);

			dstbuf += len;
			rawsize -= len;
			p->inbuf_pos++;
		}
		else
		{
			//非連続データ

			len++;

			if(len > rawsize || !_read_inbuf_need(p, len))
				return MPSDLOAD_ERR_CORRUPTED;

			memcpy(dstbuf, inbuf + p->inbuf_pos, len);

			dstbuf += len;
			rawsize -= len;
			p->inbuf_pos += len;
		}
	}

	return MPSDLOAD_ERR_OK;
}


//=========================
// sub
//=========================


/** レイヤ情報バッファを解放 */

static void _free_layerinfobuf(_psdload *p)
{
	_layerinfo *info;
	int i;

	if(p->layerinfobuf)
	{
		//名前を解放
		
		info = p->layerinfobuf;
		
		for(i = p->i.layernum; i; i--, info++)
			mFree(info->i.name);

		//

		mFree(p->layerinfobuf);
		p->layerinfobuf = NULL;
	}
}

/** イメージ用バッファ確保 */

static mBool _alloc_imagebuf(_psdload *p,int width,int height)
{
	p->rawlinebuf = (uint8_t *)mMalloc(width, FALSE);
	p->encsizebuf = (uint16_t *)mMalloc(height * 2, FALSE);

	return (p->rawlinebuf && p->encsizebuf);
}

/** イメージの範囲セット (イメージ or マスク) */

static void _set_image_area(_psdload *p,mBox *dst,int left,int top,int right,int bottom)
{
	int w,h;

	w = right - left;
	h = bottom - top;
	if(w < 0) w = 0;
	if(h < 0) h = 0;

	dst->x = left;
	dst->y = top;
	dst->w = w;
	dst->h = h;

	//全レイヤの最大幅、高さ

	if(p->layer_maxw < w) p->layer_maxw = w;
	if(p->layer_maxh < h) p->layer_maxh = h;
}

/** レイヤマスクをイメージの範囲に合わせる場合の情報セット (各レイヤ読み込み時) */

static void _set_layermask_adjust_info(_psdload *p,_layerinfo *info)
{
	mRect rcimg,rcmask,rc;

	mRectSetByBox(&rcimg, &info->i.box_img);
	mRectSetByBox(&rcmask, &info->i.box_mask);

	rc = rcmask;

	if(!mRectClipRect(&rc, &rcimg))
		p->layermask_mask_w = 0;
	else
	{
		p->layermask_mask_w = rc.x2 - rc.x1 + 1;
		p->layermask_expand_left = rcmask.x1 - rcimg.x1;
		p->layermask_expand_right = rcimg.x2 - rc.x2;
	}
}

/** 画像リソース内を ID から検索 */

static mBool _search_resource(_psdload *p,uint16_t resid,uint32_t *psize)
{
	uint32_t sig,size;
	uint16_t id;
	uint8_t b;
	int len;
	int64_t remain;

	fseek(p->fp, p->fpos_resource, SEEK_SET);

	remain = p->fsize_resource;

	while(remain > 6)
	{
		//識別子、ID (2byte)、名前の長さ

		if(!mFILEread32BE(p->fp, &sig)
			|| sig != _MAKE_SIG('8','B','I','M')
			|| !mFILEread16BE(p->fp, &id)
			|| !mFILEreadByte(p->fp, &b))
			return FALSE;

		//名前をスキップ (文字長さ分も含めて 2byte 境界)

		len = (b + 2) & (~1);

		fseek(p->fp, len - 1, SEEK_CUR);

		//データサイズ

		if(!mFILEread32BE(p->fp, &size)) return FALSE;

		//

		if(id == resid)
		{
			*psize = size;
			return TRUE;
		}
		else
		{
			//次へ (データは 2byte 境界)

			if(size & 1) size++;

			fseek(p->fp, size, SEEK_CUR);

			remain -= 4 + 2 + len + 4 + size;
		}
	}

	return FALSE;
}


//=========================
// レイヤ情報読み込み
//=========================


/** マスクレイヤデータ読み込み */

static mBool _read_layerinfo_layermask(_psdload *p,_layerinfo *info,uint32_t size)
{
	int32_t top,left,bottom,right;
	uint8_t defcol,flags;
	uint16_t padding;

	if(size == 0)
		return TRUE;
	else if(size < 20)
		fseek(p->fp, size, SEEK_CUR);
	else
	{
		//------ size >= 20 の場合

		if(!mFILEreadArgsBE(p->fp, "4444112",
			&top, &left, &bottom, &right, &defcol, &flags, &padding))
			return FALSE;

		_set_image_area(p, &info->i.box_mask, left, top, right, bottom);

		//相対位置

		if(flags & MPSDLOAD_LAYERMASK_F_RELATIVE)
		{
			info->i.box_mask.x += info->i.box_img.x;
			info->i.box_mask.y += info->i.box_img.y;
		}

		//

		info->i.layermask_defcol = defcol;
		info->i.layermask_flags = flags;

		fseek(p->fp, size - 20, SEEK_CUR);
	}

	return TRUE;
}

/** UTF16-BE を読み込んで p->inbuf に UTF-8 文字列セット */

static void _read_name_utf16(_psdload *p,int len)
{
	uint16_t *inbuf;
	int n;

	inbuf = (uint16_t *)mMalloc(len * 2, FALSE);
	if(!inbuf) return;

	if(mFILEreadArray16BE(p->fp, inbuf, len) == len)
	{
		n = mUTF16ToUTF8(inbuf, len, (char *)p->inbuf, 256);
		if(n >= 0) p->inbuf[n] = 0;
	}

	mFree(inbuf);
}

/** レイヤ情報の拡張データ (8BIM+key) を処理
 *
 * 'lsct' レイヤグループ : 戻り値で返す
 * 'luni' Unicode レイヤ名 (UTF-16BE) : p->inbuf に UTF-8 でセット
 *
 * @return レイヤグループタイプ */

static int _read_layerinfo_ex_keys(_psdload *p,uint32_t datasize)
{
	FILE *fp = p->fp;
	uint32_t sig,key,size,u32;
	long pos;
	int grouptype = 0,endflags = 0;

	while(datasize >= 12 && endflags != 3)
	{
		//識別子 (8BIM) + key (4byte) + サイズ (4byte)

		if(!mFILEread32BE(fp, &sig)
			|| sig != _MAKE_SIG('8','B','I','M')
			|| !mFILEread32BE(fp, &key)
			|| !mFILEread32BE(fp, &size))
			return 0;

		pos = ftell(fp);

		//各処理

		if(key == _MAKE_SIG('l','s','c','t'))
		{
			//'lsct' : レイヤグループ (Photoshop 6.0)

			endflags |= 1;

			if(size >= 4 && mFILEread32BE(fp, &u32))
				grouptype = u32;
		}
		else if(key == _MAKE_SIG('l','u','n','i'))
		{
			//'luni' : Unicode レイヤ名
			/* 文字数(4) + UTF16-BE 文字列 (4byte境界) */

			endflags |= 2;

			if(size >= 4
				&& mFILEread32BE(fp, &u32)
				&& u32)
				_read_name_utf16(p, u32);
		}

		//次へ

		if(datasize <= 12 + size) break;

		fseek(fp, pos + size, SEEK_SET);
		
		datasize -= 12 + size;
	}

	return grouptype;
}

/** レイヤ情報 - 拡張データ読み込み */

static int _read_layerinfo_ex(_psdload *p,_layerinfo *info,uint32_t datasize)
{
	FILE *fp = p->fp;
	uint32_t pos = 0,size;
	uint8_t len;

	//レイヤマスクデータ

	if(!mFILEread32BE(fp, &size)
		|| !_read_layerinfo_layermask(p, info, size))
		return MPSDLOAD_ERR_CORRUPTED;

	pos += 4 + size;

	//合成モードデータ

	if(!mFILEread32BE(fp, &size)
		|| fseek(fp, size, SEEK_CUR))
		return MPSDLOAD_ERR_CORRUPTED;

	pos += 4 + size;

	//レイヤ名 (inbuf にセット)
	/* レイヤ名は、文字数部分も含め 4byte 単位 */

	if(!mFILEreadByte(fp, &len))
		return MPSDLOAD_ERR_CORRUPTED;

	if(len)
	{
		if(fread(p->inbuf, 1, len, fp) != len)
			return MPSDLOAD_ERR_CORRUPTED;
	}

	p->inbuf[len] = 0;

	size = (1 + len + 3) & ~3;

	fseek(fp, size - (1 + len), SEEK_CUR);

	pos += size;

	//8BIM+key のデータを処理

	if(pos < datasize)
		info->i.group = _read_layerinfo_ex_keys(p, datasize - pos);

	//レイヤ名セット

	info->i.name = mStrdup((char *)p->inbuf);

	return MPSDLOAD_ERR_OK;
}

/** レイヤ情報読み込み */

static int _read_layerinfo(_psdload *p,_layerinfo *info)
{
	uint8_t *pd;
	int32_t i,top,left,bottom,right;
	uint16_t chnum;
	uint32_t size;
	long fpos;

	//-------- 範囲 .. チャンネル数

	if(!mFILEreadArgsBE(p->fp, "44442",
		&top, &left, &bottom, &right, &chnum))
		return MPSDLOAD_ERR_CORRUPTED;

	//範囲

	_set_image_area(p, &info->i.box_img, left, top, right, bottom);

	//チャンネル数 (最大 5 まで)

	if(chnum > 5) return MPSDLOAD_ERR_UNSUPPORTED;

	info->channels = chnum;

	//------ チャンネルデータ .. 拡張データまで

	size = chnum * 6 + 16;

	if(fread(p->inbuf, 1, size, p->fp) != size)
		return MPSDLOAD_ERR_CORRUPTED;

	pd = p->inbuf;

	//各チャンネルの ID とサイズ

	for(i = 0, size = 0; i < chnum; i++, pd += 6)
	{
		info->ch_id[i] = mGetBuf16BE(pd);
		info->ch_size[i] = mGetBuf32BE(pd + 2);

		size += info->ch_size[i];
	}

	info->ch_fullsize = size;

	//"8BIM"

	if(pd[0] != '8' || pd[1] != 'B' || pd[2] != 'I' || pd[3] != 'M')
		return MPSDLOAD_ERR_INVALID_VALUE;

	//合成モード

	info->i.blendmode = mGetBuf32BE(pd + 4);

	//不透明度

	info->i.opacity = pd[8];

	//clip

	//flags [1bit:非表示]

	if(pd[10] & 2)
		info->i.flags |= MPSDLOAD_LAYERINFO_F_HIDE;

	//0

	//--------- 拡張データ

	//拡張データサイズ

	size = mGetBuf32BE(pd + 12);

	//拡張データ

	if(size)
	{
		fpos = ftell(p->fp);
	
		i = _read_layerinfo_ex(p, info, size);

		fseek(p->fp, fpos + size, SEEK_SET);

		return i;
	}

	return MPSDLOAD_ERR_OK;
}


//=========================
// sub - main
//=========================


/** ヘッダ読み込み */

static int _read_header(_psdload *p)
{
	uint32_t sig,tmp32,h,w;
	uint16_t ver,tmp16,ch,bits,colmode;

	//読み込み

	if(!mFILEreadArgsBE(p->fp, "424224422",
		&sig, &ver, &tmp32, &tmp16, &ch, &h, &w, &bits, &colmode))
		return MPSDLOAD_ERR_CORRUPTED;

	//識別子

	if(sig != _MAKE_SIG('8','B','P','S'))
		return MPSDLOAD_ERR_NOT_PSD;

	//バージョン

	if(ver != 1) return MPSDLOAD_ERR_UNSUPPORTED;

	//一枚絵のチャンネル数

	p->i.img_channels = ch;

	//幅、高さ

	if(w == 0 || w > 30000 || h == 0 || h > 30000)
		return MPSDLOAD_ERR_INVALID_VALUE;

	p->i.width  = w;
	p->i.height = h;

	//ビット数 (1/8bit のみ対応)

	if(bits != 1 && bits != 8)
		return MPSDLOAD_ERR_UNSUPPORTED;

	p->i.bits = bits;

	//カラーモード

	if(colmode != MPSD_COLMODE_MONO
		&& colmode != MPSD_COLMODE_GRAYSCALE
		&& colmode != MPSD_COLMODE_RGB)
		return MPSDLOAD_ERR_UNSUPPORTED;

	p->i.colmode = colmode;

	//ビット数とカラーモードの組み合わせ

	if((bits == 1 && colmode != MPSD_COLMODE_MONO)
		|| (colmode == MPSD_COLMODE_MONO && bits != 1))
		return MPSDLOAD_ERR_INVALID_VALUE;

	return MPSDLOAD_ERR_OK;
}

/** レイヤデータの先頭情報読み込み */

static int _read_layertop(_psdload *p)
{
	uint32_t size;
	int16_t num;

	//レイヤ全体サイズ (0 でレイヤなし)

	if(!mFILEread32BE(p->fp, &size))
		return MPSDLOAD_ERR_CORRUPTED;

	//一枚絵イメージの位置

	p->fpos_image = ftell(p->fp) + size;

	//レイヤデータがある場合

	if(size)
	{
		//内部サイズ (0 でレイヤなし)

		if(!mFILEread32BE(p->fp, &size))
			return MPSDLOAD_ERR_CORRUPTED;

		//レイヤ数
		/* 負の値の場合があるので、絶対値にする。0 でレイヤなし。 */

		if(size)
		{
			if(!mFILEread16BE(p->fp, &num))
				return MPSDLOAD_ERR_CORRUPTED;

			p->i.layernum = (num < 0)? -num: num;
		}

		//レイヤのデータ位置

		p->fpos_layertop = ftell(p->fp);
	}

	return MPSDLOAD_ERR_OK;
}

/** PackBits Y圧縮サイズリスト読み込み
 *
 * @param pfullsize イメージ全体の圧縮サイズが入る (NULL で取得しない) */

static int _read_encsizelist(_psdload *p,int height,uint32_t *pfullsize)
{
	uint8_t *pd;
	int i,n;
	uint32_t fullsize = 0;

	//無圧縮、または空イメージ時

	if(p->compress == 0 || height == 0)
	{
		if(pfullsize) *pfullsize = 0;
		return MPSDLOAD_ERR_OK;
	}

	//バッファに読み込み

	if(fread(p->encsizebuf, 1, height * 2, p->fp) != height * 2)
		return MPSDLOAD_ERR_CORRUPTED;

	//BE -> システムエンディアンに変換 & 全体サイズ取得

	pd = (uint8_t *)p->encsizebuf;

	for(i = height; i > 0; i--, pd += 2)
	{
		n = (pd[0] << 8) | pd[1];

		*((uint16_t *)pd) = n;

		fullsize += n;
	}

	if(pfullsize) *pfullsize = fullsize;

	return MPSDLOAD_ERR_OK;
}

/** 各チャンネルのY1行イメージ読み込み */

static int _read_channel_row(_psdload *p,int width)
{
	int ret;

	//1bit の場合

	if(p->i.bits == 1)
		width = (width + 7) >> 3;

	//読み込み

	if(p->compress == 0)
	{
		//無圧縮

		if(fread(p->rawlinebuf, 1, width, p->fp) != width)
			return MPSDLOAD_ERR_CORRUPTED;
	}
	else
	{
		//PackBits

		ret = _read_packbits(p, p->rawlinebuf, width, p->encsizebuf[p->cury]);
		if(ret != MPSDLOAD_ERR_OK) return ret;
	}

	p->cury++;

	return MPSDLOAD_ERR_OK;
}

/** 一枚絵イメージ読み込み開始 */

static int _begin_image(_psdload *p)
{
	uint16_t comp;

	//バッファ確保

	if(!_alloc_imagebuf(p, p->i.width, p->i.height))
		return MPSDLOAD_ERR_ALLOC;

	//先頭位置

	fseek(p->fp, p->fpos_image, SEEK_SET);

	//圧縮タイプ

	if(!mFILEread16BE(p->fp, &comp))
		return MPSDLOAD_ERR_CORRUPTED;

	if(comp >= 2) return MPSDLOAD_ERR_UNSUPPORTED;

	p->compress = comp;

	//ファイル位置

	if(comp == 1)
	{
		p->fpos_tmp1 = ftell(p->fp);
		p->fpos_tmp2 = p->fpos_tmp1 + p->i.height * 2 * p->i.img_channels;
	}

	return MPSDLOAD_ERR_OK;
}


//=========================
// main
//=========================


/*******************//**

@defgroup psdload mPSDLoad
@brief PSD 読み込み

- 1bit モノクロの場合は常にレイヤなし。
- レイヤありの場合、カラーモードはグレイスケールか RGB である。
- カラーモードがグレイスケールの場合もアルファチャンネルが存在する場合あり。

@ingroup group_image
 
@{
@file mPSDLoad.h

************************/


/** 閉じる */

void mPSDLoad_close(mPSDLoad *load)
{
	_psdload *p = (_psdload *)load;

	if(p)
	{
		if(p->fp) fclose(p->fp);

		_free_layerinfobuf(p);

		mFree(p->inbuf);
		mFree(p->rawlinebuf);
		mFree(p->encsizebuf);
		
		mFree(p);
	}
}

/** 作成 */

mPSDLoad *mPSDLoad_new()
{
	_psdload *p;

	p = (_psdload *)mMalloc(sizeof(_psdload), TRUE);
	if(!p) return NULL;

	//バッファ

	p->inbuf = (uint8_t *)mMalloc(_INBUFSIZE, FALSE);
	if(!p->inbuf)
	{
		mPSDLoad_close((mPSDLoad *)p);
		return NULL;
	}

	return (mPSDLoad *)p;
}

/** ファイルから開く
 *
 * ヘッダ情報も読み込まれる。 */

mBool mPSDLoad_openFile(mPSDLoad *load,const char *filename)
{
	_psdload *p = (_psdload *)load;
	uint32_t size;

	//開く

	p->fp = mFILEopenUTF8(filename, "rb");

	if(!p->fp)
	{
		load->err = MPSDLOAD_ERR_OPENFILE;
		return FALSE;
	}

	//ヘッダ読み込み

	if((load->err = _read_header(p)) != MPSDLOAD_ERR_OK)
		return FALSE;

	//カラーモードデータをスキップ

	if(!mFILEread32BE(p->fp, &size)
		|| fseek(p->fp, size, SEEK_CUR))
	{
		load->err = MPSDLOAD_ERR_CORRUPTED;
		return FALSE;
	}

	//画像リソースデータ

	if(!mFILEread32BE(p->fp, &p->fsize_resource))
	{
		load->err = MPSDLOAD_ERR_CORRUPTED;
		return FALSE;
	}

	p->fpos_resource = ftell(p->fp);

	fseek(p->fp, p->fsize_resource, SEEK_CUR);

	//レイヤの先頭情報

	if((load->err = _read_layertop(p)) != MPSDLOAD_ERR_OK)
		return FALSE;

	return TRUE;
}

/** レイヤデータがあるか */

mBool mPSDLoad_isHaveLayer(mPSDLoad *load)
{
	return (load->bits != 1 && load->layernum);
}

/** Y1行生データのバッファを取得 */

uint8_t *mPSDLoad_getLineImageBuf(mPSDLoad *p)
{
	return ((_psdload *)p)->rawlinebuf;
}

/** エラーメッセージを取得
 *
 * @return NULL でなし  */

const char *mPSDLoad_getErrorMessage(int err)
{
	if(err < MPSDLOAD_ERR_NUM)
		return g_error_str[err];
	else
		return NULL;
}


//==========================
// 画像リソース
//==========================


/** 画像リソース:カレントレイヤ番号読み込み
 *
 * @param pno 格納順のインデックス */

mBool mPSDLoad_readResource_currentLayer(mPSDLoad *load,int *pno)
{
	_psdload *p = (_psdload *)load;
	uint32_t size;
	uint16_t no;

	if(!_search_resource(p, 0x0400, &size)
		|| size != 2
		|| !mFILEread16BE(p->fp, &no))
	{
		*pno = 0;
		return FALSE;
	}
	else
	{
		*pno = no;
		return TRUE;
	}
}

/** 画像リソース:解像度を DPI で読み込み */

mBool mPSDLoad_readResource_resolution_dpi(mPSDLoad *load,int *horz,int *vert)
{
	_psdload *p = (_psdload *)load;
	uint32_t size,val;
	uint16_t unit;
	uint8_t *ps;

	ps = p->inbuf;

	if(!_search_resource(p, 0x03ed, &size)
		|| size != 16
		|| fread(ps, 1, 16, p->fp) != 16)
		return FALSE;

	//horz

	val = mGetBuf32BE(ps);
	unit = mGetBuf16BE(ps + 4); //1:pixel/inch 2:pixel/cm

	if(unit == 1)
		*horz = val >> 16;
	else
		*horz = (int)((double)val / (1<<16) * 0.0254 + 0.5);
	
	//vert

	val = mGetBuf32BE(ps + 8);
	unit = mGetBuf16BE(ps + 12);

	if(unit == 1)
		*vert = val >> 16;
	else
		*vert = (int)((double)val / (1<<16) * 0.0254 + 0.5);

	return TRUE;
}


//==========================
// レイヤ
//==========================


/** レイヤ読み込み開始 */

mBool mPSDLoad_beginLayer(mPSDLoad *load)
{
	_psdload *p = (_psdload *)load;
	_layerinfo *info;
	int i,ret,num;

	num = load->layernum;

	if(num == 0) return TRUE;

	//レイヤ情報データバッファ確保

	p->layerinfobuf = (_layerinfo *)mMalloc(sizeof(_layerinfo) * num, TRUE);
	if(!p->layerinfobuf)
	{
		load->err = MPSDLOAD_ERR_ALLOC;
		return FALSE;
	}

	//レイヤ情報をすべて読み込み

	fseek(p->fp, p->fpos_layertop, SEEK_SET);

	info = p->layerinfobuf;

	for(i = num; i; i--, info++)
	{
		ret = _read_layerinfo(p, info);
		if(ret != MPSDLOAD_ERR_OK)
		{
			load->err = ret;
			return FALSE;
		}
	}

	//イメージ用バッファ確保

	if(num)
	{
		if(!_alloc_imagebuf(p, p->layer_maxw, p->layer_maxh))
		{
			load->err = MPSDLOAD_ERR_ALLOC;
			return FALSE;
		}
	}

	//

	p->curlayerno = 0;
	p->curlinfo = (_layerinfo *)p->layerinfobuf;

	return TRUE;
}

/** レイヤ情報取得 */

mBool mPSDLoad_getLayerInfo(mPSDLoad *load,int no,mPSDLoadLayerInfo *info)
{
	if(no < 0 || no >= load->layernum)
		return FALSE;
	else
	{
		*info = (((_psdload *)load)->layerinfobuf + no)->i;

		return TRUE;
	}
}

/** 全レイヤのイメージの最大幅と高さを取得 */

void mPSDLoad_getLayerImageMaxSize(mPSDLoad *load,mSize *size)
{
	_psdload *p = (_psdload *)load;

	size->w = p->layer_maxw;
	size->h = p->layer_maxh;
}

/** 各レイヤのイメージ読み込み開始 (格納順)
 *
 * @param info レイヤ情報が入る (NULL で取得しない)
 * @param skip_group  レイヤグループ関連のレイヤをスキップする (通常のイメージレイヤのみ)
 * @return イメージ範囲があるか (FALSE で空イメージ) */

mBool mPSDLoad_beginLayerImage(mPSDLoad *load,mPSDLoadLayerInfo *info,mBool skip_group)
{
	_psdload *p = (_psdload *)load;

	//レイヤグループ関連をスキップ

	if(skip_group)
	{
		while(p->curlayerno < load->layernum
			&& p->curlinfo->i.group != MPSDLOAD_LAYERGROUP_NONE)
		{
			fseek(p->fp, p->curlinfo->ch_fullsize, SEEK_CUR);

			p->curlayerno++;
			p->curlinfo++;
		}
	}

	//

	if(p->curlayerno >= load->layernum) return FALSE;

	//イメージデータ先頭位置

	p->fpos_tmp1 = ftell(p->fp);

	//

	if(info) *info = p->curlinfo->i;

	//範囲があるか

	return (p->curlinfo->i.box_img.w && p->curlinfo->i.box_img.h);
}

/** レイヤの各チャンネルイメージの読み込み開始
 *
 * @return -1 で指定チャンネルが存在しない */

int mPSDLoad_beginLayerImageChannel(mPSDLoad *load,uint16_t channel_id)
{
	_psdload *p = (_psdload *)load;
	_layerinfo *info;
	int i,pos,chno,is_mask;
	uint16_t comp;

	info = p->curlinfo;

	is_mask = (channel_id == MPSD_CHANNEL_ID_MASK);

	//------ 指定チャンネル検索

	for(i = 0, pos = 0; i < info->channels; i++)
	{
		if(info->ch_id[i] == channel_id) break;

		pos += info->ch_size[i];
	}

	if(i == info->channels) return -1;

	chno = i;

	//イメージ位置へ

	fseek(p->fp, p->fpos_tmp1 + pos, SEEK_SET);

	//-----------

	//圧縮フラグ [0:無圧縮 1:PackBits]

	if(!mFILEread16BE(p->fp, &comp))
	{
		load->err = MPSDLOAD_ERR_CORRUPTED;
		return FALSE;
	}

	if(comp != 0 && comp != 1)
	{
		load->err = MPSDLOAD_ERR_UNSUPPORTED;
		return FALSE;
	}

	p->compress = comp;

	//セット
	
	p->remain_read = info->ch_size[chno] - 2;  //圧縮フラグ分は引く
	p->cury = 0;
	p->read_channel_w = (is_mask)? info->i.box_mask.w: info->i.box_img.w;

	//レイヤマスクの場合の情報

	if(is_mask)
	{
		_set_layermask_adjust_info(p, info);

		p->cur_masky = info->i.box_img.y;
	}

	//PackBits 時

	if(comp == 1)
	{
		//サイズリスト
	
		load->err = _read_encsizelist(p,
			(is_mask)? info->i.box_mask.h: info->i.box_img.h, NULL);

		if(load->err != MPSDLOAD_ERR_OK) return FALSE;

		//inbuf に読み込み

		p->inbuf_size = p->inbuf_pos = 0;

		load->err = _read_inbuf(p);
		if(load->err != MPSDLOAD_ERR_OK) return FALSE;
	}

	return TRUE;
}

/** レイヤイメージの各チャンネルの生データY1行を読み込み */

mBool mPSDLoad_readLayerImageChannelLine(mPSDLoad *load)
{
	_psdload *p = (_psdload *)load;

	load->err = _read_channel_row(p, p->read_channel_w);

	return (load->err == MPSDLOAD_ERR_OK);
}

/** レイヤマスクのY1行を読み込み
 *
 * イメージの範囲に合わせる＆フラグの状態セット */

mBool mPSDLoad_readLayerImageChannelLine_mask(mPSDLoad *load)
{
	_psdload *p = (_psdload *)load;
	uint8_t *buf = p->rawlinebuf,defcol,rev;
	int imgw,maskw,left,right;
	mBox boxmask;

	imgw = p->curlinfo->i.box_img.w;
	maskw = p->layermask_mask_w;
	boxmask = p->curlinfo->i.box_mask;

	defcol = p->curlinfo->i.layermask_defcol;
	rev = (p->curlinfo->i.layermask_flags & MPSDLOAD_LAYERMASK_F_REVERSE);

	//イメージの位置 Y がマスクデータの範囲外ならデフォルトで埋める
	//(マスクよりイメージの方が大きい場合)

	if(p->cur_masky < boxmask.y
		|| p->cur_masky >= boxmask.y + boxmask.h)
	{
		memset(buf, (rev)? 255 - defcol: defcol, imgw);
		p->cur_masky++;
		
		return TRUE;
	}

	p->cur_masky++;

	//マスクの上端がイメージより上の場合、イメージ部分の上端に来るまでスキップ

	if(boxmask.y < p->curlinfo->i.box_img.y)
	{
		while(p->cury < p->curlinfo->i.box_img.y - boxmask.y)
		{
			if((load->err = _read_channel_row(p, p->read_channel_w)))
				return FALSE;
		}
	}

	//読み込み
	
	if((load->err = _read_channel_row(p, p->read_channel_w)))
		return FALSE;

	/* maskw : マスクデータ内で使われる部分の長さ */

	if(maskw == 0)
	{
		//全体がイメージの範囲外ならデフォルトで埋める

		memset(buf, (rev)? 255 - defcol: defcol, imgw);
	}
	else
	{
		left = p->layermask_expand_left;
		right = p->layermask_expand_right;

		if(left < 0)
		{
			//先頭部分を縮小
			memmove(buf, buf - left, maskw);
		}
		else if(left > 0)
		{
			//先頭部分を拡張
			memmove(buf + left, buf, maskw);
			memset(buf, defcol, left);
		}

		//右拡張部分を埋める

		if(right)
			memset(buf + imgw - right, defcol, right);

		//反転

		if(rev)
		{
			for(; imgw > 0; imgw--, buf++)
				*buf = 255 - *buf;
		}
	}

	return TRUE;
}

/** 各レイヤイメージの読み込み終了 */

void mPSDLoad_endLayerImage(mPSDLoad *load)
{
	_psdload *p = (_psdload *)load;

	//次のイメージ位置へ

	fseek(p->fp, p->fpos_tmp1 + p->curlinfo->ch_fullsize, SEEK_SET);

	//

	p->curlayerno++;
	p->curlinfo++;
}

/** レイヤ読み込み終了 */

void mPSDLoad_endLayer(mPSDLoad *load)
{
	_psdload *p = (_psdload *)load;

	_free_layerinfobuf(p);

	mFree(p->rawlinebuf);
	mFree(p->encsizebuf);
	
	p->rawlinebuf = NULL;
	p->encsizebuf = NULL;
}


//==========================
// 一枚絵イメージ
//==========================


/** 一枚絵イメージ読み込み開始 */

mBool mPSDLoad_beginImage(mPSDLoad *p)
{
	p->err = _begin_image((_psdload *)p);

	return (p->err == MPSDLOAD_ERR_OK);
}

/** 一枚絵イメージの各チャンネル読み込み開始 */

mBool mPSDLoad_beginImageChannel(mPSDLoad *load)
{
	_psdload *p = (_psdload *)load;
	uint32_t size;

	p->cury = 0;

	//PackBits 時
	
	if(p->compress == 1)
	{
		//Y圧縮サイズリスト読み込み

		fseek(p->fp, p->fpos_tmp1, SEEK_SET);

		load->err = _read_encsizelist(p, p->i.height, &size);
		if(load->err != MPSDLOAD_ERR_OK) return FALSE;

		//イメージ位置へ

		fseek(p->fp, p->fpos_tmp2, SEEK_SET);

		//次の読み込み位置

		p->fpos_tmp1 += p->i.height * 2;
		p->fpos_tmp2 += size;

		//inbuf 読み込み

		p->remain_read = size;
		p->inbuf_pos = p->inbuf_size = 0;

		load->err = _read_inbuf(p);
		if(load->err != MPSDLOAD_ERR_OK) return FALSE;
	}

	return TRUE;
}

/** 一枚絵イメージの各チャンネルのY1行を読み込み */

mBool mPSDLoad_readImageChannelLine(mPSDLoad *load)
{
	_psdload *p = (_psdload *)load;

	load->err = _read_channel_row(p, p->i.width);

	return (load->err == MPSDLOAD_ERR_OK);
}

/* @} */
