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

/********************************
 * グラデーション編集ダイアログ
 ********************************/

#include "mDef.h"
#include "mWidget.h"
#include "mDialog.h"
#include "mContainer.h"
#include "mWindow.h"
#include "mWidgetBuilder.h"
#include "mCheckButton.h"
#include "mColorButton.h"
#include "mArrowButton.h"
#include "mLineEdit.h"
#include "mSliderBar.h"
#include "mMenu.h"
#include "mTrans.h"
#include "mEvent.h"
#include "mUtilStr.h"

#include "GradationList.h"
#include "GradationEditWidget.h"

#include "trgroup.h"


//--------------------

typedef struct
{
	mWidget wg;
	mContainerData ct;
	mWindowData win;
	mDialogData dlg;

	GradItem *item;

	GradEditWidget *editwg[2];
	mLineEdit *ledit_pos[2],
		*ledit_opa;
	mSliderBar *sl_opa;
	mArrowButton *menubtt[2];
	mColorButton *colbt;
	mCheckButton *ck_coltype,
		*ck_flag[2];
}_edit_dlg;

//--------------------

static const char *g_wb_gradeditdlg =
//色
"gb:cts=h:lf=w:tr=1:sep=5;"
  "lb:lf=m:tr=2;"
  "le#cs:id=100:wlen=7;"
  "ck#r:id=102:tr=3:lf=m;"
  "ck#r:id=103:tr=4:lf=m;"
  "ck#r:id=104:tr=5:lf=m;"
  "colbt#d:id=105:lf=m;"
"-;"
//不透明度
"gb:cts=h:lf=w:tr=6:sep=5:mg=0,8,0,10;"
  "lb:tr=2:lf=m;"
  "le#cs:id=101:wlen=7;"
  "lb:tr=7:lf=m;"
  "sl:id=106:lf=wm:range=0,1000;"
  "le#cs:id=107:wlen=6;"
"-;"
//チェック
"ct#h:sep=4:mg=b10;"
  "ck:id=108:tr=8;"
  "ck:id=109:tr=9;"
"-;"
//ヘルプ
"lb#B:tr=10:lf=w:mg=b12;"
;

//--------------------

enum
{
	WID_EDIT_COL_POS = 100,
	WID_EDIT_A_POS,
	WID_CK_COL_DRAW,
	WID_CK_COL_BKGND,
	WID_CK_COL_SPEC,
	WID_COLBTT,
	WID_SLIDER_A,
	WID_EDIT_A_VAL,
	WID_CK_LOOP,
	WID_CK_SINGLE_COL,

	WID_EDITWG_COL = 200,
	WID_EDITWG_A,
	WID_BTT_COL_MOVELEFT,
	WID_BTT_A_MOVELEFT,
	WID_BTT_COL_MOVERIGHT,
	WID_BTT_A_MOVERIGHT,
	WID_MENUBTT_COL,
	WID_MENUBTT_A
};

enum
{
	TRID_MENU_DELETE = 100,
	TRID_MENU_DIVIDE,
	TRID_MENU_MOVE_CENTER,
	TRID_MENU_ALL_EVEN,
	TRID_MENU_REVERSE
};

//--------------------



//==========================
// sub
//==========================


/** GradEditWidget の更新 */

static void _update_editwg(_edit_dlg *p,int no)
{
	mWidgetUpdate(M_WIDGET(p->editwg[no]));
}

/** カレントポイントの情報セット */

static void _set_curinfo(_edit_dlg *p,int no)
{
	int n;
	uint32_t col;

	//位置

	mLineEditSetNum(p->ledit_pos[no],
		(int)((double)GradEditWidget_getCurrentPos(p->editwg[no]) / (1 << GRADDAT_POS_BIT) * 1000 + 0.5));

	//値

	if(no == 0)
	{
		//RGB
		
		n = GradEditWidget_getCurrentColor(p->editwg[no], &col);

		mCheckButtonSetRadioSel(p->ck_coltype, n);

		mColorButtonSetColor(p->colbt, col);
		mWidgetEnable(M_WIDGET(p->colbt), (n == 2));
	}
	else
	{
		//A
		
		n = GradEditWidget_getCurrentOpacity(p->editwg[no]);

		mSliderBarSetPos(p->sl_opa, n);
		mLineEditSetNum(p->ledit_opa, n);
	}
}

/** メニュー実行 */

static void _run_menu(_edit_dlg *p,int no)
{
	GradEditWidget *editwg;
	mMenu *menu,*sub;
	mMenuItemInfo *mi;
	int i,n;
	char m[4];
	mBox box;
	uint16_t id[] = {
		TRID_MENU_DELETE, 0xfffe, TRID_MENU_DIVIDE, 0xfffe,
		TRID_MENU_MOVE_CENTER, TRID_MENU_ALL_EVEN, TRID_MENU_REVERSE, 0xffff
	};

	editwg = p->editwg[no];

	//------- メニュー作成

	M_TR_G(TRGROUP_DLG_GRADATION_EDIT);

	menu = mMenuNew();

	mMenuAddTrArray16(menu, id);

	//無効 (位置が端の場合)

	n = GradEditWidget_getCurrentPos(editwg);

	if(n == 0 || n == GRADDAT_POS_MAX)
	{
		mMenuSetEnable(menu, TRID_MENU_DELETE, 0);
		mMenuSetEnable(menu, TRID_MENU_MOVE_CENTER, 0);
	}

	//分割数

	n = GradEditWidget_getMaxSplitNum_toright(editwg);

	if(n < 2)
		mMenuSetEnable(menu, TRID_MENU_DIVIDE, FALSE);
	else
	{
		sub = mMenuNew();

		for(i = 2; i <= n && i <= 10; i++)
		{
			mIntToStr(m, i);
			mMenuAddText_copy(sub, 1000 + i, m);
		}

		mMenuSetSubmenu(menu, TRID_MENU_DIVIDE, sub);
	}

	//--------- メニュー実行

	mWidgetGetRootBox(M_WIDGET(p->menubtt[no]), &box);

	mi = mMenuPopup(menu, NULL, box.x + box.w, box.y + box.h, MMENU_POPUP_F_RIGHT);
	i = (mi)? mi->id: -1;

	mMenuDestroy(menu);

	if(i == -1) return;

	//-------- コマンド実行

	if(i >= 1000)
		//分割
		GradEditWidget_splitPoint_toright(editwg, i - 1000);
	else
	{
		switch(i)
		{
			//カレント削除
			case TRID_MENU_DELETE:
				GradEditWidget_deletePoint(editwg, -1);
				break;
			//中間位置へ
			case TRID_MENU_MOVE_CENTER:
				GradEditWidget_moveCurrentPos_middle(editwg);
				break;
			//すべて等間隔に
			case TRID_MENU_ALL_EVEN:
				GradEditWidget_evenAllPoints(editwg);
				break;
			//反転
			case TRID_MENU_REVERSE:
				GradEditWidget_reversePoints(editwg);
				break;
		}
	}
}


//==========================
// イベント
//==========================


/** OK 時、グラデーションデータをセット */

static void _set_graddat(_edit_dlg *p)
{
	int colnum,anum;
	uint8_t flags;

	colnum = GradEditWidget_getPointNum(p->editwg[0]);
	anum = GradEditWidget_getPointNum(p->editwg[1]);

	//フラグ

	flags = 0;
	if(mCheckButtonIsChecked(p->ck_flag[0])) flags |= GRADDAT_F_LOOP;
	if(mCheckButtonIsChecked(p->ck_flag[1])) flags |= GRADDAT_F_SINGLE_COL;

	//セット

	GradItem_setDat(p->item, flags, colnum, anum,
		GradEditWidget_getBuf(p->editwg[0]),
		GradEditWidget_getBuf(p->editwg[1]));
}

/** イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY)
	{
		_edit_dlg *p = (_edit_dlg *)wg;
		int id,type,n;

		id = ev->notify.id;
		type = ev->notify.type;
	
		switch(id)
		{
			//グラデ編集 (col)
			case WID_EDITWG_COL:
				_set_curinfo(p, 0);
				break;
			//グラデ編集 (A)
			case WID_EDITWG_A:
				_set_curinfo(p, 1);

				//ポインタに変更があれば色のグラデーションも再描画
				if(type == GRADEDITWIDGET_N_MODIFY)
					_update_editwg(p, 0);
				break;

			//位置 エディット
			case WID_EDIT_COL_POS:
			case WID_EDIT_A_POS:
				if(type == MLINEEDIT_N_CHANGE)
				{
					id -= WID_EDIT_COL_POS;

					if(GradEditWidget_setCurrentPos(p->editwg[id], mLineEditGetNum(p->ledit_pos[id]))
						&& id == 1)
					{
						//A の値が変わったらカラーを再描画
						_update_editwg(p, 0);
					}
				}
				break;
			//色タイプ
			case WID_CK_COL_DRAW:
			case WID_CK_COL_BKGND:
			case WID_CK_COL_SPEC:
				n = id - WID_CK_COL_DRAW;
				
				mWidgetEnable(M_WIDGET(p->colbt), (n == 2));
			
				GradEditWidget_setColorType(p->editwg[0], n);
				break;
			//指定色
			case WID_COLBTT:
				if(type == MCOLORBUTTON_N_PRESS)
					GradEditWidget_setColorRGB(p->editwg[0], ev->notify.param1);
				break;
			//不透明度:スライダー
			case WID_SLIDER_A:
				if(type == MSLIDERBAR_N_HANDLE
					&& (ev->notify.param2 & MSLIDERBAR_HANDLE_F_CHANGE))
				{
					GradEditWidget_setOpacity(p->editwg[1], ev->notify.param1);

					mLineEditSetNum(p->ledit_opa, ev->notify.param1);

					_update_editwg(p, 0);
				}
				break;
			//不透明度:エディット
			case WID_EDIT_A_VAL:
				if(type == MLINEEDIT_N_CHANGE)
				{
					n = mLineEditGetNum(p->ledit_opa);

					GradEditWidget_setOpacity(p->editwg[1], n);

					mSliderBarSetPos(p->sl_opa, n);

					_update_editwg(p, 0);
				}
				break;

			//カレントポイント選択移動
			case WID_BTT_COL_MOVELEFT:
			case WID_BTT_A_MOVELEFT:
			case WID_BTT_COL_MOVERIGHT:
			case WID_BTT_A_MOVERIGHT:
				id -= WID_BTT_COL_MOVELEFT;
				
				GradEditWidget_moveCurrentPoint(p->editwg[id & 1], (id >= 2));
				_set_curinfo(p, id & 1);
				break;
			//メニューボタン
			case WID_MENUBTT_COL:
			case WID_MENUBTT_A:
				_run_menu(p, id - WID_MENUBTT_COL);
				break;

			//単色化
			case WID_CK_SINGLE_COL:
				GradEditWidget_setSingleColor(p->editwg[0], -1);

				_update_editwg(p, 0);
				break;

			//OK
			case M_WID_OK:
				_set_graddat(p);
				mDialogEnd(M_DIALOG(wg), TRUE);
				break;
			//キャンセル
			case M_WID_CANCEL:
				mDialogEnd(M_DIALOG(wg), FALSE);
				break;
		}
	}

	return mDialogEventHandle(wg, ev);
}


//==========================
// 作成
//==========================


/** 編集部分のウィジェット作成 */

static void _create_edit_widget(_edit_dlg *p,int no)
{
	mWidget *ct_toph,*ct_v,*ct_h;
	mArrowButton *abtt;

	ct_toph = mContainerCreate(M_WIDGET(p), MCONTAINER_TYPE_HORZ, 0, 8, MLF_EXPAND_W);
	ct_toph->margin.bottom = 6;

	p->editwg[no] = GradEditWidget_new(ct_toph, WID_EDITWG_COL + no, no, p->item->buf);

	//----- ボタン

	ct_v = mContainerCreate(ct_toph, MCONTAINER_TYPE_VERT, 0, 4, 0);
	ct_h = mContainerCreate(ct_v, MCONTAINER_TYPE_HORZ, 0, 0, 0);

	//移動

	abtt = mArrowButtonCreate(ct_h, WID_BTT_COL_MOVELEFT + no, MARROWBUTTON_S_LEFT, 0, 0);
	abtt->wg.hintOverW = abtt->wg.hintOverH = 19;
	
	abtt = mArrowButtonCreate(ct_h, WID_BTT_COL_MOVERIGHT + no, MARROWBUTTON_S_RIGHT, 0, 0);
	abtt->wg.hintOverW = abtt->wg.hintOverH = 19;

	//メニューボタン

	p->menubtt[no] = mArrowButtonCreate(ct_v,
		WID_MENUBTT_COL + no, MARROWBUTTON_S_DOWN, MLF_RIGHT, 0);
}

/** ダイアログ作成 */

static _edit_dlg *_create_dlg(mWindow *owner,GradItem *item)
{
	_edit_dlg *p;
	int i,flag;

	p = (_edit_dlg *)mDialogNew(sizeof(_edit_dlg), owner, MWINDOW_S_DIALOG_NORMAL);
	if(!p) return NULL;

	p->item = item;
	p->wg.event = _event_handle;

	//

	mContainerSetPadding_one(M_CONTAINER(p), 8);

	M_TR_G(TRGROUP_DLG_GRADATION_EDIT);

	mWindowSetTitle(M_WINDOW(p), M_TR_T(0));

	//------- 編集部分

	_create_edit_widget(p, 0);
	_create_edit_widget(p, 1);

	GradEditWidget_setAnotherModeBuf(p->editwg[0], GradEditWidget_getBuf(p->editwg[1]));
	GradEditWidget_setAnotherModeBuf(p->editwg[1], GradEditWidget_getBuf(p->editwg[0]));

	//------- 以降

	mWidgetBuilderCreateFromText(M_WIDGET(p), g_wb_gradeditdlg);

	//位置

	for(i = 0; i < 2; i++)
	{
		p->ledit_pos[i] = (mLineEdit *)mWidgetFindByID(M_WIDGET(p), WID_EDIT_COL_POS + i);
		mLineEditSetNumStatus(p->ledit_pos[i], 0, 1000, 1);
	}

	//色

	p->ck_coltype = (mCheckButton *)mWidgetFindByID(M_WIDGET(p), WID_CK_COL_DRAW);

	p->colbt = (mColorButton *)mWidgetFindByID(M_WIDGET(p), WID_COLBTT);

	//不透明度

	p->sl_opa = (mSliderBar *)mWidgetFindByID(M_WIDGET(p), WID_SLIDER_A);

	p->ledit_opa = (mLineEdit *)mWidgetFindByID(M_WIDGET(p), WID_EDIT_A_VAL);
	mLineEditSetNumStatus(p->ledit_opa, 0, 1000, 1);

	//フラグ

	flag = item->buf[0];

	for(i = 0; i < 2; i++)
	{
		p->ck_flag[i] = (mCheckButton *)mWidgetFindByID(M_WIDGET(p), WID_CK_LOOP + i);
	
		mCheckButtonSetState(p->ck_flag[i], flag & (1 << i));
	}

	//OK/キャンセル

	mContainerCreateOkCancelButton(M_WIDGET(p));

	//------ 初期化

	_set_curinfo(p, 0);
	_set_curinfo(p, 1);

	GradEditWidget_setSingleColor(p->editwg[0], flag & GRADDAT_F_SINGLE_COL);

	return p;
}


//==========================
//
//==========================


/** ダイアログ実行 */

mBool GradationEditDlg_run(mWindow *owner,GradItem *item)
{
	_edit_dlg *p;

	p = _create_dlg(owner, item);
	if(!p) return FALSE;

	mWindowMoveResizeShow_hintSize(M_WINDOW(p));

	return mDialogRun(M_DIALOG(p), TRUE);
}
