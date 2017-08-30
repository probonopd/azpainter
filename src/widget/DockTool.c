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
 * DockTool
 *
 * [Dock]ツール
 *****************************************/

#include "mDef.h"
#include "mContainerDef.h"
#include "mWidget.h"
#include "mDockWidget.h"
#include "mContainer.h"
#include "mEvent.h"
#include "mIconButtons.h"

#include "defWidgets.h"
#include "defDraw.h"
#include "defConfig.h"
#include "DockObject.h"
#include "AppResource.h"

#include "draw_main.h"
#include "draw_boxedit.h"

#include "trgroup.h"


//-------------------

typedef struct
{
	DockObject obj;

	mIconButtons *ibtool,*ibsub;
}DockTool;

//-------------------

#define _DOCKTOOL(p)  ((DockTool *)(p))
#define _DT_PTR       ((DockTool *)APP_WIDGETS->dockobj[DOCKWIDGET_TOOL])

#define CMDID_TOOL     100
#define CMDID_TOOLSUB  200

//-------------------


//========================
// sub
//========================


/** ツールごとのサブアイコンセット */

static void _set_subtype_ib(DockTool *p)
{
	int i,toolno,subno,img_top,ttip_top,bttnum;
	uint32_t flags,flags_base;
	uint8_t *pimgno,*pttip,
		imgno_sel[] = {12,14,15,16,17,18,255},
		imgno_stamp[] = {12,14,15,255},
		ttip_sel[] = {100,102,103,200,201,202},
		ttip_stamp[] = {100,102,103};

	mIconButtonsDeleteAll(p->ibsub);

	toolno = APP_DRAW->tool.no;
	subno = APP_DRAW->tool.subno[toolno];

	if(toolno == TOOL_SELECT || toolno == TOOL_STAMP)
	{
		//------ 選択範囲、スタンプ

		if(toolno == TOOL_SELECT)
			pimgno = imgno_sel, pttip = ttip_sel;
		else
			pimgno = imgno_stamp, pttip = ttip_stamp;
		
		for(i = 0; *pimgno != 255; i++, pimgno++, pttip++)
		{
			mIconButtonsAdd(p->ibsub, CMDID_TOOLSUB + i,
				*pimgno, *pttip,
				MICONBUTTONS_ITEMF_CHECKGROUP | (i == subno? MICONBUTTONS_ITEMF_CHECKED: 0));
		}
	}
	else
	{
		//------ 値が連続しているもの
	
		ttip_top = -1;
		bttnum = 0;
		flags_base = MICONBUTTONS_ITEMF_CHECKGROUP;

		switch(toolno)
		{
			//ブラシ
			case TOOL_BRUSH:
				bttnum = TOOLSUB_DRAW_NUM;
				img_top = ttip_top = 0;
				break;
			//図形塗りつぶし/消去
			case TOOL_FILL_POLYGON:
			case TOOL_FILL_POLYGON_ERASE:
				bttnum = 4;
				img_top = TOOLSUB_DRAW_NUM + 4;
				ttip_top = 100;
				break;
			//グラデーション
			case TOOL_GRADATION:
				bttnum = 4;
				img_top = TOOLSUB_DRAW_NUM;
				break;
			//矩形編集
			case TOOL_BOXEDIT:
				bttnum = 6;
				img_top = 19;
				ttip_top = 300;
				flags_base = MICONBUTTONS_ITEMF_BUTTON;
				break;
		}

		for(i = 0; i < bttnum; i++)
		{
			flags = flags_base;

			if(flags == MICONBUTTONS_ITEMF_CHECKGROUP && i == subno)
				flags |= MICONBUTTONS_ITEMF_CHECKED;
		
			mIconButtonsAdd(p->ibsub, CMDID_TOOLSUB + i, img_top + i,
				(ttip_top < 0)? -1: ttip_top + i, flags);
		}
	}
}


//========================
// ハンドラ
//========================


/** イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_COMMAND)
	{
		int id = ev->cmd.id;

		if(id >= CMDID_TOOL && id < CMDID_TOOL + TOOL_NUM)
			//ツール
			drawTool_setTool(APP_DRAW, id - CMDID_TOOL);

		else if(id >= CMDID_TOOLSUB && id < CMDID_TOOLSUB + 100)
		{
			//サブタイプ

			id -= CMDID_TOOLSUB;

			if(APP_DRAW->tool.no == TOOL_BOXEDIT)
				drawBoxEdit_run(APP_DRAW, id);
			else
				drawTool_setToolSubtype(APP_DRAW, id);
		}
	}

	return 1;
}

/** サイズ変更時 */

static void _onsize_handle(mWidget *wg)
{
	DockTool *p = _DOCKTOOL(wg->param);

	mIconButtonsCalcHintSize(p->ibtool);
	mIconButtonsCalcHintSize(p->ibsub);
}

/** mDockWidget ハンドラ : 内容作成 */

static mWidget *_dock_func_create(mDockWidget *dockwg,int id,mWidget *parent)
{
	DockTool *p = _DT_PTR;
	mWidget *ct;
	mIconButtons *ib;
	int i;

	//コンテナ

	ct = mContainerCreate(parent, MCONTAINER_TYPE_VERT, 0, 0, MLF_EXPAND_WH);

	mContainerSetPadding_one(M_CONTAINER(ct), 2);

	ct->event = _event_handle;
	ct->onSize = _onsize_handle;
	ct->notifyTarget = MWIDGET_NOTIFYTARGET_SELF;
	ct->param = (intptr_t)p;

	/* mIconButtons の hintW,H が最小サイズとして適用されないようにする */
	ct->hintOverW = ct->hintOverH = 1;

	//---- ツール

	p->ibtool = ib = mIconButtonsNew(0, ct,
		MICONBUTTONS_S_TOOLTIP | MICONBUTTONS_S_AUTO_WRAP | MICONBUTTONS_S_DESTROY_IMAGELIST
		| MICONBUTTONS_S_SEP_BOTTOM);

	ib->wg.fLayout = MLF_EXPAND_W;

	mIconButtonsSetImageList(ib, AppResource_loadIconImage("tool.png", APP_CONF->iconsize_other));
	mIconButtonsSetTooltipTrGroup(ib, TRGROUP_TOOL);

	for(i = 0; i < TOOL_NUM; i++)
		mIconButtonsAdd(ib, CMDID_TOOL + i, i, i, MICONBUTTONS_ITEMF_CHECKGROUP);

	mIconButtonsSetCheck(ib, CMDID_TOOL + APP_DRAW->tool.no, 1);

	//---- サブ

	p->ibsub = ib = mIconButtonsNew(0, ct,
		MICONBUTTONS_S_TOOLTIP | MICONBUTTONS_S_AUTO_WRAP | MICONBUTTONS_S_DESTROY_IMAGELIST);

	ib->wg.fLayout = MLF_EXPAND_W;

	mIconButtonsSetImageList(ib, AppResource_loadIconImage("toolsub.png", APP_CONF->iconsize_other));
	mIconButtonsSetTooltipTrGroup(ib, TRGROUP_TOOL_SUB);

	_set_subtype_ib(p);

	return ct;
}


//========================


/** 作成 */

void DockTool_new(mDockWidgetState *state)
{
	DockObject *p;

	p = DockObject_new(DOCKWIDGET_TOOL, sizeof(DockTool), 0, 0,
			state, _dock_func_create);

	mDockWidgetCreateWidget(p->dockwg);
}

/** アイコンサイズ変更 */

void DockTool_changeIconSize()
{
	DockTool *p = _DT_PTR;

	if(DockObject_canDoWidget((DockObject *)p))
	{
		mIconButtonsReplaceImageList(p->ibtool,
			AppResource_loadIconImage("tool.png", APP_CONF->iconsize_other));

		mIconButtonsReplaceImageList(p->ibsub,
			AppResource_loadIconImage("toolsub.png", APP_CONF->iconsize_other));

		DockObject_relayout_inWindowMode(DOCKWIDGET_TOOL);
	}
}

/** ツール変更時 */

void DockTool_changeTool()
{
	DockTool *p = _DT_PTR;

	if(DockObject_canDoWidget((DockObject *)p))
	{
		//ツール

		mIconButtonsSetCheck(p->ibtool, CMDID_TOOL + APP_DRAW->tool.no, 1);

		//サブタイプ

		_set_subtype_ib(p);
	}
}

/** サブタイプ変更時 */

void DockTool_changeToolSub()
{
	DockTool *p = _DT_PTR;

	if(DockObject_canDoWidget((DockObject *)p)
		&& APP_DRAW->tool.no != TOOL_BOXEDIT)
	{
		//チェックボタン
	
		mIconButtonsSetCheck(p->ibsub,
			CMDID_TOOLSUB + APP_DRAW->tool.subno[APP_DRAW->tool.no], 1);
	}
}
