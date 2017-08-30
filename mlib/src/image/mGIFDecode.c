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
 * mGIFDecode
 *****************************************/

#include <string.h>

#include "mDef.h"
#include "mGIFDecode.h"
#include "mIOFileBuf.h"
#include "mUtil.h"


//--------------------

struct _mGIFDecode
{
	mIOFileBuf *io;
	mGIFDecodeErrorFunc error_func;

	uint8_t *palbuf[2],		//[0]global [1]local
		*imgbuf,
		*currow;
	void *error_ptr;

	int width,
		height,
		bits,
		palnum[2],			//各パレット個数
		transparent,		//透過色インデックス (-1 でなし)
		row_remain;			//読み込み残り行数
	uint8_t bkgnd,			//背景色インデックス
		palette_no,			//現在のイメージのパレット (0:global, 1:local)
		img_destroy_type;	//画像の破棄方法

	/*--- lzw ---*/

	uint8_t *lzwbuf, /* 255:block, 4096:stack, 4096:suffix, 4096x2:prefix */
		*lzw_curbuf;

	int offx,offy,
		imgw,imgh,
		curx,cury,
		lzw_leftbits,
		lzw_curbits,
		lzw_blockremain;
	uint8_t lzw_readbyte,
		interlace_pass;
};

//-------------------

#define _PALETTENO_GLOBAL  0
#define _PALETTENO_LOCAL   1

enum
{
	_IMG_DESTROY_TYPE_NONE,
	_IMG_DESTROY_TYPE_STAY,
	_IMG_DESTROY_TYPE_FILL_BKGND,
	_IMG_DESTROY_TYPE_RESTORE
};

//--------------------

enum
{
	GIFERR_NO_IMAGE = -1,
	GIFERR_OK = 0,
	GIFERR_ALLOC,
	GIFERR_CORRUPT,
	GIFERR_NO_GIF,
	GIFERR_UNSUPPORTED_SIG,
};

const char *g_errmes[] = {
	"memory allocation to fail",
	"data is corrupted",
	"this is not GIF",
	"unsupported signature"
};

//--------------------


//**********************************
// LZW 展開
//**********************************


/** 次のバイトを取得 */

static mBool _lzw_nextbyte(mGIFDecode *p)
{
	uint8_t size;

	//次のブロックを読み込み

	if(p->lzw_blockremain <= 0)
	{
		if(!mIOFileBuf_readByte(p->io, &size) || size == 0)
			return FALSE;

		if(!mIOFileBuf_readSize(p->io, p->lzwbuf, size))
			return FALSE;

		p->lzw_blockremain = size;
		p->lzw_curbuf = p->lzwbuf;
	}

	//次のバイト

	p->lzw_readbyte = *(p->lzw_curbuf++);

	p->lzw_blockremain--;

	return TRUE;
}

/** 次のコードを取得
 *
 * @return -1 でデータが足りない */

static int _lzw_nextcode(mGIFDecode *p)
{
	int ret;

	if(p->lzw_leftbits == 0)
	{
		if(!_lzw_nextbyte(p)) return -1;

		p->lzw_leftbits = 8;
	}

	ret = p->lzw_readbyte >> (8 - p->lzw_leftbits);

	//値が次のバイトをまたぐ場合

	while(p->lzw_curbits > p->lzw_leftbits)
	{
		if(!_lzw_nextbyte(p)) return -1;

		ret |= p->lzw_readbyte << p->lzw_leftbits;

		p->lzw_leftbits += 8;
	}

	p->lzw_leftbits -= p->lzw_curbits;

	return ret & ((1 << p->lzw_curbits) - 1);
}

/** 点をセット
 *
 * @return 1 ですべての点を埋めた */

static int _lzw_setpixel(mGIFDecode *p,int c)
{
	int x,y;
	uint8_t pat[5][2] = {{1,0},{8,0},{8,4},{4,2},{2,1}};

	//色を置く

	x = p->offx + p->curx;
	y = p->offy + p->cury;

	if(x >= 0 && x < p->width && y >= 0 && y < p->height)
		*(p->imgbuf + y * p->width + x) = c;

	//次の位置

	p->curx++;

	if(p->curx >= p->imgw)
	{
		p->curx = 0;
		p->cury += pat[p->interlace_pass][0];

		while(p->cury >= p->imgh && p->interlace_pass >= 1 && p->interlace_pass <= 3)
		{
			p->interlace_pass++;
			p->cury = pat[p->interlace_pass][1];
		}

		if(p->cury >= p->imgh) return 1;
	}

	return 0;
}

/** 展開メイン処理 */

static int _lzw_main(mGIFDecode *p,int minbits)
{
	uint8_t *sp,*stack,*suffix;
	uint16_t *prefix;
	int c,oc,fc,code;
	int clearcode,endcode,newcode,topslot,slot;

	stack = p->lzwbuf + 255;
	suffix = p->lzwbuf + 255 + 4096;
	prefix = (uint16_t *)(p->lzwbuf + 255 + 4096 * 2);

	//

	topslot = 1 << p->lzw_curbits;
	clearcode = 1 << minbits;
	endcode = clearcode + 1;
	slot = newcode = endcode + 1;

	//

	sp = stack;
	oc = fc = 0;

	while(1)
	{
		c = _lzw_nextcode(p);
		if(c == -1) return GIFERR_CORRUPT;

		if(c == endcode)
			//終了コード
			break;
		else if(c == clearcode)
		{
			//クリアコード

			p->lzw_curbits = minbits + 1;
			slot = newcode;
			topslot = 1 << p->lzw_curbits;

			while((c = _lzw_nextcode(p)) == clearcode);

			if(c == -1) return GIFERR_CORRUPT;
			if(c == endcode) break;

			if(c >= slot) c = 0;

			oc = fc = c;

			if(_lzw_setpixel(p, c)) break;
		}
		else
		{
			code = c;

			if(code >= slot)
			{
				code = oc;
				*(sp++) = fc;
			}

			while(code >= newcode)
			{
				*(sp++) = suffix[code];
				code = prefix[code];
			}

			*(sp++) = code;

			if(slot < topslot)
			{
				suffix[slot] = fc = code;
				prefix[slot++] = oc;
				oc = c;
			}

			if(slot >= topslot)
			{
				if(p->lzw_curbits < 12)
				{
					topslot <<= 1;
					p->lzw_curbits++;
				}
			}

			while(sp > stack)
			{
				if(_lzw_setpixel(p, *(--sp)))
					return GIFERR_OK;
			}
		}
	}

	return GIFERR_OK;
}

/** LZW 展開開始 */

static int _decode_lzw(mGIFDecode *p,
	int offx,int offy,int imgw,int imgh,int interlace)
{
	int ret;
	uint8_t minbits;

	p->offx = offx;
	p->offy = offy;
	p->imgw = imgw;
	p->imgh = imgh;
	p->interlace_pass = (interlace)? 1: 0;

	//LZW 最小ビット長

	if(!mIOFileBuf_readByte(p->io, &minbits)
		|| minbits < 2 || minbits > 9)
		return GIFERR_CORRUPT;

	//処理

	p->lzw_leftbits = 0;
	p->lzw_blockremain = 0;
	p->lzw_curbits = minbits + 1;

	ret = _lzw_main(p, minbits);

	//成功時、ブロック終了のバイトをスキップ

	if(ret == GIFERR_OK)
	{
		if(!mIOFileBuf_readEmpty(p->io, 1))
			ret = GIFERR_CORRUPT;
	}

	return ret;
}



//**********************************
// mGIFDecode
//**********************************


//===========================
// sub
//===========================


/** デフォルトエラー関数 */

static void _error_func(mGIFDecode *p,const char *mes,void *param)
{

}

/** エラー処理 */

static void _gif_error(mGIFDecode *p,int code)
{
	(p->error_func)(p, g_errmes[code - 1], p->error_ptr);
}

/** パレット読み込み
 *
 * イメージ読み込み後に行うこと。 */

static int _read_palette(mGIFDecode *p,uint8_t flags,int palno)
{
	int num;
	uint8_t *ps,*pd,r,g,b;

	pd = p->palbuf[palno];

	//個数

	num = 1 << ((flags & 7) + 1);

	p->palnum[palno] = num;

	//読み込み

	if(!mIOFileBuf_readSize(p->io, pd, num * 3))
		return GIFERR_CORRUPT;

	//変換 (3byte => 4byte)

	ps = pd + (num - 1) * 3;
	pd += (num - 1) << 2;

	for(; num > 0; num--, ps -= 3, pd -= 4)
	{
		r = ps[0], g = ps[1], b = ps[2];
		pd[0] = r, pd[1] = g, pd[2] = b, pd[3] = 255;
	}

	return GIFERR_OK;
}

/** ヘッダ読み込み */

static int _read_header(mGIFDecode *p)
{
	uint8_t d[13],flags;
	int ret;

	if(!mIOFileBuf_readSize(p->io, d, 13))
		return GIFERR_CORRUPT;

	//ヘッダ

	if(d[0] != 'G' || d[1] != 'I' || d[2] != 'F'
		|| d[3] != '8'
		|| (d[4] != '7' && d[4] != '9')
		|| d[5] != 'a')
		return GIFERR_NO_GIF;

	//幅・高さ

	p->width  = mGetBuf16LE(d + 6);
	p->height = mGetBuf16LE(d + 8);

	//フラグ

	flags = d[10];

	//ビット数

	p->bits = ((flags >> 4) & 7) + 1;

	//背景色

	p->bkgnd = d[11];

	//グローバルパレット

	if(flags & 0x80)
	{
		ret = _read_palette(p, flags, _PALETTENO_GLOBAL);
		if(ret) return ret;
	}

	return GIFERR_OK;
}


//===========================
// 各ブロック処理
//===========================


/** 画像を読み込み */

static int _read_image(mGIFDecode *p)
{
	uint8_t d[9],flags;
	int ret;

	if(!mIOFileBuf_readSize(p->io, d, 9))
		return GIFERR_CORRUPT;

	flags = d[8];

	//ローカルパレット

	if(flags & 0x80)
	{
		ret = _read_palette(p, flags, _PALETTENO_LOCAL);
		if(ret) return ret;

		p->palette_no = _PALETTENO_LOCAL;
	}
	else
		p->palette_no = _PALETTENO_GLOBAL;

	//背景塗りつぶし

	if(p->img_destroy_type == _IMG_DESTROY_TYPE_FILL_BKGND)
	{
		memset(p->imgbuf,
			(p->transparent != -1)? p->transparent: p->bkgnd,
			p->width * p->height);
	}

	//LZW 展開

	return _decode_lzw(p, mGetBuf16LE(d), mGetBuf16LE(d + 2),
				mGetBuf16LE(d + 4), mGetBuf16LE(d + 6), flags & 0x40);
}

/** グラフィック制御ブロックを読み込み */

static int _read_graphic_ctrl(mGIFDecode *p)
{
	uint8_t d[5];

	if(!mIOFileBuf_readSize(p->io, d, 5) || d[0] < 4)
		return GIFERR_CORRUPT;

	//画像の破棄方法

	p->img_destroy_type = (d[1] >> 2) & 7;

	//透過色

	if(d[1] & 1)
		p->transparent = d[4];

	if(!mIOFileBuf_readEmpty(p->io, d[0] - 4))
		return GIFERR_CORRUPT;

	return GIFERR_OK;
}

/** サブブロックをスキップ */

static int _skip_subblock(mGIFDecode *p)
{
	uint8_t size;

	while(1)
	{
		if(!mIOFileBuf_readByte(p->io, &size))
			return GIFERR_CORRUPT;

		if(size == 0) break;

		if(!mIOFileBuf_readEmpty(p->io, size))
			return GIFERR_CORRUPT;
	}

	return GIFERR_OK;
}

/** 次の画像ブロックへ移動 */

static int _move_nextimage(mGIFDecode *p)
{
	uint8_t sig,subsig;
	int ret;

	while(mIOFileBuf_readByte(p->io, &sig))
	{
		switch(sig)
		{
			//画像ブロック
			case 0x2c:
				return GIFERR_OK;
			//終端
			case 0x3b:
				return GIFERR_NO_IMAGE;
			//サブブロック付きのブロック
			case 0x21:
				//ブロック識別子
				
				if(!mIOFileBuf_readByte(p->io, &subsig))
					return GIFERR_CORRUPT;

				//グラフィック制御ブロック

				if(subsig == 0xf9)
				{
					ret = _read_graphic_ctrl(p);
					if(ret) return ret;
				}

				//次のブロックまでスキップ
				
				ret = _skip_subblock(p);
				if(ret) return ret;
				break;
			//ほか
			default:
				return GIFERR_UNSUPPORTED_SIG;
		}
	}

	return GIFERR_NO_IMAGE;
}


//===========================
// main
//===========================


/**

@defgroup gifdecode mGIFDecode
@brief GIF 画像読み込み

@ingroup group_image
@{

@file mGIFDecode.h
@enum MGIFDECODE_FORMAT

*/


/** 閉じる */

void mGIFDecode_close(mGIFDecode *p)
{
	if(p)
	{
		mIOFileBuf_close(p->io);

		mFree(p->lzwbuf);
		mFree(p->imgbuf);
		mFree(p->palbuf[0]);
		mFree(p->palbuf[1]);
		
		mFree(p);
	}
}

/** 構造体を作成 */

mGIFDecode *mGIFDecode_create()
{
	mGIFDecode *p;

	//確保

	p = (mGIFDecode *)mMalloc(sizeof(mGIFDecode), TRUE);
	if(!p) return NULL;

	p->error_func = _error_func;

	//パレットバッファ確保

	p->palbuf[0] = (uint8_t *)mMalloc(256 * 4, TRUE);
	p->palbuf[1] = (uint8_t *)mMalloc(256 * 4, TRUE);

	if(!p->palbuf[0] || !p->palbuf[1])
	{
		mGIFDecode_close(p);
		return NULL;
	}

	//LZW バッファ確保

	p->lzwbuf = (uint8_t *)mMalloc(255 + 4096 * 2 + 4096 * 2, FALSE);

	if(!p->lzwbuf)
	{
		mGIFDecode_close(p);
		return NULL;
	}

	return p;
}

/** ファイルを開く */

mBool mGIFDecode_open_filename(mGIFDecode *p,const char *filename)
{
	p->io = mIOFileBuf_openRead_filename(filename);

	return (p->io != NULL);
}

/** バッファを開く */

mBool mGIFDecode_open_buf(mGIFDecode *p,const void *buf,uint32_t bufsize)
{
	p->io = mIOFileBuf_openRead_buf(buf, bufsize);
	return (p->io != NULL);
}

/** FILE* から開く */

mBool mGIFDecode_open_stdio(mGIFDecode *p,void *fp)
{
	p->io = mIOFileBuf_openRead_FILE(fp);
	return (p->io != NULL);
}

/** エラー時の関数をセット
 *
 * @param param エラー関数に渡されるポインタ */

void mGIFDecode_setErrorFunc(mGIFDecode *p,mGIFDecodeErrorFunc func,void *param)
{
	p->error_func = func;
	p->error_ptr = param;
}

/** ヘッダを読み込み
 *
 * @param size  画像サイズが入る */

mBool mGIFDecode_readHeader(mGIFDecode *p,mSize *size)
{
	int ret;

	//ヘッダ読み込み

	ret = _read_header(p);
	if(ret)
	{
		_gif_error(p, ret);
		return FALSE;
	}

	//画像サイズ

	size->w = p->width;
	size->h = p->height;

	//画像バッファ確保

	p->imgbuf = (uint8_t *)mMalloc(p->width * p->height, FALSE);

	if(!p->imgbuf)
	{
		_gif_error(p, GIFERR_ALLOC);
		return FALSE;
	}

	return TRUE;
}

/** パレットデータ取得
 *
 * @param pnum  パレット個数が入る
 * @return パレットバッファのポインタ。R-G-B-A 順。 */

uint8_t *mGIFDecode_getPalette(mGIFDecode *p,int *pnum)
{
	*pnum = p->palnum[p->palette_no];

	return p->palbuf[p->palette_no];
}

/** 透過色を RGB 値で取得
 *
 * @return -1 でなし */

int mGIFDecode_getTransparentColor(mGIFDecode *p)
{
	uint8_t *ppal;

	if(p->transparent != -1 && p->transparent < p->palnum[p->palette_no])
	{
		ppal = p->palbuf[p->palette_no] + (p->transparent << 2);

		return (ppal[0] << 16) | (ppal[1] << 8) | ppal[2];
	}

	return -1;
}

/** 次のイメージ読み込み
 *
 * @return [1]成功 [0]イメージがない [-1]エラー */

int mGIFDecode_nextImage(mGIFDecode *p)
{
	int ret;

	p->transparent = -1;
	p->img_destroy_type = _IMG_DESTROY_TYPE_FILL_BKGND;

	//次のイメージへ移動

	ret = _move_nextimage(p);

	if(ret == GIFERR_NO_IMAGE)
		return 0;
	else if(ret)
		return -1;

	//イメージ読み込み

	ret = _read_image(p);
	if(ret)
	{
		_gif_error(p, ret);
		return -1;
	}

	p->currow = p->imgbuf;
	p->row_remain = p->height;

	return 1;
}

/** Y1行を指定フォーマットで取得
 *
 * @param trans_to_a0  出力が RGBA で TRUE の場合、透過色を A=0 とする */

mBool mGIFDecode_getNextLine(mGIFDecode *p,void *buf,int format,mBool trans_to_a0)
{
	uint8_t *ps,*pd,*pal,*palbuf;
	int i,c;

	if(p->row_remain <= 0) return FALSE;

	if(format == MGIFDECODE_FORMAT_RAW)
		//生データ
		memcpy(buf, p->currow, p->width);
	else
	{
		//変換

		ps = p->currow;
		pd = (uint8_t *)buf;

		palbuf = p->palbuf[p->palette_no];

		for(i = p->width; i > 0; i--)
		{
			c = *(ps++);
		
			pal = palbuf + (c << 2);

			pd[0] = pal[0];
			pd[1] = pal[1];
			pd[2] = pal[2];
			pd += 3;

			//A

			if(format == MGIFDECODE_FORMAT_RGBA)
				*(pd++) = (trans_to_a0 && c == p->transparent)? 0: 255;
		}
	}

	p->currow += p->width;
	p->row_remain--;

	return TRUE;
}

/** @} */
