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
 * GIF エンコーダ
 *****************************************/

#include <stdio.h>
#include <string.h>

#include "mDef.h"
#include "mGIFEncode.h"
#include "mUtilStdio.h"
#include "mUtil.h"


//------------------

struct _mGIFEncode
{
	mGIFEncodeGlobalInfo global;
	
	FILE *fp;	
	mBool need_fp_close;

	uint8_t *lzw_out;		//LZW 出力バッファ (1ブロック分。[0]長さ [1..255]データ)
	uint32_t *lzw_hashtbl;	//LZW ハッシュテーブル
	int clearcode,
		endcode,
		runcode,
		runbits,
		depth,
		maxcode,
		shift,
		outpos,
		curcode;
	uint32_t curval;
};

//------------------

#define LZW_HASHTBL_SIZE      8192
#define LZW_HASHTBL_KEY_MASK  (LZW_HASHTBL_SIZE - 1)

//------------------


//=========================
// LZW
//=========================


/** バイト書き出し */

static void _lzw_putbyte(mGIFEncode *p,int ch)
{
	//1byte 出力

	p->lzw_out[1 + p->outpos] = ch;
	p->outpos++;

	//ブロックのバッファが一杯なら書き出し

	if(p->outpos == 255)
	{
		p->lzw_out[0] = 255;
		p->outpos = 0;

		fwrite(p->lzw_out, 1, 256, p->fp);
	}
}

/** コード書き出し */

static void _lzw_putcode(mGIFEncode *p,int code)
{
	p->curval |= code << p->shift;
	p->shift += p->runbits;

	while(p->shift >= 8)
	{
		_lzw_putbyte(p, p->curval & 0xff);

		p->curval >>= 8;
		p->shift -= 8;
	}

	if(p->runcode >= p->maxcode && code <= 4095)
	{
		p->runbits++;
		p->maxcode = 1 << p->runbits;
	}
}

/** ハッシュテーブル検索 */

static int _lzw_search_hash(mGIFEncode *p,uint32_t key)
{
	int hkey = ((key >> 12) ^ key) & LZW_HASHTBL_KEY_MASK;
	uint32_t htkey;

	while(1)
	{
		htkey = p->lzw_hashtbl[hkey] >> 12;

		if(htkey == 0xfffff)
			break;
		else if(key == htkey)
			return p->lzw_hashtbl[hkey] & 0xfff;
		else
			hkey = (hkey + 1) & LZW_HASHTBL_KEY_MASK;
	}

	return -1;
}

/** LZW 初期化 */

static void _lzw_init(mGIFEncode *p)
{
	memset(p->lzw_hashtbl, 0xff, LZW_HASHTBL_SIZE * 4);

	p->depth     = (p->global.bits < 2)? 2: p->global.bits;
	p->clearcode = 1 << p->depth;
	p->endcode   = p->clearcode + 1;
	p->runcode   = p->endcode + 1;
	p->runbits   = p->depth + 1;
	p->maxcode   = 1 << p->runbits;
	p->shift     = 0;
	p->curval    = 0;
	p->outpos    = 0;
	p->curcode   = -1;

	//LZW 最小ビット

	mFILEwriteByte(p->fp, p->depth);

	_lzw_putcode(p, p->clearcode);
}

/** LZW 終了 */

static void _lzw_finish(mGIFEncode *p)
{
	_lzw_putcode(p, p->curcode);
	_lzw_putcode(p, p->endcode);

	//残りの値を出力
	
	while(p->shift > 0)
	{
		_lzw_putbyte(p, p->curval & 0xff);

		p->curval >>= 8;
		p->shift -= 8;
	}

	//残りデータを書き込み

	if(p->outpos)
	{
		p->lzw_out[0] = p->outpos;
		fwrite(p->lzw_out, 1, p->outpos + 1, p->fp);
	}

	//サブブロック終了

	mFILEwriteByte(p->fp, 0);
}

/** LZW 圧縮 */

static void _lzw_encode(mGIFEncode *p,uint8_t *buf,int bufsize)
{
	uint8_t *ps,*psend;
	int val,ccode,newcode,hkey;
	uint32_t newkey;

	ps = buf;
	psend = buf + bufsize;

	ccode = p->curcode;
	if(ccode < 0) ccode = *(ps++);

	while(ps < psend)
	{
		val = *(ps++);

		newkey  = ((uint32_t)ccode << p->depth) + val;
		newcode = _lzw_search_hash(p, newkey);

		if(newcode >= 0)
			ccode = newcode;
		else
		{
			_lzw_putcode(p, ccode);

			ccode = val;

			if(p->runcode >= 4095)
			{
				_lzw_putcode(p, p->clearcode);

				p->runcode = p->endcode + 1;
				p->runbits = p->depth + 1;
				p->maxcode = 1 << p->runbits;

				memset(p->lzw_hashtbl, 0xff, LZW_HASHTBL_SIZE * 4);
			}
			else
			{
				hkey = ((newkey >> 12) ^ newkey) & LZW_HASHTBL_KEY_MASK;

				while((p->lzw_hashtbl[hkey] >> 12) != 0xfffff)
					hkey = (hkey + 1) & LZW_HASHTBL_KEY_MASK;

				p->lzw_hashtbl[hkey] = (newkey << 12) | (p->runcode & 0xfff);
				p->runcode++;
			}
		}
	}

	p->curcode = ccode;
}


//=========================
// sub
//=========================


/** パレット書き込み */

static void _write_palette(mGIFEncode *p,uint8_t *buf,int num,int bits)
{
	//書き込み

	fwrite(buf, 1, num * 3, p->fp);

	//余白書き込み

	mFILEwriteZero(p->fp, ((1 << bits) - num) * 3);
}


//=========================
// main
//=========================


/**************//**

@defgroup gifencode mGIFEncode
@brief GIF エンコーダ

<header> <[grpctrl] image>... <end>

@ingroup group_image
@{

@file mGIFEncode.h

@struct mGIFEncodeGlobalInfo
@struct mGIFEncodeImage
@enum MGIFENCODE_BKGNDTYPE

********************/


/** GIF エンコーダ閉じる */

void mGIFEncode_close(mGIFEncode *p)
{
	if(p)
	{
		if(p->fp)
		{
			if(p->need_fp_close)
				fclose(p->fp);
			else
				fflush(p->fp);
		}

		mFree(p->lzw_out);
		mFree(p->lzw_hashtbl);
		mFree(p);
	}
}

/** GIF エンコーダ作成
 *
 * @param info  このデータは内部でコピーされる */

mGIFEncode *mGIFEncode_create(mGIFEncodeGlobalInfo *info)
{
	mGIFEncode *p;

	//mGIFEncode

	p = (mGIFEncode *)mMalloc(sizeof(mGIFEncode), TRUE);
	if(!p) return NULL;

	p->global = *info;

	//LZW 用バッファ

	p->lzw_out = (uint8_t *)mMalloc(256, FALSE);
	p->lzw_hashtbl = (uint32_t *)mMalloc(LZW_HASHTBL_SIZE * 4, FALSE);

	if(!p->lzw_out || !p->lzw_hashtbl)
	{
		mGIFEncode_close(p);
		return NULL;
	}

	return p;
}

/** ファイル名から開く */

mBool mGIFEncode_open_filename(mGIFEncode *p,const char *filename)
{
	p->fp = mFILEopenUTF8(filename, "wb");
	p->need_fp_close = TRUE;

	return (p->fp != NULL);
}

/** FILE* から開く */

mBool mGIFEncode_open_stdio(mGIFEncode *p,void *fp)
{
	p->fp = (FILE *)fp;

	return TRUE;
}

/** 色数からビット数取得 */

int mGIFEncode_colnumToBits(int num)
{
	int i,bits;

	for(i = 128, bits = 8; bits > 1; i >>= 1, bits--)
	{
		if(num > i) return bits;
	}

	return bits;
}

/** ヘッダ書き込み */

void mGIFEncode_writeHeader(mGIFEncode *p)
{
	mGIFEncodeGlobalInfo *pg = &p->global;
	uint8_t d[13],flags;
	int pal_bits;

	//フラグ

	flags = (pg->bits - 1) << 4;

	if(pg->palette_buf)
	{
		pal_bits = mGIFEncode_colnumToBits(pg->palette_num);
		flags |= 0x80 | (pal_bits - 1);
	}

	//ヘッダ書き込み

	mSetBufLE_args(d, "s22111",
		"GIF89a",
		pg->width,
		pg->height,
		flags,
		pg->bkgnd_index,
		0);

	fwrite(d, 1, 13, p->fp);

	//グローバルパレット

	if(pg->palette_buf)
		_write_palette(p, pg->palette_buf, pg->palette_num, pal_bits);
}

/** 画像制御の拡張ブロックを書き込む
 *
 * 適用したい画像の前に書き込む。
 *
 * @param transparent  透過色のインデックス。負の値でなし。
 * @param bkgnd_type   背景の処理タイプ
 * @param time         遅延時間 (10ms 単位)
 * @param user_input   ユーザーの入力を受け付けるか */

void mGIFEncode_writeGrpCtrl(mGIFEncode *p,
	int transparent,int bkgnd_type,uint16_t time,mBool user_input)
{
	uint8_t d[8] = {0x21,0xf9,0x04,0,0,0,0,0};

	//フラグ

	d[3] = (transparent >= 0)
		| ((user_input != 0) << 1)
		| (bkgnd_type << 2);

	//時間 (10ms 単位)

	d[4] = (uint8_t)time;
	d[5] = (uint8_t)(time >> 8);

	//透過色

	if(transparent >= 0)
		d[6] = transparent;

	fwrite(d, 1, 8, p->fp);
}

/** ループ用の拡張ブロックを書き込む
 *
 * @param loopcnt ループ回数。0 で無限 */

void mGIFEncode_writeLoop(mGIFEncode *p,uint16_t loopcnt)
{
	uint8_t d[19] = {
		0x21,0xff,
		11,'N','E','T','S','C','A','P','E','2','.','0',
		3,1,0,0, 0};

	d[16] = (uint8_t)loopcnt;
	d[17] = (uint8_t)(loopcnt >> 8);

	fwrite(d, 1, 19, p->fp);
}

/** GIF の終端を書き込み */

void mGIFEncode_writeEnd(mGIFEncode *p)
{
	mFILEwriteByte(p->fp, 0x3b);
}

/** イメージ書き込みの開始 */

void mGIFEncode_startImage(mGIFEncode *p,mGIFEncodeImage *img)
{
	uint8_t d[10];
	int pal_bits;

	//----- ヘッダ

	mSetBufLE_args(d, "12222",
		0x2c, img->offsetx, img->offsety, img->width, img->height);

	//フラグ

	if(!img->palette_buf)
		d[9] = 0;
	else
	{
		pal_bits = mGIFEncode_colnumToBits(img->palette_num);
		d[9] = 0x80 | (pal_bits - 1);
	}

	fwrite(d, 1, 10, p->fp);

	//ローカルパレット

	if(img->palette_buf)
		_write_palette(p, img->palette_buf, img->palette_num, pal_bits);

	//----- LZW 初期化

	_lzw_init(p);
}

/** イメージを好きなサイズずつ書き込み
 *
 * @param buf  8bit パレットインデックス (全体で width x height byte) */

void mGIFEncode_writeImage(mGIFEncode *p,uint8_t *buf,int size)
{
	_lzw_encode(p, buf, size);
}

/** イメージ書き込みを終了 */

void mGIFEncode_endImage(mGIFEncode *p)
{
	_lzw_finish(p);
}

/* @} */
