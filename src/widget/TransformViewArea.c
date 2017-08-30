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

/*****************************************
 * 変形ダイアログ - TransformViewArea
 *****************************************/

#include <math.h>
#include <string.h>

#include "mDef.h"
#include "mGui.h"
#include "mWidgetDef.h"
#include "mWidget.h"
#include "mEvent.h"
#include "mRectBox.h"

#include "defDraw.h"
#include "defConfig.h"
#include "ImageBufRGB16.h"
#include "TileImage.h"
#include "AppCursor.h"

#include "TransformDlg_def.h"


//--------------

#define _VIEW_INIT_SIZE    500

#define _DRAGF_MOVE_CANVAS -1
#define _DRAGF_ZOOM_CANVAS -2

/* calc_homography.c */
void getHomographyRevParam(double *dst,mDoublePoint *s,mDoublePoint *d);

//--------------


//=========================
// sub
//=========================


/** XOR 枠描画 */

static void _draw_xorframe(TransformViewArea *p)
{
	TileImage *img = p->imgxor;
	RGBAFix15 pix;
	mRect rcclip;
	int i;

	TileImage_freeAllTiles(img);

	pix.v64 = 0;
	pix.a = 0x8000;

	//枠

	rcclip.x1 = rcclip.y1 = 0;
	rcclip.x2 = p->wg.w - 1;
	rcclip.y2 = p->wg.h - 1;

	TileImage_drawLinePoint4(img, p->ptCornerCanv, &rcclip);

	//4隅

	for(i = 0; i < 4; i++)
		TileImage_drawBox(img, p->ptCornerCanv[i].x - 4, p->ptCornerCanv[i].y - 4, 9, 9, &pix);
}

/** キャンバスの状態 (スクロール/表示倍率) 変更時 */

static void _change_canvas_param(TransformViewArea *p)
{
	mDoublePoint dpt[2];
	int i;

	if(p->type == TRANSFORM_TYPE_NORMAL)
	{
		//アフィン変換: 変換後の4隅のキャンバス座標を、ソース座標から取得

		dpt[0].x = dpt[0].y = 0;
		dpt[1].x = p->boxsrc.w;
		dpt[1].y = p->boxsrc.h;

		TransformViewArea_affine_source_to_canv_pt(p, p->ptCornerCanv, dpt[0].x, dpt[0].y);
		TransformViewArea_affine_source_to_canv_pt(p, p->ptCornerCanv + 1, dpt[1].x, dpt[0].y);
		TransformViewArea_affine_source_to_canv_pt(p, p->ptCornerCanv + 2, dpt[1].x, dpt[1].y);
		TransformViewArea_affine_source_to_canv_pt(p, p->ptCornerCanv + 3, dpt[0].x, dpt[1].y);
	}
	else
	{
		//射影変換: 4隅のキャンバス座標を、イメージ座標から取得

		for(i = 0; i < 4; i++)
		{
			TransformViewArea_image_to_canv_pt(p, p->ptCornerCanv + i,
				p->dptCornerDst[i].x + p->ptmov.x,
				p->dptCornerDst[i].y + p->ptmov.y);
		}
	}
}

/** 変形パラメータ変更時、作業値更新 */

static void _change_transform_param(TransformViewArea *p)
{
	if(p->type == TRANSFORM_TYPE_NORMAL)
	{
		//アフィン変換
		
		p->dcos = cos(p->angle);
		p->dsin = sin(p->angle);

		p->scalex_div = 1 / p->scalex;
		p->scaley_div = 1 / p->scaley;
	}
	else
	{
		//射影変換

		getHomographyRevParam(p->homog_param,
			p->dptCornerSrc, p->dptCornerDst);
	}

	_change_canvas_param(p);
}


//=======================
// ハンドラ sub
//=======================


/** ドラッグ移動時 (イメージ操作) */

static void _event_motion(TransformViewArea *p,mEvent *ev)
{
	mDoublePoint dpt;
	int x,y;
	double d1,d2;
	uint8_t update = 1;

	x = ev->pt.x;
	y = ev->pt.y;

	//

	if(p->fdrag == TRANSFORM_DSTAREA_OUT_IMAGE)
	{
		//アフィン変換: 回転

		d1 = TransformViewArea_affine_get_canvas_angle(p, x, y);
		
		p->angle += d1 - p->dtmp[0];

		p->dtmp[0] = d1;

		update |= 2;
	}
	else if(p->fdrag == TRANSFORM_DSTAREA_IN_IMAGE)
	{
		//共通: 平行移動

		p->ptmov.x += round((x - p->ptLastCanv.x) * p->viewparam.scalediv);
		p->ptmov.y += round((y - p->ptLastCanv.y) * p->viewparam.scalediv);
	}
	else if(p->type == TRANSFORM_TYPE_NORMAL)
	{
		//アフィン変換: 4隅の点 拡大縮小
		/* 押し時の位置とカーソル位置のソース座標から倍率計算 */

		TransformViewArea_affine_canv_to_source_for_drag(p, &dpt, x, y);

		dpt.x -= p->centerx;
		dpt.y -= p->centery;

		d1 = fabs(dpt.x / p->dtmp[0]);
		d2 = fabs(dpt.y / p->dtmp[1]);

		if(d1 == 0) d1 = 0.001;
		if(d2 == 0) d2 = 0.001;

		if(p->keep_aspect)
		{
			if(d1 > d2)
				d2 = d1;
			else
				d1 = d2;
		}

		p->scalex = d1;
		p->scaley = d2;

		update |= 2;
	}
	else
	{
		//射影変換: 4隅の点 移動

		update = TransformViewArea_onMotion_homog(p, x, y);
	}

	//更新 (タイマー)

 	if(update & 1)
		mWidgetTimerAdd_unexist(M_WIDGET(p), 0, 5, 0);

	//エディットの値更新

	if(update & 2)
		mWidgetAppendEvent_notify(NULL, M_WIDGET(p), TRANSFORM_AREA_NOTIFY_CHANGE_AFFINE_PARAM, 0, 0);
	
	//座標

	p->ptLastCanv.x = x;
	p->ptLastCanv.y = y;
}

/** 移動時 (キャンバス移動) */

static void _event_motion_canvasmove(TransformViewArea *p,int x,int y)
{
	int sx,sy;

	sx = p->canv_scrx + p->ptLastCanv.x - x;
	sy = p->canv_scry + p->ptLastCanv.y - y;

	//セット

	if(sx != p->canv_scrx || sy != p->canv_scry)
	{
		p->canv_scrx = sx;
		p->canv_scry = sy;

		TransformViewArea_update(p, TRANSFORM_UPDATE_WITH_CANVAS);
	}

	//

	p->ptLastCanv.x = x;
	p->ptLastCanv.y = y;
}

/** 移動時 (キャンバス拡大縮小) */

static void _event_motion_canvaszoom(TransformViewArea *p,int y)
{
	int n;

	n = p->canv_zoom + p->ptLastCanv.y - y;

	if(n < TRANSFORM_CANVZOOM_MIN)
		n = TRANSFORM_CANVZOOM_MIN;
	else if(n > TRANSFORM_CANVZOOM_MAX)
		n = TRANSFORM_CANVZOOM_MAX;

	if(n != p->canv_zoom)
	{
		TransformViewArea_setCanvasZoom(p, n);

		mWidgetAppendEvent_notify(NULL, M_WIDGET(p), TRANSFORM_AREA_NOTIFY_CHANGE_CANVAS_ZOOM, 0, 0);
	}

	p->ptLastCanv.y = y;
}

/** 押し時 */

static void _event_press(TransformViewArea *p,mEvent *ev)
{
	int isleft,type,cursor = -1;

	isleft = (ev->pt.btt == M_BTT_LEFT);

	if(ev->pt.btt == M_BTT_MIDDLE
		|| (isleft && (ev->pt.state & M_MODS_CTRL)))
	{
		//左+Ctrl or 中ボタン: キャンバス移動 (ドラッグ中低品質)

		type = _DRAGF_MOVE_CANVAS;
		cursor = APP_CURSOR_HAND_DRAG;

		p->low_quality = TRUE;
	}
	else if(isleft && (ev->pt.state & M_MODS_SHIFT))
	{
		//左+Shift: キャンバス拡大縮小

		type = _DRAGF_ZOOM_CANVAS;
		cursor = APP_CURSOR_ZOOM_DRAG;
		
		TransformViewArea_startCanvasZoomDrag(p);
	}
	else if(isleft && p->cur_area != TRANSFORM_DSTAREA_NONE)
	{
		//変形操作

		type = p->cur_area;
		
		switch(type)
		{
			//平行移動
			case TRANSFORM_DSTAREA_IN_IMAGE:
				cursor = APP_CURSOR_HAND_DRAG;
				break;
			//アフィン変換:回転
			/* dtmp[0] : 前回の角度 */
			case TRANSFORM_DSTAREA_OUT_IMAGE:
				p->dtmp[0] = TransformViewArea_affine_get_canvas_angle(p, ev->pt.x, ev->pt.y);
				break;
			//4隅の点
			/* dtmp[0,1] : 操作点のソース位置
			 * dtmp[2,3] : 開始時の逆倍率 */
			default:
				if(p->type == TRANSFORM_TYPE_NORMAL)
					TransformViewArea_onPress_affineZoom(p, ev->pt.x, ev->pt.y);
				break;
		}
	}
	else
		return;

	//グラブ

	p->fdrag = type;
	p->dragbtt = ev->pt.btt;

	p->ptLastCanv.x = ev->pt.x;
	p->ptLastCanv.y = ev->pt.y;

	if(cursor < 0)
		mWidgetGrabPointer(M_WIDGET(p));
	else
		mWidgetGrabPointer_cursor(M_WIDGET(p), AppCursor_getForDrag(cursor));
}

/** グラブ解放
 *
 * @param ev  フォーカスアウト時は NULL */

static void _ungrab(TransformViewArea *p,mEvent *ev)
{
	if(p->fdrag)
	{
		//キャンバス移動/ズーム時
	
		if(p->fdrag == _DRAGF_MOVE_CANVAS || p->fdrag == _DRAGF_ZOOM_CANVAS)
		{
			p->low_quality = FALSE;
			mWidgetUpdate(M_WIDGET(p));

			//カーソル下のエリア再セット

			if(ev)
				TransformViewArea_onMotion_nodrag(p, ev->pt.x, ev->pt.y);
		}

		//グラブ解除

		p->fdrag = TRANSFORM_DSTAREA_NONE;
		mWidgetUngrabPointer(M_WIDGET(p));

		//タイマーが残っている場合、更新

		if(mWidgetTimerDelete(M_WIDGET(p), 0))
			TransformViewArea_update(p, TRANSFORM_UPDATE_WITH_TRANSPARAM);
	}
}


//=======================
// ハンドラ main
//=======================


/** イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	TransformViewArea *p = (TransformViewArea *)wg;

	switch(ev->type)
	{
		//ポインタ
		case MEVENT_POINTER:
			if(ev->pt.type == MEVENT_POINTER_TYPE_MOTION)
			{
				if(p->fdrag == TRANSFORM_DSTAREA_NONE)
					TransformViewArea_onMotion_nodrag(p, ev->pt.x, ev->pt.y);
				else if(p->fdrag == _DRAGF_MOVE_CANVAS)
					//キャンバススクロール
					_event_motion_canvasmove(p, ev->pt.x, ev->pt.y);
				else if(p->fdrag == _DRAGF_ZOOM_CANVAS)
					//キャンバス拡大縮小
					_event_motion_canvaszoom(p, ev->pt.y);
				else
					//イメージ操作
					_event_motion(p, ev);
			}
			else if(ev->pt.type == MEVENT_POINTER_TYPE_PRESS)
			{
				if(!p->fdrag)
					_event_press(p, ev);
			}
			else if(ev->pt.type == MEVENT_POINTER_TYPE_RELEASE)
			{
				if(p->dragbtt == ev->pt.btt)
					_ungrab(p, ev);
			}
			break;
	
		//タイマー
		case MEVENT_TIMER:
			mWidgetTimerDelete(wg, 0);

			TransformViewArea_update(p, TRANSFORM_UPDATE_WITH_TRANSPARAM);
			break;

		case MEVENT_FOCUS:
			if(ev->focus.bOut)
				_ungrab(p, NULL);
			break;
	}

	return 1;
}

/** 描画ハンドラ */

static void _draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	TransformViewArea *p = (TransformViewArea *)wg;
	CanvasDrawInfo *di = &p->canvinfo;
	mBox box;

	//blendimg から背景描画

	di->boxdst.w = wg->w;
	di->boxdst.h = wg->h;
	di->scrollx = p->canv_scrx - (wg->w >> 1);
	di->scrolly = p->canv_scry - (wg->h >> 1);
	di->originx = p->canv_origx;
	di->originy = p->canv_origy;

	if(!p->low_quality && p->canv_zoom != 100 && p->canv_zoom < 200)
		ImageBufRGB16_drawMainCanvas_oversamp(APP_DRAW->blendimg, pixbuf, di);
	else 
		ImageBufRGB16_drawMainCanvas_nearest(APP_DRAW->blendimg, pixbuf, di);

	//変換後イメージ

	if(p->type == TRANSFORM_TYPE_NORMAL)
	{
		TileImage_drawPixbuf_affine(p->imgcopy, pixbuf, &p->boxsrc,
			p->scalex_div, p->scaley_div, p->dcos, p->dsin, &p->ptmov, di);
	}
	else
	{
		if(TransformViewArea_homog_getDstCanvasBox(p, &box))
		{
			TileImage_drawPixbuf_homography(p->imgcopy, pixbuf, &box,
				p->homog_param, &p->ptmov,
				p->boxsrc.x, p->boxsrc.y, p->boxsrc.w, p->boxsrc.h, di);
		}
	}

	//XOR 枠

	_draw_xorframe(p);

	TileImage_blendXorToPixbuf(p->imgxor, pixbuf, &di->boxdst);
}

/** サイズ変更時 */

static void _onsize_handle(mWidget *wg)
{
	_change_canvas_param((TransformViewArea *)wg);
}

/** 破棄ハンドラ */

static void _destroy_handle(mWidget *wg)
{
	TileImage_free(((TransformViewArea *)wg)->imgxor);
}


//=========================
// 初期化
//=========================


/** 射影変換ソース位置セット */

static void _set_corner_src(TransformViewArea *p)
{
	mRect rc;
	mDoublePoint *ppt = p->dptCornerSrc;
	double dx1,dy1;

	mRectSetByBox(&rc, &p->boxsrc);
	rc.x2++;
	rc.y2++;

	//[!] 座標値が 0 だと射影変換計算時に正しく計算できないので少しずらす

	dx1 = (rc.x1 == 0)? 0.001: rc.x1;
	dy1 = (rc.y1 == 0)? 0.001: rc.y1;

	ppt[0].x = dx1;
	ppt[0].y = dy1;
	ppt[1].x = rc.x2;
	ppt[1].y = dy1;
	ppt[2].x = rc.x2;
	ppt[2].y = rc.y2;
	ppt[3].x = dx1;
	ppt[3].y = rc.y2;
}

/** 作成 */

TransformViewArea *TransformViewArea_create(mWidget *parent,int wid)
{
	TransformViewArea *p;
	CanvasDrawInfo *di;
	mBox box;

	//作成 (初期サイズは前回のサイズ)

	p = (TransformViewArea *)mWidgetNew(sizeof(TransformViewArea), parent);

	p->wg.id = wid;
	p->wg.fLayout = MLF_EXPAND_WH;
	p->wg.destroy = _destroy_handle;
	p->wg.event = _event_handle;
	p->wg.draw = _draw_handle;
	p->wg.onSize = _onsize_handle;
	p->wg.fEventFilter |= MWIDGET_EVENTFILTER_POINTER;
	p->wg.initW = (APP_CONF->size_boxeditdlg.w > 0)? APP_CONF->size_boxeditdlg.w: _VIEW_INIT_SIZE;
	p->wg.initH = (APP_CONF->size_boxeditdlg.h > 0)? APP_CONF->size_boxeditdlg.h: _VIEW_INIT_SIZE;

	//

	p->canv_zoom = 100;
	p->boxsrc = APP_DRAW->boxedit.box;

	p->centerx = p->boxsrc.w * 0.5;
	p->centery = p->boxsrc.h * 0.5;

	p->canv_origx = p->boxsrc.x + p->centerx;
	p->canv_origy = p->boxsrc.y + p->centery;

	p->keep_aspect = ((APP_CONF->boxedit_flags & TRANSFORM_CONFIG_F_KEEPASPECT) != 0);

	p->viewparam.scale = p->viewparam.scalediv = 1;

	//XOR 用イメージ作成 (デスクトップ全体)

	mGetDesktopWorkBox(&box);

	p->imgxor = TileImage_new(TILEIMAGE_COLTYPE_ALPHA1, box.w, box.h);
	if(!(p->imgxor))
	{
		mWidgetDestroy(M_WIDGET(p));
		return NULL;
	}

	//CanvasDrawInfo (背景の描画情報)

	di = &p->canvinfo;

	memset(di, 0, sizeof(CanvasDrawInfo));

	di->imgw = APP_DRAW->imgw;
	di->imgh = APP_DRAW->imgh;
	di->bkgndcol = 0xd0d0d0;
	di->param = &p->viewparam;

	//射影変換ソース位置

	_set_corner_src(p);

	//パラメータ値

	TransformViewArea_resetTransformParam(p);

	return p;
}

/** 初期化
 *
 * 矩形範囲に合わせて、表示倍率セット */

void TransformViewArea_init(TransformViewArea *p)
{
	mBox *box = &APP_DRAW->boxedit.box;
	double dh,dv;
	int n;

	//表示倍率: 矩形範囲全体が表示できる状態に (縮小のみ)

	dh = (double)p->wg.initW / (box->w * 2);
	dv = (double)p->wg.initH / (box->h * 2);

	if(dh > dv) dh = dv;
	if(dh > 1) dh = 1;

	n = (int)(dh * 100 + 0.5);
	if(n < 1) n = 1;

	TransformViewArea_setCanvasZoom(p, n);

	//パラメータ作業用

	_change_transform_param(p);
}


//============================
// ダイアログからも使われる
//============================


/** 表示倍率セット */

void TransformViewArea_setCanvasZoom(TransformViewArea *p,int zoom)
{
	p->canv_zoom = zoom;

	p->viewparam.scale = zoom * 0.01;
	p->viewparam.scalediv = 1.0 / p->viewparam.scale;

	TransformViewArea_update(p, TRANSFORM_UPDATE_WITH_CANVAS);
}

/** 変形パラメータ値をリセット */

void TransformViewArea_resetTransformParam(TransformViewArea *p)
{
	p->ptmov.x = p->ptmov.y = 0;

	//アフィン変換

	p->scalex = p->scaley = 1;
	p->angle = 0;

	//射影変換: 4隅のイメージ位置

	memcpy(p->dptCornerDst, p->dptCornerSrc, sizeof(mDoublePoint) * 4);
}

/** パラメータ更新＋ウィジェット更新 */

void TransformViewArea_update(TransformViewArea *p,uint8_t flags)
{
	if(flags & TRANSFORM_UPDATE_WITH_CANVAS)
		_change_canvas_param(p);

	if(flags & TRANSFORM_UPDATE_WITH_TRANSPARAM)
		_change_transform_param(p);

	mWidgetUpdate(M_WIDGET(p));
}

/** キャンバス拡大縮小のドラッグ開始時 */

void TransformViewArea_startCanvasZoomDrag(TransformViewArea *p)
{
	mDoublePoint pt;

	p->low_quality = 1;

	//イメージ原点を現在の中央位置に

	TransformViewArea_canv_to_image_dpt(p, &pt, p->wg.w * 0.5, p->wg.h * 0.5);

	p->canv_origx = pt.x;
	p->canv_origy = pt.y;
	p->canv_scrx = p->canv_scry = 0;

	_change_canvas_param(p);
}
