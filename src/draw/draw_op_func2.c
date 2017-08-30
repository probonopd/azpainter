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
 * 操作 - いろいろ2
 *****************************************/
/*
 * 選択範囲、矩形範囲
 */

#include <stdlib.h>	//abs
#include <math.h>

#include "mDef.h"
#include "mRectBox.h"

#include "defDraw.h"

#include "draw_main.h"
#include "draw_calc.h"
#include "draw_select.h"
#include "draw_boxedit.h"
#include "draw_op_def.h"
#include "draw_op_sub.h"
#include "draw_op_func.h"

#include "macroToolOpt.h"

#include "TileImage.h"
#include "TileImageDrawInfo.h"
#include "LayerItem.h"
#include "LayerList.h"

#include "FillPolygon.h"
#include "DrawFill.h"

#include "AppCursor.h"
#include "MainWinCanvas.h"



//==================================
// 選択範囲 : 図形選択
//==================================
/*
 * - 選択範囲イメージは tileimgSel (1bit)
 */


/** 四角形の場合 多角形の点セット */

static mBool _set_drawbox_polygon(DrawData *p)
{
	mDoublePoint pt[4];
	int i;

	drawOpSub_getDrawBoxPoints(p, pt);

	p->w.fillpolygon = FillPolygon_new();
	if(!p->w.fillpolygon) return FALSE;
	
	for(i = 0; i < 4; i++)
		FillPolygon_addPoint(p->w.fillpolygon, pt[i].x, pt[i].y);

	return FillPolygon_closePoint(p->w.fillpolygon);
}

/** 選択範囲を図形からセット
 *
 * @param type 0:四角形 1:多角形またはフリーハンド */

void drawOp_setSelect(DrawData *p,int type)
{
	mBool del;

	if(!drawSel_createImage(p)) return;

	MainWinCanvasArea_setCursor_wait();

	drawOpSub_setDrawInfo_select();

	//範囲削除か (+Ctrl)

	del = (p->w.press_state & M_MODS_CTRL);

	if(del) p->w.rgbaDraw.a = 0;

	//描画

	TileImageDrawInfo_clearDrawRect();

	if(type == 0 && p->canvas_angle == 0)
	{
		//------ 四角形、キャンバス回転なしの場合

		mBox box;
		
		drawOpSub_getDrawBox_noangle(p, &box);

		TileImage_drawFillBox(p->tileimgSel, box.x, box.y, box.w, box.h, &p->w.rgbaDraw);
	}
	else
	{
		//----- 多角形

		mBool draw = TRUE;

		//四角形 (回転あり) の場合

		if(type == 0)
			draw = _set_drawbox_polygon(p);

		//描画

		if(draw)
		{
			TileImage_drawFillPolygon(p->tileimgSel, p->w.fillpolygon,
				&p->w.rgbaDraw, FALSE);
		}

		drawOpSub_freeFillPolygon(p);
	}

	//

	if(del)
		//範囲削除
		drawSel_freeEmpty(p);
	else
		//範囲追加
		mRectUnion(&p->sel.rcsel, &g_tileimage_dinfo.rcdraw);

	//描画部分を更新

	drawUpdate_rect_canvas_forSelect(p, &g_tileimage_dinfo.rcdraw);

	MainWinCanvasArea_restoreCursor();
}


//=================================
// 自動選択
//=================================


/** 離し */

static mBool _magicwand_release(DrawData *p)
{
	DrawFill *draw;
	LayerItem *item;
	mPoint pt;
	int type,diff;
	uint32_t val;
	mBool disable_ref;

	//設定値

	val = p->tool.opt_magicwand;

	type = MAGICWAND_GET_TYPE(val);
	diff = MAGICWAND_GET_COLOR_DIFF(val);

	if(p->w.press_state & M_MODS_CTRL)	//+Ctrl : 透明で判定
		type = DRAWFILL_TYPE_AUTO_ANTIALIAS;

	if(p->w.press_state & M_MODS_SHIFT) //+Shift : 判定元無効
		disable_ref = TRUE;
	else
		disable_ref = MAGICWAND_IS_DISABLE_REF(val);

	//判定元イメージリンクセット & 先頭を取得
	/* 判定元指定がなく、カレントがフォルダの場合は item == NULL */

	item = LayerList_setLink_filltool(p->layerlist, p->curlayer, disable_ref);

	if(!item) return TRUE;

	//イメージ作成

	if(!drawSel_createImage(p)) return TRUE;
	
	//塗りつぶし初期化

	drawOpSub_getImagePoint_int(p, &pt);

	draw = DrawFill_new(p->tileimgSel, item->img, &pt, type, diff, 100);
	if(!draw) return TRUE;

	//準備

	MainWinCanvasArea_setCursor_wait();

	drawOpSub_setDrawInfo_select();

	TileImageDrawInfo_clearDrawRect();

	//塗りつぶし実行

	DrawFill_run(draw, &p->w.rgbaDraw);
	DrawFill_free(draw);

	//範囲追加

	mRectUnion(&p->sel.rcsel, &g_tileimage_dinfo.rcdraw);

	//更新

	drawUpdate_rect_canvas_forSelect(p, &g_tileimage_dinfo.rcdraw);

	//

	MainWinCanvasArea_restoreCursor();

	return TRUE;
}

/** 押し */

mBool drawOp_magicwand_press(DrawData *p)
{
	drawOpSub_setOpInfo(p, DRAW_OPTYPE_GENERAL, NULL, _magicwand_release, 0);

	return TRUE;
}


//==================================
// 選択範囲 : 範囲の位置移動
//==================================
/*
	pttmp[0] : 開始時のオフセット位置
	pttmp[1]  : オフセット位置の総相対移動数
	ptd_tmp[0] : 総相対移動 px 数 (キャンバス座標)
	rcdraw    : 更新範囲 (タイマー用)
*/


/** 移動 */

static void _selmove_motion(DrawData *p,uint32_t state)
{
	mPoint pt;
	mRect rc;

	if(drawCalc_moveImage_onMotion(p, p->tileimgSel, state, &pt))
	{
		//オフセット位置移動

		TileImage_moveOffset_rel(p->tileimgSel, pt.x, pt.y);

		//更新範囲 (移動前+移動後)

		drawCalc_unionRect_relmove(&rc, &p->sel.rcsel, pt.x, pt.y);

		//rcsel 移動

		mRectRelMove(&p->sel.rcsel, pt.x, pt.y);

		//更新

		mRectUnion(&p->w.rcdraw, &rc);

		MainWinCanvasArea_setTimer_updateSelectMove();
	}
}

/** 離し */

static mBool _selmove_release(DrawData *p)
{
	//タイマークリア&更新

	MainWinCanvasArea_clearTimer_updateSelectMove();

	//カーソル

	MainWinCanvasArea_setCursor_forTool();

	return TRUE;
}

/** 押し時 */

mBool drawOp_selmove_press(DrawData *p)
{
	if(!drawSel_isHave()) return FALSE;

	drawOpSub_setOpInfo(p, DRAW_OPTYPE_GENERAL,
		_selmove_motion, _selmove_release, 0);

	drawOpSub_startMoveImage(p, p->tileimgSel);

	MainWinCanvasArea_setCursor(APP_CURSOR_SEL_MOVE);

	return TRUE;
}


//=========================================
// 選択範囲 : 範囲内イメージの移動/コピー
//=========================================
/*
	pttmp[0] : 開始時のオフセット位置
	pttmp[1]  : オフセット位置の総相対移動数
	ptd_tmp[0] : 総相対移動 px 数 (キャンバス座標)
	rcdraw    : 更新範囲 (タイマー用)
*/


/** 移動 */

static void _selimgmove_motion(DrawData *p,uint32_t state)
{
	mPoint pt;
	mRect rc;

	if(drawCalc_moveImage_onMotion(p, p->tileimgSel, 0, &pt))
	{
		//オフセット位置移動

		TileImage_moveOffset_rel(p->tileimgSel, pt.x, pt.y);
		TileImage_moveOffset_rel(p->tileimgTmp, pt.x, pt.y);
		
		//更新範囲 (移動前+移動後)

		drawCalc_unionRect_relmove(&rc, &p->sel.rcsel, pt.x, pt.y);

		//rcsel 移動

		mRectRelMove(&p->sel.rcsel, pt.x, pt.y);

		//更新

		mRectUnion(&p->w.rcdraw, &rc);

		MainWinCanvasArea_setTimer_updateSelectImageMove();
	}
}

/** 離し */

static mBool _selimgmove_release(DrawData *p)
{
	mBool update,hidesel;

	//枠非表示戻す

	hidesel = p->w.hide_canvas_select;

	p->w.hide_canvas_select = FALSE;

	//タイマークリア&更新

	update = MainWinCanvasArea_clearTimer_updateSelectImageMove();

	//枠非表示で、タイマークリア時に更新されていない場合、枠が表示されるよう再描画

	if(hidesel && !update)
		drawUpdate_rect_imgcanvas_fromRect(p, &p->sel.rcsel);

	//貼り付け (+Shift で上書き)

	drawOpSub_setDrawInfo_select_paste();

	if(p->w.press_state & M_MODS_SHIFT)
		g_tileimage_dinfo.funcColor = TileImage_colfunc_overwrite;

	TileImage_pasteSelImage(p->w.dstimg, p->tileimgTmp, &p->sel.rcsel);

	//作業用イメージ解放
	/* 更新時、tileimgTmp == NULL で通常更新となるので、
	 * 更新を行う前に解放すること。 */

	drawOpSub_freeTmpImage(p);

	//更新
	/* g_tileimg_dinfo.rcdraw の範囲で更新。
	 * 切り取り時は、切り取られた範囲+貼り付け範囲 になっている。 */

	drawOpSub_finishDraw_single(p);

	//カーソル

	MainWinCanvasArea_setCursor_forTool();

	return TRUE;
}

/** 押し時 */

mBool drawOp_selimgmove_press(DrawData *p,mBool cut)
{
	if(!drawSel_isHave()
		|| drawOpSub_canDrawLayer_mes(p)
		|| !drawOpSub_isPressInSelect(p))
		return FALSE;

	//描画準備 (切り取り、貼り付け用)

	drawOpSub_setDrawInfo(p, -1, 0);
	drawOpSub_clearDrawMasks();

	g_tileimage_dinfo.funcColor = TileImage_colfunc_overwrite;

	drawOpSub_beginDraw(p);

	//範囲内イメージ作成

	p->tileimgTmp = TileImage_createSelCopyImage(p->curlayer->img,
		p->tileimgSel, &p->sel.rcsel, cut, TRUE);

	if(!p->tileimgTmp) return FALSE;

	//

	drawOpSub_setOpInfo(p, DRAW_OPTYPE_SELIMGMOVE,
		_selimgmove_motion, _selimgmove_release, 0);

	drawOpSub_startMoveImage(p, p->tileimgSel);

	MainWinCanvasArea_setCursor(APP_CURSOR_SEL_MOVE);

	//+Ctrl で枠非表示

	if(p->w.press_state & M_MODS_CTRL)
		p->w.hide_canvas_select = TRUE;

	return TRUE;
}


//=============================
// 矩形編集 : 範囲リサイズ
//=============================
/*
 * pttmp[0] : 左上位置
 * pttmp[1] : 右下位置
 * pttmp[2] : 前回のイメージ位置
 */


enum
{
	_BOXEDIT_EDGE_LEFT   = 1<<0,
	_BOXEDIT_EDGE_TOP    = 1<<1,
	_BOXEDIT_EDGE_RIGHT  = 1<<2,
	_BOXEDIT_EDGE_BOTTOM = 1<<3
};


/** 非操作時の移動時 : カーソル変更 */

void drawOp_boxedit_nograb_motion(DrawData *p)
{
	mPoint pt;
	mRect rc,rcsel;
	int w,pos,no;

	//カーソル下のイメージ位置

	drawOpSub_getImagePoint_int(p, &pt);

	//現在の選択範囲
	/* 表示枠は外側にはみ出している形なので、
	 * そのままの範囲だと、少し枠の内側で判定される感じになる。
	 * なので、範囲の座標を少し大きめにしておく。 */

	mRectSetByBox(&rcsel, &p->boxedit.box);

	rcsel.x1--;
	rcsel.y1--;
	rcsel.x2 += 2;
	rcsel.y2 += 2;

	//カーソルと範囲の各辺との距離

	rc.x1 = abs(rcsel.x1 - pt.x);
	rc.y1 = abs(rcsel.y1 - pt.y);
	rc.x2 = abs(rcsel.x2 - pt.x);
	rc.y2 = abs(rcsel.y2 - pt.y);

	//判定の半径幅 (イメージの px 単位)

	if(p->canvas_zoom >= 2000)
		w = 2;
	else
		w = (int)(p->viewparam.scalediv * 5 + 0.5);

	//カーソルに近い辺と四隅を判定

	if(rc.x1 < w && rc.y1 < w)
		pos = _BOXEDIT_EDGE_LEFT | _BOXEDIT_EDGE_TOP; //左上
	else if(rc.x1 < w && rc.y2 < w)
		pos = _BOXEDIT_EDGE_LEFT | _BOXEDIT_EDGE_BOTTOM; //左下
	else if(rc.x2 < w && rc.y1 < w)
		pos = _BOXEDIT_EDGE_RIGHT | _BOXEDIT_EDGE_TOP; //右上
	else if(rc.x2 < w && rc.y2 < w)
		pos = _BOXEDIT_EDGE_RIGHT | _BOXEDIT_EDGE_BOTTOM; //右下
	else if(rc.x1 < w && rcsel.y1 <= pt.y && pt.y <= rcsel.y2)
		pos = _BOXEDIT_EDGE_LEFT;	//左
	else if(rc.x2 < w && rcsel.y1 <= pt.y && pt.y <= rcsel.y2)
		pos = _BOXEDIT_EDGE_RIGHT;	//右
	else if(rc.y1 < w && rcsel.x1 <= pt.x && pt.x <= rcsel.x2)
		pos = _BOXEDIT_EDGE_TOP;	//上
	else if(rc.y2 < w && rcsel.x1 <= pt.x && pt.x <= rcsel.x2)
		pos = _BOXEDIT_EDGE_BOTTOM;	//下
	else
		pos = 0;

	//カーソル変更

	if(pos != p->boxedit.cursor_resize)
	{
		p->boxedit.cursor_resize = pos;

		if(pos == 0)
			no = APP_CURSOR_SELECT;
		else if(pos == (_BOXEDIT_EDGE_LEFT | _BOXEDIT_EDGE_TOP)
			|| pos == (_BOXEDIT_EDGE_RIGHT | _BOXEDIT_EDGE_BOTTOM))
			no = APP_CURSOR_LEFT_TOP;
		else if(pos == (_BOXEDIT_EDGE_LEFT | _BOXEDIT_EDGE_BOTTOM)
			|| pos == (_BOXEDIT_EDGE_RIGHT | _BOXEDIT_EDGE_TOP))
			no = APP_CURSOR_RIGHT_TOP;
		else if(pos == _BOXEDIT_EDGE_LEFT || pos == _BOXEDIT_EDGE_RIGHT)
			no = APP_CURSOR_RESIZE_HORZ;
		else
			no = APP_CURSOR_RESIZE_VERT;

		MainWinCanvasArea_setCursor(no);
	}
}

/** 移動 */

static void _boxedit_resize_motion(DrawData *p,uint32_t state)
{
	mPoint pt;
	int flags,n,mx,my;

	//移動数

	drawOpSub_getImagePoint_int(p, &pt);

	mx = pt.x - p->w.pttmp[2].x;
	my = pt.y - p->w.pttmp[2].y;

	if(mx == 0 && my == 0) return;

	//移動

	drawOpXor_drawBox_image(p);

	flags = p->boxedit.cursor_resize;

	if(flags & _BOXEDIT_EDGE_LEFT)
	{
		//左

		n = p->w.pttmp[0].x + mx;

		if(n < 0) n = 0;
		else if(n > p->w.pttmp[1].x) n = p->w.pttmp[1].x;

		p->w.pttmp[0].x = n;
	}
	else if(flags & _BOXEDIT_EDGE_RIGHT)
	{
		//右

		n = p->w.pttmp[1].x + mx;

		if(n < p->w.pttmp[0].x) n = p->w.pttmp[0].x;
		else if(n >= p->imgw) n = p->imgw - 1;

		p->w.pttmp[1].x = n;
	}

	if(flags & _BOXEDIT_EDGE_TOP)
	{
		//上

		n = p->w.pttmp[0].y + my;

		if(n < 0) n = 0;
		else if(n > p->w.pttmp[1].y) n = p->w.pttmp[1].y;

		p->w.pttmp[0].y = n;
	}
	else if(flags & _BOXEDIT_EDGE_BOTTOM)
	{
		//下

		n = p->w.pttmp[1].y + my;

		if(n < p->w.pttmp[0].y) n = p->w.pttmp[0].y;
		else if(n >= p->imgh) n = p->imgh - 1;

		p->w.pttmp[1].y = n;
	}

	drawOpXor_drawBox_image(p);

	//

	p->w.pttmp[2] = pt;
}

/** 離し */

static mBool _boxedit_resize_release(DrawData *p)
{
	mBox *boxdst = &p->boxedit.box;

	drawOpXor_drawBox_image(p);

	//範囲セット

	boxdst->x = p->w.pttmp[0].x;
	boxdst->y = p->w.pttmp[0].y;
	boxdst->w = p->w.pttmp[1].x - p->w.pttmp[0].x + 1;
	boxdst->h = p->w.pttmp[1].y - p->w.pttmp[0].y + 1;

	//更新

	drawUpdate_rect_canvas_forBoxEdit(p, boxdst);

	//リサイズカーソル再判定

	drawOp_boxedit_nograb_motion(p);

	return TRUE;
}

/** 矩形範囲リサイズ、押し時 */

static mBool _boxedit_resize_press(DrawData *p)
{
	mBox box;

	drawOpSub_setOpInfo(p, DRAW_OPTYPE_GENERAL,
		_boxedit_resize_motion, _boxedit_resize_release, 0);

	//

	box = p->boxedit.box;

	p->w.pttmp[0].x = box.x;
	p->w.pttmp[0].y = box.y;
	p->w.pttmp[1].x = box.x + box.w - 1;
	p->w.pttmp[1].y = box.y + box.h - 1;

	drawOpSub_getImagePoint_int(p, &p->w.pttmp[2]);

	//枠を消して、XOR 描画

	p->boxedit.box.w = 0;

	drawUpdate_rect_canvas_forBoxEdit(p, &box);
	
	drawOpXor_drawBox_image(p);

	return TRUE;
}


//=========================================
// 矩形範囲
//=========================================


/** 押し */

mBool drawOp_boxedit_press(DrawData *p)
{
	if(p->boxedit.cursor_resize)
		//リサイズ開始
		return _boxedit_resize_press(p);
	else
	{
		//範囲選択
		
		drawBoxEdit_setBox(p, NULL);

		return drawOpXor_boximage_press(p, DRAW_OPSUB_SET_BOXEDIT);
	}
}
