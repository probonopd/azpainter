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

/***************************
 * カラーパレットデータ
 ***************************/
/*
 * 内容が変更されたときは ColorPalette::exit_save を TRUE にして、
 * 終了時に保存されるようにする。
 */

#include <string.h>
#include <stdio.h>

#include "mDef.h"
#include "mStr.h"
#include "mGui.h"
#include "mUtilStdio.h"

#include "defMacros.h"

#include "ColorPalette.h"


//--------------------

static ColorPalette g_colpal_body;

ColorPalette *g_colorpalette = &g_colpal_body;

#define _BODY  g_colpal_body

//--------------------



//==================================
// sub
//==================================


/** 設定ファイル開く */

static FILE *_open_configfile(const char *fname,mBool write)
{
	mStr str = MSTR_INIT;
	FILE *fp;

	mAppGetConfigPath(&str, fname);

	fp = mFILEopenUTF8(str.buf, (write)? "wb": "rb");

	mStrFree(&str);

	return fp;
}

/** 設定ファイルから読み込み */

static void _load_configfile()
{
	FILE *fp;
	ColorPaletteDat *ppal;
	uint8_t ver,tabnum;
	uint16_t num;
	int i;

	//開く

	fp = _open_configfile(CONFIG_FILENAME_COLPALETTE, FALSE);
	if(!fp) return;

	//ヘッダ

	if(!mFILEreadCompareStr(fp, "AZPLPLD")
		|| !mFILEreadByte(fp, &ver)
		|| ver != 1
		|| !mFILEreadByte(fp, &tabnum))
	{
		fclose(fp);
		return;
	}

	if(tabnum > COLORPALETTE_NUM)
		tabnum = COLORPALETTE_NUM;

	//パレットデータ

	ppal = _BODY.pal;

	for(i = 0; i < tabnum; i++, ppal++)
	{
		//個数 (データなし、または 0 で終端)

		if(!mFILEread16LE(fp, &num) || num == 0 || num > COLORPALETTEDAT_MAXNUM)
			break;

		//確保

		if(!ColorPaletteDat_alloc(ppal, num)) break;

		//データ

		if(fread(ppal->buf, 1, num * 3, fp) != num * 3)
			break;
	}

	fclose(fp);
}


//===============================
// ColorPalette (メインデータ)
//===============================


/** 初期化 */

void ColorPalette_init()
{
	memset(g_colorpalette, 0, sizeof(ColorPalette));
}

/** 解放 */

void ColorPalette_free()
{
	int i;

	for(i = 0; i < COLORPALETTE_NUM; i++)
		ColorPaletteDat_free(_BODY.pal + i);
}

/** 設定ファイル読み込み */

void ColorPalette_load()
{
	ColorPaletteDat *p;
	int i;

	//ファイル読み込み

	_load_configfile(CONFIG_FILENAME_COLPALETTE);

	//確保されていないパレットを作成

	p = _BODY.pal;

	for(i = 0; i < COLORPALETTE_NUM; i++, p++)
	{
		if(!p->buf)
		{
			if(ColorPaletteDat_alloc(p, (i == 0)? 128: 64))
				ColorPaletteDat_fillWhite(p);
		}
	}
}

/** 設定ファイル書き込み */

void ColorPalette_savefile()
{
	FILE *fp;
	ColorPaletteDat *ppal;
	int i;

	//データが変更された時のみ

	if(!_BODY.exit_save) return;

	//開く

	fp = _open_configfile(CONFIG_FILENAME_COLPALETTE, TRUE);
	if(!fp) return;

	//ヘッダ

	fputs("AZPLPLD", fp);

	mFILEwriteByte(fp, 1);
	mFILEwriteByte(fp, COLORPALETTE_NUM);

	//パレットデータ

	ppal = _BODY.pal;

	for(i = 0; i < COLORPALETTE_NUM; i++, ppal++)
	{
		//個数

		mFILEwrite16LE(fp, ppal->num);

		//データ

		fwrite(ppal->buf, 1, ppal->num * 3, fp);
	}

	fclose(fp);
}

/** ver.1 の設定データを変換 */

void ColorPalette_convert_from_ver1()
{
	FILE *fpin,*fpout;
	uint8_t ver,tabnum,*buf,*ps,*pd,r,g,b;
	uint16_t num;
	int i,j,size;

	//読み込み

	fpin = _open_configfile("palette.dat", FALSE);
	if(!fpin) return;
	
	if(!mFILEreadCompareStr(fpin, "AZPLPLD")
		|| !mFILEreadByte(fpin, &ver)
		|| ver != 0
		|| !mFILEreadByte(fpin, &tabnum))
	{
		fclose(fpin);
		return;
	}

	if(tabnum > COLORPALETTE_NUM)
		tabnum = COLORPALETTE_NUM;

	//書き込み

	fpout = _open_configfile(CONFIG_FILENAME_COLPALETTE, TRUE);
	if(!fpout)
	{
		fclose(fpin);
		return;
	}

	fputs("AZPLPLD", fpout);
	mFILEwriteByte(fpout, 1);
	mFILEwriteByte(fpout, tabnum);

	//パレットデータ

	for(i = 0; i < tabnum; i++)
	{
		//個数 (データなし、または 0 で終端)

		if(!mFILEread16LE(fpin, &num) || num == 0 || num > COLORPALETTEDAT_MAXNUM)
			break;

		//データ読み込み

		size = num << 2;

		buf = (uint8_t *)mMalloc(size, FALSE);
		if(!buf) break;

		if(fread(buf, 1, size, fpin) != size)
		{
			mFree(buf);
			break;
		}
		
		//ver.2 のデータに変換 (BGRA -> RGB)

		ps = pd = buf;

		for(j = num; j; j--, ps += 4, pd += 3)
		{
			r = ps[2], g = ps[1], b = ps[0];
			pd[0] = r, pd[1] = g, pd[2] = b;
		}

		//書き込み

		fwrite(&num, 1, 2, fpout);
		fwrite(buf, 1, num * 3, fpout);

		mFree(buf);
	}

	fclose(fpin);
	fclose(fpout);
}



//===============================
// ColorPaletteDat (各データ)
//===============================



/** バッファサイズ変更 */

static mBool _resize_pal(ColorPaletteDat *p,int num)
{
	uint8_t *pbuf;

	pbuf = mRealloc(p->buf, num * 3);
	if(!pbuf) return FALSE;

	p->buf = pbuf;
	p->num = num;

	return TRUE;
}



/** 解放 */

void ColorPaletteDat_free(ColorPaletteDat *p)
{
	if(p->buf)
	{
		mFree(p->buf);
		p->buf = NULL;
		p->num = 0;
	}
}

/** バッファ確保 */

mBool ColorPaletteDat_alloc(ColorPaletteDat *p,int num)
{
	ColorPaletteDat_free(p);

	if(num < 1)
		num = 1;
	else if(num > COLORPALETTEDAT_MAXNUM)
		num = COLORPALETTEDAT_MAXNUM;

	p->buf = (uint8_t *)mMalloc(num * 3, FALSE);
	if(!p->buf) return FALSE;

	p->num = num;

	return TRUE;
}

/** サイズ変更 */

mBool ColorPaletteDat_resize(ColorPaletteDat *p,int num)
{
	int num_src;

	num_src = p->num;

	//リサイズ

	if(!_resize_pal(p, num))
		return FALSE;

	//増えた分をクリア

	if(num > num_src)
		memset(p->buf + num_src * 3, 255, (num - num_src) * 3);

	_BODY.exit_save = TRUE;

	return TRUE;
}

/** 白で埋める */

void ColorPaletteDat_fillWhite(ColorPaletteDat *p)
{
	memset(p->buf, 255, p->num * 3);
}

/** RGB 値を取得 */

uint32_t ColorPaletteDat_getColor(ColorPaletteDat *p,int no)
{
	uint8_t *buf;

	buf = p->buf + no * 3;

	return M_RGB(buf[0], buf[1], buf[2]);
}

/** RGB 値をセット */

void ColorPaletteDat_setColor(ColorPaletteDat *p,int no,uint32_t col)
{
	uint8_t *buf = p->buf + no * 3;

	buf[0] = M_GET_R(col);
	buf[1] = M_GET_G(col);
	buf[2] = M_GET_B(col);

	_BODY.exit_save = TRUE;
}

/** 指定間をグラデーション */

void ColorPaletteDat_gradation(ColorPaletteDat *p,int no1,int no2)
{
	int i,j,len;
	uint8_t *p1,*p2,*pd;
	double d;

	if(no1 > no2)
		i = no1, no1 = no2, no2 = i;

	len = no2 - no1;
	if(len < 2) return;

	//

	p1 = p->buf + no1 * 3;
	p2 = p->buf + no2 * 3;
	pd = p1 + 3;

	for(i = 1; i < len; i++, pd += 3)
	{
		d = (double)i / len;
		
		for(j = 0; j < 3; j++)
			pd[j] = (int)((p2[j] - p1[j]) * d + p1[j] + 0.5);
	}

	_BODY.exit_save = TRUE;
}

/** 24bit 画像からパレット作成 */

void ColorPaletteDat_createFromImage(ColorPaletteDat *p,uint8_t *buf,int w,int h)
{
	uint32_t *palbuf,*pp,col;
	int num,ix,iy,i;

	//作業用に最大数分を確保

	palbuf = (uint32_t *)mMalloc(COLORPALETTEDAT_MAXNUM * 4, FALSE);
	if(!palbuf) return;

	//取得

	num = 0;

	for(iy = h; iy; iy--)
	{
		for(ix = w; ix; ix--, buf += 3)
		{
			col = M_RGB(buf[0], buf[1], buf[2]);

			//パレットにあるか

			for(i = num, pp = palbuf; i; i--, pp++)
			{
				if(*pp == col) break;
			}

			//追加

			if(i == 0)
			{
				if(num == COLORPALETTEDAT_MAXNUM) goto END;

				palbuf[num++] = col;
			}
		}
	}

END:
	//パレットセット

	if(ColorPaletteDat_alloc(p, num))
	{
		buf = p->buf;
		pp = palbuf;
	
		for(i = num; i > 0; i--, buf += 3)
		{
			col = *(pp++);

			buf[0] = M_GET_R(col);
			buf[1] = M_GET_G(col);
			buf[2] = M_GET_B(col);
		}

		_BODY.exit_save = TRUE;
	}

	mFree(palbuf);
}


//===============================
// パレットファイル読み込み
//===============================


/** APL 読み込み (LE) */

static mBool _loadpal_apl(ColorPaletteDat *p,const char *filename)
{
	FILE *fp;
	uint8_t ver,c[4],*pd;
	uint16_t num;
	int i;
	mBool ret = FALSE;

	fp = mFILEopenUTF8(filename, "rb");
	if(!fp) return FALSE;

	//ヘッダ

	if(!mFILEreadCompareStr(fp, "AZPPAL")
		|| !mFILEreadByte(fp, &ver)
		|| ver != 0)
		goto END;

	//個数

	if(!mFILEread16LE(fp, &num) || num == 0)
		goto END;

	//確保

	if(!ColorPaletteDat_alloc(p, num))
		goto END;

	//データ

	pd = p->buf;

	for(i = p->num; i > 0; i--, pd += 3)
	{
		if(fread(c, 1, 4, fp) != 4) break;

		pd[0] = c[2];
		pd[1] = c[1];
		pd[2] = c[0];
	}

	ret = TRUE;
END:
	fclose(fp);

	return ret;
}

/** ACO 読み込み (BE) */

static mBool _loadpal_aco(ColorPaletteDat *p,const char *filename)
{
	FILE *fp;
	uint16_t ver,num,len;
	uint8_t dat[10],*pd;
	mBool ret = FALSE;
	int i;

	fp = mFILEopenUTF8(filename, "rb");
	if(!fp) return FALSE;

	//バージョン

	if(!mFILEread16BE(fp, &ver)
		|| (ver != 1 && ver != 2))
		goto END;

	//個数

	if(!mFILEread16BE(fp, &num) || num == 0)
		goto END;

	//確保

	if(!ColorPaletteDat_alloc(p, num))
		goto END;

	//データ

	pd = p->buf;

	for(i = p->num; i > 0; i--, pd += 3)
	{
		/* [2byte] color space (0:RGB)
		 * [2byte x 4] R,G,B,Z */

		if(fread(dat, 1, 10, fp) != 10) break;

		//ver2 時、色名をスキップ

		if(ver == 2)
		{
			fseek(fp, 2, SEEK_CUR);
			
			if(!mFILEread16BE(fp, &len)) break;
			fseek(fp, len << 1, SEEK_CUR);
		}

		//RGB (上位バイトのみ)

		pd[0] = dat[2];
		pd[1] = dat[4];
		pd[2] = dat[6];
	}

	ret = TRUE;
END:
	fclose(fp);

	return ret;
}

/** パレットファイル読み込み (APL,ACO) */

mBool ColorPaletteDat_loadFile(ColorPaletteDat *p,const char *filename)
{
	mStr ext = MSTR_INIT;
	mBool ret;

	//拡張子から判定

	mStrPathGetExt(&ext, filename);

	if(mStrCompareCaseEq(&ext, "apl"))
		ret = _loadpal_apl(p, filename);
	else if(mStrCompareCaseEq(&ext, "aco"))
		ret = _loadpal_aco(p, filename);
	else
		ret = FALSE;

	mStrFree(&ext);

	if(ret) _BODY.exit_save = TRUE;

	return ret;
}


//===============================
// パレットファイル保存
//===============================


/** APL ファイルに保存 */

mBool ColorPaletteDat_saveFile_apl(ColorPaletteDat *p,const char *filename)
{
	FILE *fp;
	int i;
	uint8_t d[4],*ps;

	fp = mFILEopenUTF8(filename, "wb");
	if(!fp) return FALSE;

	fputs("AZPPAL", fp);
	mFILEwriteByte(fp, 0);

	mFILEwrite16LE(fp, p->num);

	ps = p->buf;
	d[3] = 0;

	for(i = p->num; i > 0; i--, ps += 3)
	{
		d[0] = ps[2];
		d[1] = ps[1];
		d[2] = ps[0];

		fwrite(d, 1, 4, fp);
	}

	fclose(fp);

	return TRUE;
}

/** ACO ファイルに保存 */

mBool ColorPaletteDat_saveFile_aco(ColorPaletteDat *p,const char *filename)
{
	FILE *fp;
	int i;
	uint8_t d[10],*ps;

	fp = mFILEopenUTF8(filename, "wb");
	if(!fp) return FALSE;

	mFILEwrite16BE(fp, 1);
	mFILEwrite16BE(fp, p->num);

	ps = p->buf;
	memset(d, 0, 10);

	for(i = p->num; i > 0; i--, ps += 3)
	{
		d[2] = ps[0];
		d[4] = ps[1];
		d[6] = ps[2];

		fwrite(d, 1, 10, fp);
	}

	fclose(fp);

	return TRUE;
}

