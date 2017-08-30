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
 * 操作 - いろいろ1
 *****************************************/
/*
 * 図形塗りつぶし、塗りつぶし、グラデーション、移動ツール、スタンプ
 * キャンバス移動、キャンバス回転、上下ドラッグキャンバス倍率変更
 * スポイト、中間色作成、色置き換え
 * テキスト描画
 */


#include <math.h>

#include "mDef.h"
#include "mRectBox.h"
#include "mNanoTime.h"
#include "mFont.h"
#include "mStr.h"

#include "defDraw.h"
#include "defWidgets.h"

#include "draw_main.h"
#include "draw_calc.h"
#include "draw_select.h"
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
#include "Undo.h"
#include "DrawFont.h"

#include "AppCursor.h"
#include "MainWinCanvas.h"
#include "Docks_external.h"


//----------------

mBool DrawTextDlg_run(mWindow *owner);

//----------------


//==============================
// 共通
//==============================


/** 離し : キャンバス状態変更用 */

static mBool _common_release_canvas(DrawData *p)
{
	MainWinCanvasArea_clearTimer_updateArea();

	drawCanvas_normalQuality();
	drawUpdate_canvasArea();
	
	return TRUE;
}

/** 離し : グラブ解除しない */

mBool drawOp_common_norelease(DrawData *p)
{
	return FALSE;
}


//=================================
// 描画
//=================================


/** 四角形塗りつぶし描画 (領域に対する) */

void drawOpDraw_fillBox(DrawData *p)
{
	if(p->canvas_angle == 0)
	{
		//キャンバス回転無しの場合

		mBox box;

		drawOpSub_getDrawBox_noangle(p, &box);

		drawOpSub_setDrawInfo(p, p->w.optoolno, 0);
		drawOpSub_beginDraw_single(p);

		TileImage_drawFillBox(p->w.dstimg, box.x, box.y, box.w, box.h, &p->w.rgbaDraw);
		
		drawOpSub_endDraw_single(p);
	}
	else
	{
		//キャンバス回転ありの場合、多角形で

		mDoublePoint pt[4];
		int i;

		drawOpSub_getDrawBoxPoints(p, pt);

		if((p->w.fillpolygon = FillPolygon_new()))
		{
			for(i = 0; i < 4; i++)
				FillPolygon_addPoint(p->w.fillpolygon, pt[i].x, pt[i].y);

			if(FillPolygon_closePoint(p->w.fillpolygon))
				drawOpDraw_fillPolygon(p);
		
			drawOpSub_freeFillPolygon(p);
		}
	}
}

/** 楕円塗りつぶし */

void drawOpDraw_fillEllipse(DrawData *p)
{
	mDoublePoint pt,pt_r;

	drawOpSub_getDrawEllipseParam(p, &pt, &pt_r);

	//描画

	drawOpSub_setDrawInfo(p, p->w.optoolno, 0);
	drawOpSub_beginDraw_single(p);

	TileImage_drawFillEllipse(p->w.dstimg, pt.x, pt.y, pt_r.x, pt_r.y,
		&p->w.rgbaDraw, p->w.ntmp[0],
		&p->viewparam, p->canvas_mirror);

	drawOpSub_endDraw_single(p);
}

/** 多角形/投げ縄 塗りつぶし */

void drawOpDraw_fillPolygon(DrawData *p)
{
	drawOpSub_setDrawInfo(p, p->w.optoolno, 0);
	drawOpSub_beginDraw_single(p);

	TileImage_drawFillPolygon(p->w.dstimg, p->w.fillpolygon,
		&p->w.rgbaDraw, p->w.ntmp[0]);

	drawOpSub_endDraw_single(p);
}

/** グラデーション描画 */

void drawOpDraw_gradation(DrawData *p)
{
	TileImageDrawGradInfo info;
	mPoint pt[2];
	mRect rc;
	void (*drawfunc[4])(TileImage *,int,int,int,int,mRect *,TileImageDrawGradInfo *) = {
		TileImage_drawGradation_line, TileImage_drawGradation_circle,
		TileImage_drawGradation_box, TileImage_drawGradation_radial
	};

	//描画用情報

	drawOpSub_setDrawGradationInfo(&info);
	if(!info.buf) return;

	//位置

	drawCalc_areaToimage_pt(p, pt, p->w.pttmp[0].x, p->w.pttmp[0].y);
	drawCalc_areaToimage_pt(p, pt + 1, p->w.pttmp[1].x, p->w.pttmp[1].y);

	//範囲 (選択範囲があるなら、その範囲)

	drawSel_getFullDrawRect(p, &rc);

	//描画

	drawOpSub_setDrawInfo(p, TOOL_GRADATION, 0);
	drawOpSub_beginDraw_single(p);

	(drawfunc[p->tool.subno[TOOL_GRADATION]])(p->w.dstimg,
		pt[0].x, pt[0].y, pt[1].x, pt[1].y, &rc, &info);

	mFree(info.buf);

	drawOpSub_endDraw_single(p);
}


//=================================
// 塗りつぶし
//=================================


/** 離し */

static mBool _fill_release(DrawData *p)
{
	DrawFill *draw;
	LayerItem *item;
	mPoint pt;
	int type,diff,opacity;
	uint32_t val;
	mBool disable_ref;

	//設定値

	if(p->w.optoolno == TOOL_FILL)
	{
		//通常塗りつぶし
		
		val = p->tool.opt_fill;

		type = FILL_GET_TYPE(val);
		diff = FILL_GET_COLOR_DIFF(val);
		opacity = FILL_GET_OPACITY(val);

		//+Ctrl でタイプをアンチエイリアス自動に

		if(p->w.press_state & M_MODS_CTRL)
			type = DRAWFILL_TYPE_AUTO_ANTIALIAS;

		if(p->w.press_state & M_MODS_SHIFT) //+Shift : 判定元無効
			disable_ref = TRUE;
		else
			disable_ref = FILL_IS_DISABLE_REF(val);
	}
	else
	{
		//不透明消去

		type = DRAWFILL_TYPE_OPAQUE;
		diff = 0;
		opacity = 100;
		disable_ref = TRUE;
	}

	//判定元イメージリンクセット & 先頭を取得

	item = LayerList_setLink_filltool(p->layerlist, p->curlayer, disable_ref);

	//初期化

	drawOpSub_getImagePoint_int(p, &pt);

	draw = DrawFill_new(p->curlayer->img, item->img, &pt, type, diff, opacity);
	if(!draw) return TRUE;

	//描画

	drawOpSub_setDrawInfo(p, p->w.optoolno, 0);
	drawOpSub_beginDraw_single(p);

	DrawFill_run(draw, &p->w.rgbaDraw);

	DrawFill_free(draw);

	drawOpSub_endDraw_single(p);

	return TRUE;
}

/** 押し */

mBool drawOp_fill_press(DrawData *p)
{
	if(drawOpSub_canDrawLayer_mes(p))
		//描画不可
		return FALSE;
	else
	{
		drawOpSub_setOpInfo(p, DRAW_OPTYPE_GENERAL, NULL, _fill_release, 0);
		return TRUE;
	}
}


//=============================
// スタンプ
//=============================


/** 範囲内イメージをスタンプ画像にセット (範囲選択後) */

void drawOp_setStampImage(DrawData *p)
{
	TileImage *img;
	mRect rc;

	//選択範囲イメージ作成

	img = drawOpSub_createSelectImage_byOpType(p, &rc);
	if(!img)
	{
		drawStamp_clearImage(p);
		return;
	}

	//範囲内のイメージをコピー

	TileImage_free(p->stamp.img);

	p->stamp.img = TileImage_createStampImage(p->curlayer->img, img, &rc);

	p->stamp.size.w = rc.x2 - rc.x1 + 1;
	p->stamp.size.h = rc.y2 - rc.y1 + 1;

	TileImage_free(img);

	//カーソル変更

	MainWinCanvasArea_setCursor(APP_CURSOR_STAMP);

	//プレビュー更新

	DockOption_changeStampImage();
}

/** スタンプイメージ貼り付け */

static mBool _stamp_cmd_paste(DrawData *p)
{
	mPoint pt;

	if(drawOpSub_canDrawLayer_mes(p)) return FALSE;

	//貼り付け位置

	drawOpSub_getImagePoint_int(p, &pt);

	if(!STAMP_IS_LEFTTOP(p->tool.opt_stamp))
	{
		pt.x -= p->stamp.size.w / 2;
		pt.y -= p->stamp.size.h / 2;
	}

	//描画

	drawOpSub_setDrawInfo(p, TOOL_STAMP, 0);
	drawOpSub_beginDraw(p);

	TileImage_pasteStampImage(p->w.dstimg, pt.x, pt.y, p->stamp.img,
		p->stamp.size.w, p->stamp.size.h);

	drawOpSub_finishDraw_single(p);

	//

	drawOpSub_setOpInfo(p, DRAW_OPTYPE_GENERAL, NULL, NULL, 0);

	return TRUE;
}

/** 押し */

mBool drawOp_stamp_press(DrawData *p,int subno)
{
	//+Ctrl : イメージをクリアして範囲選択へ

	if(p->w.press_state & M_MODS_CTRL)
		drawStamp_clearImage(p);

	//イメージがあれば貼付け、なければ範囲選択
	
	if(p->stamp.img)
		return _stamp_cmd_paste(p);
	else
	{
		//----- 範囲選択

		//カレントレイヤがフォルダの場合は無効

		if(drawOpSub_canDrawLayer(p) == CANDRAWLAYER_FOLDER)
			return FALSE;

		//各図形

		switch(subno)
		{
			//四角形
			case 0:
				return drawOpXor_boximage_press(p, DRAW_OPSUB_SET_STAMP);
			//多角形
			case 1:
				return drawOpXor_polygon_press(p, DRAW_OPSUB_SET_STAMP);
			//フリーハンド
			default:
				return drawOpXor_lasso_press(p, DRAW_OPSUB_SET_STAMP);
		}
	}
}


//==================================
// 移動ツール
//==================================
/*
	pttmp[0]  : 移動開始時の先頭レイヤのオフセット位置
	pttmp[1]  : オフセット位置の総相対移動数
	ptd_tmp[0] : 総相対移動 px 数 (キャンバス座標)
	rctmp[0]  : 現在の、対象レイヤすべての表示イメージ範囲
	rcdraw    : 更新範囲
	ptmp   : 移動対象レイヤアイテム (NULL ですべて)

	- フォルダレイヤ選択時は、フォルダ下すべて移動。
	- 移動対象のレイヤは DrawData::w.layer を先頭に LayerItem::link でリンクされている。
	  (フォルダ、ロックレイヤは含まれない)
*/


/** 移動 */

static void _movetool_motion(DrawData *p,uint32_t state)
{
	mPoint pt;
	mRect rc;
	LayerItem *pi;

	if(drawCalc_moveImage_onMotion(p, p->w.layer->img, state, &pt))
	{
		//オフセット位置移動

		for(pi = p->w.layer; pi; pi = pi->link)
			TileImage_moveOffset_rel(pi->img, pt.x, pt.y);

		//更新範囲
		/* rctmp[0] と、rctmp[0] を pt 分相対移動した範囲を合わせたもの */

		drawCalc_unionRect_relmove(&rc, p->w.rctmp, pt.x, pt.y);

		//現在のイメージ範囲移動

		mRectRelMove(p->w.rctmp, pt.x, pt.y);

		//更新
		/* タイマーで更新が行われると、rcdraw が空になる */

		mRectUnion(&p->w.rcdraw, &rc);

		MainWinCanvasArea_setTimer_updateMove();
	}
}

/** 離し */

static mBool _movetool_release(DrawData *p)
{
	mRect rc;

	if(p->w.pttmp[1].x || p->w.pttmp[1].y)
	{
		//undo

		Undo_addLayerMoveOffset(p->w.pttmp[1].x, p->w.pttmp[1].y, LAYERITEM(p->w.ptmp));

        //タイマークリア

        MainWinCanvasArea_clearTimer_updateMove();

        //未更新の範囲を処理

        drawUpdate_rect_imgcanvas_fromRect(p, &p->w.rcdraw);

        //キャンバスビュー更新
        /* 移動前の範囲 (p->w.rctmp[0] を総相対移動数分戻す)
         *  + 移動後の範囲 (p->w.rctmp[0]) */

        drawCalc_unionRect_relmove(&rc, p->w.rctmp, -(p->w.pttmp[1].x), -(p->w.pttmp[1].y));

        drawUpdate_canvasview(p, &rc);
	}

	return TRUE;
}

/** 押し */

mBool drawOp_movetool_press(DrawData *p)
{
	LayerItem *pi;
	int target;
	mPoint pt;

	target = p->tool.opt_move;

	//対象レイヤ (pi)

	if(target == 0)
		//カレントレイヤ
		pi = p->curlayer;
	else if(target == 2)
		//すべてのレイヤ
		pi = NULL;
	else
	{
		//掴んだレイヤ

		drawOpSub_getImagePoint_int(p, &pt);

		pi = LayerList_getItem_topPixelLayer(p->layerlist, pt.x, pt.y);
		if(!pi) return FALSE;
	}

	//移動対象レイヤ

	p->w.ptmp = pi;

	//レイヤのリンクセット & 先頭のレイヤ取得

	p->w.layer = LayerList_setLink_movetool(p->layerlist, pi);
	if(!p->w.layer) return FALSE;

	//

	drawOpSub_setOpInfo(p, DRAW_OPTYPE_GENERAL,
		_movetool_motion, _movetool_release, 0);

	//全体の表示イメージ範囲

	LayerItem_getVisibleImageRect_link(p->w.layer, &p->w.rctmp[0]);

	//作業用値、初期化

	drawOpSub_startMoveImage(p, p->w.layer->img);

	return TRUE;
}


//==============================
// キャンバス位置移動
//==============================
/*
	pttmp[0] : 開始時のスクロール位置
*/


/** 移動 */

static void _canvasmove_motion(DrawData *p,uint32_t state)
{
	mPoint pt;

	pt.x = p->w.pttmp[0].x + (int)(p->w.dptAreaPress.x - p->w.dptAreaCur.x);
	pt.y = p->w.pttmp[0].y + (int)(p->w.dptAreaPress.y - p->w.dptAreaCur.y);

	if(pt.x != p->w.pttmp[0].x || pt.y != p->w.pttmp[0].y)
	{
		p->ptScroll = pt;

		MainWinCanvas_setScrollPos();

		MainWinCanvasArea_setTimer_updateArea(5);
	}
}

/** 押し */

mBool drawOp_canvasMove_press(DrawData *p)
{
	drawOpSub_setOpInfo(p, DRAW_OPTYPE_GENERAL,
		_canvasmove_motion, _common_release_canvas, 0);

	p->w.opflags |= DRAW_OPFLAGS_MOTION_POS_INT;
	p->w.pttmp[0] = p->ptScroll;
	p->w.drag_cursor_type = APP_CURSOR_HAND_DRAG;

	drawCanvas_lowQuality();

	return TRUE;
}


//==============================
// キャンバス回転
//==============================
/*
	ntmp[0] : 現在の回転角度
	ntmp[1] : 前のキャンバス上の角度
*/


/** カーソルとキャンバス中央の角度取得 */

static int _get_canvasrotate_angle(DrawData *p)
{
	double x,y;

	x = p->w.dptAreaCur.x - p->szCanvas.w * 0.5;
	y = p->w.dptAreaCur.y - p->szCanvas.h * 0.5;

	return (int)(atan2(y, x) * 18000 / M_MATH_PI);
}

/** 移動 */

static void _canvasrotate_motion(DrawData *p,uint32_t state)
{
	int angle,n;

	angle = _get_canvasrotate_angle(p);

	//前回の角度からの移動分を加算

	n = p->w.ntmp[0] + angle - p->w.ntmp[1];

	if(n < -18000) n += 36000;
	else if(n > 18000) n -= 36000;

	p->w.ntmp[0] = n;
	p->w.ntmp[1] = angle;

	//45度補正

	if(state & M_MODS_SHIFT)
		n = n / 4500 * 4500;

	//更新

	if(n != p->canvas_angle)
	{
		drawCanvas_setZoomAndAngle(p, 0, n, 2 | DRAW_SETZOOMANDANGLE_F_NO_UPDATE, FALSE);

		MainWinCanvasArea_setTimer_updateArea(5);
	}
}

/** 押し */

mBool drawOp_canvasRotate_press(DrawData *p)
{
	drawOpSub_setOpInfo(p, DRAW_OPTYPE_GENERAL,
		_canvasrotate_motion, _common_release_canvas, 0);

	p->w.opflags |= DRAW_OPFLAGS_MOTION_POS_INT;
	p->w.ntmp[0] = p->canvas_angle;
	p->w.ntmp[1] = _get_canvasrotate_angle(p);
	p->w.drag_cursor_type = APP_CURSOR_ROTATE;

	drawCanvas_lowQuality();
	drawCanvas_setScrollReset_update(p, NULL);

	return TRUE;
}


//===============================
// 上下ドラッグでの表示倍率変更
//===============================
/*
	ntmp[0] : 開始時の表示倍率
*/


/** 移動 */

static void _canvaszoom_motion(DrawData *p,uint32_t state)
{
	int n;

	n = p->w.ntmp[0] + (int)(p->w.dptAreaPress.y - p->w.dptAreaCur.y) * 10;

	if(n != p->canvas_zoom)
	{
		drawCanvas_setZoomAndAngle(p, n, 0, 1 | DRAW_SETZOOMANDANGLE_F_NO_UPDATE, FALSE);

		MainWinCanvasArea_setTimer_updateArea(5);
	}
}

/** 押し */

mBool drawOp_canvasZoom_press(DrawData *p)
{
	drawOpSub_setOpInfo(p, DRAW_OPTYPE_GENERAL,
		_canvaszoom_motion, _common_release_canvas, 0);

	p->w.opflags |= DRAW_OPFLAGS_MOTION_POS_INT;
	p->w.ntmp[0] = p->canvas_zoom;
	p->w.drag_cursor_type = APP_CURSOR_ZOOM_DRAG;

	drawCanvas_lowQuality();
	drawCanvas_setScrollReset_update(p, NULL);

	return TRUE;
}


//===============================
// スポイト
//===============================


/** 押し
 *
 * @param enable_alt +Alt を有効にする (ブラシツールの +Alt 時は無効にする) */

mBool drawOp_spoit_press(DrawData *p,mBool enable_alt)
{
	mPoint pt;
	RGBAFix15 pix;
	uint32_t col;

	drawOpSub_setOpInfo(p, DRAW_OPTYPE_GENERAL, NULL, NULL, 0);

	drawOpSub_getImagePoint_int(p, &pt);

	if(pt.x >= 0 && pt.x < p->imgw && pt.y >= 0 && pt.y < p->imgh)
	{
		//----- 色取得

		if(p->w.press_state & M_MODS_CTRL)
		{
			//+Ctrl : カレントレイヤ (フォルダの場合は除く)

			if(drawOpSub_isFolder_curlayer()) return TRUE;

			TileImage_getPixel(p->curlayer->img, pt.x, pt.y, &pix);
		}
		else
			//全レイヤ合成後
			drawImage_getBlendColor_atPoint(p, pt.x, pt.y, &pix);

		col = RGBAFix15toRGB(&pix);

		//----- セット

		if(enable_alt && (p->w.press_state & M_MODS_ALT))
		{
			//+Alt : 色マスクに追加

			drawColorMask_addColor(p, col);
			DockColor_changeColorMask();
		}
		else if(p->w.press_state & M_MODS_SHIFT)
		{
			//+Shift : 色マスクにセット

			drawColorMask_setColor(p, col);
			DockColor_changeColorMask();
		}
		else
		{
			//描画色にセット

			drawColor_setDrawColor(col);
			DockColor_changeDrawColor();
		}
	}

	return TRUE;
}


//===============================
// 中間色作成
//===============================
/*
 * ntmp[0..2] : 最初の色 (RGB)
 */


/** 押し */

mBool drawOp_intermediateColor_press(DrawData *p)
{
	mPoint pt;
	mNanoTime nt;
	RGBAFix15 pix;
	int i,rgb[3],c;

	//スポイト

	drawOpSub_getImagePoint_int(p, &pt);
	drawImage_getBlendColor_atPoint(p, pt.x, pt.y, &pix);

	//

	mNanoTimeGet(&nt);

	if(p->w.sec_midcol == 0 || nt.sec > p->w.sec_midcol + 5)
	{
		//最初の色 (前回押し時から5秒以上経った場合は初期化)

		p->w.ntmp[0] = pix.r;
		p->w.ntmp[1] = pix.g;
		p->w.ntmp[2] = pix.b;

		p->w.sec_midcol = nt.sec;
	}
	else
	{
		//中間色作成

		for(i = 0; i < 3; i++)
		{
			c = (p->w.ntmp[i] - pix.c[i]) / 2 + pix.c[i];
			rgb[i] = RGBCONV_FIX15_TO_8(c);
		}

		drawColor_setDrawColor(M_RGB(rgb[0], rgb[1], rgb[2]));

		DockColor_changeDrawColor();

		p->w.sec_midcol = 0;
	}

	//

	drawOpSub_setOpInfo(p, DRAW_OPTYPE_GENERAL, NULL, NULL, 0);

	return TRUE;
}


//===============================
// 色置換え
//===============================


/** カレントレイヤ上で、クリックした色を描画色 or 透明に置き換える */

mBool drawOp_replaceColor_press(DrawData *p,mBool rep_tp)
{
	mPoint pt;
	RGBAFix15 pixsrc,pixdst;

	if(drawOpSub_canDrawLayer_mes(p)) return FALSE;

	//スポイト

	drawOpSub_getImagePoint_int(p, &pt);

	TileImage_getPixel(p->curlayer->img, pt.x, pt.y, &pixsrc);

	//置き換える色

	if(rep_tp)
		pixdst.v64 = 0;
	else
		drawColor_getDrawColor_rgbafix(&pixdst);

	//同じ色の場合、何もしない
	/* 透明->透明、または不透明->不透明で色が同じの場合 */

	if((pixdst.a == 0 && pixsrc.a == 0)
		|| (pixdst.a && pixsrc.a && pixsrc.r == pixdst.r && pixsrc.g == pixdst.g && pixsrc.b == pixdst.b))
		return FALSE;

	//処理

	drawOpSub_setDrawInfo_overwrite();
	drawOpSub_beginDraw_single(p);

	if(pixsrc.a == 0)
		//透明->指定色
		TileImage_replaceColor_fromTP(p->w.dstimg, &pixdst);
	else
		//不透明->指定色 or 透明
		TileImage_replaceColor_fromNotTP(p->w.dstimg, &pixsrc, &pixdst);

	drawOpSub_endDraw_single(p);
	
	//

	drawOpSub_setOpInfo(p, DRAW_OPTYPE_GENERAL, NULL, NULL, 0);

	return TRUE;
}


//===============================
// テキスト描画
//===============================
/*
 * tileimgTmp : プレビュー描画用
 * pttmp[0]   : 描画位置 (イメージ座標)
 * rcdraw     : プレビューの前回の描画範囲
 */


/** フォント作成 */

void drawText_createFont()
{
	DrawTextData *pt = &APP_DRAW->drawtext;
	mFontInfo info;

	DrawFont_free(pt->font);

	//------ mFontInfo セット

	mMemzero(&info, sizeof(mFontInfo));

	info.mask = MFONTINFO_MASK_SIZE | MFONTINFO_MASK_RENDER;

	//フォント名

	if(!mStrIsEmpty(&pt->strName))
	{
		info.mask |= MFONTINFO_MASK_FAMILY;
		mStrCopy(&info.strFamily, &pt->strName);
	}

	//スタイル

	if(!mStrIsEmpty(&pt->strStyle))
	{
		info.mask |= MFONTINFO_MASK_STYLE;
		mStrCopy(&info.strStyle, &pt->strStyle);
	}
	else
	{
		//太字
		
		info.mask |= MFONTINFO_MASK_WEIGHT;
		info.weight = (pt->weight == 0)? MFONTINFO_WEIGHT_NORMAL: MFONTINFO_WEIGHT_BOLD;

		//斜体

		info.mask |= MFONTINFO_MASK_SLANT;
		info.slant = pt->slant;
	}

	//サイズ

	info.size = pt->size;
	if(pt->flags & DRAW_DRAWTEXT_F_SIZE_PIXEL) info.size = -info.size;

	//レンダリング

	info.render = (pt->flags & DRAW_DRAWTEXT_F_ANTIALIAS)? MFONTINFO_RENDER_GRAY: MFONTINFO_RENDER_MONO;

	//------ フォント作成

	pt->font = DrawFont_create(&info);

	mFontInfoFree(&info);
}

/** ヒンティング変更 */

void drawText_setHinting(DrawData *p)
{
	DrawFont_setHinting(p->drawtext.font, p->drawtext.hinting);
}

/** テキスト描画情報セット */

static void _drawtext_set_info(DrawFontInfo *dst,DrawTextData *src,TileImage *img)
{
	dst->char_space = src->char_space;
	dst->line_space = src->line_space;

	dst->flags = 0;
	if(src->flags & DRAW_DRAWTEXT_F_VERT) dst->flags |= DRAWFONT_F_VERT;

	dst->param = img;
}

/** 点描画関数 (プレビュー用) */

static void _drawtext_setpixel_prev(int x,int y,int a,void *param)
{
	APP_DRAW->w.rgbaDraw.a = RGBCONV_8_TO_FIX15(a);

	TileImage_setPixel_subdraw((TileImage *)param, x, y, &APP_DRAW->w.rgbaDraw);
}

/** プレビュー描画 */

void drawText_drawPreview(DrawData *p)
{
	DrawTextData *pdat = &p->drawtext;
	DrawFontInfo info;
	mRect rc;

	rc = p->w.rcdraw;

	//前回のイメージと範囲をクリア

	if(!mRectIsEmpty(&rc))
	{
		TileImage_freeAllTiles(p->tileimgTmp);

		mRectEmpty(&p->w.rcdraw);
	}

	//プレビューなし、または空文字列の場合、前回の範囲を更新して終了

	if(!(pdat->flags & DRAW_DRAWTEXT_F_PREVIEW)
		|| mStrIsEmpty(&pdat->strText))
	{
		drawUpdate_rect_imgcanvas_canvasview_fromRect(p, &rc);
		return;
	}

	//描画

	_drawtext_set_info(&info, pdat, p->tileimgTmp);

	info.setpixel = _drawtext_setpixel_prev;

	g_tileimage_dinfo.funcColor = TileImage_colfunc_normal;

	TileImageDrawInfo_clearDrawRect();

	DrawFont_drawText(pdat->font, p->w.pttmp[0].x, p->w.pttmp[0].y,
		pdat->strText.buf, &info);

	//更新 (前回の範囲と結合)

	mRectUnion(&rc, &g_tileimage_dinfo.rcdraw);

	drawUpdate_rect_imgcanvas_fromRect(p, &rc);

	//範囲を記録

	p->w.rcdraw = g_tileimage_dinfo.rcdraw;
}

/** 点描画関数 (確定時) */

static void _drawtext_setpixel_draw(int x,int y,int a,void *param)
{
	APP_DRAW->w.rgbaDraw.a = RGBCONV_8_TO_FIX15(a);

	TileImage_setPixel_draw_direct((TileImage *)param, x, y, &APP_DRAW->w.rgbaDraw);
}

/** 描画処理 */

static void _drawtext_draw(DrawData *p)
{
	DrawFontInfo info;

	//描画準備

	drawOpSub_setDrawInfo(p, TOOL_TEXT, 0);

	//描画情報

	_drawtext_set_info(&info, &p->drawtext, p->w.dstimg);

	info.setpixel = _drawtext_setpixel_draw;

	//描画

	drawOpSub_beginDraw_single(p);

	DrawFont_drawText(p->drawtext.font, p->w.pttmp[0].x, p->w.pttmp[0].y,
		p->drawtext.strText.buf, &info);

	drawOpSub_endDraw_single(p);
}

/** 押し (グラブはしない) */

mBool drawOp_drawtext_press(DrawData *p)
{
	mBool ret;

	//描画可能か

	if(drawOpSub_canDrawLayer_mes(p))
		return FALSE;

	//描画位置

	drawOpSub_getImagePoint_int(p, &p->w.pttmp[0]);

	//プレビュー用イメージ作成

	if(!drawOpSub_createTmpImage_same_current_imgsize(p))
		return FALSE;

	//初期化

	mRectEmpty(&p->w.rcdraw);

	drawColor_getDrawColor_rgbafix(&p->w.rgbaDraw);

	//ダイアログ

	p->drawtext.in_dialog = TRUE;

	ret = DrawTextDlg_run(M_WINDOW(APP_WIDGETS->mainwin));

	p->drawtext.in_dialog = FALSE;

	//----------

	//プレビュー用イメージ削除

	drawOpSub_freeTmpImage(p);

	//プレビュー範囲を元に戻す
	/* [!] ダイアログ表示中にメインウィンドウのサイズを変更した場合、
	 *     キャンバスビューパレット上のイメージが、プレビューが適用された状態になっているので、
	 *     念のためキャンバスビューの範囲も戻す。 */

	drawUpdate_rect_imgcanvas_canvasview_fromRect(p, &p->w.rcdraw);

	//描画

	if(ret && !mStrIsEmpty(&p->drawtext.strText))
		_drawtext_draw(p);

	return FALSE;
}

/** ダイアログ中にキャンバス上がクリックされた時 */

void drawText_setDrawPoint_inDialog(DrawData *p,int x,int y)
{
	//位置

	drawCalc_areaToimage_pt(p, &p->w.pttmp[0], x, y);

	//プレビュー

	drawText_drawPreview(p);
}
