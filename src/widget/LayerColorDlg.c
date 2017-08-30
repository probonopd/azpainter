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
 * _dialog
 * 
 * レイヤ色選択ダイアログ
 *****************************************/

#include "mDef.h"
#include "mWidget.h"
#include "mDialog.h"
#include "mContainer.h"
#include "mWindow.h"
#include "mCheckButton.h"
#include "mLineEdit.h"
#include "mColorPreview.h"
#include "mHSVPicker.h"
#include "mSliderBar.h"
#include "mLabel.h"
#include "mTrans.h"
#include "mEvent.h"
#include "mColorConv.h"
#include "mPixbuf.h"
#include "mSysCol.h"

#include "defDraw.h"

#include "trgroup.h"
#include "trid_layer_dialogs.h"


//------------------------

typedef struct
{
	mWidget wg;
	mContainerData ct;
	mWindowData win;
	mDialogData dlg;

	uint32_t curcol;
	int coltype;

	mColorPreview *prev;
	mHSVPicker *hsv;
	mLabel *label[3];
	mSliderBar *slider[3];
	mLineEdit *edit[3];

}_dialog;

//------------------------

#define _COLTYPE_RGB 0
#define _COLTYPE_HSV 1

#define _SETCOLORVAL_CHANGE_TYPE  0
#define _SETCOLORVAL_CHANGE_HSV   1
#define _SETCOLORVAL_FROM_CURCOL  2

enum
{
	WID_HSV = 100,
	WID_PALETTE,
	WID_CK_RGB,
	WID_CK_HSV,
	WID_SLIDER1,
	WID_SLIDER2,
	WID_SLIDER3,
	WID_EDIT1,
	WID_EDIT2,
	WID_EDIT3
};

static mWidget *_palette_new(mWidget *parent);

//------------------------


//============================
// sub
//============================


/** カラー値セット (curcol から) */

static void _set_color_val(_dialog *p,int type)
{
	int i,max,c[3];
	double d[3];
	char m[2] = {0,0}, name[2][3] = {{'R','G','B'}, {'H','S','V'}};

	//色の数値取得

	if(p->coltype == _COLTYPE_RGB)
	{
		c[0] = M_GET_R(p->curcol);
		c[1] = M_GET_G(p->curcol);
		c[2] = M_GET_B(p->curcol);
	}
	else
	{
		if(type == _SETCOLORVAL_CHANGE_HSV)
			mHSVPickerGetHSVColor(p->hsv, d);
		else
			mRGBtoHSV_pac(p->curcol, d);

		c[0] = (int)(d[0] * 360 + 0.5);
		c[1] = (int)(d[1] * 255 + 0.5);
		c[2] = (int)(d[2] * 255 + 0.5);
	}

	//セット

	for(i = 0; i < 3; i++)
	{
		if(type == _SETCOLORVAL_CHANGE_TYPE)
		{
			//範囲

			max = (p->coltype == _COLTYPE_HSV && i == 0)? 359: 255;

			mSliderBarSetRange(p->slider[i], 0, max);
			mLineEditSetNumStatus(p->edit[i], 0, max, 0);

			//名前

			m[0] = name[p->coltype][i];
			mLabelSetText(p->label[i], m);
		}

		mSliderBarSetPos(p->slider[i], c[i]);
		mLineEditSetNum(p->edit[i], c[i]);
	}

	//再レイアウト

	if(type == _SETCOLORVAL_CHANGE_TYPE)
		mWidgetReLayout(p->label[0]->wg.parent);
}

/** 数値から色更新 */

static void _change_value(_dialog *p,int no)
{
	int i,c[3];
	double s,v;

	for(i = 0; i < 3; i++)
		c[i] = mSliderBarGetPos(p->slider[i]);

	if(p->coltype == _COLTYPE_RGB)
	{
		p->curcol = M_RGB(c[0], c[1], c[2]);

		mHSVPickerSetRGBColor(p->hsv, p->curcol);
	}
	else
	{
		s = c[1] / 255.0;
		v = c[2] / 255.0;
	
		p->curcol = mHSVtoRGB_pac(c[0] / 360.0, s, v);

		if(no == 0)
			mHSVPickerSetHue(p->hsv, c[0]);
		else
			mHSVPickerSetSV(p->hsv, s, v);
	}

	//プレビュー

	mColorPreviewSetColor(p->prev, p->curcol);
}


//============================


/** イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	_dialog *p = (_dialog *)wg;
	int id;

	if(ev->type == MEVENT_NOTIFY)
	{
		id = ev->notify.id;
	
		switch(id)
		{
			//RGB/HSV 切り替え
			case WID_CK_RGB:
			case WID_CK_HSV:
				p->coltype = id - WID_CK_RGB;
				_set_color_val(p, _SETCOLORVAL_CHANGE_TYPE);
				break;
			//HSV picker
			case WID_HSV:
				p->curcol = mHSVPickerGetRGBColor(p->hsv);

				mColorPreviewSetColor(p->prev, p->curcol);
				_set_color_val(p, _SETCOLORVAL_CHANGE_HSV);
				break;
			//バー
			case WID_SLIDER1:
			case WID_SLIDER2:
			case WID_SLIDER3:
				if(ev->notify.type == MSLIDERBAR_N_HANDLE
					&& (ev->notify.param2 & MSLIDERBAR_HANDLE_F_CHANGE))
				{
					id -= WID_SLIDER1;
				
					mLineEditSetNum(p->edit[id], ev->notify.param1);
					_change_value(p, id);
				}
				break;
			//エディット
			case WID_EDIT1:
			case WID_EDIT2:
			case WID_EDIT3:
				if(ev->notify.type == MLINEEDIT_N_CHANGE)
				{
					id -= WID_EDIT1;

					mSliderBarSetPos(p->slider[id], mLineEditGetNum(p->edit[id]));
					_change_value(p, id);
				}
				break;
			//パレット
			case WID_PALETTE:
				id = ev->notify.param1;
			
				if(ev->notify.type)
					//登録
					APP_DRAW->col.layercolpal[id] = p->curcol;
				else
				{
					//呼び出し

					p->curcol = APP_DRAW->col.layercolpal[id];

					mColorPreviewSetColor(p->prev, p->curcol);
					mHSVPickerSetRGBColor(p->hsv, p->curcol);

					_set_color_val(p, _SETCOLORVAL_FROM_CURCOL);
				}
				break;
		}
	}

	return mDialogEventHandle_okcancel(wg, ev);
}

/** ウィジェット作成 */

static void _create_widget(_dialog *p)
{
	mWidget *ct,*cth,*ctv;
	int i;

	cth = mContainerCreate(M_WIDGET(p), MCONTAINER_TYPE_HORZ, 0, 15, MLF_EXPAND_W);

	//-------- 左側

	ct = mContainerCreate(cth, MCONTAINER_TYPE_VERT, 0, 10, 0);

	//色プレビュー

	p->prev = mColorPreviewCreate(ct, MCOLORPREVIEW_S_FRAME, 60, 40, 0);

	mColorPreviewSetColor(p->prev, p->curcol);

	//HSV

	p->hsv = mHSVPickerCreate(ct, WID_HSV, 110, 0);

	mHSVPickerSetRGBColor(p->hsv, p->curcol);

	//--------- 右側

	ctv = mContainerCreate(cth, MCONTAINER_TYPE_VERT, 0, 0, MLF_EXPAND_W);

	//パレット

	mLabelCreate(ctv, 0, 0, M_MAKE_DW4(0,0,0,3), M_TR_T(TRID_LAYERDLGS_PALETTE_HELP));

	_palette_new(ctv);

	//RGB/HSV

	ct = mContainerCreate(ctv, MCONTAINER_TYPE_HORZ, 0, 5, 0);

	mWidgetSetMargin_b4(ct, M_MAKE_DW4(0,8,0,8));

	mCheckButtonCreate(ct, WID_CK_RGB, MCHECKBUTTON_S_RADIO, 0, 0, "RGB", FALSE);
	mCheckButtonCreate(ct, WID_CK_HSV, MCHECKBUTTON_S_RADIO, 0, 0, "HSV", TRUE);

	//バー/値

	ct = mContainerCreateGrid(ctv, 3, 5, 4, MLF_EXPAND_W);

	for(i = 0; i < 3; i++)
	{
		p->label[i] = mLabelCreate(ct, 0, MLF_MIDDLE, 0, "R");

		//

		p->slider[i] = mSliderBarCreate(ct, WID_SLIDER1 + i, 0, MLF_EXPAND_W | MLF_MIDDLE, 0);
		p->slider[i]->wg.initW = 160;

		//

		p->edit[i] = mLineEditCreate(ct, WID_EDIT1 + i,
			MLINEEDIT_S_SPIN | MLINEEDIT_S_NOTIFY_CHANGE, MLF_MIDDLE, 0);

		mLineEditSetWidthByLen(p->edit[i], 4);
	}
}

/** 作成 */

static _dialog *_dlg_new(mWindow *owner,uint32_t defcol)
{
	_dialog *p;

	p = (_dialog *)mDialogNew(sizeof(_dialog), owner, MWINDOW_S_DIALOG_NORMAL);
	if(!p) return NULL;

	p->curcol = defcol;
	p->coltype = _COLTYPE_HSV;

	p->wg.event = _event_handle;

	//

	mContainerSetPadding_one(M_CONTAINER(p), 8);

	p->ct.sepW = 15;

	M_TR_G(TRGROUP_DLG_LAYER);

	mWindowSetTitle(M_WINDOW(p), M_TR_T(TRID_LAYERDLGS_TITLE_COLOR));

	//ウィジェット

	_create_widget(p);

	_set_color_val(p, _SETCOLORVAL_CHANGE_TYPE);

	//OK/cancel

	mContainerCreateOkCancelButton(M_WIDGET(p));

	return p;
}

/** 実行 */

mBool LayerColorDlg_run(mWindow *owner,uint32_t *pcol)
{
	_dialog *p;
	int ret;

	p = _dlg_new(owner, *pcol);
	if(!p) return FALSE;

	mWindowMoveResizeShow_hintSize(M_WINDOW(p));

	ret = mDialogRun(M_DIALOG(p), FALSE);

	if(ret)
		*pcol = p->curcol;

	mWidgetDestroy(M_WIDGET(p));

	return ret;
}


//************************************
// パレット
//************************************


#define _PAL_EACHW  16
#define _PAL_EACHH  15
#define _PAL_H_NUM  (LAYERCOLPAL_NUM / 2)
#define _PAL_V_NUM  2


/** イベント */

static int _palette_event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_POINTER
		&& ev->pt.type == MEVENT_POINTER_TYPE_PRESS)
	{
		int x,y,regist;

		//ボタン

		if(ev->pt.btt == M_BTT_LEFT)
			regist = ((ev->pt.state & M_MODS_CTRL) != 0);
		else if(ev->pt.btt == M_BTT_RIGHT)
			regist = TRUE;
		else
			return 1;

		//パレット位置

		x = (ev->pt.x - 1) / _PAL_EACHW;
		y = (ev->pt.y - 1) / _PAL_EACHH;

		if(x < 0) x = 0; else if(x >= _PAL_H_NUM) x = _PAL_H_NUM - 1;
		if(y < 0) y = 0; else if(y >= _PAL_V_NUM) y = _PAL_V_NUM - 1;

		//登録時は更新

		if(regist)
			mWidgetUpdate(wg);

		//通知

		mWidgetAppendEvent_notify(NULL, wg, regist, x + y * _PAL_H_NUM, 0);
	}

	return 1;
}

/** 描画 */

static void _palette_draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	int ix,iy,x,y;
	uint32_t *pcol;

	pcol = APP_DRAW->col.layercolpal;

	for(iy = 0, y = 0; iy < _PAL_V_NUM; iy++, y += _PAL_EACHH)
	{
		for(ix = 0, x = 0; ix < _PAL_H_NUM; ix++, x += _PAL_EACHW, pcol++)
		{
			mPixbufBox(pixbuf, x, y, _PAL_EACHW + 1, _PAL_EACHH + 1, MSYSCOL(WHITE));
			mPixbufBox(pixbuf, x + 1, y + 1, _PAL_EACHW - 1, _PAL_EACHH - 1, 0);

			mPixbufFillBox(pixbuf,
				x + 2, y + 2, _PAL_EACHW - 3, _PAL_EACHH - 3, mRGBtoPix(*pcol));
		}
	}
}

/** 作成 */

mWidget *_palette_new(mWidget *parent)
{
	mWidget *p;

	p = mWidgetNew(0, parent);
	if(!p) return NULL;

	p->id = WID_PALETTE;
	p->hintW = _PAL_EACHW * _PAL_H_NUM + 1;
	p->hintH = _PAL_EACHH * _PAL_V_NUM + 1;
	p->fEventFilter |= MWIDGET_EVENTFILTER_POINTER;

	p->draw = _palette_draw_handle;
	p->event = _palette_event_handle;

	return p;
}
