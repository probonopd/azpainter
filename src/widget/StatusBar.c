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
 * StatusBar
 * 
 * ステータスバー
 *****************************************/
/*
 * ステータスバーが非表示の場合、ウィジェットは削除される。
 * 作成されていなければ、APP_WIDGETS->statusbar は NULL。
 */

#include "mDef.h"
#include "mContainerDef.h"
#include "mWidget.h"
#include "mContainer.h"
#include "mLabel.h"
#include "mTrans.h"
#include "mStr.h"

#include "defConfig.h"
#include "defWidgets.h"
#include "defDraw.h"

#include "StatusBar.h"

#include "trgroup.h"


//----------------------

struct _StatusBar
{
	mWidget wg;
	mContainerData ct;

	mLabel *label_imginfo,
		*label_help;
};

//----------------------

enum
{
	TRID_BRUSH_FREE,
	TRID_FILL,		//塗りつぶし、自由選択
	TRID_TEXT,
	TRID_MOVE,		//移動ツール、選択範囲移動
	TRID_SELECT,
	TRID_SEL_MOVECOPY,
	TRID_STAMP,
	TRID_SPOIT,

	TRID_LINE45,
	TRID_DRAWTYPE_BOX,
	TRID_DRAWTYPE_CIRCLE,
	TRID_DRAWTYPE_SUCCLINE,
	TRID_DRAWTYPE_CONCLINE,
	TRID_DRAWTYPE_BEZIER,
	TRID_DRAWTYPE_SPLINE,
	TRID_POLYGON
};

//----------------------


/** ステータスバー作成 */

void StatusBar_new()
{
	StatusBar *p;

	p = (StatusBar *)mContainerNew(sizeof(StatusBar), M_WIDGET(APP_WIDGETS->mainwin));

	APP_WIDGETS->statusbar = p;

	//

	p->wg.fLayout = MLF_EXPAND_W;
	p->wg.draw = mWidgetHandleFunc_draw_drawBkgnd;
	p->wg.drawBkgnd = mWidgetHandleFunc_drawBkgnd_fillFace;
	p->ct.sepW = 3;

	mContainerSetType(M_CONTAINER(p), MCONTAINER_TYPE_HORZ, 0);
	mContainerSetPadding_one(M_CONTAINER(p), 1);

	//ラベル

	p->label_imginfo = mLabelCreate(M_WIDGET(p), MLABEL_S_BORDER, 0, 0, NULL);

	p->label_help = mLabelCreate(M_WIDGET(p), 0,
		MLF_MIDDLE | MLF_EXPAND_W, M_MAKE_DW4(2,0,0,0), NULL);

	//セット (初期時。または非表示 => 表示に切り替わった時に再設定)

	StatusBar_setImageInfo();
	StatusBar_setHelp_tool();
}

/** 表示状態切り替え */

void StatusBar_toggleVisible()
{
	if(APP_WIDGETS->statusbar)
	{
		//非表示時は、削除
		
		mWidgetDestroy(M_WIDGET(APP_WIDGETS->statusbar));
		APP_WIDGETS->statusbar = NULL;
	}
	else
	{
		//表示
		
		StatusBar_new();
	}

	APP_CONF->fView ^= CONFIG_VIEW_F_STATUSBAR;

	mWidgetReLayout(M_WIDGET(APP_WIDGETS->mainwin));
}

/** プログレスバーを表示する位置を取得
 *
 * @param pt  Y は下端
 * @return FALSE でステータスバーは非表示 */

mBool StatusBar_getProgressBarPos(mPoint *pt)
{
	StatusBar *p = APP_WIDGETS->statusbar;
	mBox box;

	if(p)
	{
		mWidgetGetRootBox(M_WIDGET(p->label_help), &box);
		pt->x = box.x;

		mWidgetGetRootBox(M_WIDGET(p), &box);
		pt->y = box.y + box.h;

		return TRUE;
	}

	return FALSE;
}


/** 画像情報セット */

void StatusBar_setImageInfo()
{
	StatusBar *p = APP_WIDGETS->statusbar;
	mStr str = MSTR_INIT;

	if(p)
	{
		mStrSetFormat(&str, "%d x %d %d DPI",
			APP_DRAW->imgw, APP_DRAW->imgh, APP_DRAW->imgdpi);

		mLabelSetText(p->label_imginfo, str.buf);

		mStrFree(&str);

		mWidgetReLayout(M_WIDGET(p));
	}
}

/** ツールのヘルプセット */

void StatusBar_setHelp_tool()
{
	StatusBar *p = APP_WIDGETS->statusbar;
	int no,sub,id = -1;
	char drawtype[] = {
		TRID_LINE45, TRID_DRAWTYPE_BOX, TRID_DRAWTYPE_CIRCLE,
		TRID_DRAWTYPE_SUCCLINE, TRID_DRAWTYPE_CONCLINE, TRID_DRAWTYPE_BEZIER,
		TRID_DRAWTYPE_SPLINE
	},
	fillpoly[] = {
		TRID_DRAWTYPE_BOX, TRID_DRAWTYPE_CIRCLE, TRID_POLYGON, -1
	};

	if(!p) return;

	no = APP_DRAW->tool.no;
	sub = APP_DRAW->tool.subno[no];

	//文字列ID

	switch(no)
	{
		//ブラシ
		case TOOL_BRUSH:
			id = (sub == TOOLSUB_DRAW_FREE)? TRID_BRUSH_FREE: drawtype[sub - 1];
			break;
		//図形塗り
		case TOOL_FILL_POLYGON:
		case TOOL_FILL_POLYGON_ERASE:
			id = fillpoly[sub];
			break;
		//塗りつぶし
		case TOOL_FILL:
			id = TRID_FILL;
			break;
		//テキスト
		case TOOL_TEXT:
			id = TRID_TEXT;
			break;
		//移動
		case TOOL_MOVE:
			id = TRID_MOVE;
			break;
		//自由選択
		case TOOL_MAGICWAND:
			id = TRID_FILL;
			break;
		//選択範囲
		case TOOL_SELECT:
			if(sub < TOOLSUB_SEL_IMGMOVE)
				id = TRID_SELECT;
			else if(sub < TOOLSUB_SEL_POSMOVE)
				id = TRID_SEL_MOVECOPY;
			else
				id = TRID_MOVE;
			break;
		//矩形選択
		case TOOL_BOXEDIT:
			id = TRID_DRAWTYPE_BOX;
			break;
		//スタンプ
		case TOOL_STAMP:
			id = TRID_STAMP;
			break;
		//グラデーション、キャンバス回転
		case TOOL_GRADATION:
		case TOOL_CANVAS_ROTATE:
			id = TRID_LINE45;
			break;
		//スポイト
		case TOOL_SPOIT:
			id = TRID_SPOIT;
			break;
	}

	//セット

	if(id == -1)
		mLabelSetText(p->label_help, NULL);
	else
		mLabelSetText(p->label_help, M_TR_T2(TRGROUP_STATUSBAR_HELP, id));
}

