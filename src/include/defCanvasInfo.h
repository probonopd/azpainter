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

/***************************************
 * キャンバスのパラメータ情報
 ***************************************/

#ifndef DEF_CANVASINFO_H
#define DEF_CANVASINFO_H

/** 表示倍率/回転の計算時のパラメータ */

typedef struct _CanvasViewParam
{
	double scale,scalediv,	//倍率
		rd,					//角度
		cos,sin,			//cos,sin 値
		cosrev,sinrev;		//cos,sin 逆回転値
}CanvasViewParam;

/** キャンバス描画用のパラメータ */

typedef struct _CanvasDrawInfo
{
	mBox boxdst;			//描画先範囲
	double originx,originy;	//イメージの原点位置
	int scrollx,scrolly,	//スクロール位置
		mirror,				//左右反転表示か
		imgw,imgh;
	uint32_t bkgndcol;		//イメージ範囲外の色
	CanvasViewParam *param;	//計算用パラメータ
}CanvasDrawInfo;

/* draw_calc.c */

void CanvasDrawInfo_imageToarea(CanvasDrawInfo *p,double x,double y,double *px,double *py);
void CanvasDrawInfo_imageToarea_pt(CanvasDrawInfo *p,double x,double y,mPoint *pt);
void CanvasDrawInfo_getImageIncParam(CanvasDrawInfo *p,double *dst);

#endif
