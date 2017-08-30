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

/************************************************
 * DockOption
 *
 * [dock]オプションのタブ内容処理 <ツール関連>
 ************************************************/

#include "mDef.h"
#include "mContainerDef.h"
#include "mWidget.h"
#include "mContainer.h"
#include "mLabel.h"
#include "mButton.h"
#include "mCheckButton.h"
#include "mComboBox.h"
#include "mContainerView.h"
#include "mEvent.h"
#include "mTrans.h"
#include "mStr.h"

#include "defMacros.h"
#include "defDraw.h"
#include "defConfig.h"
#include "defPixelMode.h"
#include "macroToolOpt.h"

#include "ValueBar.h"
#include "PrevTileImage.h"
#include "DockOption_sub.h"
#include "FileDialog.h"

#include "draw_main.h"

#include "trgroup.h"
#include "trid_word.h"




/******************************
 * 共通関数
 ******************************/


/** メインコンテナ作成
 *
 * MLF_EXPAND_W、垂直コンテナ、境界余白は 5 */

mWidget *DockOption_createMainContainer(int size,mWidget *parent,
	int (*event)(mWidget *,mEvent *))
{
	mContainer *ct;

	ct = mContainerNew(size, parent);

	mContainerSetPadding_b4(ct, M_MAKE_DW4(3,2,3,0));

	ct->wg.fLayout = MLF_EXPAND_W;
	ct->wg.notifyTarget = MWIDGET_NOTIFYTARGET_SELF;
	ct->wg.event = event;
	ct->ct.sepW = 5;

	return (mWidget *)ct;
}

/** 濃度のラベルとバーを作成 */

ValueBar *DockOption_createDensityBar(mWidget *parent,int id,int val)
{
	mLabelCreate(parent, 0, MLF_MIDDLE | MLF_RIGHT, 0, M_TR_T2(TRGROUP_WORD, TRID_WORD_DENSITY));

	return ValueBar_new(parent, id, MLF_EXPAND_W | MLF_MIDDLE, 0, 1, 100, val);
}

/** ラベルとバーを作成 (桁なし)
 *
 * @param label_id 正の値で現在の翻訳グループ。負の値で TRGROUP_WORD。 */

ValueBar *DockOption_createBar(mWidget *parent,int wid,int label_id,int min,int max,int val)
{
	mLabelCreate(parent, 0, MLF_MIDDLE | MLF_RIGHT, 0,
		(label_id < 0)? M_TR_T2(TRGROUP_WORD, -label_id): M_TR_T(label_id));

	return ValueBar_new(parent, wid, MLF_EXPAND_W | MLF_MIDDLE, 0, min, max, val);
}

/** 塗りのラベルとコンボボックスを作成
 *
 * [!] 翻訳グループのカレントが変更されるので注意
 *
 * @return コンボボックスを返す */

mComboBox *DockOption_createPixelModeCombo(mWidget *parent,int id,uint8_t *dat,int sel)
{
	mComboBox *cb;
	int i,flag = 0;

	mLabelCreate(parent, 0, MLF_MIDDLE | MLF_RIGHT, 0, M_TR_T2(TRGROUP_WORD, TRID_WORD_PIXELMODE));

	//コンボボックス

	cb = mComboBoxCreate(parent, id, 0, MLF_EXPAND_W, 0);

	M_TR_G(TRGROUP_PIXELMODE_TOOL);

	for(i = 0; *dat != 255; dat++, i++)
	{
		mComboBoxAddItem(cb, M_TR_T(*dat), *dat);

		if(*dat == sel)
		{
			mComboBoxSetSel_index(cb, i);
			flag = 1;
		}
	}

	if(!flag)
		mComboBoxSetSel_index(cb, 0);

	return cb;
}

/** ラベルとコンボボックスを作成
 *
 * @param trid 正の値でカレントグループ。負の値で TRGROUP_WORD のグループ */

mComboBox *DockOption_createComboBox(mWidget *parent,int cbid,int trid)
{
	mLabelCreate(parent, 0, MLF_MIDDLE | MLF_RIGHT, 0,
		(trid < 0)? M_TR_T2(TRGROUP_WORD, -trid): M_TR_T(trid));

	return mComboBoxCreate(parent, cbid, 0, MLF_EXPAND_W, 0);
}


/*****************************
 * 図形塗り/消し
 *****************************/
/*
 * mWidget::param が 1 なら図形消し
 */


enum
{
	WID_FILLPL_OPACITY = 100,
	WID_FILLPL_PIXMODE,
	WID_FILLPL_ANTIALIAS
};


/** イベント */

static int _fillpoly_event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY)
	{
		uint16_t *pd;

		pd = (wg->param)? &APP_DRAW->tool.opt_fillpoly_erase: &APP_DRAW->tool.opt_fillpoly;
	
		switch(ev->notify.id)
		{
			//濃度
			case WID_FILLPL_OPACITY:
				if(ev->notify.param2)
					FILLPOLY_SET_OPACITY(pd, ev->notify.param1);
				break;
			//塗り
			case WID_FILLPL_PIXMODE:
				if(ev->notify.type == MCOMBOBOX_N_CHANGESEL)
					FILLPOLY_SET_PIXMODE(pd, ev->notify.param2);
				break;
			//アンチエイリアス
			case WID_FILLPL_ANTIALIAS:
				FILLPOLY_TOGGLE_ANTIALIAS(pd);
				break;
		}
	}

	return 1;
}

/** 作成 */

static mWidget *_create_tab_fill_polygon(mWidget *parent)
{
	mWidget *p,*ct;
	uint16_t val;
	uint8_t pixmode[] = {
		PIXELMODE_TOOL_BLEND, PIXELMODE_TOOL_COMPARE_A,
		PIXELMODE_TOOL_OVERWRITE, 255
	};

	p = DockOption_createMainContainer(0, parent, _fillpoly_event_handle);

	p->param = (APP_DRAW->tool.no == TOOL_FILL_POLYGON_ERASE);

	val = (p->param)? APP_DRAW->tool.opt_fillpoly_erase: APP_DRAW->tool.opt_fillpoly;

	//-------

	ct = mContainerCreateGrid(p, 2, 4, 5, MLF_EXPAND_W);

	//濃度

	DockOption_createDensityBar(ct, WID_FILLPL_OPACITY, FILLPOLY_GET_OPACITY(val));

	//塗り

	if(!p->param)
	{
		DockOption_createPixelModeCombo(ct,
			WID_FILLPL_PIXMODE, pixmode, FILLPOLY_GET_PIXMODE(val));
	}

	//アンチエイリアス

	mWidgetNew(0, ct);

	mCheckButtonCreate(ct, WID_FILLPL_ANTIALIAS, 0, 0, 0,
		M_TR_T2(TRGROUP_WORD, TRID_WORD_ANTIALIAS),
		FILLPOLY_IS_ANTIALIAS(val));

	return p;
}


/******************************
 * 塗りつぶし
 ******************************/


enum
{
	WID_FILL_TYPE = 100,
	WID_FILL_COLOR_DIFF,
	WID_FILL_OPACITY,
	WID_FILL_PIXMODE,
	WID_FILL_DISABLE_REF
};


/** イベント */

static int _fill_event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY)
	{
		uint32_t *pd = &APP_DRAW->tool.opt_fill;

		switch(ev->notify.id)
		{
			//判定
			case WID_FILL_TYPE:
				if(ev->notify.type == MCOMBOBOX_N_CHANGESEL)
					FILL_SET_TYPE(pd, ev->notify.param2);
				break;
			//許容色差
			case WID_FILL_COLOR_DIFF:
				if(ev->notify.param2)
					FILL_SET_COLOR_DIFF(pd, ev->notify.param1);
				break;
			//濃度
			case WID_FILL_OPACITY:
				if(ev->notify.param2)
					FILL_SET_OPACITY(pd, ev->notify.param1);
				break;
			//塗り
			case WID_FILL_PIXMODE:
				if(ev->notify.type == MCOMBOBOX_N_CHANGESEL)
					FILL_SET_PIXMODE(pd, ev->notify.param2);
				break;
			//判定元レイヤ無効
			case WID_FILL_DISABLE_REF:
				FILL_TOGGLE_DISABLE_REF(pd);
				break;
		}
	}

	return 1;
}

/** 作成 */

static mWidget *_create_tab_fill(mWidget *parent)
{
	mWidget *p,*ct;
	mComboBox *cb;
	uint32_t val;
	uint8_t pixmode[] = {
		PIXELMODE_TOOL_BLEND, PIXELMODE_TOOL_COMPARE_A,
		PIXELMODE_TOOL_OVERWRITE, PIXELMODE_TOOL_ERASE, 255
	};

	p = DockOption_createMainContainer(0, parent, _fill_event_handle);

	val = APP_DRAW->tool.opt_fill;

	M_TR_G(TRGROUP_DOCKOPT_FILL);

	//---------

	ct = mContainerCreateGrid(p, 2, 4, 5, MLF_EXPAND_W);

	//判定

	cb = DockOption_createComboBox(ct, WID_FILL_TYPE, 0);

	mComboBoxAddTrItems(cb, 4, 100, 0);
	mComboBoxSetSel_index(cb, FILL_GET_TYPE(val));

	//色差

	DockOption_createBar(ct, WID_FILL_COLOR_DIFF, 1, 0, 100, FILL_GET_COLOR_DIFF(val));

	//濃度

	DockOption_createDensityBar(ct, WID_FILL_OPACITY, FILL_GET_OPACITY(val));

	//塗り

	DockOption_createPixelModeCombo(ct, WID_FILL_PIXMODE, pixmode, FILL_GET_PIXMODE(val));

	//判定元レイヤ無効

	mCheckButtonCreate(p, WID_FILL_DISABLE_REF, 0, 0, 0,
		M_TR_T2(TRGROUP_DOCKOPT_FILL, 2), FILL_IS_DISABLE_REF(val)); 

	return p;
}


/******************************
 * 自動選択
 ******************************/


enum
{
	WID_MAGICWAND_TYPE = 100,
	WID_MAGICWAND_COLOR_DIFF,
	WID_MAGICWAND_DISABLE_REF
};


/** イベント */

static int _magicwand_event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY)
	{
		uint16_t *pd = &APP_DRAW->tool.opt_magicwand;

		switch(ev->notify.id)
		{
			//判定
			case WID_MAGICWAND_TYPE:
				if(ev->notify.type == MCOMBOBOX_N_CHANGESEL)
					MAGICWAND_SET_TYPE(pd, ev->notify.param2);
				break;
			//許容色差
			case WID_MAGICWAND_COLOR_DIFF:
				if(ev->notify.param2)
					MAGICWAND_SET_COLOR_DIFF(pd, ev->notify.param1);
				break;
			//判定元レイヤ無効
			case WID_MAGICWAND_DISABLE_REF:
				MAGICWAND_TOGGLE_DISABLE_REF(pd);
				break;
		}
	}

	return 1;
}

/** 作成 */

static mWidget *_create_tab_magicwand(mWidget *parent)
{
	mWidget *p,*ct;
	mComboBox *cb;
	uint32_t val;

	p = DockOption_createMainContainer(0, parent, _magicwand_event_handle);

	val = APP_DRAW->tool.opt_magicwand;

	M_TR_G(TRGROUP_DOCKOPT_FILL);

	//---------

	ct = mContainerCreateGrid(p, 2, 4, 6, MLF_EXPAND_W);

	//判定

	cb = DockOption_createComboBox(ct, WID_MAGICWAND_TYPE, 0);

	mComboBoxAddTrItems(cb, 4, 100, 0);
	mComboBoxSetSel_index(cb, MAGICWAND_GET_TYPE(val));

	//色差

	DockOption_createBar(ct, WID_MAGICWAND_COLOR_DIFF, 1, 0, 100, MAGICWAND_GET_COLOR_DIFF(val));

	//判定元レイヤ無効

	mCheckButtonCreate(p, WID_MAGICWAND_DISABLE_REF, 0, 0, 0,
		M_TR_T(2), MAGICWAND_IS_DISABLE_REF(val)); 

	return p;
}


/******************************
 * 移動
 ******************************/


enum
{
	WID_MOVE_TARGET = 100
};


/** イベント */

static int _move_event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY
		&& ev->notify.id >= WID_MOVE_TARGET && ev->notify.id < WID_MOVE_TARGET + 3)
		APP_DRAW->tool.opt_move = ev->notify.id - WID_MOVE_TARGET;

	return 1;
}

/** 作成 */

static mWidget *_create_tab_move(mWidget *parent)
{
	mWidget *p;
	int i,val;

	p = DockOption_createMainContainer(0, parent, _move_event_handle);

	val = APP_DRAW->tool.opt_move;

	M_TR_G(TRGROUP_DOCKOPT_MOVE);

	//対象

	for(i = 0; i < 3; i++)
	{
		mCheckButtonCreate(p, WID_MOVE_TARGET + i, MCHECKBUTTON_S_RADIO,
			0, 0, M_TR_T(i), (val == i));
	}

	return p;
}


/******************************
 * スタンプ
 ******************************/


typedef struct
{
	mWidget wg;
	mContainerData ct;

	PrevTileImage *prev;
}_tab_stamp;

enum
{
	WID_STAMP_CLEAR = 100,
	WID_STAMP_LOAD,
	WID_STAMP_OVERWRITE,
	WID_STAMP_LEFTTOP
};

enum
{
	TRID_STAMP_CLEAR,
	TRID_STAMP_LOAD,
	TRID_STAMP_OVERWRITE,
	TRID_STAMP_LEFTTOP
};


/** イメージ変更時
 *
 * 外部からのイメージ変更用 */

void DockOption_toolStamp_changeImage(mWidget *wg)
{
	PrevTileImage_setImage(((_tab_stamp *)wg)->prev,
		APP_DRAW->stamp.img,APP_DRAW->stamp.size.w,APP_DRAW->stamp.size.h);
}

/** 画像読み込み */

static void _stamp_loadimage(mWidget *wg)
{
	mStr str = MSTR_INIT;
	int ret;

	ret = FileDialog_openLayerImage(wg->toplevel, FILEFILTER_NORMAL_IMAGE,
		APP_CONF->strStampFileDir.buf, &str);

	if(ret != -1)
	{
		//ディレクトリ記録

		mStrPathGetDir(&APP_CONF->strStampFileDir, str.buf);

		//読み込み

		drawStamp_loadImage(APP_DRAW, str.buf,
			FILEDIALOG_LAYERIMAGE_IS_IGNORE_ALPHA(ret));

		DockOption_toolStamp_changeImage(wg);
	}

	mStrFree(&str);
}

/** プレビューの D&D ハンドラ */

static int _stamp_prev_dnd_handle(mWidget *wg,char **files)
{
	drawStamp_loadImage(APP_DRAW, *files, 0);

	PrevTileImage_setImage((PrevTileImage *)wg,
		APP_DRAW->stamp.img, APP_DRAW->stamp.size.w, APP_DRAW->stamp.size.h);

	return 1;
}

/** イベント */

static int _stamp_event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY)
	{
		switch(ev->notify.id)
		{
			//クリア
			case WID_STAMP_CLEAR:
				drawStamp_clearImage(APP_DRAW);
				break;
			//画像読み込み
			case WID_STAMP_LOAD:
				_stamp_loadimage(wg);
				break;
			//上書き
			case WID_STAMP_OVERWRITE:
				STAMP_TOGGLE_OVERWRITE(&APP_DRAW->tool.opt_stamp);
				break;
			//左上
			case WID_STAMP_LEFTTOP:
				STAMP_TOGGLE_LEFTTOP(&APP_DRAW->tool.opt_stamp);
				break;
		}
	}

	return 1;
}

/** 作成 */

static mWidget *_create_tab_stamp(mWidget *parent)
{
	_tab_stamp *p;
	mWidget *ct,*ct2;
	int val,i;

	p = (_tab_stamp *)DockOption_createMainContainer(sizeof(_tab_stamp),
		parent, _stamp_event_handle);

	p->ct.sepW = 4;

	val = APP_DRAW->tool.opt_stamp;

	M_TR_G(TRGROUP_DOCKOPT_STAMP);

	//プレビュー + ボタン

	ct = mContainerCreate(M_WIDGET(p), MCONTAINER_TYPE_HORZ, 0, 7, 0);
	ct->margin.bottom = 2;

	p->prev = PrevTileImage_new(ct, 70, 70, _stamp_prev_dnd_handle);

	PrevTileImage_setImage(p->prev,
		APP_DRAW->stamp.img,APP_DRAW->stamp.size.w,APP_DRAW->stamp.size.h);

	ct2 = mContainerCreate(ct, MCONTAINER_TYPE_VERT, 0, 2, 0);

	for(i = 0; i < 2; i++)
		mButtonCreate(ct2, WID_STAMP_CLEAR + i, 0, MLF_EXPAND_W, 0, M_TR_T(TRID_STAMP_CLEAR + i));

	//チェック

	mCheckButtonCreate(M_WIDGET(p), WID_STAMP_OVERWRITE, 0, 0, 0,
		M_TR_T(TRID_STAMP_OVERWRITE), STAMP_IS_OVERWRITE(val));
	
	mCheckButtonCreate(M_WIDGET(p), WID_STAMP_LEFTTOP, 0, 0, 0,
		M_TR_T(TRID_STAMP_LEFTTOP), STAMP_IS_LEFTTOP(val));

	return (mWidget *)p;
}


//==============================
// main
//==============================


/** ツールのタブ内容ウィジェット作成
 *
 * 戻り値のポインタの mWidget::param に、実際の内容のウィジェットポインタが入る。 */

mWidget *DockOption_createTab_tool(mWidget *parent)
{
	mWidget *ctv,*area;

	//内容はスクロールできるようにする

	ctv = (mWidget *)mContainerViewNew(0, parent, 0);
	ctv->fLayout = MLF_EXPAND_WH;
	ctv->hintOverW = 1;

	switch(APP_DRAW->tool.no)
	{
		//図形塗り/消し
		case TOOL_FILL_POLYGON:
		case TOOL_FILL_POLYGON_ERASE:
			area = _create_tab_fill_polygon(ctv);
			break;
		//塗りつぶし
		case TOOL_FILL:
			area = _create_tab_fill(ctv);
			break;
		//グラデーション
		case TOOL_GRADATION:
			area = DockOption_createTab_tool_grad(ctv);
			break;
		//自動選択
		case TOOL_MAGICWAND:
			area = _create_tab_magicwand(ctv);
			break;
		//移動
		case TOOL_MOVE:
			area = _create_tab_move(ctv);
			break;
		//スタンプ
		case TOOL_STAMP:
			area = _create_tab_stamp(ctv);
			break;
		default:
			area = NULL;
			break;
	}

	M_CONTAINERVIEW(ctv)->cv.area = area;

	ctv->param = (intptr_t)area;

	return ctv;
}
