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
 * DrawData : 操作関連のサブ処理関数
 **************************************/

#ifndef DRAW_OP_SUB_H
#define DRAW_OP_SUB_H

typedef struct _DrawData DrawData;
typedef struct _DrawPoint DrawPoint;
typedef struct _TileImageDrawGradInfo TileImageDrawGradInfo;

enum
{
	CANDRAWLAYER_OK,
	CANDRAWLAYER_FOLDER,
	CANDRAWLAYER_LOCK,

	CANDRAWLAYER_MES_HIDE	//レイヤが非表示
};


/*-----------*/

void drawOpSub_setOpInfo(DrawData *p,int optype,
	void (*motion)(DrawData *,uint32_t),mBool (*release)(DrawData *),int opsubtype);

int drawOpSub_canDrawLayer(DrawData *p);
int drawOpSub_canDrawLayer_mes(DrawData *p);
mBool drawOpSub_isFolder_curlayer();

/* 作業用イメージなど */

mBool drawOpSub_createTmpImage_area(DrawData *p);
mBool drawOpSub_createTmpImage_same_current_imgsize(DrawData *p);

void drawOpSub_freeTmpImage(DrawData *p);
void drawOpSub_freeFillPolygon(DrawData *p);

/* イメージ */

TileImage *drawOpSub_createSelectImage_byOpType(DrawData *p,mRect *rcarea);

/* 描画 */

void drawOpSub_beginDraw(DrawData *p);
void drawOpSub_endDraw(DrawData *p,mRect *rc,mBox *boximg);

void drawOpSub_beginDraw_single(DrawData *p);
void drawOpSub_endDraw_single(DrawData *p);

void drawOpSub_addrect_and_update(DrawData *p,mBool btimer);
void drawOpSub_finishDraw_workrect(DrawData *p);
void drawOpSub_finishDraw_single(DrawData *p);

void drawOpSub_setDrawInfo(DrawData *p,int toolno,int param);

void drawOpSub_clearDrawMasks();
void drawOpSub_setDrawInfo_select();
void drawOpSub_setDrawInfo_select_paste();
void drawOpSub_setDrawInfo_select_cut();
void drawOpSub_setDrawInfo_fillerase(mBool erase);
void drawOpSub_setDrawInfo_overwrite();
void drawOpSub_setDrawInfo_filter();
void drawOpSub_setDrawInfo_filter_comic_brush();

void drawOpSub_setDrawGradationInfo(TileImageDrawGradInfo *info);

mPixbuf *drawOpSub_beginAreaDraw();
void drawOpSub_endAreaDraw(mPixbuf *pixbuf,mBox *box);

/* ポイント取得など */

void drawOpSub_getAreaPoint_int(DrawData *p,mPoint *pt);

void drawOpSub_getImagePoint_fromDrawPoint(DrawData *p,mDoublePoint *pt,DrawPoint *dpt);
void drawOpSub_getImagePoint_double(DrawData *p,mDoublePoint *pt);
void drawOpSub_getImagePoint_int(DrawData *p,mPoint *pt);

void drawOpSub_getDrawPoint(DrawData *p,DrawPoint *dst);

/* 操作関連 */

void drawOpSub_copyTmpPoint(DrawData *p,int num);
void drawOpSub_startMoveImage(DrawData *p,TileImage *img);
mBool drawOpSub_isPressInSelect(DrawData *p);

void drawOpSub_getDrawLinePoints(DrawData *p,mDoublePoint *pt1,mDoublePoint *pt2);
void drawOpSub_getDrawBox_noangle(DrawData *p,mBox *box);
void drawOpSub_getDrawBoxPoints(DrawData *p,mDoublePoint *pt);
void drawOpSub_getDrawEllipseParam(DrawData *p,mDoublePoint *pt_ct,mDoublePoint *pt_radius);

#endif
