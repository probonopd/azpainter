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
 * コマンド関連
 *****************************************/

#include "mDef.h"
#include "mGui.h"
#include "mStr.h"
#include "mWidget.h"
#include "mIconButtons.h"
#include "mDockWidget.h"
#include "mSysDialog.h"
#include "mMessageBox.h"
#include "mTrans.h"

#include "defConfig.h"
#include "defDraw.h"
#include "defWidgets.h"
#include "AppErr.h"
#include "defCanvasKeyID.h"

#include "TileImage.h"
#include "LayerItem.h"
#include "BrushList.h"
#include "Undo.h"
#include "AppResource.h"

#include "defMainWindow.h"
#include "MainWindow.h"
#include "MainWindow_pv.h"
#include "MainWinCanvas.h"
#include "StatusBar.h"
#include "DockObject.h"
#include "Docks_external.h"
#include "CanvasDialogs.h"
#include "EnvOptDlg.h"
#include "PopupThread.h"

#include "draw_main.h"
#include "draw_layer.h"
#include "draw_select.h"
#include "draw_calc.h"

#include "trgroup.h"
#include "AppErr.h"


//--------------

mBool PanelLayoutDlg_run(mWindow *owner);

//--------------


//=======================
// 表示関連
//=======================


/** 各ペインをレイアウト */

void MainWindow_pane_layout(MainWindow *p)
{
	char *pc;
	mWidget *wg,*next;
	int i,no,pane_have[3],spno,pane_cnt;

	//各ペイン、格納するパネルがあるか

	mMemzero(pane_have, sizeof(int) * 3);

	for(i = 0; i < DOCKWIDGET_NUM; i++)
	{
		no = DockObject_getPaneNo_fromNo(i);
		pane_have[no] = 1;
	}

	//リンクを外す

	for(wg = p->ct_main->first; wg; wg = next)
	{
		next = wg->next;
		mWidgetTreeRemove(wg);
	}

	//再リンク

	spno = 0;
	pane_cnt = 0;

	for(pc = APP_CONF->pane_layout; *pc; pc++)
	{
		no = *pc - '0';

		//パネルがないペインは除外

		if(no == 0 || pane_have[no - 1])
		{
			//キャンバス/ペイン
			
			wg = (no == 0)? (mWidget *)APP_WIDGETS->canvas: APP_WIDGETS->pane[no - 1];

			mWidgetTreeAppend(wg, p->ct_main);

			if(no != 0) pane_cnt++;

			//スプリッター

			if(spno < 3)
				mWidgetTreeAppend(p->splitter[spno++], p->ct_main);
		}
	}

	//非表示のペインがある場合、最後に余分なスプリッターが付くので、外す

	if(pane_cnt != 3)
		mWidgetTreeRemove(p->ct_main->last);
}

/** ペインとスプリッターの表示反転 */

void MainWindow_toggle_show_pane_splitter(MainWindow *p)
{
	int i;

	//切り離されているものは除外

	for(i = 0; i < 3; i++)
	{
		if(APP_WIDGETS->pane[i]->parent)
			mWidgetShow(APP_WIDGETS->pane[i], -1);

		if(p->splitter[i]->parent)
			mWidgetShow(p->splitter[i], -1);
	}
}

/** パネルすべてを表示/非表示 */

void MainWindow_cmd_show_panel_all(MainWindow *p)
{
	int i;

	//各パレット表示状態反転

	for(i = 0; i < DOCKWIDGET_NUM; i++)
	{
		if(APP_WIDGETS->dockobj[i])
			mDockWidgetSetVisible(APP_WIDGETS->dockobj[i]->dockwg, -1);
	}

	//ペインとスプリッターの表示反転

	MainWindow_toggle_show_pane_splitter(p);

	mWidgetReLayout(M_WIDGET(p));

	//

	APP_CONF->fView ^= CONFIG_VIEW_F_DOCKS;
}

/** 各パネルの表示状態を反転 */

void MainWindow_cmd_show_panel_toggle(int no)
{
	if(APP_CONF->fView & CONFIG_VIEW_F_DOCKS)
		mDockWidgetShow(APP_WIDGETS->dockobj[no]->dockwg, -1);
}


//=======================
// ダイアログ
//=======================


/** DPI 値変更 */

void MainWindow_cmd_changeDPI(MainWindow *p)
{
	int n;

	M_TR_G(TRGROUP_DLG_CHANGE_DPI);

	n = APP_DRAW->imgdpi;

	if(mSysDlgInputNum(M_WINDOW(p), M_TR_T(0), M_TR_T(1), &n,
		1, 10000, MSYSDLG_INPUTNUM_F_DEFAULT))
	{
		APP_DRAW->imgdpi = n;

		StatusBar_setImageInfo();
	}
}

/** 選択範囲の拡張/縮小 */

void MainWindow_cmd_selectExpand(MainWindow *p)
{
	int n;

	//選択範囲なし

	if(!drawSel_isHave()) return;

	//

	M_TR_G(TRGROUP_DLG_SELECT_EXPAND);

	n = 1;

	if(mSysDlgInputNum(M_WINDOW(p), M_TR_T(0), M_TR_T(1), &n,
		-500, 500, MSYSDLG_INPUTNUM_F_DEFAULT))
	{
		if(n != 0)
			drawSel_expand(APP_DRAW, n);
	}
}

/** マスク類の状態をチェック */

static void _checkmasks_set(mStr *str,int trid,int on)
{
	mStrAppendFormat(str, "[%c]  %s\n", (on)? 'O':'-', M_TR_T(trid));
}

void MainWindow_cmd_checkMaskState(MainWindow *p)
{
	mStr str = MSTR_INIT;
	int i,lf;

	M_TR_G(TRGROUP_CHECK_MASKS);

	//レイヤ名

	mStrSetFormat(&str, "< %s >\n\n", APP_DRAW->curlayer->name);

	//選択範囲

	_checkmasks_set(&str, 1, drawSel_isHave());

	//レイヤ (ロック、下レイヤマスク、アルファマスク)
	
	lf = LayerItem_checkMasks(APP_DRAW->curlayer);

	for(i = 0; i < 3; i++)
		_checkmasks_set(&str, i + 2, lf & (1<<i));

	//マスクレイヤ

	_checkmasks_set(&str, 5, (APP_DRAW->masklayer != 0));

	if(APP_DRAW->masklayer)
		mStrAppendFormat(&str, "    < %s >\n", APP_DRAW->masklayer->name);

	//色マスク

	_checkmasks_set(&str, 6, APP_DRAW->col.colmask_type);

	//

	mMessageBox(M_WINDOW(p), M_TR_T(0), str.buf, MMESBOX_OK, MMESBOX_OK);

	mStrFree(&str);
}

/** 環境設定 */

void MainWindow_cmd_envoption(MainWindow *p)
{
	int f;

	f = EnvOptionDlg_run(M_WINDOW(p));

	//テーマ

	if(f & ENVOPTDLG_F_THEME)
	{
		mAppLoadThemeFile(APP_CONF->strThemeFile.buf);

		DockColor_changeTheme();
		DockColorPalette_changeTheme();
		DockColorWheel_changeTheme();
	}

	//キャンバス

	if(f & ENVOPTDLG_F_UPDATE_ALL)
		//イメージ全体変更 (チェック柄背景色変更)
		/* キャンバス背景色も変わっている場合あり */
		drawUpdate_all();
	else if(f & ENVOPTDLG_F_UPDATE_CANVAS)
	{
		//キャンバス背景色変更
		
		drawUpdate_canvasArea();
		DockCanvasView_update();
	}

	//描画カーソル変更

	if(f & ENVOPTDLG_F_CURSOR)
		MainWinCanvasArea_changeDrawCursor();

	//ツールバーボタン

	if(f & ENVOPTDLG_F_TOOLBAR_BTT)
		MainWindow_createToolBar(p, FALSE);

	//アイコンサイズ

	if(f & ENVOPTDLG_F_ICONSIZE)
	{
		//ツールバー
		mIconButtonsReplaceImageList(M_ICONBUTTONS(p->toolbar),
			AppResource_loadIconImage("toolbar.png", APP_CONF->iconsize_toolbar));

		//dock
		DockTool_changeIconSize();
		DockLayer_changeIconSize();
		DockCanvasView_changeIconSize();
		DockImageViewer_changeIconSize();

		mWidgetReLayout(M_WIDGET(p));
	}
}

/** パネル配置設定 */

void MainWindow_cmd_panelLayout(MainWindow *p)
{
	if(PanelLayoutDlg_run(M_WINDOW(p)))
	{
		//ペインの再配置
		
		MainWindow_pane_layout(p);

		//ペイン内のパネルを再配置
	
		DockObjects_relocate();

		//レイアウト

		mWidgetReLayout(p->ct_main);
	}
}

/** キャンバスサイズ変更 */

void MainWindow_cmd_resizeCanvas(MainWindow *p)
{
	CanvasResizeInfo info;
	int sw,sh,topx,topy,n;

	sw = APP_DRAW->imgw;
	sh = APP_DRAW->imgh;

	//ダイアログ

	info.w = sw;
	info.h = sh;

	if(!CanvasResizeDlg_run(M_WINDOW(p), &info))
		return;

	//サイズ変更なし

	if(info.w == sw && info.h == sh) return;

	//左上位置

	n = info.align % 3;

	if(n == 0) topx = 0;
	else if(n == 1) topx = (info.w - sw) >> 1;
	else topx = info.w - sw;

	n = info.align / 3;

	if(n == 0) topy = 0;
	else if(n == 1) topy = (info.h - sh) >> 1;
	else topy = info.h - sh;

	//実行

	if(!drawImage_resizeCanvas(APP_DRAW, info.w, info.h, topx, topy, info.crop))
		MainWindow_apperr(APPERR_ALLOC, NULL);

	MainWindow_updateNewCanvas(p, NULL);
}

/** キャンバス拡大縮小 */

void MainWindow_cmd_scaleCanvas(MainWindow *p)
{
	CanvasScaleInfo info;

	//ダイアログ

	info.w = APP_DRAW->imgw;
	info.h = APP_DRAW->imgh;
	info.dpi = APP_DRAW->imgdpi;
	info.have_dpi = TRUE;

	if(!CanvasScaleDlg_run(M_WINDOW(p), &info))
		return;

	//サイズ変更なし

	if(info.w == APP_DRAW->imgw && info.h == APP_DRAW->imgh)
		return;

	//実行

	if(!drawImage_scaleCanvas(APP_DRAW, info.w, info.h, info.dpi, info.type))
		MainWindow_apperr(APPERR_ALLOC, NULL);
	
	MainWindow_updateNewCanvas(p, NULL);
}


//==========================


/** アンドゥ/リドゥ */

void MainWindow_undoredo(MainWindow *p,mBool redo)
{
	UndoUpdateInfo info;
	mBool ret;

	if(!Undo_isHave(redo)) return;

	//実行 (ret は更新を行うかどうか。失敗したかではない)

	MainWinCanvasArea_setCursor_wait();

	ret = Undo_runUndoRedo(redo, &info);

	MainWinCanvasArea_restoreCursor();

	if(!ret) return;

	//更新

	switch(info.type)
	{
		//イメージ範囲 (単体レイヤ)
		case UNDO_UPDATE_RECT:
			drawUpdate_rect_imgcanvas_canvasview_fromRect(APP_DRAW, &info.rc);
			DockLayer_update_layer(info.layer);
			break;
		//イメージ範囲 + レイヤ一覧
		case UNDO_UPDATE_RECT_AND_LAYERLIST:
			drawUpdate_rect_imgcanvas_canvasview_fromRect(APP_DRAW, &info.rc);
			DockLayer_update_all();
			break;
		//イメージ範囲 + レイヤ一覧の指定レイヤのみ
		case UNDO_UPDATE_RECT_AND_LAYERLIST_ONE:
			drawUpdate_rect_imgcanvas_canvasview_fromRect(APP_DRAW, &info.rc);
			DockLayer_update_layer(info.layer);
			break;
		//イメージ範囲 + レイヤ一覧の指定レイヤとその下レイヤ (下レイヤに移す時)
		case UNDO_UPDATE_RECT_AND_LAYERLIST_DOUBLE:
			drawUpdate_rect_imgcanvas_canvasview_fromRect(APP_DRAW, &info.rc);

			DockLayer_update_layer(info.layer);
			DockLayer_update_layer(LayerItem_getNext(info.layer));
			break;
		//レイヤ一覧全体
		case UNDO_UPDATE_LAYERLIST:
			DockLayer_update_all();
			break;
		//レイヤ一覧の指定レイヤのみ
		case UNDO_UPDATE_LAYERLIST_ONE:
			DockLayer_update_layer(info.layer);
			break;
		//すべて + レイヤ一覧
		case UNDO_UPDATE_ALL_AND_LAYERLIST:
			drawUpdate_all_layer();
			break;
		//キャンバスサイズ変更
		case UNDO_UPDATE_CANVAS_RESIZE:
			drawImage_changeImageSize(APP_DRAW, info.rc.x1, info.rc.y1);
			MainWindow_updateNewCanvas(p, NULL);
			break;
	}
}

/** キャンバスキー押し時 */

void MainWindow_onCanvasKeyCommand(int id)
{
	DrawData *p = APP_DRAW;

	if(id == 0) return;

	if(id >= CANVASKEYID_CMD_TOOL && id < CANVASKEYID_CMD_TOOL + TOOL_NUM)
	{
		//ツール変更

		drawTool_setTool(p, id - CANVASKEYID_CMD_TOOL);
	}
	else if(id >= CANVASKEYID_CMD_DRAWTYPE && id < CANVASKEYID_CMD_DRAWTYPE + TOOLSUB_DRAW_NUM)
	{
		//描画タイプ変更 (ブラシツール時)

		if(p->tool.no == TOOL_BRUSH)
			drawTool_setToolSubtype(p, id - CANVASKEYID_CMD_DRAWTYPE);
	}
	else if(id >= CANVASKEYID_CMD_OTHER && id < CANVASKEYID_CMD_OTHER + CANVASKEY_CMD_OTHER_NUM)
	{
		//他コマンド

		id -= CANVASKEYID_CMD_OTHER;

		switch(id)
		{
			//一つ上/下のレイヤを選択
			case 0:
			case 1:
				drawLayer_currentSelUpDown(p, (id == 0));
				break;
			//カレントレイヤ表示/非表示
			case 2:
				drawLayer_revVisible(p, p->curlayer);
				DockLayer_update_curlayer(FALSE);
				break;
			//描画色/背景色切り替え
			case 3:
				drawColor_toggleDrawCol(p);
				DockColor_changeDrawColor();
				break;
			//一段階拡大/縮小
			case 4:
			case 5:
				drawCanvas_zoomStep(APP_DRAW, (id == 4));
				break;
			//キャンバス回転リセット
			case 6:
				drawCanvas_setZoomAndAngle(p, 0, 0, 2, FALSE);
				break;
			//元に戻す/やり直す
			case 7:
			case 8:
				MainWindow_undoredo(APP_WIDGETS->mainwin, (id == 8));
				break;
			//直前のブラシと切り替え
			case 9:
				if(BrushList_toggleLastItem())
					DockBrush_changeBrushSel();
				break;
			//一つ前/次のブラシ選択
			case 10:
			case 11:
				if(BrushList_moveSelect((id == 11)))
					DockBrush_changeBrushSel();
				break;
		}
	}
}


//=================================
// 選択範囲内イメージをPNG出力
//=================================


typedef struct
{
	char *filename;
	mRect rc;
}_thdata_selimgpng;


/** スレッド */

static int _thread_seloutputpng(mPopupProgress *prog,void *data)
{
	_thdata_selimgpng *p = (_thdata_selimgpng *)data;

	return TileImage_savePNG_select_rgba(
		APP_DRAW->curlayer->img, APP_DRAW->tileimgSel,
		p->filename, APP_DRAW->imgdpi, &p->rc, prog);
}

/** 選択範囲内イメージをPNG出力 */

void MainWindow_cmd_selectOutputPNG(MainWindow *p)
{
	mStr str = MSTR_INIT;
	mRect rc;
	_thdata_selimgpng dat;

	//選択範囲なし、またはフォルダレイヤの場合

	if(!drawSel_isHave()
		|| LAYERITEM_IS_FOLDER(APP_DRAW->curlayer))
		return;

	//範囲 (キャンバス範囲外は除く)
	
	TileImage_getSelectHaveRect_real(APP_DRAW->tileimgSel, &rc);

	if(!drawCalc_clipImageRect(APP_DRAW, &rc))
		return;

	//ファイル名取得

	if(mSysDlgSaveFile(M_WINDOW(p),
		"PNG file (*.png)\t*.png",
		0, APP_CONF->strSelectFileDir.buf, 0, &str, NULL))
	{
		//拡張子

		mStrPathSetExt(&str, "png");

		//ディレクトリ記録

		mStrPathGetDir(&APP_CONF->strSelectFileDir, str.buf);

		//保存

		dat.filename = str.buf;
		dat.rc = rc;

		if(PopupThread_run(&dat, _thread_seloutputpng) != 1)
			MainWindow_apperr(APPERR_FAILED, NULL);
	}

	mStrFree(&str);
}
