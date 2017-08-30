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

/******************************
 * ブラシ描画用パラメータ
 ******************************/

#ifndef BRUSHDRAWPARAM_H
#define BRUSHDRAWPARAM_H

typedef struct _ImageBuf8 ImageBuf8;

typedef struct _BrushDrawParam
{
	double shape_hard,		//円形形状の硬さパラメータ
		radius,				//半径 (px)
		opacity,			//最大濃度 [0-1]
		min_size,			//サイズ最小 [0-1]
		min_opacity,		//濃度最小 [0-1]
		interval_src,		//通常ブラシの間隔
		interval,			//実際の間隔
		random_size_min,	//ランダムサイズ最小
		random_pos_len,		//ランダム位置、距離
		water_param[3],		//水彩
		pressure_val[8];	//筆圧補正 作業値
	int angle,				//角度 [0-511]
		angle_random,		//角度ランダム幅
		rough,				//荒さ [0 で無効]
		pressure_type;		//筆圧補正タイプ
	uint32_t flags;

	ImageBuf8 *img_shape,
		*img_texture;
}BrushDrawParam;


enum 
{
	BRUSHDP_F_WATER = 1<<0,				//水彩
	BRUSHDP_F_NO_ANTIALIAS = 1<<1,		//非アンチエイリアス
	BRUSHDP_F_CURVE        = 1<<2,		//曲線補間
	BRUSHDP_F_RANDOM_SIZE  = 1<<3,		//ランダムサイズあり
	BRUSHDP_F_RANDOM_POS   = 1<<4,		//ランダム位置あり
	BRUSHDP_F_TRAVELLING_DIR = 1<<5,	//進行方向
	BRUSHDP_F_OVERWRITE_TP   = 1<<6,	//ブラシ形状の透明部分も上書き
	BRUSHDP_F_PRESSURE       = 1<<7,	//筆圧補正を行うか
	BRUSHDP_F_SHAPE_HARD_MAX = 1<<8,	//円形形状の硬さは最大
};

#endif
