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
 * キャンバス関連
 *****************************************/

#include "mDef.h"
#include "mWidgetDef.h"

#include "defMacros.h"
#include "defConfig.h"

#include "defDraw.h"
#include "draw_main.h"
#include "draw_calc.h"

#include "MainWinCanvas.h"
#include "Docks_external.h"



/** キャンバスの描画を低品質に */

void drawCanvas_lowQuality()
{
	APP_DRAW->bCanvasLowQuality = 1;
}

/** キャンバスの描画品質を元に戻す */

void drawCanvas_normalQuality()
{
	APP_DRAW->bCanvasLowQuality = 0;
}

/** スクロールリセット (値のみ)
 *
 * イメージ原点位置をセットして、スクロール位置を (0,0) にする。
 *
 * @param origin イメージ原点位置。NULL で現在のキャンバス中央。 */

void drawCanvas_setScrollReset(DrawData *p,mDoublePoint *origin)
{
	if(origin)
	{
		p->imgoriginX = origin->x;
		p->imgoriginY = origin->y;
	}
	else
		drawCalc_getImagePos_atCanvasCenter(p, &p->imgoriginX, &p->imgoriginY);

	p->ptScroll.x = p->ptScroll.y = 0;
}

/** スクロールリセット (更新も行う) */

void drawCanvas_setScrollReset_update(DrawData *p,mDoublePoint *origin)
{
	drawCanvas_setScrollReset(p, origin);
	drawCanvas_setZoomAndAngle(p, 0, 0, 0, FALSE);
}

/** スクロール位置をデフォルトに (画像の中央を原点位置とする) */

void drawCanvas_setScrollDefault(DrawData *p)
{
	p->imgoriginX = p->imgw * 0.5;
	p->imgoriginY = p->imgh * 0.5;
	p->ptScroll.x = p->ptScroll.y = 0;
}

/** 表示倍率を一段階上げ下げ */

void drawCanvas_zoomStep(DrawData *p,mBool zoomup)
{
	drawCanvas_setZoomAndAngle(p,
		drawCalc_getZoom_step(p, zoomup), 0, 1, TRUE);
}

/** 一段階左右に回転 */

void drawCanvas_rotateStep(DrawData *p,mBool left)
{
	int step;

	step = APP_CONF->canvasAngleStep;
	if(left) step = -step;

	drawCanvas_setZoomAndAngle(p, 0, p->canvas_angle + step * 100, 2, TRUE);
}

/** キャンバスをウィンドウに合わせる
 *
 * イメージ全体がキャンバス内に収まるように倍率セット。 */

void drawCanvas_fitWindow(DrawData *p)
{
	drawCanvas_setScrollDefault(p);

	drawCanvas_setZoomAndAngle(p,
		drawCalc_getZoom_fitWindow(p), 0, 3, FALSE);
}

/** キャンバス左右反転表示切り替え */

void drawCanvas_mirror(DrawData *p)
{
	mDoublePoint pt;

	drawCalc_getImagePos_atCanvasCenter(p, &pt.x, &pt.y);

	p->canvas_mirror ^= 1;

	drawCanvas_setScrollReset(p, &pt);

	drawCanvas_setZoomAndAngle(p, 0, -(p->canvas_angle), 2, FALSE);
}

/** キャンバスの領域サイズ変更時 */

void drawCanvas_setAreaSize(DrawData *p,int w,int h)
{
	p->szCanvas.w = w;
	p->szCanvas.h = h;

	//メインウィンドウ、スクロール情報変更

	MainWinCanvas_setScrollInfo();
}

/** キャンバス表示倍率と回転角度をセット
 *
 * 値を変更せずに更新のみを行う場合にも使う。
 * [!] キャンバスのスクロールバー範囲が変更される。
 *
 * @param flags [0bit] 倍率変更 [1bit] 角度変更 [2bit]キャンバス更新しない
 * @param reset_scroll スクロールを現在のキャンバス中央位置にリセット */

void drawCanvas_setZoomAndAngle(DrawData *p,int zoom,int angle,int flags,mBool reset_scroll)
{
	//スクロールリセット
	//[!] すでにスクロール位置が (0,0) ならそのまま

	if(reset_scroll && (p->ptScroll.x || p->ptScroll.y))
		drawCanvas_setScrollReset(p, NULL);

	//倍率

	if(flags & 1)
	{
		if(zoom < CANVAS_ZOOM_MIN)
			zoom = CANVAS_ZOOM_MIN;
		else if(zoom > CANVAS_ZOOM_MAX)
			zoom = CANVAS_ZOOM_MAX;

		p->canvas_zoom = zoom;
	}

	//角度

	if(flags & 2)
	{
		if(angle < -18000)
			angle += 36000;
		else if(angle > 18000)
			angle -= 36000;

		p->canvas_angle = angle;
	}

	//CanvasViewParam セット

	drawCalc_setCanvasViewParam(p);

	//メインウィンドウのスクロール

	MainWinCanvas_setScrollInfo();

	//ほか、関連するウィジェット

	DockCanvasCtrl_setPos();

	//更新

	if(!(flags & DRAW_SETZOOMANDANGLE_F_NO_UPDATE))
		drawUpdate_canvasArea();
}
