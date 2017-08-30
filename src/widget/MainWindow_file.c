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
 * ファイル関連コマンド
 *****************************************/

#include <string.h>

#include "mDef.h"
#include "mStr.h"
#include "mUtilFile.h"
#include "mMenu.h"
#include "mIconButtons.h"
#include "mMessageBox.h"
#include "mSysDialog.h"
#include "mTrans.h"

#include "defMacros.h"
#include "defDraw.h"
#include "defConfig.h"
#include "defMainWindow.h"
#include "defFileFormat.h"
#include "AppErr.h"

#define LOADERR_DEFINE
#include "defLoadErr.h"

#include "defMainWindow.h"
#include "MainWindow.h"
#include "MainWindow_pv.h"
#include "FileDialog.h"
#include "PopupThread.h"
#include "Undo.h"

#include "draw_main.h"
#include "draw_file.h"

#include "trgroup.h"
#include "trid_mainmenu.h"
#include "trid_message.h"


//----------------------

mBool NewImageDialog_run(mWindow *owner,mSize *size,int *dpi,int *layertype);
mBool SaveOptionDlg_run(mWindow *owner,int format);

//----------------------



//============================
// FileFormat
//============================


/** ファイルのヘッダからフォーマット取得 */

uint32_t FileFormat_getbyFileHeader(const char *filename)
{
	uint8_t d[8],png[8] = {0x89,0x50,0x4e,0x47,0x0d,0x0a,0x1a,0x0a};

	if(mReadFileHead(filename, d, 8))
	{
		if(memcmp(d, "AZPDATA", 7) == 0)
		{
			//APD

			if(d[7] < 2)
				return FILEFORMAT_APD | FILEFORMAT_APD_v1v2;
			else if(d[7] == 2)
				return FILEFORMAT_APD | FILEFORMAT_APD_v3;
			else
				return FILEFORMAT_UNKNOWN;
		}
		else if(memcmp(d, "AZDWDAT", 7) == 0)
			return FILEFORMAT_ADW;

		else if(memcmp(d, "8BPS", 4) == 0)
			return FILEFORMAT_PSD;

		else if(memcmp(d, png, 8) == 0)
			return FILEFORMAT_PNG;

		else if(d[0] == 0xff && d[1] == 0xd8)
			return FILEFORMAT_JPEG;

		else if(d[0] == 'G' && d[1] == 'I' && d[2] == 'F')
			return FILEFORMAT_GIF;

		else if(d[0] == 'B' && d[1] == 'M')
			return FILEFORMAT_BMP;
	}

	return FILEFORMAT_UNKNOWN;
}


//============================
// sub
//============================


/** 最近使ったファイルに追加 */

static void _add_recent_file(MainWindow *p,const char *filename)
{
	mStrArrayAddRecent(APP_CONF->strRecentFile, CONFIG_RECENTFILE_NUM, filename, TRUE);

	//メニュー

	MainWindow_setRecentFileMenu(p);
}

/** ファイル履歴のメニューをセット */

void MainWindow_setRecentFileMenu(MainWindow *p)
{
	mMenu *menu = p->menu_recentfile;

	//削除

	mMenuDeleteAll(menu);

	//ファイル

	mMenuSetStrArray(menu, MAINWINDOW_CMDID_RECENTFILE,
		APP_CONF->strRecentFile, CONFIG_RECENTFILE_NUM);

	//消去

	if(!mStrIsEmpty(APP_CONF->strRecentFile))
	{
		mMenuAddSep(menu);

		mMenuAddText_static(menu, TRMENU_FILE_RECENTFILE_CLEAR,
			M_TR_T2(TRGROUP_MAINMENU, TRMENU_FILE_RECENTFILE_CLEAR));
	}
}

/** ツールバーの開く/保存ドロップメニュー
 *
 * @return 選択された履歴の番号。-1 でキャンセル */

int MainWindow_runMenu_toolbarDrop_opensave(MainWindow *p,mBool save)
{
	mMenu *menu;
	mMenuItemInfo *mi;
	mStr *str_array;
	mBox box;
	int no;

	str_array = (save)? APP_CONF->strRecentSaveDir: APP_CONF->strRecentOpenDir;

	//履歴が一つもない (先頭が空)

	if(mStrIsEmpty(str_array)) return -1;

	//メニュー

	menu = mMenuNew();

	mMenuSetStrArray(menu, 0, str_array, CONFIG_RECENTDIR_NUM);

	mIconButtonsGetItemBox(M_ICONBUTTONS(p->toolbar),
		(save)? TRMENU_FILE_SAVE_AS: TRMENU_FILE_OPEN, &box, TRUE);

	mi = mMenuPopup(menu, NULL, box.x, box.y + box.h, 0);
	no = (mi)? mi->id: -1;

	mMenuDestroy(menu);

	return no;
}

/** 複製保存のドロップメニュー
 *
 * @return 選択された履歴の番号。-1 でキャンセル */

int MainWindow_runMenu_toolbarDrop_savedup(MainWindow *p)
{
	mMenu *menu,*sub;
	mMenuItemInfo *mi;
	mBox box;
	int no,i;
	static const char *format_str[] = {
		"APD (AzPainter)", "PSD (PhotoShop)", "PNG", "JPEG", "BMP"
	};

	M_TR_G(TRGROUP_DROPMENU_SAVEDUP);

	//保存形式サブメニュー

	sub = mMenuNew();

	mMenuAddNormal(sub, 1000, M_TR_T(1), 0, MMENUITEM_F_RADIO);

	for(i = 0; i < 5; i++)
		mMenuAddNormal(sub, 1001 + i, format_str[i], 0, MMENUITEM_F_RADIO);

	mMenuSetCheck(sub, 1000 + APP_CONF->savedup_type, 1);

	//main

	menu = mMenuNew();

	mMenuAddSubmenu(menu, 100, M_TR_T(0), sub);
	mMenuAddSep(menu);
	mMenuAddStrArray(menu, 0, APP_CONF->strRecentSaveDir, CONFIG_RECENTDIR_NUM);
	
	//

	mIconButtonsGetItemBox(M_ICONBUTTONS(p->toolbar),
		TRMENU_FILE_SAVE_DUP, &box, TRUE);

	mi = mMenuPopup(menu, NULL, box.x, box.y + box.h, 0);
	no = (mi)? mi->id: -1;

	mMenuDestroy(menu);

	//保存形式変更

	if(no >= 1000)
	{
		APP_CONF->savedup_type = no - 1000;
		return -1;
	}

	return no;
}

/** ツールバーのファイル履歴メニュー */

void MainWindow_runMenu_toolbar_recentfile(MainWindow *p)
{
	mBox box;

	//履歴が一つもない (先頭が空)

	if(mStrIsEmpty(APP_CONF->strRecentFile)) return;

	//メニュー (COMMAND イベントを送る)

	mIconButtonsGetItemBox(M_ICONBUTTONS(p->toolbar),
		TRMENU_FILE_RECENTFILE, &box, TRUE);

	mMenuPopup(p->menu_recentfile, M_WIDGET(p), box.x, box.y + box.h, 0);
}


//============================
// 新規作成
//============================


/** 新規作成 */

void MainWindow_newImage(MainWindow *p)
{
	mSize size;
	int dpi,type;

	//ダイアログ

	if(!NewImageDialog_run(M_WINDOW(p), &size, &dpi, &type))
		return;

	//保存確認

	if(!MainWindow_confirmSave(p)) return;

	//新規

	if(!drawImage_new(APP_DRAW, size.w, size.h, dpi, type))
	{
		MainWindow_apperr(APPERR_ALLOC, NULL);
		
		drawImage_new(APP_DRAW, 300, 300, dpi, type);
	}

	//更新

	MainWindow_updateNewCanvas(p, "");
}


//============================
// ファイルを開く
//============================


typedef struct
{
	const char *filename;
	char *errmes;	//確保されたエラー文字列
	int loaderr,	//LOADERR_*
		loadopt;	//開くダイアログ時のオプション
	uint32_t format;
}_thread_openfileinfo;


/** 読み込みスレッド処理 */
/*
 * 戻り値が APPERR_OK で成功。それ以外の場合エラー。
 * loaderr に LOADERR_OK 以外をセットすると、定義されたエラー文字列表示。
 */

static int _thread_load(mPopupProgress *prog,void *data)
{
	_thread_openfileinfo *p = (_thread_openfileinfo *)data;
	uint32_t format;
	int ret = APPERR_LOAD,loaderr = -100;

	format = p->format;

	if(format & FILEFORMAT_APD)
	{
		//APD

		if(format & FILEFORMAT_APD_v3)
		{
			//ver 3
			if(drawFile_load_apd_v3(APP_DRAW, p->filename, prog))
				ret = APPERR_OK;
		}
		else
			//ver 1,2
			loaderr = drawFile_load_apd_v1v2(p->filename, prog);
	}
	else if(format & FILEFORMAT_ADW)
		//ADW
		loaderr = drawFile_load_adw(p->filename, prog);
	else if(format & FILEFORMAT_PSD)
	{
		//PSD

		if(drawFile_load_psd(APP_DRAW, p->filename, prog, &p->errmes))
			ret= APPERR_OK;
	}
	else
	{
		//画像ファイル
		
		ret = drawImage_loadFile(APP_DRAW, p->filename, p->format,
			FILEDIALOG_LAYERIMAGE_IS_IGNORE_ALPHA(p->loadopt),
			prog, &p->errmes);
	}

	//loaderr に戻り値がある場合、
	//関数の戻り値と loaderr セット

	if(loaderr != -100)
	{
		if(loaderr == LOADERR_OK)
			ret = APPERR_OK;
		else
		{
			p->loaderr = loaderr;
			ret = APPERR_LOAD;
		}
	}

	return ret;
}

/** ファイルを開く (ダイアログ)
 *
 * @param recentno  ディレクトリ履歴番号 (0 で最新の履歴) */

void MainWindow_openFile(MainWindow *p,int recentno)
{
	mStr str = MSTR_INIT;
	int ret;

	//ファイル名

	ret = FileDialog_openLayerImage(M_WINDOW(p),
		"Image Files (ADW/APD/PSD/BMP/PNG/JPEG/GIF)\t*.adw;*.apd;*.psd;*.bmp;*.png;*.jpg;*.jpeg;*.gif\t"
		"AzPainter File (*.apd)\t*.apd\t"
		"All Files\t*",
		APP_CONF->strRecentOpenDir[recentno].buf, &str);

	if(ret == -1) return;

	//保存確認後、読み込み

	if(MainWindow_confirmSave(p))
		MainWindow_loadImage(p, str.buf, ret);

	mStrFree(&str);
}

/** 画像読み込み処理
 *
 * @param loadopt 開くダイアログからのオプション (0 でデフォルト) */

mBool MainWindow_loadImage(MainWindow *p,const char *filename,int loadopt)
{
	_thread_openfileinfo dat;
	int err;
	uint32_t format;

	//ヘッダからフォーマット取得

	format = FileFormat_getbyFileHeader(filename);

	if(format == FILEFORMAT_UNKNOWN)
	{
		MainWindow_apperr(APPERR_UNSUPPORTED_FORMAT, NULL);
		return FALSE;
	}

	//読み込み

	dat.filename = filename;
	dat.format = format;
	dat.errmes = NULL;
	dat.loaderr = LOADERR_OK;
	dat.loadopt = loadopt;

	err = PopupThread_run(&dat, _thread_load);
	if(err == -1) return FALSE;

	//結果

	if(err == APPERR_OK)
	{
		//---- 成功

		mStr str = MSTR_INIT;

		//フォーマット

		p->fileformat = format;

		//ファイル履歴追加
		/* filename が履歴内の文字列の場合があるので、以降
		 * ファイル名を参照する場合は、ファイル履歴の先頭を参照すること。 */

		_add_recent_file(p, filename);

		//ディレクトリ履歴

		mStrPathGetDir(&str, APP_CONF->strRecentFile[0].buf);

		mStrArrayAddRecent(APP_CONF->strRecentOpenDir, CONFIG_RECENTDIR_NUM, str.buf, TRUE);

		mStrFree(&str);

		//更新

		MainWindow_updateNewCanvas(p, APP_CONF->strRecentFile[0].buf);

		return TRUE;
	}
	else
	{
		//---- 失敗

		//エラーメッセージ

		if(dat.errmes)
		{
			MainWindow_apperr(err, dat.errmes);
			mFree(dat.errmes);
		}
		else if(dat.loaderr != LOADERR_OK)
			MainWindow_apperr(err, g_loaderr_str[dat.loaderr]);

		/* レイヤが一つもなければ新規作成。
		 * カレントレイヤが指定されていなければ途中まで読み込まれた */

		if(drawImage_onLoadError(APP_DRAW))
			MainWindow_updateNewCanvas(p, filename);
		   
		return FALSE;
	}
}


//============================
// ファイル保存 sub
//============================


enum
{
	FILETYPE_APD,
	FILETYPE_PSD,
	FILETYPE_PNG,
	FILETYPE_JPEG,
	FILETYPE_BMP
};


/** フォーマットフラグからファイル名に拡張子セット */

static void _save_set_ext(mStr *str,uint32_t format)
{
	uint32_t flag[] = {
		FILEFORMAT_APD, FILEFORMAT_PSD, FILEFORMAT_PNG,
		FILEFORMAT_JPEG, FILEFORMAT_BMP, 0
	};
	const char *ext[] = {
		"apd", "psd", "png", "jpg", "bmp"
	};
	int i;
		
	for(i = 0; flag[i]; i++)
	{
		if(format & flag[i])
		{
			mStrPathSetExt(str, ext[i]);
			break;
		}
	}
}

/** フォーマットフラグからファイルタイプ取得 */

static int _save_format_to_type(uint32_t format)
{
	uint32_t flag[] = {
		FILEFORMAT_APD, FILEFORMAT_PSD, FILEFORMAT_PNG,
		FILEFORMAT_JPEG, FILEFORMAT_BMP, 0
	};
	int i;
		
	for(i = 0; flag[i]; i++)
	{
		if(format & flag[i])
			return i;
	}

	return FILETYPE_APD;
}

/** 上書き保存時のメッセージ
 *
 * @return [0]キャンセル [1]上書き保存 [2]別名保存 */

static int _save_message(MainWindow *p)
{
	uint32_t ret;

	//ADW/GIF 形式での上書き保存は不可 => 別名保存

	if(p->fileformat & (FILEFORMAT_ADW | FILEFORMAT_GIF))
		return 2;

	//APD 以外の場合、確認

	if(!(p->fileformat & FILEFORMAT_APD)
		&& (APP_CONF->optflags & CONFIG_OPTF_MES_SAVE_APD))
	{
		ret = mMessageBox(M_WINDOW(p), NULL, M_TR_T2(TRGROUP_MESSAGE, TRID_MES_SAVE_APD),
			MMESBOX_YES | MMESBOX_CANCEL | MMESBOX_NOTSHOW, MMESBOX_CANCEL);

		//メッセージ表示しない

		if(ret & MMESBOX_NOTSHOW)
			APP_CONF->optflags ^= CONFIG_OPTF_MES_SAVE_APD;

		return (ret & MMESBOX_YES)? 1: 0;
	}

	//上書き保存メッセージ

	if(APP_CONF->optflags & CONFIG_OPTF_MES_SAVE_OVERWRITE)
	{
		mStr str = MSTR_INIT;

		mStrPathGetFileName(&str, p->strFilename.buf);
		mStrAppendText(&str, "\n\n");
		mStrAppendText(&str, M_TR_T2(TRGROUP_MESSAGE, TRID_MES_SAVE_OVERWRITE));

		ret = mMessageBox(M_WINDOW(p), NULL, str.buf,
			MMESBOX_SAVE | MMESBOX_CANCEL | MMESBOX_NOTSHOW, MMESBOX_SAVE);

		mStrFree(&str);

		//メッセージ表示しない

		if(ret & MMESBOX_NOTSHOW)
			APP_CONF->optflags ^= CONFIG_OPTF_MES_SAVE_OVERWRITE;

		//キャンセル

		if(!(ret & MMESBOX_SAVE))
			return 0;
	}

	return 1;
}

/** ファイル名とフォーマットを取得
 *
 * @return 保存フォーマット。-1 でキャンセル、-2 で上書き保存 */

static int _save_get_path_and_format(MainWindow *p,
	mStr *strpath,int savetype,int recentno)
{
	int ret,type,format;
	int type_to_format[] = {
		FILEFORMAT_APD, FILEFORMAT_PSD, FILEFORMAT_PNG,
		FILEFORMAT_JPEG, FILEFORMAT_BMP
	};

	//-------- 上書き保存
	/* 新規状態、または上書きできない場合は別名保存へ */

	if(savetype == MAINWINDOW_SAVEFILE_OVERWRITE && !mStrIsEmpty(&p->strFilename))
	{
		ret = _save_message(p);

		if(ret == 0)
			//キャンセル
			return -1;
		else if(ret == 1)
		{
			//上書き保存

			mStrCopy(strpath, &p->strFilename);
			return -2;
		}
	}

	//------- 別名保存、複製保存

	//初期ファイル名

	if(!mStrIsEmpty(&p->strFilename))
		mStrPathGetFileNameNoExt(strpath, p->strFilename.buf);

	//種類の初期選択

	type = FILETYPE_APD;

	if(savetype == MAINWINDOW_SAVEFILE_DUP)
	{
		//複製時は指定フォーマット

		if(APP_CONF->savedup_type == 0)
		{
			//現在のファイルと同じ

			if(!mStrIsEmpty(&p->strFilename))
				type = _save_format_to_type(p->fileformat);
		}
		else
			type = APP_CONF->savedup_type - 1;
	}

	//ダイアログ

	ret = mSysDlgSaveFile(M_WINDOW(p),
		"AzPainter (*.apd)\t*.apd\t"
		"PhotoShop (*.psd)\t*.psd\t"
		"PNG (*.png)\t*.png\t"
		"JPEG (*.jpg)\t*.jpg\t"
		"BMP (*.bmp)\t*.bmp\t",
		type, APP_CONF->strRecentSaveDir[recentno].buf, 0, strpath, &type);

	if(!ret) return -1;

	//フォーマット取得

	format = type_to_format[type];

	//拡張子セット

	_save_set_ext(strpath, format);

	return format;
}


//============================
// ファイル保存 main
//============================


typedef struct
{
	const char *filename;
	uint32_t format;
}_thdata_save;


/** 保存スレッド処理 */

static int _thread_save(mPopupProgress *prog,void *data)
{
	_thdata_save *p = (_thdata_save *)data;
	int ret;
	uint32_t format;

	format = p->format;

	//PNG + ALPHA

	if((format & FILEFORMAT_PNG)
		&& (APP_CONF->save.flags & CONFIG_SAVEOPTION_F_PNG_ALPHA_CHANNEL))
		format |= FILEFORMAT_ALPHA_CHANNEL;
			
	//

	if(format & FILEFORMAT_APD)
	{
		//----- APD v3
		
		ret = drawFile_save_apd_v3(APP_DRAW, p->filename, prog);
	}
	else if(format & FILEFORMAT_PSD)
	{
		//----- PSD
	
		if(!drawImage_blendImage_RGB8(APP_DRAW))
			return FALSE;
		
		if(APP_CONF->save.psd_type == 0)
			//レイヤ維持
			ret = drawFile_save_psd_layer(APP_DRAW, p->filename, prog);
		else
		{
			//1枚絵
			ret = drawFile_save_psd_image(APP_DRAW, APP_CONF->save.psd_type,
				p->filename, prog);
		}

		drawUpdate_blendImage_full(APP_DRAW);
	}
	else
	{
		//----- PNG/JPEG/BMP

		if(format & FILEFORMAT_ALPHA_CHANNEL)
			//RGBA8
			drawImage_blendImage_RGBA8(APP_DRAW);
		else
		{
			//RGB8
			
			if(!drawImage_blendImage_RGB8(APP_DRAW))
				return FALSE;
		}

		ret = drawFile_save_image(APP_DRAW, p->filename, format, prog);

		drawUpdate_blendImage_full(APP_DRAW);
	}

	return ret;
}

/** ファイル保存
 *
 * @param savetype [0]上書き保存 [1]別名保存 [2]複製保存
 * @param recentno ディレクトリ履歴番号
 * @param FALSE でキャンセルされた */

mBool MainWindow_saveFile(MainWindow *p,int savetype,int recentno)
{
	mStr str = MSTR_INIT,strdir = MSTR_INIT;
	uint32_t format;
	int ret;
	_thdata_save dat;

	//パスとフォーマット取得

	ret = _save_get_path_and_format(p, &str, savetype, recentno);
	if(ret == -1)
	{
		mStrFree(&str);
		return FALSE;
	}

	format = (ret == -2)? p->fileformat: ret;

	//保存設定 (PNG/JPEG/PSD)
	/* 上書き保存で、開いた後一度保存されている場合を除いて保存設定を行う。 */

	if(!(ret == -2 && p->saved)
		&& (format & (FILEFORMAT_PNG | FILEFORMAT_JPEG | FILEFORMAT_PSD)))
	{
		if(!SaveOptionDlg_run(M_WINDOW(p), format))
		{
			mStrFree(&str);
			return FALSE;
		}
	}

	//スレッド

	dat.filename = str.buf;
	dat.format = format;

	ret = PopupThread_run(&dat, _thread_save);

	//失敗

	if(ret != 1)
	{
		MainWindow_apperr(APPERR_FAILED, NULL);
		
		mStrFree(&str);
		return TRUE;
	}

	//----- 成功

	//編集ファイル情報

	if(savetype != MAINWINDOW_SAVEFILE_DUP)
	{
		mStrCopy(&p->strFilename, &str);

		p->fileformat = format;
		p->saved = TRUE;

		//タイトル

		MainWindow_setTitle(p);

		//undo データ変更フラグ OFF

		Undo_clearUpdateFlag();
	}

	//ファイル履歴

	_add_recent_file(p, str.buf);
	
	//ディレクトリ履歴

	mStrPathGetDir(&strdir, str.buf);

	mStrArrayAddRecent(APP_CONF->strRecentSaveDir, CONFIG_RECENTDIR_NUM, strdir.buf, TRUE);

	//

	mStrFree(&str);
	mStrFree(&strdir);

	return TRUE;
}
