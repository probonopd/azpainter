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
 * DockImageViewer
 *
 * [dock] イメージビューア
 *****************************************/

#include <stdio.h>
#include <string.h>

#include "mDef.h"
#include "mWindowDef.h"
#include "mWidget.h"
#include "mContainer.h"
#include "mDockWidget.h"
#include "mIconButtons.h"
#include "mEvent.h"
#include "mSysDialog.h"
#include "mStr.h"
#include "mMenu.h"
#include "mDirEntry.h"
#include "mList.h"
#include "mTrans.h"

#include "defWidgets.h"
#include "defConfig.h"
#include "defMacros.h"

#include "ImageBuf24.h"
#include "PopupSliderZoom.h"
#include "DockImageViewer.h"
#include "AppResource.h"

#include "trgroup.h"


//-------------------

#define TRID_TOOLTIP_TOP  100

//ツールバーのID
enum
{
	ICONBTT_NO_MENU,
	ICONBTT_NO_OPEN,
	ICONBTT_NO_PREV,
	ICONBTT_NO_NEXT,
	ICONBTT_NO_ZOOM,
	ICONBTT_NO_FIT,
	ICONBTT_NO_MIRROR
};

//メニューのID
enum
{
	CMDID_OPEN,
	CMDID_PREV,
	CMDID_NEXT,
	CMDID_CLEAR,
	CMDID_OPTION,
	CMDID_FIT,
	CMDID_MIRROR,
	CMDID_ZOOM_TOP = 1000
};

#define _IS_ZOOM_FIT  (APP_CONF->imageviewer_flags & CONFIG_IMAGEVIEWER_F_FIT)

//-------------------

DockImageViewerArea *DockImageViewerArea_new(DockImageViewer *imgviewer,mWidget *parent);

void DockCanvasView_runOptionDialog(mWindow *owner,mBool imageviewer);

//-------------------



//==============================
// sub - etc
//==============================


/** スクロール位置調整 */

void DockImageViewer_adjustScroll(DockImageViewer *p)
{
	int w,h,w2,h2,aw,ah,x1,y1,x2,y2;

	if(!p->img) return;

	w = p->img->w;
	h = p->img->h;
	w2 = w >> 1;
	h2 = h >> 1;
	aw = M_WIDGET(p->area)->w >> 1;
	ah = M_WIDGET(p->area)->h >> 1;

	//イメージの左上/右下のキャンバス位置

	x1 = -w2 * p->dscale + aw - p->scrx;
	y1 = -h2 * p->dscale + ah - p->scry;
	x2 = (w - w2) * p->dscale + aw - p->scrx;
	y2 = (h - h2) * p->dscale + ah - p->scry;

	//調整

	if(x1 > aw)
		p->scrx = -w2 * p->dscale;
	else if(x2 < aw)
		p->scrx = (w - w2) * p->dscale;

	if(y1 > ah)
		p->scry = -h2 * p->dscale;
	else if(y2 < ah)
		p->scry = (h - h2) * p->dscale;
}

/** 倍率をウィンドウに合わせる */

void DockImageViewer_setZoom_fit(DockImageViewer *p)
{
	if(p->img && _IS_ZOOM_FIT)
	{
		double h,v;

		//倍率が低い方 (等倍以上にはしない)

		h = (double)M_WIDGET(p->area)->w / p->img->w;
		v = (double)M_WIDGET(p->area)->h / p->img->h;

		if(h > v) h = v;
		if(h < 0.02) h = 0.02; else if(h > 1.0) h = 1.0;

		p->zoom = (int)(h * 100 + 0.5);
		p->dscale = h;
		p->dscalediv = 1.0 / h;

		p->scrx = p->scry = 0;
	}
}

/** 表示倍率セット
 *
 * @param zoom 100 = 等倍 */

mBool DockImageViewer_setZoom(DockImageViewer *p,int zoom)
{
	if(zoom == p->zoom || _IS_ZOOM_FIT)
		return FALSE;
	else
	{
		p->zoom = zoom;
		p->dscale = (double)zoom / 100;
		p->dscalediv = 1.0 / p->dscale;

		DockImageViewer_adjustScroll(p);

		return TRUE;
	}
}

/** エリア更新 */

static void _update_area(DockImageViewer *p)
{
	DockImageViewer_setZoom_fit(p);
	
	mWidgetUpdate(M_WIDGET(p->area));
}

/** アイコンボタンの有効/無効セット */

static void _enable_iconbutton(DockImageViewer *p)
{
	//表示倍率 (全体表示ON時は無効)
	
	mIconButtonsSetEnable(M_ICONBUTTONS(p->iconbtt), ICONBTT_NO_ZOOM, !_IS_ZOOM_FIT);
}


//==============================
// sub - image
//==============================


/** 画像をクリア */

static void _clear_image(DockImageViewer *p)
{
	ImageBuf24_free(p->img);
	p->img = NULL;

	mStrEmpty(&p->strFileName);

	_update_area(p);
}

/** 画像読み込み */

void DockImageViewer_loadImage(DockImageViewer *p,const char *filename)
{
	ImageBuf24_free(p->img);

	//読み込み

	p->img = ImageBuf24_loadFile(filename);

	if(!p->img)
		_clear_image(p);
	else
	{
		//ファイル名セット
		
		mStrSetText(&p->strFileName, filename);

		//ディレクトリ記録

		mStrPathGetDir(&APP_CONF->strImageViewerDir, filename);

		//更新

		p->scrx = p->scry = 0;

		_update_area(p);
	}
}


//==========================
// 前/次のファイル
//==========================


typedef struct
{
	mListItem i;
	char *fname;
}_FILELISTITEM;


static void _destroy_filelist(mListItem *p)
{
	mFree(((_FILELISTITEM *)p)->fname);
}

static int _sortfunc_filelist(mListItem *p1,mListItem *p2,intptr_t param)
{
	return strcmp(((_FILELISTITEM *)p1)->fname, ((_FILELISTITEM *)p2)->fname);
}

/** 前/次のファイル */

static void _move_file(DockImageViewer *p,mBool next)
{
	mDirEntry *dir;
	mStr str = MSTR_INIT,curfname = MSTR_INIT;
	mList list = MLIST_INIT;
	_FILELISTITEM *pi,*pi_curfile = NULL;

	if(mStrIsEmpty(&p->strFileName)) return;

	//現在のファイル名

	mStrPathGetFileName(&curfname, p->strFileName.buf);

	//-------- ファイルの列挙

	mStrPathGetDir(&str, p->strFileName.buf);

	dir = mDirEntryOpen(str.buf);
	if(!dir) goto END;

	while(mDirEntryRead(dir))
	{
		if(mDirEntryIsDirectory(dir))
			continue;

		mDirEntryGetFileName_str(dir, &str, FALSE);

		if(mStrPathCompareExts(&str, "png:jpg:jpeg:gif:bmp"))
		{
			pi = (_FILELISTITEM *)mListAppendNew(&list, sizeof(_FILELISTITEM), _destroy_filelist);
			if(pi)
			{
				pi->fname = mStrdup(str.buf);

				//現在のファイルか

				if(!pi_curfile && mStrPathCompareEq(&curfname, pi->fname))
					pi_curfile = pi;
			}
		}
	}

	mDirEntryClose(dir);

	//------- 読み込み

	if(pi_curfile)
	{
		//ソート
		
		mListSort(&list, _sortfunc_filelist, 0);

		//前/次のアイテム

		if(next)
			pi = (_FILELISTITEM *)pi_curfile->i.next;
		else
			pi = (_FILELISTITEM *)pi_curfile->i.prev;

		//読み込み

		if(pi)
		{
			mStrPathGetDir(&str, p->strFileName.buf);
			mStrPathAdd(&str, pi->fname);

			DockImageViewer_loadImage(p, str.buf);
		}
	}

	//---- 終了

END:
	mListDeleteAll(&list);
	mStrFree(&str);
	mStrFree(&curfname);
}


//==============================
// コマンド
//==============================


/** 全体表示切り替え */

static void _cmd_toggle_fitview(DockImageViewer *p)
{
	APP_CONF->imageviewer_flags ^= CONFIG_IMAGEVIEWER_F_FIT;

	_enable_iconbutton(p);
	_update_area(p);
}

/** 左右反転切り替え */

static void _cmd_toggle_mirror(DockImageViewer *p)
{
	APP_CONF->imageviewer_flags ^= CONFIG_IMAGEVIEWER_F_MIRROR;
	_update_area(p);
}

/** 開く */

static void _cmd_open(DockImageViewer *p)
{
	mStr str = MSTR_INIT;

	if(!mSysDlgOpenFile(DockObject_getOwnerWindow((DockObject *)p),
		FILEFILTER_NORMAL_IMAGE,
		0, APP_CONF->strImageViewerDir.buf, 0, &str))
		return;

	DockImageViewer_loadImage(p, str.buf);

	mStrFree(&str);
}

/** メニュー表示 */

static void _run_menu(DockImageViewer *p)
{
	mMenu *menu;
	mMenuItemInfo *mi;
	mBox box;
	uint16_t dat[] = {
		CMDID_OPEN, CMDID_PREV, CMDID_NEXT, CMDID_CLEAR, 0xfffe,
		CMDID_OPTION, 0xfffe,
		CMDID_FIT|0x8000, CMDID_MIRROR|0x8000, 0xffff
	};
	uint16_t zoom[] = {25,50,100,200,400,600,800,0};
	int i,id,zoom_fit;
	char m[16];

	zoom_fit = _IS_ZOOM_FIT;

	M_TR_G(TRGROUP_DOCK_IMAGEVIEWER);

	//---- メニュー作成

	menu = mMenuNew();

	mMenuAddTrArray16(menu, dat);

	//チェック

	mMenuSetCheck(menu, CMDID_FIT, zoom_fit);
	mMenuSetCheck(menu, CMDID_MIRROR, APP_CONF->imageviewer_flags & CONFIG_IMAGEVIEWER_F_MIRROR);

	//倍率

	mMenuAddSep(menu);

	for(i = 0; zoom[i]; i++)
	{
		snprintf(m, 16, "%d%%", zoom[i]);

		mMenuAddNormal(menu, CMDID_ZOOM_TOP + zoom[i], m, 0,
			MMENUITEM_F_LABEL_COPY | ((zoom_fit)? MMENUITEM_F_DISABLE: 0));
	}

	//---- 実行

	mIconButtonsGetItemBox(M_ICONBUTTONS(p->iconbtt), 0, &box, TRUE);

	mi = mMenuPopup(menu, NULL, box.x + box.w, box.y, 0);

	id = (mi)? mi->id: -1;

	mMenuDestroy(menu);

	if(id < 0) return;

	//---- コマンド

	if(id >= CMDID_ZOOM_TOP)
	{
		//倍率指定

		if(DockImageViewer_setZoom(p, id - CMDID_ZOOM_TOP))
			_update_area(p);
	}
	else
	{
		switch(id)
		{
			case CMDID_OPEN:
				_cmd_open(p);
				break;
			case CMDID_PREV:
				_move_file(p, FALSE);
				break;
			case CMDID_NEXT:
				_move_file(p, TRUE);
				break;
			case CMDID_CLEAR:
				_clear_image(p);
				break;
			//操作設定
			case CMDID_OPTION:
				DockCanvasView_runOptionDialog(
					DockObject_getOwnerWindow(APP_WIDGETS->dockobj[DOCKWIDGET_IMAGE_VIEWER]),
					TRUE);
				break;
			//全体表示
			case CMDID_FIT:
				_cmd_toggle_fitview(p);

				mIconButtonsSetCheck(M_ICONBUTTONS(p->iconbtt), ICONBTT_NO_FIT,
					APP_CONF->imageviewer_flags & CONFIG_IMAGEVIEWER_F_FIT);
				break;
			//左右反転
			case CMDID_MIRROR:
				_cmd_toggle_mirror(p);
				
				mIconButtonsSetCheck(M_ICONBUTTONS(p->iconbtt), ICONBTT_NO_MIRROR,
					APP_CONF->imageviewer_flags & CONFIG_IMAGEVIEWER_F_MIRROR);
				break;
		}
	}
}

/** 表示倍率スライダー、倍率変更時 */

static void _popupslider_handle(int zoom,void *param)
{
	if(DockImageViewer_setZoom((DockImageViewer *)param, zoom))
		_update_area((DockImageViewer *)param);
}

/** 表示倍率スライダー実行 */

static void _popupslider_run(DockImageViewer *p)
{
	mBox box;

	mIconButtonsGetItemBox(M_ICONBUTTONS(p->iconbtt), 4, &box, TRUE);
	
	PopupSliderZoom_run(box.x + box.w + 2, box.y - 50,
		2, 1000, p->zoom, 2, 50,
		_popupslider_handle, p);
}


//==============================
// ハンドラ
//==============================


/** イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	DockImageViewer *p = DOCKIMAGEVIEWER(wg->param);

	//ツールバーからのコマンド実行しか来ない

	switch(ev->type)
	{
		//ツールバー
		case MEVENT_COMMAND:
			switch(ev->cmd.id)
			{
				//メニュー
				case ICONBTT_NO_MENU:
					_run_menu(p);
					break;
				//開く
				case ICONBTT_NO_OPEN:
					_cmd_open(p);
					break;
				//前のファイル
				case ICONBTT_NO_PREV:
					_move_file(p, FALSE);
					break;
				//次のファイル
				case ICONBTT_NO_NEXT:
					_move_file(p, TRUE);
					break;
				//倍率スライダー
				case ICONBTT_NO_ZOOM:
					if(p->img && !_IS_ZOOM_FIT)
						_popupslider_run(p);
					break;
				//全体表示
				case ICONBTT_NO_FIT:
					_cmd_toggle_fitview(p);
					break;
				//左右反転
				case ICONBTT_NO_MIRROR:
					_cmd_toggle_mirror(p);
					break;
			}
			break;
	}

	return 1;
}


//==============================
// 作成
//==============================


/** mDockWidget ハンドラ : 内容作成
 * 
 * コンテナの mWidget::param に DockImageViewer のポインタが入る */

static mWidget *_dock_func_contents(mDockWidget *dockwg,int id,mWidget *parent)
{
	DockImageViewer *p;
	mWidget *ct;
	mIconButtons *ib;
	int i;

	p = DOCKIMAGEVIEWER(APP_WIDGETS->dockobj[DOCKWIDGET_IMAGE_VIEWER]);

	//コンテナ

	ct = mContainerCreate(parent, MCONTAINER_TYPE_HORZ, 0, 0, MLF_EXPAND_WH);

	ct->event = _event_handle;
	ct->notifyTarget = MWIDGET_NOTIFYTARGET_SELF;
	ct->param = (intptr_t)p;
	ct->draw = NULL;

	//アイコンボタン

	ib = mIconButtonsNew(0, ct,
		MICONBUTTONS_S_TOOLTIP | MICONBUTTONS_S_VERT | MICONBUTTONS_S_DESTROY_IMAGELIST);

	ib->wg.fLayout = MLF_EXPAND_H;

	p->iconbtt = M_WIDGET(ib);

	mIconButtonsSetImageList(ib, AppResource_loadIconImage("imgviewer.png", APP_CONF->iconsize_other));

	mIconButtonsSetTooltipTrGroup(ib, TRGROUP_DOCK_IMAGEVIEWER);

	for(i = 0; i < 7; i++)
	{
		mIconButtonsAdd(ib, i, i, TRID_TOOLTIP_TOP + i,
			(i >= ICONBTT_NO_FIT)? MICONBUTTONS_ITEMF_CHECKBUTTON: 0);
	}

	mIconButtonsCalcHintSize(ib);

	_enable_iconbutton(p);

	mIconButtonsSetCheck(ib, CMDID_FIT, APP_CONF->imageviewer_flags & CONFIG_IMAGEVIEWER_F_FIT);
	mIconButtonsSetCheck(ib, CMDID_MIRROR, APP_CONF->imageviewer_flags & CONFIG_IMAGEVIEWER_F_MIRROR);

	//表示エリア

	p->area = DockImageViewerArea_new(p, ct);

	return ct;
}

/** 破棄ハンドラ */

static void _destroy_handle(DockObject *obj)
{
	DockImageViewer *p = (DockImageViewer *)obj;

	ImageBuf24_free(p->img);

	mStrFree(&p->strFileName);
}


//=======================


/** 作成 */

void DockImageViewer_new(mDockWidgetState *state)
{
	DockImageViewer *p;

	//DockImageViewer 作成

	p = (DockImageViewer *)DockObject_new(DOCKWIDGET_IMAGE_VIEWER,
			sizeof(DockImageViewer), 0, MWINDOW_S_ENABLE_DND,
			state, _dock_func_contents);

	p->obj.destroy = _destroy_handle;

	//初期化

	p->zoom = 100;
	p->dscale = p->dscalediv = 1.0;

	//

	mDockWidgetCreateWidget(p->obj.dockwg);
}

/** アイコンサイズ変更 */

void DockImageViewer_changeIconSize()
{
	DockImageViewer *p = DOCKIMAGEVIEWER(APP_WIDGETS->dockobj[DOCKWIDGET_IMAGE_VIEWER]);

	if(DockObject_canDoWidget((DockObject *)p))
	{
		mIconButtonsReplaceImageList(M_ICONBUTTONS(p->iconbtt),
			AppResource_loadIconImage("imgviewer.png", APP_CONF->iconsize_other));

		DockObject_relayout_inWindowMode(DOCKWIDGET_IMAGE_VIEWER);
	}
}
