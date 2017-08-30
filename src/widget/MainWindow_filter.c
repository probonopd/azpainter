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
 * MainWindow
 *
 * フィルタ関連
 *****************************************/

#include "mDef.h"
#include "mRectBox.h"

#include "defDraw.h"
#include "defWidgets.h"
#include "AppErr.h"

#include "MainWindow.h"
#include "PopupThread.h"

#include "LayerItem.h"
#include "LayerList.h"
#include "TileImage.h"
#include "TileImageDrawInfo.h"
#include "FilterDrawInfo.h"

#include "draw_main.h"
#include "draw_calc.h"
#include "draw_select.h"
#include "draw_op_sub.h"

#include "trid_mainmenu.h"

#include "FilterDrawFunc.h"
#include "dataFilterDlg.h"


//-----------------

enum
{
	_FDF_PROC_COLOR = 1<<0,		//RGBを対象としたカラー処理である (不透明タイルのみ処理)
	_FDF_COPYSRC    = 1<<1,		//実際の処理時、描画先をコピーしてソースとする
	_FDF_CLIPPING   = 1<<2,		//常に、イメージ範囲外は処理しない & ソース画像クリッピング
	_FDF_NODLG_INDEX = 1<<3,	//ダイアログなし。dlgdat を数値にして ntmp[0] へセット
	_FDF_CHECKED_LAYER = 1<<4,	//チェックレイヤがあるか確認する
};

//-----------------

typedef struct
{
	const uint8_t *dlgdat;
	FilterDrawFunc func;
	uint8_t flags;
}_cmddat;

#define _INTTODAT(n)  (const uint8_t *)(n)

static _cmddat g_fcmddat[] = {
	//カラー
	{g_col_brightconst, FilterDraw_color_brightcont, _FDF_PROC_COLOR},
	{g_col_gamma, FilterDraw_color_gamma, _FDF_PROC_COLOR},
	{g_col_level, FilterDraw_color_level, _FDF_PROC_COLOR},
	{g_col_rgb, FilterDraw_color_rgb, _FDF_PROC_COLOR},
	{g_col_hsv, FilterDraw_color_hsv, _FDF_PROC_COLOR},
	{NULL, FilterDraw_color_nega, _FDF_PROC_COLOR},
	{NULL, FilterDraw_color_grayscale, _FDF_PROC_COLOR},
	{NULL, FilterDraw_color_sepia, _FDF_PROC_COLOR},
	{NULL, FilterDraw_color_gradmap, _FDF_PROC_COLOR},
	{g_col_threshold, FilterDraw_color_threshold, _FDF_PROC_COLOR},
	{g_col_threshold_dither, FilterDraw_color_threshold_dither, _FDF_PROC_COLOR},
	{g_col_posterize, FilterDraw_color_posterize, _FDF_PROC_COLOR},
	{g_col_reducecol, FilterDraw_mediancut, _FDF_PROC_COLOR},

	//色置換
	{g_col_replace_drawcol, FilterDraw_color_replace_drawcol, _FDF_PROC_COLOR},
	{_INTTODAT(0), FilterDraw_color_replace, _FDF_PROC_COLOR | _FDF_NODLG_INDEX},
	{_INTTODAT(1), FilterDraw_color_replace, _FDF_PROC_COLOR | _FDF_NODLG_INDEX},
	{_INTTODAT(2), FilterDraw_color_replace, _FDF_PROC_COLOR | _FDF_NODLG_INDEX},
	{_INTTODAT(3), FilterDraw_color_replace, _FDF_NODLG_INDEX},

	//アルファ操作(チェックレイヤ)
	{_INTTODAT(0), FilterDraw_alpha_checked, _FDF_NODLG_INDEX | _FDF_CHECKED_LAYER},
	{_INTTODAT(1), FilterDraw_alpha_checked, _FDF_NODLG_INDEX | _FDF_CHECKED_LAYER},
	{_INTTODAT(2), FilterDraw_alpha_checked, _FDF_NODLG_INDEX | _FDF_CHECKED_LAYER},
	{_INTTODAT(3), FilterDraw_alpha_checked, _FDF_NODLG_INDEX | _FDF_CHECKED_LAYER},
	{_INTTODAT(4), FilterDraw_alpha_checked, _FDF_NODLG_INDEX | _FDF_CHECKED_LAYER},
	{_INTTODAT(5), FilterDraw_alpha_checked, _FDF_NODLG_INDEX | _FDF_CHECKED_LAYER},
	{_INTTODAT(6), FilterDraw_alpha_checked, _FDF_NODLG_INDEX | _FDF_CHECKED_LAYER},
	{_INTTODAT(7), FilterDraw_alpha_checked, _FDF_NODLG_INDEX | _FDF_CHECKED_LAYER},

	//アルファ操作(カレント)
	{_INTTODAT(0), FilterDraw_alpha_current, _FDF_NODLG_INDEX},
	{_INTTODAT(1), FilterDraw_alpha_current, _FDF_NODLG_INDEX},
	{_INTTODAT(2), FilterDraw_alpha_current, _FDF_NODLG_INDEX},
	{_INTTODAT(3), FilterDraw_alpha_current, _FDF_NODLG_INDEX},

	//ぼかし
	{g_blur_blur, FilterDraw_blur, _FDF_COPYSRC},
	{g_blur_gauss, FilterDraw_gaussblur, _FDF_COPYSRC},
	{g_blur_motion, FilterDraw_motionblur, _FDF_COPYSRC},
	{g_blur_radial, FilterDraw_radialblur, _FDF_COPYSRC},
	{g_blur_lens, FilterDraw_lensblur, _FDF_COPYSRC},

	//描画
	{g_draw_cloud, FilterDraw_cloud, 0},
	{g_draw_amitone1, FilterDraw_amitone1, _FDF_CLIPPING},
	{g_draw_amitone2, FilterDraw_amitone2, _FDF_CLIPPING},
	{g_draw_rndpoint, FilterDraw_draw_randpoint, 0},
	{g_draw_edgepoint, FilterDraw_draw_edgepoint, _FDF_COPYSRC},
	{g_draw_frame, FilterDraw_draw_frame, _FDF_CLIPPING},
	{g_draw_horzvert_line, FilterDraw_draw_horzvertLine, _FDF_CLIPPING},
	{g_draw_plaid, FilterDraw_draw_plaid, _FDF_CLIPPING},

	//漫画用
	{g_comic_concline, FilterDraw_comic_concline_flash, 0},
	{g_comic_flash, FilterDraw_comic_concline_flash, 0},
	{g_comic_popupflash, FilterDraw_comic_popupflash, 0},
	{g_comic_uniflash, FilterDraw_comic_uniflash, 0},
	{g_comic_uniflash_wave, FilterDraw_comic_uniflash_wave, 0},

	//ピクセレート
	{g_pix_mozaic, FilterDraw_mozaic, 0},
	{g_pix_crystal, FilterDraw_crystal, 0},
	{g_pix_halftone, FilterDraw_halftone, 0},

	//輪郭
	{g_edge_sharp, FilterDraw_sharp, _FDF_PROC_COLOR|_FDF_COPYSRC|_FDF_CLIPPING},
	{g_edge_unsharpmask, FilterDraw_unsharpmask, _FDF_PROC_COLOR|_FDF_COPYSRC},
	{NULL, FilterDraw_sobel, _FDF_PROC_COLOR|_FDF_COPYSRC|_FDF_CLIPPING},
	{NULL, FilterDraw_laplacian, _FDF_PROC_COLOR|_FDF_COPYSRC|_FDF_CLIPPING},
	{g_edge_highpass, FilterDraw_highpass, _FDF_PROC_COLOR|_FDF_COPYSRC},

	//効果
	{g_eff_glow, FilterDraw_effect_glow, _FDF_COPYSRC},
	{g_eff_rgbshift, FilterDraw_effect_rgbshift, _FDF_COPYSRC},
	{g_eff_oilpaint, FilterDraw_effect_oilpaint, _FDF_COPYSRC},
	{g_eff_emboss, FilterDraw_effect_emboss, _FDF_PROC_COLOR|_FDF_COPYSRC},
	{g_eff_noise, FilterDraw_effect_noise, _FDF_PROC_COLOR},
	{g_eff_diffusion, FilterDraw_effect_diffusion, _FDF_COPYSRC},
	{g_eff_scratch, FilterDraw_effect_scratch, _FDF_COPYSRC},
	{g_eff_median, FilterDraw_effect_median, _FDF_COPYSRC},
	{g_eff_blurring, FilterDraw_effect_blurring, _FDF_COPYSRC},

	//変形
	{g_trans_wave, FilterDraw_trans_wave, _FDF_COPYSRC},
	{g_trans_ripple, FilterDraw_trans_ripple, _FDF_COPYSRC},
	{g_trans_polar, FilterDraw_trans_polar, _FDF_COPYSRC|_FDF_CLIPPING},
	{g_trans_radial_shift, FilterDraw_trans_radial_shift, _FDF_COPYSRC},
	{g_trans_swirl, FilterDraw_trans_swirl, _FDF_COPYSRC|_FDF_CLIPPING},

	//ほか
	{NULL, FilterDraw_lumtoAlpha, 0},
	{NULL, FilterDraw_dot_thinning, _FDF_CLIPPING},
	{g_other_antialiasing, FilterDraw_antialiasing, _FDF_COPYSRC|_FDF_CLIPPING},
	{g_other_hemming, FilterDraw_hemming, _FDF_COPYSRC},
	{g_other_3Dframe, FilterDraw_3Dframe, _FDF_CLIPPING},
	{g_other_shift, FilterDraw_shift, _FDF_COPYSRC|_FDF_CLIPPING},

	//---- 追加

	{_INTTODAT(4), FilterDraw_alpha_current, _FDF_NODLG_INDEX}	//透明以外を完全不透明に
};

//-----------------

mBool FilterDlg_run(mWindow *owner,uint16_t menuid,const uint8_t *dat,FilterDrawInfo *info);

//-----------------



/** 実行前にメッセージ表示
 *
 * @return TRUE で実行を中止 */

static mBool _show_message(int menuid,uint8_t flags)
{
	int ret,id = -1,coltype;

	coltype = APP_DRAW->curlayer->coltype;

	ret = drawOpSub_canDrawLayer(APP_DRAW);

	if(ret == CANDRAWLAYER_FOLDER)
		//フォルダ
		id = APPERR_CANNOTDRAW_FOLDER;

	else if(ret == CANDRAWLAYER_LOCK)
		//レイヤロック
		id = APPERR_CANNOTDRAW_LOCK;

	else if((flags & _FDF_PROC_COLOR)
		&& (coltype == TILEIMAGE_COLTYPE_ALPHA16 || coltype == TILEIMAGE_COLTYPE_ALPHA1))
		//RGB処理で、レイヤのカラータイプがアルファ値のみの場合
		id = APPERR_FILTER_COLTYPE;

	else if((flags & _FDF_CHECKED_LAYER) && !LayerList_haveCheckedLayer(APP_DRAW->layerlist))
		//チェックレイヤがあるか
		id = APPERR_NONE_CHECKED_LAYER;

	else if(menuid == TRMENU_FILTER_ALPHA2_TEXTURE && !APP_DRAW->img8OptTex)
		//「テクスチャ適用」で、テクスチャイメージがない
		id = APPERR_NONE_OPT_TEXTURE;
	else
		return FALSE;

	//エラー

	MainWindow_apperr(id, NULL);

	return TRUE;
}

/** 実際の実行時、処理するイメージ範囲をセット
 *
 * @return FALSE で処理なし */

static mBool _set_run_image_area(FilterDrawInfo *info,uint8_t flags)
{
	mRect rc;

	//選択範囲があれば、その範囲。なければ描画可能な全体範囲

	if(drawSel_isHave())
		rc = APP_DRAW->sel.rcsel;
	else
	{
		if(flags & _FDF_PROC_COLOR)
		{
			//色処理の場合は、タイルのある範囲 (空イメージで処理なし)
			
			if(!TileImage_getHaveImageRect_pixel(APP_DRAW->curlayer->img, &rc, NULL))
				return FALSE;
		}
		else
			//描画可能範囲
			TileImage_getCanDrawRect_pixel(APP_DRAW->curlayer->img, &rc);
	}

	//キャンバス範囲内のみ

	if(info->clipping)
	{
		if(!drawCalc_clipImageRect(APP_DRAW, &rc))
			return FALSE;
	}

	//セット

	info->rc = rc;
	mBoxSetByRect(&info->box, &rc);

	return TRUE;
}

/** フィルタ処理実行スレッド */

static int _thread_filter(mPopupProgress *prog,void *data)
{
	FilterDrawInfo *info = (FilterDrawInfo *)data;

	info->prog = prog;

	//進捗の最大は、デフォルトで処理範囲の高さ

	mPopupProgressThreadBeginSubStep_onestep(prog, 50, info->box.h);

	return (info->drawfunc)(info);
}

/** 実行 */

static void _run_filter(uint16_t menuid,const uint8_t *dat,FilterDrawFunc func,uint8_t flags)
{
	FilterDrawInfo info;
	TileImage *imgsrc = NULL;
	int ret;

	if(_show_message(menuid, flags)) return;

	//FilterDrawInfo

	mMemzero(&info, sizeof(FilterDrawInfo));

	info.drawfunc = func;
	info.clipping = ((flags & _FDF_CLIPPING) != 0);

	drawColor_getDrawColor_rgbafix(&info.rgba15Draw);

	info.rgba15Bkgnd.r = APP_DRAW->col.rgb15BkgndCol.r;
	info.rgba15Bkgnd.g = APP_DRAW->col.rgb15BkgndCol.g;
	info.rgba15Bkgnd.b = APP_DRAW->col.rgb15BkgndCol.b;
	info.rgba15Bkgnd.a = 0x8000;

	//------- ダイアログ実行

	if(dat && !(flags & _FDF_NODLG_INDEX))
	{
		info.in_dialog = TRUE;
		APP_DRAW->w.in_filter_dialog = TRUE;
		APP_DRAW->w.tileimg_filterprev = NULL;

		//ダイアログ内プレビュー用初期化

		g_tileimage_dinfo.funcDrawPixel = TileImage_setPixel_new_notp;

		//ダイアログ実行

		ret = FilterDlg_run(M_WINDOW(APP_WIDGETS->mainwin), menuid, dat, &info);

		info.in_dialog = FALSE;
		APP_DRAW->w.in_filter_dialog = FALSE;

		if(!ret) return;
	}

	//------ フィルタ処理実行

	//処理範囲セット

	if(!_set_run_image_area(&info, flags))
		return;

	//描画情報

	info.imgsrc = info.imgdst = APP_DRAW->curlayer->img;
	info.imgsel = APP_DRAW->tileimgSel;

	if(flags & _FDF_NODLG_INDEX)
		info.ntmp[0] = (intptr_t)dat;

	//描画先をコピーしてソースとする

	if(flags & _FDF_COPYSRC)
	{
		imgsrc = TileImage_newClone(info.imgdst);
		if(!imgsrc)
		{
			MainWindow_apperr(APPERR_ALLOC, NULL);
			return;
		}

		info.imgsrc = imgsrc;
	}

	//準備

	drawOpSub_setDrawInfo_filter();
	drawOpSub_beginDraw(APP_DRAW);

	//スレッド

	ret = PopupThread_run(&info, _thread_filter);

	drawOpSub_finishDraw_single(APP_DRAW);

	//複製イメージ削除

	TileImage_free(imgsrc);

	//エラー

	if(ret != 1)
		MainWindow_apperr(APPERR_FAILED, NULL);
}

/** コマンド実行 */

void MainWindow_filtercmd(int id)
{
	_cmddat *dat;

	dat = g_fcmddat + (id - TRMENU_FILTER_COLOR_BRIGHT_CONST);

	_run_filter(id, dat->dlgdat, dat->func, dat->flags);
}

