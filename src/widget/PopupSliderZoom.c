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

/**************************************************
 * PopupSliderZoom
 * 
 * キャンバスビュー/イメージビューアの倍率変更用。
 * ポップアップのスライダー
 **************************************************/

#include <stdio.h>

#include "mDef.h"
#include "mWidget.h"
#include "mWindow.h"
#include "mPopupWindow.h"
#include "mSliderBar.h"
#include "mToolTip.h"


//--------------------------

typedef struct
{
	mWidget wg;
	mContainerData ct;
	mWindowData win;
	mPopupWindowData pop;

	mSliderBar *slider;
	mToolTip *tooltip;
	void *param;
	void (*handle)(int,void*);
	
	int zoom_max,		//最大値
		step_low,		//100% 以下時の1段階 (0 で 100% 以下なし)
		step_hi,		//100% 以上時の1段階
		slider_center;	//100% の位置
}PopupSliderZoom;

//--------------------------



/** 倍率からスライダー位置取得 */

static int _zoom_to_sliderpos(PopupSliderZoom *p,int zoom)
{
	if(zoom >= 100)
		return (p->zoom_max - zoom) / p->step_hi;
	else
		return (100 - zoom) / p->step_low + p->slider_center;
}

/** スライダー位置から倍率取得 */

static int _sliderpos_to_zoom(PopupSliderZoom *p,int pos)
{
	if(pos < p->slider_center)
		return (p->slider_center - pos) * p->step_hi + 100;
	else
		return 100 - (pos - p->slider_center) * p->step_low;
}

/** ツールチップセット */

static void _set_tooltip(PopupSliderZoom *p,int zoom)
{
	char m[16];

	snprintf(m, 16, "%d%%", zoom);

	p->tooltip = mToolTipShow(p->tooltip, p->wg.x + p->wg.w, p->wg.y, m);
}

/** スライダーハンドラ */

static void _slider_handle(mSliderBar *sl,int pos,int flags)
{
	PopupSliderZoom *p = (PopupSliderZoom *)sl->wg.parent;
	int zoom;

	if(flags & MSLIDERBAR_HANDLE_F_CHANGE)
	{
		zoom = _sliderpos_to_zoom(p, pos);
	
		_set_tooltip(p, zoom);

		if(p->handle)
			(p->handle)(zoom, p->param);
	}
}

/** ポップアップ実行
 *
 * @param step_low  100% 以下の時の1段階 (0 で 100% 以下なし)
 * @param step_hi   100% 以上の時の1段階
 * @return 結果の倍率が返る */

int PopupSliderZoom_run(int rootx,int rooty,
	int min,int max,int current,int step_low,int step_hi,
	void (*handle)(int,void *),void *param)
{
	PopupSliderZoom *p;

	//作成

	p = (PopupSliderZoom *)mPopupWindowNew(sizeof(PopupSliderZoom), NULL, MWINDOW_S_NO_IM);
	if(!p) return current;

	p->wg.draw = mWidgetHandleFunc_draw_boxFrame;	//枠を描画

	p->handle = handle;
	p->param = param;

	p->zoom_max = max;
	p->step_hi = step_hi;
	p->step_low = step_low;
	p->slider_center = (max - 100) / step_hi;

	//スライダーバー

	p->slider = mSliderBarNew(0, M_WIDGET(p), MSLIDERBAR_S_VERT);

	p->slider->wg.fLayout = MLF_EXPAND_H;
	p->slider->sl.handle = _slider_handle;

	mWidgetSetMargin_one(M_WIDGET(p->slider), 1);	//枠の幅分

	mSliderBarSetStatus(p->slider, 0,
		p->slider_center + ((step_low)? (100 - min) / step_low: 0),
		_zoom_to_sliderpos(p, current));

	//サイズ

	mWidgetResize(M_WIDGET(p), p->slider->wg.hintW + 2, 140);

	//位置

	mWindowMoveAdjust(M_WINDOW(p), rootx, rooty, FALSE);

	//ツールチップ

	_set_tooltip(p, current);

	//実行

	mPopupWindowRun_show(M_POPUPWINDOW(p));

	//終了

	current = _sliderpos_to_zoom(p, p->slider->sl.pos);

	mWidgetDestroy(M_WIDGET(p->tooltip));
	mWidgetDestroy(M_WIDGET(p));

	return current;
}
