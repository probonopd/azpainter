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

/**********************************
 * main
 **********************************/

#include "mDef.h"
#include "mStr.h"
#include "mGui.h"
#include "mFont.h"
#include "mDockWidget.h"
#include "mMessageBox.h"
#include "mTrans.h"
#include "mUtilFile.h"

#include "defMacros.h"
#include "defConfig.h"
#include "defDraw.h"
#include "defWidgets.h"

#include "ConfigData.h"
#include "AppCursor.h"
#include "ColorPalette.h"
#include "MaterialImgList.h"
#include "GradationList.h"
#include "BrushSizeList.h"
#include "BrushList.h"
#include "Undo.h"
#include "DrawFont.h"
#include "SplineBuf.h"
#include "FilterSaveData.h"

#include "MainWindow.h"
#include "DockObject.h"
#include "Docks_external.h"

#include "draw_main.h"

#include "trgroup.h"
#include "trid_message.h"

#include "deftrans.h"

//イメージデータ、グローバル定義
#define IMAGEPATTERN_DEFINE
#include "dataImagePattern.h"


//-----------------------

#define _CONFIGDIR_NAME  ".azpainter"

//-----------------------
/* グローバル変数定義 */

WidgetsData g_app_widgets_body,	//WidgetsData 本体
	*g_app_widgets = NULL;
ConfigData *g_app_config = NULL;
DrawData *g_app_draw = NULL;

//-----------------------

/* configfile.c */
int appLoadConfig(mDockWidgetState *dock_state);
void appSaveConfig();

/* 各パレットオブジェクト作成 */
void DockTool_new(mDockWidgetState *state);
void DockOption_new(mDockWidgetState *state);
void DockLayer_new(mDockWidgetState *state);
void DockBrush_new(mDockWidgetState *state);
void DockColor_new(mDockWidgetState *state);
void DockColorPalette_new(mDockWidgetState *state);
void DockCanvasCtrl_new(mDockWidgetState *state);
void DockCanvasView_new(mDockWidgetState *state);
void DockImageViewer_new(mDockWidgetState *state);
void DockFilterList_new(mDockWidgetState *state);
void DockColorWheel_new(mDockWidgetState *state);

//-----------------------



//===========================
// 初期化
//===========================


/** 設定ディレクトリ内のファイルが存在するか */

static mBool _is_exist_conffile(const char *fname)
{
	mStr str = MSTR_INIT;
	mBool ret;

	mAppGetConfigPath(&str, fname);

	ret = mIsFileExist(str.buf, FALSE);

	mStrFree(&str);

	return ret;
}

/** データ移行、またはデフォルトファイルコピー */

static void _set_init_data(mBool dir_exist,mBool loadconf_init)
{
	//ver.1 のデータ移行

	if(dir_exist && loadconf_init
		&& (!_is_exist_conffile(CONFIG_FILENAME_BRUSH) && !_is_exist_conffile(CONFIG_FILENAME_COLPALETTE))
		&& (_is_exist_conffile("brush.dat") || _is_exist_conffile("palette.dat")))
	{
		/* 設定ファイルディレクトリがすでに存在 + 設定ファイルが存在しない
		 *  + ver.2 のブラシ/パレットファイルが存在しない
		 *  + ver.1 のブラシ/パレットファイルがある場合 */

		if(mMessageBox(NULL, NULL,
			M_TR_T2(TRGROUP_MESSAGE, TRID_MES_CONVERT_VER1),
			MMESBOX_YESNO, MMESBOX_YES) == MMESBOX_YES)
		{
			//ブラシとパレット変換
			
			BrushList_convert_from_ver1();
			ColorPalette_convert_from_ver1();

			//グラデーションとショートカットキー設定は、
			//ファイルが存在しない場合コピー

			mAppCopyFile_dataToConfig(CONFIG_FILENAME_GRADATION);
			mAppCopyFile_dataToConfig(CONFIG_FILENAME_SHORTCUTKEY);

			return;
		}
	}

	//デフォルトファイルコピー
	/* 設定ファイルディレクトリを新規作成した場合、
	 * または ver.1 のデータがあり ver.2 の設定ファイルがない時 */

	if(!dir_exist || loadconf_init)
	{
		mAppCopyFile_dataToConfig(CONFIG_FILENAME_BRUSH);
		mAppCopyFile_dataToConfig(CONFIG_FILENAME_GRADATION);
		mAppCopyFile_dataToConfig(CONFIG_FILENAME_COLPALETTE);
		mAppCopyFile_dataToConfig(CONFIG_FILENAME_SHORTCUTKEY);
	}
}

/** 初期化 */

static mBool _init()
{
	mDockWidgetState dock_state[DOCKWIDGET_NUM];
	int i,configdir,loadconf;

	//設定ファイル用ディレクトリ作成
	/* 戻り値が 0 で作成、-1 ですでに存在する */

	configdir = mAppCreateConfigDir(NULL);

	//ディレクトリを新規作成した場合、ブラシ/テクスチャディレクトリを作成

	if(configdir == 0)
	{
		mAppCreateConfigDir(APP_BRUSH_PATH);
		mAppCreateConfigDir(APP_TEXTURE_PATH);
	}

	//データ初期化

	mMemzero(&g_app_widgets_body, sizeof(WidgetsData));
	g_app_widgets = &g_app_widgets_body;

	ColorPalette_init();
	GradationList_init();
	MaterialImgList_init();
	BrushSizeList_init();
	SplineBuf_init();

	//フォント初期化

	if(!DrawFont_init()) return FALSE;

	//データ確保

	if(!ConfigData_new()
		|| !DrawData_new()
		|| !BrushList_new()
		|| !Undo_new())
		return FALSE;

	//設定ファイル読み込み

	loadconf = appLoadConfig(dock_state);
	if(!loadconf) return FALSE;

	//初期データファイル

	_set_init_data((configdir == -1), (loadconf == 2));

	//他データファイル読み込み

	ColorPalette_load();
	GradationList_load();
	BrushList_loadconfigfile();

	//設定

	Undo_setMaxNum(APP_CONF->undo_maxnum);

	//作業用ディレクトリ作成

	ConfigData_createTempDir();

	//カーソル作成 (! 設定読込後に行う)

	AppCursor_init(APP_CONF->cursor_buf);

	//フォント作成

	mAppSetDefaultFont(APP_CONF->strFontStyle_gui.buf);

	APP_WIDGETS->font_dock = mFontCreateFromFormat(APP_CONF->strFontStyle_dock.buf);
	APP_WIDGETS->font_dock12px = mFontCreateFromFormat_size(APP_CONF->strFontStyle_dock.buf, -12);

	//テーマ

	mAppLoadThemeFile(APP_CONF->strThemeFile.buf);

	//ウィジェット作成前の初期化

	drawInit_beforeCreateWidget();

	//メインウィンドウ作成

	MainWindow_new();

	//DockObject 作成

	DockTool_new(dock_state + DOCKWIDGET_TOOL);
	DockOption_new(dock_state + DOCKWIDGET_OPTION);
	DockLayer_new(dock_state + DOCKWIDGET_LAYER);
	DockBrush_new(dock_state + DOCKWIDGET_BRUSH);
	DockColor_new(dock_state + DOCKWIDGET_COLOR);
	DockColorPalette_new(dock_state + DOCKWIDGET_COLOR_PALETTE);
	DockCanvasCtrl_new(dock_state + DOCKWIDGET_CANVAS_CTRL);
	DockCanvasView_new(dock_state + DOCKWIDGET_CANVAS_VIEW);
	DockImageViewer_new(dock_state + DOCKWIDGET_IMAGE_VIEWER);
	DockFilterList_new(dock_state + DOCKWIDGET_FILTER_LIST);
	DockColorWheel_new(dock_state + DOCKWIDGET_COLOR_WHEEL);

	//イメージなど初期化

	drawInit_beforeShow();

	//ウィンドウ表示

	MainWindow_showStart(APP_WIDGETS->mainwin);

	for(i = 0; i < DOCKWIDGET_NUM; i++)
		DockObject_showStart(APP_WIDGETS->dockobj[i]);

	//表示後更新

	DockCanvasView_changeImageSize();
	DockLayer_update_all();

	//--------------

	//作業用ディレクトリの作成に失敗時

	if(mStrIsEmpty(&APP_CONF->strTempDirProc))
	{
		mMessageBox(NULL, NULL,
			M_TR_T2(TRGROUP_MESSAGE, TRID_MES_EMPTY_TEMPDIR), MMESBOX_OK, MMESBOX_OK);
	}
	 
	return TRUE;
}

/** 引数のファイルを開く */

static void _open_arg_file(const char *fname)
{
	mStr str = MSTR_INIT;

	mStrSetTextLocal(&str, fname, -1);
	
	MainWindow_loadImage(APP_WIDGETS->mainwin, str.buf, 0);

	mStrFree(&str);
}


//===========================
// メイン
//===========================


/** 終了処理 */

static void _finish()
{
	int i;

	//設定ファイル保存

	appSaveConfig();

	BrushList_saveconfigfile();
	ColorPalette_savefile();
	GradationList_save();

	//解放

	for(i = 0; i < DOCKWIDGET_NUM; i++)
	{
		DockObject_destroy(APP_WIDGETS->dockobj[i]);
		APP_WIDGETS->dockobj[i] = NULL;
	}

	mFontFree(APP_WIDGETS->font_dock);
	mFontFree(APP_WIDGETS->font_dock12px);

	AppCursor_free();

	DrawData_free();

	MaterialImgList_free();
	ColorPalette_free();
	GradationList_free();
	BrushSizeList_free();
	BrushList_free();
	Undo_free();
	SplineBuf_free();
	FilterSaveData_free();

	DrawFont_finish();

	//作業用ディレクトリ削除

	ConfigData_deleteTempDir();

	ConfigData_free();
}

/** メイン */

int main(int argc,char **argv)
{
	if(mAppInit(&argc, argv)) return 1;

	mAppInitPenTablet();

	//パス

	mAppSetConfigPath(_CONFIGDIR_NAME, TRUE);
	mAppSetDataPath(PACKAGE_DATA_DIR);

	//翻訳データ

	mAppLoadTranslation(g_deftransdat, NULL, PACKAGE_DATA_DIR "/tr");

	//初期化

	if(!_init())
	{
		mDebug("! failed initialize\n");
		return 1;
	}

	//

	if(argc > 1)
		_open_arg_file(argv[1]);

	//実行

	mAppRun();

	//終了

	_finish();

	mAppEnd();

	return 0;
}
