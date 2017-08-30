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
 * BMP 読み込み関数
 *****************************************/

#include <string.h>

#include "mDef.h"
#include "mLoadImage.h"

#include "mIOFileBuf.h"
#include "mUtil.h"


//--------------------

typedef struct
{
	mLoadImage *param;

	mIOFileBuf *io;
	uint8_t *rawbuf,	//Y1行分の生バッファ
		*convbuf,		//変換後のバッファ
		*palbuf,		//パレットバッファ [R-G-B-A]
		*rlebuf,		//RLE 用バッファ
		*rlebuf_end,	//RLE バッファ終端位置
		*rlebuf_cur;	//RLE バッファ現在位置

	int	bits,			//BMP ビット数 (1,4,8,16,24,32)
		palnum,			//パレット数
		compression,	//BMP圧縮タイプ
		maxR,maxG,maxB,maxA,	//16bit 各色の最大値
		convtype;		//イメージ変換タイプ

	uint32_t maskR,maskG,maskB,maskA;	//各色のビットマスク (0 で常に 0。A の場合は 0 なら不透明)
	uint8_t shiftR,shiftG,shiftB,shiftA,
		morethan_ver4,	//ver4 以降か
		end_of_bmp,		//RLE で EOB が出現したか
		rle_eof;		//RLE ファイルの最後まで読み込んだか
}_LOADBMP;

//--------------------

#define BI_RGB        0
#define BI_RLE8       1
#define BI_RLE4       2
#define BI_BITFIELDS  3

#define RET_OK        0
#define RET_ERR_NOMES -1  //メッセージはセット済み

#define RLE_BUFSIZE   1024

enum
{
	CONVTYPE_PALETTE,
	CONVTYPE_24BIT,
	CONVTYPE_32BIT_BGRA,
	CONVTYPE_BITFIELDS
};

//--------------------


//=======================
// 読み込み
//=======================


/** ファイルヘッダ読み込み */

static int _read_fileheader(_LOADBMP *p)
{
	uint8_t d[14];
	int size;

	size = mIOFileBuf_read(p->io, d, 14);
	
	//BMP ではない
	
	if(size < 2 || d[0] != 'B' || d[1] != 'M')
	{
		mLoadImage_setMessage(p->param, "This is not BMP");
		return RET_ERR_NOMES;
	}

	//データが足りない

	if(size != 14)
		return MLOADIMAGE_ERR_CORRUPT;

	return RET_OK;
}

/** マスク値からシフト値と値の最大値取得 */

static void _get_maskvalue(uint32_t mask,uint8_t *shift,int *max)
{
	if(mask == 0)
	{
		*shift = 0;
		*max = 0;
	}
	else
	{
		*shift = mGetBitOnPos(mask);
		*max = (1 << mGetBitOffPos(mask >> *shift)) - 1;
	}
}

/** ビットフィールド読み込み/セット */

static int _read_bitfield(_LOADBMP *p,int *readbytes)
{
	uint8_t d[16];
	int size;

	//RGBA マスク値

	if(p->compression == BI_BITFIELDS)
	{
		//指定あり (V4 以降ならアルファ値のマスクもある)

		size = (p->morethan_ver4)? 16: 12;

		if(!mIOFileBuf_readSize(p->io, d, size))
			return MLOADIMAGE_ERR_CORRUPT;

		p->maskR = mGetBuf32LE(d);
		p->maskG = mGetBuf32LE(d + 4);
		p->maskB = mGetBuf32LE(d + 8);

		if(size == 16)
		{
			p->maskA = mGetBuf32LE(d + 12);

			_get_maskvalue(p->maskA, &p->shiftA, &p->maxA);
		}

		*readbytes = size;
	}
	else
	{
		//指定なし = デフォルト値 (アルファマスクなし)

		if(p->bits == 16)
		{
			p->maskR = 0x7c00;
			p->maskG = 0x03e0;
			p->maskB = 0x001f;
		}
		else
		{
			p->maskR = 0xff0000;
			p->maskG = 0x00ff00;
			p->maskB = 0x0000ff;
		}
	}

	//RGB 各パラメータ

	_get_maskvalue(p->maskR, &p->shiftR, &p->maxR);
	_get_maskvalue(p->maskG, &p->shiftG, &p->maxG);
	_get_maskvalue(p->maskB, &p->shiftB, &p->maxB);

	return RET_OK;
}

/** 情報ヘッダ読み込み */

static int _read_infoheader(_LOADBMP *p,mLoadImageInfo *info)
{
	uint8_t d[40];
	int width,height,n,readbytes;
	uint32_t headersize,clrused;

	//読み込み

	if(!mIOFileBuf_readSize(p->io, d, 40))
		return MLOADIMAGE_ERR_CORRUPT;

	mConvertEndianBuf(d, -1, "44422444444");

	//------ 基本情報

	//ヘッダサイズ

	headersize = M_BUF_UINT32(d);
	if(headersize < 40) return MLOADIMAGE_ERR_UNSUPPORTED;  //OS/2 は非対応

	p->morethan_ver4 = (headersize >= 108);

	//圧縮形式

	n = p->compression = M_BUF_UINT32(d + 16);

	if(n < 0 || n > BI_BITFIELDS)
		return MLOADIMAGE_ERR_UNSUPPORTED;

	//幅・高さ

	width  = M_BUF_UINT32(d + 4);
	height = M_BUF_UINT32(d + 8);

	if(width <= 0 || height == 0)
		return MLOADIMAGE_ERR_INVALID_VALUE;

	if(height < 0)
	{
		//トップダウン
		height = -height;
		info->bottomup = FALSE;
	}
	else
		info->bottomup = TRUE;

	if((p->param->max_width > 0 && width > p->param->max_width)
		|| (p->param->max_height > 0 && height > p->param->max_height))
		return MLOADIMAGE_ERR_LARGE_SIZE;

	info->width  = width;
	info->height = height;

	//ビット数

	n = M_BUF_UINT16(d + 14);

	if(n != 1 && n != 4 && n != 8 && n != 16 && n != 24 && n != 32)
		return MLOADIMAGE_ERR_INVALID_VALUE;

	p->bits = n;

	//解像度

	info->resolution_unit = MLOADIMAGE_RESOLITION_UNIT_DPM;
	info->resolution_horz = M_BUF_UINT32(d + 24);
	info->resolution_vert = M_BUF_UINT32(d + 28);

	//パレット数

	n = clrused = M_BUF_UINT32(d + 32);

	if(p->bits >= 24)
		n = 0;
	else if(n == 0)
		n = 1 << p->bits;
	else if(n > (1 << p->bits))
		return MLOADIMAGE_ERR_INVALID_VALUE;

	p->palnum = n;

	//------- 他データ

	//ビットフィールド

	if(p->bits == 16 || p->bits == 32)
	{
		if((n = _read_bitfield(p, &readbytes)))
			return n;
	}

	//次の位置へ (パレット or イメージ)

	n = headersize - 40;
	if(p->compression == BI_BITFIELDS) n -= readbytes;

	mIOFileBuf_readEmpty(p->io, n);

	//16bit 以上の場合、カラーテーブルをスキップ

	if(p->bits >= 16 && clrused)
	{
		if(!mIOFileBuf_readEmpty(p->io, clrused << 2))
			return MLOADIMAGE_ERR_CORRUPT;
	}

	//------- 情報セット

	info->sample_bits = 8;
	info->raw_sample_bits = 8;
	info->raw_bits = p->bits;
	info->raw_pitch = (((info->width * p->bits + 7) >> 3) + 3) & (~3);

	//カラータイプ

	if(p->bits <= 8)
		info->raw_coltype = MLOADIMAGE_COLTYPE_PALETTE;
	else if(p->maskA)
		info->raw_coltype = MLOADIMAGE_COLTYPE_RGBA;
	else
		info->raw_coltype = MLOADIMAGE_COLTYPE_RGB;

	//イメージ変換タイプ

	if(p->bits <= 8)
		p->convtype = CONVTYPE_PALETTE;
	else if(p->bits == 24)
		p->convtype = CONVTYPE_24BIT;
	else if(p->bits == 32
		&& p->maskR == 0xff0000 && p->maskG == 0x00ff00 && p->maskB == 0x0000ff)
		p->convtype = CONVTYPE_32BIT_BGRA;
	else
		p->convtype = CONVTYPE_BITFIELDS;

	return RET_OK;
}

/** パレット読み込み */

static int _read_palette(_LOADBMP *p)
{
	int i,size;
	uint8_t *ppal,tmp;

	size = p->palnum << 2;

	//確保

	p->palbuf = (uint8_t *)mMalloc(size, FALSE);
	if(!p->palbuf) return MLOADIMAGE_ERR_ALLOC;

	//読み込み

	if(!mIOFileBuf_readSize(p->io, p->palbuf, size))
		return MLOADIMAGE_ERR_CORRUPT;

	//RGB 順入れ替え (BGRA => RGBA)

	ppal = p->palbuf;

	for(i = p->palnum; i > 0; i--, ppal += 4)
	{
		tmp = ppal[0];
		ppal[0] = ppal[2];
		ppal[2] = tmp;
		ppal[3] = 255;
	}

	return RET_OK;
}


//===========================
// RLE 展開
//===========================


/** RLE バッファに読み込み */

static int _read_to_rlebuf(_LOADBMP *p)
{
	int remain,read;

	remain = p->rlebuf_end - p->rlebuf_cur;

	if(p->rle_eof)
	{
		//ファイルの最後まで読み込んだ

		return (remain < 2)? MLOADIMAGE_ERR_CORRUPT: RET_OK;
	}
	else
	{
		//残りが 258byte 未満なら読み込み
		/* 非連続データの最大が 258byte なので、最大サイズ分を読み込んでおく */

		if(remain < 258)
		{
			memmove(p->rlebuf, p->rlebuf_cur, remain);

			read = mIOFileBuf_read(p->io, p->rlebuf + remain, RLE_BUFSIZE - remain);

			if(read < RLE_BUFSIZE - remain)
				p->rle_eof = TRUE;

			remain += read;
			if(remain < 2) return MLOADIMAGE_ERR_CORRUPT;
			
			p->rlebuf_end = p->rlebuf + remain;
			p->rlebuf_cur = p->rlebuf;
		}

		return RET_OK;
	}
}

/** RLE 展開 (4bit) */

static int _decompress_rle4(_LOADBMP *p)
{
	uint8_t *pd,*ps,b1,b2,val;
	int ret,width,x = 0,len,bytes,bytes_wd;

	pd = p->rawbuf;
	width = (p->param->info.width + 1) & (~1);  //奇数幅での +1px は許容

	while(1)
	{
		//読み込み

		ret = _read_to_rlebuf(p);
		if(ret) return ret;

		//2byte データ

		b1 = p->rlebuf_cur[0];
		b2 = p->rlebuf_cur[1];
		p->rlebuf_cur += 2;

		//

		if(b1 != 0)
		{
			//---- 連続データ ([1-255] + データ)

			len = b1;
			if(x + len > width) return MLOADIMAGE_ERR_DECODE;

			//出力位置が下位4bitの場合

			if(x & 1)
			{
				*(pd++) |= b2 >> 4;
				b2 = (b2 >> 4) | ((b2 & 15) << 4);
				len--;
			}

			//2px単位で出力

			for(; len > 0; len -= 2)
				*(pd++) = b2;

			//余分に出力した場合は 1px 戻る

			if(len == -1)
				*(--pd) &= 0xf0;

			x += b1;
		}
		else if(b2 == 0 || b2 == 1)
		{
			//行終わり (0 + 0) or イメージ終わり (0 + 1)

			memset(pd, 0, p->param->info.raw_pitch - (x + 1) / 2);
			if(b2 == 1) p->end_of_bmp = TRUE;

			break;
		}
		else if(b2 == 2)
			//位置移動 (0 + 2) は非対応
			return MLOADIMAGE_ERR_UNSUPPORTED;
		else
		{
			//----- 非連続データ (0 + [3-255] + データxlen)

			len = b2;
			bytes = (len + 1) >> 1;
			bytes_wd = (bytes + 1) & (~1);  //データは2byte境界

			if(x + len > width) return MLOADIMAGE_ERR_DECODE;

			if(p->rlebuf_end - p->rlebuf_cur < bytes_wd)
				return MLOADIMAGE_ERR_CORRUPT;

			ps = p->rlebuf_cur;

			if(x & 1)
			{
				//出力先の先頭が下位4bit

				for(; len > 0; len -= 2)
				{
					val = *(ps++);

					*(pd++) |= val >> 4;
					if(len != 1) *pd = (val & 15) << 4;
				}
			}
			else
			{
				//先頭が上位4bit

				memcpy(pd, ps, bytes);

				pd += bytes;

				if(len & 1)
					*(--pd) &= 0xf0;
			}
			
			x += b2;
			p->rlebuf_cur += bytes_wd;
		}
	}

	return RET_OK;
}

/** RLE 展開 (8bit) */

static int _decompress_rle8(_LOADBMP *p)
{
	uint8_t *pd,b1,b2;
	int ret,width,x = 0,bytes_wd;

	pd = p->rawbuf;
	width = p->param->info.width;

	while(1)
	{
		//読み込み

		ret = _read_to_rlebuf(p);
		if(ret) return ret;

		//2byte データ

		b1 = p->rlebuf_cur[0];
		b2 = p->rlebuf_cur[1];
		p->rlebuf_cur += 2;

		//

		if(b1 != 0)
		{
			//連続データ

			if(x + b1 > width) return MLOADIMAGE_ERR_DECODE;

			memset(pd, b2, b1);

			pd += b1;
			x += b1;
		}
		else if(b2 == 0 || b2 == 1)
		{
			//行終わり (0 + 0) or イメージ終わり (0 + 1)

			memset(pd, 0, p->param->info.raw_pitch - x);
			if(b2 == 1) p->end_of_bmp = TRUE;
			
			break;
		}
		else if(b2 == 2)
			//位置移動 (0 + 2) は非対応
			return MLOADIMAGE_ERR_UNSUPPORTED;
		else
		{
			//----- 非連続データ (0 + [3-255] + データxlen)

			if(x + b2 > width) return MLOADIMAGE_ERR_DECODE;

			bytes_wd = (b2 + 1) & (~1);

			if(p->rlebuf_end - p->rlebuf_cur < bytes_wd)
				return MLOADIMAGE_ERR_CORRUPT;

			memcpy(pd, p->rlebuf_cur, b2);

			p->rlebuf_cur += bytes_wd;

			pd += b2;
			x += b2;
		}
	}

	return RET_OK;
}


//=======================
// sub
//=======================


/** イメージ変換 */

static void _convert_image(_LOADBMP *p)
{
	int i,n,bits,shift,mask,convtype;
	uint32_t c;
	uint8_t *ps,*pd,*ppal,r,g,b,a;
	mBool to_rgba;

	bits = p->bits;
	convtype = p->convtype;
	to_rgba = (p->param->format == MLOADIMAGE_FORMAT_RGBA);

	ps = p->rawbuf;
	pd = p->convbuf;

	if(bits <= 8)
	{
		shift = 8 - bits;
		mask = (1 << bits) - 1;
	}

	//変換

	for(i = p->param->info.width; i > 0; i--)
	{
		r = g = b = 0;
		a = 255;
	
		switch(convtype)
		{
			//24bit (B-G-R)
			case CONVTYPE_24BIT:
				r = ps[2];
				g = ps[1];
				b = ps[0];
				
				ps += 3;
				break;
			//32bit (B-G-R-A)
			case CONVTYPE_32BIT_BGRA:
				r = ps[2];
				g = ps[1];
				b = ps[0];
				if(p->maskA) a = ps[3];

				ps += 4;
				break;
			//16bit/32bit (bitfield)
			case CONVTYPE_BITFIELDS:
				if(bits == 32)
				{
					c = ((uint32_t)ps[3] << 24) | (ps[2] << 16) | (ps[1] << 8) | ps[0];
					ps += 4;
				}
				else
				{
					c = (ps[1] << 8) | ps[0];
					ps += 2;
				}

				if(p->maskR) r = ((c & p->maskR) >> p->shiftR) * 255 / p->maxR;
				if(p->maskG) g = ((c & p->maskG) >> p->shiftG) * 255 / p->maxG;
				if(p->maskB) b = ((c & p->maskB) >> p->shiftB) * 255 / p->maxB;
				if(p->maskA) a = ((c & p->maskA) >> p->shiftA) * 255 / p->maxA;
				break;
			//パレットカラー
			default:
				n = (*ps >> shift) & mask;
				if(n >= p->palnum) n = 0;

				ppal = p->palbuf + (n << 2);

				r = ppal[0];
				g = ppal[1];
				b = ppal[2];

				shift -= bits;
				if(shift < 0)
				{
					shift = 8 - bits;
					ps++;
				}
				break;
		}

		//セット

		pd[0] = r;
		pd[1] = g;
		pd[2] = b;

		if(to_rgba)
		{
			pd[3] = a;
			pd += 4;
		}
		else
			pd += 3;
	}
}

/** イメージまでの情報読み込み */

static int _read_headers(_LOADBMP *p)
{
	int ret;

	//開く

	p->io = mLoadImage_openIOFileBuf(&p->param->src);
	if(!p->io) return MLOADIMAGE_ERR_OPENFILE;

	//ファイルヘッダ

	if((ret = _read_fileheader(p)))
		return ret;

	//情報ヘッダ

	if((ret = _read_infoheader(p, &p->param->info)))
		return ret;

	//パレット

	if(p->bits <= 8)
	{
		if((ret = _read_palette(p)))
			return ret;

		p->param->info.palette_num = p->palnum;
		p->param->info.palette_buf = p->palbuf;
	}

	return RET_OK;
}

/** 作業用メモリ確保 */

static int _alloc_buf(_LOADBMP *p,mLoadImage *param)
{
	//Y1行、生データ

	p->rawbuf = (uint8_t *)mMalloc(param->info.raw_pitch, FALSE);
	if(!p->rawbuf) return MLOADIMAGE_ERR_ALLOC;

	//Y1行、変換後データ

	if(param->format != MLOADIMAGE_FORMAT_RAW)
	{
		p->convbuf = (uint8_t *)mMalloc(mLoadImage_getPitch(param), FALSE);
		if(!p->convbuf) return MLOADIMAGE_ERR_ALLOC;
	}

	//RLE 用

	if(p->compression == BI_RLE4 || p->compression == BI_RLE8)
	{
		p->rlebuf = (uint8_t *)mMalloc(RLE_BUFSIZE, FALSE);
		if(!p->rlebuf) return MLOADIMAGE_ERR_ALLOC;

		p->rlebuf_end = p->rlebuf_cur = p->rlebuf;
	}

	return RET_OK;
}

/** メイン処理 */

static int _main_proc(_LOADBMP *p)
{
	int ret,i,pitch_dst,pitch_raw,rle;
	mLoadImage *param = p->param;
	
	//ヘッダ部分読み込み

	if((ret = _read_headers(p)))
		return ret;

	//メモリ確保

	if((ret = _alloc_buf(p, param)))
		return ret;

	//getinfo()

	if(param->getinfo)
	{
		if((ret = (param->getinfo)(param, &param->info)))
			return (ret < 0)? RET_ERR_NOMES: ret;
	}
	
	//イメージ読み込み

	pitch_raw = param->info.raw_pitch;
	pitch_dst = mLoadImage_getPitch(param);
	
	rle = (p->compression == BI_RLE4 || p->compression == BI_RLE8);

	for(i = param->info.height; i > 0; i--)
	{
		//p->rawbuf に生データ読み込み

		if(rle)
		{
			//RLE 圧縮

			if(p->end_of_bmp)
				memset(p->rawbuf, 0, pitch_raw);
			else
			{
				if(p->compression == BI_RLE4)
					ret = _decompress_rle4(p);
				else
					ret = _decompress_rle8(p);

				if(ret) return ret;
			}
		}
		else
		{
			//無圧縮

			if(!mIOFileBuf_readSize(p->io, p->rawbuf, pitch_raw))
				return MLOADIMAGE_ERR_CORRUPT;
		}

		//getrow()

		if(param->format == MLOADIMAGE_FORMAT_RAW)
			ret = (param->getrow)(param, p->rawbuf, pitch_dst);
		else
		{
			_convert_image(p);

			ret = (param->getrow)(param, p->convbuf, pitch_dst);
		}

		if(ret)
			return (ret < 0)? RET_ERR_NOMES: ret;
	}

	return RET_OK;
}


//========================
// main
//========================


/** BMP 読み込み
 *
 * @ingroup loadimage */

mBool mLoadImageBMP(mLoadImage *param)
{
	_LOADBMP *p;
	int ret;

	//確保

	p = (_LOADBMP *)mMalloc(sizeof(_LOADBMP), TRUE);
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
	
	mIOFileBuf_close(p->io);

	mFree(p->rawbuf);
	mFree(p->convbuf);
	mFree(p->palbuf);
	mFree(p->rlebuf);
	mFree(p);

	return (ret == RET_OK);
}

