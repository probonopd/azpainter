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
 * 操作 - XOR 処理
 *****************************************/

#include <stdlib.h>

#include "mDef.h"
#include "mRectBox.h"
#include "mPixbuf.h"

#include "defDraw.h"
#include "TileImage.h"
#include "TileImageDrawInfo.h"
#include "FillPolygon.h"

#include "draw_main.h"
#include "draw_calc.h"
#include "draw_op_def.h"
#include "draw_op_func.h"
#include "draw_op_sub.h"
#include "draw_rule.h"
#include "draw_boxedit.h"



//=============================
// sub
//=============================


/** 矩形範囲 共通処理
 *
 * pttmp[0] : 左上, pttmp[1] : 右下, pttmp[2] : 開始点 */

static void _common_rect_func(DrawData *p,int x,int y,uint32_t state)
{
	int x1,y1,x2,y2,w,h;

	//左上、左下

	if(x < p->w.pttmp[2].x)
		x1 = x, x2 = p->w.pttmp[2].x;
	else
		x1 = p->w.pttmp[2].x, x2 = x;

	if(y < p->w.pttmp[2].y)
		y1 = y, y2 = p->w.pttmp[2].y;
	else
		y1 = p->w.pttmp[2].y, y2 = y;

	//正方形調整

	if(state & M_MODS_SHIFT)
	{
		w = x2 - x1;
		h = y2 - y1;

		if(w < h)
		{
			if(x < p->w.pttmp[2].x)
				x1 = x2 - h;
			else
				x2 = x1 + h;
		}
		else
		{
			if(y < p->w.pttmp[2].y)
				y1 = y2 - w;
			else
				y2 = y1 + w;
		}
	}

	p->w.pttmp[0].x = x1, p->w.pttmp[0].y = y1;
	p->w.pttmp[1].x = x2, p->w.pttmp[1].y = y2;
}


//=============================
// XOR直線
//=============================
/*
	pttmp[0] : 始点
	pttmp[1] : 終点 (現在の位置)
*/


/** 移動 (連続直線/集中線/多角形と共用) */

static void _xorline_motion(DrawData *p,uint32_t state)
{
	mPoint pt;

	drawOpSub_getAreaPoint_int(p, &pt);

	if(pt.x == p->w.pttmp[1].x && pt.y == p->w.pttmp[1].y)
		return;

	//

	drawOpXor_drawline(p);

	if(state & M_MODS_SHIFT)
		drawCalc_fitLine45(&pt, p->w.pttmp);

	p->w.pttmp[1] = pt;

	drawOpXor_drawline(p);
}

/** 離し */

static mBool _xorline_release(DrawData *p)
{
	drawOpXor_drawline(p);

	switch(p->w.opsubtype)
	{
		//直線描画
		case DRAW_OPSUB_DRAW_LINE:
			drawOpDraw_brush_line(p);
			break;
		//ベジェ曲線、制御点操作に移行
		case DRAW_OPSUB_TO_BEZIER:
			return drawOp_xorline_to_bezier(p);
		//定規設定
		case DRAW_OPSUB_RULE_SETTING:
			drawRule_setting_line(p);
			break;
		//グラデーション描画
		case DRAW_OPSUB_DRAW_GRADATION:
			drawOpDraw_gradation(p);
			break;
	}
	
	return TRUE;
}

/** 押し */

mBool drawOpXor_line_press(DrawData *p,int opsubtype)
{
	drawOpSub_setOpInfo(p, DRAW_OPTYPE_XOR_LINE,
		_xorline_motion, _xorline_release, opsubtype);

	p->w.opflags |= DRAW_OPFLAGS_MOTION_POS_INT;

	drawOpSub_getAreaPoint_int(p, p->w.pttmp);

	p->w.pttmp[1] = p->w.pttmp[0];

	drawOpXor_drawline(p);

	return TRUE;
}


//=============================
// XOR 四角形 (領域に対する)
//=============================
/*
	pttmp[0] : 左上
	pttmp[1] : 右下
	pttmp[2] : 開始点
*/


/** 移動 */

static void _xorbox_area_motion(DrawData *p,uint32_t state)
{
	mPoint pt;

	drawOpXor_drawBox_area(p);

	drawOpSub_getAreaPoint_int(p, &pt);

	_common_rect_func(p, pt.x, pt.y, state);

	drawOpXor_drawBox_area(p);
}

/** 離し */

static mBool _xorbox_area_release(DrawData *p)
{
	drawOpXor_drawBox_area(p);

	switch(p->w.opsubtype)
	{
		//四角形枠描画
		case DRAW_OPSUB_DRAW_FRAME:
			drawOpDraw_brush_box(p);
			break;
		//四角形塗りつぶし
		case DRAW_OPSUB_DRAW_FILL:
			drawOpDraw_fillBox(p);
			break;
		//選択範囲セット
		case DRAW_OPSUB_SET_SELECT:
			drawOp_setSelect(p, 0);
			break;
	}
	
	return TRUE;
}

/** 押し */

mBool drawOpXor_boxarea_press(DrawData *p,int opsubtype)
{
	drawOpSub_setOpInfo(p, DRAW_OPTYPE_XOR_BOXAREA,
		_xorbox_area_motion, _xorbox_area_release, opsubtype);

	p->w.opflags |= DRAW_OPFLAGS_MOTION_POS_INT;

	drawOpSub_getAreaPoint_int(p, p->w.pttmp);

	p->w.pttmp[1] = p->w.pttmp[2] = p->w.pttmp[0];

	drawOpXor_drawBox_area(p);

	return TRUE;
}


//=================================
// XOR 四角形 (イメージに対する)
//=================================
/*
	pttmp[0] : 左上位置 (イメージ座標)
	pttmp[1] : 右下位置 (イメージ座標)
	pttmp[2] : 開始点 (イメージ座標)
	pttmp[3] : 前回の位置 (イメージ座標)

	boxtmp[0] : 結果の mBox
	rctmp[0]  : 結果の mRect

	[!] 範囲のクリッピングは行われないので注意
*/


/** 移動 */

static void _xorbox_image_motion(DrawData *p,uint32_t state)
{
	mPoint pt;

	drawOpSub_getImagePoint_int(p, &pt);

	if(pt.x != p->w.pttmp[3].x || pt.y != p->w.pttmp[3].y)
	{
		drawOpXor_drawBox_image(p);

		_common_rect_func(p, pt.x, pt.y, state);

		drawOpXor_drawBox_image(p);

		p->w.pttmp[3] = pt;
	}
}

/** 離し */

static mBool _xorbox_image_release(DrawData *p)
{
	drawOpXor_drawBox_image(p);

	//結果範囲 (mBox, mRect)

	mBoxSetByPoint(p->w.boxtmp, p->w.pttmp, p->w.pttmp + 1);

	p->w.rctmp[0].x1 = p->w.pttmp[0].x, p->w.rctmp[0].y1 = p->w.pttmp[0].y;
	p->w.rctmp[0].x2 = p->w.pttmp[1].x, p->w.rctmp[0].y2 = p->w.pttmp[1].y;

	//

	switch(p->w.opsubtype)
	{
		//スタンプ画像セット
		case DRAW_OPSUB_SET_STAMP:
			drawOp_setStampImage(p);
			break;
		//矩形編集範囲セット
		case DRAW_OPSUB_SET_BOXEDIT:
			drawBoxEdit_setBox(p, p->w.rctmp);
			break;
	}

	return TRUE;
}

/** 押し */

mBool drawOpXor_boximage_press(DrawData *p,int opsubtype)
{
	drawOpSub_setOpInfo(p, DRAW_OPTYPE_XOR_BOXIMAGE,
		_xorbox_image_motion, _xorbox_image_release, opsubtype);

	p->w.opflags |= DRAW_OPFLAGS_MOTION_POS_INT;

	drawOpSub_getImagePoint_int(p, p->w.pttmp);

	drawOpSub_copyTmpPoint(p, 3);

	drawOpXor_drawBox_image(p);

	return TRUE;
}


//=============================
// XOR 楕円 (領域に対する)
//=============================
/*
	pttmp[0] : 位置 (領域座標)
	pttmp[1] : 半径
	ntmp[0]  : 正円か
*/


/** 移動 */

static void _xorellipse_motion(DrawData *p,uint32_t state)
{
	mPoint pt;
	int xr,yr;

	drawOpXor_drawEllipse(p);

	drawOpSub_getAreaPoint_int(p, &pt);

	xr = abs(pt.x - p->w.pttmp[0].x);
	yr = abs(pt.y - p->w.pttmp[0].y);

	if(state & M_MODS_SHIFT)
	{
		if(xr > yr) yr = xr;
		else xr = yr;

		p->w.ntmp[0] = 1;
	}
	else
		p->w.ntmp[0] = 0;

	p->w.pttmp[1].x = xr;
	p->w.pttmp[1].y = yr;

	drawOpXor_drawEllipse(p);
}

/** 離し */

static mBool _xorellipse_release(DrawData *p)
{
	drawOpXor_drawEllipse(p);

	switch(p->w.opsubtype)
	{
		//円枠描画
		case DRAW_OPSUB_DRAW_FRAME:
			drawOpDraw_brush_ellipse(p);
			break;
		//円塗りつぶし描画
		case DRAW_OPSUB_DRAW_FILL:
			drawOpDraw_fillEllipse(p);
			break;
		//定規設定
		case DRAW_OPSUB_RULE_SETTING:
			drawRule_setting_ellipse(p);
			break;
	}
	
	return TRUE;
}

/** 押し */

mBool drawOpXor_ellipse_press(DrawData *p,int opsubtype)
{
	drawOpSub_setOpInfo(p, DRAW_OPTYPE_XOR_ELLIPSE,
		_xorellipse_motion, _xorellipse_release, opsubtype);

	p->w.opflags |= DRAW_OPFLAGS_MOTION_POS_INT;

	drawOpSub_getAreaPoint_int(p, p->w.pttmp);

	p->w.pttmp[1].x = p->w.pttmp[1].y = 0;
	p->w.ntmp[0] = 0;

	drawOpXor_drawEllipse(p);

	return TRUE;
}


//========================================
// XOR 複数直線 (連続直線/集中線)
//========================================
/*
	pttmp[0] : 始点
	pttmp[1] : 終点 (現在位置)
	pttmp[2] : 一番最初の点 (保存用)
*/


/** ２回目以降の押し時 */

static void _xorsumline_pressInGrab(DrawData *p,uint32_t state)
{
	drawOpXor_drawline(p);

	drawOpDraw_brush_lineSuccConc(p);

	//連続直線の場合、始点変更

	if(p->w.opsubtype == DRAW_OPSUB_DRAW_SUCCLINE)
		p->w.pttmp[0] = p->w.pttmp[1];

	drawOpXor_drawline(p);
}

/** 各アクション (操作終了) */

static mBool _xorsumline_action(DrawData *p,int action)
{
	//BackSpace は連続直線のみ

	if(action == DRAW_FUNCACTION_KEY_BACKSPACE
		&& p->w.opsubtype != DRAW_OPSUB_DRAW_SUCCLINE)
		return FALSE;

	//

	drawOpXor_drawline(p);

	//BackSpace (連続直線) => 始点と結ぶ

	if(action == DRAW_FUNCACTION_KEY_BACKSPACE)
	{
		p->w.pttmp[1] = p->w.pttmp[2];

		drawOpDraw_brush_lineSuccConc(p);
	}

	drawOpSub_finishDraw_workrect(p);

	return TRUE;
}

/** 複数直線の初期化 (多角形と共用) */

static void _xorsumline_init(DrawData *p,int optype,int opsubtype)
{
	drawOpSub_setOpInfo(p, optype,
		_xorline_motion, drawOp_common_norelease, opsubtype);

	p->w.opflags |= DRAW_OPFLAGS_MOTION_POS_INT;

	drawOpSub_getAreaPoint_int(p, p->w.pttmp);

	drawOpSub_copyTmpPoint(p, 2);

	drawOpXor_drawline(p);
}

/** 押し */

mBool drawOpXor_sumline_press(DrawData *p,int opsubtype)
{
	_xorsumline_init(p, DRAW_OPTYPE_XOR_SUMLINE, opsubtype);

	p->w.funcPressInGrab = _xorsumline_pressInGrab;
	p->w.funcAction = _xorsumline_action;

	//描画開始
	
	drawOpSub_setDrawInfo(p, p->w.optoolno, 0);
	drawOpSub_beginDraw(p);

	return TRUE;
}


//=================================
// 多角形
//=================================
/*
	確定した線は XOR 用イメージに描画。
	現在の線はキャンバスに直接 XOR 描画。

	pttmp などは連続直線/集中線と同じ。
	boxtmp[0] : XOR 描画用範囲
*/


/** ２回目以降の押し時 */

static void _xorpolygon_pressInGrab(DrawData *p,uint32_t state)
{
	mDoublePoint pt;

	//ポイント追加

	drawCalc_areaToimage_double(p, &pt.x, &pt.y, p->w.pttmp[1].x, p->w.pttmp[1].y);

	FillPolygon_addPoint(p->w.fillpolygon, pt.x, pt.y);

	//XOR

	drawOpXor_drawline(p);
	drawOpXor_drawPolygon(p);

	TileImage_drawLineB(p->tileimgTmp, p->w.pttmp[0].x, p->w.pttmp[0].y,
		p->w.pttmp[1].x, p->w.pttmp[1].y, &p->w.rgbaDraw, FALSE);

	drawOpXor_drawPolygon(p);

	p->w.pttmp[0] = p->w.pttmp[1];
	
	drawOpXor_drawline(p);
}

/** 各アクション (操作終了) */

static mBool _xorpolygon_action(DrawData *p,int action)
{
	//BackSpace 無効

	if(action == DRAW_FUNCACTION_KEY_BACKSPACE) return FALSE;

	//XOR 消去

	drawOpXor_drawline(p);
	drawOpXor_drawPolygon(p);

	drawOpSub_freeTmpImage(p);

	//描画

	if(action != DRAW_FUNCACTION_KEY_ESC
		&& action != DRAW_FUNCACTION_UNGRAB)
	{
		if(FillPolygon_closePoint(p->w.fillpolygon))
		{
			switch(p->w.opsubtype)
			{
				//多角形塗りつぶし
				case DRAW_OPSUB_DRAW_FILL:
					drawOpDraw_fillPolygon(p);
					break;
				//スタンプイメージセット
				case DRAW_OPSUB_SET_STAMP:
					drawOp_setStampImage(p);
					break;
				//選択範囲セット
				case DRAW_OPSUB_SET_SELECT:
					drawOp_setSelect(p, 1);
					break;
			}
		}
	}

	drawOpSub_freeFillPolygon(p);

	return TRUE;
}

/** 多角形初期化 (投げ縄と共用) */

static mBool _fillpolygon_init(DrawData *p)
{
	mDoublePoint pt;

	if(!drawOpSub_createTmpImage_area(p))
		return FALSE;

	//FillPolygon 作成

	if(!(p->w.fillpolygon = FillPolygon_new()))
	{
		drawOpSub_freeTmpImage(p);
		return FALSE;
	}

	//現在位置追加

	drawOpSub_getImagePoint_double(p, &pt);

	FillPolygon_addPoint(p->w.fillpolygon, pt.x, pt.y);

	//

	g_tileimage_dinfo.funcDrawPixel = TileImage_setPixel_new_notp;

	p->w.rgbaDraw.a = 0x8000;

	return TRUE;
}

/** 最初の押し時 */

mBool drawOpXor_polygon_press(DrawData *p,int opsubtype)
{
	if(!_fillpolygon_init(p)) return FALSE;

	//XOR 更新範囲

	p->w.boxtmp[0].x = p->w.boxtmp[0].y = 0;
	p->w.boxtmp[0].w = p->szCanvas.w;
	p->w.boxtmp[0].h = p->szCanvas.h;

	//

	_xorsumline_init(p, DRAW_OPTYPE_XOR_POLYGON, opsubtype);

	p->w.funcPressInGrab = _xorpolygon_pressInGrab;
	p->w.funcAction = _xorpolygon_action;

	return TRUE;
}


//=============================
// XOR 投げ縄
//=============================
/*
	pttmp[0] : 線描画用、現在の位置
	pttmp[1] : 前回の位置
*/


/** 移動 */

static void _xorlasso_motion(DrawData *p,uint32_t state)
{
	mDoublePoint pt;

	p->w.pttmp[1] = p->w.pttmp[0];

	drawOpSub_getAreaPoint_int(p, p->w.pttmp);

	drawOpXor_drawLasso(p, FALSE);

	//ポイント追加

	drawOpSub_getImagePoint_double(p, &pt);
	FillPolygon_addPoint(p->w.fillpolygon, pt.x, pt.y);
}

/** 離し */

static mBool _xorlasso_release(DrawData *p)
{
	drawOpXor_drawLasso(p, TRUE);
	drawOpSub_freeTmpImage(p);

	if(FillPolygon_closePoint(p->w.fillpolygon))
	{
		switch(p->w.opsubtype)
		{
			//フリーハンド塗りつぶし
			case DRAW_OPSUB_DRAW_FILL:
				drawOpDraw_fillPolygon(p);
				break;
			//スタンプイメージセット
			case DRAW_OPSUB_SET_STAMP:
				drawOp_setStampImage(p);
				break;
			//選択範囲セット
			case DRAW_OPSUB_SET_SELECT:
				drawOp_setSelect(p, 1);
				break;
		}
	}

	drawOpSub_freeFillPolygon(p);

	return TRUE;
}

/** 各アクション (キーなどの操作は無効) */

static mBool _xorlasso_action(DrawData *p,int action)
{
	if(action == DRAW_FUNCACTION_UNGRAB)
		return _xorlasso_release(p);
	else
		return FALSE;
}

/** 押し時 */

mBool drawOpXor_lasso_press(DrawData *p,int opsubtype)
{
	if(!_fillpolygon_init(p)) return FALSE;

	drawOpSub_setOpInfo(p, DRAW_OPTYPE_XOR_LASSO,
		_xorlasso_motion, _xorlasso_release, opsubtype);

	p->w.funcAction = _xorlasso_action;

	drawOpSub_getAreaPoint_int(p, p->w.pttmp);

	drawOpSub_copyTmpPoint(p, 2);

	drawOpXor_drawLasso(p, FALSE);

	return TRUE;
}


//=========================================
// 定規の位置設定 (集中線、同心円[正円])
//=========================================
/*
	pttmp[0] : 押し位置
*/


/** 離し */

static mBool _rulepoint_release(DrawData *p)
{
	drawOpXor_drawCrossPoint(p);

	return TRUE;
}

/** 押し */

mBool drawOpXor_rulepoint_press(DrawData *p)
{
	drawOpSub_setOpInfo(p, DRAW_OPTYPE_GENERAL,
		NULL, _rulepoint_release, 0);

	drawOpSub_getAreaPoint_int(p, p->w.pttmp);

	drawOpXor_drawCrossPoint(p);

	return TRUE;
}


/********************************
 * XOR 描画
 ********************************/


/** XOR 直線描画
 *
 * pttmp[0] が始点、pttmp[1] が終点 */

void drawOpXor_drawline(DrawData *p)
{
	mPixbuf *pixbuf;
	mBox box;

	if((pixbuf = drawOpSub_beginAreaDraw()))
	{
		mPixbufLine(pixbuf, p->w.pttmp[0].x, p->w.pttmp[0].y,
			p->w.pttmp[1].x, p->w.pttmp[1].y, MPIXBUF_COL_XOR);

		mBoxSetByPoint(&box, p->w.pttmp, p->w.pttmp + 1);

		drawOpSub_endAreaDraw(pixbuf, &box);
	}
}

/** XOR 四角形描画 (領域) */

void drawOpXor_drawBox_area(DrawData *p)
{
	mPixbuf *pixbuf;
	mBox box;

	if((pixbuf = drawOpSub_beginAreaDraw()))
	{
		box.x = p->w.pttmp[0].x;
		box.y = p->w.pttmp[0].y;
		box.w = p->w.pttmp[1].x - p->w.pttmp[0].x + 1;
		box.h = p->w.pttmp[1].y - p->w.pttmp[0].y + 1;

		mPixbufBoxSlow(pixbuf, box.x, box.y, box.w, box.h, MPIXBUF_COL_XOR);

		drawOpSub_endAreaDraw(pixbuf, &box);
	}
}

/** XOR 四角形描画 (イメージに対する) */

void drawOpXor_drawBox_image(DrawData *p)
{
	int x1,y1,x2,y2,i;
	mPoint pt[5];
	mPixbuf *pixbuf;
	mBox box;

	if((pixbuf = drawOpSub_beginAreaDraw()))
	{
		x1 = p->w.pttmp[0].x, y1 = p->w.pttmp[0].y;
		x2 = p->w.pttmp[1].x, y2 = p->w.pttmp[1].y;

		if(p->canvas_zoom != 1000)
			x2++, y2++;

		//イメージ -> 領域座標

		drawCalc_imageToarea_pt(p, pt, x1, y1);
		drawCalc_imageToarea_pt(p, pt + 1, x2, y1);
		drawCalc_imageToarea_pt(p, pt + 2, x2, y2);
		drawCalc_imageToarea_pt(p, pt + 3, x1, y2);
		pt[4] = pt[0];

		//更新範囲

		mBoxSetByPoints(&box, pt, 4);

		//描画

		if(x1 == x2 && y1 == y2)
			mPixbufSetPixel(pixbuf, pt[0].x, pt[0].y, MPIXBUF_COL_XOR);
		else if(x1 == x2 || y1 == y2)
			mPixbufLine(pixbuf, pt[0].x, pt[0].y, pt[2].x, pt[2].y, MPIXBUF_COL_XOR);
		else
		{
			for(i = 0; i < 4; i++)
				mPixbufLine_noend(pixbuf, pt[i].x, pt[i].y, pt[i + 1].x, pt[i + 1].y, MPIXBUF_COL_XOR);
		}

		drawOpSub_endAreaDraw(pixbuf, &box);
	}
}

/** XOR 円描画
 *
 * pttmp[0] が中心、pttmp[1] が半径(x,y) */

void drawOpXor_drawEllipse(DrawData *p)
{
	mPixbuf *pixbuf;
	mBox box;
	int x1,y1,x2,y2;

	if((pixbuf = drawOpSub_beginAreaDraw()))
	{
		x1 = p->w.pttmp[0].x - p->w.pttmp[1].x;
		y1 = p->w.pttmp[0].y - p->w.pttmp[1].y;
		x2 = p->w.pttmp[0].x + p->w.pttmp[1].x;
		y2 = p->w.pttmp[0].y + p->w.pttmp[1].y;

		mPixbufEllipse(pixbuf, x1, y1, x2, y2, MPIXBUF_COL_XOR);

		box.x = x1, box.y = y1;
		box.w = x2 - x1 + 1, box.h = y2 - y1 + 1;
		
		drawOpSub_endAreaDraw(pixbuf, &box);
	}
}

/** XOR ベジェ曲線描画 */

void drawOpXor_drawBezier(DrawData *p,mBool erase)
{
	mPixbuf *pixbuf;

	//描画

	if(!erase)
	{
		TileImage_freeAllTiles(p->tileimgTmp);

		TileImageDrawInfo_clearDrawRect();
		TileImage_drawBezier_forXor(p->tileimgTmp, p->w.pttmp, p->w.opsubtype);

		drawCalc_clipArea_toBox(p, p->w.boxtmp, &g_tileimage_dinfo.rcdraw);
	}

	//XOR

	if((pixbuf = drawOpSub_beginAreaDraw()))
	{
		TileImage_blendXorToPixbuf(p->tileimgTmp, pixbuf, p->w.boxtmp);

		drawOpSub_endAreaDraw(pixbuf, p->w.boxtmp);
	}
}

/** XOR 多角形描画 */

void drawOpXor_drawPolygon(DrawData *p)
{
	mPixbuf *pixbuf;

	if((pixbuf = drawOpSub_beginAreaDraw()))
	{
		TileImage_blendXorToPixbuf(p->tileimgTmp, pixbuf, p->w.boxtmp);

		drawOpSub_endAreaDraw(pixbuf, p->w.boxtmp);
	}
}

/** XOR 投げ縄描画 */

void drawOpXor_drawLasso(DrawData *p,mBool erase)
{
	mPixbuf *pixbuf;
	mBox box;
	mRect rc;

	if((pixbuf = drawOpSub_beginAreaDraw()))
	{
		if(erase)
		{
			//消去

			box.x = box.y = 0;
			box.w = p->szCanvas.w, box.h = p->szCanvas.h;

			TileImage_blendXorToPixbuf(p->tileimgTmp, pixbuf, &box);
		}
		else
		{
			//新しい線描画

			mRectSetByPoint_minmax(&rc, p->w.pttmp, p->w.pttmp + 1);

			if(drawCalc_clipArea_toBox(p, &box, &rc))
			{
				TileImage_blendXorToPixbuf(p->tileimgTmp, pixbuf, &box);

				TileImage_drawLineB(p->tileimgTmp, p->w.pttmp[1].x, p->w.pttmp[1].y,
					p->w.pttmp[0].x, p->w.pttmp[0].y, &p->w.rgbaDraw, FALSE);

				TileImage_blendXorToPixbuf(p->tileimgTmp, pixbuf, &box);
			}
		}

		drawOpSub_endAreaDraw(pixbuf, &box);
	}
}

/** XOR 十字線描画 */

void drawOpXor_drawCrossPoint(DrawData *p)
{
	mPixbuf *pixbuf;
	int x,y;
	mBox box;

	if((pixbuf = drawOpSub_beginAreaDraw()))
	{
		x = p->w.pttmp[0].x;
		y = p->w.pttmp[0].y;
	
		mPixbufLineH(pixbuf, x - 9, y, 19, MPIXBUF_COL_XOR);
		mPixbufLineV(pixbuf, x, y - 9, 19, MPIXBUF_COL_XOR);

		box.x = x - 9, box.y = y - 9;
		box.w = box.h = 19;

		drawOpSub_endAreaDraw(pixbuf, &box);
	}
}

/** XOR ブラシサイズ円描画 */

/* pttmp[0] : 中心位置
 * ntmp[0]  : 半径 px サイズ */

void drawOpXor_drawBrushSizeCircle(DrawData *p,mBool erase)
{
	mPixbuf *pixbuf;
	mBox box;
	int r;

	if(erase)
		box = p->w.boxtmp[0];
	else
	{
		r = p->w.ntmp[0];
		
		box.x = p->w.pttmp[0].x - r;
		box.y = p->w.pttmp[0].y - r;
		box.w = box.h = (r << 1) + 1;

		p->w.boxtmp[0] = box;
	}

	//描画

	if((pixbuf = drawOpSub_beginAreaDraw()))
	{
		mPixbufEllipse(pixbuf,
			box.x, box.y, box.x + box.w - 1, box.y + box.h - 1, MPIXBUF_COL_XOR);

		drawOpSub_endAreaDraw(pixbuf, &box);
	}
}
