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

/**********************************
 * DrawData 計算関数
 **********************************/

#ifndef DRAW_CALC_H
#define DRAW_CALC_H

typedef struct _DrawData DrawData;
typedef struct _TileImage TileImage;


void drawCalc_setCanvasViewParam(DrawData *p);
mBool drawCalc_isPointInCanvasArea(DrawData *p,mPoint *pt);

/* 座標変換 */

void drawCalc_areaToimage_double(DrawData *p,double *dx,double *dy,double x,double y);
void drawCalc_areaToimage_double_pt(DrawData *p,mDoublePoint *dst,mPoint *src);
void drawCalc_areaToimage_pt(DrawData *p,mPoint *pt,double x,double y);
void drawCalc_areaToimage_relative(DrawData *p,mPoint *dst,mPoint *src);

void drawCalc_imageToarea_pt(DrawData *p,mPoint *dst,int x,int y);
void drawCalc_imageToarea_pt_double(DrawData *p,mPoint *dst,double x,double y);

void drawCalc_getImagePos_atCanvasCenter(DrawData *p,double *px,double *py);
mBool drawCalc_imageToarea_box(DrawData *p,mBox *dst,mBox *src);
mBool drawCalc_areaToimage_box(DrawData *p,mBox *dst,mBox *src);

/* 範囲 */

mBool drawCalc_clipArea_toBox(DrawData *p,mBox *dst,mRect *src);
mBool drawCalc_clipImageRect(DrawData *p,mRect *rc);
mBool drawCalc_getImageBox_rect(DrawData *p,mBox *dst,mRect *rc);
mBool drawCalc_unionImageBox(mBox *dst,mBox *src);
void drawCalc_unionRect_relmove(mRect *dst,mRect *src,int mx,int my);

/* 描画操作 */

void drawCalc_fitLine45(mPoint *pt,mPoint *ptstart);
double drawCalc_getLineRadian_forImage(DrawData *p);
mBool drawCalc_moveImage_onMotion(DrawData *p,TileImage *img,uint32_t state,mPoint *ptret);

/* ほか */

int drawCalc_getZoom_step(DrawData *p,mBool zoomup);
int drawCalc_getZoom_fitWindow(DrawData *p);
void drawCalc_getCanvasScrollMax(DrawData *p,mSize *size);

#endif
