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
 * 更新関連
 *****************************************/

#include "mDef.h"
#include "mWindowDef.h"
#include "mWidget.h"
#include "mPixbuf.h"
#include "mRectBox.h"

#include "defConfig.h"
#include "defDraw.h"
#include "defWidgets.h"

#include "draw_op_def.h"
#include "draw_main.h"
#include "draw_calc.h"

#include "ImageBufRGB16.h"
#include "LayerList.h"
#include "LayerItem.h"
#include "TileImage.h"
#include "PixbufDraw.h"

#include "Docks_external.h"



/** キャンバスエリアを更新 */

void drawUpdate_canvasArea()
{
	mWidgetUpdate(M_WIDGET(APP_WIDGETS->canvas_area));
}

/** すべて更新
 *
 * [ 合成イメージ、キャンバスエリア、(Dock)キャンバスビュー ] */

void drawUpdate_all()
{
	drawUpdate_blendImage_full(APP_DRAW);
	drawUpdate_canvasArea();

	DockCanvasView_update();
}

/** 全体 + レイヤ一覧を更新 */

void drawUpdate_all_layer()
{
	drawUpdate_all();

	DockLayer_update_all();
}

/** 合成イメージを全体更新
 *
 * 背景とレイヤイメージ */

void drawUpdate_blendImage_full(DrawData *p)
{
	mBox box;

	box.x = box.y = 0;
	box.w = p->imgw, box.h = p->imgh;

	//背景

	if(APP_CONF->fView & CONFIG_VIEW_F_BKGND_PLAID)
	{
		ImageBufRGB16_fillPlaidBox(p->blendimg, &box,
			p->rgb15BkgndPlaid, p->rgb15BkgndPlaid + 1);
	}
	else
		ImageBufRGB16_fill(p->blendimg, &p->rgb15ImgBkgnd);

	//合成

	drawUpdate_blendImage_layer(p, &box);
}

/** 合成イメージを更新 (範囲) */

void drawUpdate_blendImage_box(DrawData *p,mBox *box)
{
	//背景

	if(APP_CONF->fView & CONFIG_VIEW_F_BKGND_PLAID)
	{
		ImageBufRGB16_fillPlaidBox(p->blendimg, box,
			p->rgb15BkgndPlaid, p->rgb15BkgndPlaid + 1);
	}
	else
		ImageBufRGB16_fillBox(p->blendimg, box, &p->rgb15ImgBkgnd);

	//合成

	drawUpdate_blendImage_layer(p, box);
}

/** 合成イメージ:レイヤイメージを範囲更新
 *
 * [!] 背景は描画しない */

void drawUpdate_blendImage_layer(DrawData *p,mBox *box)
{
	LayerItem *pi;
	int opacity;

	pi = LayerList_getItem_bottomVisibleImage(p->layerlist);

	if(p->w.optype == DRAW_OPTYPE_SELIMGMOVE && p->tileimgTmp)
	{
		//----- 選択範囲イメージの移動/コピー中

		for(; pi; pi = LayerItem_getPrevVisibleImage(pi))
		{
			opacity = LayerItem_getOpacity_real(pi);
		
			TileImage_blendToImageRGB16(pi->img, p->blendimg,
				box, opacity, pi->blendmode, pi->img8texture);

			//tileimgTmp をカレントの上に挿入

			if(pi == p->curlayer)
			{
				TileImage_blendToImageRGB16(p->tileimgTmp,
					p->blendimg, box, opacity, pi->blendmode, pi->img8texture);
			}
		}
	}
	else if(p->drawtext.in_dialog
		&& (p->drawtext.flags & DRAW_DRAWTEXT_F_PREVIEW))
	{
		//----- テキスト描画、ダイアログ中のプレビュー

		for(; pi; pi = LayerItem_getPrevVisibleImage(pi))
		{
			opacity = LayerItem_getOpacity_real(pi);
		
			TileImage_blendToImageRGB16(pi->img, p->blendimg,
				box, LayerItem_getOpacity_real(pi), pi->blendmode, pi->img8texture);

			//プレビューイメージを挿入

			if(pi == p->curlayer)
			{
				TileImage_blendToImageRGB16(p->tileimgTmp,
					p->blendimg, box, opacity, pi->blendmode, pi->img8texture);
			}
		}
	}
	else if(p->w.in_filter_dialog && p->w.tileimg_filterprev)
	{
		//---- フィルタの漫画用プレビュー時

		for(; pi; pi = LayerItem_getPrevVisibleImage(pi))
		{
			opacity = LayerItem_getOpacity_real(pi);
		
			TileImage_blendToImageRGB16(pi->img, p->blendimg,
				box, LayerItem_getOpacity_real(pi), pi->blendmode, pi->img8texture);

			//プレビューイメージを挿入

			if(pi == p->curlayer)
			{
				TileImage_blendToImageRGB16(p->w.tileimg_filterprev,
					p->blendimg, box, opacity, pi->blendmode, NULL);
			}
		}
	}
	else
	{
		//----- 通常時
		
		for(; pi; pi = LayerItem_getPrevVisibleImage(pi))
		{
			TileImage_blendToImageRGB16(pi->img, p->blendimg,
				box, LayerItem_getOpacity_real(pi), pi->blendmode, pi->img8texture);
		}
	}
}

/** キャンバスを描画
 *
 * 合成イメージをソースとして mPixbuf に描画。
 *
 * @param pixbuf 描画対象のキャンバスエリアの mPixbuf
 * @param box    NULL で全体 */

void drawUpdate_drawCanvas(DrawData *p,mPixbuf *pixbuf,mBox *box)
{
	CanvasDrawInfo di;
	mBox boximg;
	int n,w,h;

	//描画情報

	if(box)
		di.boxdst = *box;
	else
	{
		di.boxdst.x = di.boxdst.y = 0;
		di.boxdst.w = p->szCanvas.w;
		di.boxdst.h = p->szCanvas.h;
	}

	di.originx = p->imgoriginX;
	di.originy = p->imgoriginY;
	di.scrollx = p->ptScroll.x - (p->szCanvas.w >> 1);
	di.scrolly = p->ptScroll.y - (p->szCanvas.h >> 1);
	di.mirror  = p->canvas_mirror;
	di.imgw = p->imgw;
	di.imgh = p->imgh;
	di.bkgndcol = APP_CONF->colCanvasBkgnd;
	di.param = &p->viewparam;

	//------ 描画

	if(p->canvas_angle)
	{
		//回転あり

		n = p->canvas_angle;
		
		if(p->bCanvasLowQuality
			|| (p->canvas_zoom == 1000 && (n == 9000 || n == 18000 || n == 27000)))
			//キャンバス移動中、または倍率100%で90度単位の場合は低品質
			ImageBufRGB16_drawMainCanvas_rotate_normal(p->blendimg, pixbuf, &di);
		else
			ImageBufRGB16_drawMainCanvas_rotate_oversamp(p->blendimg, pixbuf, &di);
	}
	else
	{
		//回転なし (200%以下 [!] 100%は除く 時は高品質で)

		if(!p->bCanvasLowQuality && p->canvas_zoom != 1000 && p->canvas_zoom < 2000)
			ImageBufRGB16_drawMainCanvas_oversamp(p->blendimg, pixbuf, &di);
		else 
			ImageBufRGB16_drawMainCanvas_nearest(p->blendimg, pixbuf, &di);
	}

	//------ 描画先キャンバス範囲内のイメージ範囲を元にした描画

	if(drawCalc_areaToimage_box(p, &boximg, &di.boxdst))
	{
		//グリッド
		
		if(APP_CONF->fView & CONFIG_VIEW_F_GRID)
		{
			pixbufDrawGrid(pixbuf, &di.boxdst, &boximg,
				APP_CONF->grid.gridw, APP_CONF->grid.gridh, APP_CONF->grid.col_grid, &di);
		}

		//分割線

		if(APP_CONF->fView & CONFIG_VIEW_F_GRID_SPLIT)
		{
			w = p->imgw / APP_CONF->grid.splith;
			h = p->imgh / APP_CONF->grid.splitv;

			if(w < 2) w = 2;
			if(h < 2) h = 2;
			
			pixbufDrawGrid(pixbuf, &di.boxdst, &boximg,
				w, h, APP_CONF->grid.col_split, &di);
		}

		//選択範囲

		if(p->tileimgSel && !mRectIsEmpty(&p->sel.rcsel) && !p->w.hide_canvas_select)
			TileImage_drawSelectEdge(p->tileimgSel, pixbuf, &di, &boximg);
	}

	//矩形編集枠

	if(p->boxedit.box.w)
		pixbufDrawBoxEditFrame(pixbuf, &p->boxedit.box, &di);
}

/** 合成イメージの範囲更新
 *
 * @param boximg  イメージ範囲内であること */

void drawUpdate_rect_blendimg(DrawData *p,mBox *boximg)
{
	//背景

	if(APP_CONF->fView & CONFIG_VIEW_F_BKGND_PLAID)
	{
		ImageBufRGB16_fillPlaidBox(p->blendimg, boximg,
			p->rgb15BkgndPlaid, p->rgb15BkgndPlaid + 1);
	}
	else
		ImageBufRGB16_fillBox(p->blendimg, boximg, &p->rgb15ImgBkgnd);

	//レイヤ

	drawUpdate_blendImage_layer(p, boximg);
}

/** キャンバスエリアの範囲更新 */

void drawUpdate_rect_canvas(DrawData *p,mBox *boximg)
{
	mBox boxc;
	mWidget *wgarea;
	mPixbuf *pixbuf;

	wgarea = M_WIDGET(APP_WIDGETS->canvas_area);

	if(drawCalc_imageToarea_box(p, &boxc, boximg))
	{
		//キャンバス描画

		pixbuf = mWidgetBeginDirectDraw(wgarea);

		if(pixbuf)
		{
			drawUpdate_drawCanvas(p, pixbuf, &boxc);

			//[debug]
			//mPixbufBox(pixbuf, boxc.x, boxc.y, boxc.w, boxc.h, 0xff0000);

			mWidgetEndDirectDraw(wgarea, pixbuf);

			//ウィンドウ更新

			mWidgetUpdateBox_box(wgarea, &boxc);
		}
	}
}


//===========================
// 範囲更新
//===========================


/** 範囲更新 (キャンバス) : 選択範囲用 */

void drawUpdate_rect_canvas_forSelect(DrawData *p,mRect *rc)
{
	mBox box;
	int n;

	if(!mRectIsEmpty(rc))
	{
		mBoxSetByRect(&box, rc);

		//キャンバス座標上における 2px 分を拡張
	
		n = (int)(p->viewparam.scalediv * 2 + 0.5);
		if(n == 0) n = 1;

		box.x -= n, box.y -= n;
		n <<= 1;
		box.w += n, box.h += n;

		drawUpdate_rect_canvas(p, &box);
	}
}

/** 範囲更新 (キャンバス) : 矩形編集用 */

void drawUpdate_rect_canvas_forBoxEdit(DrawData *p,mBox *boxsel)
{
	mBox box;
	int n;

	if(boxsel->w)
	{
		//キャンバス座標上における 4px 分を拡張
	
		n = (int)(p->viewparam.scalediv * 4 + 0.5);
		if(n == 0) n = 1;

		box = *boxsel;

		box.x -= n, box.y -= n;
		n <<= 1;
		box.w += n, box.h += n;

		drawUpdate_rect_canvas(p, &box);
	}
}


/** 範囲更新 (合成イメージ + キャンバスエリア)
 *
 * @param boximg イメージ範囲内であること */

void drawUpdate_rect_imgcanvas(DrawData *p,mBox *boximg)
{
	//合成イメージ

	drawUpdate_rect_blendimg(p, boximg);

	//キャンバス

	drawUpdate_rect_canvas(p, boximg);
}

/** 範囲更新 (mRect)
 *
 * @param rc  イメージ範囲 */

void drawUpdate_rect_imgcanvas_fromRect(DrawData *p,mRect *rc)
{
	mBox box;

	if(drawCalc_getImageBox_rect(p, &box, rc))
		drawUpdate_rect_imgcanvas(p, &box);
}

/** 範囲更新 (選択範囲用) */

void drawUpdate_rect_imgcanvas_forSelect(DrawData *p,mRect *rc)
{
	mBox box;

	//合成イメージ

	if(drawCalc_getImageBox_rect(p, &box, rc))
		drawUpdate_rect_blendimg(p, &box);

	//キャンバス

	drawUpdate_rect_canvas_forSelect(p, rc);
}


/** 範囲更新 + [dock]キャンバスビュー */

void drawUpdate_rect_imgcanvas_canvasview_fromRect(DrawData *p,mRect *rc)
{
	mBox box;

	if(drawCalc_getImageBox_rect(p, &box, rc))
	{
		drawUpdate_rect_imgcanvas(p, &box);

		DockCanvasView_updateRect(&box);
	}
}

/** 範囲更新 + [dock]キャンバスビュー (レイヤの表示イメージ部分)
 *
 * フォルダの場合、フォルダ下の全ての表示レイヤの範囲。
 *
 * @param item  NULL でカレントレイヤ */

void drawUpdate_rect_imgcanvas_canvasview_inLayerHave(DrawData *p,LayerItem *item)
{
	mRect rc;

	if(!item) item = p->curlayer;

	if(LayerItem_getVisibleImageRect(item, &rc))
		drawUpdate_rect_imgcanvas_canvasview_fromRect(p, &rc);
}

/** [dock]キャンバスビューのみ更新 (mRect) */

void drawUpdate_canvasview(DrawData *p,mRect *rc)
{
	mBox box;

	if(drawCalc_getImageBox_rect(p, &box, rc))
		DockCanvasView_updateRect(&box);
}

/** [dock]キャンバスビューがルーペ時、更新
 *
 * ドットペンの押し時など、カーソルが移動されていない状態でイメージが更新されたときに使う。 */

void drawUpdate_canvasview_inLoupe()
{
	if(APP_CONF->canvasview_flags & CONFIG_CANVASVIEW_F_LOUPE_MODE)
		DockCanvasView_update();
}

/** 描画終了時の範囲更新
 *
 * @param boximg イメージ範囲。x < 0 で範囲なし */

void drawUpdate_endDraw_box(DrawData *p,mBox *boximg)
{
	//[Dock]キャンバスビュー

	if(boximg->x >= 0)
		DockCanvasView_updateRect(boximg);
}
