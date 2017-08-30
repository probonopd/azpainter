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
 * レイヤのコマンド
 *****************************************/

#include "mDef.h"
#include "mStr.h"
#include "mTrans.h"
#include "mSysDialog.h"

#include "defWidgets.h"
#include "defConfig.h"
#include "defDraw.h"
#include "AppErr.h"

#include "MainWindow.h"
#include "PopupThread.h"
#include "FileDialog.h"
#include "Docks_external.h"
#include "LayerDialogs.h"

#include "LayerList.h"
#include "LayerItem.h"
#include "TileImage.h"

#include "draw_main.h"
#include "draw_layer.h"

#include "trgroup.h"
#include "trid_word.h"
#include "trid_mainmenu.h"



/** 新規作成
 *
 * @param pi_above このレイヤの上に作成 */

void MainWindow_layer_new_dialog(MainWindow *p,LayerItem *pi_above)
{
	LayerNewDlgInfo info;

	mMemzero(&info, sizeof(info));

	//デフォルト名

	mStrSetFormat(&info.strname, "%s%d",
		M_TR_T2(TRGROUP_WORD, TRID_WORD_LAYER), LayerList_getNum(APP_DRAW->layerlist));

	//ダイアログ

	if(LayerNewDlg_run(M_WINDOW(p), &info))
	{
		//追加
		
		if(info.coltype < 0)
			drawLayer_newFolder(APP_DRAW, info.strname.buf, pi_above);
		else
		{
			drawLayer_newLayer(APP_DRAW, info.strname.buf, info.coltype,
				info.blendmode, info.col, pi_above);
		}
	}

	mStrFree(&info.strname);
}

/** ファイルから読み込み */

static void _load_file(MainWindow *p)
{
	mStr str = MSTR_INIT;
	int ret,err;

	ret = FileDialog_openLayerImage(M_WINDOW(p),
		"BMP/PNG/GIF/JPEG/APD(v3)\t*.bmp;*.png;*.gif;*.jpg;*.jpeg;*.apd\tAll Files\t*",
		APP_CONF->strLayerFileDir.buf, &str);

	if(ret >= 0)
	{
		//フォルダ記録

		mStrPathGetDir(&APP_CONF->strLayerFileDir, str.buf);

		//読み込み

		err = drawLayer_newLayer_file(APP_DRAW, str.buf,
			FILEDIALOG_LAYERIMAGE_IS_IGNORE_ALPHA(ret), NULL);

		if(err != APPERR_OK)
			MainWindow_apperr(err, NULL);
	}

	mStrFree(&str);
}

/** レイヤ設定 */

void MainWindow_layer_setoption(MainWindow *p,LayerItem *pi)
{
	mStr str = MSTR_INIT;
	mBool bimg,update_canvas;
	int blendmode;
	uint32_t col;

	bimg = LAYERITEM_IS_IMAGE(pi);

	mStrSetText(&str, pi->name);
	blendmode = pi->blendmode;
	col = pi->col;

	if(LayerOptionDlg_run(M_WINDOW(p), &str,
		(bimg)? &blendmode: NULL,
		(bimg && LayerItem_isType_layercolor(pi))? &col: NULL))
	{
		update_canvas = (pi->blendmode != blendmode || pi->col != col);

		//セット
	
		mStrdup_ptr(&pi->name, str.buf);

		pi->blendmode = blendmode;

		LayerItem_setLayerColor(pi, col);

		//更新

		if(update_canvas)
		{
			//合成モードか線の色変更でキャンバス更新
			
			drawUpdate_rect_imgcanvas_canvasview_inLayerHave(APP_DRAW, pi);
			DockLayer_update_layer_curparam(pi);
		}
		else
			//名前のみ
			DockLayer_update_layer(pi);
	}

	mStrFree(&str);
}

/** 線の色変更 */

void MainWindow_layer_setcolor(MainWindow *p,LayerItem *pi)
{
	uint32_t col;

	if(LayerItem_isType_layercolor(pi))
	{
		col = pi->col;
		
		if(LayerColorDlg_run(M_WINDOW(p), &col))
			drawLayer_setLayerColor(APP_DRAW, pi, col);
	}
}

/** カラータイプ変更 */

void MainWindow_layer_settype(MainWindow *p,LayerItem *pi)
{
	int ret;

	if(LAYERITEM_IS_FOLDER(pi)) return;

	ret = LayerTypeDlg_run(M_WINDOW(p), pi->coltype);
	if(ret != -1)
		drawLayer_setColorType(APP_DRAW, pi, ret & 0xff, (ret & (1<<9)) != 0);
}

/** 複数レイヤ結合 */

static void _combine_multi(MainWindow *p)
{
	int ret;

	ret = LayerCombineDlg_run(M_WINDOW(p),
		LAYERITEM_IS_FOLDER(APP_DRAW->curlayer),
		LayerList_haveCheckedLayer(APP_DRAW->layerlist));

	if(ret != -1)
	{
		drawLayer_combineMulti(APP_DRAW,
			LAYERCOMBINEDLG_GET_TARGET(ret),
			LAYERCOMBINEDLG_IS_NEWLAYER(ret),
			LAYERCOMBINEDLG_GET_COLTYPE(ret));
	}
}


//==============================
// ファイルに出力
//==============================


typedef struct
{
	const char *filename;
	int type;
}_thdata_savefile;


/** [スレッド] ファイルに出力 */

static int _thread_save_file(mPopupProgress *prog,void *data)
{
	_thdata_savefile *p = (_thdata_savefile *)data;

	if(p->type == 0)
		//apd
		return LayerItem_saveAPD_single(APP_DRAW->curlayer, p->filename, prog);
	else
	{
		//png

		return TileImage_savePNG_rgba(APP_DRAW->curlayer->img,
				p->filename, APP_DRAW->imgdpi, prog);
	}
}

/** ファイルに出力 */

static void _save_file(MainWindow *p)
{
	mStr str = MSTR_INIT;
	_thdata_savefile dat;
	int type;

	//フォルダは除く

	if(LAYERITEM_IS_FOLDER(APP_DRAW->curlayer)) return;

	//初期ファイル名 (レイヤ名から)

	mStrSetText(&str, APP_DRAW->curlayer->name);
	mStrPathReplaceDisableChar(&str, '_');

	//ファイル名取得

	if(mSysDlgSaveFile(M_WINDOW(p),
		"AzPainter file v3 (*.apd)\t*.apd\tPNG file (*.png)\t*.png",
		0, APP_CONF->strLayerFileDir.buf, 0, &str, &type))
	{
		//拡張子

		mStrPathSetExt(&str, (type == 0)? "apd": "png");

		//ディレクトリ記録

		mStrPathGetDir(&APP_CONF->strLayerFileDir, str.buf);

		//保存

		dat.filename = str.buf;
		dat.type = type;

		if(PopupThread_run(&dat, _thread_save_file) != 1)
			MainWindow_apperr(APPERR_FAILED, NULL);
	}

	mStrFree(&str);
}


//========================


/** コマンド処理 */

void MainWindow_layercmd(MainWindow *p,int id)
{
	DrawData *draw = APP_DRAW;

	switch(id)
	{
		//----- ツールバー
		
		//上へ移動
		case TRMENU_LAYER_TB_MOVE_UP:
			drawLayer_moveUpDown(draw, TRUE);
			break;
		//下へ移動
		case TRMENU_LAYER_TB_MOVE_DOWN:
			drawLayer_moveUpDown(draw, FALSE);
			break;

		//------ メインメニュー
	
		//新規作成
		case TRMENU_LAYER_NEW:
			MainWindow_layer_new_dialog(p, NULL);
			break;
		//新規フォルダ
		case TRMENU_LAYER_NEW_FOLDER:
			drawLayer_newLayer_direct(draw, -1);
			break;
		//ファイルから新規レイヤ
		case TRMENU_LAYER_NEW_FROM_FILE:
			_load_file(p);
			break;
		//複製
		case TRMENU_LAYER_COPY:
			drawLayer_copy(draw);
			break;
		//削除
		case TRMENU_LAYER_DELETE:
			drawLayer_delete(draw, TRUE);
			break;
		//イメージ消去
		case TRMENU_LAYER_ERASE:
			drawLayer_erase(draw);
			break;

		//レイヤ設定
		case TRMENU_LAYER_OPTION:
			MainWindow_layer_setoption(p, draw->curlayer);
			break;
		//線の色変更
		case TRMENU_LAYER_SETCOLOR:
			MainWindow_layer_setcolor(p, draw->curlayer);
			break;
		//カラータイプ変更
		case TRMENU_LAYER_SETTYPE:
			MainWindow_layer_settype(p, draw->curlayer);
			break;

		//下レイヤに移す
		case TRMENU_LAYER_DROP:
			drawLayer_combine(draw, TRUE);
			break;
		//下レイヤと結合
		case TRMENU_LAYER_COMBINE:
			drawLayer_combine(draw, FALSE);
			break;
		//複数レイヤ結合
		case TRMENU_LAYER_COMBINE_MULTI:
			_combine_multi(p);
			break;
		//画像の統合
		case TRMENU_LAYER_BLEND_ALL:
			drawLayer_blendAll(draw);
			break;

		//すべてのフォルダを閉じる
		case TRMENU_LAYER_CLOSE_ALL_FOLDERS:
			drawLayer_closeAllFolders(draw);
			break;

		//左右反転
		case TRMENU_LAYER_EDIT_REV_HORZ:
			drawLayer_editFullImage(draw, 0);
			break;
		//上下反転
		case TRMENU_LAYER_EDIT_REV_VERT:
			drawLayer_editFullImage(draw, 1);
			break;
		//左に回転
		case TRMENU_LAYER_EDIT_ROTATE_LEFT:
			drawLayer_editFullImage(draw, 2);
			break;
		//右に回転
		case TRMENU_LAYER_EDIT_ROTATE_RIGHT:
			drawLayer_editFullImage(draw, 3);
			break;

		//すべて表示
		case TRMENU_LAYER_VIEW_ALL_SHOW:
			drawLayer_showAll(draw, 1);
			break;
		//すべて非表示
		case TRMENU_LAYER_VIEW_ALL_HIDE:
			drawLayer_showAll(draw, 0);
			break;
		//カレントレイヤのみ表示
		case TRMENU_LAYER_VIEW_ONLY_CURRENT:
			drawLayer_showAll(draw, 2);
			break;
		//チェックレイヤの表示反転
		case TRMENU_LAYER_VIEW_REV_CHECKED:
			drawLayer_showRevChecked(draw);
			break;
		//フォルダを除くレイヤの表示反転
		case TRMENU_LAYER_VIEW_REV_IMAGE:
			drawLayer_showRevImage(draw);
			break;

		//ロック解除
		case TRMENU_LAYER_FLAG_OFF_LOCK:
			drawLayer_allFlagsOff(draw, LAYERITEM_F_LOCK);
			break;
		//塗りつぶし判定元解除
		case TRMENU_LAYER_FLAG_OFF_FILL_REF:
			drawLayer_allFlagsOff(draw, LAYERITEM_F_FILLREF);
			break;
		//チェック解除
		case TRMENU_LAYER_FLAG_OFF_CHECKED:
			drawLayer_allFlagsOff(draw, LAYERITEM_F_CHECKED);
			break;

		//ファイルに出力
		case TRMENU_LAYER_OUTPUT:
			_save_file(p);
			break;
	}
}
