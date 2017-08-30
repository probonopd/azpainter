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
 * <X11> メイン関数
 *****************************************/
 
#include <stdio.h>
#include <sys/select.h>
#include <unistd.h>
#include <fcntl.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#define MINC_X11_ATOM
#define MINC_X11_UTIL
#define MINC_X11_LOCALE
#define MINC_X11_XSHM
#define MINC_X11_XKB
#include "mSysX11.h"

#include "mAppPrivate.h"
#include "mWindowDef.h"

#include "mList.h"
#include "mEvent.h"
#include "mFontConfig.h"
#include "mFont.h"
#include "mCursor.h"

#include "x11_util.h"
#include "x11_im.h"
#include "x11_clipboard.h"
#include "x11_dnd.h"
#include "x11_xinput2.h"

#include "mNanoTime.h"
#include "mEventList.h"
#include "mAppTimer.h"
#include "mEvent_pv.h"

#include "x11_img_cursor.h"

//-------------------------

void mX11EventTranslate(void);

//-------------------------



//=================================
// sub
//=================================


/** X エラーハンドラ */

static int _xerrhandle(Display *disp,XErrorEvent *ev)
{
	char buf[256],m[32],reqbuf[256];

	//エラーを無効にする

	if(ev->request_code == 42) return 0;  /* WM_TAKE_FOCUS */

	if(ev->error_code == BadAtom && ev->request_code == 17) return 0; /* XGetAtomName() の BadAtom */

	//エラー表示

	XGetErrorText(disp, ev->error_code, buf, 256);

	snprintf(m, 32, "%d", ev->request_code);
	XGetErrorDatabaseText(disp, "XRequest", m, "", reqbuf, 256);

	fprintf(stderr, "#X Error: %d %s\n  req:%d(%s) minor:%d\n",
			ev->error_code, buf, ev->request_code, reqbuf, ev->minor_code);

	return 1;
}

/** 各アトム識別子を取得 */

static void _app_get_atoms(mAppSystem *p)
{
	const char *names[] = {
		"_MLIB_SELECTION",
		"WM_PROTOCOLS",
		"WM_DELETE_WINDOW",
		"WM_TAKE_FOCUS",
		"_NET_WM_PING",
		"_NET_WM_USER_TIME",
		"_NET_FRAME_EXTENTS",
		"_NET_WM_STATE",
		"_NET_WM_STATE_MAXIMIZED_HORZ",
		"_NET_WM_STATE_MAXIMIZED_VERT",
		"_NET_WM_STATE_HIDDEN",
		"_NET_WM_STATE_MODAL",
		"_NET_WM_STATE_ABOVE",
		"UTF8_STRING",
		"COMPOUND_TEXT",
		"CLIPBOARD",
		"TARGETS",
		"MULTIPLE",
		"CLIPBOARD_MANAGER",
		"XdndEnter",
		"XdndPosition",
		"XdndLeave",
		"XdndDrop",
		"XdndStatus",
		"XdndActionCopy",
		"text/uri-list"
	};

	XInternAtoms(p->disp, (char **)names, MX11_ATOM_NUM, False, p->atoms);
}

/** 各機能がサポートされているかチェック & 初期化 */

static void _app_check_support(mAppSystem *p)
{
	int i,cnt,major,minor,op,ev,err;
	Atom *pAtom;

	//XKB (キーボード)

	major = XkbMajorVersion;
	minor = XkbMinorVersion;

	if(XkbLibraryVersion(&major, &minor))
		XkbQueryExtension(p->disp, &op, &ev, &err, &major, &minor);

	//XShm (共有メモリ機能)

#ifdef HAVE_XEXT_XSHM

	if(XQueryExtension(p->disp, "MIT-SHM", &i, &i, &i))
	{
		if(XShmQueryVersion(p->disp, &i, &i, &i))
			p->fSupport |= MX11_SUPPORT_XSHM;
	}

#endif

	//ウィンドウマネージャの有効プロパティ

	pAtom = (Atom *)mX11GetProperty32(p->root_window,
				mX11GetAtom("_NET_SUPPORTED"), XA_ATOM, &cnt);
	
	if(pAtom)
	{
		for(i = 0; i < cnt; i++)
		{
			if(pAtom[i] == p->atoms[MX11_ATOM_NET_WM_USER_TIME])
				p->fSupport |= MX11_SUPPORT_USERTIME;
		}
		
		mFree(pAtom);
	}
}

/** 初期化 - スレッド関連 */

static void _app_init_thread(mAppSystem *p)
{
#if defined(MLIB_NO_THREAD)

	p->select_fdnum = p->connection + 1;

#else

	int fd[2];

	if(pipe(fd) == 0)
	{
		p->fd_pipe[0] = fd[0]; //read
		p->fd_pipe[1] = fd[1]; //write

		fcntl(fd[0], F_SETFL, fcntl(fd[0], F_GETFL) | O_NONBLOCK);
		fcntl(fd[1], F_SETFL, fcntl(fd[1], F_GETFL) | O_NONBLOCK);
	}

	p->select_fdnum = 1 + ((p->connection < fd[0])? fd[0]: p->connection);

	p->mutex = mMutexNew();
	mMutexLock(p->mutex);

#endif
}


//=================================
//
//=================================


/** 終了処理 */

void __mAppEnd(void)
{
	mAppSystem *p = MAPP_SYS;

	if(!p->disp) return;

	//スレッド関連

#if !defined(MLIB_NO_THREAD)

	if(p->fd_pipe[0])
	{
		close(p->fd_pipe[0]);
		close(p->fd_pipe[1]);
	}

	if(p->mutex)
	{
		mMutexUnlock(p->mutex);
		mMutexDestroy(p->mutex);
	}

#endif

	//freetype
	
	if(p->ftlib)
		FT_Done_FreeType(p->ftlib);

	//fontconfig
	
	mFontConfigEnd();

	//im
	
	mX11IM_close();

	//XI2

#if defined(MLIB_ENABLE_PENTABLET) && defined(HAVE_XEXT_XINPUT2)
	mListDeleteAll(&p->listXiDev);
#endif

	//GC

	if(p->gc_def)
		XFreeGC(p->disp, p->gc_def);

	//clipboard
	
	mX11ClipboardDestroy(p->clipb);

	//D&D

	mX11DND_free(p->dnd);

	//ディスプレイ閉じる

	XSync(p->disp, False);
	XCloseDisplay(p->disp);
}

/** 初期化
 *
 * @return 0 で成功 */

int __mAppInit(void)
{
	mAppSystem *p;

	//拡張データ確保
	
	MAPP_SYS = (mAppSystem *)mMalloc(sizeof(mAppSystem), TRUE);
	if(!MAPP_SYS) return -1;
	
	p = MAPP_SYS;

	//値初期化

	p->xi2_opcode = -1;

	//Xエラーハンドラ

	XSetErrorHandler(_xerrhandle);
	
	//ディスプレイ開く
	
	p->disp = XOpenDisplay(NULL);
	if(!p->disp)
	{
		fprintf(stderr, "failed XOpenDisplay()\n");
		return -1;
	}
	
	//ロケール＆入力メソッドセット

	if(XSupportsLocale())
	{
#if !defined(MLIB_NO_INPUT_METHOD)

		if(XSetLocaleModifiers(""))
			mX11IM_init();

#endif
	}
	
	//fontconfig
	
	mFontConfigInit();
	
	//FreeType

	if(FT_Init_FreeType(&p->ftlib))
	{
		fprintf(stderr, "failed FT_Init_FreeType()\n");
		return -1;
	}

	//X 情報

	p->root_window = DefaultRootWindow(p->disp);
	p->screen      = DefaultScreen(p->disp);
	p->visual      = DefaultVisual(p->disp, p->screen);
	p->colormap    = DefaultColormap(p->disp, p->screen);
	p->connection  = ConnectionNumber(p->disp);
	
	//カラー情報
	
	MAPP->depth = DefaultDepth(p->disp, p->screen);
	if(MAPP->depth <= 8) return -1;
	
	MAPP->maskR = p->visual->red_mask;
	MAPP->maskG = p->visual->green_mask;
	MAPP->maskB = p->visual->blue_mask;
	
	//
	
	_app_get_atoms(p);
	_app_check_support(p);

	_app_init_thread(p);
	
	//GC
		
	p->gc_def = XCreateGC(p->disp, p->root_window, 0, NULL);
	
	//クリップボード
	
	p->clipb = mX11ClipboardNew();
	if(!p->clipb) return -1;

	//D&D

	p->dnd = mX11DND_alloc();
	if(!p->dnd) return -1;

	//カーソル

	p->cursor_hsplit = mCursorCreateMono(g_cur_hsplit);
	p->cursor_vsplit = mCursorCreateMono(g_cur_vsplit);

	return 0;
}


/********//**

@addtogroup main
@{

**************/


/** ペンタブレット機能の初期化 */

mBool mAppInitPenTablet()
{
#if defined(MLIB_ENABLE_PENTABLET) && defined(HAVE_XEXT_XINPUT2)

	MAPP_SYS->xi2_opcode = mX11XI2_init();

	return (MAPP_SYS->xi2_opcode != -1);

#else
	return FALSE;
#endif
}

/** メインループを抜ける */

void __mAppQuit(void)
{
	//現在のイベントを処理して、イベントリストへ
	
	XSync(XDISP, False);
	
	mX11EventTranslate();
	
	//QUIT 追加
	
	mEventListAppend_widget(NULL, MEVENT_QUIT);
}

/** イベント同期 */

void mAppSync(void)
{
	XSync(XDISP, False);
}

/** スレッド中に内部イベントを起こした時、処理待ちの関数を抜けさせる
 *
 * スレッド関数の中で使用する。 */

void mAppWakeUpEvent()
{
#if !defined(MLIB_NO_THREAD)

	uint8_t dat = 1;

	if(MAPP_SYS->fd_pipe[1])
		write(MAPP_SYS->fd_pipe[1], &dat, 1);

#endif
}

/** mutex ロック
 *
 * スレッド内で GUI 関連の操作を行う場合はロックする。@n
 * イベント処理などを行っている間は内部でロックされているので、
 * その間はスレッド側で GUI 関連の処理をさせないようにする。 */

void mAppMutexLock(void)
{
#if !defined(MLIB_NO_THREAD)

	if(MAPP_SYS->mutex)
		mMutexLock(MAPP_SYS->mutex);

#endif
}

/** mutex ロック解除 */

void mAppMutexUnlock(void)
{
#if !defined(MLIB_NO_THREAD)

	if(MAPP_SYS->mutex)
		mMutexUnlock(MAPP_SYS->mutex);

#endif
}

/** @} */


/** 何かが起こるまで待つ
 * 
 * @retval 1 X イベントが起きた */

static int _wait_event(void)
{
	mAppSystem *p;
	fd_set fd;
	int ret,btime;
	mNanoTime nt;
	struct timeval tv;

	p = MAPP_SYS;

	//fd_set
	
	FD_ZERO(&fd);
	FD_SET(p->connection, &fd);

#if !defined(MLIB_NO_THREAD)
	if(p->fd_pipe[0])
		FD_SET(p->fd_pipe[0], &fd);
#endif
	
	//タイマーの最小時間

	btime = mAppTimerGetMinTime(&p->listTimer, &nt);
	
	if(btime)
	{
		tv.tv_sec  = nt.sec;
		tv.tv_usec = nt.nsec / 1000; //micro sec
	}
	
	//select
	
#if defined(MLIB_NO_THREAD)

	ret = select(p->select_fdnum, &fd, NULL, NULL, (btime)? &tv: NULL);

#else

	mMutexUnlock(p->mutex);
	ret = select(p->select_fdnum, &fd, NULL, NULL, (btime)? &tv: NULL);
	mMutexLock(p->mutex);

#endif

	//結果
	
	if(ret <= 0)
		return 0;
	else if(FD_ISSET(p->connection, &fd))
		return 1;
#if !defined(MLIB_NO_THREAD)
	else if(p->fd_pipe[0] && FD_ISSET(p->fd_pipe[0], &fd))
	{
		//すべてのデータを読み出す
		
		uint32_t dat;
		
		while(read(p->fd_pipe[0], &dat, 4) > 0);
	}
#endif

	return 0;
}

/** イベント処理
 * 
 * @param fwait TRUE で、イベントがない場合は来るまで待つ
 * @return 0 でループ終了。それ以外で続ける */

int __mAppRun(mBool fwait)
{
	mEvent event;
	int fend = 0;

	if(fwait) XSync(XDISP, False);
	
	while(1)
	{
		//タイマー処理
		
		mAppTimerProc(&MAPP_SYS->listTimer);

		//X イベント処理
	
		mX11EventTranslate();
		
		//内部イベント処理
		
		while(mEventListGetEvent(&event))
		{
			if(event.type == MEVENT_QUIT)
			{
				fend = 1;
				break;
			}
			
			//イベントのデバッグ情報表示
			
			if(MAPP->flags & MAPP_FLAGS_DEBUG_EVENT)
				__mEventPutDebug(&event);

			/* ウィジェットのフォーカス変更時、入力コンテキストのフォーカスも変更。
			 * 編集中にマウスクリックなどによってフォーカスが移った場合に、
			 * 編集を終了させる。
			 * (XIC では編集状態をリセットできないので、フォーカス変更によって対応する) */

			if(event.type == MEVENT_FOCUS)
				mX11IC_setFocus(event.widget->toplevel, !event.focus.bOut);

			//イベント関数
		
			(event.widget->event)(event.widget, &event);

			//イベントの確保データ解放
			
			if(event.data) mFree(event.data);
		}

		//イベント後の処理
		
		if(__mAppAfterEvent())
			XSync(XDISP, False);
		
		//

		if(fend) return 0;
		if(!fwait) break;
				
		//イベントが残っている場合は続ける

		XFlush(XDISP);

		if(XPending(XDISP)) continue;

		//何かが起こるまで待つ
		
		_wait_event();
	}
	
	return 1;
}


//==========================

/********//**

@addtogroup main
@{

**************/


/** FT_Library 取得 */

void *mGetFreeTypeLib(void)
{
	return (void *)MAPP_SYS->ftlib;
}

/** デスクトップの領域取得 */

void mGetDesktopBox(mBox *box)
{
	box->x = box->y = 0;
	box->w = XDisplayWidth(XDISP, MAPP_SYS->screen);
	box->h = XDisplayHeight(XDISP, MAPP_SYS->screen);
}

/** デスクトップの作業領域取得 */

void mGetDesktopWorkBox(mBox *box)
{
	long val[4];
	
	if(!mX11GetProperty32Array(MAPP_SYS->root_window,
		mX11GetAtom("_NET_WORKAREA"), XA_CARDINAL, val, 4))
		mGetDesktopBox(box);
	else
	{
		box->x = val[0];
		box->y = val[1];
		box->w = val[2];
		box->h = val[3];
	}
}

/* @} */
