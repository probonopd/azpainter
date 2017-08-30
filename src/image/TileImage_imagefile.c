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

/*****************************************
 * TileImage
 *
 * 画像ファイルへの読み書き
 *****************************************/

#include <stdio.h>
#include <string.h>

#include "mDef.h"
#include "mLoadImage.h"
#include "mSaveImage.h"
#include "mPopupProgress.h"
#include "mZlib.h"
#include "mUtilStdio.h"
#include "mUtil.h"

#include "defTileImage.h"
#include "TileImage.h"
#include "TileImage_coltype.h"
#include "TileImageDrawInfo.h"

#include "defFileFormat.h"


//===============================
// 画像読み込み
//===============================


typedef struct
{
	mLoadImage base;
	TileImageLoadFileInfo *dstinfo;
	mPopupProgress *prog;
	TileImage *img;
	int y,
		ignore_alpha;
}_loadimageinfo;


/** 情報取得 */

static int _loadfile_getinfo(mLoadImage *param,mLoadImageInfo *info)
{
	_loadimageinfo *p = (_loadimageinfo *)param;
	int h,v;

	//TileImage 作成

	p->img = TileImage_new(TILEIMAGE_COLTYPE_RGBA, info->width, info->height);
	if(!p->img)
		return MLOADIMAGE_ERR_ALLOC;

	p->y = (info->bottomup)? info->height - 1: 0;

	//情報セット

	if(p->dstinfo)
	{
		p->dstinfo->width = info->width;
		p->dstinfo->height = info->height;
		p->dstinfo->transparent = info->transparent_col;

		if(mLoadImage_getResolution_dpi(param, &h, &v))
			p->dstinfo->dpi = h;
		else
			p->dstinfo->dpi = 0;
	}

	//プログレス

	if(p->prog)
		mPopupProgressThreadBeginSubStep_onestep(p->prog, 20, info->height);

	return MLOADIMAGE_ERR_OK;
}

/** Y1行取得 */

static int _loadfile_getrow(mLoadImage *param,uint8_t *buf,int pitch)
{
	_loadimageinfo *p = (_loadimageinfo *)param;
	int i;
	uint8_t *ps;

	//「アルファ値を無視」時はアルファ値を最大に

	if(p->ignore_alpha)
	{
		ps = buf + 3;
		
		for(i = param->info.width; i > 0; i--, ps += 4)
			*ps = 255;
	}

	//タイルにセット

	TileImage_setRowImage_RGBA8toRGBA16(p->img, p->y, buf,
		param->info.width, param->info.height, param->info.bottomup);

	//次へ

	if(param->info.bottomup)
		p->y--;
	else
		p->y++;

	if(p->prog)
		mPopupProgressThreadIncSubStep(p->prog);

	return MLOADIMAGE_ERR_OK;
}

/** ファイルから画像読み込み
 *
 * @param format   0 で自動判別
 * @param fileinfo NULL でなし
 * @param prog     NULL でなし
 * @param errmes   エラーメッセージ文字列のポインタが入る (NULLでなし。解放必要) */

TileImage *TileImage_loadFile(const char *filename,
	uint32_t format,mBool ignore_alpha,int maxsize,
	TileImageLoadFileInfo *fileinfo,mPopupProgress *prog,char **errmes)
{
	_loadimageinfo *p;
	mLoadImageFunc func;
	mBool ret;
	TileImage *img;

	p = (_loadimageinfo *)mLoadImage_create(sizeof(_loadimageinfo));
	if(!p) return NULL;

	p->dstinfo = fileinfo;
	p->prog = prog;
	p->ignore_alpha = ignore_alpha;

	p->base.format = MLOADIMAGE_FORMAT_RGBA;
	p->base.max_width = p->base.max_height = maxsize;
	p->base.src.type = MLOADIMAGE_SOURCE_TYPE_PATH;
	p->base.src.filename = filename;
	p->base.getinfo = _loadfile_getinfo;
	p->base.getrow = _loadfile_getrow;

	//読込関数

	if(format == 0)
	{
		//自動判別
		
		if(!mLoadImage_checkFormat(&p->base.src, &func, MLOADIMAGE_CHECKFORMAT_F_ALL))
			return NULL;
	}
	else
	{
		if(format & FILEFORMAT_PNG)
			func = mLoadImagePNG;
		else if(format & FILEFORMAT_JPEG)
			func = mLoadImageJPEG;
		else if(format & FILEFORMAT_GIF)
			func = mLoadImageGIF;
		else if(format & FILEFORMAT_BMP)
			func = mLoadImageBMP;
		else
			return NULL;
	}

	//実行

	ret = (func)(M_LOADIMAGE(p));

	img = p->img;

	if(!ret)
	{
		TileImage_free(img);
		img = NULL;

		if(errmes)
			*errmes = mStrdup(p->base.message);
	}

	mLoadImage_free(M_LOADIMAGE(p));

	return img;
}



//=================================
// PNG 保存
//=================================


typedef struct
{
	mSaveImage base;

	TileImage *img;
	mPopupProgress *prog;
	int cury;
}_saveimg_png;


/** Y1行を送る */

static int _savepng_send_row(mSaveImage *info,uint8_t *buf,int pitch)
{
	_saveimg_png *p = (_saveimg_png *)info;
	int ix,y,w;
	RGBAFix15 pix;

	y = p->cury;
	w = g_tileimage_dinfo.imgw;

	for(ix = 0; ix < w; ix++, buf += 4)
	{
		TileImage_getPixel(p->img, ix, y, &pix);

		buf[0] = RGBCONV_FIX15_TO_8(pix.r);
		buf[1] = RGBCONV_FIX15_TO_8(pix.g);
		buf[2] = RGBCONV_FIX15_TO_8(pix.b);
		buf[3] = RGBCONV_FIX15_TO_8(pix.a);
	}

	p->cury++;

	mPopupProgressThreadIncSubStep(p->prog);

	return MSAVEIMAGE_ERR_OK;
}

/** PNG (アルファ付き) に保存 */

mBool TileImage_savePNG_rgba(TileImage *p,const char *filename,int dpi,
	mPopupProgress *prog)
{
	_saveimg_png *info;
	mBool ret;

	info = (_saveimg_png *)mSaveImage_create(sizeof(_saveimg_png));
	if(!info) return FALSE;

	info->img = p;
	info->prog = prog;

	info->base.output.type = MSAVEIMAGE_OUTPUT_TYPE_PATH;
	info->base.output.filename = filename;
	info->base.width = g_tileimage_dinfo.imgw;
	info->base.height = g_tileimage_dinfo.imgh;
	info->base.sample_bits = 8;
	info->base.coltype = MSAVEIMAGE_COLTYPE_RGBA;
	info->base.resolution_unit = MSAVEIMAGE_RESOLITION_UNIT_DPI;
	info->base.resolution_horz = dpi;
	info->base.resolution_vert = dpi;
	info->base.send_row = _savepng_send_row;

	//保存

	mPopupProgressThreadBeginSubStep_onestep(prog, 20, g_tileimage_dinfo.imgh);

	ret = mSaveImagePNG(M_SAVEIMAGE(info));

	mSaveImage_free(M_SAVEIMAGE(info));

	return ret;
}


//=================================
// 選択範囲内イメージ PNG 保存
//=================================


typedef struct
{
	mSaveImage base;

	TileImage *img,*imgsel;
	mPopupProgress *prog;
	int left,cury;
}_saveimg_sel_png;


/** Y1行を送る */

static int _saveselpng_send_row(mSaveImage *info,uint8_t *buf,int pitch)
{
	_saveimg_sel_png *p = (_saveimg_sel_png *)info;
	TileImage *img,*selimg;
	int ix,y,w,left;
	RGBAFix15 pix;

	img = p->img;
	selimg = p->imgsel;
	left = p->left;
	y = p->cury;
	w = info->width;

	for(ix = 0; ix < w; ix++, buf += 4)
	{
		if(TileImage_isPixelTransparent(selimg, left + ix, y))
		{
			*((uint32_t *)buf) = 0;
		}
		else
		{
			TileImage_getPixel(img, left + ix, y, &pix);

			buf[0] = RGBCONV_FIX15_TO_8(pix.r);
			buf[1] = RGBCONV_FIX15_TO_8(pix.g);
			buf[2] = RGBCONV_FIX15_TO_8(pix.b);
			buf[3] = RGBCONV_FIX15_TO_8(pix.a);
		}
	}

	p->cury++;

	mPopupProgressThreadIncSubStep(p->prog);

	return MSAVEIMAGE_ERR_OK;
}

/** 選択範囲内イメージを PNG (アルファ付き) に保存 */

mBool TileImage_savePNG_select_rgba(TileImage *img,TileImage *selimg,
	const char *filename,int dpi,mRect *rc,mPopupProgress *prog)
{
	_saveimg_sel_png *info;
	mBool ret;

	info = (_saveimg_sel_png *)mSaveImage_create(sizeof(_saveimg_sel_png));
	if(!info) return FALSE;

	info->img = img;
	info->imgsel = selimg;
	info->left = rc->x1;
	info->cury = rc->y1;
	info->prog = prog;

	info->base.output.type = MSAVEIMAGE_OUTPUT_TYPE_PATH;
	info->base.output.filename = filename;
	info->base.width = rc->x2 - rc->x1 + 1;
	info->base.height = rc->y2 - rc->y1 + 1;
	info->base.sample_bits = 8;
	info->base.coltype = MSAVEIMAGE_COLTYPE_RGBA;
	info->base.resolution_unit = MSAVEIMAGE_RESOLITION_UNIT_DPI;
	info->base.resolution_horz = dpi;
	info->base.resolution_vert = dpi;
	info->base.send_row = _saveselpng_send_row;

	//保存

	mPopupProgressThreadBeginSubStep_onestep(prog, 20, info->base.height);

	ret = mSaveImagePNG(M_SAVEIMAGE(info));

	mSaveImage_free(M_SAVEIMAGE(info));

	return ret;
}


//=============================
// タイルのイメージファイル
// (選択範囲 コピー時)
//=============================


/** タイルイメージファイル保存 */

mBool TileImage_saveTileImageFile(TileImage *p,const char *filename,uint32_t col)
{
	FILE *fp;
	mZlibEncode *enc = NULL;
	uint8_t **pptile,tposbuf[4];
	mRect rc;
	int tnum,tx,ty,ix,iy;
	fpos_t fpos;
	mBool ret = FALSE;

	//開く

	fp = mFILEopenUTF8(filename, "wb");
	if(!fp) return FALSE;
	
	//カラータイプ、線の色

	mFILEwriteByte(fp, p->coltype);
	mFILEwrite32BE(fp, col);

	//------ タイルデータ

	//イメージのある範囲

	TileImage_getHaveImageRect_pixel(p, &rc, &tnum);

	//空イメージの場合

	if(tnum == 0)
	{
		mFILEwriteZero(fp, 4 * 6);
		fclose(fp);
		return TRUE;
	}

	//左上px

	mFILEwrite32BE(fp, rc.x1);
	mFILEwrite32BE(fp, rc.y1);

	//タイル幅、高さ

	mFILEwrite32BE(fp, (rc.x2 - rc.x1 + 1) >> 6);
	mFILEwrite32BE(fp, (rc.y2 - rc.y1 + 1) >> 6);

	//タイル数

	mFILEwrite32BE(fp, tnum);

	//圧縮サイズ(仮)

	fgetpos(fp, &fpos);
	mFILEwrite32BE(fp, 0);

	//zlib

	enc = mZlibEncodeNew_simple(0, 6);
	if(!enc) goto END;

	mZlibEncodeSetIO_stdio(enc, fp);

	//タイルイメージ

	pptile = p->ppbuf;
	ty = (p->offy - rc.y1) >> 6;

	for(iy = p->tileh; iy; iy--, ty++)
	{
		tx = (p->offx - rc.x1) >> 6;
	
		for(ix = p->tilew; ix; ix--, tx++, pptile++)
		{
			if(!(*pptile)) continue;

			//タイル位置

			mSetBuf16BE(tposbuf, tx);
			mSetBuf16BE(tposbuf + 2, ty);

			mZlibEncodeSend(enc, tposbuf, 4);

			//タイルデータ

			mZlibEncodeSend(enc, *pptile, p->tilesize);
		}
	}

	mZlibEncodeFlushEnd(enc);

	//圧縮サイズ

	fsetpos(fp, &fpos);
	mFILEwrite32BE(fp, mZlibEncodeGetWriteSize(enc));
	fseek(fp, 0, SEEK_END);

	//----------

	ret = TRUE;

END:
	mZlibEncodeFree(enc);

	fclose(fp);

	return ret;
}

/** タイルイメージファイル読み込み */

TileImage *TileImage_loadTileImageFile(const char *filename,uint32_t *pcol)
{
	TileImage *p = NULL;
	FILE *fp;
	mZlibDecode *dec = NULL;
	uint8_t coltype,tposbuf[4],err = TRUE;
	uint32_t col,tnum,tilew,tileh,encsize;
	int32_t offx,offy,tx,ty;
	TileImageInfo info;
	uint8_t **pptile;

	//開く

	fp = mFILEopenUTF8(filename, "rb");
	if(!fp) return NULL;

	//カラータイプ、線の色

	if(!mFILEreadByte(fp, &coltype)
		|| !mFILEread32BE(fp, &col)
		|| coltype >= TILEIMAGE_COLTYPE_NUM)
		goto END;

	*pcol = col;

	//情報

	if(!mFILEread32BE(fp, &offx)
		|| !mFILEread32BE(fp, &offy)
		|| !mFILEread32BE(fp, &tilew)
		|| !mFILEread32BE(fp, &tileh)
		|| !mFILEread32BE(fp, &tnum)
		|| !mFILEread32BE(fp, &encsize))
		goto END;

	//空イメージの場合

	if(tnum == 0)
	{
		p = TileImage_new(coltype, 1, 1);
		err = FALSE;
		goto END;
	}
	
	//イメージ作成

	info.offx = offx;
	info.offy = offy;
	info.tilew = tilew;
	info.tileh = tileh;

	p = TileImage_newFromInfo(coltype, &info);
	if(!p) goto END;

	//zlib

	dec = mZlibDecodeNew(0, 15);
	if(!dec) goto END;

	mZlibDecodeSetIO_stdio(dec, fp);
	mZlibDecodeSetInSize(dec, encsize);

	//------ タイルデータ

	for(; tnum; tnum--)
	{
		//タイル位置
	
		if(mZlibDecodeRead(dec, tposbuf, 4)) goto END;

		tx = mGetBuf16BE(tposbuf);
		ty = mGetBuf16BE(tposbuf + 2);

		if(tx >= tilew || ty >= tileh) goto END;

		//タイル確保

		pptile = TILEIMAGE_GETTILE_BUFPT(p, tx, ty);

		if(!TileImage_allocTile_atptr(p, pptile, FALSE))
			goto END;

		//タイルイメージ

		if(mZlibDecodeRead(dec, *pptile, p->tilesize))
			goto END;
	}

	if(mZlibDecodeReadEnd(dec)) goto END;

	//----------

	err = FALSE;

END:
	mZlibDecodeFree(dec);

	fclose(fp);

	//エラー時はイメージ解放

	if(!err)
		return p;
	else
	{
		TileImage_free(p);
		return NULL;
	}
}


//=============================
// 読み書き サブ
//=============================


/** RGBA 8bit のバッファイメージからセット
 *
 * [!] srcw x srch 分のタイル配列になっていること。
 * srcbuf の (0,0) = TileImage の (offx, offy)
 *
 * 以下の読み込み時。
 * [ APD ver1 ] [ PSD 1枚絵 ] */

mBool TileImage_setImage_fromRGBA8(TileImage *p,uint8_t *srcbuf,int srcw,int srch,
	mPopupProgress *prog,int substep)
{
	RGBAFix15 *tilebuf,*pd;
	uint8_t **pptile,*ps,empty;
	int i,ix,iy,tx,ty,xx,yy,dw,dh,pitch;
	TileImageColFunc_setTileFromRGBA settilefunc;

	//作業用バッファ

	tilebuf = (RGBAFix15 *)mMalloc(64 * 64 * 8, FALSE);
	if(!tilebuf) return FALSE;

	//

	mPopupProgressThreadBeginSubStep(prog, substep, p->tileh);

	settilefunc = g_tileimage_funcs[p->coltype].setTileFromRGBA;

	pptile = p->ppbuf;

	for(ty = p->tileh, yy = 0; ty > 0; ty--, yy += 64)
	{
		for(tx = p->tilew, xx = 0; tx > 0; tx--, xx += 64, pptile++)
		{
			//srcbuf 内の 64x64 範囲を RGBAFix15 バッファに

			dw = dh = 64;

			if(xx + 64 > srcw) dw = srcw - xx;
			if(yy + 64 > srch) dh = srch - yy;

			if(dw != 64 || dh != 64)
				mMemzero(tilebuf, 64 * 64 * 8);

			ps = srcbuf + (srcw << 2) * yy + (xx << 2);
			pd = tilebuf;
			pitch = (srcw - dw) << 2;
			empty = TRUE;

			for(iy = dh; iy; iy--, ps += pitch, pd += 64 - dw)
			{
				for(ix = dw; ix; ix--, ps += 4, pd++)
				{
					if(ps[3] == 0)
						pd->v64 = 0;
					else
					{
						for(i = 0; i < 4; i++)
							pd->c[i] = RGBCONV_8_TO_FIX15(ps[i]);

						empty = FALSE;
					}
				}
			}

			//タイル作成、セット (すべて透明なら何もしない)

			if(!empty)
			{
				*pptile = TileImage_allocTile(p, FALSE);
				if(!(*pptile))
				{
					mFree(tilebuf);
					return FALSE;
				}

				(settilefunc)(*pptile, tilebuf, FALSE);
			}
		}

		mPopupProgressThreadIncSubStep(prog);
	}

	mFree(tilebuf);

	return TRUE;
}

/** Y1行分の RGBA8 データから RGBA16 タイルにセット
 *
 * [!] p のタイル配列は width x height のサイズになっていること。
 *
 * 以下の読み込み時。
 * [ 通常画像 ] */

mBool TileImage_setRowImage_RGBA8toRGBA16(TileImage *p,
	int y,uint8_t *buf,int width,int height,mBool bottomup)
{
	uint8_t **pptile,**pp;
	RGBAFix15 *pd;
	int i,j,rw,pos,tiley;
	TileImageColFunc_isTransparentTile is_transparent = g_tileimage_funcs[TILEIMAGE_COLTYPE_RGBA].isTransparentTile;

	pptile = p->ppbuf + (y >> 6) * p->tilew;
	tiley = y & 63;

	//y がタイルの先頭行なら、横一列のタイルを確保

	if((!bottomup && tiley == 0) || (bottomup && (y == height - 1 || tiley == 63)))
	{
		for(i = p->tilew, pp = pptile; i; i--, pp++)
		{
			*pp = TileImage_allocTile(p, TRUE);
			if(!(*pp)) return FALSE;
		}
	}

	//タイル単位で RGBA8 => RGBA16

	pos = tiley << 6;

	for(i = p->tilew, rw = width, pp = pptile; i; i--, pp++, rw -= 64)
	{
		pd = (RGBAFix15 *)*pp + pos;
		j = (rw < 64)? rw: 64;
		
		for(; j > 0; j--, pd++, buf += 4)
		{
			pd->r = RGBCONV_8_TO_FIX15(buf[0]);
			pd->g = RGBCONV_8_TO_FIX15(buf[1]);
			pd->b = RGBCONV_8_TO_FIX15(buf[2]);
			pd->a = RGBCONV_8_TO_FIX15(buf[3]);
		}
	}

	//y がタイルの終端なら、透明タイルを解放

	if((!bottomup && (tiley == 63 || y == height - 1)) || (bottomup && tiley == 0))
	{
		for(i = p->tilew, pp = pptile; i; i--, pp++)
		{
			if((is_transparent)(*pp))
				TileImage_freeTile(pp);
		}
	}

	return TRUE;
}

/** PSD のチャンネルイメージをセット
 *
 * 色より先にアルファ値をセットすること。
 *
 * @param chno  -1:アルファ値 0,1,2:RGB */

mBool TileImage_setChannelImage(TileImage *p,uint8_t *chbuf,int chno,int srcw,int srch)
{
	uint8_t **pptile = p->ppbuf,*buf,*ps,*psX,*psY,*pd;
	uint32_t *p32;
	uint16_t *p16;
	int tx,ty,gray,i,pitch,w,h,remw,remh,real_chno;

	gray = (p->coltype == TILEIMAGE_COLTYPE_GRAY);

	real_chno = chno;
	if(chno < 0) real_chno = (gray)? 1: 3;

	//

	buf = (uint8_t *)mMalloc(64 * 64, FALSE);
	if(!buf) return FALSE;

	//

	psY = chbuf;
	pitch = srcw * 64;

	for(ty = p->tileh, remh = srch; ty; ty--, psY += pitch, remh -= 64)
	{
		psX = psY;
		h = (remh > 64)? 64: remh;
		
		for(tx = p->tilew, remw = srcw; tx; tx--, pptile++, psX += 64, remw -= 64)
		{
			/* 色のチャンネルで、タイルが作成されていない場合、
			 * すべて透明なので、何もしない */

			if(chno >= 0 && !(*pptile)) continue;
		
			//buf に 64x64 で取得

			ps = psX;
			pd = buf;
			w = (remw > 64)? 64: remw;

			if(w != 64 || h != 64)
				memset(buf, 0, 64 * 64);

			for(i = h; i; i--)
			{
				memcpy(pd, ps, w);

				ps += srcw;
				pd += 64;
			}

			//アルファ値の場合、タイル作成

			if(chno < 0)
			{
				//すべて 0 なら何もしない

				p32 = (uint32_t *)buf;

				for(i = 64 * 64 / 4; i && !(*p32); i--, p32++);

				if(i == 0) continue;
			
				//タイル作成

				*pptile = TileImage_allocTile(p, TRUE);
				if(!(*pptile))
				{
					mFree(buf);
					return FALSE;
				}
			}

			//タイルにセット

			ps = buf;
			p16 = ((uint16_t *)*pptile) + real_chno;

			if(gray)
			{
				for(i = 64 * 64; i; i--, p16 += 2, ps++)
					*p16 = RGBCONV_8_TO_FIX15(*ps);
			}
			else
			{
				for(i = 64 * 64; i; i--, p16 += 4, ps++)
					*p16 = RGBCONV_8_TO_FIX15(*ps);
			}
		}
	}

	mFree(buf);

	return TRUE;
}

/** RGBA8 タイルから RGBA16 タイルにセット
 *
 * APD v3 読み込み */

void TileImage_setTile_RGBA8toRGBA16(uint8_t *dst,uint8_t *src)
{
	uint16_t *pd = (uint16_t *)dst;
	int i;

	for(i = 64 * 64 * 4; i; i--, src++)
		*(pd++) = RGBCONV_8_TO_FIX15(*src);
}

/** A8 タイルとして読み込まれたデータを A16 に変換
 *
 * ADW ver2 読み込み */

void TileImage_convertTile_A8toA16(uint8_t *tile)
{
	uint8_t *ps;
	uint16_t *pd;
	int i;

	ps = tile + 64 * 64 - 1;
	pd = (uint16_t *)tile + 64 * 64 - 1;

	for(i = 64 * 64; i; i--, ps--)
		*(pd--) = RGBCONV_8_TO_FIX15(*ps);
}
