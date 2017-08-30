/*$
 Copyright (C) 2013-2017 Azel.

 This file is part of AzPainter.

 AzPainter is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 AzPainter is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
$*/

/************************************
 * APD v3 読込/保存
 ************************************/

#include <stdio.h>
#include <string.h>

#include "mDef.h"
#include "mZlib.h"
#include "mUtilStdio.h"
#include "mUtil.h"
#include "mPopupProgress.h"

#include "defDraw.h"
#include "defMacros.h"

#include "LayerList.h"
#include "LayerItem.h"
#include "defTileImage.h"
#include "TileImage.h"
#include "TileImage_coltype.h"
#include "ImageBufRGB16.h"

#include "file_apd_v3.h"


//----------------------

typedef struct _loadAPDv3
{
	FILE *fp;
	mZlibDecode *dec;
	mPopupProgress *prog;

	uint8_t *tilebuf;

	int32_t layernum,
		width,
		height,
		dpi;
	uint32_t thunk_size;
	long thunk_toppos,
		filesize;
	fpos_t findthunk_toppos;	//チャンク検索の先頭位置
}loadAPDv3;

typedef struct _saveAPDv3
{
	FILE *fp;
	mZlibEncode *enc;
	mPopupProgress *prog;

	uint8_t *tilebuf;	//作業用タイルバッファ
	
	int layernum;	//レイヤ数
	uint32_t size;
	fpos_t filepos;
}saveAPDv3;

//----------------------

#define _MAKE_ID(a,b,c,d)  ((a << 24) | (b << 16) | (c << 8) | d)

static uint32_t g_blendid[] = {
	_MAKE_ID('n','o','r','m'), _MAKE_ID('m','u','l',' '), _MAKE_ID('a','d','d',' '), _MAKE_ID('s','u','b',' '),
	_MAKE_ID('s','c','r','n'), _MAKE_ID('o','v','r','l'), _MAKE_ID('h','d','l','g'), _MAKE_ID('s','t','l','g'),
	_MAKE_ID('d','o','d','g'), _MAKE_ID('b','u','r','n'), _MAKE_ID('l','b','u','n'), _MAKE_ID('v','i','v','l'),
	_MAKE_ID('l','i','n','l'), _MAKE_ID('p','i','n','l'), _MAKE_ID('l','i','g','t'), _MAKE_ID('d','a','r','k'),
	_MAKE_ID('d','i','f','f'), 0
};

//----------------------

#define _PROGRESS_STEP  10

//----------------------



//============================
// 読込
//============================


/** ヘッダの読み込み */

static mBool _read_header(loadAPDv3 *p,FILE *fp,loadapdv3_info *info)
{
	uint8_t ver,unit;

	if(!mFILEreadCompareStr(fp, "AZPDATA"))
		return FALSE;

	if(!mFILEreadByte(fp, &ver) || ver != 2)
		return FALSE;

	//幅、高さ、解像度単位、解像度

	if(!mFILEread32BE(fp, &p->width)
		|| !mFILEread32BE(fp, &p->height)
		|| !mFILEreadByte(fp, &unit)
		|| !mFILEread32BE(fp, &p->dpi)
		|| p->width <= 0 || p->height <= 0
		|| p->width >= IMAGE_SIZE_MAX
		|| p->height >= IMAGE_SIZE_MAX)
		return FALSE;

	//チャンク先頭

	fgetpos(fp, &p->findthunk_toppos);

	//dpi

	if(unit != 1 || p->dpi < 1)
		p->dpi = 96;

	//

	if(info)
	{
		info->width = p->width;
		info->height = p->height;
		info->dpi = p->dpi;
	}

	return TRUE;
}

/** 指定チャンクを探す
 *
 * @param error ファイルサイズがデータサイズより小さければエラーにする */

static mBool _read_goto_thunk(loadAPDv3 *p,uint32_t thunk_id,mBool error)
{
	uint32_t id,size;

	fsetpos(p->fp, &p->findthunk_toppos);

	while(1)
	{
		//ID、チャンクサイズ
		
		if(!mFILEread32BE(p->fp, &id)
			|| !mFILEread32BE(p->fp, &size))
			break;
	
		if(id == thunk_id)
		{
			p->thunk_toppos = ftell(p->fp);
			p->thunk_size = size;

			//データが壊れている
			
			if(error && p->filesize < p->thunk_toppos + p->thunk_size)
				return FALSE;

			return TRUE;
		}
		else if(id == _MAKE_ID('E','N','D',' '))
			break;

		if(fseek(p->fp, size, SEEK_CUR) < 0)
			break;
	}

	return FALSE;
}


//===================


/** 閉じる */

void loadAPDv3_close(loadAPDv3 *p)
{
	if(p->fp) fclose(p->fp);
	mZlibDecodeFree(p->dec);
	mFree(p->tilebuf);
	
	mFree(p);
}

/** 開く */

loadAPDv3 *loadAPDv3_open(const char *filename,loadapdv3_info *info,mPopupProgress *prog)
{
	loadAPDv3 *p;
	FILE *fp;

	p = (loadAPDv3 *)mMalloc(sizeof(loadAPDv3), TRUE);
	if(!p) return NULL;

	p->prog = prog;

	//zlib

	p->dec = mZlibDecodeNew(8192, 15);
	if(!p->dec) goto ERR;

	//作業用バッファ

	p->tilebuf = (uint8_t *)mMalloc(64 * 64 * 8, FALSE);
	if(!p->dec) goto ERR;

	//開く

	p->fp = fp = mFILEopenUTF8(filename, "rb");
	if(!fp) goto ERR;

	//ファイルサイズ
	fseek(fp, 0, SEEK_END);
	p->filesize = ftell(fp);
	rewind(fp);

	mZlibDecodeSetIO_stdio(p->dec, fp);

	//ヘッダ読み込み

	if(!_read_header(p, fp, info)) goto ERR;

	return p;

ERR:
	loadAPDv3_close(p);
	return NULL;
}

/** チャンクの読み込みを終了して次へ */

void loadAPDv3_endThunk(loadAPDv3 *p)
{
	fseek(p->fp, p->thunk_toppos + p->thunk_size, SEEK_SET);
}

/** レイヤヘッダ読み込み */

mBool loadAPDv3_readLayerHeader(loadAPDv3 *p,int *layernum,int *curno)
{
	uint16_t num,cur;

	if(!_read_goto_thunk(p, _MAKE_ID('L','Y','H','D'), TRUE)
		|| p->thunk_size < 4)
		return FALSE;

	mFILEread16BE(p->fp, &num);
	mFILEread16BE(p->fp, &cur);

	loadAPDv3_endThunk(p);

	//レイヤ数が0
	if(num == 0) return FALSE;

	//

	p->layernum = num;

	*layernum = num;
	*curno = cur;

	//プログレス

	mPopupProgressThreadSetMax(p->prog, num * _PROGRESS_STEP);

	return TRUE;
}

/** レイヤツリー読み込み & レイヤ作成 */

mBool loadAPDv3_readLayerTree_andAddLayer(loadAPDv3 *p)
{
	uint16_t pos;
	int i;

	if(!_read_goto_thunk(p, _MAKE_ID('L','Y','T','R'), TRUE))
	{
		//ツリー情報がなければ、親はすべてルートとみなす

		return LayerList_addLayers_onRoot(APP_DRAW->layerlist, p->layernum);
	}
	else
	{
		/* 先頭レイヤから順に、親のレイヤ番号 (2byte) が並んでいる。
		 * 0xffff でルート。 */

		if(p->thunk_size != p->layernum * 2) return FALSE;

		for(i = p->layernum; i > 0; i--)
		{
			mFILEread16BE(p->fp, &pos);

			if(!LayerList_addLayer_pos(APP_DRAW->layerlist, (pos == 0xffff)? -1: pos, -1))
				return FALSE;
		}
	}

	return TRUE;
}

/** レイヤ情報の読み込みを開始 */

mBool loadAPDv3_beginLayerInfo(loadAPDv3 *p)
{
	uint16_t ver;

	if(!_read_goto_thunk(p, _MAKE_ID('L','Y','I','F'), TRUE))
		return FALSE;

	//バージョン

	mFILEread16BE(p->fp, &ver);
	if(ver != 0) return FALSE;

	return TRUE;
}

/** 各レイヤ情報を読み込み */

mBool loadAPDv3_readLayerInfo(loadAPDv3 *p,LayerItem *item)
{
	FILE *fp = p->fp;
	TileImage *img;
	char *pc_name;
	uint8_t coltype,channels,opacity,amask,rgb[3];
	uint16_t exsize;
	uint32_t blendid,flags,exid;
	int32_t x1,y1,x2,y2,i;
	mRect rc;

	//カラータイプ

	mFILEreadByte(fp, &coltype);

	if(coltype >= 3 && coltype != 255) return FALSE;

	//チャンネル数

	mFILEreadByte(fp, &channels);

	if(coltype != 255
		&& channels != 1 && channels != 2 && channels != 4)
		return FALSE;

	//left,top,right,bottom

	mFILEread32BE(fp, &x1);
	mFILEread32BE(fp, &y1);
	mFILEread32BE(fp, &x2);
	mFILEread32BE(fp, &y2);

	//不透明度

	mFILEreadByte(fp, &opacity);

	if(opacity > 128) opacity = 128;

	//合成モード

	mFILEread32BE(fp, &blendid);

	//フラグ

	mFILEread32BE(fp, &flags);

	//アルファマスク

	mFILEreadByte(fp, &amask);

	//線の色

	fread(rgb, 1, 3, fp);

	//拡張データ

	while(1)
	{
		//ID
		mFILEread32BE(fp, &exid);
		if(exid == 0) break;

		//サイズ
		mFILEread16BE(fp, &exsize);

		if(exid == _MAKE_ID('n','a','m','e'))
		{
			//名前

			if(exsize)
			{
				item->name = (char *)mMalloc(exsize + 1, TRUE);
				fread(item->name, 1, exsize, fp);
			}
		}
		else if(exid == _MAKE_ID('t','e','x','r'))
		{
			//テクスチャパス

			if(exsize)
			{
				pc_name = (char *)mMalloc(exsize + 1, TRUE);
				fread(pc_name, 1, exsize, fp);

				LayerItem_setTexture(item, pc_name);

				mFree(pc_name);
			}
		}
		else
			fseek(fp, exsize, SEEK_CUR);
	}

	//--------------

	//イメージのあるレイヤ時

	if(coltype != 255)
	{
		//カラータイプ

		if(channels == 4 && (coltype == 0 || coltype == 2))
			//8bit/15bit : RGBA
			i = TILEIMAGE_COLTYPE_RGBA;
		else if(coltype == 2 && channels == 2)
			//15bit : GRAY
			i = TILEIMAGE_COLTYPE_GRAY;
		else if(coltype == 2 && channels == 1)
			//A16
			i = TILEIMAGE_COLTYPE_ALPHA16;
		else if(coltype == 1 && channels == 1)
			//A1
			i = TILEIMAGE_COLTYPE_ALPHA1;
		else
			return FALSE;

		//イメージ作成
		
		rc.x1 = x1, rc.y1 = y1;
		rc.x2 = x2, rc.y2 = y2;

		img = TileImage_newFromRect_forFile(i, &rc);
		if(!img) return FALSE;

		LayerItem_replaceImage(item, img);

		//線の色

		LayerItem_setLayerColor(item, M_RGB(rgb[0], rgb[1], rgb[2]));

		//合成モード

		for(i = 0; g_blendid[i]; i++)
		{
			if(blendid == g_blendid[i])
			{
				item->blendmode = i;
				break;
			}
		}
	}

	//

	item->opacity = opacity;
	item->flags = flags;
	item->alphamask = amask;

	item->tmp1 = coltype;
	item->tmp2 = channels;

	return TRUE;
}

/** レイヤタイルの読み込み開始 */

mBool loadAPDv3_beginLayerTile(loadAPDv3 *p)
{
	uint16_t tsize;
	uint8_t type;

	if(!_read_goto_thunk(p, _MAKE_ID('L','Y','T','L'), FALSE)
		|| p->thunk_size < 3)
		return FALSE;

	//タイルのサイズ

	if(!mFILEread16BE(p->fp, &tsize) || tsize != 64)
		return FALSE;

	//圧縮タイプ (0=zlib)

	if(!mFILEreadByte(p->fp, &type) || type != 0)
		return FALSE;

	return TRUE;
}

/** 各レイヤタイルの読み込み */

mBool loadAPDv3_readLayerTile(loadAPDv3 *p,LayerItem *item)
{
	TileImage *img;
	uint32_t num,encsize;
	uint8_t buf[4],**pptile;
	int tx,ty,tilesize;
	TileImageColFunc_getsetTileForSave settilefunc;

	//タイル数、圧縮サイズ

	if(!mFILEread32BE(p->fp, &num)
		|| !mFILEread32BE(p->fp, &encsize))
		return FALSE;

	//タイルなし

	if(num == 0)
	{
		mPopupProgressThreadAddPos(p->prog, _PROGRESS_STEP);
		return TRUE;
	}

	//

	img = item->img;
	if(!img) return FALSE;

	//------ タイル

	if(item->tmp1 == 0 && item->tmp2 == 4)
	{
		//RGBA 8bit の場合

		settilefunc = TileImage_setTile_RGBA8toRGBA16;
		tilesize = 64 * 64 * 4;
	}
	else
	{
		settilefunc = g_tileimage_funcs[img->coltype].setTileForSave;
		tilesize = img->tilesize;
	}

	mZlibDecodeReset(p->dec);
	mZlibDecodeSetInSize(p->dec, encsize);

	mPopupProgressThreadBeginSubStep(p->prog, _PROGRESS_STEP, num);

	for(; num; num--)
	{
		//タイル位置

		if(mZlibDecodeRead(p->dec, buf, 4))
			return FALSE;

		tx = mGetBuf16BE(buf);
		ty = mGetBuf16BE(buf + 2);

		if(tx >= img->tilew || ty >= img->tileh)
			return FALSE;

		pptile = TILEIMAGE_GETTILE_BUFPT(img, tx, ty);

		//タイル読み込み

		if(mZlibDecodeRead(p->dec, p->tilebuf, tilesize))
			return FALSE;

		//タイルセット

		*pptile = TileImage_allocTile(img, FALSE);
		if(!(*pptile)) return FALSE;

		(settilefunc)(*pptile, p->tilebuf);

		mPopupProgressThreadIncSubStep(p->prog);
	}

	if(mZlibDecodeReadEnd(p->dec)) return FALSE;

	return TRUE;
}


//============================
// 保存
//============================



/** 拡張データに文字列追加 */

static void _save_add_exdata_string(saveAPDv3 *p,const char *exid,const char *text)
{
	int len;

	if(text)
	{
		fputs(exid, p->fp);

		len = strlen(text);

		mFILEwrite16BE(p->fp, len);
		fwrite(text, 1, len, p->fp);

		p->size += 4 + 2 + len;
	}
}


//-----------


/** 閉じる */

void saveAPDv3_close(saveAPDv3 *p)
{
	if(p->fp) fclose(p->fp);
	mZlibEncodeFree(p->enc);
	mFree(p->tilebuf);
	
	mFree(p);
}

/** 開く */

saveAPDv3 *saveAPDv3_open(const char *filename,mPopupProgress *prog)
{
	saveAPDv3 *p;
	FILE *fp;

	p = (saveAPDv3 *)mMalloc(sizeof(saveAPDv3), TRUE);
	if(!p) return NULL;

	p->prog = prog;

	//zlib

	p->enc = mZlibEncodeNew_simple(8192, 6);
	if(!p->enc) goto ERR;

	//作業用バッファ

	p->tilebuf = (uint8_t *)mMalloc(64 * 64 * 8, FALSE);
	if(!p->tilebuf) goto ERR;

	//開く

	p->fp = fp = mFILEopenUTF8(filename, "wb");
	if(!fp) goto ERR;

	mZlibEncodeSetIO_stdio(p->enc, fp);

	//ヘッダ書き込み

	fputs("AZPDATA", fp);
	mFILEwriteByte(fp, 2);  //ver3

	mFILEwrite32BE(fp, APP_DRAW->imgw);
	mFILEwrite32BE(fp, APP_DRAW->imgh);

	//解像度単位 (0:none 1:dpi)
	mFILEwriteByte(fp, 1);

	//解像度
	mFILEwrite32BE(fp, APP_DRAW->imgdpi);

	return p;

ERR:
	saveAPDv3_close(p);
	return NULL;
}

/** 終了を書き込み */

void saveAPDv3_writeEnd(saveAPDv3 *p)
{
	fputs("END ", p->fp);
	mFILEwriteZero(p->fp, 4);
}

/** レイヤのヘッダ情報書き込み */

void saveAPDv3_writeLayerHeader(saveAPDv3 *p,int num,int curno)
{
	p->layernum = num;

	fputs("LYHD", p->fp);
	mFILEwrite32BE(p->fp, 4);

	//レイヤ数
	mFILEwrite16BE(p->fp, num);

	//カレントレイヤ番号 (上から順)
	mFILEwrite16BE(p->fp, curno);

	//プログレス

	mPopupProgressThreadSetMax(p->prog, num * _PROGRESS_STEP);
}

/** レイヤツリー情報を書き込み */

void saveAPDv3_writeLayerTree(saveAPDv3 *p)
{
	LayerItem *pi;
	int no;

	fputs("LYTR", p->fp);
	mFILEwrite32BE(p->fp, p->layernum * 2);

	//親のレイヤ番号 (2byte)

	for(pi = LayerList_getItem_top(APP_DRAW->layerlist); pi; pi = LayerItem_getNext(pi))
	{
		if(pi->i.parent)
			no = LayerList_getItemPos(APP_DRAW->layerlist, LAYERITEM(pi->i.parent));
		else
			no = 0xffff;
		
		mFILEwrite16BE(p->fp, no);
	}
}

/** (共通) チャンク終了
 *
 * begin... の関数で開始したチャンクを終了する。 */

void saveAPDv3_endThunk(saveAPDv3 *p)
{
	fsetpos(p->fp, &p->filepos);
	mFILEwrite32BE(p->fp, p->size);
	fseek(p->fp, 0, SEEK_END);
}

/** レイヤ情報書き込み開始 */

void saveAPDv3_beginLayerInfo(saveAPDv3 *p)
{
	fputs("LYIF", p->fp);

	//チャンクサイズ(仮)
	fgetpos(p->fp, &p->filepos);
	mFILEwriteZero(p->fp, 4);

	//バージョン
	mFILEwrite16BE(p->fp, 0);

	p->size = 2;
}

/** 各レイヤ情報書き込み */

void saveAPDv3_writeLayerInfo(saveAPDv3 *p,LayerItem *item)
{
	FILE *fp = p->fp;
	mRect rc;
	uint8_t channel_num[] = {4,2,1,1};

	if(LAYERITEM_IS_FOLDER(item))
	{
		//----- フォルダ時

		mFILEwriteByte(fp, 255);
		mFILEwriteZero(fp, 17);

		mFILEwriteByte(fp, item->opacity);
		mFILEwriteZero(fp, 4);
		mFILEwrite32BE(fp, item->flags);
		mFILEwriteZero(fp, 4);
	}
	else
	{
		//カラータイプ (0:8bit, 1:1bit, 2:16bit(固定少数15bit), 255:フォルダ)

		mFILEwriteByte(fp, (item->coltype == TILEIMAGE_COLTYPE_ALPHA1)? 1: 2);

		//チャンネル数

		mFILEwriteByte(fp, channel_num[item->coltype]);

		//left,top,right,bottom

		if(!TileImage_getHaveImageRect_pixel(item->img, &rc, NULL))
			mFILEwriteZero(fp, 16);
		else
		{
			mFILEwrite32BE(fp, rc.x1);
			mFILEwrite32BE(fp, rc.y1);
			mFILEwrite32BE(fp, rc.x2 + 1);
			mFILEwrite32BE(fp, rc.y2 + 1);

			//左上の位置をレイヤに記録

			item->tmp1 = rc.x1;
			item->tmp2 = rc.y1;
		}

		//不透明度

		mFILEwriteByte(fp, item->opacity);

		//合成モード

		mFILEwrite32BE(fp, g_blendid[item->blendmode]);

		//フラグ

		mFILEwrite32BE(fp, item->flags);

		//アルファマスク

		mFILEwriteByte(fp, item->alphamask);

		//線の色 (RGB)

		mFILEwriteByte(fp, M_GET_R(item->col));
		mFILEwriteByte(fp, M_GET_G(item->col));
		mFILEwriteByte(fp, M_GET_B(item->col));
	}

	p->size += 31;

	//------ 拡張データ

	//名前

	_save_add_exdata_string(p, "name", item->name);

	//テクスチャパス

	_save_add_exdata_string(p, "texr", item->texture_path);

	//終了

	mFILEwriteZero(fp, 4);
	p->size += 4;
}

/** レイヤタイルの書き込み開始 */

void saveAPDv3_beginLayerTile(saveAPDv3 *p)
{
	fputs("LYTL", p->fp);

	//チャンクサイズ (仮)
	fgetpos(p->fp, &p->filepos);
	mFILEwriteZero(p->fp, 4);

	//タイルサイズ
	mFILEwrite16BE(p->fp, 64);

	//圧縮タイプ
	mFILEwriteByte(p->fp, 0);

	p->size = 3;
}

/** 各レイヤタイルの書き込み */

void saveAPDv3_writeLayerTile(saveAPDv3 *p,LayerItem *item)
{
	uint8_t buf[4];
	int num,ix,iy,tx,ty;
	uint32_t encsize;
	fpos_t pos;
	uint8_t **pp;
	TileImage *img;
	TileImageColFunc_getsetTileForSave gettilefunc;

	img = item->img;

	//タイル数

	num = (img)? TileImage_getHaveTileNum(img): 0;

	mFILEwrite32BE(p->fp, num);

	//圧縮サイズ(仮)

	fgetpos(p->fp, &pos);
	mFILEwriteZero(p->fp, 4);

	p->size += 8;

	//フォルダまたは空イメージの場合

	if(num == 0)
	{
		mPopupProgressThreadAddPos(p->prog, _PROGRESS_STEP);
		return;
	}

	//----- タイルデータ

	mPopupProgressThreadBeginSubStep(p->prog, _PROGRESS_STEP, num);

	mZlibEncodeReset(p->enc);

	gettilefunc = g_tileimage_funcs[img->coltype].getTileForSave;

	pp = img->ppbuf;
	ty = (img->offy - item->tmp2) >> 6;

	for(iy = 0; iy < img->tileh; iy++, ty++)
	{
		tx = (img->offx - item->tmp1) >> 6;
	
		for(ix = 0; ix < img->tilew; ix++, tx++, pp++)
		{
			if(!(*pp)) continue;
			
			//タイル位置

			mSetBuf16BE(buf, tx);
			mSetBuf16BE(buf + 2, ty);

			mZlibEncodeSend(p->enc, buf, 4);

			//タイルデータ

			(gettilefunc)(p->tilebuf, *pp);

			mZlibEncodeSend(p->enc, p->tilebuf, img->tilesize);

			mPopupProgressThreadIncSubStep(p->prog);
		}
	}

	mZlibEncodeFlushEnd(p->enc);

	//圧縮サイズ

	encsize = mZlibEncodeGetWriteSize(p->enc);

	fsetpos(p->fp, &pos);
	mFILEwrite32BE(p->fp, encsize);
	fseek(p->fp, 0, SEEK_END);

	p->size += encsize;
}

/** 合成後の一枚絵イメージ書き込み
 *
 * DrawData::blendimg に RGB 8bit としてセットしてある。 */

void saveAPDv3_writeBlendImage(saveAPDv3 *p)
{
	fpos_t pos1,pos2;
	uint32_t encsize;

	fputs("BIMG", p->fp);

	fgetpos(p->fp, &pos1);
	mFILEwrite32BE(p->fp, 0);

	//カラータイプ (0:8bitRGB)

	mFILEwriteByte(p->fp, 0);

	//圧縮タイプ (0:zlib)

	mFILEwriteByte(p->fp, 0);

	//圧縮サイズ

	fgetpos(p->fp, &pos2);
	mFILEwrite32BE(p->fp, 0);

	//データ (R-G-B、左上 -> 右下順)

	mZlibEncodeReset(p->enc);

	mZlibEncodeSend(p->enc, APP_DRAW->blendimg->buf, APP_DRAW->imgw * APP_DRAW->imgh * 3);

	mZlibEncodeFlushEnd(p->enc);

	encsize = mZlibEncodeGetWriteSize(p->enc);

	//チャンクサイズ

	fsetpos(p->fp, &pos1);
	mFILEwrite32BE(p->fp, 6 + encsize);
	
	//圧縮サイズ
	
	fsetpos(p->fp, &pos2);
	mFILEwrite32BE(p->fp, encsize);
	fseek(p->fp, 0, SEEK_END);
}
