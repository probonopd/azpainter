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
 * スプライン曲線
 *****************************************/

#include "mDef.h"
#include "mRectBox.h"

#include "defDraw.h"
#include "TileImage.h"
#include "TileImageDrawInfo.h"
#include "SplineBuf.h"

#include "MainWinCanvas.h"

#include "draw_calc.h"
#include "draw_op_def.h"
#include "draw_op_sub.h"
#include "draw_op_func.h"


/*
 * ntmp[0]  : キャンバススクロール中か
 * pttmp[0] : 前のポイントの位置
 */


//===========================
// 描画
//===========================


/** XOR 更新
 *
 * @param box NULL でキャンバス全体 */

static void _update_xor(DrawData *p,mBox *box)
{
	mPixbuf *pixbuf;
	mBox boxc;

	if(box && box->w == 0) return;

	if((pixbuf = drawOpSub_beginAreaDraw()))
	{
		//全体
		
		if(!box)
		{
			boxc.x = boxc.y = 0;
			boxc.w = p->szCanvas.w;
			boxc.h = p->szCanvas.h;
			box = &boxc;
		}

		//
	
		TileImage_blendXorToPixbuf(p->tileimgTmp, pixbuf, box);

		drawOpSub_endAreaDraw(pixbuf, box);
	}
}

/** XOR 用、2点の点と線を描画 */

static void _drawxor_twopoint(TileImage *img,mPoint *pt,mPoint *ptlast,uint8_t pressure)
{
	RGBAFix15 pix;

	pix.v64 = 0;
	pix.a = 0x8000;

	if(pressure)
		TileImage_drawFillBox(img, pt->x - 2, pt->y - 2, 5, 5, &pix);
	else
		//筆圧0
		TileImage_drawBox(img, pt->x - 2, pt->y - 2, 5, 5, &pix);
	
	TileImage_drawLineB(img, ptlast->x, ptlast->y, pt->x, pt->y, &pix, FALSE);
}

/** XOR 描画 (点追加時)
 *
 * @param pt 追加する点 */

static void _drawxor_addpoint(DrawData *p,mPoint *pt,uint8_t pressure)
{
	mRect rc;
	mBox box;

	//変化する範囲 (前回の点と追加点からなる矩形)

	mRectSetByPoint(&rc, pt);
	mRectIncPoint(&rc, p->w.pttmp[0].x, p->w.pttmp[0].y);
	mRectDeflate(&rc, 2, 2);

	drawCalc_clipArea_toBox(p, &box, &rc);

	//消去

	_update_xor(p, &box);

	//描画

	_drawxor_twopoint(p->tileimgTmp, pt, p->w.pttmp, pressure);

	//XOR

	_update_xor(p, &box);
}

/** 全体の再描画 */

static void _redraw_xor(DrawData *p,mBool erase)
{
	mPoint pt,ptlast;
	int ret,first = 1;

	if(erase)
		_update_xor(p, NULL);

	TileImage_freeAllTiles(p->tileimgTmp);

	//描画

	SplineBuf_beginGetPoint();

	while((ret = SplineBuf_getPoint(&pt)))
	{
		if(first)
		{
			ptlast = pt;
			first = 0;
		}
	
		_drawxor_twopoint(p->tileimgTmp, &pt, &ptlast, (ret == 1));

		ptlast = pt;
	}

	//XOR

	_update_xor(p, NULL);
}


//===========================
// 操作
//===========================


/** 移動 */

static void _spline_motion(DrawData *p,uint32_t state)
{
	mPoint pt,pt2;
	mBool in_area;

	drawOpSub_getAreaPoint_int(p, &pt);

	//キャンバスの範囲内か

	in_area = drawCalc_isPointInCanvasArea(p, &pt);

	//

	if(p->w.ntmp[0] && in_area)
	{
		//スクロール中で範囲内に入った場合、スクロール終了
		
		MainWinCanvasArea_clearTimer_scroll(&pt2);

		SplineBuf_scrollPoint(&pt2);

		_redraw_xor(p, FALSE);

		p->w.ntmp[0] = 0;

		p->w.pttmp[0].x -= pt2.x;
		p->w.pttmp[0].y -= pt2.y;
	}
	else if(p->w.ntmp[0] == 0 && !in_area)
	{
		//非スクロール中で範囲外に出た場合、スクロール開始

		if(pt.x < 0)
			pt2.x = -20, pt2.y = 0;
		else if(pt.y < 0)
			pt2.x = 0, pt2.y = -20;
		else if(pt.x >= p->szCanvas.w)
			pt2.x = 20, pt2.y = 0;
		else
			pt2.x = 0, pt2.y = 20;

		p->w.ntmp[0] = 1;

		_update_xor(p, NULL);

		MainWinCanvasArea_setTimer_scroll(&pt2);
	}
}

/** ２回目以降の押し時 */

static void _spline_pressInGrab(DrawData *p,uint32_t state)
{
	mPoint pt;
	uint8_t pressure;

	//非スクロール時、ポイント追加

	if(p->w.ntmp[0] == 0)
	{
		pressure = ((state & M_MODS_CTRL) == 0);

		drawOpSub_getAreaPoint_int(p, &pt);
		
		if(SplineBuf_addPoint(&pt, pressure))
		{
			_drawxor_addpoint(p, &pt, pressure);

			p->w.pttmp[0] = pt;
		}
	}
}

/** 各アクション */

static mBool _spline_action(DrawData *p,int action)
{
	mBool cancel = FALSE;

	//スクロール中は無効

	if(p->w.ntmp[0]) return FALSE;

	//BS => １つ戻る (点が１つの状態なら操作をキャンセルする)

	if(action == DRAW_FUNCACTION_KEY_BACKSPACE)
	{
		cancel = SplineBuf_deleteLastPoint(p->w.pttmp);

		if(!cancel)
		{
			_redraw_xor(p, TRUE);
			return FALSE;
		}
	}

	//---- 終了

	_update_xor(p, NULL);
	
	drawOpSub_freeTmpImage(p);

	//描画
	//(BS で点が一つだった、または ESC キーの場合は描画しない)

	if(!cancel && action != DRAW_FUNCACTION_KEY_ESC)
		drawOpDraw_brush_spline(p);

	SplineBuf_free();

	return TRUE;
}

/** 押し (開始) */

mBool drawOp_spline_press(DrawData *p)
{
	uint8_t pressure;

	//XOR 用イメージ作成

	if(!drawOpSub_createTmpImage_area(p))
		return FALSE;

	//スプラインバッファ開始

	if(!SplineBuf_start())
	{
		drawOpSub_freeTmpImage(p);
		return FALSE;
	}

	//-------

	drawOpSub_setOpInfo(p, DRAW_OPTYPE_SPLINE,
		_spline_motion, drawOp_common_norelease, 0);

	p->w.funcAction = _spline_action;
	p->w.funcPressInGrab = _spline_pressInGrab;

	g_tileimage_dinfo.funcDrawPixel = TileImage_setPixel_new_notp;

	p->w.ntmp[0] = 0;

	//最初の点を追加 (+Ctrl で筆圧 0)

	pressure = ((p->w.press_state & M_MODS_CTRL) == 0);

	drawOpSub_getAreaPoint_int(p, p->w.pttmp);

	SplineBuf_addPoint(p->w.pttmp, pressure);

	_drawxor_addpoint(p, p->w.pttmp, pressure);

	return TRUE;
}
