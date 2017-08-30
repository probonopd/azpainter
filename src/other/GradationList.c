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

/*************************************
 * グラデーションデータ
 *************************************/
/*
 * データが更新された時のみ終了時に保存される。
 * データを更新したら exit_save を TRUE にすること。
 */

#include <string.h>
#include <stdio.h>

#include "mDef.h"
#include "mStr.h"
#include "mGui.h"
#include "mList.h"
#include "mPixbuf.h"
#include "mUtilStdio.h"

#include "defMacros.h"
#include "ColorValue.h"

#include "GradationList.h"


//------------------

typedef struct
{
	mList list;
	mBool exit_save;
}_graddat;

static _graddat g_dat;

//------------------

#define _GETVAL16(p)  ((*(p) << 8) | *((p) + 1))
#define _GETPOS(p)    (((*(p) << 8) | *((p) + 1)) & GRADDAT_POS_MASK)

//------------------

//描画色->背景色 の描画用データ (先頭のフラグは含まない)

static const uint8_t g_default_data[] = {
	2,2,
	0,0, 0,0,0, 0x44,0, 0,0,0,
	0,0, 4,0, 4,0, 4,0
};

//------------------


//============================
// sub
//============================


/** アイテム破棄 */

static void _destroy_item(mListItem *item)
{
	mFree(GRADITEM(item)->buf);
}

/** リストにアイテム追加 & データバッファ確保 */

static GradItem *_additem_allocbuf(int bufsize)
{
	GradItem *pi;

	pi = (GradItem *)mListAppendNew(&g_dat.list, sizeof(GradItem), _destroy_item);
	if(pi)
	{
		pi->size = bufsize;

		pi->buf = (uint8_t *)mMalloc(bufsize, FALSE);
		if(!pi->buf)
		{
			mListDelete(&g_dat.list, M_LISTITEM(pi)); 
			pi = NULL;
		}
	}

	return pi;
}

/** デフォルトデータセット (黒->白) */

static mBool _item_set_default(GradItem *pi)
{
	uint8_t dat[] = {
		0,2,2,
		0x80,0,0,0,0, 0x84,0,0xff,0xff,0xff,
		0,0,4,0, 4,0,4,0 };
	int size;

	size = sizeof(dat);

	pi->buf = (uint8_t *)mMalloc(size, FALSE);
	if(!pi->buf) return FALSE;

	memcpy(pi->buf, dat, size);

	pi->size = size;

	return TRUE;
}



/**********************************
 * GradItem
 **********************************/
/*
 * [BigEndian]
 * 
 * 1byte : フラグ (0bit:繰り返す 1bit:単色)
 * 1byte : 色の数
 * 1byte : Aの数
 * <色>
 *  2byte : 位置 (0-1024) [14-15bit:色タイプ (0:描画色 1:背景色 2:指定色)]
 *  3byte : R,G,B
 * <A>
 *  2byte : 位置
 *  2byte : A (0-1024)
 */


/** 色を取得 */

static int _get_rgb_col(uint8_t *colbuf,RGB8 *pix)
{
	int type,val;

	val = _GETVAL16(colbuf);

	type = val >> 14;

	if(type == 0)
		//描画色
		pix->r = pix->g = pix->b = 0;
	else if(type == 1)
		//背景色
		pix->r = pix->g = pix->b = 255;
	else
	{
		//指定色
		
		pix->r = colbuf[2];
		pix->g = colbuf[3];
		pix->b = colbuf[4];
	}

	return val & GRADDAT_POS_MASK;
}

/** mPixbuf にグラデーション描画
 *
 * 描画色は黒、背景色は白とする。
 * 4x4 のチェック柄を背景にしてアルファ合成。 */

void GradItem_draw(mPixbuf *pixbuf,
	int x,int y,int w,int h,uint8_t *colbuf,uint8_t *abuf,int single_col)
{
	int ix,iy,pos,n1,n2,a,xf,yy,i,colpos_l,colpos_r,apos_l,apos_r,alpha_l,alpha_r;
	uint8_t *pd,*pdy;
	mBox box;
	RGB8 pixl,pixr,pixsrc,pixdst,*pixg,
		pixbkgnd[2] = {{{0xff,0xe8,0xeb}},{{0xff,0xc8,0xce}}};

	if(!mPixbufGetClipBox_d(pixbuf, &box, x, y, w, h))
		return;

	pd = mPixbufGetBufPtFast(pixbuf, box.x, box.y);
	x = box.x - x;
	y = box.y - y;

	//

	colpos_l = _get_rgb_col(colbuf, &pixl);
	colpos_r = _get_rgb_col(colbuf + 5, &pixr);

	apos_l = 0;
	alpha_l = _GETVAL16(abuf + 2);

	apos_r = _GETVAL16(abuf + 4);
	alpha_r = _GETVAL16(abuf + 6);

	//高さ 8px まで横一列を描画

	h = box.h;
	if(h > 8) h = 8;

	for(ix = box.w; ix; ix--, x++, pd += pixbuf->bpp)
	{
		pos = (x << GRADDAT_POS_BIT) / (w - 1);

		//------ RGB

		while(pos > colpos_r)
		{
			colbuf += 5;
			
			colpos_l = colpos_r;
			pixl = pixr;

			colpos_r = _get_rgb_col(colbuf, &pixr);
		}

		if(single_col)
			pixsrc = pixl;
		else
		{
			n1 = pos - colpos_l;
			n2 = colpos_r - colpos_l;

			for(i = 0; i < 3; i++)
				pixsrc.c[i] = (pixr.c[i] - pixl.c[i]) * n1 / n2 + pixl.c[i];
		}

		//----- Alpha

		while(pos > apos_r)
		{
			abuf += 4;

			apos_l = apos_r;
			alpha_l = alpha_r;

			apos_r = _GETVAL16(abuf);
			alpha_r = _GETVAL16(abuf + 2);
		}

		a = (alpha_r - alpha_l) * (pos - apos_l) / (apos_r - apos_l) + alpha_l;

		//----- 描画

		pdy = pd;
		xf = (x & 7) >> 2;
		yy = y;

		for(iy = h; iy; iy--, yy++, pdy += pixbuf->pitch_dir)
		{
			pixg = pixbkgnd + (xf ^ ((yy & 7) >> 2));

			for(i = 0; i < 3; i++)
				pixdst.c[i] = (((pixsrc.c[i] - pixg->c[i]) * a) >> GRADDAT_ALPHA_BIT) + pixg->c[i];

			(pixbuf->setbuf)(pdy, mRGBtoPix2(pixdst.r, pixdst.g, pixdst.b));
		}
	}

	//残り Y はコピー

	n1 = pixbuf->pitch_dir;
	n2 = box.w * pixbuf->bpp;
	pdy = mPixbufGetBufPtFast(pixbuf, box.x, box.y);
	pd  = pdy + n1 * h;

	for(h = box.h - h; h > 0; h--)
	{
		memcpy(pd, pdy, n2);

		pd += n1;
		pdy += n1;
	}
}

/** グラデーションデータをセット (編集完了時) */

void GradItem_setDat(GradItem *p,
	uint8_t flags,int colnum,int anum,uint8_t *colbuf,uint8_t *abuf)
{
	uint8_t *buf;
	int colsize,asize;

	mFree(p->buf);

	colsize = colnum * 5;
	asize = anum * 4;

	//確保

	p->size = 3 + colsize + asize;
	
	p->buf = (uint8_t *)mMalloc(p->size, FALSE);
	if(!p->buf) return;

	//データセット

	buf = p->buf;

	buf[0] = flags;
	buf[1] = colnum;
	buf[2] = anum;
	buf += 3;

	memcpy(buf, colbuf, colsize);
	memcpy(buf + colsize, abuf, asize);

	//

	g_dat.exit_save = TRUE;
}

/** 描画用データに変換
 *
 * @param src  先頭のフラグデータは除く */

uint16_t *GradItem_convertDrawData(const uint8_t *src,RGBFix15 *drawcol,RGBFix15 *bkgndcol)
{
	uint16_t *dstbuf,*pd;
	const uint8_t *ps;
	int i,n;

	dstbuf = (uint16_t *)mMalloc(4 + src[0] * 8 + src[1] * 4, FALSE);
	if(!dstbuf) return NULL;

	pd = dstbuf;

	//色、Aの数

	*(pd++) = src[0];
	*(pd++) = src[1];

	//色データ

	ps = src + 2;

	for(i = src[0]; i > 0; i--, pd += 4, ps += 5)
	{
		//位置
		
		n = _GETVAL16(ps);
		
		pd[0] = (n & GRADDAT_POS_MASK) << (15 - GRADDAT_POS_BIT);

		//RGB

		n >>= 14;

		if(n == 0)
			*((RGBFix15 *)(pd + 1)) = *drawcol;
		else if(n == 1)
			*((RGBFix15 *)(pd + 1)) = *bkgndcol;
		else
		{
			pd[1] = RGBCONV_8_TO_FIX15(ps[2]);
			pd[2] = RGBCONV_8_TO_FIX15(ps[3]);
			pd[3] = RGBCONV_8_TO_FIX15(ps[4]);
		}
	}

	//Aデータ

	ps = src + 2 + src[0] * 5;

	for(i = src[1] * 2; i > 0; i--, ps += 2)
		*(pd++) = _GETVAL16(ps) << (15 - GRADDAT_ALPHA_BIT);
	
	return dstbuf;
}


/**********************************
 * グラデーションリストデータ
 **********************************/


/** 初期化 */

void GradationList_init()
{
	memset(&g_dat, 0, sizeof(_graddat));
}

/** 解放 */

void GradationList_free()
{
	mListDeleteAll(&g_dat.list);
}

/** 新規データ追加 */

GradItem *GradationList_newItem()
{
	GradItem *pi;

	if(g_dat.list.num >= GRADATION_MAX_ITEM) return NULL;

	pi = (GradItem *)mListAppendNew(&g_dat.list, sizeof(GradItem), _destroy_item);
	if(!pi) return NULL;

	if(!_item_set_default(pi))
	{
		mListDelete(&g_dat.list, M_LISTITEM(pi));
		return NULL;
	}

	g_dat.exit_save = TRUE;

	return pi;
}

/** データ削除 */

void GradationList_delItem(GradItem *item)
{
	mListDelete(&g_dat.list, M_LISTITEM(item));

	g_dat.exit_save = TRUE;
}

/** アイテム複製 */

GradItem *GradationList_copyItem(GradItem *item)
{
	GradItem *pi;

	pi = _additem_allocbuf(item->size);
	if(!pi) return NULL;

	memcpy(pi->buf, item->buf, item->size);

	g_dat.exit_save = TRUE;

	return pi;
}

/** 先頭アイテム取得 */

GradItem *GradationList_getTopItem()
{
	return (GradItem *)g_dat.list.top;
}

/** インデックスからデータバッファ取得 */

uint8_t *GradationList_getBuf_atIndex(int index)
{
	GradItem *pi;

	pi = (GradItem *)mListGetItemByIndex(&g_dat.list, index);
	if(!pi) return NULL;

	return pi->buf;
}

/** 描画用のデフォルトデータを取得 (先頭のフラグデータは除く) */

const uint8_t *GradationList_getDefaultDrawData()
{
	return g_default_data;
}


//============================
// 読み書き
//============================


/** ファイル開く */

static FILE *_open_configfile(const char *mode)
{
	mStr str = MSTR_INIT;
	FILE *fp;

	mAppGetConfigPath(&str, CONFIG_FILENAME_GRADATION);

	fp = mFILEopenUTF8(str.buf, mode);

	mStrFree(&str);

	return fp;
}

/** 設定データから読み込み */

void GradationList_load()
{
	FILE *fp;
	GradItem *pi;
	uint8_t ver,num,head[3];
	int size;

	fp = _open_configfile("rb");
	if(!fp) return;

	if(!mFILEreadCompareStr(fp, "AZPLGRAD")
		|| !mFILEreadByte(fp, &ver)
		|| ver != 0
		|| !mFILEreadByte(fp, &num))
		goto END;

	if(num > GRADATION_MAX_ITEM)
		num = GRADATION_MAX_ITEM;

	for(; num; num--)
	{
		//ヘッダ

		fread(head, 1, 3, fp);

		size = head[1] * 5 + head[2] * 4;
	
		//アイテム追加

		pi = _additem_allocbuf(3 + size);
		if(!pi) break;

		memcpy(pi->buf, head, 3);

		//データ読み込み

		fread(pi->buf + 3, 1, size, fp);
	}

END:
	fclose(fp);
}

/** 設定データに保存 */

void GradationList_save()
{
	FILE *fp;
	GradItem *pi;

	//データが更新されていない

	if(!g_dat.exit_save) return;

	//

	fp = _open_configfile("wb");
	if(!fp) return;

	fputs("AZPLGRAD", fp);
	mFILEwriteByte(fp, 0);

	mFILEwriteByte(fp, g_dat.list.num);

	for(pi = GRADITEM(g_dat.list.top); pi; pi = GRADITEM(pi->i.next))
		fwrite(pi->buf, 1, pi->size, fp);

	fclose(fp);
}
