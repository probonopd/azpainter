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
 * 矩形編集 - 変形ダイアログ
 *****************************************/

#include <stdio.h>  //snprintf
#include <string.h> //memcpy
#include <math.h>

#include "mDef.h"
#include "mStr.h"
#include "mWidget.h"
#include "mContainer.h"
#include "mWindow.h"
#include "mDialog.h"
#include "mCheckButton.h"
#include "mSliderBar.h"
#include "mLabel.h"
#include "mLineEdit.h"
#include "mGroupBox.h"
#include "mButton.h"
#include "mTrans.h"
#include "mEvent.h"
#include "mRectBox.h"

#include "defDraw.h"
#include "defConfig.h"

#include "trgroup.h"

#include "TransformDlg_def.h"


//---------------------

typedef struct
{
	mWidget wg;
	mContainerData ct;
	mWindowData win;
	mDialogData dlg;

	TransformViewArea *area;
	mLabel *lb_zoom;
	mSliderBar *sl_zoom;
	mLineEdit *edit[3];
	mWidget *wg_enterbtt;
}_dialog;

//---------------------

enum
{
	TRID_RESET = 1,
	TRID_X_SCALE,
	TRID_Y_SCALE,
	TRID_ROTATE,
	TRID_ENTER,
	TRID_KEEP_ASPECT,
	TRID_HELP,
	TRIDTRANSFORM_TYPE_TOP = 100
};

enum
{
	WID_SLIDER_ZOOM = 100,
	WID_VIEWAREA,
	WID_BTT_RESET,
	WID_EDIT_SCALEX,
	WID_EDIT_SCALEY,
	WID_EDIT_ROTATE,
	WID_BTT_ENTER,
	WID_CK_KEEPASPECT,

	WID_CKTRANSFORM_TYPE_NORMAL = 200,
	WID_CKTRANSFORM_TYPE_TRAPEZOID
};

//---------------------



/** エディットにパラメータ値セット */

static void _set_param_edit(_dialog *p)
{
	mLineEditSetDouble(p->edit[0], p->area->scalex, 5);
	mLineEditSetDouble(p->edit[1], p->area->scaley, 5);
	mLineEditSetDouble(p->edit[2], -(p->area->angle) / M_MATH_PI * 180, 3);
}

/** 表示倍率バー位置セット */

static void _set_zoom_barpos(_dialog *p)
{
	int n = p->area->canv_zoom;

	if(n <= 100)
		n--;
	else
		n = (n - 100) / TRANSFORM_CANVZOOM_UPSTEP + (100 - TRANSFORM_CANVZOOM_MIN);

	mSliderBarSetPos(p->sl_zoom, n);
}

/** 表示倍率ラベルセット */

static void _set_zoom_label(_dialog *p)
{
	char m[16];

	snprintf(m, 16, "%d%%", p->area->canv_zoom);

	mLabelSetText(p->lb_zoom, m);
}

/** 値の確定 */

static void _enter_param(_dialog *p)
{
	double sx,sy,rd;

	sx = mLineEditGetDouble(p->edit[0]);
	sy = mLineEditGetDouble(p->edit[1]);
	rd = mLineEditGetDouble(p->edit[2]);

	if(sx <= 0) sx = 0.001;
	if(sy <= 0) sy = 0.001;

	rd = fmod(rd, 180) / 180 * M_MATH_PI;

	//

	p->area->scalex = sx;
	p->area->scaley = sy;
	p->area->angle = -rd;

	TransformViewArea_update(p->area, TRANSFORM_UPDATE_WITH_TRANSPARAM);
}

/** XY倍率のエディット編集時、縦横比維持の場合はもう一方にも同じ値をセット */

static void _set_edit_keepaspect(_dialog *p,int no)
{
	mStr str = MSTR_INIT;

	mLineEditGetTextStr(p->edit[no], &str);

	mLineEditSetText(p->edit[no ^ 1], str.buf);

	mStrFree(&str);
}

/** パラメータリセット */

static void _reset_param(_dialog *p)
{
	TransformViewArea_resetTransformParam(p->area);
	TransformViewArea_update(p->area, TRANSFORM_UPDATE_WITH_TRANSPARAM);

	_set_param_edit(p);
}

/** 変形タイプ変更 */

static void _set_transform_type(_dialog *p,int type)
{
	int i,f;

	p->area->type = type;

	//パラメータリセット

	_reset_param(p);

	//有効/無効

	f = (type == TRANSFORM_TYPE_NORMAL);

	for(i = 0; i < 3; i++)
		mWidgetEnable(M_WIDGET(p->edit[i]), f);

	mWidgetEnable(p->wg_enterbtt, f);
}


//====================


/** 表示倍率スライダーのハンドラ */

static void _sliderbar_handle(mSliderBar *p,int pos,int flags)
{
	TransformViewArea *area = (TransformViewArea *)p->wg.param;

	//ドラッグ中は低品質表示

	if(flags & MSLIDERBAR_HANDLE_F_PRESS)
		//ドラッグ開始時
		TransformViewArea_startCanvasZoomDrag(area);
	else if(flags & MSLIDERBAR_HANDLE_F_RELEASE)
		area->low_quality = 0;

	//値変更

	if(flags & MSLIDERBAR_HANDLE_F_CHANGE)
	{
		if(pos <= 100 - TRANSFORM_CANVZOOM_MIN)
			pos++;
		else
			pos = (pos - (100 - TRANSFORM_CANVZOOM_MIN)) * TRANSFORM_CANVZOOM_UPSTEP + 100;
	
		TransformViewArea_setCanvasZoom(area, pos);
	
		mWidgetAppendEvent_notify(NULL, M_WIDGET(p), 1000, 0, 0);
	}

	mWidgetUpdate(M_WIDGET(area));
}

/** イベント */

static int _dlg_event_handle(mWidget *wg,mEvent *ev)
{
	_dialog *p = (_dialog *)wg;
	int id;

	if(ev->type == MEVENT_NOTIFY)
	{
		id = ev->notify.id;

		switch(id)
		{
			//ビューエリア
			case WID_VIEWAREA:
				if(ev->notify.type == TRANSFORM_AREA_NOTIFY_CHANGE_AFFINE_PARAM)
					//アフィン変換のパラメータ変更
					_set_param_edit(p);
				else
				{
					//キャンバス表示倍率変更

					_set_zoom_barpos(p);
					_set_zoom_label(p);
				}	
				break;
		
			//表示倍率スライダー (値変更時)
			case WID_SLIDER_ZOOM:
				if(ev->notify.type == 1000)
					_set_zoom_label(p);
				break;

			//XY倍率エディット
			case WID_EDIT_SCALEX:
			case WID_EDIT_SCALEY:
				if(ev->notify.type == MLINEEDIT_N_CHANGE
					&& p->area->keep_aspect)
					_set_edit_keepaspect(p, id - WID_EDIT_SCALEX);
				break;

			//タイプ
			case WID_CKTRANSFORM_TYPE_NORMAL:
			case WID_CKTRANSFORM_TYPE_TRAPEZOID:
				_set_transform_type(p, id - WID_CKTRANSFORM_TYPE_NORMAL);
				break;
		
			//リセット
			case WID_BTT_RESET:
				_reset_param(p);
				break;
			//確定
			case WID_BTT_ENTER:
				_enter_param(p);
				break;
			//縦横比維持
			case WID_CK_KEEPASPECT:
				p->area->keep_aspect ^= 1;
				break;
		}
	}

	return mDialogEventHandle_okcancel(wg, ev);
}


//=======================


/** 作成 */

static _dialog *_create_dialog(mWindow *owner,TileImage *imgcopy)
{
	_dialog *p;
	mWidget *ct,*ct2,*ct3;
	int i;

	p = (_dialog *)mDialogNew(sizeof(_dialog), owner, MWINDOW_S_DIALOG_NORMAL);
	if(!p) return NULL;

	p->wg.event = _dlg_event_handle;

	//

	mContainerSetType(M_CONTAINER(p), MCONTAINER_TYPE_HORZ, 0);
	mContainerSetPadding_one(M_CONTAINER(p), 8);

	p->ct.sepW = 12;

	M_TR_G(TRGROUP_DLG_BOXEDITTRANS);

	mWindowSetTitle(M_WINDOW(p), M_TR_T(0));

	//========= [左側]

	ct = mContainerCreate(M_WIDGET(p), MCONTAINER_TYPE_VERT, 0, 8, MLF_EXPAND_WH);

	//ビューエリア

	p->area = TransformViewArea_create(ct, WID_VIEWAREA);

	//---- 表示倍率

	ct2 = mContainerCreate(ct, MCONTAINER_TYPE_HORZ, 0, 6, MLF_EXPAND_W);

	//ラベル

	p->lb_zoom = mLabelCreate(ct2, MLABEL_S_CENTER | MLABEL_S_BORDER, MLF_MIDDLE, 0, NULL);

	mWidgetSetHintOverW_fontTextWidth(M_WIDGET(p->lb_zoom), "99999%");

	//スライダー

	p->sl_zoom = mSliderBarCreate(ct2, WID_SLIDER_ZOOM, 0, MLF_EXPAND_W | MLF_MIDDLE, 0);

	p->sl_zoom->wg.param = (intptr_t)p->area;
	p->sl_zoom->sl.handle = _sliderbar_handle;

	mSliderBarSetStatus(p->sl_zoom, 0,
		(100 - TRANSFORM_CANVZOOM_MIN) + (TRANSFORM_CANVZOOM_MAX - 100) / TRANSFORM_CANVZOOM_UPSTEP, 0);

	//========= [右側]

	ct = mContainerCreate(M_WIDGET(p), MCONTAINER_TYPE_VERT, 0, 6, MLF_EXPAND_H);

	//タイプ

	for(i = 0; i < 2; i++)
	{
		mCheckButtonCreate(ct, WID_CKTRANSFORM_TYPE_NORMAL + i, MCHECKBUTTON_S_RADIO, 0, 0,
			M_TR_T(TRIDTRANSFORM_TYPE_TOP + i), (i == 0));
	}

	//リセットボタン

	mButtonCreate(ct, WID_BTT_RESET, 0, MLF_EXPAND_W, M_MAKE_DW4(0,8,0,0), M_TR_T(TRID_RESET));

	//数値

	ct2 = (mWidget *)mGroupBoxNew(0, ct, 0);

	ct2->fLayout = MLF_EXPAND_W;
	ct2->margin.top = 10;
	M_CONTAINER(ct2)->ct.sepW = 8;

	mContainerSetType(M_CONTAINER(ct2), MCONTAINER_TYPE_VERT, 0);

	ct3 = mContainerCreateGrid(ct2, 2, 5, 7, MLF_EXPAND_W);

	for(i = 0; i < 3; i++)
	{
		mLabelCreate(ct3, 0, MLF_MIDDLE, 0, M_TR_T(TRID_X_SCALE + i));

		p->edit[i] = mLineEditCreate(ct3, WID_EDIT_SCALEX + i,
			(i == 2)? 0: MLINEEDIT_S_NOTIFY_CHANGE, MLF_EXPAND_W, 0);
	}

	p->wg_enterbtt = (mWidget *)mButtonCreate(ct2, WID_BTT_ENTER, 0, MLF_EXPAND_W, 0, M_TR_T(TRID_ENTER));
	p->wg_enterbtt->fState |= MWIDGET_STATE_ENTER_DEFAULT;

	//縦横比維持

	mCheckButtonCreate(ct, WID_CK_KEEPASPECT, 0, 0, M_MAKE_DW4(0,6,0,0),
		M_TR_T(TRID_KEEP_ASPECT), APP_CONF->boxedit_flags & TRANSFORM_CONFIG_F_KEEPASPECT);

	//ヘルプ

	mLabelCreate(ct, MLABEL_S_BORDER,
		MLF_EXPAND_W | MLF_EXPAND_Y | MLF_BOTTOM,
		M_MAKE_DW4(0,6,0,8), M_TR_T(TRID_HELP));

	//OK/キャンセル

	mButtonCreate(ct, M_WID_OK, 0, MLF_EXPAND_W, 0, M_TR_T2(M_TRGROUP_SYS, M_TRSYS_OK));
	mButtonCreate(ct, M_WID_CANCEL, 0, MLF_EXPAND_W, 0, M_TR_T2(M_TRGROUP_SYS, M_TRSYS_CANCEL));

	//------- 初期化

	p->area->imgcopy = imgcopy;

	TransformViewArea_init(p->area);

	_set_param_edit(p);

	//倍率初期値 (縮小のみなので拡大はない)

	_set_zoom_barpos(p);
	_set_zoom_label(p);
	
	return p;
}

/** OK 時 */

static int _set_data(TransformViewArea *area,double *dst,mRect *rcdst)
{
	int type = area->type;

	if(type == TRANSFORM_TYPE_NORMAL)
	{
		//アフィン変換

		dst[0] = area->scalex;
		dst[1] = area->scaley;
		dst[2] = area->dcos;
		dst[3] = area->dsin;
		dst[4] = area->ptmov.x;
		dst[5] = area->ptmov.y;
	}
	else
	{
		//射影変換

		memcpy(dst, area->homog_param, sizeof(double) * 9);

		//平行移動

		dst[9] = area->ptmov.x;
		dst[10] = area->ptmov.y;

		//描画範囲

		TransformViewArea_homog_getDstImageRect(area, rcdst);
	}

	return type;
}

/** ダイアログ実行
 *
 * @param dst   描画用パラメータ (最大11個)
 * @param rcdst 射影変換の描画先イメージ範囲
 * @return -1:キャンセル 0:通常 1:遠近 */

int BoxEditTransform_run(mWindow *owner,TileImage *imgcopy,double *dst,mRect *rcdst)
{
	_dialog *p;
	int ret;

	p = _create_dialog(owner, imgcopy);
	if(!p) return -1;

	//実行

	mWindowMoveResizeShow_hintSize(M_WINDOW(p));

	ret = mDialogRun(M_DIALOG(p), FALSE);

	//ビューのサイズ記録

	APP_CONF->size_boxeditdlg.w = p->area->wg.w;
	APP_CONF->size_boxeditdlg.h = p->area->wg.h;

	//オプションフラグ

	APP_CONF->boxedit_flags = 0;
	if(p->area->keep_aspect) APP_CONF->boxedit_flags |= TRANSFORM_CONFIG_F_KEEPASPECT;

	//戻り値 + パラメータ値

	if(!ret)
		ret = -1;
	else
		ret = _set_data(p->area, dst, rcdst);
	
	//

	mWidgetDestroy(M_WIDGET(p));

	return ret;
}
