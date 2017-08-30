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

/**************************************
 * フィルタ処理
 *
 * カラー/アルファ値
 **************************************/

#include <math.h>

#include "mDef.h"
#include "mPopupProgress.h"
#include "mRandXorShift.h"

#include "defTileImage.h"
#include "TileImage.h"
#include "LayerList.h"
#include "LayerItem.h"
#include "ImageBuf8.h"

#include "defDraw.h"
#include "draw_op_sub.h"

#include "FilterDrawInfo.h"
#include "filter_sub.h"


//--------------------

typedef void (*ColorCommonFunc)(FilterDrawInfo *,int,int,RGBAFix15 *);

//--------------------


//===========================
// カラー処理共通
//===========================
/*
 * - レイヤのカラータイプが A16/A1 の場合、
 *   処理しても効果はないので、ダイアログ前に弾いている。
 *   なので、RGBA or GRAY のタイプのみ。
 * 
 * - ダイアログ中のキャンバスプレビュー時は、
 *   imgdst は imgsrc から複製されている。
 *
 * - RGB のみの操作なので、A=0 の点は変更なしとなる。
 *   そのため、確保されているタイルだけ処理すれば良い。
 */


/** キャンバスプレビュー用フィルタ処理 (RGBA) */

static mBool _color_common_forPreview_RGBA(FilterDrawInfo *info,ColorCommonFunc func)
{
	TileImageTileRectInfo tinfo;
	TileImage *imgsel;
	uint8_t **ppsrc,**ppdst;
	RGBAFix15 *ps,*pd,pix;
	int px,py,tx,ty,ix,iy,xx,yy;

	imgsel = info->imgsel;

	//タイル範囲

	ppsrc = TileImage_getTileRectInfo(info->imgsrc, &tinfo, &info->box);
	if(!ppsrc) return TRUE;

	ppdst = TILEIMAGE_GETTILE_BUFPT(info->imgdst, tinfo.rctile.x1, tinfo.rctile.y1);

	//タイルごとに処理

	py = tinfo.pxtop.y;

	for(ty = tinfo.rctile.y1; ty <= tinfo.rctile.y2; ty++, py += 64)
	{
		px = tinfo.pxtop.x;

		for(tx = tinfo.rctile.x1; tx <= tinfo.rctile.x2; tx++, px += 64, ppsrc++, ppdst++)
		{
			if(!(*ppsrc)) continue;

			//タイル単位で処理

			ps = (RGBAFix15 *)*ppsrc;
			pd = (RGBAFix15 *)*ppdst;

			for(iy = 0, yy = py; iy < 64; iy++, yy++)
			{
				for(ix = 0, xx = px; ix < 64; ix++, xx++, ps++, pd++)
				{
					if(ps->a
						&& tinfo.rcclip.x1 <= xx && xx < tinfo.rcclip.x2
						&& tinfo.rcclip.y1 <= yy && yy < tinfo.rcclip.y2
						&& (!imgsel || TileImage_isPixelOpaque(imgsel, xx, yy)))
					{
						pix = *ps;

						(func)(info, xx, yy, &pix);

						*pd = pix;
					}
				}
			}
		}

		ppsrc += tinfo.pitch;
		ppdst += tinfo.pitch;
	}

	return TRUE;
}

/** キャンバスプレビュー用フィルタ処理 (GRAY) */

static mBool _color_common_forPreview_GRAY(FilterDrawInfo *info,ColorCommonFunc func)
{
	TileImageTileRectInfo tinfo;
	TileImage *imgsel;
	uint8_t **ppsrc,**ppdst;
	uint16_t *ps,*pd;
	int px,py,tx,ty,ix,iy,xx,yy;
	RGBAFix15 pix;

	imgsel = info->imgsel;

	//タイル範囲

	ppsrc = TileImage_getTileRectInfo(info->imgsrc, &tinfo, &info->box);
	if(!ppsrc) return TRUE;

	ppdst = TILEIMAGE_GETTILE_BUFPT(info->imgdst, tinfo.rctile.x1, tinfo.rctile.y1);

	//タイルごとに処理

	py = tinfo.pxtop.y;

	for(ty = tinfo.rctile.y1; ty <= tinfo.rctile.y2; ty++, py += 64)
	{
		px = tinfo.pxtop.x;

		for(tx = tinfo.rctile.x1; tx <= tinfo.rctile.x2; tx++, px += 64, ppsrc++, ppdst++)
		{
			if(!(*ppsrc)) continue;

			//タイル単位で処理

			ps = (uint16_t *)*ppsrc;
			pd = (uint16_t *)*ppdst;

			for(iy = 0, yy = py; iy < 64; iy++, yy++)
			{
				for(ix = 0, xx = px; ix < 64; ix++, xx++, ps += 2, pd += 2)
				{
					if(ps[1]
						&& tinfo.rcclip.x1 <= xx && xx < tinfo.rcclip.x2
						&& tinfo.rcclip.y1 <= yy && yy < tinfo.rcclip.y2
						&& (!imgsel || TileImage_isPixelOpaque(imgsel, xx, yy)))
					{
						pix.r = pix.g = pix.b = ps[0];
						pix.a = ps[1];

						(func)(info, xx, yy, &pix);

						pd[0] = (pix.r + pix.g + pix.b) / 3;
					}
				}
			}
		}

		ppsrc += tinfo.pitch;
		ppdst += tinfo.pitch;
	}

	return TRUE;
}

/** 実際の処理
 *
 * グラデーションマップの場合はアルファ値が変化する場合あり。 */

static mBool _color_common_forReal(FilterDrawInfo *info,ColorCommonFunc func)
{
	TileImage *img;
	int ix,iy;
	mRect rc;
	RGBAFix15 pix;

	img = info->imgdst;
	rc = info->rc;

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		for(ix = rc.x1; ix <= rc.x2; ix++)
		{
			TileImage_getPixel(img, ix, iy, &pix);

			if(pix.a)
			{
				(func)(info, ix, iy, &pix);

				TileImage_setPixel_draw_direct(img, ix, iy, &pix);
			}
		}

		mPopupProgressThreadIncSubStep(info->prog);
	}

	return TRUE;
}

/** 色操作の共通フィルタ処理 */

static mBool _color_common(FilterDrawInfo *info,ColorCommonFunc func)
{
	if(!info->in_dialog)
		return _color_common_forReal(info, func);
	else
	{
		//プレビュー

		if(info->imgsrc->coltype == TILEIMAGE_COLTYPE_RGBA)
			return _color_common_forPreview_RGBA(info, func);
		else
			return _color_common_forPreview_GRAY(info, func);
	}
}

/** ピクセル処理 (共通) : ptmp のテーブルから取得 */

static void _colfunc_from_table(FilterDrawInfo *info,int x,int y,RGBAFix15 *pix)
{
	uint16_t *tbl = (uint16_t *)info->ptmp[0];

	pix->r = tbl[pix->r];
	pix->g = tbl[pix->g];
	pix->b = tbl[pix->b];
}


//========================
// カラー
//========================


/** 明度・コントラスト調整
 *
 * 明度 : -100 .. 100
 * コントラスト : -100 .. 100 */

static void _colfunc_brightcont(FilterDrawInfo *info,int x,int y,RGBAFix15 *pix)
{
	uint16_t *tbl = (uint16_t *)info->ptmp[0];
	int i,val[3];

	//コントラスト

	for(i = 0; i < 3; i++)
		val[i] = tbl[pix->c[i]];

	//明度

	if(info->val_bar[0])
	{
		FilterSub_RGBtoYCrCb(val);

		val[0] += info->ntmp[0];  //Y に加算

		FilterSub_YCrCbtoRGB(val);
	}

	for(i = 0; i < 3; i++)
		pix->c[i] = val[i];
}

mBool FilterDraw_color_brightcont(FilterDrawInfo *info)
{
	uint16_t *buf;
	int i,n,mul;

	//コントラスト テーブル作成

	buf = FilterSub_createColorTable(info);
	if(!buf) return FALSE;

	mul = (int)(info->val_bar[1] / 100.0 * 256 + 0.5);

	for(i = 0; i <= 0x8000; i++)
	{
		n = i + ((i - 0x4000) * mul >> 8);

		if(n < 0) n = 0;
		else if(n > 0x8000) n = 0x8000;

		buf[i] = n;
	}

	//明度

	info->ntmp[0] = (int)(info->val_bar[0] / 100.0 * 0x8000 + 0.5);
	
	//

	_color_common(info, _colfunc_brightcont);

	mFree(buf);

	return TRUE;
}

/** ガンマ補正 */

mBool FilterDraw_color_gamma(FilterDrawInfo *info)
{
	uint16_t *buf;
	int i,n;
	double d;

	//テーブル作成

	buf = FilterSub_createColorTable(info);
	if(!buf) return FALSE;

	d = 1.0 / (info->val_bar[0] * 0.01);

	for(i = 0; i <= 0x8000; i++)
	{
		n = round(pow((double)i / 0x8000, d) * 0x8000);

		if(n < 0) n = 0;
		else if(n > 0x8000) n = 0x8000;

		buf[i] = n;
	}
	
	//

	_color_common(info, _colfunc_from_table);

	mFree(buf);

	return TRUE;
}

/** レベル補正 */

mBool FilterDraw_color_level(FilterDrawInfo *info)
{
	uint16_t *buf;
	int i,n,in_min,in_mid,in_max,out_min,out_max,out_mid;
	double d1,d2,d;

	in_min = info->val_bar[0];
	in_mid = info->val_bar[1];
	in_max = info->val_bar[2];
	out_min = info->val_bar[3];
	out_max = info->val_bar[4];

	out_mid = (out_max - out_min) >> 1;

	d1 = (double)out_mid / (in_mid - in_min);
	d2 = (double)(out_max - out_min - out_mid) / (in_max - in_mid);

	//テーブル作成

	buf = FilterSub_createColorTable(info);
	if(!buf) return FALSE;

	for(i = 0; i <= 0x8000; i++)
	{
		//入力制限

		n = i;

		if(n < in_min) n = in_min;
		else if(n > in_max) n = in_max;

		//補正

		if(n < in_mid)
			d = (n - in_min) * d1;
		else
			d = (n - in_mid) * d2 + out_mid;
		
		buf[i] = (int)(d + out_min + 0.5);
	}
	
	//

	_color_common(info, _colfunc_from_table);

	mFree(buf);

	return TRUE;
}

/** RGB 補正
 *
 * R,G,B : -100 .. 100 */

static void _colfunc_rgb(FilterDrawInfo *info,int x,int y,RGBAFix15 *pix)
{
	int i,n;

	for(i = 0; i < 3; i++)
	{
		n = pix->c[i] + info->ntmp[i];

		if(n < 0) n = 0;
		else if(n > 0x8000) n = 0x8000;

		pix->c[i] = n;
	}
}

mBool FilterDraw_color_rgb(FilterDrawInfo *info)
{
	int i;

	for(i = 0; i < 3; i++)
		info->ntmp[i] = (int)(info->val_bar[i] / 100.0 * 0x8000 + 0.5);

	return _color_common(info, _colfunc_rgb);
}

/** HSV 調整 */

static void _colfunc_hsv(FilterDrawInfo *info,int x,int y,RGBAFix15 *pix)
{
	int val[3],i;

	for(i = 0; i < 3; i++)
		val[i] = pix->c[i];

	FilterSub_RGBtoHSV(val);

	for(i = 0; i < 3; i++)
	{
		val[i] += info->ntmp[i];

		if(i == 0)
		{
			if(val[0] < 0) val[0] += 360;
			else if(val[0] >= 360) val[0] -= 360;
		}
		else
		{
			if(val[i] < 0) val[i] = 0;
			else if(val[i] > 0x8000) val[i] = 0x8000;
		}
	}

	FilterSub_HSVtoRGB(val);

	for(i = 0; i < 3; i++)
		pix->c[i] = val[i];
}

mBool FilterDraw_color_hsv(FilterDrawInfo *info)
{
	info->ntmp[0] = info->val_bar[0];
	info->ntmp[1] = (int)(info->val_bar[1] / 100.0 * 0x8000 + 0.5);
	info->ntmp[2] = (int)(info->val_bar[2] / 100.0 * 0x8000 + 0.5);

	return _color_common(info, _colfunc_hsv);
}

/** ネガポジ反転 */

static void _colfunc_nega(FilterDrawInfo *info,int x,int y,RGBAFix15 *pix)
{
	int i;

	for(i = 0; i < 3; i++)
		pix->c[i] = 0x8000 - pix->c[i];
}

mBool FilterDraw_color_nega(FilterDrawInfo *info)
{
	return _color_common(info, _colfunc_nega);
}

/** グレイスケール */

static void _colfunc_grayscale(FilterDrawInfo *info,int x,int y,RGBAFix15 *pix)
{
	pix->r = pix->g = pix->b = RGB15_TO_GRAY(pix->r, pix->g, pix->b);
}

mBool FilterDraw_color_grayscale(FilterDrawInfo *info)
{
	return _color_common(info, _colfunc_grayscale);
}

/** セピアカラー */

static void _colfunc_sepia(FilterDrawInfo *info,int x,int y,RGBAFix15 *pix)
{
	int v;

	v = RGB15_TO_GRAY(pix->r, pix->g, pix->b);

	pix->r = (v + 1928 > 0x8000)? 0x8000: v + 1928;
	pix->g = v;
	pix->b = (v - 964 < 0)? 0: v - 964;
}

mBool FilterDraw_color_sepia(FilterDrawInfo *info)
{
	return _color_common(info, _colfunc_sepia);
}

/** グラデーションマップ */

static void _colfunc_gradmap(FilterDrawInfo *info,int x,int y,RGBAFix15 *pix)
{
	int v;

	v = RGB15_TO_GRAY(pix->r, pix->g, pix->b);

	TileImage_getGradationColor(pix, (double)v / 0x8000, (TileImageDrawGradInfo *)info->ptmp[0]);
}

mBool FilterDraw_color_gradmap(FilterDrawInfo *info)
{
	TileImageDrawGradInfo ginfo;

	drawOpSub_setDrawGradationInfo(&ginfo);

	info->ptmp[0] = &ginfo;

	_color_common(info, _colfunc_gradmap);

	mFree(ginfo.buf);

	return TRUE;
}

/** 2値化
 *
 * 0.1 .. 100.0 */

static void _colfunc_threshold(FilterDrawInfo *info,int x,int y,RGBAFix15 *pix)
{
	int v;

	v = RGB15_TO_GRAY(pix->r, pix->g, pix->b);

	v = (v < info->ntmp[0])? 0: 0x8000;

	pix->r = pix->g = pix->b = v;
}

mBool FilterDraw_color_threshold(FilterDrawInfo *info)
{
	info->ntmp[0] = (int)(info->val_bar[0] / 1000.0 * 0x8000 + 0.5);

	return _color_common(info, _colfunc_threshold);
}

/** 2値化 (ディザ)
 *
 * 0:bayer2x2, 1:bayer4x4, 2:渦巻き4x4, 3:網点4x4, 4:ランダム */

static void _colfunc_threshold_dither(FilterDrawInfo *info,int x,int y,RGBAFix15 *pix)
{
	uint8_t val_bayer2x2[2][2] = {{0,2},{3,1}},
	val_4x4[3][4][4] = {
		{{0 ,8,2,10}, {12,4,14, 6}, {3,11, 1,9}, {15, 7,13, 5}},
		{{13,7,6,12}, { 8,1, 0, 5}, {9, 2, 3,4}, {14,10,11,15}},
		{{10,4,6, 8}, {12,0, 2,14}, {7, 9,11,5}, { 3,15,13, 1}}
	};
	int v,cmp;

	//比較値

	switch(info->val_combo[0])
	{
		case 0:
			cmp = val_bayer2x2[y & 1][x & 1] << (15 - 2);
			break;
		case 1:
		case 2:
		case 3:
			cmp = val_4x4[info->val_combo[0] - 1][y & 3][x & 3] << (15 - 4);
			break;
		case 4:
			cmp = mRandXorShift_getIntRange(0, 0x8000 - 1);
			break;
	}

	//

	v = RGB15_TO_GRAY(pix->r, pix->g, pix->b);
	v = (v <= cmp)? 0: 0x8000;

	pix->r = pix->g = pix->b = v;
}

mBool FilterDraw_color_threshold_dither(FilterDrawInfo *info)
{
	return _color_common(info, _colfunc_threshold_dither);
}

/** ポスタリゼーション */

mBool FilterDraw_color_posterize(FilterDrawInfo *info)
{
	uint16_t *buf;
	int i,level,n;

	//テーブル作成

	buf = FilterSub_createColorTable(info);
	if(!buf) return FALSE;

	level = info->val_bar[0];

	for(i = 0; i < 0x8000; i++)
	{
		n = i * level >> 15;

		buf[i] = (n == level - 1)? 0x8000: (n << 15) / (level - 1);
	}

	buf[0x8000] = 0x8000;
	
	//

	_color_common(info, _colfunc_from_table);

	mFree(buf);

	return TRUE;
}


//=============================
// 色置換
//=============================


/** 描画色置換 */

static void _colfunc_replace_drawcol(FilterDrawInfo *info,int x,int y,RGBAFix15 *pix)
{
	if(pix->r == info->rgba15Draw.r
		&& pix->g == info->rgba15Draw.g
		&& pix->b == info->rgba15Draw.b)
	{
		pix->r = info->val_bar[0];
		pix->g = info->val_bar[1];
		pix->b = info->val_bar[2];
	}
}

mBool FilterDraw_color_replace_drawcol(FilterDrawInfo *info)
{
	return _color_common(info, _colfunc_replace_drawcol);
}

/** 色置換 (共通)
 *
 * @return TRUE で色を変更 */

static mBool _commfunc_replace(FilterDrawInfo *info,int x,int y,RGBAFix15 *pix)
{
	mBool drawcol = FALSE;

	//描画色かどうか

	if(pix->a && info->ntmp[0] < 3)
	{
		drawcol = (pix->r == info->rgba15Draw.r
			&& pix->g == info->rgba15Draw.g
			&& pix->b == info->rgba15Draw.b);
	}

	//

	switch(info->ntmp[0])
	{
		//描画色を透明に
		case 0:
			if(drawcol)
			{
				pix->v64 = 0;
				return TRUE;
			}
			break;
		//描画色以外を透明に
		case 1:
			if(pix->a && !drawcol)
			{
				pix->v64 = 0;
				return TRUE;
			}
			break;
		//描画色を背景色に
		case 2:
			if(drawcol)
			{
				pix->r = info->rgba15Bkgnd.r;
				pix->g = info->rgba15Bkgnd.g;
				pix->b = info->rgba15Bkgnd.b;
				return TRUE;
			}
			break;
		//透明色を描画色に
		case 3:
			if(pix->a == 0)
			{
				*pix = info->rgba15Draw;
				return TRUE;
			}
			break;
	}

	return FALSE;
}

mBool FilterDraw_color_replace(FilterDrawInfo *info)
{
	return FilterSub_drawCommon(info, _commfunc_replace);
}


//=============================
// アルファ操作
//=============================


/** アルファ操作 (チェックレイヤ) */

static mBool _commfunc_alpha_checked(FilterDrawInfo *info,int x,int y,RGBAFix15 *pixdst)
{
	LayerItem *pi = (LayerItem *)info->ptmp[0];
	RGBAFix15 pix;
	int type,a;

	type = info->ntmp[0];
	a = pixdst->a;

	switch(type)
	{
		//透明な部分を透明に
		//不透明な部分を透明に
		case 0:
		case 1:
			//すべてのレイヤが透明なら pi == NULL
			
			for(; pi; pi = pi->link)
			{
				TileImage_getPixel(pi->img, x, y, &pix);

				if(pix.a) break;
			}

			if((type == 0 && !pi) || (type == 1 && pi))
				a = 0;
			break;
		//値を合成してコピー (対象上で透明な部分は除く)
		case 2:
			if(pixdst->a)
			{
				//各レイヤのアルファ値を合成
				
				for(a = 0; pi; pi = pi->link)
				{
					TileImage_getPixel(pi->img, x, y, &pix);

					a = (((a + pix.a) << 4) - (a * pix.a >> 11) + 8) >> 4;
				}
			}
			break;
		//値を足す
		case 3:
			if(pixdst->a != 0x8000)
			{
				for(; pi; pi = pi->link)
				{
					TileImage_getPixel(pi->img, x, y, &pix);

					a += pix.a;

					if(a >= 0x8000)
					{
						a = 0x8000;
						break;
					}
				}
			}
			break;
		//値を引く
		case 4:
			if(pixdst->a != 0)
			{
				for(; pi; pi = pi->link)
				{
					TileImage_getPixel(pi->img, x, y, &pix);

					a -= pix.a;

					if(a <= 0)
					{
						a = 0;
						break;
					}
				}
			}
			break;
		//値を乗算
		case 5:
			if(pixdst->a != 0)
			{
				for(; pi && a; pi = pi->link)
				{
					TileImage_getPixel(pi->img, x, y, &pix);

					a = (a * pix.a + 0x4000) >> 15;
				}
			}
			break;
		//(単体) 明るい色ほど透明に
		//(単体) 暗い色ほど透明に
		case 6:
		case 7:
			TileImage_getPixel(pi->img, x, y, &pix);

			if(pix.a)
			{
				a = RGB15_TO_GRAY(pix.r, pix.g, pix.b);
				if(type == 6) a = 0x8000 - a;
			}
			break;
	}

	//アルファ値セット

	if(a == pixdst->a)
		return FALSE;
	else
	{
		if(a == 0)
			pixdst->v64 = 0;
		else
			pixdst->a = a;

		return TRUE;
	}
}

mBool FilterDraw_alpha_checked(FilterDrawInfo *info)
{
	//チェックレイヤのリンクをセット

	info->ptmp[0] = LayerList_setLink_checked(APP_DRAW->layerlist);

	return FilterSub_drawCommon(info, _commfunc_alpha_checked);
}

/** アルファ操作 (カレント) */

static mBool _commfunc_alpha_current(FilterDrawInfo *info,int x,int y,RGBAFix15 *pix)
{
	int a;

	a = pix->a;

	switch(info->ntmp[0])
	{
		//明るい色ほど透明に
		case 0:
			if(a)
				a = 0x8000 - RGB15_TO_GRAY(pix->r, pix->g, pix->b);
			break;
		//暗い色ほど透明に
		case 1:
			if(a)
				a = RGB15_TO_GRAY(pix->r, pix->g, pix->b);
			break;
		//テクスチャ適用
		case 2:
			if(a)
				a = (a * ImageBuf8_getPixel_forTexture(APP_DRAW->img8OptTex, x, y) + 127) / 255;
			break;
		//アルファ値からグレイスケール作成 ([!] RGB も変更)
		case 3:
			pix->r = pix->g = pix->b = a;
			pix->a = 0x8000;
			return TRUE;
		//透明以外を完全不透明に
		case 4:
			if(a) a = 0x8000;
			break;
	}

	//アルファ値セット

	if(a == pix->a)
		return FALSE;
	else
	{
		if(a == 0)
			pix->v64 = 0;
		else
			pix->a = a;

		return TRUE;
	}
}

mBool FilterDraw_alpha_current(FilterDrawInfo *info)
{
	return FilterSub_drawCommon(info, _commfunc_alpha_current);
}
