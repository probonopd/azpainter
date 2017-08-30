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

/************************************
 * 環境設定定義
 ************************************/

#include "mStrDef.h"


/** 作業用データ */

typedef struct
{
	uint8_t fchange;	//データ変更フラグ

	int initw,
		inith,
		init_dpi,
		undo_maxnum,
		undo_maxbufsize,
		step[4],
		iconsize[3],
		cursor_bufsize,
		toolbar_btts_size;
	uint32_t canvcol[3],	//0:キャンバス背景 1,2:チェック柄色
		optflags;
	uint8_t default_btt_cmd[CONFIG_POINTERBTT_NUM],
		pentab_btt_cmd[CONFIG_POINTERBTT_NUM],
		*cursor_buf,
		*toolbar_btts;
	mStr strSysDir[3],
		strThemeFile,
		strFontStyle[2];
}EnvOptDlgCurData;

/** fchange */

enum
{
	ENVOPTDLG_CHANGE_F_CURSOR = 1<<0,
	ENVOPTDLG_CHANGE_F_TOOLBAR_BTTS = 1<<1
};

/** 文字列ID */

enum
{
	//タブ
	TRID_TAB_OPT1 = 1,
	TRID_TAB_OPT2,
	TRID_TAB_STEP,
	TRID_TAB_BTT,
	TRID_TAB_SYSTEM,
	TRID_TAB_CURSOR,
	TRID_TAB_INTERFACE,
	TRID_TAB_OPT3,

	//設定1
	TRID_OPT1_INIT_W = 100,
	TRID_OPT1_INIT_H,
	TRID_OPT1_BKGND_CANVAS,
	TRID_OPT1_BKGND_PLAID,
	TRID_OPT1_UNDO_NUM,
	TRID_OPT1_UNDO_BUFSIZE,
	TRID_OPT1_INIT_DPI,

	//設定2
	TRID_OPT2_TOP = 200,

	//増減
	TRID_STEP_TOP = 300,

	//ボタン
	TRID_BTT_DEFAULT = 400,
	TRID_BTT_PENTAB,
	TRID_BTT_ERASER,
	TRID_BTT_CMD_DEFAULT,
	TRID_BTT_CMD_TOOL,
	TRID_BTT_CMD_OHTER,
	TRID_BTT_CMD_COMMAND,
	TRID_BTT_BUTTON,
	TRID_BTT_BUTTON_GET,
	TRID_BTT_SET,

	//システム
	TRID_SYSTEM_TEMPDIR = 500,
	TRID_SYSTEM_BRUSH_DIR,
	TRID_SYSTEM_TEXTURE_DIR,

	//描画カーソル
	TRID_CURSOR_HOTSPOT = 600,
	TRID_CURSOR_LOAD,
	TRID_CURSOR_DEFAULT,
	TRID_CURSOR_HELP,

	//インターフェイス
	TRID_INTERFACE_THEME,
	TRID_INTERFACE_ICON,
	TRID_INTERFACE_ICON_TOOLBAR,
	TRID_INTERFACE_ICON_LAYER,
	TRID_INTERFACE_ICON_OTHER,
	TRID_INTERFACE_CUSTOMIZE_TOOLBAR,
	TRID_INTERFACE_FONT,
	TRID_INTERFACE_FONT_DEFAULT,
	TRID_INTERFACE_FONT_PANEL,

	//メッセージ
	TRID_MES_TEMPDIR = 1000,
	TRID_MES_UNDOBUFSIZE
};


typedef void (*EnvOptDlg_funcGetValue)(mWidget *,EnvOptDlgCurData *);

mWidget *EnvOptDlg_createContentsBase(mWidget *parent,int size,EnvOptDlg_funcGetValue func);
