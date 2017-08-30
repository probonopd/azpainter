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
 * 操作 - サブ関数
 *****************************************/

#include <stdlib.h>
#include <math.h>

#include "mDef.h"
#include "mRectBox.h"
#include "mWidget.h"
#include "mMessageBox.h"
#include "mTrans.h"

#include "defWidgets.h"
#include "defConfig.h"
#include "defDraw.h"
#include "defPixelMode.h"

#include "LayerItem.h"
#include "LayerList.h"
#include "BrushItem.h"
#include "BrushList.h"
#include "GradationList.h"

#include "TileImage.h"
#include "TileImageDrawInfo.h"
#include "MainWinCanvas.h"
#include "macroToolOpt.h"
#include "BrushDrawParam.h"

#include "DrawPointBuf.h"
#include "FillPolygon.h"
#include "Undo.h"

#include "draw_main.h"
#include "draw_calc.h"
#include "draw_op_def.h"
#include "draw_op_sub.h"

#include "trgroup.h"


//---------------------------
/* 塗りタイプ関数 */

static void (*g_pixmodefunc_brush[])(TileImage *,RGBAFix15 *,RGBAFix15 *,void *) = {
	TileImage_colfunc_normal, TileImage_colfunc_normal, TileImage_colfunc_compareA,
	TileImage_colfunc_overwrite, TileImage_colfunc_overwrite,
	TileImage_colfunc_dodge, TileImage_colfunc_burn, TileImage_colfunc_add,
	TileImage_colfunc_erase
};

static void (*g_pixmodefunc_tool[])(TileImage *,RGBAFix15 *,RGBAFix15 *,void *) = {
	TileImage_colfunc_normal, TileImage_colfunc_compareA,
	TileImage_colfunc_overwrite, TileImage_colfunc_erase
};

//---------------------------



/** 操作開始時の情報セット */

void drawOpSub_setOpInfo(DrawData *p,int optype,
	void (*motion)(DrawData *,uint32_t),mBool (*release)(DrawData *),int opsubtype)
{
	p->w.optype = optype;
	p->w.opsubtype = opsubtype;

	p->w.funcMotion = motion;
	p->w.funcRelease = release;
	p->w.funcPressInGrab = NULL;
	p->w.funcAction = NULL;
}

/** 現在のレイヤに描画可能かどうか
 *
 * @return 0 で可能 */

int drawOpSub_canDrawLayer(DrawData *p)
{
	if(LAYERITEM_IS_FOLDER(p->curlayer))
		//フォルダ
		return CANDRAWLAYER_FOLDER;
	else if(LayerItem_isLock_real(p->curlayer))
		//ロック (上位フォルダがロックありの場合もON)
		return CANDRAWLAYER_LOCK;
	else
		return CANDRAWLAYER_OK;
}

/** 現在のレイヤに描画可能かどうか
 *
 * 非表示レイヤも判定。メッセージ表示付き。*/

int drawOpSub_canDrawLayer_mes(DrawData *p)
{
	int ret;

	ret = drawOpSub_canDrawLayer(p);

	//非表示レイヤ

	if(!ret && !LayerItem_isVisible_real(p->curlayer))
		ret = CANDRAWLAYER_MES_HIDE;

	//メッセージ

	if(ret)
	{
		mMessageBox(M_WINDOW(APP_WIDGETS->mainwin), NULL,
			M_TR_T2(TRGROUP_DRAW_MESSAGE, ret), MMESBOX_OK, MMESBOX_OK);
	}

	return ret;
}

/** カレントレイヤがフォルダか */

mBool drawOpSub_isFolder_curlayer()
{
	return LAYERITEM_IS_FOLDER(APP_DRAW->curlayer);
}


//========================


/** 領域の XOR 用に tileimgTmp を作成 */

mBool drawOpSub_createTmpImage_area(DrawData *p)
{
	p->tileimgTmp = TileImage_new(TILEIMAGE_COLTYPE_ALPHA1, p->szCanvas.w, p->szCanvas.h);

	return (p->tileimgTmp != NULL);
}

/** tileimgTmp 作成 (カレントと同じカラータイプ、イメージサイズ分) */

mBool drawOpSub_createTmpImage_same_current_imgsize(DrawData *p)
{
	p->tileimgTmp = TileImage_new(p->curlayer->coltype, p->imgw, p->imgh);
	if(!p->tileimgTmp) return FALSE;

	//A1/A16 の場合、カレントと同じ色に

	TileImage_setImageColor(p->tileimgTmp, p->curlayer->col);

	return TRUE;
}

/** tileimgTmp を削除 */

void drawOpSub_freeTmpImage(DrawData *p)
{
	TileImage_free(p->tileimgTmp);
	p->tileimgTmp = NULL;
}

/** FillPolygon を解放 */

void drawOpSub_freeFillPolygon(DrawData *p)
{
	FillPolygon_free(p->w.fillpolygon);
	p->w.fillpolygon = NULL;
}


//=============================
// イメージ
//=============================


/** XOR での範囲選択後、選択範囲イメージとして TileImage [A1] を作成
 *
 * スタンプ時用。
 * 結果はキャンバスイメージの範囲内のみとなる。
 * (キャンバス上で見えない部分は範囲選択されない)
 *
 * @param rcarea  選択範囲の矩形領域座標が入る
 * @return キャンバス範囲外や失敗時は NULL */

TileImage *drawOpSub_createSelectImage_byOpType(DrawData *p,mRect *rcarea)
{
	TileImage *img = NULL;
	mBox box;
	mRect rc;
	RGBAFix15 pix;

	pix.a = 0x8000;

	switch(p->w.optype)
	{
		//矩形 (イメージに対する)
		case DRAW_OPTYPE_XOR_BOXIMAGE:
			//キャンバス範囲内にクリッピング
			
			if(!drawCalc_getImageBox_rect(p, &box, &p->w.rctmp[0]))
				return NULL;

			//作成

			img = TileImage_new(TILEIMAGE_COLTYPE_ALPHA1, box.w, box.h);
			if(!img) return NULL;

			TileImage_setOffset(img, box.x, box.y);

			//矩形内を塗りつぶし

			g_tileimage_dinfo.funcDrawPixel = TileImage_setPixel_new;

			TileImage_drawFillBox(img, box.x, box.y, box.w, box.h, &pix);

			mRectSetByBox(rcarea, &box);
			break;

		//多角形、フリーハンド
		case DRAW_OPTYPE_XOR_POLYGON:
		case DRAW_OPTYPE_XOR_LASSO:
			//範囲の矩形

			FillPolygon_getDrawRect(p->w.fillpolygon, &rc);
			
			if(!drawCalc_getImageBox_rect(p, &box, &rc))
				return NULL;
		
			//作成
			
			img = TileImage_new(TILEIMAGE_COLTYPE_ALPHA1, box.w, box.h);
			if(!img) return NULL;

			TileImage_setOffset(img, box.x, box.y);

			//多角形塗りつぶし

			g_tileimage_dinfo.funcDrawPixel = TileImage_setPixel_new_drawrect;

			TileImageDrawInfo_clearDrawRect();
		
			TileImage_drawFillPolygon(img, p->w.fillpolygon, &pix, FALSE);

			drawOpSub_freeFillPolygon(p);

			//実際に描画された範囲

			*rcarea = g_tileimage_dinfo.rcdraw;

			if(!drawCalc_clipImageRect(p, rcarea))
			{
				TileImage_free(img);
				img = NULL;
			}
			break;
	}

	return img;
}


//=============================
// 描画開始/終了
//=============================


/** 描画開始
 *
 * [!] この前に drawOpSub_setDrawInfo() を必ず実行すること。 */

void drawOpSub_beginDraw(DrawData *p)
{
	TileImageDrawInfo *info = &g_tileimage_dinfo;

	//描画情報の初期化

	info->err = 0;

	TileImageDrawInfo_clearDrawRect();

	//現在のレイヤイメージ情報取得

	TileImage_getInfo(p->curlayer->img, &info->tileimginfo);

	//描画前保存用イメージ作成

	p->tileimgDraw = TileImage_createSame(p->tileimgDraw, p->curlayer->img, -1);
	info->img_save = p->tileimgDraw;
	
	if(!info->img_save) info->err = 1;

	//ブラシ描画のストローク重ね塗り用の濃度バッファイメージ作成

	if(p->w.drawinfo_flags & DRAW_DRAWINFO_F_BRUSH_STROKE)
	{
		p->tileimgDraw2 = TileImage_createSame(p->tileimgDraw2, p->curlayer->img, TILEIMAGE_COLTYPE_ALPHA16);
		info->img_brush_stroke = p->tileimgDraw2;

		if(!info->img_brush_stroke) info->err = 1;
	}
	else
		info->img_brush_stroke = NULL;
}

/** 描画終了
 *
 * @param rc  描画されたイメージ範囲
 * @param boximg 更新用イメージ範囲 (キャンバス範囲内調整済み) */

void drawOpSub_endDraw(DrawData *p,mRect *rc,mBox *boximg)
{
	if(g_tileimage_dinfo.err)
	{
		//エラー時はアンドゥクリア

		Undo_deleteAll();
	}
	else
	{
		//----- エラーなし
		
		//空タイルを解放

		TileImage_freeEmptyTiles_byUndo(p->curlayer->img, p->tileimgDraw);

		//undo

		Undo_addTilesImage(&g_tileimage_dinfo.tileimginfo, rc);
	}

	//タイル削除

	TileImage_freeAllTiles(p->tileimgDraw);
	TileImage_freeAllTiles(p->tileimgDraw2);
}

/** 単体描画開始 */

void drawOpSub_beginDraw_single(DrawData *p)
{
	MainWinCanvasArea_setCursor_wait();
	
	drawOpSub_beginDraw(p);
}

/** 単体描画終了 */

void drawOpSub_endDraw_single(DrawData *p)
{
	drawOpSub_finishDraw_single(p);

	MainWinCanvasArea_restoreCursor();
}


//=============================
// 更新/終了の共通処理
//=============================


/** 描画された範囲を全体の範囲に追加し、キャンバス更新
 *
 * [自由線/連続直線/集中線]
 *
 * @param btimer TRUE でタイマー更新セット。FALSE で直接更新。 */

void drawOpSub_addrect_and_update(DrawData *p,mBool btimer)
{
	mBox box;

	if(!mRectIsEmpty(&g_tileimage_dinfo.rcdraw))
	{
		//全体の範囲追加

		mRectUnion(&p->w.rcdraw, &g_tileimage_dinfo.rcdraw);

		//更新

		if(drawCalc_getImageBox_rect(p, &box, &g_tileimage_dinfo.rcdraw))
		{
			if(btimer)
				MainWinCanvasArea_setTimer_updateRect(&box);
			else
				drawUpdate_rect_imgcanvas(p, &box);
		}
	}
}

/** DrawData::w.rcdraw の範囲で描画終了処理を行う
 *
 * 複数描画時用。イメージとキャンバスは更新済みである。 */

void drawOpSub_finishDraw_workrect(DrawData *p)
{
	mBox box;

	if(!mRectIsEmpty(&p->w.rcdraw))
	{
		//イメージ範囲

		if(!drawCalc_getImageBox_rect(p, &box, &p->w.rcdraw))
			box.x = -1;

		//描画終了

		drawOpSub_endDraw(p, &p->w.rcdraw, &box);

		//更新

		MainWinCanvasArea_clearTimer_updateRect(TRUE);

		//[dock]キャンバスビュー更新
		/* 固定表示時は、ストローク描画後に更新 */

		drawUpdate_endDraw_box(p, &box);
	}
}

/** TileImageDrawInfo::rcdraw の範囲で描画終了処理を行う
 *
 * 単体描画時用。イメージとキャンバスの更新も行う。 */

void drawOpSub_finishDraw_single(DrawData *p)
{
	mBox box;

	if(!mRectIsEmpty(&g_tileimage_dinfo.rcdraw))
	{
		//イメージ範囲

		if(!drawCalc_getImageBox_rect(p, &box, &g_tileimage_dinfo.rcdraw))
			box.x = -1;

		//UNDO

		drawOpSub_endDraw(p, &g_tileimage_dinfo.rcdraw, &box);

		//更新

		if(box.x != -1)
		{
			drawUpdate_rect_imgcanvas(p, &box);
			drawUpdate_endDraw_box(p, &box);
		}
	}
}


//==============================
// 描画前のセット
//==============================


/** ブラシの描画情報セット */

static void _setdrawinfo_brush(DrawData *p,TileImageDrawInfo *info,mBool registered)
{
	BrushDrawParam *param;
	BrushItem *pi;
	char *pc;

	BrushList_getDrawInfo(&pi, &param, registered);

	info->brushparam = param;

	//塗り

	info->funcColor = g_pixmodefunc_brush[pi->pixelmode];

	//描画色

	p->w.rgbaDraw.a = Density100toFix15(pi->opacity);

	info->rgba_brush = p->w.rgbaDraw;

	//手ブレ補正
	//[!] 定規が ON の場合は補正なし

	if(p->rule.type)
		DrawPointBuf_setSmoothing(DRAWPOINTBUF_TYPE_NONE, 0);
	else
		DrawPointBuf_setSmoothing(pi->hosei_type, pi->hosei_strong);

	//間隔

	param->interval = param->interval_src;

	//テクスチャ

	pc = pi->texture_path;

	if(!pc)
		info->texture = NULL;
	else if(pc[0] == '?' && pc[1] == 0)
		info->texture = p->img8OptTex;
	else
		info->texture = param->img_texture;

	//タイプごと

	p->w.ntmp[0] = 0;

	switch(pi->type)
	{
		//消しゴム
		case BRUSHITEM_TYPE_ERASE:
			info->funcColor = TileImage_colfunc_erase;
			/* 以下、ストローク重ね塗りは通常ブラシと共通 */

		//通常ブラシ
		case BRUSHITEM_TYPE_NORMAL:
			//ストローク重ね塗り
			if(pi->pixelmode == PIXELMODE_BLEND_STROKE)
			{
				info->funcDrawPixel = TileImage_setPixel_draw_brush_stroke;
				
				p->w.drawinfo_flags |= DRAW_DRAWINFO_F_BRUSH_STROKE;
			}
			break;
		//水彩 (常にピクセル重ね塗り)
		case BRUSHITEM_TYPE_WATER:
			info->funcColor = TileImage_colfunc_normal;
			break;
		//ぼかし
		case BRUSHITEM_TYPE_BLUR:
			info->funcColor = TileImage_colfunc_blur;

			//濃度を元に間隔を設定
			param->interval = (100 - pi->opacity) * 0.01 + 0.04;
			break;

		//ドットペン
		case BRUSHITEM_TYPE_DOTPEN:
			if(pi->pixelmode == PIXELMODE_BLEND_STROKE)
				info->funcDrawPixel = TileImage_setPixel_draw_dot_stroke;

			p->w.ntmp[0] = 1;
			break;

		//指先
		case BRUSHITEM_TYPE_FINGER:
			TileImage_setDotStyle(pi->radius);
		
			info->funcDrawPixel = TileImage_setPixel_draw_dotstyle_direct;
			info->funcColor = TileImage_colfunc_finger;

			p->w.ntmp[0] = 2;
			break;
	}
}

/** 描画前の各情報セット
 *
 * @param toolno  ツール番号。-1 でマスクなどのみセット */

void drawOpSub_setDrawInfo(DrawData *p,int toolno,int param)
{
	TileImageDrawInfo *info = &g_tileimage_dinfo;
	uint32_t val;

	info->funcDrawPixel = TileImage_setPixel_draw_direct;
	info->funcColor = TileImage_colfunc_normal;

	//テクスチャ

	info->texture = p->img8OptTex;

	//選択範囲

	info->img_sel = p->tileimgSel;
	
	//レイヤマスク

	if(LAYERITEM_IS_MASK_UNDER(p->curlayer))
		//下レイヤでマスク
		info->img_mask = LayerList_getMaskImage(p->layerlist, p->curlayer);
	else if(p->masklayer && p->masklayer != p->curlayer)
		//レイヤマスク指定あり (カレントがレイヤマスクならなし)
		info->img_mask = p->masklayer->img;
	else
		info->img_mask = NULL;

	//アルファマスク

	info->alphamask_type = p->curlayer->alphamask;

	//色マスク

	info->colmask_type = p->col.colmask_type;
	info->colmask_col = p->col.rgb15Colmask;

	//描画先イメージ

	p->w.dstimg = p->curlayer->img;

	//描画色

	p->w.rgbaDraw.r = p->col.rgb15DrawCol.r;
	p->w.rgbaDraw.g = p->col.rgb15DrawCol.g;
	p->w.rgbaDraw.b = p->col.rgb15DrawCol.b;
	p->w.rgbaDraw.a = 0x8000;

	//

	p->w.drawinfo_flags = 0;

	//---------

	if(toolno < 0) return;

	switch(toolno)
	{
		//ブラシ
		/* param: 登録ブラシか
		 * ntmp[0] : ブラシのタイプが入る */
		case TOOL_BRUSH:
			_setdrawinfo_brush(p, info, (param != 0));
			break;
	
		//図形塗りつぶし
		/* ntmp[0] : アンチエイリアス */
		case TOOL_FILL_POLYGON:
		case TOOL_FILL_POLYGON_ERASE:
			val = (toolno == TOOL_FILL_POLYGON)? p->tool.opt_fillpoly: p->tool.opt_fillpoly_erase;

			p->w.rgbaDraw.a = Density100toFix15(FILLPOLY_GET_OPACITY(val));
			p->w.ntmp[0] = FILLPOLY_IS_ANTIALIAS(val);

			if(toolno == TOOL_FILL_POLYGON_ERASE)
				info->funcColor = TileImage_colfunc_erase;
			else
				info->funcColor = g_pixmodefunc_tool[FILLPOLY_GET_PIXMODE(val)];
			break;

		//塗りつぶし
		case TOOL_FILL:
			val = p->tool.opt_fill;
			
			p->w.rgbaDraw.a = Density100toFix15(FILL_GET_OPACITY(val));
			info->funcColor = g_pixmodefunc_tool[FILL_GET_PIXMODE(val)];
			break;
	
		//不透明範囲消去 (テクスチャ無効)
		case TOOL_FILL_ERASE:
			info->funcColor = TileImage_colfunc_erase;
			info->texture = NULL;
			break;

		//グラデーション
		case TOOL_GRADATION:
			info->funcColor = g_pixmodefunc_tool[GRAD_GET_PIXMODE(p->tool.opt_grad)];
			break;

		//スタンプ
		case TOOL_STAMP:
			info->texture = NULL;

			//上書き
			if(STAMP_IS_OVERWRITE(p->tool.opt_stamp))
				info->funcColor = TileImage_colfunc_overwrite;
			break;

		//矩形編集
		case TOOL_BOXEDIT:
			drawOpSub_clearDrawMasks();

			info->funcColor = TileImage_colfunc_overwrite;
			break;
	}
}

/** 描画時のマスク類 (選択範囲/レイヤマスク/テクスチャ/色マスク/Aマスク) をクリア */

void drawOpSub_clearDrawMasks()
{
	TileImageDrawInfo *p = &g_tileimage_dinfo;

	p->img_sel = NULL;
	p->img_mask = NULL;
	p->texture = NULL;
	p->colmask_type = 0;
	p->alphamask_type = 0;
}

/** 選択範囲の図形描画/自動選択用にセット */

void drawOpSub_setDrawInfo_select()
{
	TileImageDrawInfo *p = &g_tileimage_dinfo;

	drawOpSub_clearDrawMasks();

	p->funcDrawPixel = TileImage_setPixel_subdraw;
	p->funcColor = TileImage_colfunc_overwrite;

	APP_DRAW->w.rgbaDraw.a = 0x8000;
}

/** 範囲イメージの移動/コピー時の貼付け用 */

void drawOpSub_setDrawInfo_select_paste()
{
	drawOpSub_setDrawInfo(APP_DRAW, -1, 0);
	drawOpSub_clearDrawMasks();
}

/** 選択範囲 切り取り用 */

void drawOpSub_setDrawInfo_select_cut()
{
	drawOpSub_setDrawInfo(APP_DRAW, -1, 0);

	drawOpSub_clearDrawMasks();

	g_tileimage_dinfo.funcColor = TileImage_colfunc_overwrite;
}

/** 塗りつぶし/消去コマンド用 */

void drawOpSub_setDrawInfo_fillerase(mBool erase)
{
	drawOpSub_setDrawInfo(APP_DRAW, -1, 0);

	g_tileimage_dinfo.funcColor = (erase)? TileImage_colfunc_erase: TileImage_colfunc_overwrite;
}

/** 上書き描画用に情報セット
 *
 * 色置き換え */

void drawOpSub_setDrawInfo_overwrite()
{
	drawOpSub_setDrawInfo(APP_DRAW, -1, 0);
	drawOpSub_clearDrawMasks();

	g_tileimage_dinfo.funcColor = TileImage_colfunc_overwrite;
}

/** フィルタ用に情報セット */

void drawOpSub_setDrawInfo_filter()
{
	drawOpSub_setDrawInfo(APP_DRAW, -1, 0);
	drawOpSub_clearDrawMasks();

	g_tileimage_dinfo.img_sel = APP_DRAW->tileimgSel;

	g_tileimage_dinfo.funcColor = TileImage_colfunc_overwrite;
}

/** フィルタ漫画用のブラシ描画情報セット */

void drawOpSub_setDrawInfo_filter_comic_brush()
{
	g_tileimage_dinfo.brushparam = BrushList_getBrushDrawParam_forFilter();
}

/** グラデーション描画時の情報をセット
 *
 * 終了時、info->buf を解放すること */

void drawOpSub_setDrawGradationInfo(TileImageDrawGradInfo *info)
{
	DrawData *p = APP_DRAW;
	const uint8_t *rawbuf;
	uint32_t val;
	uint8_t flags = 0;

	val = p->tool.opt_grad;

	//フラグ (ツール設定から)

	if(GRAD_IS_REVERSE(val)) flags |= TILEIMAGE_DRAWGRAD_F_REVERSE;
	if(GRAD_IS_LOOP(val)) flags |= TILEIMAGE_DRAWGRAD_F_LOOP;

	//グラデーションデータ
	/* rawbuf == NULL で [描画色->背景色] データ。
	 * rawbuf は先頭のフラグデータは除く。 */

	if(GRAD_IS_NOT_CUSTOM(val))
		rawbuf = NULL;
	else
		rawbuf = GradationList_getBuf_atIndex(GRAD_GET_SELNO(val));

	if(!rawbuf)
		rawbuf = GradationList_getDefaultDrawData();
	else
	{
		if(*rawbuf & GRADDAT_F_LOOP) flags |= TILEIMAGE_DRAWGRAD_F_LOOP;
		if(*rawbuf & GRADDAT_F_SINGLE_COL) flags |= TILEIMAGE_DRAWGRAD_F_SINGLE_COL;

		rawbuf++;
	}

	//info にセット

	info->buf = GradItem_convertDrawData(rawbuf, &p->col.rgb15DrawCol, &p->col.rgb15BkgndCol);
	info->flags = flags;
	info->opacity = (int)(GRAD_GET_OPACITY(val) / 100.0 * 256.0 + 0.5);
}


//=================================
// キャンバス領域への直接描画
//=================================


/** キャンバス領域への直接描画開始 */

mPixbuf *drawOpSub_beginAreaDraw()
{
	return mWidgetBeginDirectDraw(M_WIDGET(APP_WIDGETS->canvas_area));
}

/** キャンバス領域への直接描画終了 */

void drawOpSub_endAreaDraw(mPixbuf *pixbuf,mBox *box)
{
	//[debug]
	//mPixbufBox(pixbuf, box->x, box->y, box->w, box->h, mRGBtoPix(0xff0000));

	mWidgetEndDirectDraw(M_WIDGET(APP_WIDGETS->canvas_area), pixbuf);

	mWidgetUpdateBox_box(M_WIDGET(APP_WIDGETS->canvas_area), box);
}


//============================
// 位置取得
//============================


/** 現在の領域位置を int で取得 */

void drawOpSub_getAreaPoint_int(DrawData *p,mPoint *pt)
{	
	pt->x = floor(p->w.dptAreaCur.x);
	pt->y = floor(p->w.dptAreaCur.y);
}

/** DrawPoint の領域位置からイメージ位置 (mDoublePoint) を取得 */

void drawOpSub_getImagePoint_fromDrawPoint(DrawData *p,mDoublePoint *pt,DrawPoint *dpt)
{
	drawCalc_areaToimage_double(p, &pt->x, &pt->y, dpt->x, dpt->y);
}

/** 現在のイメージ位置を int で取得 */

void drawOpSub_getImagePoint_int(DrawData *p,mPoint *pt)
{
	double x,y;

	drawCalc_areaToimage_double(p, &x, &y, p->w.dptAreaCur.x, p->w.dptAreaCur.y);

	pt->x = floor(x);
	pt->y = floor(y);
}

/** 現在のイメージ位置を取得 (double) */

void drawOpSub_getImagePoint_double(DrawData *p,mDoublePoint *pt)
{
	drawCalc_areaToimage_double(p, &pt->x, &pt->y, p->w.dptAreaCur.x, p->w.dptAreaCur.y);
}

/** 描画位置取得 (DrawPoint) */

void drawOpSub_getDrawPoint(DrawData *p,DrawPoint *dst)
{
	drawCalc_areaToimage_double(p, &dst->x, &dst->y, p->w.dptAreaCur.x, p->w.dptAreaCur.y);

	dst->pressure = p->w.dptAreaCur.pressure;
}


//============================
// 操作関連
//============================


/** pttmp[0] を 1 .. num までにコピー */

void drawOpSub_copyTmpPoint(DrawData *p,int num)
{
	int i;

	for(i = 1; i <= num; i++)
		p->w.pttmp[i] = p->w.pttmp[0];
}

/** イメージのドラッグ移動操作開始時の初期化  */

void drawOpSub_startMoveImage(DrawData *p,TileImage *img)
{
	TileImage_getOffset(img, p->w.pttmp);

	p->w.ptd_tmp[0].x = p->w.ptd_tmp[0].y = 0;
	p->w.pttmp[1].x = p->w.pttmp[1].y = 0;

	mRectEmpty(&p->w.rcdraw);
}

/** 押し時、選択範囲の中かどうか */

mBool drawOpSub_isPressInSelect(DrawData *p)
{
	mPoint pt;

	drawOpSub_getImagePoint_int(p, &pt);

	return TileImage_isPixelOpaque(p->tileimgSel, pt.x, pt.y);
}


/** 直線描画時の位置を取得 */

void drawOpSub_getDrawLinePoints(DrawData *p,mDoublePoint *pt1,mDoublePoint *pt2)
{
	drawCalc_areaToimage_double(p, &pt1->x, &pt1->y, p->w.pttmp[0].x, p->w.pttmp[0].y);
	drawCalc_areaToimage_double(p, &pt2->x, &pt2->y, p->w.pttmp[1].x, p->w.pttmp[1].y);
}

/** 回転なし時の四角形描画の範囲取得 */

void drawOpSub_getDrawBox_noangle(DrawData *p,mBox *box)
{
	mPoint pt[4];

	drawCalc_areaToimage_pt(p, pt    , p->w.pttmp[0].x, p->w.pttmp[0].y);
	drawCalc_areaToimage_pt(p, pt + 1, p->w.pttmp[1].x, p->w.pttmp[0].y);
	drawCalc_areaToimage_pt(p, pt + 2, p->w.pttmp[1].x, p->w.pttmp[1].y);
	drawCalc_areaToimage_pt(p, pt + 3, p->w.pttmp[0].x, p->w.pttmp[1].y);

	mBoxSetByPoints(box, pt, 4);
}

/** 四角形描画時の4点を取得 */

void drawOpSub_getDrawBoxPoints(DrawData *p,mDoublePoint *pt)
{
	mDoublePoint pt1,pt2;

	pt1.x = p->w.pttmp[0].x, pt1.y = p->w.pttmp[0].y;
	pt2.x = p->w.pttmp[1].x, pt2.y = p->w.pttmp[1].y;

	drawCalc_areaToimage_double(p, &pt[0].x, &pt[0].y, pt1.x, pt1.y);
	drawCalc_areaToimage_double(p, &pt[1].x, &pt[1].y, pt2.x, pt1.y);
	drawCalc_areaToimage_double(p, &pt[2].x, &pt[2].y, pt2.x, pt2.y);
	drawCalc_areaToimage_double(p, &pt[3].x, &pt[3].y, pt1.x, pt2.y);
}

/** 楕円描画時の位置と半径を取得 */

void drawOpSub_getDrawEllipseParam(DrawData *p,mDoublePoint *pt_ct,mDoublePoint *pt_radius)
{
	double xr,yr;

	drawOpSub_getImagePoint_fromDrawPoint(p, pt_ct, &p->w.dptAreaPress);

	xr = fabs(p->w.dptAreaPress.x - p->w.dptAreaCur.x);
	yr = fabs(p->w.dptAreaPress.y - p->w.dptAreaCur.y);

	//ntmp[0] : 正円か

	if(p->w.ntmp[0])
	{
		if(xr > yr) yr = xr;
		else xr = yr;
	}

	pt_radius->x = xr * p->viewparam.scalediv;
	pt_radius->y = yr * p->viewparam.scalediv;
}
