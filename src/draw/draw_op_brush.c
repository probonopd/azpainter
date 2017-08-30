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
 * DrawData
 *
 * 操作 - ブラシ
 *****************************************/
/*
 * - drawOpSub_setDrawInfo() 後、ntmp[0] にブラシのタイプがセットされる。
 */

#include <math.h>

#include "mDef.h"

#include "defDraw.h"
#include "defConfig.h"

#include "TileImage.h"
#include "TileImageDrawInfo.h"
#include "BrushList.h"
#include "BrushItem.h"
#include "BrushDrawParam.h"
#include "DrawPointBuf.h"
#include "SplineBuf.h"

#include "draw_main.h"
#include "draw_calc.h"
#include "draw_op_def.h"
#include "draw_op_sub.h"
#include "draw_op_func.h"

#include "Docks_external.h"


//-----------------

//ntmp[0] の値
enum
{
	_BRUSHTYPE_BRUSH,
	_BRUSHTYPE_DOTPEN,
	_BRUSHTYPE_FINGER
};

//-----------------



//===========================
// ブラシ描画 [自由線]
//===========================


/** 離し */

static mBool _brush_free_release(DrawData *p)
{
	DrawPointBuf_pt pt;

	//残りの線を描画

	TileImageDrawInfo_clearDrawRect();

	while(DrawPointBuf_getFinishPoint(&pt))
		TileImage_drawBrushFree(p->w.dstimg, pt.x, pt.y, pt.pressure);

	drawOpSub_addrect_and_update(p, TRUE);

	//終了

	drawOpSub_finishDraw_workrect(p);

	return TRUE;
}

/** 移動 */

static void _brush_free_motion(DrawData *p,uint32_t state)
{
	DrawPoint dpt;
	DrawPointBuf_pt pt;

	//位置

	drawOpSub_getDrawPoint(p, &dpt);

	if(p->rule.type)
		(p->rule.funcGetPoint)(p, &dpt.x, &dpt.y);

	if(p->w.opflags & DRAW_OPFLAGS_BRUSH_PRESSURE_MAX)
		dpt.pressure = 1;

	DrawPointBuf_addPoint(dpt.x, dpt.y, dpt.pressure);

	//描画
	/* pt = 手ぶれ補正後の位置 */

	if(DrawPointBuf_getPoint(&pt))
	{
		TileImageDrawInfo_clearDrawRect();

		TileImage_drawBrushFree(p->w.dstimg, pt.x, pt.y, pt.pressure);

		drawOpSub_addrect_and_update(p, TRUE);
	}
}

/** 押し */

mBool drawOp_brush_free_press(DrawData *p,mBool registered,mBool pressure_max)
{
	DrawPoint dpt;

	drawOpSub_setOpInfo(p, DRAW_OPTYPE_DRAW_FREE,
		_brush_free_motion, _brush_free_release, 0);

	//描画情報

	drawOpSub_setDrawInfo(p, TOOL_BRUSH, registered);

	//筆圧最大フラグ

	if(pressure_max)
		p->w.opflags |= DRAW_OPFLAGS_BRUSH_PRESSURE_MAX;

	//位置

	drawOpSub_getDrawPoint(p, &dpt);
	if(pressure_max) dpt.pressure = 1;

	DrawPointBuf_setFirstPoint(dpt.x, dpt.y, dpt.pressure);

	//開始

	TileImage_beginDrawBrush_free(p->w.dstimg, dpt.x, dpt.y, dpt.pressure);

	drawOpSub_beginDraw(p);

	return TRUE;
}


//=============================
// ドットペン [自由線]
//=============================
/*
 * pttmp[0] : 前回の位置
 */


/** 描画 */

static void _dotpen_draw(DrawData *p,mPoint *ptcur,mPoint *ptlast)
{
	//前回と同じ位置なら描画しない

	if(ptcur->x != ptlast->x || ptcur->y != ptlast->y)
	{
		//描画

		TileImageDrawInfo_clearDrawRect();

		TileImage_drawLineB(p->w.dstimg, ptlast->x, ptlast->y,
			ptcur->x, ptcur->y, &p->w.rgbaDraw, TRUE);

		drawOpSub_addrect_and_update(p, FALSE);

		//位置記録

		*ptlast = *ptcur;
	}
}

/** 移動 */

static void _dotpen_free_motion(DrawData *p,uint32_t state)
{
	DrawPoint dpt;
	mPoint pt;

	drawOpSub_getDrawPoint(p, &dpt);

	if(p->rule.type)
		(p->rule.funcGetPoint)(p, &dpt.x, &dpt.y);

	DrawPointBuf_addPoint(dpt.x, dpt.y, dpt.pressure);

	if(DrawPointBuf_getPoint_int(&pt))
		_dotpen_draw(p, &pt, p->w.pttmp);
}

/** 離し */

static mBool _dotpen_free_release(DrawData *p)
{
	mPoint pt;

	//手ブレ補正時、残りの線を描画

	while(DrawPointBuf_getFinishPoint_int(&pt))
		_dotpen_draw(p, &pt, p->w.pttmp);

	drawOpSub_finishDraw_workrect(p);
	
	return TRUE;
}

/** 押し
 *
 * @param registered 登録ブラシか */

mBool drawOp_dotpen_free_press(DrawData *p,mBool registered)
{
	DrawPoint dpt;
	mPoint pt;

	drawOpSub_setOpInfo(p, DRAW_OPTYPE_DRAW_FREE,
		_dotpen_free_motion, _dotpen_free_release, 0);

	//描画情報

	drawOpSub_setDrawInfo(p, TOOL_BRUSH, registered);

	//描画位置

	drawOpSub_getDrawPoint(p, &dpt);

	DrawPointBuf_setFirstPoint(dpt.x, dpt.y, dpt.pressure);
	DrawPointBuf_getPoint_int(&pt);

	p->w.pttmp[0] = pt;

	//描画開始

	drawOpSub_beginDraw(p);

	//最初の点を打つ

	(g_tileimage_dinfo.funcDrawPixel)(p->w.dstimg, pt.x, pt.y, &p->w.rgbaDraw);

	//更新

	drawOpSub_addrect_and_update(p, FALSE);

	//[dock]キャンバスビュー : ルーペ時は描画後に更新が行われないので、ここで行う

	drawUpdate_canvasview_inLoupe();

	return TRUE;
}


//=============================
// 指先 [自由線]
//=============================
/* 移動/離しは、ドットペンと共通 */


/** 押し */

mBool drawOp_finger_free_press(DrawData *p,mBool registered)
{
	DrawPoint dpt;
	mPoint pt;

	drawOpSub_setOpInfo(p, DRAW_OPTYPE_DRAW_FREE,
		_dotpen_free_motion, _dotpen_free_release, 0);

	//描画情報

	drawOpSub_setDrawInfo(p, TOOL_BRUSH, registered);

	//描画位置

	drawOpSub_getDrawPoint(p, &dpt);

	DrawPointBuf_setFirstPoint(dpt.x, dpt.y, dpt.pressure);
	DrawPointBuf_getPoint_int(&pt);

	p->w.pttmp[0] = pt;

	//開始位置の色をバッファにセット

	TileImage_setFingerBuf(p->w.dstimg, pt.x, pt.y);

	//描画開始

	drawOpSub_beginDraw(p);

	return TRUE;
}


//=============================
// 自由線以外の描画共通処理
//=============================


/** 自由線以外の描画開始時、開始点の状態をセット */

static void _set_startpoint(DrawData *p,mDoublePoint *pt)
{
	switch(p->w.ntmp[0])
	{
		//水彩時
		case _BRUSHTYPE_BRUSH:
			TileImage_beginDrawBrush_notfree(p->w.dstimg, pt->x, pt->y);
			break;
		//指先
		case _BRUSHTYPE_FINGER:
			TileImage_setFingerBuf(p->w.dstimg, pt->x, pt->y);
			break;
	}
}

/** 自由線以外の描画開始
 *
 * @param pt 開始点 */

static void _begin_drawbrush(DrawData *p,mDoublePoint *pt)
{
	drawOpSub_setDrawInfo(p, TOOL_BRUSH, 0);
	drawOpSub_beginDraw_single(p);

	_set_startpoint(p, pt);
}


//=============================
// 描画処理
//=============================


/** 直線描画 */

void drawOpDraw_brush_line(DrawData *p)
{
	mDoublePoint pt1,pt2;

	drawOpSub_getDrawLinePoints(p, &pt1, &pt2);

	_begin_drawbrush(p, &pt1);

	if(p->w.ntmp[0])
		TileImage_drawLineB(p->w.dstimg, pt1.x, pt1.y, pt2.x, pt2.y, &p->w.rgbaDraw, FALSE);
	else
	{
		TileImage_drawBrushLine_headtail(p->w.dstimg,
			pt1.x, pt1.y, pt2.x, pt2.y, p->headtail.curval[0]);
	}

	drawOpSub_endDraw_single(p);
}

/** 四角形枠描画 */

void drawOpDraw_brush_box(DrawData *p)
{
	mDoublePoint pt[4];
	int i,n;

	drawOpSub_getDrawBoxPoints(p, pt);

	_begin_drawbrush(p, pt);

	if(p->w.ntmp[0])
	{
		//ドットペン/指先
		
		for(i = 0; i < 4; i++)
		{
			n = (i + 1) & 3;
		
			TileImage_drawLineB(p->w.dstimg,
				pt[i].x, pt[i].y, pt[n].x, pt[n].y, &p->w.rgbaDraw, TRUE);
		}
	}
	else
		TileImage_drawBrushBox(p->w.dstimg, pt);

	drawOpSub_endDraw_single(p);
}

/** 楕円枠描画 */

void drawOpDraw_brush_ellipse(DrawData *p)
{
	mDoublePoint pt,pt_r,ptst;
	double x;

	drawOpSub_getDrawEllipseParam(p, &pt, &pt_r);

	//開始点は角度 0 の位置

	x = pt_r.x * p->viewparam.cosrev;
	if(p->canvas_mirror) x = -x;

	ptst.x = pt.x + x;
	ptst.y = pt.y + pt_r.x * p->viewparam.sinrev;

	_begin_drawbrush(p, &ptst);

	//

	TileImage_drawEllipse(p->w.dstimg, pt.x, pt.y, pt_r.x, pt_r.y, &p->w.rgbaDraw,
		p->w.ntmp[0], &p->viewparam, p->canvas_mirror);

	drawOpSub_endDraw_single(p);
}

/** 連続直線/集中線 描画 */

void drawOpDraw_brush_lineSuccConc(DrawData *p)
{
	mDoublePoint pt1,pt2;

	drawOpSub_getDrawLinePoints(p, &pt1, &pt2);

	TileImageDrawInfo_clearDrawRect();

	_set_startpoint(p, &pt1);

	if(p->w.ntmp[0])
		TileImage_drawLineB(p->w.dstimg, pt1.x, pt1.y, pt2.x, pt2.y, &p->w.rgbaDraw, FALSE);
	else
	{
		TileImage_drawBrushLine_headtail(p->w.dstimg,
			pt1.x, pt1.y, pt2.x, pt2.y, p->headtail.curval[0]);
	}

	drawOpSub_addrect_and_update(p, FALSE);
}

/** スプライン描画 */

void drawOpDraw_brush_spline(DrawData *p)
{
	mDoublePoint pt[5];
	mPoint ptlast;
	uint8_t pressure[2],first = 1,dotpen;
	double t = 0;

	if(!SplineBuf_initDraw()) return;

	drawOpSub_setDrawInfo(p, TOOL_BRUSH, 0);
	drawOpSub_beginDraw_single(p);

	dotpen = (p->w.ntmp[0] != _BRUSHTYPE_BRUSH);

	while(SplineBuf_getNextDraw(pt, pressure))
	{
		//最初の点 (水彩、指先の準備)
		
		if(first)
			_set_startpoint(p, pt);

		//2点間描画
	
		t = TileImage_drawSpline(p->w.dstimg, pt, pressure, &p->w.rgbaDraw,
				t, (dotpen)? &ptlast: NULL, first);

		first = 0;
	}

	drawOpSub_endDraw_single(p);
}


//===========================
// ベジェ曲線
//===========================
/*
	pttmp[0-3] : 始点、制御点1、制御点2、終点
	boxtmp[0]  : XOR 描画された範囲
	opsubtype  : 0=1つ目の制御点  1=2つ目の制御点
*/


/** 移動 */

static void _bezier_motion(DrawData *p,uint32_t state)
{
	mPoint pt;

	drawOpXor_drawBezier(p, TRUE);

	drawOpSub_getAreaPoint_int(p, &pt);

	if(state & M_MODS_SHIFT)
		drawCalc_fitLine45(&pt, p->w.pttmp + (p->w.opsubtype? 3: 0));

	p->w.pttmp[1 + p->w.opsubtype] = pt;

	if(p->w.opsubtype == 0)
		p->w.pttmp[2] = pt;

	drawOpXor_drawBezier(p, FALSE);
}

/** 離し */

static mBool _bezier_release(DrawData *p)
{
	drawOpXor_drawBezier(p, TRUE);

	if(p->w.opsubtype == 0)
	{
		//----- 2つ目の制御点へ

		p->w.pttmp[2] = p->w.pttmp[1];
		p->w.opsubtype = 1;

		drawOpXor_drawBezier(p, FALSE);

		return FALSE;
	}
	else
	{
		//----- 終了

		mDoublePoint pt[4];
		int i;

		//イメージ位置

		for(i = 0; i < 4; i++)
			drawCalc_areaToimage_double_pt(p, pt + i, p->w.pttmp + i);

		//描画

		_begin_drawbrush(p, pt);

		TileImage_drawBezier(p->w.dstimg, pt, &p->w.rgbaDraw,
			p->w.ntmp[0], p->headtail.curval[1]);

		drawOpSub_endDraw_single(p);

		//

		drawOpSub_freeTmpImage(p);

		return TRUE;
	}
}

/** 各アクション (操作終了) */

static mBool _bezier_action(DrawData *p,int action)
{
	switch(action)
	{
		//右ボタン/ESC/グラブ解放 : キャンセル
		case DRAW_FUNCACTION_RBTT_UP:
		case DRAW_FUNCACTION_KEY_ESC:
		case DRAW_FUNCACTION_UNGRAB:
			drawOpXor_drawBezier(p, TRUE);
			drawOpSub_freeTmpImage(p);
			return TRUE;
		//BackSpace : 一つ目の制御点に戻る
		case DRAW_FUNCACTION_KEY_BACKSPACE:
			if(p->w.opsubtype == 1)
			{
				p->w.opsubtype = 0;
				_bezier_motion(p, 0);
			}
			return FALSE;
		//ほか : 何もしない
		default:
			return FALSE;
	}
}

/** xorline から制御点指定へ移行
 *
 * @return グラブ解除するか */

mBool drawOp_xorline_to_bezier(DrawData *p)
{
	if(!drawOpSub_createTmpImage_area(p))
		return TRUE;
		
	drawOpSub_setOpInfo(p, DRAW_OPTYPE_XOR_BEZIER,
		_bezier_motion, _bezier_release, 0);

	p->w.funcAction = _bezier_action;

	p->w.pttmp[3] = p->w.pttmp[1];
	p->w.pttmp[1] = p->w.pttmp[2] = p->w.pttmp[3];

	//作業用イメージへの描画用
	g_tileimage_dinfo.funcDrawPixel = TileImage_setPixel_new_drawrect;

	drawOpXor_drawBezier(p, FALSE);

	return FALSE;
}


//=====================================================
// Shift+左右ドラッグ でのブラシサイズ変更
//=====================================================
/*
	pttmp[0]   : 押し時の位置
	ntmp[0]    : 半径サイズ(px)
	ntmp[1]    : 押し時のブラシサイズ
	ntmp[2]    : 前回のブラシサイズ
	boxtmp[0]  : XOR 描画範囲
	ptmp       : 対象ブラシアイテムポインタ (BrushItem *)
*/


/** 移動 */

static void _dragBrushSize_motion(DrawData *p,uint32_t state)
{
	BrushItem *item = (BrushItem *)p->w.ptmp;
	int n,finger;

	finger = (item->type == BRUSHITEM_TYPE_FINGER);

	//半径

	n = p->w.ntmp[1] + (int)((p->w.dptAreaCur.x - p->w.dptAreaPress.x) * (finger? 1: APP_CONF->dragBrushSize_step));

	n = BrushItem_adjustRadius(item, n);
	
	//変更

	if(n != p->w.ntmp[2])
	{
		p->w.ntmp[2] = n;
		
		//円描画
		
		drawOpXor_drawBrushSizeCircle(p, TRUE);

		p->w.ntmp[0] = (int)(n * (finger? 1: 0.1) * p->viewparam.scale + 0.5);

		drawOpXor_drawBrushSizeCircle(p, FALSE);

		//値変更 + バー更新
		/* 対象が登録ブラシで、選択ブラシが登録ブラシでない場合は、
		 * 値のみ変更。 */

		if(item == BrushList_getRegisteredItem())
			BrushList_updateval_radius_reg(n);
		else
		{
			if(finger) n *= 10;

			DockBrush_changeBrushSize(n);
		}
	}
}

/** 離し */

static mBool _dragBrushSize_release(DrawData *p)
{
	drawOpXor_drawBrushSizeCircle(p, TRUE);
	
	return TRUE;
}

/** 押し */

mBool drawOp_dragBrushSize_press(DrawData *p,mBool registered)
{
	BrushItem *item;
	int radius;

	//登録ブラシの場合、登録ブラシが存在するか

	if(registered && !BrushList_getRegisteredItem())
		return FALSE;

	//対象アイテム (ドットペン時は除く)

	radius = BrushList_getChangeBrushSizeInfo(&item, registered);

	if(item->type == BRUSHITEM_TYPE_DOTPEN)
		return FALSE;

	//

	drawOpSub_setOpInfo(p, DRAW_OPTYPE_GENERAL,
		_dragBrushSize_motion, _dragBrushSize_release, 0);

	//押し時の位置

	drawOpSub_getAreaPoint_int(p, &p->w.pttmp[0]);

	p->w.ptmp = item;
	p->w.ntmp[0] = (int)(radius * (item->type == BRUSHITEM_TYPE_FINGER? 1: 0.1) * p->viewparam.scale + 0.5);
	p->w.ntmp[1] = p->w.ntmp[2] = radius;

	//円描画

	drawOpXor_drawBrushSizeCircle(p, FALSE);

	return TRUE;
}
