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

/*************************************
 * 設定ファイルの読み書き
 *************************************/

#include <stdlib.h>
#include <stdio.h>

#include "mDef.h"
#include "mGui.h"
#include "mAppDef.h"
#include "mStr.h"
#include "mIniRead.h"
#include "mIniWrite.h"
#include "mWidgetDef.h"
#include "mWindow.h"
#include "mDockWidget.h"

#include "defMacros.h"
#include "defConfig.h"
#include "defDraw.h"
#include "defWidgets.h"
#include "defScalingType.h"
#include "defCanvasKeyID.h"

#include "ConfigData.h"
#include "DockObject.h"
#include "macroToolOpt.h"
#include "FilterSaveData.h"


//-------------------

#define _INIT_VAL        -100000

//パネル名

static const char *g_dock_name[] = {
	"tool", "option", "layer", "brush", "color", "colpal",
	"cvctrl", "cvview", "imgview", "filterl", "colwheel"
};

//-------------------



/********************************
 * 読み込み
 ********************************/


/** 読み込み前の初期化 */

static void _load_init_data(ConfigData *cf,DrawData *pdw)
{
	int i;

	/* [!] 確保時にゼロクリアはされている */

	//中間色、左右の色

	for(i = 0; i < COLPAL_GRADNUM; i++)
		pdw->col.gradcol[i][1] = 0xffffff;

	//グリッド登録リスト

	for(i = 0; i < CONFIG_GRIDREG_NUM; i++)
		cf->grid.reglist[i] = (100 << 16) | 100;

	//水彩プリセット

	ConfigData_waterPreset_default();

	//ボタン操作
	/* 中ボタン:キャンバス移動
	 * 右ボタン:スポイト
	 * ホイール上:一段階拡大
	 * ホイール下:一段階縮小 */

	cf->default_btt_cmd[2] = CANVASKEYID_OP_TOOL + TOOL_CANVAS_MOVE;
	cf->default_btt_cmd[3] = CANVASKEYID_OP_TOOL + TOOL_SPOIT;
	cf->default_btt_cmd[4] = CANVASKEYID_CMD_OTHER_ZOOM_UP;
	cf->default_btt_cmd[5] = CANVASKEYID_CMD_OTHER_ZOOM_DOWN;
}

/** 新規作成サイズ (履歴/登録)
 *
 * カンマで区切った４つのデータ ("単位, DPI, 幅, 高さ") */

static void _load_imagesize(mIniRead *ini,ImageSizeData *dat)
{
	int i,buf[4];
	char m[16];

	for(i = 0; i < CONFIG_IMAGESIZE_NUM; i++)
	{
		snprintf(m, 16, "%d", i);

		if(mIniReadNums(ini, m, buf, 4, 4, FALSE) < 4)
		{
			dat[i].unit = 255;
			break;
		}

		dat[i].unit = buf[0];
		dat[i].dpi = buf[1];
		dat[i].w = buf[2];
		dat[i].h = buf[3];
	}
}

/** 定規記録データ読み込み */

static void _load_draw_rule_record(mIniRead *ini)
{
	const char *key,*param;
	char *end;
	int no,i;
	RuleRecord *rec;

	if(!mIniReadSetGroup(ini, "rule_record")) return;

	while(mIniReadGetNextItem(ini, &key, &param))
	{
		no = atoi(key);
		if(no < 0 || no >= RULE_RECORD_NUM) continue;

		rec = APP_DRAW->rule.record + no;
	
		//タイプ

		rec->type = strtoul(param, &end, 10);

		if(rec->type == 0 || !(*end)) continue;

		//値

		for(i = 0; i < 4; i++)
		{
			rec->d[i] = strtod(end + 1, &end);
			if(!(*end)) break;
		}
	}
}

/** パネル配置値を正規化 */

static void _normalize_panel_layout(ConfigData *cf)
{
	char *pc;
	char buf[4];
	int i,no,pos;

	//----- ペイン

	for(i = 0; i < 4; i++)
		buf[i] = -1;

	//各ペインの位置

	for(pc = cf->pane_layout, pos = 0; *pc; pc++)
	{
		no = *pc - '0';
		if(no >= 0 && no <= 3 && buf[no] == -1)
			buf[no] = pos++;
	}

	//記述されていないもの

	for(i = 0; i < 4; i++)
	{
		if(buf[i] == -1)
			buf[i] = pos++;
	}

	//再セット

	pc = cf->pane_layout;

	for(i = 0; i < 4; i++)
	{
		for(no = 0; no < 4 && buf[no] != i; no++);
	
		*(pc++) = no + '0';
	}

	*pc = 0;

	//------ パネル

	DockObject_normalize_layout_config();
}

/** ウィジェット状態読み込み */

static void _load_widgets_state(mIniRead *ini,ConfigData *cf,mDockWidgetState *dock_state)
{
	int i;
	int32_t val[6];
	mBox *pbox;
	uint8_t inith[] = { 92,180,200,200,161,147,59,168,158,150,200 };

	mIniReadSetGroup(ini, "widgets");

	//メインウィンドウ

	mIniReadBox(ini, "mainwin", &cf->box_mainwin, _INIT_VAL, _INIT_VAL, _INIT_VAL, _INIT_VAL);

	cf->mainwin_maximized = mIniReadInt(ini, "maximized", 1);
	cf->dockbrush_height[0] = mIniReadInt(ini, "brush_sizeh", 58);
	cf->dockbrush_height[1] = mIniReadInt(ini, "brush_listh", 100);

	//ペイン幅

	for(i = 0; i < 3; i++)
		cf->pane_width[i] = 230;

	mIniReadNums(ini, "pane_width", cf->pane_width, 3, 2, FALSE);

	//配置

	mIniReadTextBuf(ini, "pane_layout", cf->pane_layout, 5, "1023");
	mIniReadTextBuf(ini, "dock_layout", cf->dock_layout, 16, "tol:ivrb:cwpf");

	_normalize_panel_layout(cf);

	//ダイアログ

	mIniReadBox(ini, "textdlg", &cf->box_textdlg, 0, 0, 0, 0);
	mIniReadSize(ini, "boxeditdlg", &cf->size_boxeditdlg, 0, 0);
	mIniReadPoint(ini, "filterdlg", &cf->pt_filterdlg, _INIT_VAL, _INIT_VAL);

	//dock widget

	for(i = 0; i < DOCKWIDGET_NUM; i++)
	{
		pbox = &dock_state[i].boxWin;

		if(mIniReadNums(ini, g_dock_name[i], val, 6, 4, FALSE) == 6)
		{
			pbox->x = val[0];
			pbox->y = val[1];
			pbox->w = val[2];
			pbox->h = val[3];

			dock_state[i].dockH = val[4];
			dock_state[i].flags = val[5];
		}
		else
		{
			//デフォルト
			/* イメージビューア、フィルタ一覧は閉じる */

			pbox->x = pbox->y = 0;
			pbox->w = pbox->h = 250;

			dock_state[i].dockH = inith[i];
			dock_state[i].flags = MDOCKWIDGET_F_VISIBLE;

			if(i != DOCKWIDGET_IMAGE_VIEWER && i != DOCKWIDGET_FILTER_LIST)
				dock_state[i].flags |= MDOCKWIDGET_F_EXIST;
		}
	}
}

/** ConfigData 読み込み */

static void _load_configdata(mIniRead *ini,ConfigData *cf)
{
	int no;
	uint32_t u32;

	//----- 環境

	mIniReadSetGroup(ini, "env");

	cf->init_imgw = mIniReadInt(ini, "init_imgw", 400);
	cf->init_imgh = mIniReadInt(ini, "init_imgh", 400);
	cf->init_dpi = mIniReadInt(ini, "init_dpi", 300);

	cf->optflags = mIniReadHex(ini, "optflags",
		CONFIG_OPTF_MES_SAVE_OVERWRITE | CONFIG_OPTF_MES_SAVE_APD);

	cf->fView = mIniReadHex(ini, "fview",
		CONFIG_VIEW_F_TOOLBAR | CONFIG_VIEW_F_STATUSBAR | CONFIG_VIEW_F_DOCKS | CONFIG_VIEW_F_BKGND_PLAID | CONFIG_VIEW_F_FILTERDLG_PREVIEW);

	cf->savedup_type = mIniReadInt(ini, "savedup_type", 0);
	cf->canvas_resize_flags = mIniReadInt(ini, "canvas_resize_flags", CONFIG_CANVAS_RESIZE_F_CROP);
	cf->canvas_scale_type = mIniReadInt(ini, "canvas_scale_type", SCALING_TYPE_LANCZOS2);
	cf->boxedit_flags = mIniReadInt(ini, "boxedit_flags", 0);
	
	cf->undo_maxbufsize = mIniReadInt(ini, "undo_maxbufsize", 1024 * 1024);
	cf->undo_maxnum = mIniReadInt(ini, "undo_maxnum", 100);

	cf->canvasZoomStep_low = mIniReadInt(ini, "zoomstep_low", 15);
	cf->canvasZoomStep_hi = mIniReadInt(ini, "zoomstep_hi", 100);
	cf->canvasAngleStep = mIniReadInt(ini, "anglestep", 15);
	cf->dragBrushSize_step = mIniReadInt(ini, "dragbrushsize_step", 10);
	cf->iconsize_toolbar = mIniReadInt(ini, "iconsize_toolbar", 20);
	cf->iconsize_layer = mIniReadInt(ini, "iconsize_layer", 13);
	cf->iconsize_other = mIniReadInt(ini, "iconsize_other", 16);

	cf->colCanvasBkgnd = mIniReadHex(ini, "col_canvasbkgnd", 0xc0c0c0);
	cf->colBkgndPlaid[0] = mIniReadHex(ini, "col_bkgndplaid1", 0xd0d0d0);
	cf->colBkgndPlaid[1] = mIniReadHex(ini, "col_bkgndplaid2", 0xf0f0f0);

	cf->cursor_buf = mIniReadBase64(ini, "cursor_draw", &cf->cursor_bufsize);

	if(mIniReadIsHaveKey(ini, "toolbar_btts"))
	{
		cf->toolbar_btts = mIniReadNums_alloc(ini, "toolbar_btts", 1, FALSE, 1, &cf->toolbar_btts_size);

		if(cf->toolbar_btts)
			*(cf->toolbar_btts + cf->toolbar_btts_size - 1) = 255;
	}

	//----- dock 関連

	mIniReadSetGroup(ini, "dock");

	cf->dockcolor_tabno = mIniReadInt(ini, "color_tabno", 0);
	cf->dockcolpal_tabno = mIniReadInt(ini, "colpal_tabno", 0);
	cf->dockopt_tabno = mIniReadInt(ini, "opt_tabno", 0);
	cf->dockbrush_expand_flags = mIniReadInt(ini, "brush_expand", 0xffff);
	cf->dockcolwheel_type = mIniReadInt(ini, "colwheel_type", 0);

	mIniReadNums(ini, "water_preset", cf->water_preset, CONFIG_WATER_PRESET_NUM, 4, TRUE);

	//----- キャンバスビュー

	mIniReadSetGroup(ini, "canvasview");

	cf->canvasview_flags = mIniReadInt(ini, "flags", CONFIG_CANVASVIEW_F_FIT);
	cf->canvasview_zoom_normal = mIniReadInt(ini, "zoom_norm", 100);
	cf->canvasview_zoom_loupe = mIniReadInt(ini, "zoom_loupe", 600);

	//ボタン

	cf->canvasview_btt[0] = 1;
	cf->canvasview_btt[2] = 2;

	mIniReadNums(ini, "btt", cf->canvasview_btt, 4, 1, FALSE);

	//----- イメージビューア

	mIniReadSetGroup(ini, "imageviewer");

	mIniReadStr(ini, "dir", &cf->strImageViewerDir, NULL);

	cf->imageviewer_flags = mIniReadInt(ini, "flags", CONFIG_IMAGEVIEWER_F_FIT);

	//ボタン

	cf->imageviewer_btt[1] = 1;
	cf->imageviewer_btt[2] = 2;
	cf->imageviewer_btt[3] = 2;

	mIniReadNums(ini, "btt", cf->imageviewer_btt, 4, 1, FALSE);

	//フラグ、左右反転は起動時常にOFF

	cf->imageviewer_flags &= ~CONFIG_IMAGEVIEWER_F_MIRROR;

	//----- グリッド

	mIniReadSetGroup(ini, "grid");

	cf->grid.gridw = mIniReadInt(ini, "gridw", 64);
	cf->grid.gridh = mIniReadInt(ini, "gridh", 64);
	cf->grid.col_grid = mIniReadHex(ini, "gridcol", 0x1a0040ff);

	cf->grid.splith = mIniReadInt(ini, "sph", 2);
	cf->grid.splitv = mIniReadInt(ini, "spv", 2);
	cf->grid.col_split = mIniReadHex(ini, "spcol", 0x1aff00ff);

	mIniReadNums(ini, "reg", cf->grid.reglist, CONFIG_GRIDREG_NUM, 4, TRUE);

	//----- 保存設定

	mIniReadSetGroup(ini, "save");

	cf->save.flags = mIniReadHex(ini, "flags", 0);
	cf->save.jpeg_quality = mIniReadInt(ini, "jpeg_qua", 85);
	cf->save.jpeg_sampling_factor = mIniReadInt(ini, "jpeg_samp", 444);
	cf->save.png_complevel = mIniReadInt(ini, "png_comp", 6);
	cf->save.psd_type = mIniReadInt(ini, "psd_type", 0);

	//----- 操作設定

	mIniReadSetGroup(ini, "button");

	mIniReadNums(ini, "default", cf->default_btt_cmd, CONFIG_POINTERBTT_NUM, 1, FALSE);
	mIniReadNums(ini, "pentab", cf->pentab_btt_cmd, CONFIG_POINTERBTT_NUM, 1, FALSE);

	//----- 文字列

	mIniReadSetGroup(ini, "string");

	mIniReadStr(ini, "fontstyle_gui", &cf->strFontStyle_gui, "family=sans-serif;size=10");
	mIniReadStr(ini, "fontstyle_dock", &cf->strFontStyle_dock, "family=sans-serif;size=9");

	mIniReadStr(ini, "tempdir", &cf->strTempDir, NULL);
	mIniReadStr(ini, "layerfiledir", &cf->strLayerFileDir, NULL);
	mIniReadStr(ini, "selectfiledir", &cf->strSelectFileDir, NULL);
	mIniReadStr(ini, "stampfiledir", &cf->strStampFileDir, NULL);

	mIniReadStr(ini, "user_texdir", &cf->strUserTextureDir, NULL);
	mIniReadStr(ini, "user_brushdir", &cf->strUserBrushDir, NULL);

	mIniReadStr(ini, "layernamelist", &cf->strLayerNameList, NULL);
	mIniReadStr(ini, "theme", &cf->strThemeFile, NULL);

	ConfigData_setTempDir_default();

	//素材ディレクトリが空の場合は設定ディレクトリ下

	if(mStrIsEmpty(&cf->strUserTextureDir))
		mAppGetConfigPath(&cf->strUserTextureDir, "texture");

	if(mStrIsEmpty(&cf->strUserBrushDir))
		mAppGetConfigPath(&cf->strUserBrushDir, "brush");

	//------ ファイル履歴

	mIniReadSetGroup(ini, "recentfile");
	mIniReadNoStrs(ini, 0, cf->strRecentFile, CONFIG_RECENTFILE_NUM);

	//------ 開いたディレクトリ履歴

	mIniReadSetGroup(ini, "recent_opendir");
	mIniReadNoStrs(ini, 0, cf->strRecentOpenDir, CONFIG_RECENTDIR_NUM);

	//------ 保存ディレクトリ履歴

	mIniReadSetGroup(ini, "recent_savedir");
	mIniReadNoStrs(ini, 0, cf->strRecentSaveDir, CONFIG_RECENTDIR_NUM);

	//----- 新規作成サイズ

	mIniReadSetGroup(ini, "newimg_recent");
	_load_imagesize(ini, cf->imgsize_recent);

	mIniReadSetGroup(ini, "newimg_record");
	_load_imagesize(ini, cf->imgsize_record);

	//----- キャンバスキー

	if(mIniReadSetGroup(ini, "canvaskey"))
	{
		while(mIniReadGetNextItem_nonum32(ini, &no, &u32, FALSE))
			cf->canvaskey[no] = u32;
	}

	//----- フィルタデータ

	FilterSaveData_getConfig(ini);
}

/** DrawData 読み込み */

static void _load_drawdata(mIniRead *ini,DrawData *p)
{
	int n;

	//------- main

	mIniReadSetGroup(ini, "draw_main");

	mIniReadStr(ini, "opttex", &p->strOptTexPath, NULL);

	//------- カラー

	mIniReadSetGroup(ini, "draw_color");

	p->col.drawcol = mIniReadHex(ini, "drawcol", 0);
	p->col.bkgndcol = mIniReadHex(ini, "bkgndcol", 0xffffff);

	n = mIniReadNums(ini, "colmask", p->col.colmask_col, COLORMASK_MAXNUM, 4, TRUE);
	if(n == 0) n = 1;
	
	p->col.colmask_num = n;
	p->col.colmask_col[n] = -1;

	mIniReadNums(ini, "gradcol", p->col.gradcol, COLPAL_GRADNUM * 2, 4, TRUE);

	p->col.colpal_sel = mIniReadInt(ini, "colpal_sel", 0);
	p->col.colpal_cellw = mIniReadInt(ini, "colpal_cellw", 14);
	p->col.colpal_cellh = mIniReadInt(ini, "colpal_cellh", 14);
	p->col.colpal_max_column = mIniReadInt(ini, "colpal_maxcol", 0);

	mIniReadNums(ini, "layercolpal", p->col.layercolpal, LAYERCOLPAL_NUM, 4, TRUE);

	//------- ツール

	mIniReadSetGroup(ini, "draw_tool");

	p->tool.no = mIniReadInt(ini, "no", 0);
	mIniReadNums(ini, "subno", p->tool.subno, TOOL_NUM, 1, FALSE);
	
	p->tool.opt_fillpoly = mIniReadHex(ini, "opt_fillpoly", FILLPOLY_OPT_DEFAULT);
	p->tool.opt_fillpoly_erase = mIniReadHex(ini, "opt_fillpoly_erase", FILLPOLY_OPT_DEFAULT);
	p->tool.opt_move = mIniReadInt(ini, "opt_move", 0);
	p->tool.opt_grad = mIniReadHex(ini, "opt_grad", GRAD_OPT_DEFAULT);
	p->tool.opt_fill = mIniReadHex(ini, "opt_fill", FILL_OPT_DEFAULT);
	p->tool.opt_stamp = mIniReadInt(ini, "opt_stamp", STAMP_OPT_DEFAULT);
	p->tool.opt_magicwand = mIniReadInt(ini, "opt_magicwand", MAGICWAND_OPT_DEFAULT);

	//-------- 入り抜き

	mIniReadSetGroup(ini, "headtail");

	p->headtail.selno = mIniReadInt(ini, "sel", 0);
	p->headtail.curval[0] = mIniReadHex(ini, "line", 0);
	p->headtail.curval[1] = mIniReadHex(ini, "bezier", 0);

	mIniReadNums(ini, "record", p->headtail.record, HEADTAIL_RECORD_NUM, 4, TRUE);

	//------- テキスト描画

	mIniReadSetGroup(ini, "drawtext");

	mIniReadStr(ini, "name", &p->drawtext.strName, NULL);
	mIniReadStr(ini, "style", &p->drawtext.strStyle, NULL);

	p->drawtext.size = mIniReadInt(ini, "size", 12);
	p->drawtext.weight = mIniReadInt(ini, "weight", 0);
	p->drawtext.slant = mIniReadInt(ini, "slant", 0);
	p->drawtext.hinting = mIniReadInt(ini, "hinting", 0);
	p->drawtext.char_space = mIniReadInt(ini, "charsp", 0);
	p->drawtext.line_space = mIniReadInt(ini, "linesp", 0);
	p->drawtext.flags = mIniReadInt(ini, "flags", DRAW_DRAWTEXT_F_PREVIEW | DRAW_DRAWTEXT_F_ANTIALIAS);

	//定規

	_load_draw_rule_record(ini);
}

/** 設定ファイル読み込み
 *
 * @return 0:失敗 1:ファイルが存在 2:初期状態 */

int appLoadConfig(mDockWidgetState *dock_state)
{
	mIniRead *ini;
	int ret;

	ini = mIniReadLoadFile2(MAPP->pathConfig, CONFIG_FILENAME_MAIN);
	if(!ini) return FALSE;

	//バージョン

	mIniReadSetGroup(ini, "azpainter");

	if(mIniReadInt(ini, "ver", 0) != 2)
	{
		mIniReadEmpty(ini);
		ret = 2;
	}
	else
		ret = 1;

	//mlib

	mIniReadSetGroup(ini, "mlib");

	mIniReadFileDialogConfig(ini, "filedialog", MAPP->filedialog_config);

	//読み込み前の初期化

	_load_init_data(APP_CONF, APP_DRAW);

	//ウィジェット状態

	_load_widgets_state(ini, APP_CONF, dock_state);

	//ConfigData

	_load_configdata(ini, APP_CONF);

	//DrawData

	_load_drawdata(ini, APP_DRAW);

	mIniReadEnd(ini);

	return ret;
}


/********************************
 * 書き込み
 ********************************/


/** 新規作成サイズ */

static void _save_imagesize(FILE *fp,ImageSizeData *dat)
{
	int i,buf[4];
	char m[16];

	for(i = 0; i < CONFIG_IMAGESIZE_NUM; i++)
	{
		if(dat[i].unit == 255) break;
	
		buf[0] = dat[i].unit;
		buf[1] = dat[i].dpi;
		buf[2] = dat[i].w;
		buf[3] = dat[i].h;
	
		snprintf(m, 16, "%d", i);
		mIniWriteNums(fp, m, buf, 4, 4, FALSE);
	}
}

/** 定規記録データ 書き込み */

static void _save_draw_rule_record(FILE *fp)
{
	RuleRecord *rec = APP_DRAW->rule.record;
	int i;

	mIniWriteGroup(fp, "rule_record");

	for(i = 0; i < RULE_RECORD_NUM; i++, rec++)
	{
		if(rec->type)
		{
			fprintf(fp, "%d=%d,%e,%e,%e,%e\n",
				i, rec->type, rec->d[0], rec->d[1], rec->d[2], rec->d[3]);
		}
	}
}

/** ウィジェット状態 */

static void _save_widgets_state(FILE *fp,ConfigData *cf)
{
	WidgetsData *wg = APP_WIDGETS;
	mDockWidgetState st;
	int i;
	int32_t val[6];

	mIniWriteGroup(fp, "widgets");

	//メインウィンドウ

	mWindowGetSaveBox(M_WINDOW(wg->mainwin), &cf->box_mainwin);

	mIniWriteBox(fp, "mainwin", &cf->box_mainwin);
	mIniWriteInt(fp, "maximized", mWindowIsMaximized(M_WINDOW(wg->mainwin)));
	mIniWriteInt(fp, "brush_sizeh", cf->dockbrush_height[0]);
	mIniWriteInt(fp, "brush_listh", cf->dockbrush_height[1]);

	//ペイン幅

	for(i = 0; i < 3; i++)
		cf->pane_width[i] = wg->pane[i]->w;

	mIniWriteNums(fp, "pane_width", cf->pane_width, 3, 2, FALSE);

	//配置

	mIniWriteText(fp, "pane_layout", cf->pane_layout);
	mIniWriteText(fp, "dock_layout", cf->dock_layout);

	//ダイアログ

	mIniWriteBox(fp, "textdlg", &cf->box_textdlg);
	mIniWriteSize(fp, "boxeditdlg", &cf->size_boxeditdlg);
	mIniWritePoint(fp, "filterdlg", &cf->pt_filterdlg);

	//dock widget

	for(i = 0; i < DOCKWIDGET_NUM; i++)
	{
		if(wg->dockobj[i])
		{
			mDockWidgetGetState(wg->dockobj[i]->dockwg, &st);

			val[0] = st.boxWin.x;
			val[1] = st.boxWin.y;
			val[2] = st.boxWin.w;
			val[3] = st.boxWin.h;
			val[4] = st.dockH;
			val[5] = st.flags;

			mIniWriteNums(fp, g_dock_name[i], val, 6, 4, FALSE);
		}
	}
}

/** ConfigData */

static void _save_configdata(FILE *fp,ConfigData *cf)
{
	int i;

	//----- env

	mIniWriteGroup(fp, "env");

	mIniWriteInt(fp, "init_imgw", cf->init_imgw);
	mIniWriteInt(fp, "init_imgh", cf->init_imgh);
	mIniWriteInt(fp, "init_dpi", cf->init_dpi);

	mIniWriteHex(fp, "optflags", cf->optflags);
	mIniWriteHex(fp, "fview", cf->fView);
	mIniWriteInt(fp, "savedup_type", cf->savedup_type);
	mIniWriteInt(fp, "canvasview_flags", cf->canvasview_flags);
	mIniWriteInt(fp, "canvas_resize_flags", cf->canvas_resize_flags);
	mIniWriteInt(fp, "canvas_scale_type", cf->canvas_scale_type);
	mIniWriteInt(fp, "boxedit_flags", cf->boxedit_flags);
	mIniWriteInt(fp, "undo_maxbufsize", cf->undo_maxbufsize);
	mIniWriteInt(fp, "undo_maxnum", cf->undo_maxnum);

	mIniWriteInt(fp, "zoomstep_low", cf->canvasZoomStep_low);
	mIniWriteInt(fp, "zoomstep_hi", cf->canvasZoomStep_hi);
	mIniWriteInt(fp, "anglestep", cf->canvasAngleStep);
	mIniWriteInt(fp, "dragbrushsize_step", cf->dragBrushSize_step);
	mIniWriteInt(fp, "iconsize_toolbar", cf->iconsize_toolbar);
	mIniWriteInt(fp, "iconsize_layer", cf->iconsize_layer);
	mIniWriteInt(fp, "iconsize_other", cf->iconsize_other);
	
	mIniWriteInt(fp, "canvasview_zoom_norm", cf->canvasview_zoom_normal);
	mIniWriteInt(fp, "canvasview_zoom_loupe", cf->canvasview_zoom_loupe);
	
	mIniWriteHex(fp, "col_canvasbkgnd", cf->colCanvasBkgnd);
	mIniWriteHex(fp, "col_bkgndplaid1", cf->colBkgndPlaid[0]);
	mIniWriteHex(fp, "col_bkgndplaid2", cf->colBkgndPlaid[1]);

	mIniWriteBase64(fp, "cursor_draw", cf->cursor_buf, cf->cursor_bufsize);

	if(cf->toolbar_btts)
		mIniWriteNums(fp, "toolbar_btts", cf->toolbar_btts, cf->toolbar_btts_size - 1, 1, FALSE);

	//----- dock 関連

	mIniWriteGroup(fp, "dock");

	mIniWriteInt(fp, "color_tabno", cf->dockcolor_tabno);
	mIniWriteInt(fp, "colpal_tabno", cf->dockcolpal_tabno);
	mIniWriteInt(fp, "opt_tabno", cf->dockopt_tabno);
	mIniWriteInt(fp, "brush_expand", cf->dockbrush_expand_flags);
	mIniWriteInt(fp, "colwheel_type", cf->dockcolwheel_type);
	
	mIniWriteNums(fp, "water_preset", cf->water_preset, CONFIG_WATER_PRESET_NUM, 4, TRUE);

	//----- キャンバスビュー

	mIniWriteGroup(fp, "canvasview");

	mIniWriteInt(fp, "flags", cf->canvasview_flags);
	mIniWriteInt(fp, "zoom_norm", cf->canvasview_zoom_normal);
	mIniWriteInt(fp, "zoom_loupe", cf->canvasview_zoom_loupe);
	mIniWriteNums(fp, "btt", cf->canvasview_btt, 4, 1, FALSE);

	//----- イメージビューア

	mIniWriteGroup(fp, "imageviewer");

	mIniWriteStr(fp, "dir", &cf->strImageViewerDir);
	mIniWriteInt(fp, "flags", cf->imageviewer_flags);
	mIniWriteNums(fp, "btt", cf->imageviewer_btt, 4, 1, FALSE);

	//----- グリッド

	mIniWriteGroup(fp, "grid");

	mIniWriteInt(fp, "gridw", cf->grid.gridw);
	mIniWriteInt(fp, "gridh", cf->grid.gridh);
	mIniWriteHex(fp, "gridcol", cf->grid.col_grid);
	
	mIniWriteInt(fp, "sph", cf->grid.splith);
	mIniWriteInt(fp, "spv", cf->grid.splitv);
	mIniWriteHex(fp, "spcol", cf->grid.col_split);

	mIniWriteNums(fp, "reg", cf->grid.reglist, CONFIG_GRIDREG_NUM, 4, TRUE);

	//----- 保存設定

	mIniWriteGroup(fp, "save");

	mIniWriteHex(fp, "flags", cf->save.flags);
	mIniWriteInt(fp, "jpeg_qua", cf->save.jpeg_quality);
	mIniWriteInt(fp, "jpeg_samp", cf->save.jpeg_sampling_factor);
	mIniWriteInt(fp, "png_comp", cf->save.png_complevel);
	mIniWriteInt(fp, "psd_type", cf->save.psd_type);

	//----- 操作

	mIniWriteGroup(fp, "button");

	mIniWriteNums(fp, "default", cf->default_btt_cmd, CONFIG_POINTERBTT_NUM, 1, FALSE);
	mIniWriteNums(fp, "pentab", cf->pentab_btt_cmd, CONFIG_POINTERBTT_NUM, 1, FALSE);

	//----- 文字列

	mIniWriteGroup(fp, "string");

	mIniWriteStr(fp, "fontstyle_gui", &cf->strFontStyle_gui);
	mIniWriteStr(fp, "fontstyle_dock", &cf->strFontStyle_dock);

	mIniWriteStr(fp, "tempdir", &cf->strTempDir);
	mIniWriteStr(fp, "layerfiledir", &cf->strLayerFileDir);
	mIniWriteStr(fp, "selectfiledir", &cf->strSelectFileDir);
	mIniWriteStr(fp, "stampfiledir", &cf->strStampFileDir);

	mIniWriteStr(fp, "user_texdir", &cf->strUserTextureDir);
	mIniWriteStr(fp, "user_brushdir", &cf->strUserBrushDir);

	mIniWriteStr(fp, "layernamelist", &cf->strLayerNameList);
	mIniWriteStr(fp, "theme", &cf->strThemeFile);

	//----- ファイル履歴

	mIniWriteGroup(fp, "recentfile");
	mIniWriteNoStrs(fp, 0, cf->strRecentFile, CONFIG_RECENTFILE_NUM);

	//----- 開いたディレクトリ履歴

	mIniWriteGroup(fp, "recent_opendir");
	mIniWriteNoStrs(fp, 0, cf->strRecentOpenDir, CONFIG_RECENTDIR_NUM);

	//----- 保存ディレクトリ履歴

	mIniWriteGroup(fp, "recent_savedir");
	mIniWriteNoStrs(fp, 0, cf->strRecentSaveDir, CONFIG_RECENTDIR_NUM);

	//----- 新規作成サイズ

	mIniWriteGroup(fp, "newimg_recent");
	_save_imagesize(fp, cf->imgsize_recent);

	mIniWriteGroup(fp, "newimg_record");
	_save_imagesize(fp, cf->imgsize_record);

	//----- キャンバスキー

	mIniWriteGroup(fp, "canvaskey");

	for(i = 0; i < 256; i++)
	{
		if(cf->canvaskey[i])
			mIniWriteNoInt(fp, i, cf->canvaskey[i]);
	}

	//----- フィルタデータ

	FilterSaveData_setConfig(fp);
}

/** DrawData 書き込み */

static void _save_drawdata(FILE *fp,DrawData *p)
{
	//------ main

	mIniWriteGroup(fp, "draw_main");

	mIniWriteStr(fp, "opttex", &p->strOptTexPath);

	//------ カラー

	mIniWriteGroup(fp, "draw_color");

	mIniWriteHex(fp, "drawcol", p->col.drawcol);
	mIniWriteHex(fp, "bkgndcol", p->col.bkgndcol);
	mIniWriteNums(fp, "colmask", p->col.colmask_col, p->col.colmask_num, 4, TRUE);
	mIniWriteNums(fp, "gradcol", p->col.gradcol, COLPAL_GRADNUM * 2, 4, TRUE);
	mIniWriteInt(fp, "colpal_sel", p->col.colpal_sel);
	mIniWriteInt(fp, "colpal_cellw", p->col.colpal_cellw);
	mIniWriteInt(fp, "colpal_cellh", p->col.colpal_cellh);
	mIniWriteInt(fp, "colpal_maxcol", p->col.colpal_max_column);
	mIniWriteNums(fp, "layercolpal", p->col.layercolpal, LAYERCOLPAL_NUM, 4, TRUE);

	//------- ツール

	mIniWriteGroup(fp, "draw_tool");

	mIniWriteInt(fp, "no", p->tool.no);
	mIniWriteNums(fp, "subno", p->tool.subno, TOOL_NUM, 1, FALSE);

	mIniWriteHex(fp, "opt_fillpoly", p->tool.opt_fillpoly);
	mIniWriteHex(fp, "opt_fillpoly_erase", p->tool.opt_fillpoly_erase);
	mIniWriteInt(fp, "opt_move", p->tool.opt_move);
	mIniWriteHex(fp, "opt_grad", p->tool.opt_grad);
	mIniWriteHex(fp, "opt_fill", p->tool.opt_fill);
	mIniWriteInt(fp, "opt_stamp", p->tool.opt_stamp);
	mIniWriteInt(fp, "opt_magicwand", p->tool.opt_magicwand);

	//-------- headtail

	mIniWriteGroup(fp, "headtail");

	mIniWriteInt(fp, "sel", p->headtail.selno);
	mIniWriteHex(fp, "line", p->headtail.curval[0]);
	mIniWriteHex(fp, "bezier", p->headtail.curval[1]);

	mIniWriteNums(fp, "record", p->headtail.record, HEADTAIL_RECORD_NUM, 4, TRUE);

	//------- テキスト描画

	mIniWriteGroup(fp, "drawtext");

	mIniWriteStr(fp, "name", &p->drawtext.strName);
	mIniWriteStr(fp, "style", &p->drawtext.strStyle);
	mIniWriteInt(fp, "weight", p->drawtext.weight);
	mIniWriteInt(fp, "slant", p->drawtext.slant);
	mIniWriteInt(fp, "size", p->drawtext.size);
	mIniWriteInt(fp, "hinting", p->drawtext.hinting);
	mIniWriteInt(fp, "charsp", p->drawtext.char_space);
	mIniWriteInt(fp, "linesp", p->drawtext.line_space);
	mIniWriteInt(fp, "flags", p->drawtext.flags);

	//定規

	_save_draw_rule_record(fp);
}

/** 設定ファイル書き込み (メイン) */

void appSaveConfig()
{
	FILE *fp;

	fp = mIniWriteOpenFile2(MAPP->pathConfig, CONFIG_FILENAME_MAIN);
	if(!fp) return;

	//バージョン

	mIniWriteGroup(fp, "azpainter");
	mIniWriteInt(fp, "ver", 2);

	//mlib

	mIniWriteGroup(fp, "mlib");
	
	mIniWriteFileDialogConfig(fp, "filedialog", MAPP->filedialog_config);

	//ウィジェット状態

	_save_widgets_state(fp, APP_CONF);

	//ConfigData

	_save_configdata(fp, APP_CONF);

	//DrawData

	_save_drawdata(fp, APP_DRAW);

	fclose(fp);
}
