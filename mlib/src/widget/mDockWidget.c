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
 * mDockWidget
 *****************************************/
/*
 * ==== ウィンドウモード時 ====
 * 
 * mWindow : mWidgetData::param に mDockWidget *
 *   |- 内容 (contents)
 *
 * [!] ct_dock は NULL
 * 
 * ==== ドッキング時 ====
 *
 * 格納先 (dockparent)
 *   |- コンテナ (ct_dock) : mWidgetData::param に mDockWidget *
 *       |- ヘッダ (header)
 *       |- 内容 (contents)
 *   |- スプリッター (常に作成。状況に応じて非表示)
 *
 */

#include "mDef.h"

#include "mDockWidget.h"

#include "mGui.h"
#include "mWindowDef.h"
#include "mWidget.h"
#include "mContainer.h"
#include "mWindow.h"
#include "mEvent.h"
#include "mDockHeader.h"
#include "mSplitter.h"
#include "mUtil.h"


//------------------------

typedef struct _mDockWidget
{
	uint32_t style,
		winstyle,
		flags;
	int id,
		notify_id,		//通知のID
		dockH;
	intptr_t param;
	mBox boxWin;

	char *title;
	mWindow *win,
		/* ウィンドウモード時のウィンドウウィジェット。
		 * ドッキングモード時 or 閉じている時は NULL */
		*owner;
	mWidget *dockparent,
		*contents,		//内容のウィジェット
		*notifywg;		//通知先ウィジェット
	mContainer *ctdock;
	mDockHeader *header;
	mSplitter *splitter;
	mFont *font;

	mDockWidgetCallbackCreate func_create;
	mDockWidgetCallbackArrange func_arrange;
}mDockWidget;

//------------------------

static int _handle_event(mWidget *wg,mEvent *ev);

//------------------------


//==============================
// sub
//==============================


/** 通知イベント追加 */

static void _send_notify(mDockWidget *p,int type)
{
	if(p->notifywg)
	{
		mWidgetAppendEvent_notify_id(p->notifywg,
			NULL, p->notify_id, type, (intptr_t)p, 0);
	}
}

/** 格納先の挿入位置取得 */

static mWidget *_get_insert_pos(mDockWidget *p)
{
	mWidget *wg;
	mDockWidget *wgdock;

	for(wg = p->dockparent->first; wg; wg = wg->next)
	{
		wgdock = (mDockWidget *)wg->param;

		if(wgdock && (p->func_arrange)(p, wgdock->id, p->id) > 0)
			return wg;
	}

	return NULL;
}

/** 親の格納ウィジェット内に常に拡張の dock があり、かつ内容が表示されているか */

static mBool _is_dock_exist_expand(mDockWidget *p)
{
	mWidget *wg;
	mDockWidget *dockwg;

	for(wg = p->dockparent->first; wg; wg = wg->next)
	{
		dockwg = (mDockWidget *)wg->param;

		if(dockwg
			&& (dockwg->style & MDOCKWIDGET_S_EXPAND)
			&& (dockwg->flags & MDOCKWIDGET_F_VISIBLE)
			&& !(dockwg->flags & MDOCKWIDGET_F_FOLDED))
			return TRUE;
	}

	return FALSE;
}

/** 現在の配置状況から、各ウィジェットの状態を変更 */

static void _set_dock_state(mDockWidget *p)
{
	mWidget *wgdock,*wg;
	mDockWidget *pdock;
	mBool blast,bshow,bfold;

	//内容が表示されている最後の dock か
	//(p 以降に内容が表示されている dock が存在するなら FALSE)

	blast = TRUE;

	for(wg = (mWidget *)p->splitter; wg; wg = wg->next)
	{
		pdock = (mDockWidget *)wg->param;

		if(pdock
			&& (pdock->flags & MDOCKWIDGET_F_VISIBLE)
			&& !(pdock->flags & MDOCKWIDGET_F_FOLDED))
		{
			blast = FALSE;
			break;
		}
	}

	//

	wgdock = (mWidget *)p->ctdock;
	bshow = ((p->flags & MDOCKWIDGET_F_VISIBLE) != 0);
	bfold = ((p->flags & MDOCKWIDGET_F_FOLDED) != 0);

	//高さ

	wgdock->h = p->dockH;

	//折りたたみ時は内容のウィジェットを非表示

	mWidgetShow(p->contents, !bfold);

	//全体の表示

	mWidgetShow(wgdock, bshow);

	//レイアウト変更

	if(bshow)
	{
		wgdock->fLayout &= ~(MLF_FIX_H | MLF_EXPAND_H);

		//折りたたみ時は推奨サイズ (ヘッダのみ表示)

		if(!bfold)
		{
			if((p->style & MDOCKWIDGET_S_EXPAND)
				|| (blast && !_is_dock_exist_expand(p)))
				/* 常に拡張、
				 * または位置が終端で、ドック内に拡張のものがない場合は拡張。 */
				wgdock->fLayout |= MLF_EXPAND_H;
			else
				//固定
				wgdock->fLayout |= MLF_FIX_H;
		}
	}

	//スプリッター

	mWidgetShow(M_WIDGET(p->splitter), (bshow && !blast && !bfold));
}

/** すべてのドッキングの状態変更 */

static void _set_dock_state_all(mWidget *parent)
{
	mWidget *wg;

	for(wg = parent->first; wg; wg = wg->next)
	{
		if(wg->param)
			_set_dock_state((mDockWidget *)wg->param);
	}
}

/** ドッキングの高さを記録 */

static void _save_dock_height(mDockWidget *p)
{
	if(p->ctdock
		&& !(p->flags & MDOCKWIDGET_F_FOLDED)
		&& (p->ctdock->wg.fLayout & MLF_FIX_H))
		p->dockH = p->ctdock->wg.h;
}

/** すべてのドッキングの高さを記録 */

static void _save_dock_height_all(mWidget *parent)
{
	mWidget *wg;

	for(wg = parent->first; wg; wg = wg->next)
	{
		if(wg->param)
			_save_dock_height((mDockWidget *)wg->param);
	}
}

/** ドックを再レイアウト */

static void _relayout_dock(mDockWidget *p,mBool relayout)
{
	_set_dock_state_all(p->dockparent);

	if(relayout) mWidgetReLayout(p->dockparent);
}


//==============================
// スプリッター
//==============================


/** サイズ変更の対象ウィジェットを取得
 *
 * 閉じている内容ウィジェットは除く。 */

static int _splitter_get_target(mWidget *current,mWidget **ret_prev,mWidget **ret_next)
{
	mWidget *wg,*wgprev,*wgnext;
	mDockWidget *dockwg;

	wgprev = wgnext = NULL;

	//上方向の対象を検索
	/* 表示されている&閉じていない最初のウィジェット */

	for(wg = current->prev; wg; wg = wg->prev)
	{
		dockwg = (mDockWidget *)wg->param;

		if(dockwg
			&& (dockwg->flags & MDOCKWIDGET_F_VISIBLE)
			&& !(dockwg->flags & MDOCKWIDGET_F_FOLDED))
		{
			wgprev = wg;
			break;
		}
	}

	if(!wgprev) return 0;

	//下方向の対象を検索
	/* EXPAND のウィジェットがある場合は、それを対象に (間のウィジェットはサイズ固定)。
	 * なければ直近の対象ウィジェット。 */

	for(wg = current->next, wgnext = NULL; wg; wg = wg->next)
	{
		dockwg = (mDockWidget *)wg->param;

		if(dockwg)
		{
			//表示されている&閉じていない最初のウィジェット
			
			if(!wgnext
				&& (dockwg->flags & MDOCKWIDGET_F_VISIBLE)
				&& !(dockwg->flags & MDOCKWIDGET_F_FOLDED))
				wgnext = wg;

			//EXPAND の場合

			if(wg->fLayout & MLF_EXPAND_H)
			{
				wgnext = wg;
				break;
			}
		}
	}

	if(!wgnext) return 0;

	//

	*ret_prev = wgprev;
	*ret_next = wgnext;

	return 1;
}

/** スプリッターのコールバック */

static int _splitter_callback(mSplitter *p,mSplitterTargetInfo *info)
{
	mWidget *wgprev,*wgnext;

	if(!_splitter_get_target(M_WIDGET(p), &wgprev, &wgnext))
		return 0;

	info->wgprev = wgprev;
	info->wgnext = wgnext;

	info->prev_cur = wgprev->h;
	info->next_cur = wgnext->h;

	info->prev_min = info->next_min = ((mWidget *)info->param)->h;
	info->prev_max = info->next_max = info->prev_cur + info->next_cur;

	return 1;
}


//==============================
// 作成など
//==============================


/** ヘッダを作成 */

static void _create_header(mDockWidget *p,mWidget *parent)
{
	uint32_t s = 0;

	if(p->style & MDOCKWIDGET_S_HAVE_CLOSE) s |= MDOCKHEADER_S_HAVE_CLOSE;
	if(p->style & MDOCKWIDGET_S_HAVE_SWITCH) s |= MDOCKHEADER_S_HAVE_SWITCH;
	if(p->style & MDOCKWIDGET_S_HAVE_FOLD) s |= MDOCKHEADER_S_HAVE_FOLD;

	if(p->win)
	{
		s |= MDOCKHEADER_S_SWITCH_DOWN;
		s &= ~(MDOCKHEADER_S_HAVE_FOLD | MDOCKHEADER_S_HAVE_CLOSE);
	}
	
	if(p->flags & MDOCKWIDGET_F_FOLDED) s |= MDOCKHEADER_S_FOLDED;

	p->header = mDockHeaderNew(0, parent, s);

	mDockHeaderSetTitle(p->header, p->title);
}

/** ウィンドウとして作成 */

static mBool _create_window(mDockWidget *p)
{
	mWindow *win;

	win = (mWindow *)mWindowNew(0, p->owner, p->winstyle);
	if(!win) return FALSE;

	p->win = win;

	//

	win->wg.param = (intptr_t)p;
	win->wg.event = _handle_event;
	win->wg.font = p->font;

	mWindowSetTitle(win, p->title);

	//ウィジェット

	_create_header(p, M_WIDGET(win));

	p->contents = (p->func_create)(p, p->id, M_WIDGET(win));

	//サイズ・位置

	mWidgetResize(M_WIDGET(win), p->boxWin.w, p->boxWin.h);
	mWindowMoveAdjust(p->win, p->boxWin.x, p->boxWin.y, FALSE);

	return TRUE;
}

/** ドッキングウィジェット作成 */

static mBool _create_dockwidget(mDockWidget *p)
{
	mContainer *ct;
	mSplitter *sp;
	mWidget *ins;

	if(!p->dockparent) return FALSE;

	ins = _get_insert_pos(p);

	//メインコンテナ

	ct = mContainerNew(0, p->dockparent);

	p->ctdock = ct;

	ct->wg.param = (intptr_t)p;
	ct->wg.notifyTarget = MWIDGET_NOTIFYTARGET_SELF;
	ct->wg.event = _handle_event;
	ct->wg.font = p->font;

	ct->wg.fLayout = MLF_EXPAND_W;

	//ヘッダ

	_create_header(p, M_WIDGET(ct));

	//内容

	p->contents = (p->func_create)(p, p->id, M_WIDGET(ct));

	if(p->style & MDOCKWIDGET_S_NO_FOCUS)
		mWidgetNoTakeFocus_under(p->contents);

	if(p->flags & MDOCKWIDGET_F_FOLDED)
		p->contents->fState &= ~MWIDGET_STATE_VISIBLE;

	//----- スプリッター
	/* wg.param は NULL にしておく。
	 * 親コンテナからの検索時に内容ウィジェットと区別できるようにする。 */

	sp = mSplitterNew(0, p->dockparent, MSPLITTER_S_VERT | MSPLITTER_S_NOTIFY_MOVED);

	p->splitter = sp;

	sp->wg.fLayout = MLF_EXPAND_W;
	sp->wg.notifyTarget = MWIDGET_NOTIFYTARGET_WIDGET;
	sp->wg.notifyWidget = (mWidget *)p->ctdock;

	mSplitterSetCallback_getTarget(sp, _splitter_callback, (intptr_t)p->header);

	//----- 位置移動

	if(ins)
	{
		mWidgetTreeMove(M_WIDGET(ct), p->dockparent, ins);
		mWidgetTreeMove(M_WIDGET(sp), p->dockparent, ins);
	}

	//----- 状態セット

	_set_dock_state_all(p->dockparent);

	return TRUE;
}

/** ウィジェット作成 */

static void _create_widget(mDockWidget *p)
{
	if(p->flags & MDOCKWIDGET_F_WINDOW)
	{
		if(!(p->win) && _create_window(p))
			p->flags |= MDOCKWIDGET_F_EXIST;
	}
	else
	{
		if(!(p->ctdock) && _create_dockwidget(p))
		{
			p->flags |= MDOCKWIDGET_F_EXIST;

			//再レイアウト
			
			if(p->dockparent->fUI & MWIDGET_UI_LAYOUTED)
				mWidgetReLayout(p->dockparent);
		}
	}
}

/** ウィジェットを閉じる (破棄)
 *
 * @param user_action  -1 で、モード切り替え時など内部で実行される処理 */

static void _close_widget(mDockWidget *p,int user_action)
{
	mBool closed = FALSE;

	//閉じる

	if(p->flags & MDOCKWIDGET_F_WINDOW)
	{
		if(p->win)
		{
			mWindowGetSaveBox(p->win, &p->boxWin);
		
			mWidgetDestroy(M_WIDGET(p->win));
			p->win = NULL;

			closed = TRUE;
		}
	}
	else
	{
		if(p->ctdock)
		{
			//高さ記録

			_save_dock_height_all(p->dockparent);

			//破棄
		
			mWidgetDestroy(M_WIDGET(p->ctdock));
			mWidgetDestroy(M_WIDGET(p->splitter));

			p->ctdock = NULL;
			p->splitter = NULL;

			//再レイアウト

			_relayout_dock(p, TRUE);

			closed = TRUE;
		}
	}

	//

	p->flags &= ~MDOCKWIDGET_F_EXIST;
	p->header = NULL;
	p->contents = NULL;

	//イベント

	if(user_action >= 1 && closed)
		_send_notify(p, MDOCKWIDGET_NOTIFY_CLOSE);
}

/** 表示状態変更
 *
 * @param type 0:非表示 1:表示 */

static void _set_visible(mDockWidget *p,int type)
{
	if(p->flags & MDOCKWIDGET_F_WINDOW)
	{
		//ウィンドウ

		if(p->win)
			mWidgetShow(M_WIDGET(p->win), type);
	}
	else
	{
		//ドック

		if(p->ctdock)
		{
			mWidgetShow(M_WIDGET(p->ctdock), type);
			mWidgetShow(M_WIDGET(p->splitter), type);

			_relayout_dock(p, TRUE);
		}
	}
}


//==============================
// ハンドラ
//==============================


/** 折りたたむ */

static void _toggle_folded(mDockWidget *p,mBool user_action)
{
	p->flags ^= MDOCKWIDGET_F_FOLDED;

	mDockHeaderSetFolded(p->header, -1);
	mWidgetShow(p->contents, -1);

	_relayout_dock(p, TRUE);

	//通知

	if(user_action)
		_send_notify(p, MDOCKWIDGET_NOTIFY_TOGGLE_FOLD);
}

/** モード切り替え */

static void _toggle_window_mode(mDockWidget *p,mBool user_action)
{
	_close_widget(p, -1);

	p->flags ^= MDOCKWIDGET_F_WINDOW;

	_create_widget(p);

	//[!]非表示時の対応はしてない

	mDockWidgetShowWindow(p);

	//通知

	if(user_action)
		_send_notify(p, MDOCKWIDGET_NOTIFY_TOGGLE_SWITCH);
}

/** イベントハンドラ */

int _handle_event(mWidget *wg,mEvent *ev)
{
	mDockWidget *p = (mDockWidget *)wg->param;

	switch(ev->type)
	{
		case MEVENT_NOTIFY:
			if(ev->notify.widgetFrom == (mWidget *)p->splitter)
			{
				//スプリッターでのサイズ変更時 (高さ保存)

				if(ev->notify.type == MSPLITTER_N_MOVED)
					_save_dock_height_all(p->dockparent);
			}
			else if(ev->notify.widgetFrom == (mWidget *)p->header
				&& ev->notify.type == MDOCKHEADER_N_BUTTON)
			{
				//ヘッダのボタン

				switch(ev->notify.param1)
				{
					//閉じる
					case MDOCKHEADER_BTT_CLOSE:
						_close_widget(p, TRUE);
						break;
					//折りたたむ
					case MDOCKHEADER_BTT_FOLD:
						_toggle_folded(p, TRUE);
						break;
					//モード切り替え
					case MDOCKHEADER_BTT_SWITCH:
						_toggle_window_mode(p, TRUE);
						break;
				}
			}
			break;

		//ウィンドウの閉じるボタン
		case MEVENT_CLOSE:
			_close_widget(p, TRUE);
			break;
	}

	return 1;
}


//============================


/****************//**

@defgroup dockwidget mDockWidget
@brief ウィジェットへのドッキングとウィンドウへの分離が可能なウィジェット

- 閉じるボタンで消した場合、ウィジェットは破棄され、再表示される時に再度作成される。\n
そのため、再表示時には内部のウィジェットも再度初期化しなければならない。
- 折り畳むと、内容のウィジェットが非表示になる。
- 「閉じる」とは別に、ウィジェットを存在させたまま表示だけを非表示にすることもできる。

@attention
内容ウィジェット内でタブなどを使う場合、
内容の作成ハンドラ内で mWidgetReLayout() を実行しないこと。\n
mDockWidget 側で内容作成後に再レイアウトが実行されるので、二重処理になってしまう。\n
また、初期時に畳んで非表示状態の場合、内容の表示/非表示は作成ハンドラ後にセットされるので、
作成ハンドラ内で再レイアウトすると、内容が表示の状態で計算されるので、
高さが正しい状態にならない。

@ingroup group_widget
@{

@file mDockWidget.h
@struct mDockWidgetState
@enum MDOCKWIDGET_STYLE
@enum MDOCKWIDGET_FLAGS
@enum MDOCKWIDGET_NOTIFY

@var MDOCKWIDGET_STYLE::MDOCKWIDGET_S_HAVE_CLOSE
閉じるボタンを付ける

@var MDOCKWIDGET_STYLE::MDOCKWIDGET_S_HAVE_SWITCH
モード変更のボタンを付ける

@var MDOCKWIDGET_STYLE::MDOCKWIDGET_S_HAVE_FOLD
折りたたむボタンを付ける

@var MDOCKWIDGET_STYLE::MDOCKWIDGET_S_EXPAND
内容の高さを拡張する

@var MDOCKWIDGET_STYLE::MDOCKWIDGET_S_NO_FOCUS
ドッキング状態時は、内容のウィジェットにフォーカスが移らないようにする。\n
ウィンドウモード時は常にフォーカスあり。


@var MDOCKWIDGET_FLAGS::MDOCKWIDGET_F_EXIST
ウィジェットが作成されている

@var MDOCKWIDGET_FLAGS::MDOCKWIDGET_F_VISIBLE
ウィジェットが表示されている

@var MDOCKWIDGET_FLAGS::MDOCKWIDGET_F_WINDOW
ウィンドウモード

@var MDOCKWIDGET_FLAGS::MDOCKWIDGET_F_FOLDED
内容が畳まれている

************************/


/** 破棄 */

void mDockWidgetDestroy(mDockWidget *p)
{
	if(p)
	{
		mFree(p->title);
		mFree(p);
	}
}

/** 作成
 *
 * ウィジェットはまだ作成されない。
 * 
 * @attention 自動で解放されないので、明示的に削除すること。
 */

mDockWidget *mDockWidgetNew(int id,uint32_t style)
{
	mDockWidget *p;

	p = (mDockWidget *)mMalloc(sizeof(mDockWidget), TRUE);
	if(!p) return NULL;

	p->id = id;
	p->style = style;

	p->dockH = 100;
	p->boxWin.w = p->boxWin.h = 200;

	return p;
}

/** 内容作成関数をセット
 *
 * @param func 作成したトップのウィジェットを返す */

void mDockWidgetSetCallback_create(mDockWidget *p,mDockWidgetCallbackCreate func)
{
	p->func_create = func;
}

/** 配置決定関数をセット
 *
 * @param func id1 が id2 より下なら 0 以外を返す。*/

void mDockWidgetSetCallback_arrange(mDockWidget *p,mDockWidgetCallbackArrange func)
{
	p->func_arrange = func;
}

/** ウィンドウ表示時の情報セット
 *
 * @param owner ウィンドウのオーナー
 * @param winstyle ウィンドウスタイル */

void mDockWidgetSetWindowInfo(mDockWidget *p,mWindow *owner,uint32_t winstyle)
{
	p->owner = owner;
	p->winstyle = winstyle;
}

/** ドッキング先のウィジェットセット */

void mDockWidgetSetDockParent(mDockWidget *p,mWidget *parent)
{
	p->dockparent = parent;
}

/** 状態の情報をセット */

void mDockWidgetSetState(mDockWidget *p,mDockWidgetState *info)
{
	p->flags = info->flags;
	p->dockH = info->dockH;
	p->boxWin = info->boxWin;
}

/** タイトルをセット
 *
 * ヘッダ部分に表示される。英小文字とスペースのみ。 */

void mDockWidgetSetTitle(mDockWidget *p,const char *title)
{
	M_FREE_NULL(p->title);

	p->title = mStrdup(title);
}

/** パラメータをセット */

void mDockWidgetSetParam(mDockWidget *p,intptr_t param)
{
	p->param = param;
}

/** フォントセット */

void mDockWidgetSetFont(mDockWidget *p,mFont *font)
{
	p->font = font;
}

/** 通知先のウィジェットセット
 *
 * 通知元ウィジェットは NULL になるので、id で判別させること。 */

void mDockWidgetSetNotifyWidget(mDockWidget *p,mWidget *wg,int id)
{
	p->notifywg = wg;
	p->notify_id = id;
}

//===================


/** 状態を取得 */

void mDockWidgetGetState(mDockWidget *p,mDockWidgetState *state)
{
	if(p->win)
		mWindowGetSaveBox(p->win, &p->boxWin);
	else if(p->ctdock)
		_save_dock_height(p);

	//

	state->boxWin = p->boxWin;
	state->dockH = p->dockH;
	state->flags = p->flags;
}

/** ウィジェットが作成されているか
 *
 * 「×」で閉じている状態では、ウィジェットは存在していない。 */

mBool mDockWidgetIsCreated(mDockWidget *p)
{
	return (p && (p->flags & MDOCKWIDGET_F_EXIST) != 0);
}

/** 内容のウィジェットが実際に表示されている状態か
 *
 * @return 「×」で閉じている or 畳まれている or 非表示時は FALSE */

mBool mDockWidgetIsVisibleContents(mDockWidget *p)
{
	return (p && p->contents && mWidgetIsVisibleReal(p->contents));
}

/** 現在の状態において、フォーカスを受け取ることができるか
 * 
 * ウィンドウ表示状態なら、常に受け取る。\n
 * ドッキング状態なら、MDOCKWIDGET_S_NO_FOCUS があれば受け取らない。 */

mBool mDockWidgetCanTakeFocus(mDockWidget *p)
{
	return (p &&
		((p->flags & MDOCKWIDGET_F_WINDOW) || !(p->style & MDOCKWIDGET_S_NO_FOCUS)));
}

/** 現在、ウィンドウモードになっているか */

mBool mDockWidgetIsWindowMode(mDockWidget *p)
{
	return (p && (p->flags & MDOCKWIDGET_F_WINDOW));
}

/** ウィンドウモード時、ウィンドウウィジェットを取得
 *
 * @return ドッキング状態、または閉じている場合は NULL */

mWindow *mDockWidgetGetWindow(mDockWidget *p)
{
	return (p && (p->flags & MDOCKWIDGET_F_WINDOW) && p->win)? p->win: NULL;
}

/** パラメータを取得 */

intptr_t mDockWidgetGetParam(mDockWidget *p)
{
	return p->param;
}


//=================


/** 現在の設定状態でウィジェットを作成する
 *
 * ウィンドウ分離状態の場合、表示は行われないので、この後に mDockWidgetShowWindow() を実行すること。 */

void mDockWidgetCreateWidget(mDockWidget *p)
{
	if(p->flags & MDOCKWIDGET_F_EXIST)
		_create_widget(p);
}

/** ウィンドウを表示
 *
 * ウィジェット作成後、最初に表示する場合に行う。 */

void mDockWidgetShowWindow(mDockWidget *p)
{
	if(p->win && (p->flags & MDOCKWIDGET_F_VISIBLE))
		mWidgetShow(M_WIDGET(p->win), 1);
}

/** 存在状態を変更
 *
 * OFF にすると、「×」で閉じたのと同じ状態になり、ウィジェットは破棄される。
 * 
 * @param type [0]破棄(閉じる) [正]あり [負]反転 */

void mDockWidgetShow(mDockWidget *p,int type)
{
	if(mGetChangeState(type, p->flags & MDOCKWIDGET_F_EXIST, &type))
	{
		if(type)
		{
			//作成
			_create_widget(p);
			mDockWidgetShowWindow(p);
		}
		else
			//破棄
			_close_widget(p, FALSE);
	}
}

/** 表示状態を変更
 *
 * ウィジェット状態はそのままで、非表示にする。 */

void mDockWidgetSetVisible(mDockWidget *p,int type)
{
	if(mGetChangeState(type, p->flags & MDOCKWIDGET_F_VISIBLE, &type))
	{
		p->flags ^= MDOCKWIDGET_F_VISIBLE;

		//作成済みの場合、表示切り替え

		if(p->flags & MDOCKWIDGET_F_EXIST)
			_set_visible(p, type);
	}
}

/** ウィンドウモード/ドッキング状態の変更
 *
 * 非表示時は無効。
 * 
 * @param type 0:ドッキング 1:ウィンドウ 負:切り替え */

void mDockWidgetSetWindowMode(mDockWidget *p,int type)
{
	if((p->flags & MDOCKWIDGET_F_VISIBLE)
		&& mGetChangeState(type, p->flags & MDOCKWIDGET_F_WINDOW, &type))
	{
		if(p->flags & MDOCKWIDGET_F_EXIST)
			//切り替え
			_toggle_window_mode(p, FALSE);
		else
			//未作成ならフラグだけ変更
			p->flags ^= MDOCKWIDGET_F_WINDOW;
	}
}

/** 内容の再配置を行う
 *
 * 親ウィジェットを変えるなら、この前に mDockWidgetSetDockParent() を実行する。
 *
 * @param relayout 再レイアウトを実行するか。FALSE の場合、手動で親ウィジェットを再レイアウトする。 */

void mDockWidgetRelocate(mDockWidget *p,mBool relayout)
{
	mWidget *ins,*parent;

	if(p->ctdock)
	{
		parent = p->ctdock->wg.parent; //現在の実際の親

		//挿入先
		
		ins = _get_insert_pos(p);

		if(M_WIDGET(p->ctdock) == ins) return;

		//位置移動
		
		mWidgetTreeMove(M_WIDGET(p->ctdock), p->dockparent, ins);
		mWidgetTreeMove(M_WIDGET(p->splitter), p->dockparent, ins);

		//状態セット (親ウィジェトが変わったなら、元の親も)

		_relayout_dock(p, relayout);

		if(parent != p->dockparent)
		{
			_set_dock_state_all(parent);
			if(relayout) mWidgetReLayout(parent);
		}
	}
}

/** @} */
