/*$
Copyright (c) 2017, Azel
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERSAND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
$*/

/*****************************************
 * mWindow [トップレベルウィンドウ]
 *****************************************/
 

#include "mDef.h"

#include "mWindowDef.h"
#include "mSysCol.h"

#include "mGui.h"
#include "mStr.h"
#include "mWidget.h"
#include "mContainer.h"
#include "mWindow.h"
#include "mMenuBar.h"

#include "mUtil.h"
#include "mPixbuf.h"
#include "mRectBox.h"
#include "mImageBuf.h"
#include "mLoadImage.h"

#include "mWindow_pv.h"


//==========================


/** ウィンドウリサイズ時の処理 */

void __mWindowOnResize(mWindow *p,int w,int h)
{
	p->wg.w = w;
	p->wg.h = h;
	
	mPixbufResizeStep(p->win.pixbuf, w, h, 32, 32);
}


//==========================

/**
@var mWindowData::focus_widget
現在のフォーカスウィジェット

@var mWindowData::accelerator
関連付けられたアクセラレータ。ウィンドウ上で定義されたキーが押されると、アクセラレータとして実行される。

@var mWindowData::fStyle
ウィンドウスタイル
*/


/**
@defgroup window mWindow
@brief トップレベルウィンドウ

デフォルトで非表示。

<h3>継承</h3>
mWidget \> mContainer \> mWindow

@ingroup group_window
@{

@file mWindowDef.h
@file mWindow.h
@def M_WINDOW(p)
@struct _mWindow
@struct mWindowData
@enum MWINDOW_STYLE

@var MWINDOW_STYLE::MWINDOW_S_NO_IM
入力メソッドを無効にする

@var MWINDOW_STYLE::MWINDOW_S_NO_RESIZE
ウィンドウのリサイズが出来ないようにする

@var MWINDOW_STYLE::MWINDOW_S_DIALOG
ダイアログタイプ。mlib では実質的な処理な何も行わないが、OS 特有のウィンドウフラグを付加する。 \n
X11 であれば、これを付けた方が確実にモーダルダイアログなウィンドウとなる (ウィンドウアクティブでちらつきが起こらないなど)。

@var MWINDOW_STYLE::MWINDOW_S_ENABLE_DND
このウィンドウへのドロップを許可する
*/



/** ウィンドウ作成
 * 
 * 初期状態で非表示。 */

mWidget *mWindowNew(int size,mWindow *owner,uint32_t style)
{
	mWindow *p;
	
	if(size < sizeof(mWindow)) size = sizeof(mWindow);
	
	p = (mWindow *)mContainerNew(size, NULL);
	if(!p) return NULL;
	
	//
		
	p->wg.drawBkgnd = mWidgetHandleFunc_drawBkgnd_fillFace;
	p->wg.fState &= ~MWIDGET_STATE_VISIBLE;
	p->wg.fType |= MWIDGET_TYPE_WINDOW;
	p->wg.notifyTarget = MWIDGET_NOTIFYTARGET_SELF;

	p->win.owner = owner;
	p->win.fStyle = style;
	
	//イメージ作成
	
	p->win.pixbuf = mPixbufCreate(32, 32);
	if(!p->win.pixbuf)
	{
		mWidgetDestroy(M_WIDGET(p));
		return NULL;
	}
	
	//sys 確保&ウィンドウ作成
	
	if(__mWindowNew(p))
	{
		mWidgetDestroy(M_WIDGET(p));
		return NULL;
	}

	//D&D 有効

	if(style & MWINDOW_S_ENABLE_DND)
		mWindowEnableDND(p);
	
	return M_WIDGET(p);
}

/** メニューバーのメニューデータを取得 */

mMenu *mWindowGetMenuInMenuBar(mWindow *p)
{
	if(p->win.menubar)
		return (p->win.menubar)->mb.menu;
	else
		return NULL;
}

/** フォーカスウィジェットを取得 */

mWidget *mWindowGetFocusWidget(mWindow *p)
{
	return p->win.focus_widget;
}

/** PNG ファイルからアイコンセット
 *
 * @param filename 「!/」で始まっている場合はデータディレクトリからの相対位置 */

void mWindowSetIconFromFile(mWindow *p,const char *filename)
{
	mImageBuf *img;
	mLoadImageSource src;
	char *path;

	path = mAppGetFilePath(filename);
	if(!path) return;

	src.type = MLOADIMAGE_SOURCE_TYPE_PATH;
	src.filename = path;

	img = mImageBufLoadImage(&src, (mDefEmptyFunc)mLoadImagePNG, 4, NULL);
	if(img)
	{
		__mWindowSetIcon(p, img);

		mImageBufFree(img);
	}

	mFree(path);
}

/** バッファ (PNG データ) からアイコンセット */

void mWindowSetIconFromBufPNG(mWindow *p,const void *buf,uint32_t bufsize)
{
	mImageBuf *img;
	mLoadImageSource src;

	src.type = MLOADIMAGE_SOURCE_TYPE_BUF;
	src.buf = buf;
	src.bufsize = bufsize;

	img = mImageBufLoadImage(&src, (mDefEmptyFunc)mLoadImagePNG, 4, NULL);
	if(img)
	{
		__mWindowSetIcon(p, img);

		mImageBufFree(img);
	}
}

/** 最小化 */

mBool mWindowMinimize(mWindow *p,int type)
{
	if(mGetChangeState(type, mWindowIsMinimized(p), &type))
	{
		__mWindowMinimize(p, type);
		return TRUE;
	}
	else
		return FALSE;
}

/** 最大化 */

mBool mWindowMaximize(mWindow *p,int type)
{
	if(mGetChangeState(type, mWindowIsMaximized(p), &type))
	{
		__mWindowMaximize(p, type);
		return TRUE;
	}
	else
		return FALSE;
}

/** デスクトップ内に収まるように調整して位置移動 */

void mWindowMoveAdjust(mWindow *p,int x,int y,mBool workarea)
{
	mPoint pt;
	
	pt.x = x, pt.y = y;
	mWindowAdjustPosDesktop(p, &pt, workarea);
	
	mWidgetMove(M_WIDGET(p), pt.x, pt.y);
}

/** 指定ウィンドウの中央位置に移動
 *
 * @param win NULL で画面中央 */

void mWindowMoveCenter(mWindow *p,mWindow *win)
{
	mBox box;
	mSize size;
	int x,y,x2,y2;

	mWindowGetFullSize(p, &size);

	if(win)
		mWindowGetFullBox(win, &box);
	else
		mGetDesktopWorkBox(&box);

	//中央位置へ

	x = box.x + ((box.w - size.w) >> 1);
	y = box.y + ((box.h - size.h) >> 1);

	//デスクトップ領域内に収まるように調整

	if(win)
		mGetDesktopWorkBox(&box);

	x2 = box.x + box.w;
	y2 = box.y + box.h;

	if(x + size.w > x2) x = x2 - size.w;
	if(y + size.h > y2) y = y2 - size.h;
	if(x < box.x) x = box.x;
	if(y < box.y) y = box.y;

	mWidgetMove(M_WIDGET(p), x, y);
}

/** 初期表示処理
 *
 * @param box セットする位置とサイズ
 * @param defbox デフォルトの位置とサイズ (位置が負の値の場合は、画面中央)
 * @param defval 位置・サイズがこの値の場合、デフォルト値を使う
 * @param show 表示するか
 * @param mazimize 最大化するか */

void mWindowShowInit(mWindow *p,mBox *box,mBox *defbox,int defval,
	mBool show,mBool maximize)
{
	//サイズ

	mWidgetResize(M_WIDGET(p),
		(box->w == defval)? defbox->w: box->w,
		(box->h == defval)? defbox->h: box->h);

	//位置

	if(box->x != defval && box->y != defval)
		mWindowMoveAdjust(p, box->x, box->y, FALSE);
	else
	{
		//デフォルト位置
		
		if(defbox->x >= 0 && defbox->y >= 0)
			mWindowMoveAdjust(p, defbox->x, defbox->y, FALSE);
		else
			mWindowMoveCenter(p, NULL);
	}

	//最大化

	if(maximize) mWindowMaximize(p, 1);

	//表示

	if(show) mWidgetShow(M_WIDGET(p), 1);
}

/** 初期表示処理 (位置のみ)
 *
 * @param defx,defy 負の値で画面中央 */

void mWindowShowInitPos(mWindow *p,mPoint *pt,int defx,int defy,int defval,mBool show,mBool maximize)
{
	if(pt->x != defval && pt->y != defval)
		mWindowMoveAdjust(p, pt->x, pt->y, FALSE);
	else
	{
		//デフォルト位置

		if(defx >= 0 && defy >= 0)
			mWindowMoveAdjust(p, defx, defy, FALSE);
		else
			mWindowMoveCenter(p, NULL);
	}

	//最大化

	if(maximize) mWindowMaximize(p, 1);

	//表示

	if(show) mWidgetShow(M_WIDGET(p), 1);
}

/** 位置移動＆サイズ変更して表示
 *
 * 位置は、オーナーウィンドウの中央。
 *
 * @param w,h 負の値で推奨サイズ */

void mWindowMoveResizeShow(mWindow *p,int w,int h)
{
	if(w < 0 || h < 0)
	{
		mGuiCalcHintSize();
	
		if(w < 0) w = p->wg.initW;
		if(h < 0) h = p->wg.initH;
	}

	mWidgetResize(M_WIDGET(p), w, h);
	mWindowMoveCenter(p, p->win.owner);
	mWidgetShow(M_WIDGET(p), 1);
}

/** 位置移動＆サイズ変更して表示 (推奨サイズ) */

void mWindowMoveResizeShow_hintSize(mWindow *p)
{
	mGuiCalcHintSize();

	mWidgetResize(M_WIDGET(p), p->wg.initW, p->wg.initH);
	mWindowMoveCenter(p, p->win.owner);
	mWidgetShow(M_WIDGET(p), 1);
}

/** ウィンドウ位置を、デスクトップ内に収まるように調整
 * 
 * @param workarea TRUE で作業領域内に収める */

void mWindowAdjustPosDesktop(mWindow *p,mPoint *pt,mBool workarea)
{
	mBox box;
	mSize size;
	int x,y,x2,y2;
	
	x = pt->x, y = pt->y;
	
	mWindowGetFullSize(p, &size);

	if(workarea)
		mGetDesktopWorkBox(&box);
	else
		mGetDesktopBox(&box);

	x2 = box.x + box.w;
	y2 = box.y + box.h;
		
	if(x + size.w > x2) x = x2 - size.w;
	if(y + size.h > y2) y = y2 - size.h;
	if(x < box.x) x = box.x;
	if(y < box.y) y = box.y;
	
	pt->x = x, pt->y = y;
}

/** 枠も含めたウィンドウ全体のサイズ取得 */

void mWindowGetFullSize(mWindow *p,mSize *size)
{
	mRect rc;

	mWindowGetFrameWidth(p, &rc);
	
	size->w = rc.x1 + rc.x2 + p->wg.w;
	size->h = rc.y1 + rc.y2 + p->wg.h;
}

/** 枠も含めたウィンドウ全体の位置とサイズ取得 */

void mWindowGetFullBox(mWindow *p,mBox *box)
{
	mPoint pt;
	mSize size;

	mWindowGetFrameRootPos(p, &pt);
	mWindowGetFullSize(p, &size);

	box->x = pt.x, box->y = pt.y;
	box->w = size.w, box->h = size.h;
}

/** 更新範囲追加 */

void mWindowUpdateRect(mWindow *p,mRect *rc)
{
	if(p->wg.fUI & MWIDGET_UI_UPDATE)
		mRectUnion(&p->win.rcUpdate, rc);
	else
	{
		p->wg.fUI |= MWIDGET_UI_UPDATE;
		p->win.rcUpdate = *rc;
	}
}

/** @} */
