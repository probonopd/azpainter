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
 * TileImage
 ************************************/

#ifndef TILEIMAGE_H
#define TILEIMAGE_H

#include "ColorValue.h"

typedef struct _TileImage  TileImage;
typedef struct _ImageBufRGB16 ImageBufRGB16;
typedef struct _ImageBuf8 ImageBuf8;
typedef struct _mPopupProgress mPopupProgress;
typedef struct _CanvasDrawInfo CanvasDrawInfo;
typedef struct _CanvasViewParam CanvasViewParam;
typedef struct _FillPolygon FillPolygon;

typedef void (*TileImageSetPixelFunc)(TileImage *,int,int,RGBAFix15 *);

typedef struct _TileImageInfo
{
	int tilew,tileh,offx,offy;
}TileImageInfo;

typedef struct
{
	int width,height,dpi,transparent;
}TileImageLoadFileInfo;

typedef struct
{
	mRect rctile,
		rcclip;		//x2,y2 は+1
	mPoint pxtop;
	int pitch;
}TileImageTileRectInfo;

typedef struct _TileImageDrawGradInfo
{
	uint16_t *buf;
	int opacity,	//0-256
		flags;
}TileImageDrawGradInfo;


enum
{
	TILEIMAGE_COLTYPE_RGBA,
	TILEIMAGE_COLTYPE_GRAY,
	TILEIMAGE_COLTYPE_ALPHA16,
	TILEIMAGE_COLTYPE_ALPHA1,

	TILEIMAGE_COLTYPE_NUM
};

#define TILEIMAGE_DRAWGRAD_F_LOOP       1
#define TILEIMAGE_DRAWGRAD_F_SINGLE_COL 2
#define TILEIMAGE_DRAWGRAD_F_REVERSE    4


/*---- function ----*/

mBool TileImage_init();
void TileImage_finish();

/* free */

void TileImage_free(TileImage *p);
void TileImage_freeTile(uint8_t **pptile);
void TileImage_freeTile_atPos(TileImage *p,int tx,int ty);
void TileImage_freeTile_atPixel(TileImage *p,int x,int y);
void TileImage_freeTileBuf(TileImage *p);
void TileImage_freeAllTiles(TileImage *p);
mBool TileImage_freeEmptyTiles(TileImage *p);
void TileImage_freeEmptyTiles_byUndo(TileImage *p,TileImage *imgundo);

/* new */

TileImage *TileImage_new(int type,int w,int h);
TileImage *TileImage_newFromInfo(int type,TileImageInfo *info);
TileImage *TileImage_newFromRect(int type,mRect *rc);
TileImage *TileImage_newFromRect_forFile(int type,mRect *rc);
TileImage *TileImage_newClone(TileImage *src);

/* */

TileImage *TileImage_createSame(TileImage *p,TileImage *src,int type);
void TileImage_clear(TileImage *p);

/* tilebuf */

mBool TileImage_reallocTileBuf_fromImageSize(TileImage *p,int w,int h);
mBool TileImage_resizeTileBuf_includeImage(TileImage *p);
mBool TileImage_resizeTileBuf_combine(TileImage *p,TileImage *src);
mBool TileImage_resizeTileBuf_forUndo(TileImage *p,TileImageInfo *info);

/* tile */

uint8_t *TileImage_allocTile(TileImage *p,mBool clear);
mBool TileImage_allocTile_atptr(TileImage *p,uint8_t **ppbuf,mBool clear);
uint8_t *TileImage_getTileAlloc_atpos(TileImage *p,int tx,int ty,mBool clear);

/* set */

void TileImage_setOffset(TileImage *p,int offx,int offy);
void TileImage_moveOffset_rel(TileImage *p,int x,int y);
void TileImage_setImageColor(TileImage *p,uint32_t col);

/* calc */

mBool TileImage_pixel_to_tile(TileImage *p,int x,int y,int *tilex,int *tiley);
void TileImage_pixel_to_tile_nojudge(TileImage *p,int x,int y,int *tilex,int *tiley);
void TileImage_pixel_to_tile_rect(TileImage *p,mRect *rc);
void TileImage_tile_to_pixel(TileImage *p,int tx,int ty,int *px,int *py);
void TileImage_getAllTilesPixelRect(TileImage *p,mRect *rc);

/* get */

void TileImage_getOffset(TileImage *p,mPoint *pt);
void TileImage_getInfo(TileImage *p,TileImageInfo *info);
void TileImage_getCanDrawRect_pixel(TileImage *p,mRect *rc);
mBool TileImage_getTileRect_fromPixelBox(TileImage *p,mRect *rc,mBox *box);
uint8_t **TileImage_getTileRectInfo(TileImage *p,TileImageTileRectInfo *info,mBox *box);
mBool TileImage_getHaveImageRect_pixel(TileImage *p,mRect *rc,int *tilenum);
int TileImage_getHaveTileNum(TileImage *p);

mBool TileImage_clipCanDrawRect(TileImage *p,mRect *rc);

/* etc */

void TileImage_blendToImageRGB16(TileImage *p,ImageBufRGB16 *dst,mBox *boxdst,int opacity,int blendmode,ImageBuf8 *imgtex);
void TileImage_blendXorToPixbuf(TileImage *p,mPixbuf *pixbuf,mBox *boxdst);
void TileImage_drawPreview(TileImage *p,mPixbuf *pixbuf,int x,int y,int boxw,int boxh,int sw,int sh);
void TileImage_drawFilterPreview(TileImage *p,mPixbuf *pixbuf,mBox *box);
uint32_t *TileImage_getHistogram(TileImage *p);

/* pixel */

mBool TileImage_isPixelOpaque(TileImage *p,int x,int y);
mBool TileImage_isPixelTransparent(TileImage *p,int x,int y);

void TileImage_getPixel(TileImage *p,int x,int y,RGBAFix15 *pix);
void TileImage_getPixel_clip(TileImage *p,int x,int y,RGBAFix15 *pix);
void TileImage_getBlendPixel(TileImage *p,int x,int y,RGBFix15 *pix,int opacity,int blendmode,ImageBuf8 *imgtex);

void TileImage_setPixel_draw_direct(TileImage *p,int x,int y,RGBAFix15 *pix);
void TileImage_setPixel_draw_dot_stroke(TileImage *p,int x,int y,RGBAFix15 *pix);
void TileImage_setPixel_draw_brush_stroke(TileImage *p,int x,int y,RGBAFix15 *pix);
void TileImage_setPixel_draw_dotstyle_direct(TileImage *p,int x,int y,RGBAFix15 *pix);
void TileImage_setPixel_draw_addsub(TileImage *p,int x,int y,int v);

void TileImage_setPixel_new(TileImage *p,int x,int y,RGBAFix15 *pix);
void TileImage_setPixel_new_notp(TileImage *p,int x,int y,RGBAFix15 *pix);
void TileImage_setPixel_new_drawrect(TileImage *p,int x,int y,RGBAFix15 *pix);
void TileImage_setPixel_new_colfunc(TileImage *p,int x,int y,RGBAFix15 *pix);
void TileImage_setPixel_subdraw(TileImage *p,int x,int y,RGBAFix15 *pix);

void TileImage_colfunc_normal(TileImage *p,RGBAFix15 *dst,RGBAFix15 *src,void *param);
void TileImage_colfunc_compareA(TileImage *p,RGBAFix15 *dst,RGBAFix15 *src,void *param);
void TileImage_colfunc_overwrite(TileImage *p,RGBAFix15 *dst,RGBAFix15 *src,void *param);
void TileImage_colfunc_erase(TileImage *p,RGBAFix15 *dst,RGBAFix15 *src,void *param);
void TileImage_colfunc_dodge(TileImage *p,RGBAFix15 *dst,RGBAFix15 *src,void *param);
void TileImage_colfunc_burn(TileImage *p,RGBAFix15 *dst,RGBAFix15 *src,void *param);
void TileImage_colfunc_add(TileImage *p,RGBAFix15 *dst,RGBAFix15 *src,void *param);
void TileImage_colfunc_blur(TileImage *p,RGBAFix15 *dst,RGBAFix15 *src,void *param);
void TileImage_colfunc_finger(TileImage *p,RGBAFix15 *dst,RGBAFix15 *src,void *param);
void TileImage_colfunc_inverse_alpha(TileImage *p,RGBAFix15 *dst,RGBAFix15 *src,void *param);

/* draw */

void TileImage_drawLineB(TileImage *p,
	int x1,int y1,int x2,int y2,RGBAFix15 *pix,mBool nostart);
void TileImage_drawLine_forFilter(TileImage *p,
	int x1,int y1,int x2,int y2,RGBAFix15 *pix,mRect *rcclip);

void TileImage_drawLine_clip(TileImage *p,int x1,int y1,int x2,int y2,RGBAFix15 *pix,mRect *rcclip);
void TileImage_drawLinePoint4(TileImage *p,mPoint *pt,mRect *rcclip);

void TileImage_drawEllipse(TileImage *p,double cx,double cy,double xr,double yr,
    RGBAFix15 *pix,mBool dotpen,CanvasViewParam *param,mBool mirror);
void TileImage_drawBox(TileImage *p,int x,int y,int w,int h,RGBAFix15 *pix);
void TileImage_drawBezier_forXor(TileImage *p,mPoint *pt,mBool drawline2);
void TileImage_drawBezier(TileImage *p,mDoublePoint *pt,RGBAFix15 *pix,
	mBool dotpen,uint32_t headtail);
double TileImage_drawSpline(TileImage *p,mDoublePoint *pt,uint8_t *pressure,
	RGBAFix15 *pixdraw,double tbrush,mPoint *ptlast_dot,mBool first);

void TileImage_drawFillBox(TileImage *p,int x,int y,int w,int h,RGBAFix15 *pix);
void TileImage_drawFillEllipse(TileImage *p,
	double cx,double cy,double xr,double yr,RGBAFix15 *pix,mBool antialias,
	CanvasViewParam *param,mBool mirror);
mBool TileImage_drawFillPolygon(TileImage *p,FillPolygon *fillpolygon,
	RGBAFix15 *pix,mBool antialias);

void TileImage_getGradationColor(RGBAFix15 *pixdst,double pos,TileImageDrawGradInfo *info);

void TileImage_drawGradation_line(TileImage *p,
	int x1,int y1,int x2,int y2,mRect *rc,TileImageDrawGradInfo *info);
void TileImage_drawGradation_circle(TileImage *p,
	int x1,int y1,int x2,int y2,mRect *rc,TileImageDrawGradInfo *info);
void TileImage_drawGradation_box(TileImage *p,
	int x1,int y1,int x2,int y2,mRect *rc,TileImageDrawGradInfo *info);
void TileImage_drawGradation_radial(TileImage *p,
	int x1,int y1,int x2,int y2,mRect *rc,TileImageDrawGradInfo *info);

/* draw brush */

void TileImage_beginDrawBrush_free(TileImage *p,double x,double y,double pressure);
void TileImage_beginDrawBrush_notfree(TileImage *p,double x,double y);

void TileImage_drawBrushFree(TileImage *p,double x,double y,double pressure);
double TileImage_drawBrushLine(TileImage *p,
	double x1,double y1,double x2,double y2,
	double press_st,double press_ed,double t_start);
void TileImage_drawBrushLine_headtail(TileImage *p,
	double x1,double y1,double x2,double y2,uint32_t headtail);

void TileImage_drawBrushBox(TileImage *p,mDoublePoint *pt);

/* draw sub */

void TileImage_setDotStyle(int radius);
void TileImage_setFingerBuf(TileImage *p,int x,int y);

void TileImage_drawPixels_fromA1(TileImage *dst,TileImage *src,RGBAFix15 *pix);
void TileImage_combine_forA1(TileImage *dst,TileImage *src);
mBool TileImage_drawLineH_forA1(TileImage *p,int x1,int x2,int y);
mBool TileImage_drawLineV_forA1(TileImage *p,int y1,int y2,int x);

/* edit */

void TileImage_convertType(TileImage *p,int newtype,mBool bLumtoAlpha);
void TileImage_combine(TileImage *dst,TileImage *src,mRect *rc,int opasrc,int opadst,
	int blendmode,ImageBuf8 *src_texture,mPopupProgress *prog);
void TileImage_setImage_fromImageBufRGB16(TileImage *p,
	ImageBufRGB16 *src,mPopupProgress *prog);

void TileImage_replaceColor_fromTP(TileImage *p,RGBAFix15 *pixdst);
void TileImage_replaceColor_fromNotTP(TileImage *p,RGBAFix15 *src,RGBAFix15 *dst);

TileImage *TileImage_createCropImage(TileImage *src,int offx,int offy,int w,int h);
void TileImage_fullReverse_horz(TileImage *p);
void TileImage_fullReverse_vert(TileImage *p);
void TileImage_fullRotate_left(TileImage *p);
void TileImage_fullRotate_right(TileImage *p);

TileImage *TileImage_createStampImage(TileImage *src,TileImage *sel,mRect *rc);
void TileImage_pasteStampImage(TileImage *dst,int x,int y,TileImage *src,int srcw,int srch);

TileImage *TileImage_createSelCopyImage(TileImage *src,TileImage *sel,mRect *rc,mBool cut,mBool for_move);
void TileImage_pasteSelImage(TileImage *dst,TileImage *src,mRect *rcarea);

TileImage *TileImage_createCopyImage_box(TileImage *src,mBox *box,mBool clear);
TileImage *TileImage_createCopyImage_forTransform(TileImage *src,TileImage *sel,mBox *box);
void TileImage_clearImageBox_forTransform(TileImage *p,TileImage *sel,mBox *box);
void TileImage_restoreCopyImage_box(TileImage *dst,TileImage *src,mBox *box);

void TileImage_rectReverse_horz(TileImage *p,mBox *box);
void TileImage_rectReverse_vert(TileImage *p,mBox *box);
void TileImage_rectRotate_left(TileImage *p,TileImage *src,mBox *box);
void TileImage_rectRotate_right(TileImage *p,TileImage *src,mBox *box);

void TileImage_copyImage_rect(TileImage *dst,TileImage *src,mRect *rcarea);

void TileImage_transformAffine(TileImage *dst,TileImage *src,mBox *box,
	double scalex,double scaley,double dcos,double dsin,double movx,double movy,
	mPopupProgress *prog);
void TileImage_drawPixbuf_affine(TileImage *p,mPixbuf *pixbuf,mBox *boxsrc,
	double scalex,double scaley,double dcos,double dsin,mPoint *ptmov,CanvasDrawInfo *canvinfo);

void TileImage_transformHomography(TileImage *dst,TileImage *src,mRect *rcdst,
	double *param,int sx,int sy,int sw,int sh,mPopupProgress *prog);
void TileImage_drawPixbuf_homography(TileImage *p,mPixbuf *pixbuf,mBox *boxdst,
	double *param,mPoint *ptmov,int sx,int sy,int sw,int sh,CanvasDrawInfo *canvinfo);

/* select */

void TileImage_inverseSelect(TileImage *p);
void TileImage_getSelectHaveRect_real(TileImage *p,mRect *dst);
void TileImage_expandSelect(TileImage *p,int cnt,mPopupProgress *prog);
void TileImage_drawSelectEdge(TileImage *p,mPixbuf *pixbuf,CanvasDrawInfo *info,mBox *boximg);

/* scaling */

mBool TileImage_scaling(TileImage *dst,TileImage *src,int type,mSize *size_src,mSize *size_dst,mPopupProgress *prog);

/* imagefile */

TileImage *TileImage_loadFile(const char *filename,
	uint32_t format,mBool ignore_alpha,int maxsize,
	TileImageLoadFileInfo *info,mPopupProgress *prog,char **errmes);

mBool TileImage_savePNG_rgba(TileImage *p,const char *filename,int dpi,mPopupProgress *prog);
mBool TileImage_savePNG_select_rgba(TileImage *img,TileImage *selimg,
	const char *filename,int dpi,mRect *rc,mPopupProgress *prog);

mBool TileImage_saveTileImageFile(TileImage *p,const char *filename,uint32_t col);
TileImage *TileImage_loadTileImageFile(const char *filename,uint32_t *pcol);

mBool TileImage_setImage_fromRGBA8(TileImage *p,uint8_t *srcbuf,int srcw,int srch,mPopupProgress *prog,int substep);

mBool TileImage_setRowImage_RGBA8toRGBA16(TileImage *p,
	int y,uint8_t *buf,int width,int height,mBool bottomup);

mBool TileImage_setChannelImage(TileImage *p,uint8_t *chbuf,int chno,int srcw,int srch);

void TileImage_setTile_RGBA8toRGBA16(uint8_t *dst,uint8_t *src);
void TileImage_convertTile_A8toA16(uint8_t *tile);

#endif
