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
 * 漫画用
 **************************************/
/*
 * プレビュー中は、出力先イメージは A16 タイプ。
 */


#include <math.h>

#include "mDef.h"
#include "mRandXorShift.h"

#include "defTileImage.h"
#include "TileImage.h"
#include "TileImageDrawInfo.h"

#include "BrushDrawParam.h"
#include "defCanvasInfo.h"

#include "FilterDrawInfo.h"
#include "filter_sub.h"



//=======================
// sub
//=======================


/** 描画色取得 & プレビューのレイヤ色セット */

static void _set_draw_color(FilterDrawInfo *info,RGBAFix15 *dst)
{
	TileImage *imgdst = info->imgdst;

	if(info->in_dialog && info->val_ckbtt[1])
	{
		//赤色でプレビュー

		dst->v64 = 0;
		dst->a = 0x8000;

		imgdst->rgb.r = 0x8000;
		imgdst->rgb.g = imgdst->rgb.b = 0;
	}
	else
	{
		//実際の描画 or 通常プレビュー時
	
		*dst = info->rgba15Draw;

		if(info->in_dialog)
		{
			imgdst->rgb.r = info->rgba15Draw.r;
			imgdst->rgb.g = info->rgba15Draw.g;
			imgdst->rgb.b = info->rgba15Draw.b;
		}
	}

	//ブラシの描画色

	g_tileimage_dinfo.rgba_brush = *dst;
}

/** ブラシ、アンチエイリアス設定 */

static void _setbrush_antialias(mBool on)
{
	if(on)
		g_tileimage_dinfo.brushparam->flags &= ~BRUSHDP_F_NO_ANTIALIAS;
	else
		g_tileimage_dinfo.brushparam->flags |= BRUSHDP_F_NO_ANTIALIAS;
}


//=======================


/** 集中線/フラッシュ描画 共通 */
/*
 * <type>
 *  0: 集中線/フラッシュ (円の外周から中心へ向けて)
 *  1: ベタフラッシュ (円の外周から外へ向けて&中を塗りつぶし)
 *  2: ウニフラッシュ
 *
 * <valbar>
 *  [0] 円の半径(px)
 *  [1] 円の縦横比 (-1.000〜+1.000)
 *  [2] 線の密度最小 (360度を分割する数÷2)
 *  [3] 間隔ランダム幅 (0-99, 角度ステップに対する%)
 *  [4] 線の半径(px)
 *  [5] 線の半径ランダム (px, 0.0-100.0)
 *  [6] 線の長さ (0.1-100.0, 円の半径に対する%)
 *  [7] 線の長さランダム幅 (0.0-100.0, 半径に対する%)
 * 
 * <描画タイプ>
 *  0: ブラシ・アンチエイリアスあり
 *  1: ブラシ・アンチエイリアスなし
 *  2: 1pxドット線 */

static mBool _comic_common_flash(FilterDrawInfo *info,int type)
{
	int drawtype;
	uint32_t headtail;
	double r,brush_size,brush_sizernd,line_len,line_lenrnd,angle_step,angle_rnd,xscale,yscale;
	double angle,dtmp,dcos,dsin,draw_rmin,draw_rmax,xx,yy,x1,y1,x2,y2;
	mDoublePoint dpt_ct;
	RGBAFix15 pixdraw;
	mBool prev_simple;
	BrushDrawParam *brushparam = g_tileimage_dinfo.brushparam;
	TileImage *imgdst = info->imgdst;

	//簡易プレビュー

	prev_simple = (info->in_dialog && info->val_ckbtt[0]);

	//描画色

	_set_draw_color(info, &pixdraw);

	//------ 準備

	drawtype = info->val_combo[0];
	dpt_ct.x = info->imgx;
	dpt_ct.y = info->imgy;
	r = info->val_bar[0];
	brush_size = info->val_bar[4] * 0.1;
	brush_sizernd = info->val_bar[5] * 0.1;
	line_len = r * (info->val_bar[6] * 0.001);
	line_lenrnd = r * (info->val_bar[7] * 0.001);
	angle_step = M_MATH_PI * 2 / (info->val_bar[2] * 2);
	angle_rnd = angle_step * (info->val_bar[3] * 0.01);

	//円のX,Y倍率

	if(info->val_bar[1] < 0)
		xscale = 1, yscale = 1.0 + info->val_bar[1] * 0.001;
	else
		xscale = 1.0 - info->val_bar[1] * 0.001, yscale = 1;

	//入り抜き (ウニフラッシュの場合 50:50、他は 0:100)

	headtail = (type == 2)? (500 << 16) | 500: 1000;

	//描画情報

	_setbrush_antialias(drawtype != 1);

	brushparam->min_opacity = (drawtype != 0);

	g_tileimage_dinfo.funcColor =
		(drawtype == 0 && !prev_simple)? TileImage_colfunc_normal: TileImage_colfunc_overwrite;

	//-------- 描画

	FilterSub_progBeginOneStep(info, 50, info->val_bar[2] * 2);

	for(angle = 0; angle < M_MATH_PI * 2; angle += angle_step)
	{
		//角度

		dtmp = angle;
		
		if(info->val_bar[3])
			dtmp += mRandXorShift_getDouble() * angle_rnd;

		dcos = cos(dtmp);
		dsin = sin(dtmp);

		//描画する線の、円の内側の半径と外側の半径

		switch(type)
		{
			//集中線、フラッシュ
			case 0:
				draw_rmax = r;

				draw_rmin = r - line_len;
				if(info->val_bar[7])
					draw_rmin -= mRandXorShift_getDouble() * line_lenrnd;
				break;
			//ベタフラッシュ
			case 1:
				draw_rmax = r;

				draw_rmin = r + line_len;
				if(info->val_bar[7])
					draw_rmin += mRandXorShift_getDouble() * line_lenrnd;
				break;
			//ウニフラッシュ
			default:
				dtmp = line_len;
				if(info->val_bar[7])
					dtmp += mRandXorShift_getDouble() * line_lenrnd;

				dtmp *= 0.5;

				draw_rmax = r + dtmp;
				draw_rmin = r - dtmp;
				break;
		}

		//線の描画位置

		xx = dcos * xscale;
		yy = dsin * yscale;

		x1 = draw_rmax * xx + dpt_ct.x;
		y1 = draw_rmax * yy + dpt_ct.y;
		x2 = draw_rmin * xx + dpt_ct.x;
		y2 = draw_rmin * yy + dpt_ct.y;

		//ブラシ半径

		dtmp = brush_size;

		if(info->val_bar[5])
			dtmp += mRandXorShift_getDouble() * brush_sizernd;

		brushparam->radius = dtmp;

		//描画

		if(drawtype == 2)
			//1pxドット線
			TileImage_drawLineB(imgdst, floor(x1), floor(y1), floor(x2), floor(y2), &pixdraw, FALSE);
		else if(!prev_simple)
			//ブラシ:通常描画
			TileImage_drawBrushLine_headtail(imgdst, x1, y1, x2, y2, headtail);
		else
		{
			//ブラシ:簡易プレビュー

			if(dtmp <= 0.5)
				TileImage_drawLineB(imgdst, floor(x1), floor(y1), floor(x2), floor(y2), &pixdraw, FALSE);
			else if(type == 2)
			{
				//ウニフラッシュ

				xx = x1 + (x2 - x1) * 0.5;
				yy = y1 + (y2 - y1) * 0.5;

				dsin = dtmp * -dsin * xscale;
				dcos = dtmp * dcos * yscale;

				TileImage_drawLineB(imgdst, x1, y1, xx + dsin, yy + dcos, &pixdraw, FALSE);
				TileImage_drawLineB(imgdst, xx + dsin, yy + dcos, x2, y2, &pixdraw, FALSE);
				TileImage_drawLineB(imgdst, x2, y2, xx - dsin, yy - dcos, &pixdraw, FALSE);
				TileImage_drawLineB(imgdst, xx - dsin, yy - dcos, x1, y1, &pixdraw, FALSE);
			}
			else
			{
				//集中線、フラッシュ

				dsin = dtmp * -dsin * xscale;
				dcos = dtmp * dcos * yscale;

				TileImage_drawLineB(imgdst, x2, y2, x1 + dsin, y1 + dcos, &pixdraw, FALSE);
				TileImage_drawLineB(imgdst, x1 + dsin, y1 + dcos, x1 - dsin, y1 - dcos, &pixdraw, FALSE);
				TileImage_drawLineB(imgdst, x1 - dsin, y1 - dcos, x2, y2, &pixdraw, FALSE);
			}
		}

		FilterSub_progIncSubStep(info);
	}

	//ベタフラッシュ、内側塗りつぶし

	if(type == 1)
	{
		CanvasViewParam param;

		param.cos = param.cosrev = 1;
		param.sin = param.sinrev = 0;

		if(prev_simple)
		{
			TileImage_drawEllipse(imgdst, dpt_ct.x, dpt_ct.y, r * xscale, r * yscale,
				&pixdraw, TRUE, &param, FALSE);
		}
		else
		{
			TileImage_drawFillEllipse(imgdst, dpt_ct.x, dpt_ct.y, r * xscale, r * yscale,
				&pixdraw, FALSE, &param, FALSE);
		}
	}

	return TRUE;
}

/** 集中線、フラッシュ */

mBool FilterDraw_comic_concline_flash(FilterDrawInfo *info)
{
	return _comic_common_flash(info, 0);
}

/** ベタフラッシュ */

mBool FilterDraw_comic_popupflash(FilterDrawInfo *info)
{
	return _comic_common_flash(info, 1);
}

/** ウニフラッシュ */

mBool FilterDraw_comic_uniflash(FilterDrawInfo *info)
{
	return _comic_common_flash(info, 2);
}

/** ウニフラッシュ(波) */
/*
 * <valbar>
 * [0] 半径(px)
 * [1] 縦横比(-1.000〜+1.000)
 * [2] 波の密度 (360度の分割数)
 * [3] 波の長さ (半径に対する%)
 * [4] 線の太さ(px)
 * [5] 線の長さ(0.1-100.0, 半径に対する%)
 * [6] 抜きの太さ(%) */

mBool FilterDraw_comic_uniflash_wave(FilterDrawInfo *info)
{
	int drawtype,i;
	double r,line_len,angle_step,angle_step2,xscale,yscale;
	double angle,dtmp,dcos,dsin,tbl_radius[6],x1,y1,x2,y2;
	mDoublePoint dpt_ct;
	RGBAFix15 pixdraw;
	mBool prev_simple;
	BrushDrawParam *brushparam = g_tileimage_dinfo.brushparam;
	TileImage *imgdst = info->imgdst;

	//簡易プレビュー

	prev_simple = (info->in_dialog && info->val_ckbtt[0]);

	//描画色

	_set_draw_color(info, &pixdraw);

	//------ 準備

	drawtype = info->val_combo[0];
	dpt_ct.x = info->imgx;
	dpt_ct.y = info->imgy;
	r = info->val_bar[0];
	line_len = r * (info->val_bar[5] * 0.001);
	angle_step = M_MATH_PI * 2 / info->val_bar[2];
	angle_step2 = angle_step / 6;

	//円のX,Y倍率

	if(info->val_bar[1] < 0)
		xscale = 1, yscale = 1.0 + info->val_bar[1] * 0.001;
	else
		xscale = 1.0 - info->val_bar[1] * 0.001, yscale = 1;

	//半径のテーブル

	dtmp = r * (info->val_bar[3] * 0.001);

	tbl_radius[0] = 0;
	tbl_radius[1] = dtmp / 3;
	tbl_radius[2] = dtmp * 2 / 3;
	tbl_radius[3] = dtmp;
	tbl_radius[4] = tbl_radius[2];
	tbl_radius[5] = tbl_radius[1];

	//描画情報

	brushparam->radius = info->val_bar[4] * 0.1;
	brushparam->min_size = info->val_bar[6] * 0.01;
	brushparam->min_opacity = 1;

	_setbrush_antialias(drawtype != 1);

	g_tileimage_dinfo.funcColor =
		(drawtype == 0 && !prev_simple)? TileImage_colfunc_normal: TileImage_colfunc_overwrite;

	//-------- 描画

	FilterSub_progBeginOneStep(info, 50, info->val_bar[2]);

	for(angle = 0; angle < M_MATH_PI * 2; angle += angle_step)
	{
		//1つの波に対して6個描画

		for(i = 0; i < 6; i++)
		{
			dtmp = angle + i * angle_step2;

			dcos = cos(dtmp) * xscale;
			dsin = sin(dtmp) * yscale;

			//線の位置

			dtmp = r + tbl_radius[i];

			x1 = dtmp * dcos + dpt_ct.x;
			y1 = dtmp * dsin + dpt_ct.y;

			dtmp -= line_len;

			x2 = dtmp * dcos + dpt_ct.x;
			y2 = dtmp * dsin + dpt_ct.y;

			//描画

			if(drawtype == 2)
				//1pxドット線
				TileImage_drawLineB(imgdst, floor(x1), floor(y1), floor(x2), floor(y2), &pixdraw, FALSE);
			else if(!prev_simple)
				//ブラシ:通常描画
				TileImage_drawBrushLine_headtail(imgdst, x1, y1, x2, y2, 1000);
			else
			{
				//簡易プレビュー

				dsin = brushparam->radius * -dsin * xscale;
				dcos = brushparam->radius * dcos * yscale;

				TileImage_drawLineB(imgdst, x2, y2, x1 + dsin, y1 + dcos, &pixdraw, FALSE);
				TileImage_drawLineB(imgdst, x1 + dsin, y1 + dcos, x1 - dsin, y1 - dcos, &pixdraw, FALSE);
				TileImage_drawLineB(imgdst, x1 - dsin, y1 - dcos, x2, y2, &pixdraw, FALSE);
			}
		}

		FilterSub_progIncSubStep(info);
	}

	return TRUE;
}
