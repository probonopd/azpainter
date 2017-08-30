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
 * フィルタ描画情報
 ************************************/

#ifndef FILTERDRAWINFO_H
#define FILTERDRAWINFO_H

#include "ColorValue.h"

typedef struct _FilterDrawInfo FilterDrawInfo;
typedef struct _mPopupProgress mPopupProgress;
typedef struct _TileImage TileImage;

typedef mBool (*FilterDrawFunc)(FilterDrawInfo *);


#define FILTER_BAR_NUM       8	//最大8=漫画用集中線など
#define FILTER_CHECKBTT_NUM  3	//最大3=ハーフトーン
#define FILTER_COMBOBOX_NUM  3	//最大3=縁に沿って点描画


struct _FilterDrawInfo
{
	TileImage *imgsrc,
		*imgdst,
		*imgref,	//参照イメージ先頭
		*imgsel;	//選択範囲 (カラー処理のプレビュー用)
	mPopupProgress *prog;

	mRect rc;	//イメージを処理する範囲 (イメージ座標)
	mBox box;

	FilterDrawFunc drawfunc;

	RGBAFix15 rgba15Draw,	//描画色
		rgba15Bkgnd;		//背景色

	int imgx,imgy,		//イメージ位置 (キャンバス上クリック位置)
		val_bar[FILTER_BAR_NUM];
	uint8_t	val_ckbtt[FILTER_CHECKBTT_NUM],
		val_combo[FILTER_COMBOBOX_NUM];

	mBool in_dialog,	//ダイアログ中は TRUE、実際の描画時は FALSE
		clipping;		//イメージ範囲外をクリッピングするか

	/* フィルタ処理中の作業用 */

	int ntmp[4];	//最大4=RGBずらし
	void *ptmp[1];	//テーブルバッファ用
};

#endif
