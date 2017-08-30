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
 * DockOption
 *
 * [テクスチャ] [定規] [入り抜き]
 ********************************************/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "mDef.h"
#include "mStr.h"
#include "mContainerDef.h"
#include "mWidget.h"
#include "mContainer.h"
#include "mEvent.h"
#include "mLabel.h"
#include "mComboBox.h"
#include "mButton.h"
#include "mCheckButton.h"
#include "mListView.h"
#include "mPixbuf.h"
#include "mSysCol.h"
#include "mMenu.h"
#include "mTrans.h"

#include "defDraw.h"

#include "SelMaterial.h"
#include "PrevImage8.h"
#include "ValueBar.h"

#include "draw_main.h"
#include "draw_rule.h"

#include "DockOption_sub.h"

#include "trgroup.h"


//--------------------

//定規タイプイメージ (1bit: 121x21)
static const unsigned char g_img_rule[]={
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x01,
0x01,0x00,0x10,0x21,0x84,0x21,0x84,0x30,0x40,0x80,0x01,0x00,0x10,0x00,0x00,0x01,
0x01,0x00,0x90,0x10,0x42,0x21,0x84,0x50,0x40,0x40,0x01,0x1f,0x10,0x00,0x00,0x01,
0x79,0x9e,0x57,0x08,0x21,0x21,0x84,0x90,0x40,0x20,0xc1,0x60,0x10,0x00,0x00,0x01,
0xfd,0x9e,0x37,0x84,0x10,0x21,0x84,0x10,0x41,0x10,0x21,0x80,0x10,0xf0,0x01,0x01,
0xcd,0x9e,0x17,0x42,0x08,0xff,0xff,0x1f,0x42,0x08,0x11,0x00,0x11,0x0c,0x06,0x01,
0x85,0x82,0x10,0x21,0x84,0x21,0x84,0x10,0x44,0x04,0x09,0x0e,0x12,0x02,0x08,0x01,
0x85,0x82,0x90,0x10,0x42,0x21,0x84,0x10,0x48,0x02,0x09,0x11,0x12,0x01,0x10,0x01,
0x85,0x82,0x50,0x08,0x21,0x21,0x84,0x10,0x50,0x01,0x85,0x20,0x94,0xe0,0x20,0x01,
0x85,0x9e,0x37,0x84,0x10,0x21,0x84,0x10,0xe0,0x00,0x45,0x40,0x54,0x10,0x41,0x01,
0x85,0x9e,0x17,0x42,0x08,0xff,0xff,0xff,0xff,0xff,0x45,0x40,0x54,0x08,0x42,0x01,
0x85,0x9e,0x17,0x21,0x84,0x21,0x84,0x10,0xe0,0x00,0x45,0x40,0x54,0x10,0x41,0x01,
0x85,0x82,0x90,0x10,0x42,0x21,0x84,0x10,0x50,0x01,0x85,0x20,0x94,0xe0,0x20,0x01,
0x85,0x82,0x50,0x08,0x21,0x21,0x84,0x10,0x48,0x02,0x09,0x11,0x12,0x01,0x10,0x01,
0x85,0x82,0x30,0x84,0x10,0x21,0x84,0x10,0x44,0x04,0x09,0x0e,0x12,0x02,0x08,0x01,
0xcd,0x82,0x10,0x42,0x08,0xff,0xff,0x1f,0x42,0x08,0x11,0x00,0x11,0x0c,0x06,0x01,
0xfd,0x82,0x10,0x21,0x84,0x21,0x84,0x10,0x41,0x10,0x21,0x80,0x10,0xf0,0x01,0x01,
0x79,0x82,0x90,0x10,0x42,0x21,0x84,0x90,0x40,0x20,0xc1,0x60,0x10,0x00,0x00,0x01,
0x01,0x00,0x50,0x08,0x21,0x21,0x84,0x50,0x40,0x40,0x01,0x1f,0x10,0x00,0x00,0x01,
0x01,0x00,0x30,0x84,0x10,0x21,0x84,0x30,0x40,0x80,0x01,0x00,0x10,0x00,0x00,0x01,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x01 };

//--------------------


//******************************
// テクスチャ
//******************************


typedef struct
{
	mWidget wg;
	mContainerData ct;

	SelMaterial *sel;
	PrevImage8 *prev;
}_tab_texture;


/** プレビューにイメージをセット */

static void _texture_set_prev(_tab_texture *p)
{
	PrevImage8_setImage(p->prev, APP_DRAW->img8OptTex, FALSE);
}

/** イベント */

static int _texture_event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->notify.id == 100)
	{
		//パスが変更された

		_tab_texture *p = (_tab_texture *)wg;

		SelMaterial_getName(p->sel, &APP_DRAW->strOptTexPath);

		drawTexture_loadOptionTextureImage(APP_DRAW);

		_texture_set_prev(p);

		return TRUE;
	}

	return FALSE;
}

/** 作成 */

mWidget *DockOption_createTab_texture(mWidget *parent)
{
	_tab_texture *p;

	//メインコンテナ

	p = (_tab_texture *)DockOption_createMainContainer(
		sizeof(_tab_texture), parent, _texture_event_handle);

	p->wg.fLayout = MLF_EXPAND_WH;
	p->ct.sepW = 6;

	//素材選択

	p->sel = SelMaterial_new(M_WIDGET(p), 100, SELMATERIAL_TYPE_TEXTURE);

	SelMaterial_setName(p->sel, APP_DRAW->strOptTexPath.buf);

	//プレビュー

	p->prev = PrevImage8_new(M_WIDGET(p), 4, 4, MLF_EXPAND_WH);

	_texture_set_prev(p);

	return (mWidget *)p;
}


//******************************
// 定規
//******************************


typedef struct
{
	mWidget wg;
	mContainerData ct;

	mWidget *wg_type;
	mLabel *lb_type;
}_tab_rule;

enum
{
	WID_RULE_TYPE = 100,
	WID_RULE_BTT_CALL,
	WID_RULE_BTT_RECORD
};


//----------- タイプ選択ウィジェット

/** 描画 */

static void _rule_type_draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	//背景

	mPixbufFillBox(pixbuf, 0, 0, wg->w, wg->h, MSYSCOL(WHITE));

	//選択の背景

	mPixbufFillBox(pixbuf, APP_DRAW->rule.type * 20 + 1, 1, 19, 19, mRGBtoPix(0xff9999));

	//イメージ

	mPixbufDrawBitPattern(pixbuf, 0, 0, g_img_rule, wg->w, wg->h, 0);
}

/** イベント */

static int _rule_type_event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_POINTER
		&& ev->pt.type == MEVENT_POINTER_TYPE_PRESS
		&& ev->pt.btt == M_BTT_LEFT)
	{
		//左ボタン押し時、定規タイプ変更

		int n;

		n = ev->pt.x / 20;
		if(n >= RULE_TYPE_NUM) n = RULE_TYPE_NUM - 1;

		if(n != APP_DRAW->rule.type)
		{
			drawRule_setType(APP_DRAW, n);

			mWidgetAppendEvent_notify(NULL, wg, 0, 0, 0);

			mWidgetUpdate(wg);
		}
	}

	return 1;
}

//-----------

/** 記録メニュー実行 */

static int _rule_run_record_menu(mWidget *bttwg)
{
	mMenu *menu;
	int i,n;
	RuleRecord *rec;
	mStr str = MSTR_INIT;
	mMenuItemInfo *pi;
	mBox box;

	M_TR_G(TRGROUP_DOCKOPT_RULE);

	//メニュー

	menu = mMenuNew();

	rec = APP_DRAW->rule.record;

	for(i = 0; i < RULE_RECORD_NUM; i++, rec++)
	{
		mStrSetFormat(&str, "[%d] ", i + 1);

		if(rec->type == 0)
			mStrAppendText(&str, "---");
		else
		{
			mStrAppendFormat(&str, "%s : ", M_TR_T(rec->type));
		
			switch(rec->type)
			{
				//角度
				case RULE_TYPE_PARALLEL_LINE:
				case RULE_TYPE_GRID_LINE:
					n = round(-rec->d[0] * 18000 / M_MATH_PI);
					mStrAppendFormat(&str, "angle %d.%d", n / 100, abs(n % 100));
					break;
				//位置
				case RULE_TYPE_CONC_LINE:
				case RULE_TYPE_CIRCLE:
					mStrAppendFormat(&str, "(%d,%d)", (int)rec->d[0], (int)rec->d[1]);
					break;
				//楕円
				case RULE_TYPE_ELLIPSE:
					n = round(rec->d[3] * 18000 / M_MATH_PI);
					mStrAppendFormat(&str, "(%d,%d) angle %d.%d",
						(int)rec->d[0], (int)rec->d[1], n / 100, abs(n % 100));
					break;
			}
		}

		mMenuAddText_copy(menu, i, str.buf);
	}

	mWidgetGetRootBox(bttwg, &box);

	pi = mMenuPopup(menu, NULL, box.x, box.y + box.h, 0);

	n = (pi)? pi->id: -1;

	mMenuDestroy(menu);

	mStrFree(&str);

	return n;
}

/** イベント */

static int _rule_event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY)
	{
		_tab_rule *p = (_tab_rule *)wg;
		int no;
	
		switch(ev->notify.id)
		{
			//タイプ変更
			case WID_RULE_TYPE:
				mLabelSetText(p->lb_type, M_TR_T2(TRGROUP_DOCKOPT_RULE, APP_DRAW->rule.type));
				break;
			//呼び出し
			case WID_RULE_BTT_CALL:
				no = _rule_run_record_menu(ev->notify.widgetFrom);
				if(no != -1)
				{
					drawRule_callRecord(APP_DRAW, no);

					//タイプ選択
					mWidgetUpdate(p->wg_type);
					mLabelSetText(p->lb_type, M_TR_T2(TRGROUP_DOCKOPT_RULE, APP_DRAW->rule.type));
				}
				break;
			//記録
			case WID_RULE_BTT_RECORD:
				no = _rule_run_record_menu(ev->notify.widgetFrom);
				if(no != -1)
					drawRule_setRecord(APP_DRAW, no);
				break;
		}
	}

	return 1;
}

/** 定規ウィジェット作成 */

mWidget *DockOption_createTab_rule(mWidget *parent)
{
	_tab_rule *p;
	mWidget *ct,*wg;

	M_TR_G(TRGROUP_DOCKOPT_RULE);

	//メインコンテナ

	p = (_tab_rule *)DockOption_createMainContainer(
		sizeof(_tab_rule), parent, _rule_event_handle);

	p->ct.sepW = 7;

	//タイプ

	p->wg_type = wg = mWidgetNew(0, M_WIDGET(p));

	wg->id = WID_RULE_TYPE;
	wg->hintW = 121;
	wg->hintH = 21;
	wg->draw = _rule_type_draw_handle;
	wg->event = _rule_type_event_handle;
	wg->fEventFilter |= MWIDGET_EVENTFILTER_POINTER;

	//タイプ名

	p->lb_type = mLabelCreate(M_WIDGET(p), MLABEL_S_BORDER, MLF_EXPAND_W, 0, 0);

	mLabelSetText(p->lb_type, M_TR_T(APP_DRAW->rule.type));

	//ボタン

	ct = mContainerCreate(M_WIDGET(p), MCONTAINER_TYPE_HORZ, 0, 4, 0);

	mButtonCreate(ct, WID_RULE_BTT_CALL, 0, 0, 0, M_TR_T(100));
	mButtonCreate(ct, WID_RULE_BTT_RECORD, 0, 0, 0, M_TR_T(101));

	return (mWidget *)p;
}


//******************************
// 入り抜き
//******************************


typedef struct
{
	mWidget wg;
	mContainerData ct;

	ValueBar *bar[2];
}_tab_headtail;

enum
{
	WID_HEADTAIL_LINE = 100,
	WID_HEADTAIL_BEZIER,
	WID_HEADTAIL_BAR_HEAD,
	WID_HEADTAIL_BAR_TAIL,
	WID_HEADTAIL_BTT_CALL,
	WID_HEADTAIL_BTT_RECORD
};

enum
{
	TRID_HEADTAIL_LINE,
	TRID_HEADTAIL_BEZIER,
	TRID_HEADTAIL_HEAD,
	TRID_HEADTAIL_TAIL,
	TRID_HEADTAIL_CALL,
	TRID_HEADTAIL_RECORD
};


/** 呼出/記録メニュー実行 */

static int _headtail_run_record_menu(mWidget *bttwg)
{
	mMenu *menu;
	mMenuItemInfo *mi;
	int i,n1,n2;
	uint32_t *prec;
	mBox box;
	char m[32];

	menu = mMenuNew();

	prec = APP_DRAW->headtail.record;

	for(i = 0; i < HEADTAIL_RECORD_NUM; i++, prec++)
	{
		n1 = *prec >> 16;
		n2 = *prec & 0xffff;
	
		snprintf(m, 32, "[%d] %d.%d : %d.%d", i + 1,
			n1 / 10, n1 % 10, n2 / 10, n2 % 10);
		
		mMenuAddText_copy(menu, i, m);
	}

	mWidgetGetRootBox(bttwg, &box);

	mi = mMenuPopup(menu, NULL, box.x, box.y + box.h, 0);

	n1 = (mi)? mi->id: -1;

	mMenuDestroy(menu);

	return n1;
}

/** バーの値セット */

static void _headtail_set_bar_value(_tab_headtail *p)
{
	uint32_t val;

	val = APP_DRAW->headtail.curval[APP_DRAW->headtail.selno];

	ValueBar_setPos(p->bar[0], val >> 16);
	ValueBar_setPos(p->bar[1], val & 0xffff);
}

/** イベント */

static int _headtail_event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY)
	{
		_tab_headtail *p = (_tab_headtail *)wg;
		int no;
	
		switch(ev->notify.id)
		{
			//タイプ選択
			case WID_HEADTAIL_LINE:
			case WID_HEADTAIL_BEZIER:
				APP_DRAW->headtail.selno = ev->notify.id - WID_HEADTAIL_LINE;

				_headtail_set_bar_value(p);
				break;
			//バー:入り
			case WID_HEADTAIL_BAR_HEAD:
				if(ev->notify.param2)
				{
					APP_DRAW->headtail.curval[APP_DRAW->headtail.selno] &= 0xffff;
					APP_DRAW->headtail.curval[APP_DRAW->headtail.selno] |= (uint32_t)ev->notify.param1 << 16;
				}
				break;
			//バー:抜き
			case WID_HEADTAIL_BAR_TAIL:
				if(ev->notify.param2)
				{
					APP_DRAW->headtail.curval[APP_DRAW->headtail.selno] &= 0xffff0000;
					APP_DRAW->headtail.curval[APP_DRAW->headtail.selno] |= ev->notify.param1;
				}
				break;
			//呼び出し
			case WID_HEADTAIL_BTT_CALL:
				no = _headtail_run_record_menu(ev->notify.widgetFrom);
				if(no != -1)
				{
					APP_DRAW->headtail.curval[APP_DRAW->headtail.selno] =
						APP_DRAW->headtail.record[no];

					_headtail_set_bar_value(p);
				}
				break;
			//記録
			case WID_HEADTAIL_BTT_RECORD:
				no = _headtail_run_record_menu(ev->notify.widgetFrom);
				if(no != -1)
				{
					APP_DRAW->headtail.record[no] =
						APP_DRAW->headtail.curval[APP_DRAW->headtail.selno];
				}
				break;
		}
	}

	return 1;
}

/** 入り抜きウィジェット作成 */

mWidget *DockOption_createTab_headtail(mWidget *parent)
{
	_tab_headtail *p;
	mWidget *ct;
	int i;

	M_TR_G(TRGROUP_DOCKOPT_HEADTAIL);

	//メインコンテナ

	p = (_tab_headtail *)DockOption_createMainContainer(
		sizeof(_tab_headtail), parent, _headtail_event_handle);

	p->ct.sepW = 7;

	//タイプ選択

	ct = mContainerCreate(M_WIDGET(p), MCONTAINER_TYPE_HORZ, 0, 5, 0);

	for(i = 0; i < 2; i++)
	{
		mCheckButtonCreate(ct, WID_HEADTAIL_LINE + i, MCHECKBUTTON_S_RADIO, 0, 0,
			M_TR_T(TRID_HEADTAIL_LINE + i), (APP_DRAW->headtail.selno == i));
	}

	//バー

	ct = mContainerCreateGrid(M_WIDGET(p), 2, 4, 5, MLF_EXPAND_W);

	for(i = 0; i < 2; i++)
	{
		mLabelCreate(ct, 0, MLF_RIGHT | MLF_MIDDLE, 0, M_TR_T(TRID_HEADTAIL_HEAD + i));

		p->bar[i] = ValueBar_new(ct, WID_HEADTAIL_BAR_HEAD + i, MLF_EXPAND_W | MLF_MIDDLE,
			1, 0, 1000, 0);
	}

	_headtail_set_bar_value(p);

	//ボタン

	ct = mContainerCreate(M_WIDGET(p), MCONTAINER_TYPE_HORZ, 0, 4, MLF_RIGHT);
	ct->margin.top = 4;

	for(i = 0; i < 2; i++)
		mButtonCreate(ct, WID_HEADTAIL_BTT_CALL + i, 0, 0, 0, M_TR_T(TRID_HEADTAIL_CALL + i));

	return (mWidget *)p;
}
