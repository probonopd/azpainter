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

/********************************************
 * MainWinCanvas/MainWinCanvasArea
 *
 * メインウィンドウのキャンバス部分ウィジェット
 ********************************************/

#include "mDef.h"
#include "mEvent.h"
#include "mWidget.h"
#include "mScrollBar.h"
#include "mKeyDef.h"
#include "mRectBox.h"

#include "defWidgets.h"
#include "defDraw.h"
#include "defConfig.h"
#include "defMainWinCanvas.h"

#include "MainWinCanvas.h"
#include "MainWindow.h"

#include "draw_main.h"
#include "draw_calc.h"
#include "draw_op_main.h"
#include "draw_boxedit.h"
#include "draw_text.h"
#include "draw_op_def.h"

#include "AppCursor.h"


//----------------

/* FilterDlg.c */
void FilterDlg_onCanvasClick(mWidget *wg,int x,int y);

//----------------


/***************************************
 * [MainWinCanvasArea] 領域部分
 ***************************************/


enum
{
	_TIMERID_UPDATE_AREA = 1,
	_TIMERID_UPDATE_RECT,
	_TIMERID_UPDATE_MOVE,
	_TIMERID_UPDATE_MOVE_SELECT,
	_TIMERID_UPDATE_MOVE_SELECT_IMAGE,
	_TIMERID_SCROLL
};


/** キー押し時 */

static void _area_event_keydown(MainWinCanvasArea *p,mEvent *ev)
{
	//スペースキー
	
	if(ev->key.code == MKEY_SPACE)
		p->bPressSpace = TRUE;

	//現在押されているキーを記録 (キャンバスキー用)

	if(ev->key.rawcode < 256)
		p->press_rawkey = ev->key.rawcode;

	//

	if(APP_DRAW->w.optype)
	{
		//操作中時、各ツールに対応した処理

		if(drawOp_onKeyDown(APP_DRAW, ev->key.code))
			mWidgetUngrabPointer(M_WIDGET(p));
	}
	else if(!ev->key.bGrab && ev->key.rawcode < 256)
	{
		//キャンバスキーのコマンド実行 (非操作時)

		MainWindow_onCanvasKeyCommand(APP_CONF->canvaskey[ev->key.rawcode]);
	}
}

/** 描画 */

static void _area_draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	drawUpdate_drawCanvas(APP_DRAW, pixbuf, NULL);
}

/** サイズ変更時 */

static void _area_onsize_handle(mWidget *wg)
{
	drawCanvas_setAreaSize(APP_DRAW, wg->w, wg->h);
}

/** ポインタをグラブする */

static void _area_grab(mWidget *wg,int deviceid)
{
	mCursor cursor;

	//自由線描画時は、押し時に使われたデバイスの情報のみ有効とする

	if(APP_DRAW->w.optype != DRAW_OPTYPE_DRAW_FREE)
		deviceid = 0;

	//ドラッグ中のカーソル

	if(APP_DRAW->w.drag_cursor_type == -1)
		cursor = 0;
	else
		cursor = AppCursor_getForDrag(APP_DRAW->w.drag_cursor_type);

	//グラブ
	
	mWidgetGrabPointer_device_cursor(wg, deviceid, cursor);
}

/** タイマー処理 */

static void _area_event_timer(MainWinCanvasArea *p,int id)
{
	if(id == _TIMERID_SCROLL)
	{
		//スクロール

		APP_DRAW->ptScroll.x += p->ptTimerScrollDir.x;
		APP_DRAW->ptScroll.y += p->ptTimerScrollDir.y;

		p->ptTimerScrollSum.x += p->ptTimerScrollDir.x;
		p->ptTimerScrollSum.y += p->ptTimerScrollDir.y;

		MainWinCanvas_setScrollPos();
		drawUpdate_canvasArea();
	}
	else
	{
		//------- 更新
		
		//タイマー削除
		
		mWidgetTimerDelete(M_WIDGET(p), id);

		//

		switch(id)
		{
			//範囲更新
			case _TIMERID_UPDATE_RECT:
				drawUpdate_rect_imgcanvas(APP_DRAW, &p->boxUpdate);

				p->boxUpdate.x = -1;
				break;

			//領域更新
			case _TIMERID_UPDATE_AREA:
				mWidgetUpdate(M_WIDGET(p));
				break;

			//イメージ移動
			/* DrawData::w.rcdraw の範囲を更新して、範囲をクリア */
			case _TIMERID_UPDATE_MOVE:
				drawUpdate_rect_imgcanvas_fromRect(APP_DRAW, &APP_DRAW->w.rcdraw);
				mRectEmpty(&APP_DRAW->w.rcdraw);
				break;

			//選択範囲移動
			case _TIMERID_UPDATE_MOVE_SELECT:
				drawUpdate_rect_canvas_forSelect(APP_DRAW, &APP_DRAW->w.rcdraw);
				mRectEmpty(&APP_DRAW->w.rcdraw);
				break;

			//選択範囲イメージ移動
			case _TIMERID_UPDATE_MOVE_SELECT_IMAGE:
				drawUpdate_rect_imgcanvas_forSelect(APP_DRAW, &APP_DRAW->w.rcdraw);
				mRectEmpty(&APP_DRAW->w.rcdraw);
				break;
		}
	}
}

/** イベント */

static int _area_event_handle(mWidget *wg,mEvent *ev)
{
	MainWinCanvasArea *p = (MainWinCanvasArea *)wg;

	switch(ev->type)
	{
		//デバイス
		case MEVENT_PENTABLET:
			switch(ev->pen.type)
			{
				//移動
				case MEVENT_POINTER_TYPE_MOTION:
					drawOp_onMotion(APP_DRAW, ev);
					break;
				//ダブルクリック
				case MEVENT_POINTER_TYPE_DBLCLK:
					/* 左ボタン時、drawOp_onLBttDblClk() の戻り値が
					 *  TRUE  => ダブルクリックとして処理してグラブ解除
					 *  FALSE => クリック扱いにするため PRESS へ続く */
					if(ev->pen.btt == M_BTT_LEFT
						&& drawOp_onLBttDblClk(APP_DRAW, ev))
					{
						mWidgetUngrabPointer(wg);
						break;
					}
				//押し
				case MEVENT_POINTER_TYPE_PRESS:
					if(drawOp_onPress(APP_DRAW, ev))
						_area_grab(wg, ev->pen.device_id);
					break;
				//離し
				case MEVENT_POINTER_TYPE_RELEASE:
					if(drawOp_onRelease(APP_DRAW, ev))
						mWidgetUngrabPointer(wg);
					break;
			}
			break;

		//デバイス (ダイアログ表示中)
		case MEVENT_PENTABLET_MODAL:
			//左クリック
			if(ev->pen.type == MEVENT_POINTER_TYPE_PRESS && ev->pen.btt == M_BTT_LEFT)
			{
				if(APP_DRAW->drawtext.in_dialog)
					//テキストダイアログ
					drawText_setDrawPoint_inDialog(APP_DRAW, ev->pen.x, ev->pen.y);

				else if(APP_DRAW->w.in_filter_dialog)
					//フィルタダイアログ
					FilterDlg_onCanvasClick(M_WIDGET(APP_DRAW->w.ptmp), ev->pen.x, ev->pen.y);
			}
			break;

		//タイマー
		case MEVENT_TIMER:
			_area_event_timer(p, ev->timer.id);
			break;

		//キー押し
		case MEVENT_KEYDOWN:
			_area_event_keydown(p, ev);
			break;
		//キー離し
		case MEVENT_KEYUP:
			if(ev->key.code == MKEY_SPACE)
				p->bPressSpace = FALSE;

			if(ev->key.rawcode == p->press_rawkey)
				p->press_rawkey = -1;
			break;

		//フォーカスが別のウィンドウに移った or 予期せぬグラブ解除
		case MEVENT_FOCUS:
			if(ev->focus.bOut)
			{
				drawOp_onUngrab(APP_DRAW);

				p->bPressSpace = FALSE;
				p->press_rawkey = -1;
			}
			break;
		
		//カーソルが離れた
		/* 矩形範囲ツールでリサイズカーソルになっている場合、元に戻す */
		case MEVENT_LEAVE:
			drawBoxEdit_restoreCursor(APP_DRAW);
			break;
	}

	return 1;
}

/** D&D */

static int _area_ondnd_handle(mWidget *wg,char **files)
{
	if(MainWindow_confirmSave(APP_WIDGETS->mainwin))
		MainWindow_loadImage(APP_WIDGETS->mainwin, *files, 0);

	return 1;
}

/** MainWinCanvasArea 作成 */

static MainWinCanvasArea *_create_area(mWidget *parent)
{
	MainWinCanvasArea *p;

	p = (MainWinCanvasArea *)mScrollViewAreaNew(sizeof(MainWinCanvasArea), parent);
	if(!p) return NULL;

	APP_WIDGETS->canvas_area = p;

	p->wg.draw = _area_draw_handle;
	p->wg.event = _area_event_handle;
	p->wg.onSize = _area_onsize_handle;
	p->wg.onDND = _area_ondnd_handle;
	
	p->wg.fEventFilter |= MWIDGET_EVENTFILTER_PENTABLET | MWIDGET_EVENTFILTER_KEY;
	p->wg.fState |= MWIDGET_STATE_TAKE_FOCUS | MWIDGET_STATE_ENABLE_DROP;
	p->wg.fAcceptKeys = MWIDGET_ACCEPTKEY_ENTER | MWIDGET_ACCEPTKEY_ESCAPE;
	p->wg.fOption |= MWIDGET_OPTION_DISABLE_SCROLL_EVENT;

	p->boxUpdate.x = -1;
	p->press_rawkey = -1;

	//カーソル

	MainWinCanvasArea_setCursor_forTool();

	return p;
}

/** スペースキーが押されているか */

mBool MainWinCanvasArea_isPressKey_space()
{
	return (APP_WIDGETS->canvas_area)->bPressSpace;
}

/** 現在押されているキーを取得
 *
 * @return -1 でなし */

int MainWinCanvasArea_getPressRawKey()
{
	return (APP_WIDGETS->canvas_area)->press_rawkey;
}


//============================
// タイマーセット/クリア
//============================


/** タイマーセット:領域更新 */

void MainWinCanvasArea_setTimer_updateArea(int time)
{
	mWidgetTimerAdd_unexist(M_WIDGET(APP_WIDGETS->canvas_area), _TIMERID_UPDATE_AREA, time, 0);
}

/** タイマークリア:領域更新 */

void MainWinCanvasArea_clearTimer_updateArea()
{
	mWidgetTimerDelete(M_WIDGET(APP_WIDGETS->canvas_area), _TIMERID_UPDATE_AREA);
}

/** タイマーセット:範囲更新 (描画更新用) */

void MainWinCanvasArea_setTimer_updateRect(mBox *boximg)
{
	MainWinCanvasArea *p = APP_WIDGETS->canvas_area;

	//タイマーが存在する場合はそのまま

	if(drawCalc_unionImageBox(&p->boxUpdate, boximg))
		mWidgetTimerAdd_unexist(M_WIDGET(p), _TIMERID_UPDATE_RECT, 2, 0);
}

/** 範囲更新タイマーをクリア
 *
 * @param update  残っている範囲を更新するか */

void MainWinCanvasArea_clearTimer_updateRect(mBool update)
{
	MainWinCanvasArea *p = APP_WIDGETS->canvas_area;

	if(mWidgetTimerDelete(M_WIDGET(p), _TIMERID_UPDATE_RECT))
	{
		if(update)
			drawUpdate_rect_imgcanvas(APP_DRAW, &p->boxUpdate);
	}

	p->boxUpdate.x = -1;
}

/** イメージの移動処理の更新タイマーをセット */

void MainWinCanvasArea_setTimer_updateMove()
{
	mWidgetTimerAdd_unexist(M_WIDGET(APP_WIDGETS->canvas_area), _TIMERID_UPDATE_MOVE, 5, 0);
}

/** イメージの移動処理の更新タイマーをクリア */

void MainWinCanvasArea_clearTimer_updateMove()
{
	mWidgetTimerDelete(M_WIDGET(APP_WIDGETS->canvas_area), _TIMERID_UPDATE_MOVE);
}

/** 選択範囲移動のタイマーセット */

void MainWinCanvasArea_setTimer_updateSelectMove()
{
	mWidgetTimerAdd_unexist(M_WIDGET(APP_WIDGETS->canvas_area),
		_TIMERID_UPDATE_MOVE_SELECT, 5, 0);
}

/** 選択範囲移動のタイマークリア */

void MainWinCanvasArea_clearTimer_updateSelectMove()
{
	if(mWidgetTimerDelete(M_WIDGET(APP_WIDGETS->canvas_area), _TIMERID_UPDATE_MOVE_SELECT))
		drawUpdate_rect_canvas_forSelect(APP_DRAW, &APP_DRAW->w.rcdraw);
}

/** 選択範囲イメージ移動のタイマーセット */

void MainWinCanvasArea_setTimer_updateSelectImageMove()
{
	mWidgetTimerAdd_unexist(M_WIDGET(APP_WIDGETS->canvas_area),
		_TIMERID_UPDATE_MOVE_SELECT_IMAGE, 5, 0);
}

/** 選択範囲イメージ移動のタイマークリア */

mBool MainWinCanvasArea_clearTimer_updateSelectImageMove()
{
	if(mWidgetTimerDelete(M_WIDGET(APP_WIDGETS->canvas_area), _TIMERID_UPDATE_MOVE_SELECT_IMAGE))
	{
		drawUpdate_rect_imgcanvas_forSelect(APP_DRAW, &APP_DRAW->w.rcdraw);
		return TRUE;
	}

	return FALSE;
}

/** タイマーセット:スクロール (スプラインのキャンバス範囲外スクロール) */

void MainWinCanvasArea_setTimer_scroll(mPoint *ptdir)
{
	MainWinCanvasArea *p = APP_WIDGETS->canvas_area;

	p->ptTimerScrollDir = *ptdir;
	p->ptTimerScrollSum.x = p->ptTimerScrollSum.y = 0;

	mWidgetTimerAdd(M_WIDGET(p), _TIMERID_SCROLL, 50, 0); 
}

/** タイマークリア:スクロール
 *
 * @param ptsum 総移動数が入る */

void MainWinCanvasArea_clearTimer_scroll(mPoint *ptsum)
{
	MainWinCanvasArea *p = APP_WIDGETS->canvas_area;

	mWidgetTimerDelete(M_WIDGET(p), _TIMERID_SCROLL);

	*ptsum = p->ptTimerScrollSum;
}


//=========================
// カーソル
//=========================


/** 環境設定で描画カーソルが変更された時 */

void MainWinCanvasArea_changeDrawCursor()
{
	mWidget *wg = M_WIDGET(APP_WIDGETS->canvas_area);

	mWidgetSetCursor(wg, 0);

	AppCursor_setDrawCursor(APP_CONF->cursor_buf);

	MainWinCanvasArea_setCursor_forTool();
}

/** ツールのカーソル形状セット */

void MainWinCanvasArea_setCursor_forTool()
{
	int no = drawCursor_getToolCursor(APP_DRAW->tool.no);

	mWidgetSetCursor(M_WIDGET(APP_WIDGETS->canvas_area), AppCursor_getForCanvas(no));
}

/** 指定カーソルセット */

void MainWinCanvasArea_setCursor(int no)
{
	mWidgetSetCursor(M_WIDGET(APP_WIDGETS->canvas_area), AppCursor_getForCanvas(no));
}

/** カーソルを砂時計にセット */

void MainWinCanvasArea_setCursor_wait()
{
	MainWinCanvasArea *p = APP_WIDGETS->canvas_area;

	p->cursor_restore = p->wg.cursorCur;
	
	mWidgetSetCursor(M_WIDGET(p), AppCursor_getWaitCursor());
}

/** カーソルを(砂時計から)元に戻す */

void MainWinCanvasArea_restoreCursor()
{
	MainWinCanvasArea *p = APP_WIDGETS->canvas_area;

	mWidgetSetCursor(M_WIDGET(p), p->cursor_restore);
}



/*********************************
 * [MainWinCanvas]
 *********************************/


//========================
// sub
//========================


/** スクロール処理 */

static void _view_scroll(MainWinCanvas *p,int pos,int flags,mBool vert)
{
	if(flags & MSCROLLBAR_N_HANDLE_F_PRESS)
		//ドラッグ開始 (ドラッグ中は低品質表示)
		drawCanvas_lowQuality();
	else if(flags & MSCROLLBAR_N_HANDLE_F_RELEASE)
	{
		//ドラッグ終了
		drawCanvas_normalQuality();
		drawUpdate_canvasArea();
	}

	//位置変更

	if(flags & MSCROLLBAR_N_HANDLE_F_CHANGE)
	{
		if(vert)
			APP_DRAW->ptScroll.y = pos - p->ptScrollBase.y;
		else
			APP_DRAW->ptScroll.x = pos - p->ptScrollBase.x;

		drawUpdate_canvasArea();
	}
}

/** イベントハンドラ */

static int _view_event_handle(mWidget *wg,mEvent *ev)
{
	MainWinCanvas *p = (MainWinCanvas *)wg;

	switch(ev->type)
	{
		case MEVENT_NOTIFY:
			if(ev->notify.widgetFrom == M_WIDGET(p->sv.scrh))
			{
				//水平スクロール

				if(ev->notify.type == MSCROLLBAR_N_HANDLE)
					_view_scroll(p, ev->notify.param1, ev->notify.param2, FALSE);
			}
			else if(ev->notify.widgetFrom == M_WIDGET(p->sv.scrv))
			{
				//垂直スクロール

				if(ev->notify.type == MSCROLLBAR_N_HANDLE)
					_view_scroll(p, ev->notify.param1, ev->notify.param2, TRUE);
			}
			break;
	}

	return 1;
}


//========================
// main
//========================


/** メインウィンドウキャンバス作成 */

void MainWinCanvas_new(mWidget *parent)
{
	MainWinCanvas *p;

	p = (MainWinCanvas *)mScrollViewNew(sizeof(MainWinCanvas), parent,
			MSCROLLVIEW_S_FIX_HORZVERT | MSCROLLVIEW_S_FRAME | MSCROLLVIEW_S_SCROLL_NOTIFY_SELF);
	if(!p) return;

	APP_WIDGETS->canvas = p;

	p->wg.event = _view_event_handle;
	p->wg.fLayout = MLF_EXPAND_WH;
	p->wg.notifyTarget = MWIDGET_NOTIFYTARGET_SELF;

	//area

	p->sv.area = (mScrollViewArea *)_create_area(M_WIDGET(p));
}

/** スクロール情報セット */

void MainWinCanvas_setScrollInfo()
{
	MainWinCanvas *p = APP_WIDGETS->canvas;
	mSize size;

	drawCalc_getCanvasScrollMax(APP_DRAW, &size);

	//スクロールバーの基準位置
	/* スクロールバーが中央に来る位置 */

	p->ptScrollBase.x = (size.w - p->sv.area->wg.w) >> 1;
	p->ptScrollBase.y = (size.h - p->sv.area->wg.h) >> 1;

	//セット

	mScrollBarSetStatus(p->sv.scrh, 0, size.w, p->sv.area->wg.w);
	mScrollBarSetStatus(p->sv.scrv, 0, size.h, p->sv.area->wg.h);

	//位置をリセット

	mScrollBarSetPos(p->sv.scrh, p->ptScrollBase.x);
	mScrollBarSetPos(p->sv.scrv, p->ptScrollBase.y);
}

/** スクロール位置セット */

void MainWinCanvas_setScrollPos()
{
	MainWinCanvas *p = APP_WIDGETS->canvas;
	
	mScrollBarSetPos(p->sv.scrh, APP_DRAW->ptScroll.x + p->ptScrollBase.x);
	mScrollBarSetPos(p->sv.scrv, APP_DRAW->ptScroll.y + p->ptScrollBase.y);
}
