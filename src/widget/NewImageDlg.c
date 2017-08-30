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
 * NewImageDlg
 * 
 * イメージ新規作成ダイアログ
 *****************************************/

#include "mDef.h"
#include "mStr.h"
#include "mWidget.h"
#include "mWindow.h"
#include "mDialog.h"
#include "mContainer.h"
#include "mLabel.h"
#include "mComboBox.h"
#include "mLineEdit.h"
#include "mListView.h"
#include "mTab.h"
#include "mEvent.h"
#include "mWidgetBuilder.h"
#include "mMessageBox.h"
#include "mTrans.h"

#include "defMacros.h"
#include "defConfig.h"

#include "trgroup.h"


//------------------------

static const char *g_builder =
"ct#h:sep=12:lf=wh;"
  "ct#v:lf=m;"
    "ct#g3:sep=6,7;"
      //幅
      "lb:lf=rm:TR=0,0;"
      "le#c:id=100:wlen=8;"
      "sp;"
      //高さ
      "lb:lf=rm:TR=0,1;"
      "le#c:id=101:wlen=8;"
      "cb:id=102:t=px\tcm\tinch;"
      //解像度
      "lb:lf=rm:tr=1;"
      "le#cs:id=103:wlen=6;"
      "lb:lf=m:t=dpi;"
    "-;"
    //レイヤタイプ
    "ct#h:sep=6:mg=t15;"
      "lb:lf=m:tr=2;"
      "cb:id=104;"
    "-;"
    //pxプレビュー
    "lb#cB:id=105:lf=w:mg=t18;"
  "-;"
  //リスト
  "ct#v:sep=4:lf=wh;"
    "tab#ts:id=106:lf=w;"
    "lv#,fv:id=107:lf=wh;"
    "ct#h:sep=3:lf=r;"
      "bt:id=108:tr=3;"
      "bt:id=109:tr=4;";

//------------------------

#define TABNO_RECENT  0
#define TABNO_RECORD  1
#define TABNO_DEFINED 2

#define WIDGETID_TOP 100
#define TRID_MES_LIMIT 1000

enum
{
	WNO_WIDTH,
	WNO_HEIGHT,
	WNO_UNIT,
	WNO_DPI,
	WNO_LAYERTYPE,
	WNO_LABEL_PX,
	WNO_TAB,
	WNO_LIST,
	WNO_BTT_ADD,
	WNO_BTT_DEL,

	WIDGET_NUM
};

//------------------------

typedef struct
{
	mWidget wg;
	mContainerData ct;
	mWindowData win;
	mDialogData dlg;

	mWidget *widgets[WIDGET_NUM];
	int tabno;
}NewImageDlg;

//------------------------


//========================
// 設定データ
//========================


/** 新規作成の履歴に追加 */

static void _set_config_recent(ImageSizeData *src)
{
	ImageSizeData *dst = APP_CONF->imgsize_recent;
	int i,del;

	del = CONFIG_IMAGESIZE_NUM - 1;

	//同じ値が存在するか

	for(i = 0; i < CONFIG_IMAGESIZE_NUM; i++)
	{
		if(dst[i].unit == 255) break;
	
		if(src->unit == dst[i].unit && src->dpi == dst[i].dpi
			&& src->w == dst[i].w && src->h == dst[i].h)
		{
			del = i;
			break;
		}
	}

	//挿入位置を空ける

	for(i = del; i > 0; i--)
		dst[i] = dst[i - 1];

	//先頭にセット

	dst[0] = *src;
}


//========================
// リストにデータセット
//========================


/** 履歴/登録リストセット */

static void _set_list_recent_record(mListView *lv,ImageSizeData *p)
{
	mStr str = MSTR_INIT;
	int i;

	for(i = 0; i < CONFIG_IMAGESIZE_NUM; i++, p++)
	{
		if(p->unit == 255) break;
	
		if(p->unit == 0)
			//px
			mStrSetFormat(&str, "%d x %d px : %ddpi", p->w, p->h, p->dpi);
		else
		{
			//cm,inch
			
			mStrSetFormat(&str, "%d.%02d x %d.%02d %s : %ddpi",
				p->w / 100, p->w % 100,
				p->h / 100, p->h % 100,
				(p->unit == 1)? "cm": "inch", p->dpi);
		}

		mListViewAddItem_textparam(lv, str.buf, i);
	}

	mStrFree(&str);
}

/** 規定リストセット */

static void _set_list_defined(mListView *lv)
{
	int i,j;
	mStr str = MSTR_INIT;
	uint32_t num[2][7] = { //mm 単位
		{
			(594 << 16) | 841, (420 << 16) | 594, (297 << 16) | 420,
			(210 << 16) | 297, (148 << 16) | 210, (105 << 16) | 148,
			(74 << 16) | 105
		},
		{
			(728 << 16) | 1030, (515 << 16) | 728, (364 << 16) | 515,
			(257 << 16) | 364, (182 << 16) | 257, (128 << 16) | 182,
			(91 << 16) | 128
		}
	};

	for(i = 0; i < 2; i++)
	{
		for(j = 0; j < 7; j++)
		{
			mStrSetFormat(&str, "%c%d : %d x %d mm",
				'A' + i, j + 1, num[i][j] >> 16, num[i][j] & 0xffff);

			mListViewAddItem_textparam(lv, str.buf, num[i][j]);
		}
	}

	mStrFree(&str);
}

/** タブ番号からリストセット */

static void _set_list(NewImageDlg *p,int no)
{
	mListView *lv;

	lv = M_LISTVIEW(p->widgets[WNO_LIST]);

	mListViewDeleteAllItem(lv);

	if(no == TABNO_RECENT)
		_set_list_recent_record(lv, APP_CONF->imgsize_recent);
	else if(no == TABNO_RECORD)
		_set_list_recent_record(lv, APP_CONF->imgsize_record);
	else
		_set_list_defined(lv);
}


//========================
// sub
//========================


/** ウィジェットに幅・高さの値セット */

static void _set_width_height(NewImageDlg *p,int wno,int size,int unit)
{
	if(unit == 0)
		mLineEditSetInt(M_LINEEDIT(p->widgets[wno]), size);
	else
		mLineEditSetDouble(M_LINEEDIT(p->widgets[wno]), size * 0.01, 2);
}

/** 履歴/登録リストからウィジェットに値セット */

static void _set_from_data(NewImageDlg *p,ImageSizeData *dat)
{
	mComboBoxSetSel_index(M_COMBOBOX(p->widgets[WNO_UNIT]), dat->unit);

	_set_width_height(p, WNO_WIDTH, dat->w, dat->unit);
	_set_width_height(p, WNO_HEIGHT, dat->h, dat->unit);

	mLineEditSetInt(M_LINEEDIT(p->widgets[WNO_DPI]), dat->dpi);
}

/** リストボタンの有効/無効 */

static void _enable_list_button(NewImageDlg *p)
{
	int f = (p->tabno == TABNO_RECORD);

	mWidgetEnable(p->widgets[WNO_BTT_ADD], f);
	mWidgetEnable(p->widgets[WNO_BTT_DEL], f);
}

/** 幅か高さ取得 */

static int _get_width_height(NewImageDlg *p,int wno,int unit)
{
	int n;

	if(unit == 0)
		n = mLineEditGetInt(M_LINEEDIT(p->widgets[wno]));
	else
		n = mLineEditGetDouble(M_LINEEDIT(p->widgets[wno])) * 100 + 0.5;

	return (n < 1)? 1: n;
}

/** ウィジェットからデータ取得 */

static void _get_data_from_widget(NewImageDlg *p,ImageSizeData *dat)
{
	int n;

	n = mComboBoxGetSelItemIndex(M_COMBOBOX(p->widgets[WNO_UNIT]));

	dat->unit = n;
	dat->w = _get_width_height(p, WNO_WIDTH, n);
	dat->h = _get_width_height(p, WNO_HEIGHT, n);
	dat->dpi = mLineEditGetNum(M_LINEEDIT(p->widgets[WNO_DPI]));
}

/** 登録リストに追加 */

static void _add_record(NewImageDlg *p)
{
	int i;

	//空き位置取得

	for(i = 0; i < CONFIG_IMAGESIZE_NUM; i++)
	{
		if(APP_CONF->imgsize_record[i].unit == 255) break;
	}

	if(i == CONFIG_IMAGESIZE_NUM) return;

	//セット

	_get_data_from_widget(p, APP_CONF->imgsize_record + i);

	if(i != CONFIG_IMAGESIZE_NUM - 1)
		APP_CONF->imgsize_record[i + 1].unit = 255;

	//リスト

	_set_list(p, p->tabno);
}

/** 登録リストから削除 */

static void _del_record(NewImageDlg *p)
{
	mListView *lv;
	int i,no;

	lv = M_LISTVIEW(p->widgets[WNO_LIST]);

	no = mListViewGetItemIndex(lv, NULL);
	if(no == -1) return;

	mListViewDeleteItem_sel(lv, NULL);

	//データ削除

	for(i = no; i < CONFIG_IMAGESIZE_NUM - 1; i++)
		APP_CONF->imgsize_record[i] = APP_CONF->imgsize_record[i + 1];

	APP_CONF->imgsize_record[CONFIG_IMAGESIZE_NUM - 1].unit = 255;
}

/** px サイズを取得 */

static void _get_pxsize(mSize *size,ImageSizeData *dat)
{
	double dw,dh;
	int w,h;

	w = dat->w;
	h = dat->h;

	if(dat->unit != 0)
	{
		dw = dat->w * 0.01 * dat->dpi;
		dh = dat->h * 0.01 * dat->dpi;

		if(dat->unit == 1)
		{
			dw /= 2.54;
			dh /= 2.54;
		}

		w = (int)(dw + 0.5);
		h = (int)(dh + 0.5);

		if(w < 1) w = 1;
		if(h < 1) h = 1;
	}

	size->w = w;
	size->h = h;
}

/** px サイズをセット */

static void _set_label_pxsize(NewImageDlg *p)
{
	ImageSizeData dat;
	mSize size;
	mStr str = MSTR_INIT;

	_get_data_from_widget(p, &dat);
	_get_pxsize(&size, &dat);

	mStrSetFormat(&str, "%d x %d px", size.w, size.h);

	mLabelSetText(M_LABEL(p->widgets[WNO_LABEL_PX]), str.buf);

	mStrFree(&str);
}


//========================
// イベント
//========================


/** OK 終了時 */

static void _end_ok(NewImageDlg *p)
{
	ImageSizeData dat;
	mSize size;

	_get_data_from_widget(p, &dat);
	_get_pxsize(&size, &dat);

	//サイズ制限チェック

	if(size.w > IMAGE_SIZE_MAX || size.h > IMAGE_SIZE_MAX)
	{
		mStr str = MSTR_INIT;

		mStrSetFormat(&str, M_TR_T2(TRGROUP_DLG_NEWIMAGE, TRID_MES_LIMIT), IMAGE_SIZE_MAX);
	
		mMessageBox(M_WINDOW(p), NULL, str.buf, MMESBOX_OK, MMESBOX_OK);

		mStrFree(&str);

		return;
	}

	//履歴の先頭にセット

	_set_config_recent(&dat);

	mDialogEnd(M_DIALOG(p), TRUE);
}

/** リスト項目選択時 */

static void _change_list_sel(NewImageDlg *p,int param)
{
	if(p->tabno == TABNO_RECENT)
		//履歴
		_set_from_data(p, APP_CONF->imgsize_recent + param);
	else if(p->tabno == TABNO_RECORD)
		//登録
		_set_from_data(p, APP_CONF->imgsize_record + param);
	else
	{
		//規定
	
		mComboBoxSetSel_index(M_COMBOBOX(p->widgets[WNO_UNIT]), 1);

		mLineEditSetDouble(M_LINEEDIT(p->widgets[WNO_WIDTH]), (param >> 16) * 0.1, 1);
		mLineEditSetDouble(M_LINEEDIT(p->widgets[WNO_HEIGHT]), (param & 0xffff) * 0.1, 1);
	}

	//px

	_set_label_pxsize(p);
}

/** イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	NewImageDlg *p = (NewImageDlg *)wg;

	if(ev->type == MEVENT_NOTIFY)
	{
		switch(ev->notify.id)
		{
			//幅、高さ、DPI
			case WIDGETID_TOP + WNO_WIDTH:
			case WIDGETID_TOP + WNO_HEIGHT:
			case WIDGETID_TOP + WNO_DPI:
				if(ev->notify.type == MLINEEDIT_N_CHANGE)
					_set_label_pxsize(p);
				break;
			//単位
			case WIDGETID_TOP + WNO_UNIT:
				if(ev->notify.type == MCOMBOBOX_N_CHANGESEL)
					_set_label_pxsize(p);
				break;
		
			//リスト項目選択時
			case WIDGETID_TOP + WNO_LIST:
				if(ev->notify.type == MLISTVIEW_N_CHANGE_FOCUS
					|| ev->notify.type == MLISTVIEW_N_CLICK_ON_FOCUS)
					_change_list_sel(p, (int)ev->notify.param2);
				break;
			//タブ変更時
			case WIDGETID_TOP + WNO_TAB:
				if(ev->notify.type == MTAB_N_CHANGESEL)
				{
					p->tabno = ev->notify.param1;

					_enable_list_button(p);

					_set_list(p, p->tabno);
				}
				break;
			//追加
			case WIDGETID_TOP + WNO_BTT_ADD:
				_add_record(p);
				break;
			//削除
			case WIDGETID_TOP + WNO_BTT_DEL:
				_del_record(p);
				break;
		
			//OK
			case M_WID_OK:
				_end_ok(p);
				break;
			//CANCEL
			case M_WID_CANCEL:
				mDialogEnd(M_DIALOG(wg), FALSE);
				break;
		}
	}

	return mDialogEventHandle(wg, ev);
}


//========================
// 作成
//========================


/** ウィジェット初期化 */

static void _init_widget(NewImageDlg *p)
{
	int i;
	ImageSizeData dat;

	for(i = 0; i < WIDGET_NUM; i++)
		p->widgets[i] = mWidgetFindByID(M_WIDGET(p), WIDGETID_TOP + i);

	//解像度

	mLineEditSetNumStatus(M_LINEEDIT(p->widgets[WNO_DPI]), 1, 9999, 0);

	//レイヤタイプ

	M_TR_G(TRGROUP_LAYERTYPE);
	mComboBoxAddTrItems(M_COMBOBOX(p->widgets[WNO_LAYERTYPE]), 4, 0, 0);
	M_TR_G(TRGROUP_DLG_NEWIMAGE);
	
	mComboBoxSetWidthAuto(M_COMBOBOX(p->widgets[WNO_LAYERTYPE]));
	mComboBoxSetSel_index(M_COMBOBOX(p->widgets[WNO_LAYERTYPE]), 0);

	//最新の履歴から値セット

	if(APP_CONF->imgsize_recent[0].unit != 255)
		dat = APP_CONF->imgsize_recent[0];
	else
	{
		dat.unit = 0;
		dat.dpi = 300;
		dat.w = dat.h = 500;
	}
	
	_set_from_data(p, &dat);

	//px

	_set_label_pxsize(p);

	//----------

	//タブ

	for(i = 0; i < 3; i++)
		mTabAddItemText(M_TAB(p->widgets[WNO_TAB]), M_TR_T(100 + i));

	mTabSetSel_index(M_TAB(p->widgets[WNO_TAB]), TABNO_RECENT);

	//リスト

	_set_list(p, TABNO_RECENT);

	mWidgetSetInitSize_fontHeight(p->widgets[WNO_LIST], 16, 12);

	//ボタン無効

	_enable_list_button(p);
}

/** 作成 */

static NewImageDlg *_create_dlg(mWindow *owner)
{
	NewImageDlg *p;

	//作成
	
	p = (NewImageDlg *)mDialogNew(sizeof(NewImageDlg), owner, MWINDOW_S_DIALOG_NORMAL);
	if(!p) return NULL;
	
	p->wg.event = _event_handle;

	//

	M_TR_G(TRGROUP_DLG_NEWIMAGE);

	mContainerSetPadding_one(M_CONTAINER(p), 8);
	p->ct.sepW = 15;

	mWindowSetTitle(M_WINDOW(p), M_TR_T(0));

	//ウィジェット

	mWidgetBuilderCreateFromText(M_WIDGET(p), g_builder);

	_init_widget(p);

	//OK/Cancel

	mContainerCreateOkCancelButton(M_WIDGET(p));

	return p;
}


//=====================


/** 新規作成ダイアログ実行 */

mBool NewImageDialog_run(mWindow *owner,mSize *size,int *dpi,int *layertype)
{
	NewImageDlg *p;
	mBool ret;

	p = _create_dlg(owner);
	if(!p) return FALSE;

	mWindowMoveResizeShow_hintSize(M_WINDOW(p));

	ret = mDialogRun(M_DIALOG(p), FALSE);

	if(ret)
	{
		_get_pxsize(size, APP_CONF->imgsize_recent);

		*dpi = APP_CONF->imgsize_recent[0].dpi;

		*layertype = mComboBoxGetSelItemIndex(M_COMBOBOX(p->widgets[WNO_LAYERTYPE]));
	}

	mWidgetDestroy(M_WIDGET(p));

	return ret;
}
