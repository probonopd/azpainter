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
 * グリッド設定ダイアログ
 *****************************************/

#include "mDef.h"
#include "mStr.h"
#include "mWidget.h"
#include "mContainer.h"
#include "mWindow.h"
#include "mDialog.h"
#include "mColorButton.h"
#include "mLineEdit.h"
#include "mWidgetBuilder.h"
#include "mEvent.h"
#include "mMenu.h"
#include "mTrans.h"

#include "defConfig.h"

#include "trgroup.h"


//------------------------

typedef struct
{
	mWidget wg;
	mContainerData ct;
	mWindowData win;
	mDialogData dlg;

	mLineEdit *le[6];
	mColorButton *colbtt[2];
}_griddlg;

//------------------------

static const char *g_build =
"ct#h:sep=8;"
  "gb:cts=g3:sep=6,7:tr=1;"
	//幅
    "lb:lf=mr:TR=0,0;"
    "le#s:id=100:wlen=6;"
    "arrbt#d:id=130:lf=m;"
    //高さ
    "lb:lf=mr:TR=0,1;"
    "le#s:id=101:wlen=6;"
    "sp;"
    //色
    "lb:lf=mr:TR=0,2;"
    "colbt#d:id=120:lf=m;"
    "sp;"
    //濃度
    "lb:lf=mr:TR=0,3;"
    "le#s:id=102:wlen=6;"
  "-;"
  "gb:cts=g2:sep=6,7:tr=2;"
	//横分割数
    "lb:lf=mr:tr=3;"
    "le#s:id=103:wlen=6;"
    //縦分割数
    "lb:lf=mr:tr=4;"
    "le#s:id=104:wlen=6;"
    //色
    "lb:lf=mr:TR=0,2;"
    "colbt#d:id=121:lf=m;"
    //濃度
    "lb:lf=mr:TR=0,3;"
    "le#s:id=105:wlen=6;"
;

//------------------------

enum
{
	WID_EDIT_TOP = 100,
	WID_COLBTT_TOP = 120,
	WID_BTT_REGLIST = 130,

	GRID_MINSIZE = 1,
	GRID_MAXSIZE = 10000
};

//------------------------


/** 登録リストメニュー */

static void _run_regmenu(_griddlg *p)
{
	mMenu *menu;
	mMenuItemInfo *mi;
	mStr str = MSTR_INIT;
	mBox box;
	int i,w,h,id;
	uint32_t val;

	w = mLineEditGetNum(p->le[0]);
	h = mLineEditGetNum(p->le[1]);

	menu = mMenuNew();

	//呼び出し用

	for(i = 0; i < CONFIG_GRIDREG_NUM; i++)
	{
		val = APP_CONF->grid.reglist[i];
		mStrSetFormat(&str, "[%d] %d x %d", i + 1, val >> 16, val & 0xffff);

		mMenuAddText_copy(menu, i, str.buf);
	}

	mMenuAddSep(menu);

	//登録用

	for(i = 0; i < CONFIG_GRIDREG_NUM; i++)
	{
		mStrSetFormat(&str, "[%d] <- %d x %d", i + 1, w, h);

		mMenuAddText_copy(menu, 1000 + i, str.buf);
	}

	mStrFree(&str);

	//実行

	mWidgetGetRootBox(mWidgetFindByID(M_WIDGET(p), WID_BTT_REGLIST), &box);

	mi = mMenuPopup(menu, NULL, box.x, box.y + box.h, 0);
	id = (mi)? mi->id: -1;

	mMenuDestroy(menu);

	if(id == -1) return;

	//

	if(id < 1000)
	{
		//呼び出し

		val = APP_CONF->grid.reglist[id];

		mLineEditSetNum(p->le[0], val >> 16);
		mLineEditSetNum(p->le[1], val & 0xffff);
	}
	else
	{
		//登録

		APP_CONF->grid.reglist[id - 1000] = (w << 16) | h;
	}
}

/** イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY && ev->notify.id == WID_BTT_REGLIST)
		_run_regmenu((_griddlg *)wg);

	return mDialogEventHandle_okcancel(wg, ev);
}

/** 作成 */

static _griddlg *_create_dlg(mWindow *owner)
{
	_griddlg *p;
	mWidget *ct;
	mLineEdit *le;
	int i;

	//作成
	
	p = (_griddlg *)mDialogNew(sizeof(_griddlg), owner, MWINDOW_S_DIALOG_NORMAL);
	if(!p) return NULL;
	
	p->wg.event = _event_handle;

	//

	M_TR_G(TRGROUP_DLG_GRID_OPT);

	mContainerSetPadding_one(M_CONTAINER(p), 8);
	p->ct.sepW = 16;

	mWindowSetTitle(M_WINDOW(p), M_TR_T(0));

	//------ ウィジェット

	ct = mContainerCreate(M_WIDGET(p), MCONTAINER_TYPE_VERT, 0, 12, 0);

	mWidgetBuilderCreateFromText(ct, g_build);

	//edit

	for(i = 0; i < 6; i++)
	{
		le = p->le[i] = (mLineEdit *)mWidgetFindByID(ct, WID_EDIT_TOP + i);

		if(i == 2 || i == 5)
			mLineEditSetNumStatus(le, 1, 100, 0);
		else if(i < 3)
			mLineEditSetNumStatus(le, GRID_MINSIZE, GRID_MAXSIZE, 0);
		else
			mLineEditSetNumStatus(le, 2, 100, 0);
	}

	mLineEditSetNum(p->le[0], APP_CONF->grid.gridw);
	mLineEditSetNum(p->le[1], APP_CONF->grid.gridh);
	mLineEditSetNum(p->le[2], (int)((APP_CONF->grid.col_grid >> 24) / 128.0 * 100 + 0.5));

	mLineEditSetNum(p->le[3], APP_CONF->grid.splith);
	mLineEditSetNum(p->le[4], APP_CONF->grid.splitv);
	mLineEditSetNum(p->le[5], (int)((APP_CONF->grid.col_split >> 24) / 128.0 * 100 + 0.5));

	//色

	for(i = 0; i < 2; i++)
		p->colbtt[i] = (mColorButton *)mWidgetFindByID(ct, WID_COLBTT_TOP + i);

	mColorButtonSetColor(p->colbtt[0], APP_CONF->grid.col_grid);
	mColorButtonSetColor(p->colbtt[1], APP_CONF->grid.col_split);

	//OK/Cancel

	mContainerCreateOkCancelButton(M_WIDGET(p));

	return p;
}


//=====================


/** グリッド設定ダイアログ実行 */

mBool GridOptionDlg_run(mWindow *owner)
{
	_griddlg *p;
	mBool ret;

	p = _create_dlg(owner);
	if(!p) return FALSE;

	mWindowMoveResizeShow_hintSize(M_WINDOW(p));

	ret = mDialogRun(M_DIALOG(p), FALSE);

	if(ret)
	{
		APP_CONF->grid.gridw = mLineEditGetNum(p->le[0]);
		APP_CONF->grid.gridh = mLineEditGetNum(p->le[1]);
		APP_CONF->grid.col_grid = mColorButtonGetColor(p->colbtt[0]);
		APP_CONF->grid.col_grid |= (int)(mLineEditGetNum(p->le[2]) / 100.0 * 128.0 + 0.5) << 24;

		APP_CONF->grid.splith = mLineEditGetNum(p->le[3]);
		APP_CONF->grid.splitv = mLineEditGetNum(p->le[4]);
		APP_CONF->grid.col_split = mColorButtonGetColor(p->colbtt[1]);
		APP_CONF->grid.col_split |= (int)(mLineEditGetNum(p->le[5]) / 100.0 * 128.0 + 0.5) << 24;
	}

	mWidgetDestroy(M_WIDGET(p));

	return ret;
}
