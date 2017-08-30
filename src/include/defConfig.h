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

/*****************************
 * 設定データ
 *****************************/

#ifndef DEF_CONFIG_H
#define DEF_CONFIG_H

#include "mStrDef.h"
#include "mListDef.h"

/** 個数 */

#define CONFIG_RECENTFILE_NUM   10	//最近使ったファイル
#define CONFIG_RECENTDIR_NUM    8   //ディレクトリの履歴
#define CONFIG_IMAGESIZE_NUM    10	//新規作成の履歴と登録
#define CONFIG_POINTERBTT_NUM   16  //ポインタデバイスのボタン最大数
#define CONFIG_POINTERBTT_MAXNO 15  //ボタン番号の最大値
#define CONFIG_GRIDREG_NUM		6	//グリッド登録
#define CONFIG_WATER_PRESET_NUM 5	//水彩プリセット数


/** 新規作成の履歴・記録データ */

typedef struct
{
	int w,h,dpi;
	uint8_t unit; //0:px 1:cm 2:inch 255:none
}ImageSizeData;

/** 保存設定 */

typedef struct
{
	uint32_t flags;
	uint16_t jpeg_sampling_factor;
	uint8_t jpeg_quality,
		png_complevel,
		psd_type;
}ConfigSaveOption;

/** グリッド設定 */

typedef struct
{
	int gridw,gridh,
		splith,splitv;
	uint32_t col_grid,col_split,	//上位8bitが濃度 (100%=128)
		reglist[CONFIG_GRIDREG_NUM];	//幅・高さ登録リスト (上位16bit:幅、下位16bit:高さ)
}ConfigGridData;

/*-----------------*/


/** ConfigData */

typedef struct _ConfigData
{
	ConfigSaveOption save;	//保存設定
	ConfigGridData grid;	//グリッド設定

	ImageSizeData imgsize_recent[CONFIG_IMAGESIZE_NUM],	//新規作成の履歴
		imgsize_record[CONFIG_IMAGESIZE_NUM];			//新規作成の記録

	mBox box_mainwin,			//メインウィンドウの位置とサイズ
		box_textdlg;			//テキストダイアログの位置とサイズ
	mSize size_boxeditdlg;		//矩形編集ダイアログのビュー部分のサイズ (0 で初期値)
	mPoint pt_filterdlg;		//フィルタダイアログ位置

	int dockbrush_height[2],	//ブラシパレットの各高さ
		canvasview_zoom_normal,	//キャンバスビュー:非ルーペ時の倍率 (100%=100)
		canvasview_zoom_loupe,	//ルーペ時の倍率
		savedup_type,			//複製保存時の保存形式 (0 で現在のファイルと同じ)
		undo_maxbufsize;		//アンドゥ最大バッファサイズ

	uint32_t fView,				//表示フラグ
		optflags,				//オプションフラグ
		colCanvasBkgnd,			//キャンバス背景色
		colBkgndPlaid[2],		//イメージ背景色・チェック柄の色
		dockbrush_expand_flags,	//ブラシ:各項目のヘッダの展開フラグ
		water_preset[CONFIG_WATER_PRESET_NUM];	//水彩プリセット (10bit x 3)

	uint16_t pane_width[3],		//各ペインの幅
		init_imgw,				//起動時の初期幅、高さ、DPI
		init_imgh,
		init_dpi,
		canvasZoomStep_low,			//キャンバス倍率、1段階 (100%以下)
		canvasZoomStep_hi,			//キャンバス倍率、1段階 (100%以上)
		canvasAngleStep,			//キャンバス回転角度、1段階
		dragBrushSize_step,			//+Shift ブラシサイズ変更の増減幅
		undo_maxnum,				//アンドゥ最大回数
		iconsize_toolbar,			//アイコンサイズ(ツールバー)
		iconsize_layer,				//アイコンサイズ(レイヤ)
		iconsize_other;				//アイコンサイズ(ほか)

	uint8_t mainwin_maximized,		//メインウィンドウが最大化されているか
		canvas_resize_flags,		//キャンバスサイズ変更フラグ
		canvas_scale_type,			//キャンバス拡大縮小の補間方法
		canvasview_flags,			//キャンバスビュー:フラグ
		canvasview_btt[5],			//キャンバスビュー:操作 (0:L, 1:Ctrl+L, 2:Shift+L, 3:R, 4:M)
		imageviewer_flags,			//イメージビューア:フラグ
		imageviewer_btt[5],			//イメージビューア:操作
		boxedit_flags,				//矩形編集ダイアログのフラグ
		dockcolor_tabno,			//カラー:タブ番号
		dockcolpal_tabno,			//カラーパレット:タブ番号
		dockopt_tabno,				//オプション:タブ番号
		dockcolwheel_type,			//カラーホイールタイプ
		default_btt_cmd[CONFIG_POINTERBTT_NUM],	//デフォルトの各ボタンのコマンド
				/* 0:消しゴム側 1:左ボタン... */
		pentab_btt_cmd[CONFIG_POINTERBTT_NUM];	//ペンタブレットの各ボタンのコマンド

	char pane_layout[5],		//ペイン配置
		dock_layout[16];		//パネル配置順

	mStr strFontStyle_gui,		//GUI フォントスタイル
		strFontStyle_dock,		//パレットフォントスタイル
		strTempDir,				//作業用ディレクトリ (top)
		strTempDirProc,			//プロセスごとの作業用ディレクトリ
		strImageViewerDir,		//イメージビューア開くディレクトリ
		strLayerFileDir,		//レイヤファイルディレクトリ
		strSelectFileDir,		//選択範囲ファイルディレクトリ
		strStampFileDir,		//スタンプ画像ディレクトリ
		strUserTextureDir,		//ユーザーのテクスチャディレクトリ
		strUserBrushDir,		//ユーザーのブラシ画像ディレクトリ
		strLayerNameList,		//レイヤ名登録リスト
		strThemeFile,			//テーマファイル (空でデフォルト)
		strRecentFile[CONFIG_RECENTFILE_NUM],	//最近使ったファイル
		strRecentOpenDir[CONFIG_RECENTDIR_NUM],	//開いたディレクトリの履歴
		strRecentSaveDir[CONFIG_RECENTDIR_NUM];	//保存ディレクトリの履歴

	uint8_t canvaskey[256];		//キャンバスキー (各キーに対応するコマンドID)

	uint8_t *cursor_buf,		//描画用カーソルデータ
		*toolbar_btts;			//ツールバーに表示するボタン (NULL でデフォルト)
	int cursor_bufsize,
		toolbar_btts_size;
}ConfigData;


extern ConfigData  *g_app_config;
#define APP_CONF   g_app_config


/*---- flags ----*/


/* imageviewer_flags */
enum
{
	CONFIG_IMAGEVIEWER_F_FIT = 1<<0,	//全体表示
	CONFIG_IMAGEVIEWER_F_MIRROR = 1<<1	//左右反転表示
};

/* canvas_resize_flags */
#define CONFIG_CANVAS_RESIZE_F_CROP  1  //範囲外を切り取る

/* optflags */
enum
{
	CONFIG_OPTF_MES_SAVE_OVERWRITE = 1<<0,		//上書き保存確認
	CONFIG_OPTF_MES_SAVE_APD = 1<<1,			//上書き時、APD で保存するか確認
	CONFIG_OPTF_APD_NOINCLUDE_BLENDIMG = 1<<2,	//APD 保存時、一枚絵イメージを含めない
	CONFIG_OPTF_FILTERLIST_DBLCLK = 1<<3		//フィルタ一覧はダブルクリックで実行
};

/* fView */
enum
{
	CONFIG_VIEW_F_TOOLBAR = 1<<0,
	CONFIG_VIEW_F_STATUSBAR = 1<<1,
	CONFIG_VIEW_F_DOCKS = 1<<2,
	CONFIG_VIEW_F_BKGND_PLAID = 1<<3,
	CONFIG_VIEW_F_GRID = 1<<4,
	CONFIG_VIEW_F_GRID_SPLIT = 1<<5,
	CONFIG_VIEW_F_FILTERDLG_PREVIEW = 1<<6
};

/* canvasview_flags */
enum
{
	CONFIG_CANVASVIEW_F_LOUPE_MODE = 1<<0,
	CONFIG_CANVASVIEW_F_FIT = 1<<1
};

/* ConfigSaveOption::flags */
enum
{
	CONFIG_SAVEOPTION_F_JPEG_PROGRESSIVE  = 1<<0,
	CONFIG_SAVEOPTION_F_PNG_ALPHA_CHANNEL = 1<<1,
	CONFIG_SAVEOPTION_F_PSD_UNCOMPRESSED  = 1<<2
};

#endif
