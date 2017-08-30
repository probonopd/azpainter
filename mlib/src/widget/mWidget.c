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
 * mWidget [ウィジェット]
 *****************************************/
 
#include "mDef.h"

#include "mAppDef.h"
#include "mWindowDef.h"

#include "mGui.h"
#include "mWidget.h"
#include "mWindow.h"
#include "mPixbuf.h"
#include "mFont.h"
#include "mCursor.h"
#include "mEvent.h"
#include "mUtil.h"
#include "mSysCol.h"

#include "mEventList.h"
#include "mWidget_pv.h"
#include "mWindow_pv.h"
#include "mEvent_pv.h"
#include "mPixbuf_pv.h"



//==========================
// sub
//==========================


/** イベントハンドラ */

static int _handle_event(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_CLOSE)
	{
		mAppQuit();
		return 1;
	}
	
	return 0;
}

/** 背景を描画 */

void __mWidgetDrawBkgnd(mWidget *wg,mBox *box,mBool force)
{
	mPixbuf *pixbuf = wg->toplevel->win.pixbuf;
	mBox box1;
	mPoint pt;
	mWidget *p;
	
	//範囲 (絶対位置)
	
	if(box)
		box1 = *box;
	else
	{
		box1.x = box1.y = 0;
		box1.w = wg->w, box1.h = wg->h;
	}
	
	box1.x += wg->absX;
	box1.y += wg->absY;
	
	//描画関数がなければ親の関数を使う

	for(p = wg; p; p = p->parent)
	{
		//背景描画しない
		
		if(p->fOption & MWIDGET_OPTION_NO_DRAW_BKGND) break;

		//親によってすでに描画されている

		if(!force && (p->fUI & MWIDGET_UI_DRAWED_BKGND)) break;

		//描画
	
		if(p->drawBkgnd)
		{
			mPixbufSetOffset(pixbuf, 0, 0, &pt);
		
			(p->drawBkgnd)(p, pixbuf, &box1);

			mPixbufSetOffset(pixbuf, pt.x, pt.y, NULL);

			//フラグON (全体描画時のみ)

			if(!box && wg->first)
				wg->fUI |= MWIDGET_UI_DRAWED_BKGND;

			break;
		}
	}
}

/** 通知先ウィジェット取得 */

static mWidget *_get_notify_widget(mWidget *p,mBool enable_interrupt)
{
	while(1)
	{
		//割り込み

		if(enable_interrupt)
		{
			if(p->notifyTargetInterrupt == MWIDGET_NOTIFYTARGET_INT_SELF)
				return p;
		}

		//通常値

		switch(p->notifyTarget)
		{
			//親の値を使う
			case MWIDGET_NOTIFYTARGET_PARENT_NOTIFY:
				//ルートの子を最上位とする
				if(!p->parent->parent) return p;

				p = p->parent;
				continue;
			//自身
			case MWIDGET_NOTIFYTARGET_SELF:
				return p;
			//指定ウィジェット
			case MWIDGET_NOTIFYTARGET_WIDGET:
				return p->notifyWidget;
			//１つ上の親
			case MWIDGET_NOTIFYTARGET_PARENT:
				return (!p->parent->parent)? p: p->parent;
			//２つ上の親
			case MWIDGET_NOTIFYTARGET_PARENT2:
				if(!p->parent->parent)
					return p;
				else
				{
					p = p->parent;
					return (!p->parent->parent)? p: p->parent;
				}
			//トップレベル
			case MWIDGET_NOTIFYTARGET_TOPLEVEL:
				return M_WIDGET(p->toplevel);
		}
	}

	return NULL;
}

//==========================


/********************//**

@addtogroup widget

- すべてのウィジェットは、mWidget から派生する。
- fState は、デフォルトで \b MWIDGET_STATE_VISIBLE と \b MWIDGET_STATE_ENABLED が ON。
- 幅・高さはデフォルトで１。
- 通知先は、デフォルトで親のウィジェットの指定値と同じ。

@ingroup group_widget
@{

@file mWidgetDef.h
@file mWidget.h
@struct _mWidget
@enum MWIDGET_TYPE_FLAGS
@enum MWIDGET_STATE_FLAGS
@enum MWIDGET_OPTION_FLAGS
@enum MWIDGET_UI_FLAGS
@enum MWIDGET_EVENTFILTER_FLAGS
@enum MWIDGET_ACCEPTKEY_FLAGS
@enum MWIDGET_NOTIFY_TARGET
@enum MWIDGET_NOTIFY_TARGET_INTERRUPT
@enum MWIDGET_LAYOUT_FLAGS

************************/


/** ウィジェット削除
 *
 * @return ツリー上における次のウィジェット */

mWidget *mWidgetDestroy(mWidget *p)
{
	mWidget *pw,*next;
	mWindow *win;

	if(!p) return NULL;

	//グラブ解放

	if(p == MAPP->widgetGrab)
		mWidgetUngrabPointer(p);

	//破棄ハンドラ

	if(p->destroy)
		(p->destroy)(p);

	//フォーカス取り除く
	
	__mWidgetRemoveFocus(p);

	//タイマー削除
	
	mWidgetTimerDeleteAll(p);

	//下位ウィジェット削除
	
	for(pw = p->first; pw; )
		pw = mWidgetDestroy(pw);
	
	//ウィンドウ破棄
	
	if(p->fType & MWIDGET_TYPE_WINDOW)
	{
		win = M_WINDOW(p);
	
		mPixbufFree(win->win.pixbuf);
		
		if(win->win.sys)
		{
			__mWindowDestroy(win);
			
			mFree(win->win.sys);
		}
	}

	//カーソル削除

	if(p->fOption & MWIDGET_OPTION_DESTROY_CURSOR)
		mCursorFree(p->cursorCur);

	//残っているイベント削除
	
	mEventListDeleteWidget(p);

	//app 関連
	
	if(p == MAPP->widgetOver)
		MAPP->widgetOver = NULL;
	
	//リンクをはずす
	
	next = p->next;

	mWidgetTreeRemove(p);
	
	//解放
	
	mFree(p);

	return next;
}

/** ウィジェット新規作成
 * 
 * @param size   構造体の全サイズ (最小で sizeof(mWidget))
 * @param parent NULL でルートウィジェット */

mWidget *mWidgetNew(int size,mWidget *parent)
{
	mWidget *p;
	
	if(size < sizeof(mWidget))
		size = sizeof(mWidget);

	//親 (parent == 1 でルートウィジェット用)

	if(parent == (mWidget *)1)
		parent = NULL;
	else if(!parent)
		parent = MAPP->widgetRoot;

	//作成

	p = (mWidget *)mMalloc(size, TRUE);
	if(!p) return NULL;
		
	p->parent = parent;
	p->fState = MWIDGET_STATE_VISIBLE | MWIDGET_STATE_ENABLED;
	p->notifyTarget = MWIDGET_NOTIFYTARGET_PARENT_NOTIFY;

	p->event  = _handle_event;
	
	p->w = p->h = 1;
	p->hintW = p->hintH = 1;
	
	//親がある場合
	
	if(parent)
	{
		//最上位のウィンドウをセット

		if(parent == MAPP->widgetRoot)
			p->toplevel = M_WINDOW(p);
		else
			p->toplevel = parent->toplevel;

		//サイズ計算フラグ

		__mWidgetSetTreeUpper_ui(p, MWIDGET_UI_FOLLOW_CALC | MWIDGET_UI_CALC);
	}
	
	//リンクを追加
	
	mWidgetTreeAppend(p, parent);
	
	return p;
}

/** p->destroy() を実行 */

void mWidgetHandleDestroy(mWidget *p)
{
	if(p->destroy)
		(p->destroy)(p);
}

/** draw() 用ハンドラ関数。mWidgetDrawBkgnd() で塗りつぶす。 */

void mWidgetHandleFunc_draw_drawBkgnd(mWidget *p,mPixbuf *pixbuf)
{
	mWidgetDrawBkgnd(p, NULL);
}

/** draw() 用ハンドラ関数。枠を描画する。 */

void mWidgetHandleFunc_draw_boxFrame(mWidget *p,mPixbuf *pixbuf)
{
	mPixbufBox(pixbuf, 0, 0, p->w, p->h, 0);
}

/** drawBkgnd() 用ハンドラ関数。FACE 色で全体を塗りつぶす。 */

void mWidgetHandleFunc_drawBkgnd_fillFace(mWidget *p,mPixbuf *pixbuf,mBox *box)
{
	mPixbufFillBox(pixbuf, box->x, box->y, box->w, box->h, MSYSCOL(FACE));
}


//============================
// ウィジェットツリー
//============================


/** p が root の子または root であるか */

mBool mWidgetIsTreeChild(mWidget *p,mWidget *root)
{
	for(; p; p = p->parent)
	{
		if(p == root) return TRUE;
	}

	return FALSE;
}


/** ウィジェットツリーの位置を親の子の一番先頭に移動 */

void mWidgetMoveTree_first(mWidget *p)
{
	mWidgetTreeMove(p, p->parent, p->parent->first);
}


//============================
// 情報取得
//============================


/** 表示されているか (自身のフラグの状態) */

mBool mWidgetIsVisible(mWidget *p)
{
	if(p)
		return ((p->fState & MWIDGET_STATE_VISIBLE) != 0);
	else
		return FALSE;
}

/** 表示されているか (親の状態も含む) */

mBool mWidgetIsVisibleReal(mWidget *wg)
{
	mWidget *p;

	for(p = wg; p->parent; p = p->parent)
	{
		if(!(p->fState & MWIDGET_STATE_VISIBLE))
			return FALSE;
	}

	return TRUE;
}

/** フォント取得
 * 
 * font が NULL の場合は親のフォント。\n
 * 親のフォントが全て NULL ならデフォルトフォント。 */

mFont *mWidgetGetFont(mWidget *p)
{
	for(; p; p = p->parent)
	{
		if(p->font) return p->font;
	}

	return MAPP->fontDefault;
}

/** フォントの高さ取得 */

int mWidgetGetFontHeight(mWidget *p)
{
	mFont *font = mWidgetGetFont(p);
	
	return font->height;
}

/** 指定位置下にある子ウィジェット取得
 *
 * 非表示ウィジェットは対象外。
 *
 * @param root この下位のウィジェットのみ対象。また、x,y はこのウィジェットでの位置。
 * @return 該当するウィジェットがない場合は root が返る */

mWidget *mWidgetGetUnderWidget(mWidget *root,int x,int y)
{
	mWidget *p,*last;
	mRect rc;
	mBool flag;
	
	x += root->absX;
	y += root->absY;

	for(p = last = root; p; )
	{
		//範囲内にあるか
		/* ルートがウィンドウの場合、順に直接判定すれば問題ない。
		 * ウィジェットツリーの途中の場合は、親の状態も適用して正確に判断。 */

		if(root->fType & MWIDGET_TYPE_WINDOW)
		{
			flag = ((p->fState & MWIDGET_STATE_VISIBLE)
				&& p->absX <= x && x < p->absX + p->w
				&& p->absY <= y && y < p->absY + p->h);
		}
		else
		{
			flag = (mWidgetIsVisibleReal(p)
				&& __mWidgetGetClipRect(p, &rc)
				&& rc.x1 <= x && x <= rc.x2
				&& rc.y1 <= y && y <= rc.y2);
		}

		//
	
		if(flag)
		{
			//子ウィジェットも対象
			//(最下層ならそこで終了)
			
			last = p;

			if(!p->first) break;
			
			p = __mWidgetGetTreeNext_root(p, root);
		}
		else
			p = __mWidgetGetTreeNextPass_root(p, root);
	}
	
	return last;
}

/** 通知先ウィジェット取得 */

mWidget *mWidgetGetNotifyWidget(mWidget *p)
{
	return _get_notify_widget(p, TRUE);
}

/** 通知先ウィジェット取得 (nNotifyTarget の値から) */

mWidget *mWidgetGetNotifyWidgetRaw(mWidget *p)
{
	return _get_notify_widget(p, FALSE);
}

/** 相対座標位置がウィジェット範囲内か */

mBool mWidgetIsContain(mWidget *p,int x,int y)
{
	return (x >= 0 && x < p->w && y >= 0 && y < p->h);
}

/** 現在のカーソル位置がウィジェット内にあるか */

mBool mWidgetIsCursorIn(mWidget *p)
{
	mPoint pt;

	mWidgetGetCursorPos(p, &pt);

	return mWidgetIsContain(p, pt.x, pt.y);
}

/** id でウィジェットを検索
 *
 * @param root このウィジェットの下位のみ対象 */

mWidget *mWidgetFindByID(mWidget *root,int id)
{
	mWidget *p;
	
	for(p = root; p && p->id != id; p = __mWidgetGetTreeNext_root(p, root));
	
	return p;
}

/** mBox に x,y,w,h をセット */

void mWidgetGetBox(mWidget *p,mBox *box)
{
	box->x = p->x;
	box->y = p->y;
	box->w = p->w;
	box->h = p->h;
}

/** ルート座標での位置とサイズを取得
 *
 * トップレベルウィンドウの場合、枠は含まない。 */

void mWidgetGetRootBox(mWidget *p,mBox *box)
{
	mPoint pt;

	pt.x = pt.y = 0;
	mWidgetMapPoint(p, NULL, &pt);

	box->x = pt.x, box->y = pt.y;
	box->w = p->w, box->h = p->h;
}

/** レイアウトにおける、親の最大内側サイズ取得
 *
 * 余白やマージンは除くサイズ。 */

void mWidgetGetLayoutMaxSize(mWidget *p,mSize *size)
{
	mWidget *parent = p->parent;
	int w,h;

	//親の範囲

	w = parent->w, h = parent->h;

	if(parent->fType & MWIDGET_TYPE_CONTAINER)
	{
		w -= M_CONTAINER(parent)->ct.padding.left + M_CONTAINER(parent)->ct.padding.right;
		h -= M_CONTAINER(parent)->ct.padding.top + M_CONTAINER(parent)->ct.padding.bottom;
	}

	//マージン

	w -= p->margin.left + p->margin.right;
	h -= p->margin.top + p->margin.bottom;

	if(w < 1) w = 1;
	if(h < 1) h = 1;

	size->w = w;
	size->h = h;
}


//============================
// 変更
//============================


/** 有効/無効状態変更 */

void mWidgetEnable(mWidget *p,int type)
{
	if(mGetChangeState(type, (p->fState & MWIDGET_STATE_ENABLED), &type))
	{
		p->fState ^= MWIDGET_STATE_ENABLED;

		//フォーカスウィジェットが無効状態になったらフォーカス解除

		__mWidgetRemoveFocus_byDisable(p);

		mWidgetUpdate(p);
	}
}

/** 表示状態変更
 *
 * @attention mWindow で MWINDOW_S_POPUP スタイルの場合、アクティブにはならない。
 * 
 * @param type [0]非表示 [正]表示 [負]反転 */

void mWidgetShow(mWidget *p,int type)
{
	if(mGetChangeState(type, (p->fState & MWIDGET_STATE_VISIBLE), &type))
	{
		p->fState ^= MWIDGET_STATE_VISIBLE;

		if(p->fType & MWIDGET_TYPE_WINDOW)
		{
			//ウィンドウ
			/* 非表示状態で更新があった場合は描画が行われていないので、
			 * 表示時は再描画を行う。*/

			if(type)
			{
				mWidgetUpdate(p);
				__mWindowShow(M_WINDOW(p));
			}
			else
				__mWindowHide(M_WINDOW(p));
		}
		else
		{
			/* ウィジェットの表示状態が変わった場合、
			 * 親の推奨サイズを再計算する必要がある。 */
			mWidgetCalcHintSize(p->parent);
			
			mWidgetUpdate(p->parent);
		}

		//フォーカスウィジェットが無効状態になったらフォーカス解除

		__mWidgetRemoveFocus_byDisable(p);
	}
}

/** 位置・サイズ変更 */

mBool mWidgetMoveResize(mWidget *p,int x,int y,int w,int h)
{
	//位置

	mWidgetMove(p, x, y);

	//サイズ
		
	return mWidgetResize(p, w, h);
}

/** 位置変更
 *
 * ウィンドウ時は、枠を含む左上位置。 */

void mWidgetMove(mWidget *p,int x,int y)
{
	mWidget *p2 = NULL;

	p->x = x, p->y = y;
	
	if(p->fType & MWIDGET_TYPE_WINDOW)
		__mWindowMove(M_WINDOW(p), x, y);
	else
	{
		p->absX = (p->parent)->absX + x;
		p->absY = (p->parent)->absY + y;

		//子ウィジェットの絶対位置変更

		while(1)
		{
			p2 = __mWidgetGetTreeNext_root(p2, p);
			if(!p2) break;

			p2->absX = (p2->parent)->absX + p2->x;
			p2->absY = (p2->parent)->absY + p2->y;
		}

		mWidgetUpdate(p->parent);
	}
}

/** リサイズ
 * 
 * @param w,h ウィンドウ時は、枠などを含まないサイズ */

mBool mWidgetResize(mWidget *p,int w,int h)
{
	mBool flag;

	if(w < 1) w = 1;
	if(h < 1) h = 1;

	/* サイズ変更するか
	 * (レイアウトハンドラがある場合は常にレイアウトを行う必要があるので処理) */

	flag = (!(p->fType & MWIDGET_TYPE_WINDOW) && p->layout);
	if(!flag) flag = (w != p->w || h != p->h);

	if(flag)
	{
		p->w = w;
		p->h = h;
	
		if(p->fType & MWIDGET_TYPE_WINDOW)
		{
			__mWindowOnResize(M_WINDOW(p), w, h);
			__mWindowResize(M_WINDOW(p), w, h);
		}
		
		__mWidgetOnResize(p);

		return TRUE;
	}

	return FALSE;
}

/** フォーカスをセット
 *
 * @return フォーカスがない状態から、フォーカスがセットされたか */

mBool mWidgetSetFocus(mWidget *p)
{
	return __mWindowSetFocus(p->toplevel, p, MEVENT_FOCUS_BY_UNKNOWN);
}

/** フォーカスセットと mWidgetUpdate() を行う
 *
 * @param force TRUE で常に再描画。FALSE の場合、フォーカスが移った時のみ再描画を行う */

void mWidgetSetFocus_update(mWidget *p,mBool force)
{
	mBool ret;

	ret = mWidgetSetFocus(p);

	if(force || ret)
		mWidgetUpdate(p);
}

/** 下位全てのウィジェットでフォーカスを受け付けないようにする */

void mWidgetNoTakeFocus_under(mWidget *root)
{
	mWidget *p;

	for(p = root; p; p = __mWidgetGetTreeNext_root(p, root))
		p->fState &= ~MWIDGET_STATE_TAKE_FOCUS;
}


//============================
// レイアウト関連
//============================


/** レイアウトサイズを計算させる
 *
 * 親方向のフラグも ON。\n
 * @attention フラグを ON にするだけなので、実際に計算させる場合は
 *  mGuiCalcHintSize() を実行する。 */

void mWidgetCalcHintSize(mWidget *p)
{
	//上位方向にフラグ ON
	
	__mWidgetSetTreeUpper_ui(p, MWIDGET_UI_FOLLOW_CALC | MWIDGET_UI_CALC);
}

/** レイアウト実行
 *
 * 推奨サイズ計算とレイアウト。 */

void mWidgetLayout(mWidget *p)
{
	mGuiCalcHintSize();

	if(p->layout)
		(p->layout)(p);
}

/** 再レイアウト
 *
 * このウィジェットの推奨サイズを再計算させレイアウト＆再描画。 */

void mWidgetReLayout(mWidget *p)
{
	mWidgetCalcHintSize(p);

	mWidgetLayout(p);
	mWidgetUpdate(p);
}

/** マージンを上下左右同じ値にセット */

void mWidgetSetMargin_one(mWidget *p,int val)
{
	p->margin.left = p->margin.top = p->margin.right = p->margin.bottom = val;
}

/** ウィジェットの外側の余白幅セット
 * 
 * @param val 上位バイトから順に left,top,right,bottom */

void mWidgetSetMargin_b4(mWidget *p,uint32_t val)
{
	p->margin.left   = (uint8_t)(val >> 24);
	p->margin.top    = (uint8_t)(val >> 16);
	p->margin.right  = (uint8_t)(val >> 8);
	p->margin.bottom = (uint8_t)val;
}

/** hintOverW を、文字列の幅からセット */

void mWidgetSetHintOverW_fontTextWidth(mWidget *p,const char *text)
{
	p->hintOverW = mFontGetTextWidth(mWidgetGetFont(p), text, -1);
}

/** 初期レイアウトサイズをフォント高さの n 倍にセット */

void mWidgetSetInitSize_fontHeight(mWidget *p,int wmul,int hmul)
{
	int fonth = mWidgetGetFontHeight(p);

	p->initW = fonth * wmul;
	p->initH = fonth * hmul;
}


//============================
// 描画・更新
//============================


/** 背景描画 (親がすでに背景描画されていた場合、描画しない)
 *
 * @param box NULL でウィジェット全体 */

void mWidgetDrawBkgnd(mWidget *p,mBox *box)
{
	__mWidgetDrawBkgnd(p, box, FALSE);
}

/** 背景描画 (常に描画) */

void mWidgetDrawBkgnd_force(mWidget *p,mBox *box)
{
	__mWidgetDrawBkgnd(p, box, TRUE);
}

/** 全体を描画・更新させる */

void mWidgetUpdate(mWidget *p)
{
	mWidget *p2;

	mWidgetUpdateBox_d(p, 0, 0, p->w, p->h);

	//下位ウィジェットも更新させる

	for(p2 = p; p2; p2 = __mWidgetGetTreeNext_root(p2, p))
		p2->fUI |= MWIDGET_UI_FOLLOW_DRAW | MWIDGET_UI_DRAW;
	
	//上位方向には、FOLLOW フラグ ON
	
	__mWidgetSetTreeUpper_ui(p, MWIDGET_UI_FOLLOW_DRAW);
}

/** 直接描画開始
 *
 * @return NULL の場合、非表示やクリッピングなどによって描画する必要なし */

mPixbuf *mWidgetBeginDirectDraw(mWidget *p)
{
	mPixbuf *pixbuf;
	mRect rcclip;

	//全体描画要求がある、または非表示状態

	if((p->fUI & MWIDGET_UI_DRAW) || !mWidgetIsVisibleReal(p))
		return NULL;

	//クリッピング範囲取得

	if(!__mWidgetGetClipRect(p, &rcclip)) return NULL;

	pixbuf = (p->toplevel)->win.pixbuf;
	
	mPixbufSetOffset(pixbuf, p->absX, p->absY, NULL);
	__mPixbufSetClipMaster(pixbuf, &rcclip);

	return pixbuf;
}

/** 直接描画終了 */

void mWidgetEndDirectDraw(mWidget *p,mPixbuf *pixbuf)
{
	if(pixbuf)
	{
		mPixbufSetOffset(pixbuf, 0, 0, NULL);
		__mPixbufSetClipMaster(pixbuf, NULL);
	}
}

/** 更新範囲追加 */

void mWidgetUpdateBox_d(mWidget *p,int x,int y,int w,int h)
{
	mRect rc;
	
	rc.x1 = p->absX + x;
	rc.y1 = p->absY + y;
	rc.x2 = rc.x1 + w - 1;
	rc.y2 = rc.y1 + h - 1;

	mWindowUpdateRect(p->toplevel, &rc);
}

/** 更新範囲追加 (mBox) */

void mWidgetUpdateBox_box(mWidget *p,mBox *box)
{
	mRect rc;
	
	rc.x1 = p->absX + box->x;
	rc.y1 = p->absY + box->y;
	rc.x2 = rc.x1 + box->w - 1;
	rc.y2 = rc.y1 + box->h - 1;

	mWindowUpdateRect(p->toplevel, &rc);
}


//============================
// ほか
//============================


/** イベント追加 */

mEvent *mWidgetAppendEvent(mWidget *p,int type)
{
	return mEventListAppend_widget(p, type);
}

/** イベントキュー内に同じイベントがある場合は追加しない */

mEvent *mWidgetAppendEvent_only(mWidget *p,int type)
{
	return mEventListAppend_only(p, type);
}

/** NOTIFY イベント追加
 * 
 * @param wg   NULL で送り元の通知先 ( mWidgetGetNotifyWidget(from) の戻り値)。
 *             \b MWIDGET_NOTIFYWIDGET_RAW (1) で mWidgetGetNotifyWidgetRaw(from) の戻り値。
 * @param from 送り元のウィジェット  */

void mWidgetAppendEvent_notify(mWidget *wg,
	mWidget *from,int type,intptr_t param1,intptr_t param2)
{
	mEvent *ev;
	
	if(!wg)
		wg = mWidgetGetNotifyWidget(from);
	else if(wg == MWIDGET_NOTIFYWIDGET_RAW)
		wg = mWidgetGetNotifyWidgetRaw(from);

	//

	ev = mEventListAppend_widget(wg, MEVENT_NOTIFY);
	if(!ev) return;
		
	ev->notify.widgetFrom = from;
	ev->notify.type = type;
	ev->notify.id = from->id;
	ev->notify.param1 = param1;
	ev->notify.param2 = param2;
}

/** NOTIFY イベント追加 (ID 指定) */

void mWidgetAppendEvent_notify_id(mWidget *wg,mWidget *from,
	int id,int type,intptr_t param1,intptr_t param2)
{
	mEvent *ev;
	
	if(!wg)
		wg = mWidgetGetNotifyWidget(from);

	ev = mEventListAppend_widget(wg, MEVENT_NOTIFY);
	if(!ev) return;
		
	ev->notify.widgetFrom = from;
	ev->notify.type = type;
	ev->notify.id = id;
	ev->notify.param1 = param1;
	ev->notify.param2 = param2;
}

/** COMMAND イベント追加 */

void mWidgetAppendEvent_command(mWidget *wg,int id,intptr_t param,int by)
{
	mEvent *ev;
	
	if(!wg) return;
	
	ev = mEventListAppend_widget(wg, MEVENT_COMMAND);
	if(!ev) return;
	
	ev->cmd.id = id;
	ev->cmd.param = param;
	ev->cmd.by = by;
}

/** ポインタをグラブ (カーソル＆デバイス指定)
 *
 * @param deviceid グラブするデバイス指定。0 以下で指定なし。 */

mBool mWidgetGrabPointer_device_cursor(mWidget *p,int deviceid,mCursor cur)
{
	if(MAPP->widgetGrab)
		return FALSE;
	else
	{
		//デバッグなどでグラブを無効にする (--disable-grab)
		
		if(MAPP->flags & MAPP_FLAGS_DISABLE_GRAB)
		{
			MAPP->widgetGrab = p;
			return TRUE;
		}
		
		//
	
		if(!__mWindowGrabPointer(p->toplevel, cur, deviceid))
			return FALSE;
		else
		{
			MAPP->widgetGrab = p;
			return TRUE;
		}
	}
}

/** ポインタをグラブ (グラブ中のカーソル指定) */

mBool mWidgetGrabPointer_cursor(mWidget *p,mCursor cur)
{
	return mWidgetGrabPointer_device_cursor(p, 0, cur);
}

/** ポインタをグラブ
 * 
 * グラブ中もキーイベントが送られてくるので注意。 */

mBool mWidgetGrabPointer(mWidget *p)
{
	return mWidgetGrabPointer_cursor(p, 0);
}

/** ポインタのグラブを解放 */

void mWidgetUngrabPointer(mWidget *p)
{
	if(p == MAPP->widgetGrab)
	{
		/* [!] 実際に解放する前に NULL にする
		 *     (UNGRAB イベントが起きてしまうので) */
		MAPP->widgetGrab = NULL;

		__mWindowUngrabPointer(p->toplevel);

		//LEAVE/ENTER イベント

		__mEventUngrabPointer(p);
	}
}

/** キーボードのグラブ */

mBool mWidgetGrabKeyboard(mWidget *p)
{
	if(MAPP->widgetGrabKey)
		return FALSE;
	else
	{
		//デバッグなどでグラブを無効にする (--disable-grab)
		
		if(MAPP->flags & MAPP_FLAGS_DISABLE_GRAB)
		{
			MAPP->widgetGrabKey = p;
			return TRUE;
		}
		
		//
	
		if(!__mWindowGrabKeyboard(p->toplevel))
			return FALSE;
		else
		{
			MAPP->widgetGrabKey = p;
			return TRUE;
		}
	}
}

/** キーボードのグラブを解放 */

void mWidgetUngrabKeyboard(mWidget *p)
{
	if(p == MAPP->widgetGrabKey)
	{
		MAPP->widgetGrabKey = NULL;

		__mWindowUngrabKeyboard(p->toplevel);
	}
}

/** 指定 ID のタイマーが存在しない場合のみ追加 */

void mWidgetTimerAdd_unexist(mWidget *p,
	uint32_t timerid,uint32_t msec,intptr_t param)
{
	if(!mWidgetTimerIsExist(p, timerid))
		mWidgetTimerAdd(p, timerid, msec, param);
}

/** カーソルセット
 *
 * @param cur 0 で標準カーソルに戻す */

void mWidgetSetCursor(mWidget *wg,mCursor cur)
{
	if(wg->cursorCur != cur)
	{
		wg->cursorCur = cur;

		if(mWidgetIsCursorIn(wg))
			__mWindowSetCursor(wg->toplevel, cur);
	}
}


//============================
// ウィジェットツリー
//============================


/** ツリーに追加 */

void mWidgetTreeAppend(mWidget *p,mWidget *parent)
{
	if(p && parent)
	{
		p->parent = parent;
		p->prev = parent->last;
		p->next = NULL;
		
		parent->last = p;
		
		if(p->prev)
			(p->prev)->next = p;
		else
			parent->first = p;
	}
}

/** ツリーの指定位置に挿入
 *
 * @param ins NULL で最後に追加 */

void mWidgetTreeInsert(mWidget *p,mWidget *parent,mWidget *ins)
{
	if(!ins)
		mWidgetTreeAppend(p, parent);
	else
	{
		p->parent = parent;

		if(ins->prev)
			ins->prev->next = p;
		else
			parent->first = p;

		p->prev = ins->prev;
		p->next = ins;
		ins->prev = p;
	}
}

/** ツリーから取り外す */

void mWidgetTreeRemove(mWidget *p)
{
	if(p)
	{
		if(p->prev)
			(p->prev)->next = p->next;
		else if(p->parent)
			(p->parent)->first = p->next;
		
		if(p->next)
			(p->next)->prev = p->prev;
		else if(p->parent)
			(p->parent)->last = p->prev;
		
		p->parent = p->prev = p->next = NULL;
	}
}

/** 指定ウィジェットの前に移動
 *
 * @param ins NULL で終端 */

void mWidgetTreeMove(mWidget *p,mWidget *parent,mWidget *ins)
{
	if(p != ins)
	{
		mWidgetTreeRemove(p);
		mWidgetTreeInsert(p, parent, ins);
	}
}


/**

@var MWIDGET_OPTION_FLAGS::MWIDGET_OPTION_NO_DRAW_BKGND
背景を描画しない

@var MWIDGET_OPTION_FLAGS::MWIDGET_OPTION_DESTROY_CURSOR
ウィジェットの削除時に現在のカーソルも削除する

@var MWIDGET_OPTION_FLAGS::MWIDGET_OPTION_ONFOCUS_ALLKEY_TO_WINDOW
このウィジェットにフォーカスがある時、すべてのキー入力はトップレベルウィンドウへ送る

@var MWIDGET_OPTION_FLAGS::MWIDGET_OPTION_ONFOCUS_NORMKEY_TO_WINDOW
このウィジェットにフォーカスがある時、通常のキー入力 (TAB/Enter/ESC 以外) はトップレベルウィンドウへ送る

@var MWIDGET_OPTION_FLAGS::MWIDGET_OPTION_DISABLE_SCROLL_EVENT
ポインタデバイスのスクロールボタンが押された場合、スクロールイベントにせずに通常のポインタイベントにする。

*/

/**

@var MWIDGET_NOTIFY_TARGET::MWIDGET_NOTIFYTARGET_PARENT_NOTIFY
親ウィジェットの通知先の値を使う。親もこの値なら、さらにその親へと辿る。

@var MWIDGET_NOTIFY_TARGET::MWIDGET_NOTIFYTARGET_SELF
自身へ送る

@var MWIDGET_NOTIFY_TARGET::MWIDGET_NOTIFYTARGET_PARENT
親ウィジェットへ送る

@var MWIDGET_NOTIFY_TARGET::MWIDGET_NOTIFYTARGET_PARENT2
親の親ウィジェットへ送る

@var MWIDGET_NOTIFY_TARGET::MWIDGET_NOTIFYTARGET_TOPLEVEL
トップレベルウィンドウに送る

@var MWIDGET_NOTIFY_TARGET::MWIDGET_NOTIFYTARGET_WIDGET
notifyWidget で指定されたウィジェットへ送る

*/
	
/**

@var MWIDGET_NOTIFY_TARGET_INTERRUPT::MWIDGET_NOTIFYTARGET_INT_NONE
notifyTarget の値を使う

@var MWIDGET_NOTIFY_TARGET_INTERRUPT::MWIDGET_NOTIFYTARGET_INT_SELF
自身へ通知を送る

*/

/* @} */

/**

@var _mWidget::toplevel
トップレベルのウィジェット

@var _mWidget::notifyWidget
通知先のウィジェット

@var _mWidget::absX
トップレベルウィンドウに対する絶対位置 X

@var _mWidget::hintW
レイアウト時の推奨サイズ幅。\n
calcHintSize() 実行時にセットされるので、直接代入しても通常はそこで上書きされる。

@var _mWidget::initW
レイアウト時の初期表示時の幅 (0 で指定なし)。\n
コンテナの場合は常に計算してセットされる。

@var _mWidget::hintOverW
レイアウト時の推奨サイズ幅を強制的に上書きする場合に指定 (0 で使わない)

@var _mWidget::hintMinW
レイアウト時の推奨サイズの最小幅 (0 で使わない)。\n
hintOverW の方が優先される。

@var _mWidget::fState
ウィジェットの状態フラグ

@var _mWidget::fOption
ウィジェットの動作オプションフラグ

@var _mWidget::fLayout
レイアウトフラグ

@var _mWidget::fUI
レイアウトや描画などで使うフラグ

@var _mWidget::fEventFilter
受け取るイベントのフラグ

@var _mWidget::fType
ウィジェットのタイプのフラグ

@var _mWidget::fAcceptKeys
イベントで受け付けるキーのフラグ

@var _mWidget::notifyTarget
このウィジェットが MEVENT_NOTIFY を送る際に、送り先となるウィジェットの指定

@var _mWidget::notifyTargetInterrupt
\b MWIDGET_NOTIFYTARGET_INT_NONE でない場合、notifyTarget を使わずにこの値を使う

@var _mWidget::margin
ウィジェットのマージン

*/

