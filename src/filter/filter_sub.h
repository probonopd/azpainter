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
 * フィルタサブ関数
 ***************************************/

#ifndef FILTER_SUB_H
#define FILTER_SUB_H

#define RGB15_TO_GRAY(r,g,b)  (((r) * 4899 + (g) * 9617 + (b) * 1868) >> 14)

/* 3x3フィルタの情報 */

typedef struct
{
	double mul[9],divmul,add;
	mBool grayscale;
}Filter3x3Info;

/* 点描画の情報 */

typedef struct
{
	TileImage *imgdst,*imgsrc;
	int coltype,radius,opacity,masktype;
	FilterDrawInfo *info;
}FilterDrawPointInfo;


typedef mBool (*FilterDrawCommonFunc)(FilterDrawInfo *,int,int,RGBAFix15 *);
typedef void (*FilterDrawPointFunc)(int,int,FilterDrawPointInfo *);
typedef void (*FilterDrawGaussBlurFunc)(int,int,RGBAFix15 *,FilterDrawInfo *);
typedef void (*FilterDrawGaussBlurSrcFunc)(RGBAFix15 *,int,FilterDrawInfo *);

/*---------*/

/* draw */

mBool FilterSub_drawCommon(FilterDrawInfo *info,FilterDrawCommonFunc func);
mBool FilterSub_draw3x3(FilterDrawInfo *info,Filter3x3Info *dat);

mBool FilterSub_gaussblur(FilterDrawInfo *info,
	TileImage *imgsrc,int radius,FilterDrawGaussBlurFunc setpix,FilterDrawGaussBlurSrcFunc setsrc);

/* sub */

uint16_t *FilterSub_createColorTable(FilterDrawInfo *info);
mBool FilterSub_getPixelsToBuf(TileImage *img,mRect *rcarea,RGBAFix15 *buf,mBool clipping);
uint8_t *FilterSub_createCircleShape(int radius,int *pixnum);
void FilterSub_copyImage_forPreview(FilterDrawInfo *info);

/* get */

void FilterSub_getImageSize(int *w,int *h);
void FilterSub_getPixelFunc(TileImageSetPixelFunc *func);
void FilterSub_getPixelSrc_clip(FilterDrawInfo *info,int x,int y,RGBAFix15 *pix);
void FilterSub_getDrawColor_fromType(FilterDrawInfo *info,int type,RGBAFix15 *pix);
TileImage *FilterSub_getCheckedLayerImage();

/* 線形補間色取得 */

void FilterSub_getLinerColor(TileImage *img,double x,double y,RGBAFix15 *pix,uint8_t flags);
void FilterSub_getLinerColor_bkgnd(TileImage *img,double dx,double dy,
	int x,int y,RGBAFix15 *pix,int type);

/* adv color */

void FilterSub_getAdvColor(double *d,double weight_mul,RGBAFix15 *pix);

void FilterSub_addAdvColor(double *d,RGBAFix15 *pix);
void FilterSub_addAdvColor_weight(double *d,double weight,RGBAFix15 *pix);
void FilterSub_addAdvColor_at(TileImage *img,int x,int y,double *d,mBool clipping);
void FilterSub_addAdvColor_at_weight(TileImage *img,int x,int y,double *d,double weight,mBool clipping);

/* 点描画 */

void FilterSub_getDrawPointFunc(int type,FilterDrawPointFunc *dst);
void FilterSub_initDrawPoint(FilterDrawInfo *info,mBool compare_alpha);

/* progress */

void FilterSub_progSetMax(FilterDrawInfo *info,int max);
void FilterSub_progInc(FilterDrawInfo *info);
void FilterSub_progIncSubStep(FilterDrawInfo *info);
void FilterSub_progBeginOneStep(FilterDrawInfo *info,int step,int max);
void FilterSub_progBeginSubStep(FilterDrawInfo *info,int step,int max);

/* color */

void FilterSub_RGBtoYCrCb(int *val);
void FilterSub_YCrCbtoRGB(int *val);
void FilterSub_RGBtoHSV(int *val);
void FilterSub_HSVtoRGB(int *val);

#endif
