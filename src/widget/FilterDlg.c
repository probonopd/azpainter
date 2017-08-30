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
 * フィルタダイアログ
 *************************************/

#include "mDef.h"
#include "mGui.h"
#include "mStr.h"
#include "mRectBox.h"
#include "mWidget.h"
#include "mDialog.h"
#include "mContainer.h"
#include "mWindow.h"
#include "mLabel.h"
#include "mButton.h"
#include "mCheckButton.h"
#include "mLineEdit.h"
#include "mComboBox.h"
#include "mEvent.h"
#include "mTrans.h"
#include "mUtil.h"

#include "defWidgets.h"
#include "defConfig.h"
#include "defDraw.h"

#include "LayerItem.h"
#include "defTileImage.h"
#include "TileImage.h"
#include "TileImageDrawInfo.h"
#include "AppCursor.h"

#include "FilterDrawInfo.h"

#include "FilterBar.h"
#include "FilterPrev.h"
#include "FilterDefWidget.h"
#include "FilterSaveData.h"

#include "draw_main.h"
#include "draw_calc.h"
#include "draw_select.h"
#include "draw_op_sub.h"

#include "trgroup.h"


//--------------------

typedef struct
{
	mWidget wg;
	mContainerData ct;
	mWindowData win;
	mDialogData dlg;

	uint16_t menuid;		//各フィルタのメニュー項目文字列ID
	FilterDrawInfo *info;

	TileImage *img_prev,	//プレビュー用イメージ
		*img_current;		//カレントイメージ (ポインタ保存用)

	uint8_t dat_flags,
		prevtype,		//プレビュータイプ
		bPreview;		//プレビューのチェックが ON か

	char saveid_bar[FILTER_BAR_NUM],		//データ保存の識別ID (文字)
		saveid_combo[FILTER_COMBOBOX_NUM],
		saveid_ckbtt[FILTER_COMBOBOX_NUM];

	int wgnum_bar,		//各ウィジェットの作成数
		wgnum_ckbtt,
		wgnum_combo;

	int16_t defval_bar[FILTER_BAR_NUM];	//バーの初期値 (リセット時用)

	char *savedata_text;	//保存データのテキスト (読み込み用。NULL でなし)

	mRect rcLastUpdate;		//漫画用プレビュー時、前回の更新範囲

	FilterPrev *prevwg;
	FilterBar *bar[FILTER_BAR_NUM];
	mLineEdit *edit_bar[FILTER_BAR_NUM];
	mCheckButton *ckbtt[FILTER_CHECKBTT_NUM];
	mComboBox *combo[FILTER_COMBOBOX_NUM];
	mWidget *wgdef;		//定義ウィジェット
}_filter_dlg;

//--------------------

#define DATFLAGS_MASK_PREVTYPE  7
#define DATFLAGS_RESET_BUTTON   (1<<3)	//リセットボタンあり
#define DATFLAGS_GET_CANVAS_POS (1<<4)	//キャンバス位置取得
#define DATFLAGS_SAVE_DATA      (1<<5)	//データ保存

//プレビュータイプ
enum
{
	PREVTYPE_NONE,
	PREVTYPE_IN_DIALOG,
	PREVTYPE_CANVAS_FOR_COLOR,	//キャンバスプレビュー (カラー処理用)
	PREVTYPE_CANVAS_FOR_COMIC	//キャンバスプレビュー (描画-漫画用)
};

//ウィジェットタイプ
enum
{
	WGTYPE_BAR,
	WGTYPE_CHECKBUTTON,
	WGTYPE_COMBOBOX,
	WGTYPE_LABEL,

	WGTYPE_DEF_CLIPPING = 128,
	WGTYPE_DEF_LEVEL,
	WGTYPE_DEF_REPLACE_COL
};

//--------------------

enum
{
	WID_FILTERPREV = 100,
	WID_CK_PREVIEW,
	WID_BTT_RESET,

	WID_BAR_TOP = 200,
	WID_BAREDIT_TOP = 250,
	WID_CKBTT_TOP = 300,
	WID_COMBO_TOP = 400,

	WID_DEF_LEVEL = 500,
	WID_DEF_CLIPPING,
	WID_DEF_REPLACE_COL
};

enum
{
	TRID_RESET = 1,
	TRID_CLIPPING,

	//R,G,B,X,Y 用ID (仮値)
	TRID_LABEL_ALPHABET = 240
};

//--------------------



//===========================
// プレビュー
//===========================


/** プレビューを OFF へ変更時、元のイメージを表示 */

static void _change_preview_off(_filter_dlg *p)
{
	FilterDrawInfo *info = p->info;

	switch(p->prevtype)
	{
		//ダイアログ内プレビュー
		case PREVTYPE_IN_DIALOG:
			if(p->prevwg && p->img_prev)
			{
				TileImage_freeAllTiles(p->img_prev);

				FilterPrev_drawImage(p->prevwg, info->imgsrc);
			}
			break;

		//キャンバスプレビュー (カラー処理用)
		case PREVTYPE_CANVAS_FOR_COLOR:
			if(p->img_prev)
			{
				mWidgetSetCursor(M_WIDGET(p), AppCursor_getWaitCursor());

				APP_DRAW->curlayer->img = p->img_current;

				drawUpdate_rect_imgcanvas_fromRect(APP_DRAW, &info->rc);

				APP_DRAW->curlayer->img = info->imgdst;

				mWidgetSetCursor(M_WIDGET(p), 0);
			}
			break;

		//キャンバスプレビュー (漫画用)
		case PREVTYPE_CANVAS_FOR_COMIC:
			//前回の描画範囲がある場合、クリア

			if(p->img_prev && !mRectIsEmpty(&p->rcLastUpdate))
			{
				mWidgetSetCursor(M_WIDGET(p), AppCursor_getWaitCursor());

				TileImage_freeAllTiles(p->img_prev);

				drawUpdate_rect_imgcanvas_fromRect(APP_DRAW, &p->rcLastUpdate);

				mRectEmpty(&p->rcLastUpdate);

				mWidgetSetCursor(M_WIDGET(p), 0);
			}
			break;
	}
}

/** プレビュー実行 */

static void _run_preview(_filter_dlg *p)
{
	FilterDrawInfo *info = p->info;
	mRect rc;

	switch(p->prevtype)
	{
		//ダイアログ内プレビュー
		case PREVTYPE_IN_DIALOG:
			if(p->bPreview && p->prevwg && p->img_prev)
			{
				mWidgetSetCursor(M_WIDGET(p), AppCursor_getWaitCursor());

				TileImage_freeAllTiles(p->img_prev);

				(info->drawfunc)(info);

				FilterPrev_drawImage(p->prevwg, p->img_prev);

				mWidgetSetCursor(M_WIDGET(p), 0);
			}
			break;

		//キャンバスプレビュー (カラー処理用)
		case PREVTYPE_CANVAS_FOR_COLOR:
			if(p->bPreview && p->img_prev)
			{
				mWidgetSetCursor(M_WIDGET(p), AppCursor_getWaitCursor());

				(info->drawfunc)(info);

				drawUpdate_rect_imgcanvas_fromRect(APP_DRAW, &info->rc);

				mWidgetSetCursor(M_WIDGET(p), 0);
			}
			break;

		//キャンバスプレビュー (漫画用)
		case PREVTYPE_CANVAS_FOR_COMIC:
			if(p->bPreview && p->img_prev)
			{
				mWidgetSetCursor(M_WIDGET(p), AppCursor_getWaitCursor());

				//前回の描画範囲がある場合、クリア

				rc = p->rcLastUpdate;

				if(!mRectIsEmpty(&p->rcLastUpdate))
				{
					TileImage_freeAllTiles(p->img_prev);
					mRectEmpty(&p->rcLastUpdate);
				}

				//描画

				TileImageDrawInfo_clearDrawRect();

				(info->drawfunc)(info);

				//更新 (前回の描画範囲と合わせた範囲)

				mRectUnion(&rc, &g_tileimage_dinfo.rcdraw);

				drawUpdate_rect_imgcanvas_fromRect(APP_DRAW, &rc);

				//描画範囲記録

				p->rcLastUpdate = g_tileimage_dinfo.rcdraw;

				mWidgetSetCursor(M_WIDGET(p), 0);
			}
			break;
	}
}

/** キャンバスプレビュー時の処理範囲セット */

static mBool _set_canvasprev_rect(FilterDrawInfo *info,int prevtype)
{
	mRect rc;

	if(prevtype == PREVTYPE_CANVAS_FOR_COMIC)
	{
		//---- 漫画用 (キャンバス全体)
		
		rc.x1 = rc.y1 = 0;
		rc.x2 = APP_DRAW->imgw - 1;
		rc.y2 = APP_DRAW->imgh - 1;
	}
	else
	{
		//---- カラー用
	
		if(drawSel_isHave())
			//選択範囲がある場合はその範囲
			rc = APP_DRAW->sel.rcsel;
		else
		{
			//選択範囲がない場合
			/* 透明でない範囲。
			 * 空イメージの場合は処理しても意味がないので、プレビューなし。 */
				
			if(!TileImage_getHaveImageRect_pixel(APP_DRAW->curlayer->img, &rc, NULL))
				return FALSE;
		}

		//キャンバス範囲外は処理しても見えないので除外

		if(!drawCalc_clipImageRect(APP_DRAW, &rc))
			return FALSE;
	}

	//セット

	info->rc = rc;
	mBoxSetByRect(&info->box, &rc);

	return TRUE;
}

/** プレビュー初期化 */

static void _init_preview(_filter_dlg *p)
{
	FilterDrawInfo *info = p->info;
	TileImage *curimg;

	curimg = APP_DRAW->curlayer->img;

	switch(p->prevtype)
	{
		//ダイアログ内プレビュー (img_prev が NULL でプレビューなし)
		case PREVTYPE_IN_DIALOG:
			if(p->prevwg)
			{
				//プレビューの描画範囲を mRect,mBox で取得
				
				FilterPrev_getDrawArea(p->prevwg, &info->rc, &info->box);

				//描画結果用プレビューイメージ作成

				p->img_prev = TileImage_newFromRect(curimg->coltype, &info->rc);

				if(p->img_prev)
				{
					p->img_prev->rgb = curimg->rgb;

					//

					info->imgsrc = curimg;
					info->imgdst = p->img_prev;

					/* プレビューが初期状態で ON の場合はプレビュー実行。
					 * OFF の場合はソースイメージをセット */

					if(p->bPreview)
						_run_preview(p);
					else
						FilterPrev_drawImage(p->prevwg, curimg);
				}
			}
			break;

		//キャンバスプレビュー (カラー処理用)
		/* カレントレイヤのイメージをプレビュー用画像と置き換える。
		 * カレントレイヤイメージをソースとし、プレビュー用画像に描画する。
		 * (img_prev が NULL でプレビューなし)
		 */
		case PREVTYPE_CANVAS_FOR_COLOR:
			if(_set_canvasprev_rect(info, PREVTYPE_CANVAS_FOR_COLOR))
			{
				//プレビュー用にカレントイメージを複製
			
				p->img_prev = TileImage_newClone(curimg);

				if(p->img_prev)
				{
					//カレントレイヤのイメージと置換え
					
					p->img_current = curimg;

					APP_DRAW->curlayer->img = p->img_prev;

					//

					info->imgsrc = curimg;
					info->imgdst = p->img_prev;

					_run_preview(p);
				}
			}
			break;

		//キャンバスプレビュー (漫画用)
		case PREVTYPE_CANVAS_FOR_COMIC:
			if(_set_canvasprev_rect(info, PREVTYPE_CANVAS_FOR_COMIC))
			{
				//イメージ作成 (色は1色のみなので、A16)
				
				p->img_prev = TileImage_newFromRect(TILEIMAGE_COLTYPE_ALPHA16, &info->rc);

				if(p->img_prev)
				{
					//描画初期化

					drawOpSub_setDrawInfo_filter_comic_brush();

					g_tileimage_dinfo.funcDrawPixel = TileImage_setPixel_subdraw;

					//

					APP_DRAW->w.tileimg_filterprev = p->img_prev; //キャンバス更新時に挿入させる

					info->imgsrc = curimg;
					info->imgdst = p->img_prev;

					mRectEmpty(&p->rcLastUpdate);

					_run_preview(p);
				}
			}
			break;
	}
}

/** エディットのテキスト変更時のプレビュー */

static void _run_preview_edit(_filter_dlg *p)
{
	if(p->prevtype != PREVTYPE_NONE && p->bPreview)
	{
		//タイマーで遅延
		mWidgetTimerAdd(M_WIDGET(p), 0, 300, 0);
	}
}


//===========================
// イベント
//===========================


/** リセット */

static void _reset_value(_filter_dlg *p)
{
	int i,n;

	//バーの値

	for(i = 0; i < p->wgnum_bar; i++)
	{
		n = p->defval_bar[i];
		
		FilterBar_setPos(p->bar[i], n);
		mLineEditSetNum(p->edit_bar[i], n);

		p->info->val_bar[i] = n;
	}

	_run_preview(p);
}

/** 通知 */

static void _event_notify(_filter_dlg *p,mEvent *ev)
{
	int id,type,n;
	FilterDrawInfo *info = p->info;

	id = ev->notify.id;
	type = ev->notify.type;

	if(id >= WID_BAR_TOP && id < WID_BAR_TOP + FILTER_BAR_NUM)
	{
		//バー

		if(ev->notify.param2)
			_run_preview(p);
		else
		{
			id -= WID_BAR_TOP;
			n = ev->notify.param1;

			info->val_bar[id] = n;

			mLineEditSetNum(p->edit_bar[id], n);
		}
	}
	else if(id >= WID_BAREDIT_TOP && id < WID_BAREDIT_TOP + FILTER_BAR_NUM)
	{
		//バーのエディット

		if(type == MLINEEDIT_N_CHANGE)
		{
			id -= WID_BAREDIT_TOP;
			n = mLineEditGetNum(p->edit_bar[id]);

			info->val_bar[id] = n;

			FilterBar_setPos(p->bar[id], n);

			_run_preview_edit(p);
		}
	}
	else if(id >= WID_COMBO_TOP && id < WID_COMBO_TOP + FILTER_COMBOBOX_NUM)
	{
		//コンボボックス

		if(type == MCOMBOBOX_N_CHANGESEL)
		{
			info->val_combo[id - WID_COMBO_TOP] = ev->notify.param2;

			_run_preview(p);
		}
	}
	else if(id >= WID_CKBTT_TOP && id < WID_CKBTT_TOP + FILTER_CHECKBTT_NUM)
	{
		//チェックボタン

		info->val_ckbtt[id - WID_CKBTT_TOP] ^= 1;

		_run_preview(p);
	}
	else
	{
		//----- ほか
		
		switch(id)
		{
			//プレビューの表示位置変更
			case WID_FILTERPREV:
				FilterPrev_getDrawArea(p->prevwg, &info->rc, &info->box);

				if(p->img_prev)
					TileImage_setOffset(p->img_prev, info->box.x, info->box.y);

				if(p->bPreview)
					_run_preview(p);
				else
					FilterPrev_drawImage(p->prevwg, info->imgsrc);
				break;
		
			//レベル補正
			case WID_DEF_LEVEL:
				FilterWgLevel_getValue((FilterWgLevel *)p->wgdef, info->val_bar);

				_run_preview(p);
				break;
			//クリッピング
			case WID_DEF_CLIPPING:
				info->clipping ^= 1;

				_run_preview(p);
				break;
			//色置き換え
			case WID_DEF_REPLACE_COL:
				FilterWgRepcol_getColor((FilterWgRepcol *)p->wgdef, info->val_bar);
			
				if(ev->notify.type == FILTERWGREPCOL_N_CHANGE_EDIT)
					_run_preview_edit(p);
				else
					_run_preview(p);
				break;

			//プレビュー
			case WID_CK_PREVIEW:
				p->bPreview ^= 1;
				APP_CONF->fView ^= CONFIG_VIEW_F_FILTERDLG_PREVIEW;

				if(p->bPreview)
					_run_preview(p);
				else
					_change_preview_off(p);
				break;
			//リセット
			case WID_BTT_RESET:
				_reset_value(p);
				break;
		}
	}
}

/** イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	switch(ev->type)
	{
		case MEVENT_NOTIFY:
			_event_notify((_filter_dlg *)wg, ev);
			break;

		//タイマー (プレビュー更新)
		case MEVENT_TIMER:
			mWidgetTimerDelete(wg, 0);

			_run_preview((_filter_dlg *)wg);
			break;
	}

	return mDialogEventHandle_okcancel(wg, ev);
}


//===========================
// 作成
//===========================


/** ウィジェット作成 : バー + エディット */

static void _create_wg_bar(_filter_dlg *p,mWidget *ctg,const uint8_t *dat,int size)
{
	mWidget *ct;
	FilterBar *bar;
	mLineEdit *edit;
	int no,barw,min,max,def,center,dig,init,savid,reset_val;

	//----- 値

	barw = mGetBuf16BE(dat);
	min = (int16_t)mGetBuf16BE(dat + 2);
	max = (int16_t)mGetBuf16BE(dat + 4);

	//デフォルト値 (省略の場合、最小が負なら 0、そうでなければ最小値)

	if(size > 6)
		def = (int16_t)mGetBuf16BE(dat + 6);
	else
		def = (min < 0)? 0: min;

	//中央の値 (省略の場合、最小が負なら 0、そうでなければ最小値=なし)

	if(size > 8)
		center = (int16_t)mGetBuf16BE(dat + 8);
	else
		center = (min < 0)? 0: min;
	
	if(size > 10) dig = dat[10]; else dig = 0;
	if(size > 11) init = dat[11]; else init = 0;
	if(size > 12) savid = dat[12]; else savid = 0;

	//---- 初期値

	if(init == 1)
		def = APP_DRAW->imgdpi;

	//リセット用の値

	reset_val = def;

	//保存値取得して初期値に

	if(savid)
		FilterSaveData_getValue(p->savedata_text, savid, &def);

	//--------

	no = p->wgnum_bar;

	ct = mContainerCreate(ctg, MCONTAINER_TYPE_HORZ, 0, 6, MLF_EXPAND_W);

	//バー

	p->bar[no] = bar = FilterBar_new(ct, WID_BAR_TOP + no, MLF_EXPAND_W | MLF_MIDDLE, barw,
		min, max, def, center);

	//エディット

	p->edit_bar[no] = edit = mLineEditCreate(ct, WID_BAREDIT_TOP + no,
		MLINEEDIT_S_SPIN | MLINEEDIT_S_NOTIFY_CHANGE, MLF_MIDDLE, 0);

	mLineEditSetWidthByLen(edit, 6);
	mLineEditSetNumStatus(edit, min, max, dig);
	mLineEditSetNum(edit, def);

	//セット

	p->defval_bar[no] = reset_val;	//リセット時用に保存
	p->saveid_bar[no] = savid;

	p->info->val_bar[no] = def; //初期値

	p->wgnum_bar++;
}

/** ウィジェット作成: チェックボタン */

static void _create_wg_checkbtt(_filter_dlg *p,mWidget *ctg,int trid,const uint8_t *dat,int size)
{
	int no,savid,def;
	uint8_t checked = 0;

	//値

	if(size > 0) checked = dat[0];
	if(size > 1) savid = dat[1]; else savid = 0;

	if(savid
		&& FilterSaveData_getValue(p->savedata_text, savid, &def))
		checked = (def != 0);

	//作成

	no = p->wgnum_ckbtt;

	p->ckbtt[no] = mCheckButtonCreate(ctg, WID_CKBTT_TOP + no, 0, 0, 0,
		M_TR_T(trid), checked);

	//

	p->saveid_ckbtt[no] = savid;

	p->info->val_ckbtt[no] = (checked != 0);

	p->wgnum_ckbtt++;
}

/** ウィジェット作成: コンボボックス */

static void _create_wg_combobox(_filter_dlg *p,mWidget *ctg,const uint8_t *dat,int size)
{
	mComboBox *cb;
	int num,trid,def,no,savid;

	//値

	num = dat[0];
	trid = mGetBuf16BE(dat + 1);

	if(size > 3) def = dat[3]; else def = 0;
	if(size > 4) savid = dat[4]; else savid = 0;

	if(savid)
		FilterSaveData_getValue(p->savedata_text, savid, &def);

	//作成

	no = p->wgnum_combo;

	p->combo[no] = cb = mComboBoxCreate(ctg, WID_COMBO_TOP + no, 0, 0, 0);

	mComboBoxAddTrItems(cb, num, trid, 0);
	mComboBoxSetWidthAuto(cb);
	mComboBoxSetSel_index(cb, def);

	//

	p->saveid_combo[no] = savid;

	p->info->val_combo[no] = def;

	p->wgnum_combo++;
}

/** ウィジェット作成: レベル補正 */

static void _create_wg_def_level(_filter_dlg *p,mWidget *ctg)
{
	uint32_t *hist;

	//ヒストグラム取得

	hist = TileImage_getHistogram(APP_DRAW->curlayer->img);
	if(!hist) return;

	//作成

	p->wgdef = (mWidget *)FilterWgLevel_new(ctg, WID_DEF_LEVEL, hist);

	mFree(hist);

	//値

	FilterWgLevel_getValue((FilterWgLevel *)p->wgdef, p->info->val_bar);
}

/** 項目ウィジェット作成 メイン */

static void _create_item_widgets(_filter_dlg *p,mWidget *parent,const uint8_t *dat)
{
	mWidget *ctg;
	uint8_t type,trid,size;
	char m[2] = {0,0},alphabet[] = {'R','G','B','X','Y'};

	ctg = mContainerCreateGrid(parent, 2, 6, 7, MLF_EXPAND_W);

	while(1)
	{
		//ウィジェットタイプ
		
		type = *(dat++);
		if(type == 255) break;

		//

		if(type >= 128)
		{
			//----- 定義ウィジェット

			switch(type)
			{
				//レベル補正
				case WGTYPE_DEF_LEVEL:
					_create_wg_def_level(p, ctg);
					break;
				//クリッピング
				case WGTYPE_DEF_CLIPPING:
					mWidgetNew(0, ctg);
					mCheckButtonCreate(ctg, WID_DEF_CLIPPING, 0, 0, 0,
						M_TR_T(TRID_CLIPPING), TRUE);

					p->info->clipping = TRUE;
					break;
				//描画色置換
				case WGTYPE_DEF_REPLACE_COL:
					p->wgdef = (mWidget *)FilterWgRepcol_new(ctg, WID_DEF_REPLACE_COL, APP_DRAW->col.drawcol);

					FilterWgRepcol_getColor((FilterWgRepcol *)p->wgdef, p->info->val_bar);
					break;
			}
		}
		else
		{
			//ラベルの文字列ID (0 でなし)
			
			trid = *(dat++);

			//データサイズ

			size = *(dat++);

			//ラベル

			if(trid && (type == WGTYPE_BAR || type == WGTYPE_COMBOBOX))
			{
				//バー/コンボボックス
				
				if(trid >= TRID_LABEL_ALPHABET)
				{
					//アルファベット
					m[0] = alphabet[trid - TRID_LABEL_ALPHABET];
					mLabelCreate(ctg, 0, MLF_MIDDLE, 0, m);
				}
				else
					mLabelCreate(ctg, 0, MLF_MIDDLE, 0, M_TR_T(trid));
			}
			else
				mWidgetNew(0, ctg);

			//各ウィジェット

			switch(type)
			{
				//バー
				case WGTYPE_BAR:
					_create_wg_bar(p, ctg, dat, size);
					break;
				//チェックボタン
				case WGTYPE_CHECKBUTTON:
					_create_wg_checkbtt(p, ctg, trid, dat, size);
					break;
				//コンボボックス
				case WGTYPE_COMBOBOX:
					_create_wg_combobox(p, ctg, dat, size);
					break;
				//ラベル
				case WGTYPE_LABEL:
					mLabelCreate(ctg, MLABEL_S_BORDER, 0, 0, M_TR_T(trid));
					break;
			}

			dat += size;
		}
	}
}

/** 内容のウィジェット作成メイン */

static void _create_widgets(_filter_dlg *p,const uint8_t *dat,mSize *prevsize)
{
	mWidget *ct,*wg;

	M_TR_G(TRGROUP_DLG_FILTER);

	//------- プレビュー + ウィジェット

	ct = mContainerCreate(M_WIDGET(p), MCONTAINER_TYPE_HORZ, 0, 0, MLF_EXPAND_W);

	//プレビュー

	if(p->prevtype == PREVTYPE_IN_DIALOG)
	{
		p->prevwg = FilterPrev_new(ct, WID_FILTERPREV, prevsize->w, prevsize->h,
			APP_DRAW->curlayer->img, APP_DRAW->imgw, APP_DRAW->imgh);
	}

	//項目ウィジェット

	_create_item_widgets(p, ct, dat);

	//------ ボタン

	ct = mContainerCreate(M_WIDGET(p), MCONTAINER_TYPE_HORZ, 0, 3, MLF_EXPAND_W);
	ct->margin.top = 15;

	//プレビューチェック

	if(p->prevtype != PREVTYPE_NONE)
	{
		mCheckButtonCreate(ct, WID_CK_PREVIEW, 0, 0, M_MAKE_DW4(0,0,4,0),
			M_TR_T2(M_TRGROUP_SYS, M_TRSYS_PREVIEW), p->bPreview);
	}

	//OK

	wg = (mWidget *)mButtonCreate(ct, M_WID_OK, 0, MLF_EXPAND_X | MLF_RIGHT,
		0, M_TR_T2(M_TRGROUP_SYS, M_TRSYS_OK));

	wg->fState |= MWIDGET_STATE_ENTER_DEFAULT;

	//キャンセル

	mButtonCreate(ct, M_WID_CANCEL, 0, 0, 0, M_TR_T2(M_TRGROUP_SYS, M_TRSYS_CANCEL));

	//リセット

	if(p->dat_flags & DATFLAGS_RESET_BUTTON)
		mButtonCreate(ct, WID_BTT_RESET, 0, 0, 0, M_TR_T(TRID_RESET));
}

/** ダイアログ作成メイン */

static _filter_dlg *_create_dlg(mWindow *owner,
	uint16_t menuid,const uint8_t *dat,FilterDrawInfo *info)
{
	_filter_dlg *p;
	mSize prevsize;

	p = (_filter_dlg *)mDialogNew(sizeof(_filter_dlg), owner, MWINDOW_S_DIALOG_NORMAL);
	if(!p) return NULL;

	p->menuid = menuid;
	p->info = info;
	p->bPreview = ((APP_CONF->fView & CONFIG_VIEW_F_FILTERDLG_PREVIEW) != 0);

	p->wg.event = _event_handle;

	//

	mContainerSetPadding_one(M_CONTAINER(p), 10);

	mWindowSetTitle(M_WINDOW(p), M_TR_T2(TRGROUP_MAINMENU, menuid));

	//----- データからウィジェット作成

	//先頭データ

	p->dat_flags = *(dat++);
	p->prevtype = p->dat_flags & DATFLAGS_MASK_PREVTYPE;

	//プレビューサイズ (ダイアログ内プレビュー時)

	if(p->prevtype == PREVTYPE_IN_DIALOG)
	{
		prevsize.w = mGetBuf16BE(dat);
		prevsize.h = mGetBuf16BE(dat + 2);

		dat += 4;
	}

	//保存データ検索

	if(p->dat_flags & DATFLAGS_SAVE_DATA)
		p->savedata_text = FilterSaveData_getText(menuid);

	//ウィジェット作成

	_create_widgets(p, dat, &prevsize);

	//--------------

	//キャンバス上でのクリックを取得するため、ダイアログのポインタセット

	APP_DRAW->w.ptmp = p;

	//キャンバス位置のデフォルトをイメージ中央に

	if(p->dat_flags & DATFLAGS_GET_CANVAS_POS)
	{
		info->imgx = APP_DRAW->imgw / 2;
		info->imgy = APP_DRAW->imgh / 2;
	}

	//選択範囲

	info->imgsel = APP_DRAW->tileimgSel;

	//プレビュー初期化

	_init_preview(p);

	return p;
}


//==============================


/** データ保存: 文字列に追加 */

static void _save_data_add(mStr *str,char id,int val)
{
	if(id)
	{
		mStrAppendChar(str, id);
		mStrAppendInt(str, val);
	}
}

/** データ保存 */

static void _save_data(_filter_dlg *p)
{
	mStr str = MSTR_INIT;
	int i;

	//バー

	for(i = 0; i < p->wgnum_bar; i++)
		_save_data_add(&str, p->saveid_bar[i], p->info->val_bar[i]);

	//チェック

	for(i = 0; i < p->wgnum_ckbtt; i++)
		_save_data_add(&str, p->saveid_ckbtt[i], p->info->val_ckbtt[i]);

	//コンボボックス

	for(i = 0; i < p->wgnum_combo; i++)
		_save_data_add(&str, p->saveid_combo[i], p->info->val_combo[i]);

	FilterSaveData_setData(p->menuid, str.buf);

	mStrFree(&str);
}

/** 終了処理 */

static void _finish(_filter_dlg *p,mBool ok)
{
	mBox box;

	//タイマー消去

	mWidgetTimerDelete(M_WIDGET(p), 0);

	//プレビュー終了処理

	switch(p->prevtype)
	{
		//キャンバスプレビュー (カラー用)
		case PREVTYPE_CANVAS_FOR_COLOR:
			if(p->img_prev)
			{
				//カレントレイヤのイメージポインタを戻す
				
				APP_DRAW->curlayer->img = p->img_current;

				//キャンセル時は更新
				
				if(!ok)
					drawUpdate_rect_imgcanvas_canvasview_fromRect(APP_DRAW, &p->info->rc);
			}
			break;

		//漫画用 (前回の描画範囲があれば更新)
		case PREVTYPE_CANVAS_FOR_COMIC:
			if(p->img_prev)
			{
				APP_DRAW->w.tileimg_filterprev = NULL;
			
				drawUpdate_rect_imgcanvas_canvasview_fromRect(APP_DRAW, &p->rcLastUpdate);
			}
			break;
	}

	TileImage_free(p->img_prev);

	//OK 時、データ保存

	if(ok && (p->dat_flags & DATFLAGS_SAVE_DATA))
		_save_data(p);

	//キャンバスプレビュー時、ウィンドウ位置記録

	if(p->prevtype == PREVTYPE_CANVAS_FOR_COLOR
		|| p->prevtype == PREVTYPE_CANVAS_FOR_COMIC)
	{
		mWindowGetSaveBox(M_WINDOW(p), &box);

		APP_CONF->pt_filterdlg.x = box.x;
		APP_CONF->pt_filterdlg.y = box.y;
	}
}

/** 初期表示 (キャンバスプレビューあり時) */

static void _show_canvas_preview(_filter_dlg *p)
{
	mPoint pt;

	pt = APP_CONF->pt_filterdlg;

	if(pt.x == -100000 && pt.y == -100000)
		//指定なし時
		mWindowMoveResizeShow_hintSize(M_WINDOW(p));
	else
	{
		//リサイズ

		mGuiCalcHintSize();
		mWidgetResize(M_WIDGET(p), p->wg.initW, p->wg.initH);

		//位置

		mWindowAdjustPosDesktop(M_WINDOW(p), &pt, TRUE);
		mWidgetMove(M_WIDGET(p), pt.x, pt.y);
		
		mWidgetShow(M_WIDGET(p), 1);
	}
}

/** ダイアログ実行 */

mBool FilterDlg_run(mWindow *owner,uint16_t menuid,const uint8_t *dat,FilterDrawInfo *info)
{
	_filter_dlg *p;
	mBool ret;

	//ダイアログ作成

	p = _create_dlg(owner, menuid, dat, info);
	if(!p) return FALSE;

	//位置、サイズ、表示

	if(p->prevtype == PREVTYPE_NONE || p->prevtype == PREVTYPE_IN_DIALOG)
		//プレビューなし or ダイアログ内
		mWindowMoveResizeShow_hintSize(M_WINDOW(p));
	else
		_show_canvas_preview(p);
	
	//
	
	ret = mDialogRun(M_DIALOG(p), FALSE);

	_finish(p, ret);

	mWidgetDestroy(M_WIDGET(p));

	return ret;
}

/** ダイアログ中、キャンバス上が左クリックされた時 */

void FilterDlg_onCanvasClick(mWidget *wg,int x,int y)
{
	_filter_dlg *p = (_filter_dlg *)wg;
	mPoint pt;

	if(p->dat_flags & DATFLAGS_GET_CANVAS_POS)
	{
		drawCalc_areaToimage_pt(APP_DRAW, &pt, x, y);

		p->info->imgx = pt.x;
		p->info->imgy = pt.y;

		_run_preview(p);
	}
}
