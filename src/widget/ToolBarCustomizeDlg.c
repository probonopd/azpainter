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
 * ToolBarCustomizeDlg
 *
 * ツールバーカスタマイズダイアログ
 *****************************************/

#include "mDef.h"
#include "mWidget.h"
#include "mDialog.h"
#include "mContainer.h"
#include "mWindow.h"
#include "mListView.h"
#include "mWidgetBuilder.h"
#include "mTrans.h"
#include "mEvent.h"

#include "AppResource.h"

#include "trgroup.h"


//----------------------

typedef struct
{
	mWidget wg;
	mContainerData ct;
	mWindowData win;
	mDialogData dlg;

	mListView *list[2];
}_dialog;

//----------------------

static const char *g_wgbuild =
"ct#g3:lf=wh:sep=6,6;"
  "lv#,vf:id=100:lf=wh;"
  "ct#v:lf=my:sep=15;"
    "arrbt#lf:id=102;"
    "arrbt#rf:id=103;"
  "-;"
  "lv#,vf:id=101:lf=wh;"

  "ct#h:sep=6;"
    "arrbt#uf:id=104;"
    "arrbt#f:id=105;"
;

//----------------------

#define BTTNO_END 255
#define BTTNO_SEP 254

#define TRID_SEP  1

enum
{
	WID_LIST_LEFT = 100,
	WID_LIST_RIGHT,

	WID_TO_LEFT,
	WID_TO_RIGHT,

	WID_LEFT_UP,
	WID_LEFT_DOWN
};

//----------------------


/** 初期リストセット */

static void _init_list(_dialog *p,uint8_t *buf)
{
	mListView *list;
	const uint8_t *btts;
	const char *txt_sep;
	int i,j,no;

	txt_sep = M_TR_T2(TRGROUP_DLG_TOOLBAR_CUSTOMIZE, TRID_SEP);

	M_TR_G(TRGROUP_TOOLBAR_TOOLTIP);

	btts = buf;
	if(!btts) btts = AppResource_getToolbarBtt_default();

	//表示するボタン

	list = p->list[0];

	for(i = 0; btts[i] != BTTNO_END; i++)
	{
		no = btts[i];

		if(no == BTTNO_SEP)
			mListViewAddItem(list, txt_sep, -1, MLISTVIEW_ITEM_F_STATIC_TEXT, BTTNO_SEP);
		else
			mListViewAddItem(list, M_TR_T(no), -1, MLISTVIEW_ITEM_F_STATIC_TEXT, no);
	}

	//表示しないボタン

	list = p->list[1];

	mListViewAddItem(list, txt_sep, -1, MLISTVIEW_ITEM_F_STATIC_TEXT, BTTNO_SEP);

	for(i = 0; i < APP_RESOURCE_TOOLBAR_ICON_NUM; i++)
	{
		//btts 内にあるか

		for(j = 0; btts[j] != BTTNO_END && btts[j] != i; j++);

		if(btts[j] == BTTNO_END)
			mListViewAddItem(list, M_TR_T(i), -1, MLISTVIEW_ITEM_F_STATIC_TEXT, i);
	}
}

/** リストの選択項目を左右に移動
 *
 * @param listno 移動元のリスト番号 */

static void _move_list(_dialog *p,int listno)
{
	mListView *listsrc,*listdst;
	mListViewItem *src,*dst;
	int srcno;

	listsrc = p->list[listno];
	listdst = p->list[!listno];

	//アイテム

	src = mListViewGetFocusItem(listsrc);
	if(!src) return;
	
	dst = mListViewGetFocusItem(listdst);

	srcno = src->param;

	//

	if(listno == 0)
	{
		//左 -> 右
		/* 区切りは追加しない。また、常に終端へ移動。
		 * 左は削除後の次のアイテムを選択。 */

		if(srcno != BTTNO_SEP)
			mListViewAddItem(listdst, src->text, -1, MLISTVIEW_ITEM_F_STATIC_TEXT, srcno);

		mListViewDeleteItem_sel(listsrc, src);
	}
	else
	{
		//左 <- 右
		/* 左の選択位置へ挿入。選択位置がない場合は終端。
		 * 次を続けて移動しやすいように、移動後の次のアイテムを選択。
		 * 右が区切りの場合は削除しない。 */
		
		dst = mListViewInsertItem(listdst, dst, src->text, -1, MLISTVIEW_ITEM_F_STATIC_TEXT, srcno);

		if(dst)
			mListViewSetFocusItem(listdst, M_LISTVIEWITEM(dst->i.next));

		if(srcno != BTTNO_SEP)
			mListViewDeleteItem_sel(listsrc, src);
	}
}


//==========================


/** イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY)
	{
		_dialog *p = (_dialog *)wg;
		
		switch(ev->notify.id)
		{
			//右から左へ
			case WID_TO_LEFT:
				_move_list(p, 1);
				break;
			//左から右へ
			case WID_TO_RIGHT:
				_move_list(p, 0);
				break;
			//up/down
			case WID_LEFT_UP:
			case WID_LEFT_DOWN:
				mListViewMoveItem_updown(p->list[0], NULL, (ev->notify.id == WID_LEFT_DOWN));
				break;
		}
	}

	return mDialogEventHandle_okcancel(wg, ev);
}

/** 作成 */

static _dialog *_create_dialog(mWindow *owner,uint8_t *bttbuf)
{
	_dialog *p;

	p = (_dialog *)mDialogNew(sizeof(_dialog), owner, MWINDOW_S_DIALOG_NORMAL);
	if(!p) return NULL;

	p->wg.event = _event_handle;

	//

	mContainerSetPadding_one(M_CONTAINER(p), 10);

	p->ct.sepW = 18;

	M_TR_G(TRGROUP_DLG_TOOLBAR_CUSTOMIZE);

	mWindowSetTitle(M_WINDOW(p), M_TR_T(0));

	//ウィジェット

	mWidgetBuilderCreateFromText(M_WIDGET(p), g_wgbuild);

	p->list[0] = (mListView *)mWidgetFindByID(M_WIDGET(p), WID_LIST_LEFT);
	p->list[1] = (mListView *)mWidgetFindByID(M_WIDGET(p), WID_LIST_RIGHT);
	
	//リストセット

	_init_list(p, bttbuf);

	mWidgetSetInitSize_fontHeight(M_WIDGET(p->list[0]), 15, 20);
	mWidgetSetInitSize_fontHeight(M_WIDGET(p->list[1]), 15, 20);

	//OK/cancel

	mContainerCreateOkCancelButton(M_WIDGET(p));

	return p;
}

/** OK */

static void _set_ok(_dialog *p,uint8_t **ppdst,int *pdstsize)
{
	mListViewItem *pi;
	int num;
	uint8_t *buf,*pd;

	num = mListViewGetItemNum(p->list[0]);

	buf = (uint8_t *)mMalloc(num + 1, FALSE);
	if(!buf) return;

	//セット

	pd = buf;
	pi = mListViewGetTopItem(p->list[0]);

	for(; pi; pi = M_LISTVIEWITEM(pi->i.next))
		*(pd++) = pi->param;

	*pd = BTTNO_END;

	//

	mFree(*ppdst);
	
	*ppdst = buf;
	*pdstsize = num + 1;
}

/** ダイアログ実行 */

mBool ToolBarCustomizeDlg_run(mWindow *owner,uint8_t **ppdst,int *pdstsize)
{
	_dialog *p;
	int ret;

	p = _create_dialog(owner, *ppdst);
	if(!p) return FALSE;

	mWindowMoveResizeShow_hintSize(M_WINDOW(p));

	ret = mDialogRun(M_DIALOG(p), FALSE);

	if(ret) _set_ok(p, ppdst, pdstsize);

	mWidgetDestroy(M_WIDGET(p));
	
	return ret;
}
