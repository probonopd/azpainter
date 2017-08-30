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
 * <X11> ドラッグ＆ドロップ処理
 *****************************************/

#include <string.h>

#define MINC_X11_ATOM
#include "mSysX11.h"

#include "mAppPrivate.h"
#include "mWindowDef.h"
#include "mWidget.h"
#include "mStr.h"
#include "mUtilStr.h"

#include "x11_dnd.h"

#include "x11_util.h"


//------------------

struct _mX11DND
{
	Window srcwin,
		dstwin;
	int bUriList;

	mWindow *targetwin;
	mWidget *targetwg;
};

//------------------


//======================
// sub
//======================


/*
//XdndEnter 時、データタイプのアトム値セット

static mBool _enter_set_atomtypes(mX11DND *p,XClientMessageEvent *ev)
{
	int i;
	void *buf;

	if(ev->data.l[1] & 1)
	{
		//4つ以上の場合 -> プロパティから読み込み

		buf = mX11GetProperty32(p->srcwin, mX11GetAtom("XdndTypeList"), XA_ATOM, &i);
		if(!buf) return FALSE;

		p->atomtypes = (Atom *)mMalloc(sizeof(Atom) * i, FALSE);
		if(!p->atomtypes)
		{
			mFree(buf);
			return FALSE;
		}

		memcpy(p->atomtypes, buf, sizeof(Atom) * i);

		mFree(buf);
	}
	else
	{
		//3つ以下 -> イベントデータから取得

		p->atomtypes = (Atom *)mMalloc(sizeof(Atom) * 3, FALSE);
		if(!p->atomtypes) return FALSE;

		for(i = 2; i <= 4; i++)
		{
			if(ev->data.l[i])
				p->atomtypes[p->typenum++] = (Atom)ev->data.l[i];
		}
	}

	return TRUE;
}
*/

/** 状態をクリア */

static void _clear_state(mX11DND *p)
{
	memset(p, 0, sizeof(mX11DND));
}

/** XdndEnter 時、MIME タイプチェック */

static void _enter_check_type(mX11DND *p,XClientMessageEvent *ev)
{
	int i,num;
	Atom *buf,urilist;

	urilist = MX11ATOM(text_uri_list);

	//"text/uri-list" が存在するか

	if(ev->data.l[1] & 1)
	{
		//4つ以上の場合 -> プロパティから読み込み

		buf = (Atom *)mX11GetProperty32(p->srcwin, mX11GetAtom("XdndTypeList"), XA_ATOM, &num);

		if(buf)
		{
			for(i = 0; i < num; i++)
			{
				if(buf[i] == urilist)
				{
					p->bUriList = TRUE;
					break;
				}
			}
			
			mFree(buf);
		}
	}
	else
	{
		//3つ以下 -> イベントデータから

		for(i = 2; i <= 4; i++)
		{
			if(ev->data.l[i] == urilist)
			{
				p->bUriList = TRUE;
				break;
			}
		}
	}
}

/** XdndPosition 時のイベント送信 */

static void _position_send(Window srcwin,Window dstwin,int flags)
{
	XEvent ev;

	mX11SetEventClientMessage(&ev, srcwin, MX11ATOM(XdndStatus));

	ev.xclient.data.l[0] = dstwin;
	ev.xclient.data.l[1] = flags;
	ev.xclient.data.l[4] = MX11ATOM(XdndActionCopy);

	XSendEvent(XDISP, srcwin, True, NoEventMask, &ev);
}

/** ファイル名リスト取得 */

static char **_drop_getfiles(mX11DND *p)
{
	mStr str = MSTR_INIT;
	void *uribuf;
	char **buf,**pp;
	int num;
	const char *top,*end;

	//str にタブで区切った複数ファイル名をセット

	uribuf = mX11GetSelectionDat(p->dstwin,
			mX11GetAtom("XdndSelection"), MX11ATOM(text_uri_list), TRUE, NULL);
	if(!uribuf) return NULL;

	num = mStrSetURIList(&str, (const char *)uribuf, TRUE);

	mFree(uribuf);

	//char * の配列を作成

	buf = (char **)mMalloc(sizeof(char *) * (num + 1), TRUE);
	if(buf)
	{
		top = str.buf;
	
		for(pp = buf; ; pp++)
		{
			if(!mGetStrNextSplit(&top, &end, '\t')) break;

			*pp = mStrndup(top, end - top);
			if(!*pp) break;

			top = end;
		}
	}

	//

	mStrFree(&str);

	return buf;
}

/** ドロップを終了させる */

static void _drop_finish(mX11DND *p)
{
	XEvent ev;

	//送信

	if(p->dstwin)
	{
		mX11SetEventClientMessage(&ev, p->srcwin, mX11GetAtom("XdndFinished"));

		ev.xclient.data.l[0] = p->dstwin;
		ev.xclient.data.l[1] = 1;
		ev.xclient.data.l[2] = MX11ATOM(XdndActionCopy);

		XSendEvent(XDISP, p->srcwin, True, NoEventMask, &ev);
	}

	//クリア

	_clear_state(p);
}


//======================
// イベント処理
//======================


/** XdndEnter : D&Dのカーソルがトップレベルウィンドウ内に入った */

static void _on_enter(mX11DND *p,XClientMessageEvent *ev)
{
	_clear_state(p);

	//バージョン

	if(((ev->data.l[1] >> 24) & 255) > 5) return;

	//ソースウィンドウ

	p->srcwin = ev->data.l[0];

	//MIME タイプチェック

	_enter_check_type(p, ev);
}

/** XdndPosition : カーソル位置を元に、対象のターゲットに対してドロップが有効かを返す  */

static void _on_position(mX11DND *p,XClientMessageEvent *ev)
{
	mWindow *win;
	mWidget *wg;
	mPoint pt;
	int flags = 0;

	if(p->srcwin != ev->data.l[0]) return;

	p->dstwin = ev->window;

	//ドロップ対象のトップレベルウィンドウ

	for(win = M_WINDOW(MAPP->widgetRoot->first); win; win = M_WINDOW(win->wg.next))
	{
		if(WINDOW_XID(win) == ev->window)
			break;
	}

	//----- ドロップ先取得＆ドロップ可能か

	/* flags: [0bit] ドロップを受け付ける [1bit] 矩形内移動中も XdndPosition の送信が必要 */

	p->targetwin = NULL;
	p->targetwg  = NULL;

	if(!win
		|| !p->bUriList
		|| (MAPP_PV->runCurrent->modal && MAPP_PV->runCurrent->modal != win))
	{
		/* uri-list でない場合、
		 * またはモーダル中でドロップ先がモーダルウィンドウ以外の場合は無効 */

		flags = 0;
	}
	else
	{
		p->targetwin = win;
	
		if(win->wg.fState & MWIDGET_STATE_ENABLE_DROP)
			//トップレベルがドロップ可の場合
			flags = 1;
		else
		{
			//----- カーソル位置下のウィジェットで判定

			flags = 2;

			//ルート位置 -> win での位置

			pt.x = ((unsigned long)ev->data.l[2] >> 16) & 0xffff;
			pt.y = ev->data.l[2] & 0xffff;

			mWidgetMapPoint(NULL, M_WIDGET(win), &pt);

			//ウィジェット取得

			wg = mWidgetGetUnderWidget(M_WIDGET(win), pt.x, pt.y);

			if(wg != M_WIDGET(win))
			{
				p->targetwg = wg;

				if(wg->fState & MWIDGET_STATE_ENABLE_DROP)
					flags |= 1;
			}
		}
	}

	//---- イベント送信

	_position_send(p->srcwin, ev->window, flags);
}

/** XdndDrop : ドロップされた時  */

static void _on_drop(mX11DND *p,XClientMessageEvent *ev)
{
	char **files = NULL;
	mWindow *win;
	mWidget *wg;

	if(p->srcwin != ev->data.l[0]) return;

	//ファイル名取得

	if(p->targetwin)
		files = _drop_getfiles(p);

	//終了

	win = p->targetwin;
	wg  = p->targetwg;

	_drop_finish(p);

	//ハンドラ実行
	/* 先にトップレベルで実行。FALSE が返ったら子ウィジェットで実行 */

	if(files)
	{
		if(!win->wg.onDND
			|| !(win->wg.onDND)(M_WIDGET(win), files))
		{
			if(wg && wg->onDND)
				(wg->onDND)(wg, files);
		}

		mFreeStrsBuf(files);
	}
}


//======================
// メイン
//======================


/** 解放 */

void mX11DND_free(mX11DND *p)
{
	mFree(p);
}

/** 確保 */

mX11DND *mX11DND_alloc()
{
	return (mX11DND *)mMalloc(sizeof(mX11DND), TRUE);
}

/** ClientMessage イベント処理 */

mBool mX11DND_onClientMessage(mX11DND *p,XClientMessageEvent *ev)
{
	if(ev->message_type == MX11ATOM(XdndEnter))
		_on_enter(p, ev);
	else if(ev->message_type == MX11ATOM(XdndPosition))
		_on_position(p, ev);
	else if(ev->message_type == MX11ATOM(XdndDrop))
		_on_drop(p, ev);
	else if(ev->message_type == MX11ATOM(XdndLeave))
	{
		//カーソルがトップレベルウィンドウから離れた時

		if(p->srcwin == ev->data.l[0])
			_clear_state(p);
	}
	else
		return FALSE;

	return TRUE;
}
