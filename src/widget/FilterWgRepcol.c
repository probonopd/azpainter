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
 *
 * 描画色置換のウィジェット
 *************************************/

#include "mDef.h"
#include "mContainerDef.h"
#include "mWidget.h"
#include "mContainer.h"
#include "mLabel.h"
#include "mLineEdit.h"
#include "mCheckButton.h"
#include "mColorPreview.h"
#include "mEvent.h"
#include "mColorConv.h"

#include "FilterBar.h"
#include "FilterDefWidget.h"
#include "ColorValue.h"


//----------------

struct _FilterWgRepcol
{
	mWidget wg;
	mContainerData ct;

	int coltype,	//カラータイプ (0:RGB 1:HSV 2:HLS)
		rgb[3];		//RGB値 (8bit)

	mColorPreview *prev[2];
	mLabel *label[3];
	FilterBar *bar[3];
	mLineEdit *edit[3];
};

//----------------

enum
{
	WID_CK_RGB = 100,
	WID_CK_HSV,
	WID_CK_HLS,
	
	WID_BAR_TOP = 110,
	WID_EDIT_TOP = 120
};

//----------------


/** 通知 */

static void _notify(FilterWgRepcol *p,int notify_type)
{
	mWidgetAppendEvent_notify(M_WIDGET(p->wg.toplevel), M_WIDGET(p),
		notify_type, 0, 0);
}

/** カラータイプ変更 */

static void _change_coltype(FilterWgRepcol *p)
{
	char m[2] = {0,0},name[3][3] = {{'R','G','B'},{'H','S','V'},{'H','L','S'}};
	int i,type,max,val[3];
	double d[3];

	type = p->coltype;

	//RGB から各カラーへ

	switch(type)
	{
		//RGB
		case 0:
			val[0] = p->rgb[0];
			val[1] = p->rgb[1];
			val[2] = p->rgb[2];
			break;
		//HSV,HLS
		case 1:
		case 2:
			if(type == 1)
				mRGBtoHSV(p->rgb[0], p->rgb[1], p->rgb[2], d);
			else
				mRGBtoHLS(p->rgb[0], p->rgb[1], p->rgb[2], d);

			val[0] = (int)(d[0] * 360 + 0.5);
			val[1] = (int)(d[1] * 255 + 0.5);
			val[2] = (int)(d[2] * 255 + 0.5);
			break;
	}

	//

	for(i = 0; i < 3; i++)
	{
		max = (type != 0 && i == 0)? 359: 255;
	
		//ラベル

		m[0] = name[type][i];
		mLabelSetText(p->label[i], m);

		//バー

		FilterBar_setStatus(p->bar[i], 0, max, val[i]);

		//エディット

		mLineEditSetNumStatus(p->edit[i], 0, max, 0);
		mLineEditSetNum(p->edit[i], val[i]);
	}
}

/** 色値変更時 */

static void _change_val(FilterWgRepcol *p)
{
	int i,val[3];
	uint32_t col;

	//値取得

	for(i = 0; i < 3; i++)
		val[i] = FilterBar_getPos(p->bar[i]);

	//RGB 取得

	switch(p->coltype)
	{
		//RGB
		case 0:
			p->rgb[0] = val[0];
			p->rgb[1] = val[1];
			p->rgb[2] = val[2];
			break;
		//HSV
		case 1:
			mHSVtoRGB(val[0] / 360.0, val[1] / 255.0, val[2] / 255.0, p->rgb);
			break;
		//HLS
		case 2:
			mHLStoRGB(val[0], val[1] / 255.0, val[2] / 255.0, p->rgb);
			break;
	}

	col = M_RGB(p->rgb[0], p->rgb[1], p->rgb[2]);

	//プレビュー

	mColorPreviewSetColor(p->prev[1], col);
}

/** イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY)
	{
		FilterWgRepcol *p = (FilterWgRepcol *)wg;
		int id = ev->notify.id;

		if(id >= WID_CK_RGB && id <= WID_CK_HLS)
		{
			//カラータイプ変更

			p->coltype = id - WID_CK_RGB;

			_change_coltype(p);

			mWidgetReLayout(wg);
		}
		else if(id >= WID_BAR_TOP && id < WID_BAR_TOP + 3)
		{
			//バー (ボタン離し時に、通知してプレビューさせる)

			mLineEditSetNum(p->edit[id - WID_BAR_TOP], ev->notify.param1);

			_change_val(p);

			if(ev->notify.param2)
				_notify(p, FILTERWGREPCOL_N_UPDATE_PREV);
		}
		else if(id >= WID_EDIT_TOP && id < WID_EDIT_TOP + 3)
		{
			//エディット

			id -= WID_EDIT_TOP;

			FilterBar_setPos(p->bar[id], mLineEditGetNum(p->edit[id]));

			_change_val(p);
			_notify(p, FILTERWGREPCOL_N_CHANGE_EDIT);
		}
	}

	return 1;
}

/** 作成 */

FilterWgRepcol *FilterWgRepcol_new(mWidget *parent,int id,uint32_t drawcol)
{
	FilterWgRepcol *p;
	mWidget *ct,*ct2;
	int i;
	const char *coltype_name[3] = {"RGB", "HSV", "HLS"};

	//作成

	p = (FilterWgRepcol *)mContainerNew(sizeof(FilterWgRepcol), parent);

	mContainerSetType(M_CONTAINER(p), MCONTAINER_TYPE_HORZ, 0);

	p->wg.id = id;
	p->wg.event = _event_handle;
	p->wg.notifyTarget = MWIDGET_NOTIFYTARGET_SELF;
	p->wg.fLayout = MLF_EXPAND_W;
	p->ct.sepW = 12;

	p->coltype = 2;

	p->rgb[0] = M_GET_R(drawcol);
	p->rgb[1] = M_GET_G(drawcol);
	p->rgb[2] = M_GET_B(drawcol);

	//色プレビュー

	ct = mContainerCreate(M_WIDGET(p), MCONTAINER_TYPE_VERT, 0, 10, 0);

	for(i = 0; i < 2; i++)
	{
		p->prev[i] = mColorPreviewCreate(ct, MCOLORPREVIEW_S_FRAME, 40, 30, 0);

		mColorPreviewSetColor(p->prev[i], drawcol);
	}

	//------- カラー

	ct = mContainerCreate(M_WIDGET(p), MCONTAINER_TYPE_VERT, 0, 15, MLF_EXPAND_W);

	//タイプ

	ct2 = mContainerCreate(ct, MCONTAINER_TYPE_HORZ, 0, 4, 0);

	for(i = 0; i < 3; i++)
	{
		mCheckButtonCreate(ct2, WID_CK_RGB + i, MCHECKBUTTON_S_RADIO, 0, 0,
			coltype_name[i], (i == 2));
	}

	//バー

	ct2 = mContainerCreateGrid(ct, 3, 5, 7, MLF_EXPAND_W);

	for(i = 0; i < 3; i++)
	{
		p->label[i] = mLabelCreate(ct2, 0, MLF_MIDDLE, 0, NULL);

		p->bar[i] = FilterBar_new(ct2, WID_BAR_TOP + i, MLF_MIDDLE | MLF_EXPAND_W, 240,
			0, 255, 0, 0);

		p->edit[i] = mLineEditCreate(ct2, WID_EDIT_TOP + i,
			MLINEEDIT_S_SPIN | MLINEEDIT_S_NOTIFY_CHANGE, MLF_MIDDLE, 0);

		mLineEditSetWidthByLen(p->edit[i], 5);
	}

	_change_coltype(p);

	return p;
}

/** 置換え色取得 */

void FilterWgRepcol_getColor(FilterWgRepcol *p,int *dst)
{
	int i;

	for(i = 0; i < 3; i++)
		dst[i] = RGBCONV_8_TO_FIX15(p->rgb[i]);
}
