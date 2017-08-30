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
 * 矩形編集
 *****************************************/

#include "mDef.h"

#include "defWidgets.h"
#include "defDraw.h"

#include "draw_main.h"
#include "draw_calc.h"
#include "draw_op_sub.h"

#include "TileImage.h"
#include "TileImageDrawInfo.h"
#include "LayerItem.h"

#include "MainWindow.h"
#include "MainWinCanvas.h"
#include "PopupThread.h"

#include "AppErr.h"


//------------

int BoxEditTransform_run(mWindow *owner,TileImage *imgcopy,double *dst,mRect *rcdst);

//------------


/** 矩形編集時、リサイズカーソル状態の場合、元に戻す */

void drawBoxEdit_restoreCursor(DrawData *p)
{
	if(p->boxedit.cursor_resize)
	{
		MainWinCanvasArea_setCursor_forTool();

		p->boxedit.cursor_resize = 0;
	}
}

/** 範囲をクリア (更新なし) */

void drawBoxEdit_clear_noupdate(DrawData *p)
{
	if(p->boxedit.box.w)
	{
		p->boxedit.box.w = 0;

		drawBoxEdit_restoreCursor(p);
	}
}

/** 矩形編集の範囲セット
 *
 * @param rcsel NULL で解除 */

void drawBoxEdit_setBox(DrawData *p,mRect *rcsel)
{
	mBox box;

	if(!rcsel)
	{
		//解除

		if(p->boxedit.box.w)
		{
			//カーソル戻す

			drawBoxEdit_restoreCursor(p);

			//

			box = p->boxedit.box;

			p->boxedit.box.w = 0;

			drawUpdate_rect_canvas_forBoxEdit(p, &box);

		}
	}
	else
	{
		//セット (1x1 でなし)

		if(drawCalc_getImageBox_rect(p, &box, rcsel))
		{
			if(box.w != 1 && box.h != 1)
			{
				p->boxedit.box = box;

				drawUpdate_rect_canvas_forBoxEdit(p, &box);
			}
		}
	}
}


//============================
// 実行
//============================


/** 左右/上下反転、90度回転 */

static void _run_reverse_and_rotate90(DrawData *p,int type)
{
	TileImage *img = NULL;
	mBox box;

	if(drawOpSub_canDrawLayer_mes(p)) return;

	box = p->boxedit.box;

	//----------

	drawOpSub_setDrawInfo(p, TOOL_BOXEDIT, 0);
	drawOpSub_beginDraw_single(p);

	//回転時は、範囲内イメージをコピー & クリア
	/* [!] 描画を開始しているので、イメージの作成失敗時も続ける */

	if(type >= 2)
		img = TileImage_createCopyImage_box(p->w.dstimg, &box, TRUE);

	//描画

	switch(type)
	{
		case 0:
			TileImage_rectReverse_horz(p->w.dstimg, &box);
			break;
		case 1:
			TileImage_rectReverse_vert(p->w.dstimg, &box);
			break;
		case 2:
			TileImage_rectRotate_left(p->w.dstimg, img, &box);
			break;
		case 3:
			TileImage_rectRotate_right(p->w.dstimg, img, &box);
			break;
	}

	TileImage_free(img);

	drawOpSub_endDraw_single(p);
}

/** トリミング */

static void _run_trimming(DrawData *p)
{
	mBox box;

	box = p->boxedit.box;

	if(!drawImage_resizeCanvas(p, box.w, box.h, -box.x, -box.y, TRUE))
		MainWindow_apperr(APPERR_ALLOC, NULL);

	MainWindow_updateNewCanvas(APP_WIDGETS->mainwin, NULL);
}


//=======================
// 変形
//=======================


typedef struct
{
	int type;
	TileImage *imgdst,*imgcopy;
	double *dparam;
	mBox box;
	mRect *rcdst;
}_thdata_trans;


/** スレッド */

static int _thread_transform(mPopupProgress *prog,void *data)
{
	_thdata_trans *p = (_thdata_trans *)data;
	double *d = p->dparam;

	if(p->type == 0)
	{
		//アフィン変換
		
		TileImage_transformAffine(p->imgdst, p->imgcopy, &p->box,
			d[0], d[1], d[2], d[3], d[4], d[5], prog);
	}
	else
	{
		//射影変換

		TileImage_transformHomography(p->imgdst, p->imgcopy, p->rcdst,
			d, p->box.x, p->box.y, p->box.w, p->box.h, prog);
	}

	return 1;
}

/** 変形、描画処理 */

static void _draw_transform(DrawData *p,TileImage *imgcopy,int type,double *d,mRect *rcdst)
{
	mBox box;
	_thdata_trans dat;

	box = p->boxedit.box;

	drawOpSub_setDrawInfo(p, TOOL_BOXEDIT, 0);
	drawOpSub_beginDraw_single(p);

	//範囲内を消去
	//(setDrawInfo() で色関数は上書きにセットされている)

	TileImage_clearImageBox_forTransform(p->w.dstimg, p->tileimgSel, &box);

	//変形描画は重ね塗り

	g_tileimage_dinfo.funcColor = TileImage_colfunc_normal;

	//スレッド

	dat.type = type;
	dat.imgdst = p->w.dstimg;
	dat.imgcopy = imgcopy;
	dat.dparam = d;
	dat.box = box;
	dat.rcdst = rcdst;

	PopupThread_run(&dat, _thread_transform);

	drawOpSub_endDraw_single(p);
}

/** 変形 */

static void _run_transform(DrawData *p)
{
	TileImage *imgcopy;
	mBox box;
	mRect rc;
	double d[11];
	int ret;

	if(drawOpSub_canDrawLayer_mes(p)) return;

	box = p->boxedit.box;

	//[!] ダイアログ中も XOR 描画などで使用する

	g_tileimage_dinfo.funcDrawPixel = TileImage_setPixel_new;

	//範囲内イメージをコピー＆消去

	imgcopy = TileImage_createCopyImage_forTransform(p->curlayer->img, p->tileimgSel, &box);
	if(!imgcopy) return;

	//プレビュー用に blendimg を更新
	//(キャンバスは更新しないので画面上は変更なし)

	drawUpdate_blendImage_box(p, &box);

	//ダイアログ

	ret = BoxEditTransform_run(M_WINDOW(APP_WIDGETS->mainwin), imgcopy, d, &rc);

	//消去したイメージを一旦元に戻す

	TileImage_restoreCopyImage_box(p->curlayer->img, imgcopy, &box);

	//

	if(ret >= 0)
		//描画
		_draw_transform(p, imgcopy, ret, d, &rc);
	else
	{
		//キャンセル時は blendimg を元に戻す
		//(OK 時は描画時に更新されるので必要ない)

		drawUpdate_blendImage_box(p, &box);
	}

	//イメージ解放

	TileImage_free(imgcopy);
}


//=======================


/** 編集実行 */

void drawBoxEdit_run(DrawData *p,int type)
{
	if(p->boxedit.box.w == 0) return;

	switch(type)
	{
		case 0:
		case 1:
		case 2:
		case 3:
			_run_reverse_and_rotate90(p, type);
			break;
		case 4:
			_run_transform(p);
			break;
		case 5:
			_run_trimming(p);
			break;
	}
}
