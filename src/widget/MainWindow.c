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
 * メインウィンドウ
 *****************************************/

#include <stdio.h>

#include "mDef.h"
#include "mGui.h"
#include "mStr.h"
#include "mEvent.h"
#include "mWidget.h"
#include "mContainer.h"
#include "mWindow.h"
#include "mMenuBar.h"
#include "mMenu.h"
#include "mAccelerator.h"
#include "mSplitter.h"
#include "mIconButtons.h"
#include "mMessageBox.h"
#include "mDockWidget.h"
#include "mSysDialog.h"
#include "mUtilFile.h"
#include "mTrans.h"
#include "mAppDef.h"
#include "mIniRead.h"

#include "defMacros.h"
#include "defWidgets.h"
#include "defConfig.h"
#include "defMainWindow.h"

#include "MainWindow.h"
#include "MainWindow_pv.h"
#include "MainWinCanvas.h"
#include "StatusBar.h"
#include "DockObject.h"
#include "Docks_external.h"

#include "LayerItem.h"
#include "Undo.h"
#include "AppResource.h"

#include "defDraw.h"
#include "draw_main.h"
#include "draw_select.h"

#include "trgroup.h"
#include "trid_mainmenu.h"
#include "trid_message.h"

#define MAINMENUDAT_DEFINE
#include "dataMainMenu.h"


//----------------------

#define _APP_VERSION_TEXT  "AzPainter ver 2.1.0\n\nCopyright (C) 2013-2017 Azel"

#define _APP_LICENSE_TEXT \
"AzPainter is free software: you can redistribute it and/or modify\n" \
"it under the terms of the GNU General Public License as published by\n" \
"the Free Software Foundation, either version 3 of the License, or\n" \
"(at your option) any later version.\n\n" \
"AzPainter is distributed in the hope that it will be useful,\n" \
"but WITHOUT ANY WARRANTY; without even the implied warranty of\n" \
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n" \
"GNU General Public License for more details.\n\n" \
"You should have received a copy of the GNU General Public License\n" \
"along with this program.  If not, see <http://www.gnu.org/licenses/>."

//----------------------

static int _event_handle(mWidget *wg,mEvent *ev);

/* dialog */

mBool GridOptionDlg_run(mWindow *owner);
mBool ShortcutKeyDlg_run(mWindow *owner);
mBool CanvasKeyDlg_run(mWindow *owner);

//----------------------



//=========================
// 作成 sub
//=========================


/** メニュー作成 */

static void _create_menu(MainWindow *p)
{
	mMenuBar *bar;
	mMenu *menu;
	int i;
	char m[16];
	uint16_t zoom[] = {25,50,100,200,1000,0};

	bar = mMenuBarNew(0, M_WIDGET(p), 0);

	p->win.menubar = bar;

	//データからメニュー作成

	M_TR_G(TRGROUP_MAINMENU);

	mMenuBarCreateMenuTrArray16(bar, g_mainmenudat, 1000);

	p->menu_main = mMenuBarGetMenu(bar);

	//最近使ったファイル、サブメニュー

	menu = mMenuNew();
	p->menu_recentfile = menu;

	MainWindow_setRecentFileMenu(p);

	mMenuBarSetItemSubmenu(bar, TRMENU_FILE_RECENTFILE, menu);

	//表示倍率

	menu = mMenuGetSubMenu(p->menu_main, TRMENU_VIEW_CANVAS_ZOOM);

	for(i = 0; zoom[i]; i++)
	{
		snprintf(m, 16, "%d%%", zoom[i]);
		mMenuAddText_copy(menu, MAINWINDOW_CMDID_ZOOM_TOP + zoom[i], m);
	}
}

/** ツールバー作成 */

void MainWindow_createToolBar(MainWindow *p,mBool init)
{
	mIconButtons *ib;
	const uint8_t *buf;
	int i,no,id,cmdid;
	uint32_t f;
	uint16_t bttid[] = {
		TRMENU_FILE_NEW, TRMENU_FILE_OPEN|0x8000, TRMENU_FILE_SAVE,
		TRMENU_FILE_SAVE_AS|0x8000, TRMENU_FILE_SAVE_DUP|0x8000,
		TRMENU_VIEW_PALETTE_VISIBLE|0x4000, TRMENU_VIEW_CANVAS_MIRROR|0x4000,
		TRMENU_VIEW_BKGND_PLAID|0x4000, TRMENU_VIEW_GRID|0x4000,
		TRMENU_VIEW_GRID_SPLIT|0x4000,
		TRMENU_VIEW_CANVAS_ZOOM, TRMENU_EDIT_UNDO, TRMENU_EDIT_REDO,
		TRMENU_LAYER_ERASE, TRMENU_SEL_RELEASE,
		TRMENU_FILE_RECENTFILE
	};

	//

	if(!init)
	{
		ib = M_ICONBUTTONS(p->toolbar);

		mIconButtonsDeleteAll(ib);
	}
	else
	{
		//作成
		
		ib = mIconButtonsNew(0, M_WIDGET(p),
			MICONBUTTONS_S_TOOLTIP | MICONBUTTONS_S_DESTROY_IMAGELIST);

		p->toolbar = (mWidget *)ib;

		ib->wg.fLayout = MLF_EXPAND_W;

		mIconButtonsSetImageList(ib, AppResource_loadIconImage("toolbar.png", APP_CONF->iconsize_toolbar));
		mIconButtonsSetTooltipTrGroup(ib, TRGROUP_TOOLBAR_TOOLTIP);

		if(!(APP_CONF->fView & CONFIG_VIEW_F_TOOLBAR))
			mWidgetShow(M_WIDGET(ib), 0);
	}

	//ボタン追加

	buf = APP_CONF->toolbar_btts;
	if(!buf) buf = AppResource_getToolbarBtt_default();

	for(i = 0; buf[i] != 255; i++)
	{
		no = buf[i];
		if(no >= APP_RESOURCE_TOOLBAR_ICON_NUM && no < 254)
			break;
		
		if(no == 254)
		{
			id = 0xffff;
			cmdid = -1;
		}
		else
		{
			id = bttid[no];
			cmdid = id & 0x3fff;
		}

		if(id == 0xffff)
			f = MICONBUTTONS_ITEMF_SEP;
		else if(id & 0x8000)
			f = MICONBUTTONS_ITEMF_DROPDOWN;
		else if(id & 0x4000)
			f = MICONBUTTONS_ITEMF_CHECKBUTTON;
		else
			f = 0;
		
		mIconButtonsAdd(ib, cmdid, no, no, f);
	}

	//チェック

	mIconButtonsSetCheck(ib, TRMENU_VIEW_PALETTE_VISIBLE, APP_CONF->fView & CONFIG_VIEW_F_DOCKS);
	mIconButtonsSetCheck(ib, TRMENU_VIEW_BKGND_PLAID, APP_CONF->fView & CONFIG_VIEW_F_BKGND_PLAID);
	mIconButtonsSetCheck(ib, TRMENU_VIEW_GRID, APP_CONF->fView & CONFIG_VIEW_F_GRID);
	mIconButtonsSetCheck(ib, TRMENU_VIEW_GRID_SPLIT, APP_CONF->fView & CONFIG_VIEW_F_GRID_SPLIT);

	//
	
	mIconButtonsCalcHintSize(ib);
}

/** メイン部分作成 */

static void _create_main(MainWindow *p)
{
	mWidget *ct,*wg;
	int i;

	//水平コンテナ

	p->ct_main = ct = mContainerCreate(M_WIDGET(p), MCONTAINER_TYPE_HORZ, 0, 0, MLF_EXPAND_WH);

	//キャンバス

	MainWinCanvas_new(ct);

	//ペイン

	for(i = 0; i < 3; i++)
	{
		//ペインウィジェット
	
		wg = mContainerCreate(ct, MCONTAINER_TYPE_VERT, 0, 0, MLF_FIX_W | MLF_EXPAND_H);

		APP_WIDGETS->pane[i] = wg;

		wg->w = APP_CONF->pane_width[i];
		wg->draw = mWidgetHandleFunc_draw_drawBkgnd;
		wg->drawBkgnd = mWidgetHandleFunc_drawBkgnd_fillFace;

		/* サイドバーの中身の高さが推奨サイズに適用されないように。
		 * これをしておかないと、ステータスバーが隠れてしまう。 */
		wg->hintOverH = 1;

		//スプリッター

		p->splitter[i] = (mWidget *)mSplitterNew(0, ct, MSPLITTER_S_HORZ);
	}

	//ペインレイアウト

	MainWindow_pane_layout(p);

	//パネル自体が非表示の場合

	if(!(APP_CONF->fView & CONFIG_VIEW_F_DOCKS))
		MainWindow_toggle_show_pane_splitter(p);
}

/** ショートカットキーセット (設定ファイルから読み込み) */

static void _set_shortcutkey(MainWindow *p)
{
	mIniRead *ini;
	int cmdid;
	uint32_t key;

	ini = mIniReadLoadFile2(MAPP->pathConfig, CONFIG_FILENAME_SHORTCUTKEY);
	if(!ini) return;

	if(mIniReadSetGroup(ini, "sckey"))
	{
		//次の項目の番号と数値を取得
	
		while(mIniReadGetNextItem_nonum32(ini, &cmdid, &key, TRUE))
		{
			mAcceleratorAdd(p->win.accelerator, cmdid, key, NULL);

			mMenuSetShortcutKey(p->menu_main, cmdid, key);
		}
	}

	mIniReadEnd(ini);
}


//=========================
// main
//=========================


/** 解放処理 */

static void _destroy_handle(mWidget *wg)
{
	MainWindow *p = (MainWindow *)wg;
	int i;

	mStrFree(&p->strFilename);
	
	mAcceleratorDestroy(p->win.accelerator);

	/* ペインとスプリッターは、パネルが一つもない場合切り離されているので、
	 * 切り離されているものはここで削除する。
	 * (切り離していないものは削除しないこと) */

	for(i = 0; i < 3; i++)
	{
		if(!(APP_WIDGETS->pane[i]->parent))
			mWidgetDestroy(APP_WIDGETS->pane[i]);

		if(!(p->splitter[i]->parent))
			mWidgetDestroy(p->splitter[i]);
	}
}

/** メインウィンドウ作成 */

void MainWindow_new()
{
	MainWindow *p;

	//ウィンドウ
	
	p = (MainWindow *)mWindowNew(sizeof(MainWindow), NULL,
			MWINDOW_S_NORMAL | MWINDOW_S_NO_IM);
	if(!p) return;
	
	APP_WIDGETS->mainwin = p;

	p->wg.destroy = _destroy_handle;
	p->wg.event = _event_handle;
	p->wg.fOption |= MWIDGET_OPTION_NO_DRAW_BKGND;  //背景は描画しない

	//アイコン

	mWindowSetIconFromFile(M_WINDOW(p), "!/icon.png");

	//有効

	mWindowEnablePenTablet(M_WINDOW(p));
	mWindowEnableDND(M_WINDOW(p));

	//メニュー

	_create_menu(p);

	//ツールバー

	MainWindow_createToolBar(p, TRUE);

	//アクセラレータ

	p->win.accelerator = mAcceleratorNew();

	mAcceleratorSetDefaultWidget(p->win.accelerator, M_WIDGET(p));

	//ショートカットキー

	_set_shortcutkey(p);

	//メイン部分

	_create_main(p);

	//ステータスバー

	if(APP_CONF->fView & CONFIG_VIEW_F_STATUSBAR)
		StatusBar_new();
}

/** 初期表示 */

void MainWindow_showStart(MainWindow *p)
{
	mBox box;

	//タイトル

	MainWindow_setTitle(p);

	//ステータスバー情報

	StatusBar_setImageInfo();

	//表示

	box.x = box.y = -1;
	box.w = 750;
	box.h = 500;

	mWindowShowInit(M_WINDOW(p), &APP_CONF->box_mainwin, &box, -100000,
		TRUE, (APP_CONF->mainwin_maximized));
}

/** 終了 */

void MainWindow_quit()
{
	if(MainWindow_confirmSave(APP_WIDGETS->mainwin))
		mAppQuit();
}

/** タイトルバーの文字列セット */

void MainWindow_setTitle(MainWindow *p)
{
	mStr str = MSTR_INIT;

	//文字列

	if(mStrIsEmpty(&p->strFilename))
		mStrSetText(&str, "untitled");
	else
		mStrPathGetFileName(&str, p->strFilename.buf);

	mStrAppendText(&str, " - " APPNAME);

	//

	mWindowSetTitle(M_WINDOW(p), str.buf);

	mStrFree(&str);
}

/** APPERR_* のエラーメッセージ表示
 *
 * @param detail 詳細メッセージ (NULL でなし) */

void MainWindow_apperr(int err,const char *detail)
{
	if(!detail)
		mMessageBoxErrTr(M_WINDOW(APP_WIDGETS->mainwin), TRGROUP_APPERR, err);
	else
	{
		mStr str = MSTR_INIT;

		mStrSetText(&str, M_TR_T2(TRGROUP_APPERR, err));
		mStrAppendText(&str, "\n\n");
		mStrAppendText(&str, detail);

		mMessageBoxErr(M_WINDOW(APP_WIDGETS->mainwin), str.buf);

		mStrFree(&str);
	}
}

/** 現在のイメージの保存確認
 *
 * @return FALSE で処理をキャンセル */

mBool MainWindow_confirmSave(MainWindow *p)
{
	uint32_t ret;

	//変更なし
	
	if(!Undo_isChange()) return TRUE;

	//確認

	ret = mMessageBox(M_WINDOW(p), NULL,
		M_TR_T2(TRGROUP_MESSAGE, TRID_MES_SAVE_CONFIRM),
		MMESBOX_SAVE | MMESBOX_SAVENO | MMESBOX_CANCEL, MMESBOX_CANCEL);

	if(ret == MMESBOX_CANCEL)
		return FALSE;
	else if(ret == MMESBOX_SAVE)
	{
		if(!MainWindow_saveFile(p, MAINWINDOW_SAVEFILE_OVERWRITE, 0))
			return FALSE;
	}

	return TRUE;
}

/** イメージサイズが変更された後の更新
 *
 * @param filename 読み込んだファイル名。NULL でサイズのみ変更。空文字列で新規作成。 */

void MainWindow_updateNewCanvas(MainWindow *p,const char *filename)
{
	//ファイル変更時

	if(filename)
	{
		mStrSetText(&p->strFilename, filename);

		p->saved = FALSE;
	}

	//タイトル

	MainWindow_setTitle(p);

	//各変更

	drawImage_afterChange(APP_DRAW, (filename != NULL));

	StatusBar_setImageInfo();
}

/** プログレスバーを表示する位置 (ルート座標) を取得 */

void MainWindow_getProgressBarPos(mPoint *pt)
{
	mBox box;

	if(!StatusBar_getProgressBarPos(pt))
	{
		//ステータスバーが非表示ならメインウィンドウの左下

		mWidgetGetRootBox(M_WIDGET(APP_WIDGETS->mainwin), &box);

		pt->x = box.x;
		pt->y = box.y + box.h;
	}
}


//============================
// コマンド
//============================


/** ツールバーのチェックボタンを反転 */

static void _toolbar_button_toggle(MainWindow *p,int id,mEvent *ev)
{
	if(ev->cmd.by != MEVENT_COMMAND_BY_ICONBUTTONS_BUTTON)
		mIconButtonsSetCheck(M_ICONBUTTONS(p->toolbar), id, -1);
}

/** ツールバーボタンからの表示倍率メニュー */

static void _runmenu_canvas_zoom(MainWindow *p)
{
	mMenu *menu;
	mMenuItemInfo *mi;
	int i;
	mBox box;
	char m[16];

	menu = mMenuNew();

	for(i = 25; i <= 1000; )
	{
		snprintf(m, 16, "%d%%", i);

		mMenuAddText_copy(menu, i, m);
	
		i += (i < 100)? 25: 100;
	}

	mIconButtonsGetItemBox(M_ICONBUTTONS(p->toolbar), TRMENU_VIEW_CANVAS_ZOOM, &box, TRUE);

	mi = mMenuPopup(menu, NULL, box.x, box.y + box.h, 0);
	i = (mi)? mi->id: -1;

	mMenuDestroy(menu);

	//

	if(i != -1)
		drawCanvas_setZoomAndAngle(APP_DRAW, i * 10, 0, 1, TRUE);
}

/** 履歴からの開く時 */

static void _open_recentfile(MainWindow *p,int no)
{
	if(!mIsFileExist(APP_CONF->strRecentFile[no].buf, FALSE))
	{
		//ファイルが存在しない

		mMessageBox(M_WINDOW(p), NULL,
			M_TR_T2(TRGROUP_MESSAGE, TRID_MES_RECENTFILE_NOTEXIST),
			MMESBOX_OK, MMESBOX_OK);

		mStrArrayShiftUp(APP_CONF->strRecentFile, no, CONFIG_RECENTFILE_NUM - 1);

		MainWindow_setRecentFileMenu(p);
	}
	else
	{
		//開く
		
		if(MainWindow_confirmSave(p))
			MainWindow_loadImage(p, APP_CONF->strRecentFile[no].buf, 0);
	}
}


//============================
// ハンドラ
//============================


/** コマンド */

static void _event_command(MainWindow *p,mEvent *ev)
{
	int id = ev->cmd.id,n;

	//レイヤ関連

	if(id >= TRMENU_LAYER_TOP && id <= TRMENU_LAYER_BOTTOM)
	{
		MainWindow_layercmd(p, id);
		return;
	}

	//フィルタ関連

	if(id >= TRMENU_FILTER_TOP && id <= TRMENU_FILTER_BOTTOM)
	{
		MainWindow_filtercmd(id);
		return;
	}

	//ファイル履歴

	if(id >= MAINWINDOW_CMDID_RECENTFILE && id < MAINWINDOW_CMDID_RECENTFILE + CONFIG_RECENTFILE_NUM)
	{
		_open_recentfile(p, id - MAINWINDOW_CMDID_RECENTFILE);
		return;
	}

	//各パネル表示

	if(id >= TRMENU_VIEW_PALETTE_TOOL && id < TRMENU_VIEW_PALETTE_TOOL + DOCKWIDGET_NUM)
	{
		MainWindow_cmd_show_panel_toggle(id - TRMENU_VIEW_PALETTE_TOOL);
		return;
	}

	//表示倍率

	if(id >= MAINWINDOW_CMDID_ZOOM_TOP && id < MAINWINDOW_CMDID_ZOOM_TOP + 2000)
	{
		drawCanvas_setZoomAndAngle(APP_DRAW,
			(id - MAINWINDOW_CMDID_ZOOM_TOP) * 10, 0, 1, TRUE);
		return;
	}

	//

	switch(id)
	{
		//ツールバー:倍率指定
		case TRMENU_VIEW_CANVAS_ZOOM:
			_runmenu_canvas_zoom(p);
			break;
		//ツールバー:ファイル履歴
		case TRMENU_FILE_RECENTFILE:
			MainWindow_runMenu_toolbar_recentfile(p);
			break;
	
		//---- ファイル

		//新規作成
		case TRMENU_FILE_NEW:
			MainWindow_newImage(p);
			break;
		//開く
		case TRMENU_FILE_OPEN:
			if(ev->cmd.by == MEVENT_COMMAND_BY_ICONBUTTONS_DROP)
			{
				n = MainWindow_runMenu_toolbarDrop_opensave(p, FALSE);
				if(n != -1)
					MainWindow_openFile(p, n);
			}
			else
				MainWindow_openFile(p, 0);
			break;
		//上書き保存
		case TRMENU_FILE_SAVE:
			MainWindow_saveFile(p, MAINWINDOW_SAVEFILE_OVERWRITE, 0);
			break;
		//別名で保存
		case TRMENU_FILE_SAVE_AS:
			if(ev->cmd.by == MEVENT_COMMAND_BY_ICONBUTTONS_DROP)
			{
				n = MainWindow_runMenu_toolbarDrop_opensave(p, TRUE);
				if(n != -1)
					MainWindow_saveFile(p, MAINWINDOW_SAVEFILE_RENAME, n);
			}
			else
				MainWindow_saveFile(p, MAINWINDOW_SAVEFILE_RENAME, 0);
			break;
		//複製を保存
		case TRMENU_FILE_SAVE_DUP:
			if(ev->cmd.by == MEVENT_COMMAND_BY_ICONBUTTONS_DROP)
			{
				n = MainWindow_runMenu_toolbarDrop_savedup(p);
				if(n != -1)
					MainWindow_saveFile(p, MAINWINDOW_SAVEFILE_DUP, n);
			}
			else
				MainWindow_saveFile(p, MAINWINDOW_SAVEFILE_DUP, 0);
			break;
		//履歴消去
		case TRMENU_FILE_RECENTFILE_CLEAR:
			mStrArrayFree(APP_CONF->strRecentFile, CONFIG_RECENTFILE_NUM);
			MainWindow_setRecentFileMenu(p);
			break;
		//終了
		case TRMENU_FILE_EXIT:
			MainWindow_quit();
			break;

		//---- 編集

		//元に戻す/やり直す
		case TRMENU_EDIT_UNDO:
		case TRMENU_EDIT_REDO:
			MainWindow_undoredo(p, (id == TRMENU_EDIT_REDO));
			break;
		//マスク類の状態をチェック
		case TRMENU_EDIT_CHECK_MASKS:
			MainWindow_cmd_checkMaskState(p);
			break;
		//塗りつぶし/消去
		case TRMENU_EDIT_FILL:
		case TRMENU_EDIT_ERASE:
			drawSel_fill_erase(APP_DRAW, (id == TRMENU_EDIT_ERASE));
			break;
		//キャンバスサイズ変更
		case TRMENU_EDIT_RESIZE_CANVAS:
			MainWindow_cmd_resizeCanvas(p);
			break;
		//キャンバス拡大縮小
		case TRMENU_EDIT_SCALE_CANVAS:
			MainWindow_cmd_scaleCanvas(p);
			break;
		//DPI値変更
		case TRMENU_EDIT_CHANGE_DPI:
			MainWindow_cmd_changeDPI(p);
			break;

		//---- 選択範囲

		//解除
		case TRMENU_SEL_RELEASE:
			drawSel_release(APP_DRAW, TRUE);
			break;
		//反転
		case TRMENU_SEL_INVERSE:
			drawSel_inverse(APP_DRAW);
			break;
		//すべて選択
		case TRMENU_SEL_ALL:
			drawSel_all(APP_DRAW);
			break;
		//コピー
		case TRMENU_SEL_COPY:
			drawSel_copy_cut(APP_DRAW, FALSE);
			break;
		//切り取り
		case TRMENU_SEL_CUT:
			drawSel_copy_cut(APP_DRAW, TRUE);
			break;
		//貼り付け
		case TRMENU_SEL_PASTE_NEWLAYER:
			drawSel_paste_newlayer(APP_DRAW);
			break;
		//範囲の拡張/縮小
		case TRMENU_SEL_EXPAND:
			MainWindow_cmd_selectExpand(p);
			break;
		//PNG出力
		case TRMENU_SEL_OUTPUT_PNG:
			MainWindow_cmd_selectOutputPNG(p);
			break;
		
		//---- 表示

		//パレット表示
		case TRMENU_VIEW_PALETTE_VISIBLE:
			MainWindow_cmd_show_panel_all(p);
			_toolbar_button_toggle(p, id, ev);
			break;
		//パレット:すべてウィンドウモードに
		case TRMENU_VIEW_PALETTE_ALL_WINDOW:
			if(APP_CONF->fView & CONFIG_VIEW_F_DOCKS)
				DockObjects_all_windowMode(1);
			break;
		//パレット:すべて格納
		case TRMENU_VIEW_PALETTE_ALL_DOCK:
			if(APP_CONF->fView & CONFIG_VIEW_F_DOCKS)
				DockObjects_all_windowMode(0);
			break;
		//キャンバス左右反転表示
		case TRMENU_VIEW_CANVAS_MIRROR:
			_toolbar_button_toggle(p, id, ev);
			drawCanvas_mirror(APP_DRAW);
			break;
		//背景をチェック柄で表示
		case TRMENU_VIEW_BKGND_PLAID:
			APP_CONF->fView ^= CONFIG_VIEW_F_BKGND_PLAID;

			_toolbar_button_toggle(p, id, ev);
			drawUpdate_all();
			break;
		//グリッド
		case TRMENU_VIEW_GRID:
			APP_CONF->fView ^= CONFIG_VIEW_F_GRID;

			_toolbar_button_toggle(p, id, ev);
			drawUpdate_canvasArea();
			break;
		//分割線
		case TRMENU_VIEW_GRID_SPLIT:
			APP_CONF->fView ^= CONFIG_VIEW_F_GRID_SPLIT;

			_toolbar_button_toggle(p, id, ev);
			drawUpdate_canvasArea();
			break;
		//ツールバー
		case TRMENU_VIEW_TOOLBAR:
			APP_CONF->fView ^= CONFIG_VIEW_F_TOOLBAR;

			mWidgetShow(p->toolbar, -1);
			mWidgetReLayout(M_WIDGET(p));
			break;
		//ステータスバー
		case TRMENU_VIEW_STATUSBAR:
			StatusBar_toggleVisible();
			break;

		//表示倍率拡大
		case TRMENU_VIEW_CANVASZOOM_UP:
			drawCanvas_zoomStep(APP_DRAW, TRUE);
			break;
		//表示倍率縮小
		case TRMENU_VIEW_CANVASZOOM_DOWN:
			drawCanvas_zoomStep(APP_DRAW, FALSE);
			break;
		//ウィンドウに合わせる
		case TRMENU_VIEW_CANVASZOOM_FIT:
			drawCanvas_fitWindow(APP_DRAW);
			break;

		//左回転
		case TRMENU_VIEW_CANVASROTATE_LEFT:
			drawCanvas_rotateStep(APP_DRAW, TRUE);
			break;
		//右回転
		case TRMENU_VIEW_CANVASROTATE_RIGHT:
			drawCanvas_rotateStep(APP_DRAW, FALSE);
			break;
		//角度指定
		case TRMENU_VIEW_CANVASROTATE_0:
		case TRMENU_VIEW_CANVASROTATE_90:
		case TRMENU_VIEW_CANVASROTATE_180:
		case TRMENU_VIEW_CANVASROTATE_270:
			drawCanvas_setZoomAndAngle(APP_DRAW, 0,
				(id - TRMENU_VIEW_CANVASROTATE_0) * 9000, 2, TRUE);
			break;

		//---- 設定

		//環境設定
		case TRMENU_OPT_ENV:
			MainWindow_cmd_envoption(p);
			break;
		//グリッド設定
		case TRMENU_OPT_GRID:
			if(GridOptionDlg_run(M_WINDOW(p)))
				drawUpdate_canvasArea();
			break;
		//ショートカットキー設定
		case TRMENU_OPT_SHORTCUTKEY:
			ShortcutKeyDlg_run(M_WINDOW(p));
			break;
		//キャンバスキー設定
		case TRMENU_OPT_CANVASKEY:
			CanvasKeyDlg_run(M_WINDOW(p));
			break;
		//パネル配置設定
		case TRMENU_OPT_DOCK_ARRANGE:
			MainWindow_cmd_panelLayout(p);
			break;
		//バージョン情報
		case TRMENU_OPT_VERSION:
			mSysDlgAbout_license(M_WINDOW(p), _APP_VERSION_TEXT, _APP_LICENSE_TEXT);
			break;
	}
}

/** レイヤのメニュー有効/無効 */

static void _menupopup_layer(mMenu *menu)
{
	LayerItem *li = APP_DRAW->curlayer;
	int f;
	uint16_t *pid,id[] = {
		TRMENU_LAYER_COPY, TRMENU_LAYER_ERASE, TRMENU_LAYER_SETTYPE,
		TRMENU_LAYER_DROP, TRMENU_LAYER_COMBINE, TRMENU_LAYER_OUTPUT,
		0xffff
	};

	mMenuSetEnable(menu, TRMENU_LAYER_SETCOLOR, LayerItem_isType_layercolor(li));

	//フォルダ時無効

	f = LAYERITEM_IS_IMAGE(li);

	for(pid = id; *pid != 0xffff; pid++)
		mMenuSetEnable(menu, *pid, f);
}

/** メニューポップアップ */

static void _event_menupopup(MainWindow *p,mMenu *menu,int itemid)
{
	int i,f;

	switch(itemid)
	{
		//レイヤ
		case TRMENU_TOP_LAYER:
			_menupopup_layer(menu);
			break;

		//選択範囲
		case TRMENU_TOP_SELECT:
			f = drawSel_isHave();
			
			mMenuSetEnable(menu, TRMENU_SEL_RELEASE, f);
			mMenuSetEnable(menu, TRMENU_SEL_EXPAND, f);

			f = (f && APP_DRAW->curlayer->img);

			mMenuSetEnable(menu, TRMENU_SEL_COPY, f);
			mMenuSetEnable(menu, TRMENU_SEL_CUT, f);
			break;
	
		//表示
		case TRMENU_TOP_VIEW:
			mMenuSetCheck(menu, TRMENU_VIEW_CANVAS_MIRROR, APP_DRAW->canvas_mirror);
			mMenuSetCheck(menu, TRMENU_VIEW_BKGND_PLAID, APP_CONF->fView & CONFIG_VIEW_F_BKGND_PLAID);
			mMenuSetCheck(menu, TRMENU_VIEW_GRID, APP_CONF->fView & CONFIG_VIEW_F_GRID);
			mMenuSetCheck(menu, TRMENU_VIEW_GRID_SPLIT, APP_CONF->fView & CONFIG_VIEW_F_GRID_SPLIT);
			mMenuSetCheck(menu, TRMENU_VIEW_TOOLBAR, APP_CONF->fView & CONFIG_VIEW_F_TOOLBAR);
			mMenuSetCheck(menu, TRMENU_VIEW_STATUSBAR, APP_CONF->fView & CONFIG_VIEW_F_STATUSBAR);
			break;

		//表示 >> パレット
		case TRMENU_VIEW_PALETTE:
			f = ((APP_CONF->fView & CONFIG_VIEW_F_DOCKS) != 0);

			mMenuSetCheck(menu, TRMENU_VIEW_PALETTE_VISIBLE, f);

			mMenuSetEnable(menu, TRMENU_VIEW_PALETTE_ALL_WINDOW, f);
			mMenuSetEnable(menu, TRMENU_VIEW_PALETTE_ALL_DOCK, f);
		
			for(i = 0; i < DOCKWIDGET_NUM; i++)
			{
				mMenuSetCheck(menu, TRMENU_VIEW_PALETTE_TOOL + i,
					DockObject_canDoWidget(APP_WIDGETS->dockobj[i]));

				//パレット自体を非表示の場合は無効
				mMenuSetEnable(menu, TRMENU_VIEW_PALETTE_TOOL + i, f);
			}
			break;
	}
}

/** イベントハンドラ */

int _event_handle(mWidget *wg,mEvent *ev)
{
	MainWindow *p = (MainWindow *)wg;

	switch(ev->type)
	{
		//コマンド
		case MEVENT_COMMAND:
			_event_command(p, ev);
			break;

		//メニューポップアップ
		case MEVENT_MENU_POPUP:
			if(ev->popup.bMenuBar)
				_event_menupopup(p, ev->popup.menu, ev->popup.itemID);
			break;
	
		//閉じるボタン
		case MEVENT_CLOSE:
			MainWindow_quit(p);
			break;
		
		default:
			return FALSE;
	}

	return TRUE;
}
