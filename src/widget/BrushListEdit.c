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
 * ブラシリスト編集ダイアログ
 *****************************************/

#include "mDef.h"
#include "mWidget.h"
#include "mDialog.h"
#include "mContainer.h"
#include "mWindow.h"
#include "mLabel.h"
#include "mButton.h"
#include "mListView.h"
#include "mIconButtons.h"
#include "mMessageBox.h"
#include "mSysDialog.h"
#include "mTrans.h"
#include "mEvent.h"
#include "mStr.h"
#include "mMenu.h"
#include "mImageList.h"

#include "defConfig.h"
#include "BrushItem.h"
#include "BrushList.h"
#include "AppResource.h"

#include "trgroup.h"
#include "trid_word.h"



//----------------------

typedef struct
{
	mWidget wg;
	mContainerData ct;
	mWindowData win;
	mDialogData dlg;

	mImageList *imglist;

	mListView *list_group,
		*list_brush;
	mIconButtons *ib_brush;

	BrushItem copyitem;	//コピーされたアイテム
}_dialog;

//----------------------

enum
{
	TRID_GROUP = 1,
	TRID_BRUSH,

	TRID_TTIP_TOP = 100,

	TRID_BRUSHNAME_DLG_TITLE = 500,

	TRID_MES_DELETE_GROUP = 1000,
	TRID_MES_NOTDELETE_GROUP
};

enum
{
	WID_LIST_GROUP = 100,
	WID_LIST_BRUSH,

	CMDID_GROUP_TOP = 100,
	CMDID_BRUSH_TOP = 200,
	
	CMDID_ADD = 0,
	CMDID_DEL,
	CMDID_EDIT,
	CMDID_COPY,
	CMDID_PASTE,
	CMDID_UP,
	CMDID_DOWN,
	CMDID_MOVE_GROUP,

	CMDID_MAX
};

//----------------------

mBool GroupOptionDlg_run(mWindow *owner,int type,mStr *strname,int *colnum);

//----------------------



//=========================
// sub
//=========================


/** グループリストセット */

static void _setlist_group(_dialog *p)
{
	BrushGroupItem *gi;

	for(gi = BrushList_getTopGroup(); gi; gi = BRUSHGROUPITEM(gi->i.next))
		mListViewAddItem_textparam(p->list_group, gi->name, (intptr_t)gi);
}

/** ブラシリストセット */

static void _setlist_brush(_dialog *p,BrushGroupItem *group)
{
	BrushItem *pi;

	mListViewDeleteAllItem(p->list_brush);

	for(pi = BRUSHITEM(group->list.top); pi; pi = BRUSHITEM(pi->i.next))
		mListViewAddItem_textparam(p->list_brush, pi->name, (intptr_t)pi);
}


//=========================
// コマンド
//=========================


/** 新規グループ/編集
 *
 * @param group NULL で新規 */

static void _cmd_group_new_edit(_dialog *p,BrushGroupItem *group,mListViewItem *lvi)
{
	mStr str = MSTR_INIT;
	int colnum;
	mListViewItem *sel;

	if(group)
	{
		mStrSetText(&str, group->name);
		colnum = group->colnum;
	}

	if(GroupOptionDlg_run(p->wg.toplevel, (group != NULL), &str, &colnum))
	{
		if(group)
		{
			//変更

			mStrdup_ptr(&group->name, str.buf);
			group->colnum = colnum;

			mListViewSetItemText(p->list_group, lvi, str.buf);
		}
		else
		{
			//新規

			group = BrushList_newGroup(str.buf, colnum);

			if(group)
			{
				sel = mListViewAddItem_textparam(p->list_group, str.buf, (intptr_t)group);

				mListViewSetFocusItem(p->list_group, sel);

				_setlist_brush(p, group);
			}
		}
	}

	mStrFree(&str);
}

/** グループ削除 */

static void _cmd_group_delete(_dialog *p,BrushGroupItem *group,mListViewItem *lvi)
{
	mListViewItem *sel;

	M_TR_G(TRGROUP_DLG_BRUSHLISTEDIT);

	//グループがひとつだけの場合は削除不可

	if(BrushList_getGroupNum() == 1)
	{
		mMessageBox(M_WINDOW(p), NULL, M_TR_T(TRID_MES_NOTDELETE_GROUP),
			MMESBOX_OK, MMESBOX_OK);

		return;
	}

	//削除確認

	if(mMessageBox(M_WINDOW(p), NULL, M_TR_T(TRID_MES_DELETE_GROUP),
		MMESBOX_YESNO, MMESBOX_NO) != MMESBOX_YES)
		return;

	//削除
	
	BrushList_deleteGroup(group);

	//リスト

	sel = mListViewDeleteItem_sel(p->list_group, lvi);

	_setlist_brush(p, BRUSHGROUPITEM(sel->param));
}

/** ブラシ追加/貼り付け */

static void _cmd_brush_new(_dialog *p,BrushGroupItem *group,mBool paste)
{
	BrushItem *item;
	mListViewItem *lvi;

	if(paste)
		item = BrushList_pasteItem_new(group, &p->copyitem);
	else
		item = BrushList_newBrush(group);

	if(item)
	{
		lvi = mListViewAddItem_textparam(p->list_brush, item->name, (intptr_t)item);

		mListViewSetFocusItem(p->list_brush, lvi);
	}
}

/** ブラシ編集 */

static void _cmd_brush_edit(_dialog *p,BrushItem *item,mListViewItem *lvi)
{
	mStr str = MSTR_INIT;

	mStrSetText(&str, item->name);

	if(mSysDlgInputText(M_WINDOW(p),
		M_TR_T2(TRGROUP_DLG_BRUSHLISTEDIT, TRID_BRUSHNAME_DLG_TITLE),
		M_TR_T2(TRGROUP_WORD, TRID_WORD_NAME),
		&str, MSYSDLG_INPUTTEXT_F_NOEMPTY))
	{
		mStrdup_ptr(&item->name, str.buf);

		mListViewSetItemText(p->list_brush, lvi, str.buf);
	}

	mStrFree(&str);
}

/** ブラシ、グループ移動 */

static void _cmd_brush_movegroup(_dialog *p,BrushItem *item,mListViewItem *lvi)
{
	mMenu *menu;
	mMenuItemInfo *mi;
	BrushGroupItem *gi;
	mBox box;

	//メニュー

	menu = mMenuNew();

	for(gi = BrushList_getTopGroup(); gi; gi = BRUSHGROUPITEM(gi->i.next))
	{
		if(gi != item->group)
		{
			mi = mMenuAddText_static(menu, 0, gi->name);
			if(mi) mi->param1 = (intptr_t)gi;
		}
	}

	mIconButtonsGetItemBox(p->ib_brush, CMDID_BRUSH_TOP + CMDID_MOVE_GROUP, &box, TRUE);

	mi = mMenuPopup(menu, NULL, box.x, box.y + box.h, 0);
	gi = (mi)? BRUSHGROUPITEM(mi->param1): NULL;

	mMenuDestroy(menu);

	//移動 & アイテム削除

	if(gi)
	{
		BrushList_moveItem(item, gi, NULL);

		mListViewDeleteItem_sel(p->list_brush, lvi);
	}
}


//=========================
// イベント
//=========================


/** コマンドイベント */

static void _command_event(_dialog *p,int id)
{
	mBool groupcmd;
	mListViewItem *lvi_group,*lvi_brush;
	BrushGroupItem *group = NULL;
	BrushItem *brush = NULL;

	if(id >= CMDID_GROUP_TOP && id < CMDID_GROUP_TOP + CMDID_MAX)
		id -= CMDID_GROUP_TOP, groupcmd = TRUE;
	else if(id >= CMDID_BRUSH_TOP && id < CMDID_BRUSH_TOP + CMDID_MAX)
		id -= CMDID_BRUSH_TOP, groupcmd = FALSE;
	else
		return;

	//選択

	lvi_group = mListViewGetFocusItem(p->list_group);
	lvi_brush = mListViewGetFocusItem(p->list_brush);

	if(lvi_group) group = BRUSHGROUPITEM(lvi_group->param);
	if(lvi_brush) brush = BRUSHITEM(lvi_brush->param);
	
	//選択があるか

	if(id != CMDID_ADD && id != CMDID_PASTE)
	{
		if((groupcmd && !group) || (!groupcmd && !brush))
			return;
	}

	//

	switch(id)
	{
		//追加
		case CMDID_ADD:
			if(groupcmd)
				_cmd_group_new_edit(p, NULL, NULL);
			else
				_cmd_brush_new(p, group, FALSE);
			break;
		//削除
		case CMDID_DEL:
			if(groupcmd)
				_cmd_group_delete(p, group, lvi_group);
			else
			{
				BrushList_deleteItem(brush);

				mListViewDeleteItem_sel(p->list_brush, lvi_brush);
			}
			break;
		//編集
		case CMDID_EDIT:
			if(groupcmd)
				_cmd_group_new_edit(p, group, lvi_group);
			else
				_cmd_brush_edit(p, brush, lvi_brush);
			break;
		//上/下へ移動
		case CMDID_UP:
		case CMDID_DOWN:
			if(groupcmd)
			{
				BrushList_moveGroup_updown(group, (id == CMDID_UP));

				mListViewMoveItem_updown(p->list_group, lvi_group, (id == CMDID_DOWN));
			}
			else
			{
				BrushList_moveItem_updown(brush, (id == CMDID_UP));

				mListViewMoveItem_updown(p->list_brush, lvi_brush, (id == CMDID_DOWN));
			}
			break;
		//コピー
		case CMDID_COPY:
			BrushItem_copy(&p->copyitem, brush);

			//コピー済みのフラグ用にセット
			p->copyitem.group = (BrushGroupItem *)1;
			break;
		//貼り付け
		case CMDID_PASTE:
			if(p->copyitem.group)
				_cmd_brush_new(p, group, TRUE);
			break;
		//グループ移動
		case CMDID_MOVE_GROUP:
			if(BrushList_getGroupNum() != 1)
				_cmd_brush_movegroup(p, brush, lvi_brush);
			break;
	}
}

/** イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	switch(ev->type)
	{
		case MEVENT_NOTIFY:
			if(ev->notify.id == WID_LIST_GROUP
				&& ev->notify.type == MLISTVIEW_N_CHANGE_FOCUS)
			{
				//グループ選択変更

				_setlist_brush((_dialog *)wg, BRUSHGROUPITEM(ev->notify.param2));
			}
			else if(ev->notify.id == M_WID_OK)
				mDialogEnd(M_DIALOG(wg), TRUE);
			break;
	
		case MEVENT_COMMAND:
			_command_event((_dialog *)wg, ev->cmd.id);
			break;
	}

	return mDialogEventHandle(wg, ev);
}


//=======================


/** 破棄ハンドラ */

static void _destroy_handle(mWidget *wg)
{
	_dialog *p = (_dialog *)wg;

	mImageListFree(p->imglist);

	BrushItem_destroy_handle(M_LISTITEM(&p->copyitem));
}

/** アイコンボタン作成 */

static mIconButtons *_create_iconbuttons(_dialog *p,mWidget *parent,mBool brush)
{
	mIconButtons *ib;
	uint8_t imgno_group[] = {0,1,2,5,6};
	int i;

	ib = mIconButtonsNew(0, parent, MICONBUTTONS_S_VERT |  MICONBUTTONS_S_TOOLTIP);
	
	mIconButtonsSetImageList(ib, p->imglist);
	mIconButtonsSetTooltipTrGroup(ib, TRGROUP_DLG_BRUSHLISTEDIT);

	if(brush)
	{
		for(i = 0; i < 8; i++)
			mIconButtonsAdd(ib, i + CMDID_BRUSH_TOP, i, i + TRID_TTIP_TOP, MICONBUTTONS_ITEMF_BUTTON);
	}
	else
	{
		for(i = 0; i < 5; i++)
		{
			mIconButtonsAdd(ib, imgno_group[i] + CMDID_GROUP_TOP,
				imgno_group[i], imgno_group[i] + TRID_TTIP_TOP, MICONBUTTONS_ITEMF_BUTTON);
		}
	}

	mIconButtonsCalcHintSize(ib);

	return ib;
}	

/** 作成 */

static _dialog *_dialog_create(mWindow *owner)
{
	_dialog *p;
	mWidget *cth,*ctv,*cth2;
	mListView *lv;

	p = (_dialog *)mDialogNew(sizeof(_dialog), owner, MWINDOW_S_DIALOG_NORMAL);
	if(!p) return NULL;

	p->wg.destroy = _destroy_handle;
	p->wg.event = _event_handle;

	//

	mContainerSetPadding_one(M_CONTAINER(p), 8);

	p->ct.sepW = 18;

	M_TR_G(TRGROUP_DLG_BRUSHLISTEDIT);

	mWindowSetTitle(M_WINDOW(p), M_TR_T(0));

	//イメージリスト

	p->imglist = AppResource_loadIconImage("brushedit.png", APP_CONF->iconsize_other);

	//-------

	cth = mContainerCreate(M_WIDGET(p), MCONTAINER_TYPE_HORZ, 0, 12, MLF_EXPAND_WH);

	//--- グループ

	ctv = mContainerCreate(cth, MCONTAINER_TYPE_VERT, 0, 2, MLF_EXPAND_WH);

	mLabelCreate(ctv, 0, 0, 0, M_TR_T(TRID_GROUP));

	cth2 = mContainerCreate(ctv, MCONTAINER_TYPE_HORZ, 0, 3, MLF_EXPAND_WH);

	//リスト

	p->list_group = lv = mListViewNew(0, cth2, MLISTVIEW_S_AUTO_WIDTH, MSCROLLVIEW_S_HORZVERT_FRAME);

	lv->wg.id = WID_LIST_GROUP;
	lv->wg.fLayout = MLF_EXPAND_WH;

	mWidgetSetInitSize_fontHeight(M_WIDGET(lv), 13, 18);

	//ボタン

	_create_iconbuttons(p, cth2, FALSE);

	//---- ブラシ

	ctv = mContainerCreate(cth, MCONTAINER_TYPE_VERT, 0, 2, MLF_EXPAND_WH);

	mLabelCreate(ctv, 0, 0, 0, M_TR_T(TRID_BRUSH));

	cth2 = mContainerCreate(ctv, MCONTAINER_TYPE_HORZ, 0, 3, MLF_EXPAND_WH);

	//リスト

	p->list_brush = lv = mListViewNew(0, cth2, MLISTVIEW_S_AUTO_WIDTH, MSCROLLVIEW_S_HORZVERT_FRAME);

	lv->wg.id = WID_LIST_BRUSH;
	lv->wg.fLayout = MLF_EXPAND_WH;

	mWidgetSetInitSize_fontHeight(M_WIDGET(lv), 13, 18);

	//ボタン

	p->ib_brush = _create_iconbuttons(p, cth2, TRUE);

	//OK

	mButtonCreate(M_WIDGET(p), M_WID_OK, 0, MLF_RIGHT, 0, M_TR_T2(M_TRGROUP_SYS, M_TRSYS_OK));

	//-------

	//リストセット

	_setlist_group(p);
	_setlist_brush(p, BrushList_getTopGroup());

	//グループ選択

	mListViewSetFocusItem_findParam(p->list_group, (intptr_t)BrushList_getTopGroup());

	return p;
}

/** ブラシリスト編集ダイアログ */

void BrushListEditDlg_run(mWindow *owner)
{
	_dialog *p;

	p = _dialog_create(owner);
	if(!p) return;

	mWindowMoveResizeShow_hintSize(M_WINDOW(p));

	mDialogRun(M_DIALOG(p), TRUE);
}
