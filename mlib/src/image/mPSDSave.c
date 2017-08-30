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
 * mPSDSave
 *****************************************/

#include <stdio.h>
#include <string.h>

#include "mDef.h"
#include "mUtilStdio.h"
#include "mUtil.h"
#include "mUtilCharCode.h"

#include "mPSDSave.h"


//-------------------

typedef struct
{
	long fpos_chinfo;
	int left,top,width,height;
	uint8_t channels;
}_layerinfo;


struct _mPSDSave
{
	FILE *fp;
	mPSDSaveInfo *info;

	uint8_t *buf1,		//作業用バッファ1
		*buf2,			//作業用バッファ2
		*rowbuf,		//チャンネルのY1行バッファ
		*outbuf,		//PackBits 書き込み用バッファ
						//(無圧縮時は Unicode 名用バッファ)
		compression;	//0:無圧縮 1:PackBits

	_layerinfo *curlinfo;

	fpos_t fpos1,
		fpos2;
	uint32_t size1,
		size2;
	int layernum,
		pitch,
		cury,
		curlayer,
		curchannel,
		outsize,	//outbuf の現在の書き込みサイズ
		layer_maxw,	//レイヤイメージの最大サイズ
		layer_maxh,
		channel_id;
};

//-------------------

#define _OUTBUFSIZE  (8 * 1024)
#define _MAX_UNICODE_NAME_LEN 256  //レイヤ名 Unicode の最大文字数

//-------------------



//========================
// PackBits 圧縮
//========================


/** outbuf のデータをファイルに書き込み */

static void _write_outbuf(mPSDSave *p)
{
	if(p->outsize)
	{
		fwrite(p->outbuf, 1, p->outsize, p->fp);
		p->outsize = 0;
	}
}

/** データ書き込み
 *
 * @param type 0:非連続 1:連続
 * @return 書き込んだサイズ */

static int _packbits_write(mPSDSave *p,uint8_t type,uint8_t *buf,int cnt)
{
	char len;
	int size;
	uint8_t *pd;

	//長さ

	len = cnt - 1;
	if(type) len = -len;

	//データサイズ

	size = 1 + ((type)? 1: cnt);

	//バッファを超える場合はファイルに出力

	if(p->outsize + size >= _OUTBUFSIZE)
		_write_outbuf(p);

	//バッファに追加

	pd = p->outbuf + p->outsize;

	*((char *)pd) = len;

	if(type)
		pd[1] = *buf;
	else
		memcpy(pd + 1, buf, cnt);

	p->outsize += size;

	return size;
}

/** PackBits 圧縮
 *
 * outbuf に圧縮データを書き込み、バッファがいっぱいになったらファイルへ書き込む。
 *
 * @return 圧縮サイズ */

static int _encode_packbits(mPSDSave *p,uint8_t *buf,int size)
{
	uint8_t *ps,*psend,*ps2,val;
	int write = 0;

	ps = buf;
	psend = ps + size;

	while(ps < psend)
	{
		if(ps == psend - 1 || *ps != ps[1])
		{
			//非連続データ (次の連続データが見つかるまで)

			for(ps2 = ps; ps - ps2 < 128; ps++)
			{
				if(ps == psend - 1)
				{
					ps++;
					break;
				}
				else if(*ps == ps[1])
					break;
			}

			write += _packbits_write(p, 0, ps2, ps - ps2);
		}
		else
		{
			//連続データ

			val = *ps;
			ps2 = ps;

			for(ps += 2; ps < psend && ps - ps2 < 128 && *ps == val; ps++);

			write += _packbits_write(p, 1, ps2, ps - ps2);
		}
	}

	return write;
}


//=========================
// sub
//=========================


/** Unicode レイヤ名を outbuf にセット
 *
 * NULL 文字は含まない。
 * データは 4byte 境界。
 *
 * @return 文字数 */

static int _set_layername_unicode(mPSDSave *p,const char *name)
{
	uint8_t *pd;
	uint32_t c,c2;
	int ret,len;

	pd = p->outbuf;
	len = 0;

	while(*name && len < _MAX_UNICODE_NAME_LEN)
	{
		ret = mUTF8ToUCS4Char(name, -1, &c, &name);

		if(ret < 0)
			return 0;
		else if(ret == 0)
		{
			if(c <= 0xffff)
			{
				pd[0] = c >> 8;
				pd[1] = (uint8_t)c;
				pd += 2;
				len++;
			}
			else
			{
				c -= 0x10000;
				c2 = c >> 16;
				
				pd[0] = 0xd8 | (c2 >> 2);
				pd[1] = ((c2 & 3) << 6) | ((c >> 10) & 63);
				pd[2] = 0xdc | ((c >> 8) & 3);
				pd[3] = (uint8_t)c;

				pd += 4;
				len += 2;
			}
		}
	}

	//余白追加 (4byte境界)

	if(len & 1)
		pd[0] = pd[1] = 0;

	return len;
}


//=========================
// main
//=========================


/*******************//**

@defgroup psdsave mPSDSave
@brief PSD 書き込み

@ingroup group_image
 
@{
@file mPSDSave.h

@struct mPSDSaveInfo
@enum MPSD_COLMODE
@enum MPSD_BLENDMODE

************************/


/** 閉じる */

void mPSDSave_close(mPSDSave *p)
{
	if(p)
	{
		if(p->fp) fclose(p->fp);

		mFree(p->buf1);
		mFree(p->buf2);
		mFree(p->rowbuf);
		mFree(p->outbuf);
		
		mFree(p);
	}
}

/** ファイルから開く
 *
 * @param packbits PackBits 圧縮を行うか (FALSE で無圧縮) */

mPSDSave *mPSDSave_openFile(const char *filename,mPSDSaveInfo *info,mBool packbits)
{
	mPSDSave *p;
	uint8_t dat[26];

	//確保

	p = (mPSDSave *)mMalloc(sizeof(mPSDSave), TRUE);
	if(!p) return NULL;

	p->info = info;
	p->compression = (packbits != 0);

	//PackBits 書き込み用バッファ
	//(無圧縮時は Unicode 名用バッファ)

	p->outbuf = (uint8_t *)mMalloc(
		(packbits)? _OUTBUFSIZE: ((_MAX_UNICODE_NAME_LEN * 2 + 3) & ~3), FALSE);

	if(!p->outbuf)
	{
		mPSDSave_close(p);
		return NULL;
	}

	//ファイル開く

	p->fp = mFILEopenUTF8(filename, "wb");
	if(!p->fp)
	{
		mPSDSave_close(p);
		return NULL;
	}

	//ファイルヘッダ書き込み

	mSetBufBE_args(dat, "s24224422",
		"8BPS",
		1,		//version
		0, 0,	//reversed
		p->info->img_channels, p->info->height, p->info->width,
		p->info->bits, p->info->colmode);

	fwrite(dat, 1, 26, p->fp);

	//カラーモードデータサイズ (0)

	mFILEwriteZero(p->fp, 4);

	return p;
}

/** イメージ書き込み先バッファ取得 */

uint8_t *mPSDSave_getLineImageBuf(mPSDSave *p)
{
	return p->rowbuf;
}


//=========================
// 画像リソースデータ
//=========================
/*
 * fpos1 : 全体サイズ書き込み位置
 * size1 : 全体サイズ
 * 
 * 各データは 2byte 境界。
 * 奇数の場合は、データサイズはそのままで、余白データを 1byte 追加。
 */


/** 画像リソースデータ開始 */

void mPSDSave_beginResource(mPSDSave *p)
{
	//全体サイズ (仮)

	fgetpos(p->fp, &p->fpos1);
	mFILEwriteZero(p->fp, 4);

	p->size1 = 0;
}

/** 画像リソースデータ終了 */

void mPSDSave_endResource(mPSDSave *p)
{
	//データが空の場合、すでに仮サイズで 0 を書き込んでいるので、何もしない

	if(p->size1)
	{
		fsetpos(p->fp, &p->fpos1);
		mFILEwrite32BE(p->fp, p->size1);
		fseek(p->fp, 0, SEEK_END);
	}
}

/** 画像リソース:解像度を DPI 単位で書き込み */

void mPSDSave_writeResource_resolution_dpi(mPSDSave *p,int dpi_horz,int dpi_vert)
{
	uint8_t dat[28] = {
		0x38,0x42,0x49,0x4d,    //'8BIM'
		0x03,0xed,              //ID (0x03ed)
		0x00,0x00,              //name
		0x00,0x00,0x00,0x10,    //size
		0x00,0x00,0x00,0x00, 0x00,0x01, 0x00,0x02, //H
		0x00,0x00,0x00,0x00, 0x00,0x01, 0x00,0x02  //V

		/* [4byte:16+16bit固定小数点] [2byte:1=pixel per inch] [2byte:2=cm] */
	};

	//固定小数点数の整数部分をセット

	mSetBuf16BE(dat + 12, dpi_horz);
	mSetBuf16BE(dat + 20, dpi_vert);

	fwrite(dat, 1, 28, p->fp);

	p->size1 += 28;
}

/** 画像リソース:カレントレイヤ番号を書き込み
 *
 * @param no 一番下のレイヤを 0 とする */

void mPSDSave_writeResource_currentLayer(mPSDSave *p,int no)
{
	uint8_t dat[14] = {
		0x38,0x42,0x49,0x4d,	//'8BIM'
		0x04,0x00,				//ID (0x0400)
		0x00,0x00,				//name
		0x00,0x00,0x00,0x02,	//size
		0x00,0x00
	};

	mSetBuf16BE(dat + 12, no);
	fwrite(dat, 1, 14, p->fp);

	p->size1 += 14;
}


//=============================
// 一枚絵イメージ
//=============================
/*
 * fpos1 : Y圧縮サイズリスト書き込み位置
 * buf1  : Y圧縮サイズリストバッファ
 */


/** 一枚絵イメージ 書き込み開始
 *
 * レイヤデータ書き込み後。*/

mBool mPSDSave_beginImage(mPSDSave *p)
{
	int i,lsize;

	//チャンネルのY1行サイズ

	if(p->info->bits == 1)
		p->pitch = (p->info->width + 7) >> 3;
	else
		p->pitch = p->info->width;

	//Y1行バッファ

	p->rowbuf = (uint8_t *)mMalloc(p->pitch, FALSE);
	if(!p->rowbuf) return FALSE;

	//PackBits Y圧縮サイズリストバッファ

	if(p->compression)
	{
		lsize = p->info->height * 2;

		p->buf1 = (uint8_t *)mMalloc(lsize, TRUE);
		if(!p->buf1) return FALSE;
	}

	//圧縮タイプ

	mFILEwrite16BE(p->fp, p->compression);

	//Y圧縮サイズリストを仮出力

	if(p->compression)
	{
		fgetpos(p->fp, &p->fpos1);

		for(i = p->info->img_channels; i > 0; i--)
			fwrite(p->buf1, 1, lsize, p->fp);
	}
	
	return TRUE;
}

/** 一枚絵イメージ、各チャンネルの書き込み開始 */

void mPSDSave_beginImageChannel(mPSDSave *p)
{
	p->cury = 0;
	p->outsize = 0;
}

/** 一枚絵イメージ、各チャンネルのY1行イメージ書き込み
 *
 * mPSDSave_getLineImageBuf() で取得したバッファに書き込んでおく。 */

void mPSDSave_writeImageChannelLine(mPSDSave *p)
{
	int size;

	if(p->cury < p->info->height)
	{
		if(p->compression == 0)
		{
			//無圧縮

			fwrite(p->rowbuf, 1, p->pitch, p->fp);
		}
		else
		{
			//PackBits

			size = _encode_packbits(p, p->rowbuf, p->pitch);

			mSetBuf16BE(p->buf1 + (p->cury << 1), size);
		}

		p->cury++;
	}
}

/** 一枚絵イメージ、各チャンネルの書き込み終了 */

void mPSDSave_endImageChannel(mPSDSave *p)
{
	//バッファの残りデータを出力

	_write_outbuf(p);

	//Y圧縮サイズリスト

	if(p->compression)
	{
		fsetpos(p->fp, &p->fpos1);
		fwrite(p->buf1, 1, p->info->height * 2, p->fp);

		//次の書き込み位置

		fgetpos(p->fp, &p->fpos1);
		fseek(p->fp, 0, SEEK_END);
	}
}


//=============================
// レイヤ
//=============================
/*
 * fpos1 : 全体サイズ書き込み位置
 * fpos2 : 各チャンネルのY圧縮サイズリスト書き込み位置
 * buf1  : 全レイヤの情報データ記録用
 * buf2  : Y圧縮サイズリストバッファ
 * size1 : 各チャンネルの圧縮サイズ
 * size2 : レイヤデータ全体のサイズ
 */


/** レイヤなしの場合、一枚絵イメージの前に書き込み */

void mPSDSave_writeLayerNone(mPSDSave *p)
{
	mFILEwrite32BE(p->fp, 0);
}

/** レイヤ書き込み開始
 *
 * @param num レイヤ数 */

mBool mPSDSave_beginLayer(mPSDSave *p,int num)
{
	p->layernum = num;
	p->curlayer = 0;
	p->layer_maxw = p->layer_maxh = 0;
	p->size2 = 2;  //2 = レイヤ数

	//レイヤ情報バッファ確保

	p->buf1 = (uint8_t *)mMalloc(sizeof(_layerinfo) * num, FALSE);
	if(!p->buf1) return FALSE;

	p->curlinfo = (_layerinfo *)p->buf1;

	//書き込み

	fgetpos(p->fp, &p->fpos1);

	mFILEwriteZero(p->fp, 8);	//全体サイズ(4) + 内部全体サイズ(4)
	mFILEwrite16BE(p->fp, num);	//レイヤ数

	return TRUE;
}

/** レイヤ情報書き込み
 *
 * mPSDSave_beginLayer() 後、一番下のレイヤを先頭として、全レイヤ分を書き込む。 */

void mPSDSave_writeLayerInfo(mPSDSave *p,mPSDSaveLayerInfo *info)
{
	FILE *fp = p->fp;
	_layerinfo *winfo;
	int i,name_len,name_len4,name_uni_len,name_uni_size4,exsize;

	if(p->curlayer >= p->layernum) return;

	//必要な情報を保存

	winfo = p->curlinfo;

	winfo->left = info->left;
	winfo->top = info->top;
	winfo->width = info->right - info->left;
	winfo->height = info->bottom - info->top;

	//イメージの最大サイズを記録しておく

	if(p->layer_maxw < winfo->width) p->layer_maxw = winfo->width;
	if(p->layer_maxh < winfo->height) p->layer_maxh = winfo->height;

	//------ 基本データ書き込み

	//範囲

	mFILEwrite32BE(fp, info->top);
	mFILEwrite32BE(fp, info->left);
	mFILEwrite32BE(fp, info->bottom);
	mFILEwrite32BE(fp, info->right);

	//チャンネル数

	mFILEwrite16BE(fp, info->channels);

	//チャンネルデータ

	winfo->fpos_chinfo = ftell(fp);

	for(i = 0; i < info->channels; i++)
	{
		//ID (仮)

		mFILEwrite16BE(fp, 0);

		//データサイズ (仮)
		/* 範囲なしの場合、圧縮フラグ[2byte]のみなので、2 をセットしておく */

		mFILEwrite32BE(fp, 2);
	}

	//8BIM

	fputs("8BIM", fp);

	//合成モード

	mFILEwrite32BE(fp, info->blendmode);

	//不透明度

	mFILEwriteByte(fp, info->opacity);

	//clip

	mFILEwriteByte(fp, 0);

	//フラグ [0bit:透明色保護 1bit:非表示]

	mFILEwriteByte(fp, (info->hide)? 2: 0);

	//0

	mFILEwriteByte(fp, 0);

	//-------- 拡張データ

	/* 4byte: 以下の拡張データサイズ
	 * 4byte: レイヤマスクデータサイズ
	 * 4byte: 合成データサイズ
	 * パスカル文字列: レイヤ名 [!]4byte境界(長さのバイトも含む)
	 * 8BIM + key のデータが並ぶ
	 */

	name_len = name_uni_len = 0;
	exsize = 0;

	//レイヤ名長さ

	if(info->name)
	{
		name_len = strlen(info->name);
		if(name_len > 255) name_len = 255;
	}

	name_len4 = (name_len + 1 + 3) & ~3;

	//Unicode レイヤ名 (outbuf にセット)

	if(name_len)
	{
		name_uni_len = _set_layername_unicode(p, info->name);

		name_uni_size4 = (name_uni_len * 2 + 3) & ~3;
		exsize += 8 + 4 + 4 + name_uni_size4;
	}

	//****

	//拡張データサイズ

	mFILEwrite32BE(fp, 8 + name_len4 + exsize);

	//レイヤマスクデータサイズ + 合成データサイズ

	mFILEwriteZero(fp, 8);

	//レイヤ名

	mFILEwriteByte(fp, name_len);
	fwrite(info->name, 1, name_len, fp);
	mFILEwriteZero(fp, name_len4 - name_len - 1);	//余白

	//レイヤ名 (Unicode)

	if(name_uni_len)
	{
		fputs("8BIMluni", fp);
		mFILEwrite32BE(fp, 4 + name_uni_size4);
		mFILEwrite32BE(fp, name_uni_len);
		fwrite(p->outbuf, 1, name_uni_size4, fp);
	}

	//次へ

	p->curlayer++;
	p->curlinfo++;
	p->size2 += 66 + name_len4 + exsize;
}

/** レイヤ情報書き込み終了
 *
 * イメージ書き込みの準備を行う。 */

mBool mPSDSave_endLayerInfo(mPSDSave *p)
{
	p->curlayer = 0;
	p->curlinfo = (_layerinfo *)p->buf1;

	//バッファ確保

	p->rowbuf = (uint8_t *)mMalloc(p->layer_maxw, FALSE);
	if(!p->rowbuf) return FALSE;

	if(p->compression)
	{
		p->buf2 = (uint8_t *)mMalloc(p->layer_maxh * 2, FALSE);
		if(!p->buf2) return FALSE;
	}

	return TRUE;
}

/** 各レイヤのイメージ書き込み開始
 *
 * 空イメージの場合は、イメージデータが書き込まれるで、チャンネルイメージを描画する必要はない。
 *
 * @param box イメージ範囲が入る (NULL で取得しない)
 * @return イメージ範囲があるか (FALSE で空イメージ) */

mBool mPSDSave_beginLayerImage(mPSDSave *p,mBox *box)
{
	_layerinfo *winfo;

	if(p->curlayer >= p->layernum) return FALSE;

	p->curchannel = 0;

	winfo = p->curlinfo;

	//イメージ範囲

	if(box)
	{
		box->x = winfo->left;
		box->y = winfo->top;
		box->w = winfo->width;
		box->h = winfo->height;
	}

	//空かどうか

	if(winfo->width && winfo->height)
		return TRUE;
	else
	{
		//空ならイメージデータ書き込み
		/* 圧縮フラグ(2byte) = 0:無圧縮 x 4 ch */

		mFILEwriteZero(p->fp, 2 * 4);

		p->size2 += 8;

		return FALSE;
	}
}

/** レイヤの各チャンネル書き込み開始 */

void mPSDSave_beginLayerImageChannel(mPSDSave *p,int channel_id)
{
	p->channel_id = channel_id;
	p->cury = 0;
	p->outsize = 0;
	p->size1 = 2;

	//圧縮フラグ

	mFILEwrite16BE(p->fp, p->compression);

	//Y圧縮サイズリスト (仮)

	if(p->compression)
	{
		fgetpos(p->fp, &p->fpos2);
		
		fwrite(p->buf2, 1, p->curlinfo->height * 2, p->fp);

		p->size1 += p->curlinfo->height * 2;
	}
}

/** レイヤイメージの最大幅取得 */

int mPSDSave_getLayerImageMaxWidth(mPSDSave *p)
{
	return p->layer_maxw;
}

/** レイヤのチャンネルイメージ Y1行を出力 */

void mPSDSave_writeLayerImageChannelLine(mPSDSave *p)
{
	int size;

	if(p->cury < p->curlinfo->height)
	{
		if(p->compression == 0)
		{
			//無圧縮

			fwrite(p->rowbuf, 1, p->curlinfo->width, p->fp);

			size = p->curlinfo->width;
		}
		else
		{
			//PackBits
			
			size = _encode_packbits(p, p->rowbuf, p->curlinfo->width);

			mSetBuf16BE(p->buf2 + (p->cury << 1), size);
		}

		p->cury++;
		p->size1 += size;
	}
}

/** レイヤのチャンネルイメージ書き込み終了 */

void mPSDSave_endLayerImageChannel(mPSDSave *p)
{
	//PackBits 残りを出力

	_write_outbuf(p);

	//チャンネル情報を書き込み

	fseek(p->fp, p->curlinfo->fpos_chinfo + p->curchannel * 6, SEEK_SET);

	mFILEwrite16BE(p->fp, p->channel_id);
	mFILEwrite32BE(p->fp, p->size1);

	//Y圧縮サイズリスト書き込み

	if(p->compression)
	{
		fsetpos(p->fp, &p->fpos2);
		fwrite(p->buf2, 1, p->curlinfo->height * 2, p->fp);
	}

	fseek(p->fp, 0, SEEK_END);

	//

	p->curchannel++;
	p->size2 += p->size1;
}

/** 各レイヤイメージ書き込み終了 */

void mPSDSave_endLayerImage(mPSDSave *p)
{
	p->curlayer++;
	p->curlinfo++;
}

/** レイヤの書き込み終了 */

void mPSDSave_endLayer(mPSDSave *p)
{
	//2byte 境界

	if(p->size2 & 1)
	{
		mFILEwriteByte(p->fp, 0);
		p->size2++;
	}

	//全体サイズ書き込み

	fsetpos(p->fp, &p->fpos1);

	mFILEwrite32BE(p->fp, p->size2 + 4);
	mFILEwrite32BE(p->fp, p->size2);

	fseek(p->fp, 0, SEEK_END);

	//バッファ解放
	
	mFree(p->buf1);
	mFree(p->buf2);
	mFree(p->rowbuf);

	p->buf1 = p->buf2 = p->rowbuf = NULL;
}

/* @} */
