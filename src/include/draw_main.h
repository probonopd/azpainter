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

/***************************
 * DrawData メイン関数
 ***************************/

#ifndef DRAW_MAIN_H
#define DRAW_MAIN_H

typedef struct _DrawData  DrawData;
typedef struct _LayerItem LayerItem;
typedef struct _ImageBuf8 ImageBuf8;
typedef union _RGBFix15  RGBFix15;
typedef union _RGBAFix15 RGBAFix15;

typedef struct _mPopupProgress mPopupProgress;


#define DRAW_SETZOOMANDANGLE_F_NO_UPDATE  4


/* init */

mBool DrawData_new();
void DrawData_free();

void drawInit_beforeCreateWidget();
void drawInit_beforeShow();

void drawConfig_changeCanvasColor(DrawData *p);

/* image */

void drawImage_afterChange(DrawData *p,mBool change_file);
mBool drawImage_changeImageSize(DrawData *p,int w,int h);

mBool drawImage_new(DrawData *p,int w,int h,int dpi,int coltype);
mBool drawImage_onLoadError(DrawData *p);
int drawImage_loadFile(DrawData *p,const char *filename,
	uint32_t format,mBool ignore_alpha,mPopupProgress *prog,char **errmes);

mBool drawImage_resizeCanvas(DrawData *p,int w,int h,int movx,int movy,mBool crop);
mBool drawImage_scaleCanvas(DrawData *p,int w,int h,int dpi,int type);

void drawImage_blendImage_real(DrawData *p);
mBool drawImage_blendImage_RGB8(DrawData *p);
void drawImage_blendImage_RGBA8(DrawData *p);
void drawImage_getBlendColor_atPoint(DrawData *p,int x,int y,RGBAFix15 *dst);

/* etc */

mBool drawTexture_loadOptionTextureImage(DrawData *p);
int drawCursor_getToolCursor(int toolno);

/* color */

void drawColor_getDrawColor_rgb(int *dst);
void drawColor_getDrawColor_rgbafix(RGBAFix15 *dst);

void drawColor_setDrawColor(uint32_t col);
void drawColor_setBkgndColor(uint32_t col);
void drawColor_toggleDrawCol(DrawData *p);

void drawColorMask_changeColor(DrawData *p);
void drawColorMask_setType(DrawData *p,int type);
void drawColorMask_setColor(DrawData *p,int col);
mBool drawColorMask_addColor(DrawData *p,int col);
mBool drawColorMask_delColor(DrawData *p,int no);
void drawColorMask_clear(DrawData *p);

/* tool */

void drawTool_setTool(DrawData *p,int no);
void drawTool_setToolSubtype(DrawData *p,int subno);

/* stamp */

void drawStamp_clearImage(DrawData *p);
void drawStamp_loadImage(DrawData *p,const char *filename,mBool ignore_alpha);

/* canvas */

void drawCanvas_lowQuality();
void drawCanvas_normalQuality();

void drawCanvas_setScrollReset(DrawData *p,mDoublePoint *origin);
void drawCanvas_setScrollReset_update(DrawData *p,mDoublePoint *origin);
void drawCanvas_setScrollDefault(DrawData *p);
void drawCanvas_zoomStep(DrawData *p,mBool zoomup);
void drawCanvas_rotateStep(DrawData *p,mBool left);
void drawCanvas_fitWindow(DrawData *p);
void drawCanvas_mirror(DrawData *p);
void drawCanvas_setAreaSize(DrawData *p,int w,int h);

void drawCanvas_setZoomAndAngle(DrawData *p,int zoom,int angle,int flags,mBool reset_scroll);

/* update */

void drawUpdate_canvasArea();

void drawUpdate_all();
void drawUpdate_all_layer();

void drawUpdate_blendImage_full(DrawData *p);
void drawUpdate_blendImage_box(DrawData *p,mBox *box);
void drawUpdate_blendImage_layer(DrawData *p,mBox *box);

void drawUpdate_drawCanvas(DrawData *p,mPixbuf *pixbuf,mBox *box);

void drawUpdate_rect_canvas(DrawData *p,mBox *boximg);
void drawUpdate_rect_canvas_forSelect(DrawData *p,mRect *rc);
void drawUpdate_rect_canvas_forBoxEdit(DrawData *p,mBox *box);

void drawUpdate_rect_blendimg(DrawData *p,mBox *boximg);

void drawUpdate_rect_imgcanvas(DrawData *p,mBox *boximg);
void drawUpdate_rect_imgcanvas_fromRect(DrawData *p,mRect *rc);
void drawUpdate_rect_imgcanvas_forSelect(DrawData *p,mRect *rc);

void drawUpdate_rect_imgcanvas_canvasview_fromRect(DrawData *p,mRect *rc);
void drawUpdate_rect_imgcanvas_canvasview_inLayerHave(DrawData *p,LayerItem *item);

void drawUpdate_canvasview(DrawData *p,mRect *rc);
void drawUpdate_canvasview_inLoupe();

void drawUpdate_endDraw_box(DrawData *p,mBox *boximg);

#endif
