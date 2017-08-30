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

/********************************
 * ブラシアイテムデータ
 ********************************/

#ifndef BRUSHITEM_H
#define BRUSHITEM_H

#include "mListDef.h"

typedef struct _BrushGroupItem BrushGroupItem;

#define BRUSHITEM(p)  ((BrushItem *)(p))

typedef struct _BrushItem
{
	mListItem i;
	
	BrushGroupItem *group;	//所属グループ

	char *name,			//名前
		*shape_path,	//ブラシ形状ファイルパス (空で円形)
		*texture_path;	//テクスチャファイルパス (空でなし[強制]、"?" でオプション指定を使う)

	uint32_t pressure_val;	//筆圧補正値(共通)
					/* 曲線 : 2byte ガンマ値 (1.00=100)
					 * 線形1点 : 下位8byte から [IN,OUT]
					 * 線形2点 : 下位8byte から 1[IN,OUT] 2[IN,OUT] */
	uint16_t radius,	//半径サイズ [1.0=10]
		radius_min,		//半径最小
		radius_max,		//半径最大
		min_size,		//最小サイズ(%) [1.0=10]
		min_opacity,	//最小濃度(%) [1.0=10]
		interval,		//点の間隔 [1.00=100]
		rand_size_min,	//ランダム、サイズ最小 [1.0=10]
		rand_pos_range,	//ランダム、位置の最大距離 [1.00=100]
		rotate_angle,	//回転角度 (0-359)
		water_param[3];	//水彩パラメータ (0:色の補充 1:キャンバス色の割合 2:描画色の割合)
	uint8_t type,		//ブラシタイプ
		opacity,		//濃度 (0-100)
		pixelmode,		//塗りタイプ
		hosei_type,		//補正タイプ
		hosei_strong,	//補正強さ (1-)
		hardness,		//形状硬さ (0-100)
		roughness,		//荒さ (0-100)
		rotate_rand_w,	//回転ランダム幅 (0-180)
		pressure_type,	//筆圧補正タイプ (0:曲線 1:線形1点 2:線形2点)
		flags;			//フラグ
}BrushItem;

enum
{
	BRUSHITEM_TYPE_NORMAL,
	BRUSHITEM_TYPE_ERASE,
	BRUSHITEM_TYPE_WATER,
	BRUSHITEM_TYPE_BLUR,
	BRUSHITEM_TYPE_DOTPEN,
	BRUSHITEM_TYPE_FINGER,
	
	BRUSHITEM_TYPE_NUM
};

enum
{
	BRUSHITEM_F_AUTO_SAVE = 1<<0,
	BRUSHITEM_F_ROTATE_TRAVELLING_DIR = 1<<1,
	BRUSHITEM_F_ANTIALIAS = 1<<2,
	BRUSHITEM_F_CURVE     = 1<<3,
};

enum
{
	BRUSHITEM_16BITVAL_NUM = 12,
	BRUSHITEM_8BITVAL_NUM  = 10,

	BRUSHITEM_RADIUS_MIN = 3,
	BRUSHITEM_RADIUS_MAX = 6000,
	BRUSHITEM_FINGER_RADIUS_MIN = 1,
	BRUSHITEM_FINGER_RADIUS_MAX = 50,
	BRUSHITEM_INTERVAL_MIN = 5,
	BRUSHITEM_INTERVAL_MAX = 1000,
	BRUSHITEM_RANDOM_POS_MAX = 5000,
	BRUSHITEM_PRESSURE_CURVE_MIN = 1,
	BRUSHITEM_PRESSURE_CURVE_MAX = 600,
	BRUSHITEM_HOSEI_STRONG_MAX = 15
};


void BrushItem_destroy_handle(mListItem *item);

void BrushItem_setDefault(BrushItem *p);
int BrushItem_adjustRadius(BrushItem *p,int n);
void BrushItem_adjustRadiusMinMax(BrushItem *p);
int BrushItem_getPixelModeNum(BrushItem *p);
int BrushItem_getPosInGroup(BrushItem *p);
void BrushItem_copy(BrushItem *dst,BrushItem *src);
void BrushItem_getTextFormat(BrushItem *p,mStr *str);
void BrushItem_setFromText(BrushItem *p,const char *text);

#endif
