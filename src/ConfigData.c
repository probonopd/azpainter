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
 * ConfigData 関数
 * 
 * (環境設定関連の処理)
 *****************************************/

#include <string.h>

#include "mDef.h"
#include "mAppDef.h"
#include "mStr.h"
#include "mPath.h"
#include "mUtilFile.h"
#include "mDirEntry.h"

#include "defConfig.h"



/** 解放 */

void ConfigData_free()
{
	ConfigData *p = APP_CONF;

	if(p)
	{
		mStrFree(&p->strFontStyle_gui);
		mStrFree(&p->strFontStyle_dock);
		mStrFree(&p->strTempDir);
		mStrFree(&p->strTempDirProc);
		mStrFree(&p->strImageViewerDir);
		mStrFree(&p->strLayerFileDir);
		mStrFree(&p->strSelectFileDir);
		mStrFree(&p->strStampFileDir);
		mStrFree(&p->strUserTextureDir);
		mStrFree(&p->strUserBrushDir);
		mStrFree(&p->strLayerNameList);
		mStrFree(&p->strThemeFile);

		mStrArrayFree(p->strRecentFile, CONFIG_RECENTFILE_NUM);
		mStrArrayFree(p->strRecentOpenDir, CONFIG_RECENTDIR_NUM);
		mStrArrayFree(p->strRecentSaveDir, CONFIG_RECENTDIR_NUM);

		mFree(p->toolbar_btts);
		mFree(p->cursor_buf);

		mFree(p);
	}
}

/** ConfigData 作成 */

mBool ConfigData_new()
{
	ConfigData *p;

	p = (ConfigData *)mMalloc(sizeof(ConfigData), TRUE);
	if(!p)
		return FALSE;
	else
	{
		APP_CONF = p;
		return TRUE;
	}
}

/** 作業用ディレクトリのデフォルトパスをセット (ini 読み込み後) */

void ConfigData_setTempDir_default()
{
	char *path;

	if(mStrIsEmpty(&APP_CONF->strTempDir))
	{
		path = mGetTempPath();
	
		mStrSetText(&APP_CONF->strTempDir, path);

		mFree(path);
	}
}

/** 作業用ディレクトリ削除 */

void ConfigData_deleteTempDir()
{
	mDirEntry *dir;
	mStr str = MSTR_INIT;

	//パスが空ならなし

	if(mStrIsEmpty(&APP_CONF->strTempDirProc)) return;

	//残っているファイルを削除

	dir = mDirEntryOpen(APP_CONF->strTempDirProc.buf);
	if(!dir) return;

	while(mDirEntryRead(dir))
	{
		if(!mDirEntryIsDirectory(dir))
		{
			mDirEntryGetFileName_str(dir, &str, TRUE);
			mDeleteFile(str.buf);
		}
	}

	mDirEntryClose(dir);

	mStrFree(&str);

	//ディレクトリを削除

	mDeleteDir(APP_CONF->strTempDirProc.buf);
}

/** 作業用ディレクトリ作成
 *
 * strTmpDirProc に実際のディレクトリパスセット。 */

void ConfigData_createTempDir()
{
	char *name;
	mStr *pstr;

	//パスセット先

	pstr = &APP_CONF->strTempDirProc;

	//'<tmp>/azpainter<process>'

	mStrCopy(pstr,  &APP_CONF->strTempDir);
	mStrPathAdd(pstr, "azpainter");

	name = mGetProcessTempName();
	mStrAppendText(pstr, name);
	mFree(name);

	//存在していたら削除

	if(mIsFileExist(pstr->buf, TRUE))
		ConfigData_deleteTempDir();

	//作成 (失敗したらパスを空に)

	if(!mCreateDir(pstr->buf))
		mStrFree(pstr);
}

/** 作業用ディレクトリのパス取得
 *
 * @return FALSE でパスなし(作業用ディレクトリなし) */

mBool ConfigData_getTempPath(mStr *str,const char *add)
{
	if(mStrIsEmpty(&APP_CONF->strTempDirProc))
		return FALSE;
	else
	{
		mStrCopy(str, &APP_CONF->strTempDirProc);
		mStrPathAdd(str, add);

		return TRUE;
	}
}

/** 水彩プリセットをデフォルトにセット */

void ConfigData_waterPreset_default()
{
	uint32_t dat[] = {
		800|(1000<<10)|(500<<20), 500|(900<<10)|(400<<20), 300|(600<<10)|(200<<20),
		100|(1000<<10)|(50<<20), 100|(100<<10)|(200<<20)
	};

	memcpy(APP_CONF->water_preset, dat, CONFIG_WATER_PRESET_NUM * 4);
}
