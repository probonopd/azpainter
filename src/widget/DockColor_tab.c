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
 * DockColor のタブ内容
 *****************************************/

#include <stdlib.h>

#include "mDef.h"
#include "mContainerDef.h"
#include "mWidget.h"
#include "mContainer.h"
#include "mLabel.h"
#include "mButton.h"
#include "mHSVPicker.h"
#include "mEvent.h"
#include "mSysDialog.h"
#include "mStr.h"
#include "mTrans.h"
#include "mColorConv.h"

#include "defDraw.h"
#include "defWidgets.h"

#include "DockObject.h"
#include "ValueBar.h"

#include "draw_main.h"

#include "trgroup.h"


//----------------

typedef struct
{
	mWidget wg;
	mContainerData ct;

	ValueBar *bar[3];
}_tabct_rgb_hsv;

enum
{
	TRID_RGB_INPUT,
	TRID_RGB_INPUT_MES
};

#define _NOTIFY_PARAM_RGB  -1

//----------------

/*
 * 通知の param1 は、
 * RGB値なら負の値、
 * HSV値なら 32bit にパックする (9:8:8bit - 360:255:255)。
 *
 * 描画色変更時のハンドラは、
 * hsvcol : 負の値でRGB描画色、それ以外で HSV パック値
 */



//*****************************
// 共通関数
//*****************************


/** タブのメインコンテナ作成
 *
 * MLF_EXPAND_W、垂直コンテナ、境界余白は 5 */

static mContainer *_create_main_container(int size,mWidget *parent,
	int (*event)(mWidget *,mEvent *))
{
	mContainer *ct;

	ct = mContainerNew(size, parent);

	ct->wg.fLayout = MLF_EXPAND_W;
	ct->wg.notifyTargetInterrupt = MWIDGET_NOTIFYTARGET_INT_SELF; //子の通知は自身で受け取る
	ct->wg.event = event;
	ct->ct.sepW = 5;

	return ct;
}

/** HSV バーに値セット */

static void _set_hsv_bar(ValueBar **bar,int hsvcol)
{
	int i;
	double d[3];

	if(hsvcol < 0)
	{
		//RGB
		
		mRGBtoHSV_pac(APP_DRAW->col.drawcol, d);

		for(i = 0; i < 3; i++)
			ValueBar_setPos(bar[i], (i == 0)? (int)(d[0] * 360 + 0.5): (int)(d[i] * 255 + 0.5));
	}
	else
	{
		//HSV
		
		ValueBar_setPos(bar[0], hsvcol >> 16);
		ValueBar_setPos(bar[1], (hsvcol >> 8) & 255);
		ValueBar_setPos(bar[2], hsvcol & 255);
	}
}


/***************************************
 * RGB
 ***************************************/


/** 数値入力 */

static void _rgb_input(_tabct_rgb_hsv *p)
{
	mStr str = MSTR_INIT;
	int i,c[3] = {0,0,0};

	M_TR_G(TRGROUP_DOCK_COLOR);

	if(!mSysDlgInputText(DockObject_getOwnerWindow(APP_WIDGETS->dockobj[DOCKWIDGET_COLOR]),
			M_TR_T(TRID_RGB_INPUT), M_TR_T(TRID_RGB_INPUT_MES),
			&str, MSYSDLG_INPUTTEXT_F_NOEMPTY))
		return;

	mStrToIntArray_range(&str, c, 3, 0, 0, 255);

	mStrFree(&str);

	//セット

	for(i = 0; i < 3; i++)
		ValueBar_setPos(p->bar[i], c[i]);

	drawColor_setDrawColor(M_RGB(c[0], c[1], c[2]));
	
	mWidgetAppendEvent_notify(MWIDGET_NOTIFYWIDGET_RAW, M_WIDGET(p), 0, _NOTIFY_PARAM_RGB, 0);
}

/** イベント */

static int _rgb_event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY)
	{
		if(ev->notify.id == 100)
			//数値入力
			_rgb_input((_tabct_rgb_hsv *)wg);
		else
		{
			//バーの値変更時

			int i,c[3];

			for(i = 0; i < 3; i++)
				c[i] = ValueBar_getPos(((_tabct_rgb_hsv *)wg)->bar[i]);

			drawColor_setDrawColor(M_RGB(c[0], c[1], c[2]));

			mWidgetAppendEvent_notify(MWIDGET_NOTIFYWIDGET_RAW, wg, 0, _NOTIFY_PARAM_RGB, 0);
		}
	}

	return 1;
}

/** 描画色変更時 */

static void _rgb_change_drawcol(mWidget *wg,int hsvcol)
{
	_tabct_rgb_hsv *p = (_tabct_rgb_hsv *)wg;
	int i,c[3];

	drawColor_getDrawColor_rgb(c);

	for(i = 0; i < 3; i++)
		ValueBar_setPos(p->bar[i], c[i]);
}

/** タブ内容作成 */

mWidget *DockColor_tabRGB_create(mWidget *parent,int id)
{
	_tabct_rgb_hsv *p;
	mWidget *ct;
	int i,c[3];
	char m[2] = {0,0}, name[3] = {'R','G','B'};

	p = (_tabct_rgb_hsv *)_create_main_container(sizeof(_tabct_rgb_hsv), parent, _rgb_event_handle);

	p->wg.id = id;
	p->wg.param = (intptr_t)_rgb_change_drawcol;

	//バー

	ct = mContainerCreateGrid(M_WIDGET(p), 2, 5, 5, MLF_EXPAND_W);

	drawColor_getDrawColor_rgb(c);

	for(i = 0; i < 3; i++)
	{
		//ラベル

		m[0] = name[i];
		mLabelCreate(ct, 0, MLF_MIDDLE, 0, m);

		//バー

		p->bar[i] = ValueBar_new(ct, 0, MLF_EXPAND_W | MLF_MIDDLE,
			0, 0, 255, c[i]);
	}

	//ボタン

	mButtonCreate(M_WIDGET(p), 100, 0, MLF_RIGHT,
		0, M_TR_T2(TRGROUP_DOCK_COLOR, TRID_RGB_INPUT));

	return (mWidget *)p;
}



/***************************************
 * HSV バー
 ***************************************/


/** イベント */

static int _hsv_event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY)
	{
		//バーの値変更時

		int i,c[3];

		for(i = 0; i < 3; i++)
			c[i] = ValueBar_getPos(((_tabct_rgb_hsv *)wg)->bar[i]);

		drawColor_setDrawColor(mHSVtoRGB_pac(c[0] / 360.0, c[1] / 255.0, c[2] / 255.0));

		mWidgetAppendEvent_notify(MWIDGET_NOTIFYWIDGET_RAW, wg, 0,
			(c[0] << 16) | (c[1] << 8) | c[2], 0);
	}

	return 1;
}

/** 描画色変更時 */

static void _hsv_change_drawcol(mWidget *wg,int hsvcol)
{
	_tabct_rgb_hsv *p = (_tabct_rgb_hsv *)wg;

	_set_hsv_bar(p->bar, hsvcol);
}

/** タブ内容作成 */

mWidget *DockColor_tabHSV_create(mWidget *parent,int id)
{
	_tabct_rgb_hsv *p;
	char m[2] = {0,0}, name[3] = {'H','S','V'};
	int i;
	double d[3];

	p = (_tabct_rgb_hsv *)_create_main_container(sizeof(_tabct_rgb_hsv), parent, _hsv_event_handle);

	p->wg.id = id;
	p->wg.param = (intptr_t)_hsv_change_drawcol;

	mContainerSetTypeGrid(M_CONTAINER(p), 2, 5, 5);

	//バー

	mRGBtoHSV_pac(APP_DRAW->col.drawcol, d);

	for(i = 0; i < 3; i++)
	{
		//ラベル

		m[0] = name[i];
		mLabelCreate(M_WIDGET(p), 0, MLF_MIDDLE, 0, m);

		//バー

		p->bar[i] = ValueBar_new(M_WIDGET(p), 0, MLF_EXPAND_W | MLF_MIDDLE,
			0, 0, (i == 0)? 359: 255,
			(i == 0)? (int)(d[0] * 360 + 0.5): (int)(d[i] * 255 + 0.5));
	}

	return (mWidget *)p;
}


/***************************************
 * HSV カラーマップ
 ***************************************/


typedef struct
{
	mWidget wg;
	mContainerData ct;

	mHSVPicker *pic;
	ValueBar *bar[3];
}_tabct_hsvmap;


/** イベント */

static int _hsvmap_event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY)
	{
		_tabct_hsvmap *p = (_tabct_hsvmap *)wg;
		int col = -1,i,c[3];
		double d[3];

		//色取得

		if(ev->notify.id == 100)
		{
			//HSV ピッカー

			if(ev->notify.type == MHSVPICKER_N_CHANGE_H
				|| ev->notify.type == MHSVPICKER_N_CHANGE_SV)
			{
				col = ev->notify.param1;

				//HSV バー

				mHSVPickerGetHSVColor(p->pic, d);

				c[0] = (int)(d[0] * 360 + 0.5);
				c[1] = (int)(d[1] * 255 + 0.5);
				c[2] = (int)(d[2] * 255 + 0.5);

				for(i = 0; i < 3; i++)
					ValueBar_setPos(p->bar[i], c[i]);
			}
		}
		else
		{
			//バーの値変更時


			for(i = 0; i < 3; i++)
				c[i] = ValueBar_getPos(p->bar[i]);

			d[0] = c[0] / 360.0;
			d[1] = c[1] / 255.0;
			d[2] = c[2] / 255.0;

			col = mHSVtoRGB_pac(d[0], d[1], d[2]);

			mHSVPickerSetHSVColor(p->pic, d[0], d[1], d[2]);
		}

		//更新

		if(col != -1)
		{
			drawColor_setDrawColor(col);

			mWidgetAppendEvent_notify(MWIDGET_NOTIFYWIDGET_RAW, wg, 0,
				(c[0] << 16) | (c[1] << 8) | c[2], 0);
		}
	}

	return 1;
}

/** 描画色変更時 */

static void _hsvmap_change_drawcol(mWidget *wg,int hsvcol)
{
	_tabct_hsvmap *p = (_tabct_hsvmap *)wg;

	_set_hsv_bar(p->bar, hsvcol);

	if(hsvcol < 0)
		mHSVPickerSetRGBColor(p->pic, APP_DRAW->col.drawcol);
	else
	{
		mHSVPickerSetHSVColor(p->pic,
			(hsvcol >> 16) / 360.0,
			((hsvcol >> 8) & 255) / 255.0,
			(hsvcol & 255) / 255.0);
	}
}

/** タブ内容作成 */

mWidget *DockColor_tabHSVMap_create(mWidget *parent,int id)
{
	_tabct_hsvmap *p;
	mWidget *ct;
	char m[2] = {0,0}, name[3] = {'H','S','V'};
	int i;
	double d[3];

	p = (_tabct_hsvmap *)_create_main_container(sizeof(_tabct_hsvmap), parent, _hsvmap_event_handle);

	p->wg.id = id;
	p->wg.param = (intptr_t)_hsvmap_change_drawcol;

	mContainerSetType(M_CONTAINER(p), MCONTAINER_TYPE_HORZ, 0);
	p->ct.sepW = 10;

	//バー

	ct = mContainerCreateGrid(M_WIDGET(p), 2, 5, 7, MLF_EXPAND_W);

	mRGBtoHSV_pac(APP_DRAW->col.drawcol, d);

	for(i = 0; i < 3; i++)
	{
		//ラベル

		m[0] = name[i];
		mLabelCreate(ct, 0, MLF_MIDDLE, 0, m);

		//バー

		p->bar[i] = ValueBar_new(ct, 0, MLF_EXPAND_W | MLF_MIDDLE,
			0, 0, (i == 0)? 359: 255,
			(i == 0)? (int)(d[0] * 360 + 0.5): (int)(d[i] * 255 + 0.5));
	}

	//HSV ピッカー

	p->pic = mHSVPickerCreate(M_WIDGET(p), 100, 90, 0);

	mHSVPickerSetRGBColor(p->pic, APP_DRAW->col.drawcol);

	return (mWidget *)p;
}

/** テーマ変更時 */

void DockColor_tabHSVMap_changeTheme(mWidget *wg)
{
	mHSVPickerRedraw(((_tabct_hsvmap *)wg)->pic);
}
