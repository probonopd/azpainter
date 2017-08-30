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

/*****************************************************
 * TileImage へ描画する際の情報
 *
 * (g_tileimage_dinfo としてグローバル定義されている)
 *****************************************************/

#ifndef TILEIMAGE_DRAWINFO_H
#define TILEIMAGE_DRAWINFO_H

typedef struct _BrushDrawParam BrushDrawParam;
typedef struct _ImageBuf8 ImageBuf8;


typedef struct _TileImageDrawInfo
{
	int imgw,imgh,		//イメージの幅、高さ
		err;			//エラー

	RGBFix15 *colmask_col;	//色マスクの色 (r=0xffff で終端)

	uint8_t colmask_type,	//色マスクタイプ
		alphamask_type;		//アルファマスクタイプ

	mRect rcdraw;			//描画された範囲

	TileImageInfo tileimginfo;	//アンドゥ用、描画開始時のタイル情報

	TileImage *img_save,	//描画前の保存用
		*img_brush_stroke,	//ブラシストローク濃度用
		*img_mask,			//マスクイメージ (NULL でなし)
		*img_sel;			//選択範囲
	ImageBuf8 *texture;		//テクスチャイメージ (NULL でなし)

	BrushDrawParam *brushparam;	//ブラシ描画パラメータ
	RGBAFix15 rgba_brush;		//ブラシ描画時の色

	void (*funcDrawPixel)(TileImage *,int,int,RGBAFix15 *);			//ピクセル描画関数
	void (*funcColor)(TileImage *,RGBAFix15 *,RGBAFix15 *,void *);	//色計算関数
}TileImageDrawInfo;


extern TileImageDrawInfo g_tileimage_dinfo;

void TileImageDrawInfo_clearDrawRect();

#endif
