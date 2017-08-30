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
 * DrawData
 * 
 * 選択範囲関連
 **************************************/
/*
 * tileimgSel : 選択範囲がない時は NULL または空の状態。
 * sel.rcsel  : 選択範囲全体を含む矩形 (おおよその範囲)
 */

#include "mDef.h"
#include "mRectBox.h"
#include "mStr.h"
#include "mUtilFile.h"

#include "defDraw.h"

#include "draw_main.h"
#include "draw_select.h"
#include "draw_layer.h"
#include "draw_op_sub.h"

#include "TileImage.h"
#include "TileImageDrawInfo.h"
#include "LayerItem.h"
#include "ConfigData.h"

#include "MainWinCanvas.h"
#include "PopupThread.h"


//---------------

#define _CURSOR_WAIT    MainWinCanvasArea_setCursor_wait();
#define _CURSOR_RESTORE MainWinCanvasArea_restoreCursor();

#define _FILENAME_SELCOPY  "selcopy.dat"

//---------------



//======================
// sub
//======================


/** キャンバスイメージ全体の範囲を取得 */

static void _get_canvimage_rect(DrawData *p,mRect *rc)
{
	rc->x1 = rc->y1 = 0;
	rc->x2 = p->imgw - 1;
	rc->y2 = p->imgh - 1;
}

/** 現在のレイヤにおける選択範囲の描画範囲を取得 */

static void _get_cur_drawrect(DrawData *p,mRect *rc)
{
	if(drawOpSub_isFolder_curlayer())
		//フォルダの場合はイメージ全体
		_get_canvimage_rect(p, rc);
	else
		//カレントレイヤの描画可能範囲
		TileImage_getCanDrawRect_pixel(p->curlayer->img, rc);
}


//======================
//
//======================


/** 選択範囲があるか */

mBool drawSel_isHave()
{
	return (APP_DRAW->tileimgSel && !mRectIsEmpty(&APP_DRAW->sel.rcsel));
}

/** 選択範囲解除 */

void drawSel_release(DrawData *p,mBool update)
{
	if(p->tileimgSel)
	{
		//イメージ削除

		TileImage_free(p->tileimgSel);
		p->tileimgSel = NULL;

		//更新

		if(update)
			drawUpdate_rect_canvas_forSelect(p, &p->sel.rcsel);
	}
}


//======================
// コマンド
//======================


/** 選択範囲反転 */

void drawSel_inverse(DrawData *p)
{
	if(!drawSel_createImage(p)) return;

	_CURSOR_WAIT

	//描画

	TileImageDrawInfo_clearDrawRect();

	TileImage_inverseSelect(p->tileimgSel);

	//透明部分を解放

	drawSel_freeEmpty(p);

	//更新

	drawUpdate_rect_canvas_forSelect(p, &g_tileimage_dinfo.rcdraw);

	_CURSOR_RESTORE
}

/** すべて選択 */

void drawSel_all(DrawData *p)
{
	mRect rc;

	//イメージ再作成

	drawSel_release(p, FALSE);

	if(!drawSel_createImage(p)) return;

	//描画 (キャンバスイメージの範囲)

	_CURSOR_WAIT

	_get_canvimage_rect(p, &rc);

	g_tileimage_dinfo.funcDrawPixel = TileImage_setPixel_new;

	p->w.rgbaDraw.a = 0x8000;

	TileImage_drawFillBox(p->tileimgSel, rc.x1, rc.y1,
		rc.x2 - rc.x1 + 1, rc.y2 - rc.y1 + 1, &p->w.rgbaDraw);

	//範囲セット

	p->sel.rcsel = rc;

	//更新

	drawUpdate_rect_canvas_forSelect(p, &rc);

	_CURSOR_RESTORE
}

/** 塗りつぶし/消去 */

void drawSel_fill_erase(DrawData *p,mBool erase)
{
	mRect rc;

	if(drawOpSub_canDrawLayer_mes(p)) return;

	/* [!] beginDraw/endDraw でカーソルは変更される */

	//範囲

	drawSel_getFullDrawRect(p, &rc);

	//描画

	drawOpSub_setDrawInfo_fillerase(erase);
	drawOpSub_beginDraw_single(p);

	TileImage_drawFillBox(p->w.dstimg, rc.x1, rc.y1,
		rc.x2 - rc.x1 + 1, rc.y2 - rc.y1 + 1, &p->w.rgbaDraw);

	drawOpSub_endDraw_single(p);
}

/** コピー/切り取り */

void drawSel_copy_cut(DrawData *p,mBool cut)
{
	int ret;
	TileImage *img;
	mStr str = MSTR_INIT;

	//選択範囲なし

	if(!drawSel_isHave()) return;

	//カレントがフォルダの場合は除く。ロック時はコピーのみ

	ret = drawOpSub_canDrawLayer(p);

	if(ret == CANDRAWLAYER_FOLDER) return;

	if(ret == CANDRAWLAYER_LOCK) cut = FALSE;

	//現在のコピーイメージを削除

	TileImage_free(p->sel.tileimgCopy);
	p->sel.tileimgCopy = NULL;

	//---------

	_CURSOR_WAIT

	//----- コピー/切り取り

	//切り取り準備

	if(cut)
	{
		drawOpSub_setDrawInfo_select_cut();
		drawOpSub_beginDraw(p);
	}

	//コピー/切り取り

	img = TileImage_createSelCopyImage(p->curlayer->img, p->tileimgSel,
		&p->sel.rcsel, cut, FALSE);

	//切り取り後

	if(cut)
		drawOpSub_finishDraw_single(p);

	//---- イメージ保持
	/* デフォルトで、作業用ディレクトリにファイル保存。
	 * 作業用ディレクトリがない or 保存に失敗した場合は sel.tileimgCopy に保持。 */

	if(img)
	{
		if(ConfigData_getTempPath(&str, _FILENAME_SELCOPY)
			&& TileImage_saveTileImageFile(img, str.buf, p->curlayer->col))
		{
			//ファイル出力に成功した場合、イメージ削除

			TileImage_free(img);
		}
		else
		{
			//ファイル出力できない場合、メモリ上に保持

			p->sel.tileimgCopy = img;
			p->sel.col_copyimg = p->curlayer->col;
		}

		mStrFree(&str);
	}

	//

	_CURSOR_RESTORE
}

/** 新規レイヤに貼り付け */

void drawSel_paste_newlayer(DrawData *p)
{
	mStr str = MSTR_INIT;
	TileImage *img;
	uint32_t col;

	//コピーイメージが存在しない

	if(!p->sel.tileimgCopy
		&& !(ConfigData_getTempPath(&str, _FILENAME_SELCOPY) && mIsFileExist(str.buf, FALSE)))
	{
		mStrFree(&str);
		return;
	}

	//レイヤのイメージ用意 (ファイルの場合、読み込み)

	if(p->sel.tileimgCopy)
	{
		img = TileImage_newClone(p->sel.tileimgCopy);
		col = p->sel.col_copyimg;
	}
	else
		img = TileImage_loadTileImageFile(str.buf, &col);

	mStrFree(&str);

	if(!img) return;

	//新規レイヤ

	if(!drawLayer_newLayer_fromImage(p, img, col))
		TileImage_free(img);
}


/** [スレッド] 拡張/縮小 */

static int _thread_expand(mPopupProgress *prog,void *data)
{
	TileImage_expandSelect(APP_DRAW->tileimgSel, *((int *)data), prog);

	return 1;
}

/** 範囲の拡張/縮小 */

void drawSel_expand(DrawData *p,int cnt)
{
	//準備

	g_tileimage_dinfo.funcColor = TileImage_colfunc_overwrite;

	TileImageDrawInfo_clearDrawRect();

	//スレッド

	PopupThread_run(&cnt, _thread_expand);

	//範囲

	if(cnt > 0)
		p->sel.rcsel = g_tileimage_dinfo.rcdraw;
	else
		drawSel_freeEmpty(p);

	//更新

	drawUpdate_rect_canvas_forSelect(p, &g_tileimage_dinfo.rcdraw);
}


//======================
// 作業用処理
//======================


/** イメージ全体に対して描画する際の範囲取得 */

void drawSel_getFullDrawRect(DrawData *p,mRect *rc)
{
	if(drawSel_isHave())
		*rc = p->sel.rcsel;
	else
		_get_cur_drawrect(p, rc);
}

/** 選択範囲イメージを確保 (範囲セット前に行う)
 *
 * @return FALSE で失敗 */

mBool drawSel_createImage(DrawData *p)
{
	mRect rc;

	if(p->tileimgSel)
		return TRUE;
	else
	{
		//描画可能範囲から作成
		
		_get_cur_drawrect(p, &rc);

		p->tileimgSel = TileImage_newFromRect(TILEIMAGE_COLTYPE_ALPHA1, &rc);

		mRectEmpty(&p->sel.rcsel);

		return (p->tileimgSel != NULL);
	}
}

/** 範囲削除後の処理
 *
 * 透明タイルを解放して範囲を再計算。
 * すべて透明ならイメージ削除。 */

void drawSel_freeEmpty(DrawData *p)
{
	if(p->tileimgSel)
	{
		if(TileImage_freeEmptyTiles(p->tileimgSel))
		{
			//すべて透明なら削除
			
			TileImage_free(p->tileimgSel);
			p->tileimgSel = NULL;
		}
		else
			//点が残っていれば、範囲を再計算
			TileImage_getHaveImageRect_pixel(p->tileimgSel, &p->sel.rcsel, NULL);
	}
}

