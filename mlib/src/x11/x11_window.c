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
 * <X11> ウィンドウ
 *****************************************/


#include <string.h>
#include <unistd.h>

#define MINC_X11_ATOM
#include "mSysX11.h"

#include "mWindowDef.h"
#include "mAppDef.h"
#include "mImageBuf.h"

#include "x11_window.h"
#include "x11_util.h"
#include "x11_im.h"
#include "x11_xinput2.h"


//-----------------------------------------
/* _MOTIF_WM_HINTS のデータフラグ */

#define MWM_HINTS_FUNCTIONS     (1<<0)
#define MWM_HINTS_DECORATIONS   (1<<1)
#define MWM_HINTS_INPUT_MODE    (1<<2)

#define MWM_FUNC_ALL            (1<<0)
#define MWM_FUNC_RESIZE         (1<<1)
#define MWM_FUNC_MOVE           (1<<2)
#define MWM_FUNC_MINIMIZE       (1<<3)
#define MWM_FUNC_MAXIMIZE       (1<<4)
#define MWM_FUNC_CLOSE          (1<<5)

#define MWM_INPUT_MODELESS                  0
#define MWM_INPUT_PRIMARY_APPLICATION_MODAL 1
#define MWM_INPUT_SYSTEM_MODAL              2
#define MWM_INPUT_FULL_APPLICATION_MODAL    3

#define MWM_DECOR_ALL           (1<<0)
#define MWM_DECOR_BORDER        (1<<1)
#define MWM_DECOR_RESIZEH       (1<<2)
#define MWM_DECOR_TITLE         (1<<3)
#define MWM_DECOR_MENU          (1<<4)
#define MWM_DECOR_MINIMIZE      (1<<5)
#define MWM_DECOR_MAXIMIZE      (1<<6)

//-----------------------------------------


/** ウィンドウ破棄 */

void __mWindowDestroy(mWindow *p)
{
	if(p->win.sys->xid)
	{
		mX11IC_destroy(p);
	
		XDestroyWindow(XDISP, p->win.sys->xid);

		XSync(XDISP, False);
	}
}

/** ウィンドウ作成 */

int __mWindowNew(mWindow *p)
{
	mWindowSysDat *sys;
	XSetWindowAttributes attr;
	Window id;
	long pid;
	Atom atom[3];
	const char *pstr;
	
	//拡張データ確保
	
	p->win.sys = (mWindowSysDat *)mMalloc(sizeof(mWindowSysDat), TRUE);
	if(!p->win.sys) return -1;
	
	sys = p->win.sys;

	//ウィンドウ属性

	attr.bit_gravity = ForgetGravity;
	attr.override_redirect = (p->win.fStyle & MWINDOW_S_POPUP)? 1: 0;
	attr.do_not_propagate_mask = ButtonPressMask | ButtonReleaseMask | PointerMotionMask| ButtonMotionMask;

	attr.event_mask = ExposureMask | EnterWindowMask | LeaveWindowMask
		| FocusChangeMask | StructureNotifyMask | KeyPressMask | KeyReleaseMask
		| ButtonPressMask | ButtonReleaseMask | PointerMotionMask | PropertyChangeMask;
	
	//ウィンドウ作成
	
	id = XCreateWindow(XDISP, XROOTWINDOW,
		0, 0, 1, 1, 0,
		CopyFromParent, CopyFromParent, CopyFromParent,
		CWBitGravity | CWDontPropagate | CWOverrideRedirect | CWEventMask,
		&attr);

	if(!id) return -1;
	
	sys->xid = id;
	sys->eventmask = attr.event_mask;
	
	//プロセスID
	
	pid = getpid();
	
	mX11SetPropertyCARDINAL(id, mX11GetAtom("_NET_WM_PID"), &pid, 1);
	
	//ウィンドウタイプ
	
	if(p->win.fStyle & MWINDOW_S_DIALOG)
	{
		pstr = "_NET_WM_WINDOW_TYPE_DIALOG";
		
		sys->fMapRequest |= MX11_WIN_MAP_REQUEST_MODAL;
	}
	else if(p->win.fStyle & MWINDOW_S_TOOL)
		pstr = "_NET_WM_WINDOW_TYPE_UTILITY";
	else
		pstr = "_NET_WM_WINDOW_TYPE_NORMAL";
	
	mX11WindowSetWindowType(p, pstr);

	//ユーザータイム
	
	if(!(p->win.fStyle & MWINDOW_S_POPUP)
		&& (MAPP_SYS->fSupport & MX11_SUPPORT_USERTIME))
	{
		//_NET_WM_USER_TIME 用の子ウィンドウ

		sys->usertime_xid = XCreateSimpleWindow(XDISP, id, -1, -1, 1, 1, 0, 0, 0);

		//_NET_WM_USER_TIME_WINDOW セット
		
		mX11SetProperty32(id, "_NET_WM_USER_TIME_WINDOW",
			XA_WINDOW, &(sys->usertime_xid), 1);
		
		mX11WindowSetUserTime(p, 0);
	}
	
	//装飾
	
	mX11WindowSetDecoration(p);
	
	//オーナー
	
	if(p->win.owner && (p->win.fStyle & MWINDOW_S_OWNER))
		XSetTransientForHint(XDISP, id, (p->win.owner)->win.sys->xid);

	//WM_PROTOCOLS

	if(!(p->win.fStyle & MWINDOW_S_POPUP))
	{
		atom[0] = MAPP_SYS->atoms[MX11_ATOM_WM_DELETE_WINDOW];
		atom[1] = MAPP_SYS->atoms[MX11_ATOM_WM_TAKE_FOCUS];
		atom[2] = MAPP_SYS->atoms[MX11_ATOM_NET_WM_PING];
		
		XSetWMProtocols(XDISP, id, atom, 3);
	}
	
	//入力コンテキスト
	
#if !defined(MLIB_NO_INPUT_METHOD)
	
	if(!(p->win.fStyle & MWINDOW_S_NO_IM))
		mX11IC_init(p);
	
#endif

	return 0;
}

/** ウィンドウ表示 */

void __mWindowShow(mWindow *p)
{
	mX11WindowSetUserTime(p, MAPP_SYS->timeUser);

	p->win.sys->fMapRequest |= MX11_WIN_MAP_REQUEST_MOVE;

	XMapWindow(XDISP, WINDOW_XID(p));
	XRaiseWindow(XDISP, WINDOW_XID(p));
	XFlush(XDISP);
}

/** ウィンドウ非表示 */

void __mWindowHide(mWindow *p)
{
	XWithdrawWindow(XDISP, WINDOW_XID(p), MAPP_SYS->screen);
	XFlush(XDISP);
}

/** 最小化 */

void __mWindowMinimize(mWindow *p,int type)
{
	if(type)
	{
		if(p->win.sys->fStateReal & MX11_WIN_STATE_REAL_MAP)
			mX11Send_NET_WM_STATE(WINDOW_XID(p), 1, MAPP_SYS->atoms[MX11_ATOM_NET_WM_STATE_HIDDEN], 0);
		else
			p->win.sys->fMapRequest |= MX11_WIN_MAP_REQUEST_HIDDEN;
	}
	else
		mX11Send_NET_WM_STATE(WINDOW_XID(p), 0, MAPP_SYS->atoms[MX11_ATOM_NET_WM_STATE_HIDDEN], 0);
}

/** 最大化 */

void __mWindowMaximize(mWindow *p,int type)
{
	if(type)
	{
		//------ 最大化

		if(p->win.sys->fStateReal & MX11_WIN_STATE_REAL_MAP)
		{
			mX11Send_NET_WM_STATE(WINDOW_XID(p), 1,
				MAPP_SYS->atoms[MX11_ATOM_NET_WM_STATE_MAXIMIZED_HORZ],
				MAPP_SYS->atoms[MX11_ATOM_NET_WM_STATE_MAXIMIZED_VERT]);
		}
		else
			p->win.sys->fMapRequest |= MX11_WIN_MAP_REQUEST_MAXIMIZE;
	}
	else
	{
		mX11Send_NET_WM_STATE(WINDOW_XID(p), 0,
			MAPP_SYS->atoms[MX11_ATOM_NET_WM_STATE_MAXIMIZED_HORZ],
			MAPP_SYS->atoms[MX11_ATOM_NET_WM_STATE_MAXIMIZED_VERT]);
	}
}

/** 位置変更 */

void __mWindowMove(mWindow *p,int x,int y)
{
	XWindowChanges wc;

	//非表示時はMAP時にセット

	if(!(p->wg.fState & MWIDGET_STATE_VISIBLE))
		p->win.sys->fMapRequest |= MX11_WIN_MAP_REQUEST_MOVE;

	//

	wc.x = x;
	wc.y = y;

	XReconfigureWMWindow(XDISP, WINDOW_XID(p), MAPP_SYS->screen, CWX | CWY, &wc);
}

/** サイズ変更 */

void __mWindowResize(mWindow *p,int w,int h)
{
	XWindowChanges wc;
	
	wc.width  = w;
	wc.height = h;

	XReconfigureWMWindow(XDISP, WINDOW_XID(p), MAPP_SYS->screen,
		CWWidth | CWHeight, &wc);
}

/** ポインタをグラブ */

mBool __mWindowGrabPointer(mWindow *p,mCursor cur,int device_id)
{
#if defined(MLIB_ENABLE_PENTABLET) && defined(HAVE_XEXT_XINPUT2)

	/* XI2 を有効にした X ウィンドウ上で、ボタンが押されている状態でグラブを行う場合、
	 * グラブ対象のウィジェットが XI2 を必要としない場合でも
	 * 常に XI2 側でグラブを行わなければ、正しくグラブされない。
	 * 
	 * そのため、常に XI2 でグラブを試し、失敗した場合は通常グラブとする。 */

	if(MAPP_SYS->xi2_opcode != -1)
	{
		if(mX11XI2_pt_grab(WINDOW_XID(p), (Cursor)cur, device_id))
			return TRUE;
	}

#endif

	return (XGrabPointer(XDISP, WINDOW_XID(p), False,
			ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
			GrabModeAsync, GrabModeAsync,
			None, (Cursor)cur, CurrentTime) == GrabSuccess);
}

/** グラブを解放 */

void __mWindowUngrabPointer(mWindow *p)
{
#if defined(MLIB_ENABLE_PENTABLET) && defined(HAVE_XEXT_XINPUT2)

	if(MAPP_SYS->xi2_opcode != -1 && MAPP_SYS->xi2_grab)
	{
		mX11XI2_pt_ungrab();
		return;
	}

#endif

	XUngrabPointer(XDISP, CurrentTime);
	XFlush(XDISP);
}

/** キーボードをグラブ */

mBool __mWindowGrabKeyboard(mWindow *p)
{
	return (XGrabKeyboard(XDISP, WINDOW_XID(p), False,
		GrabModeAsync, GrabModeAsync, CurrentTime) == GrabSuccess);
}

/** キーボードのグラブを解除 */

void __mWindowUngrabKeyboard(mWindow *p)
{
	XUngrabKeyboard(XDISP, CurrentTime);
	XFlush(XDISP);
}

/** カーソルセット
 *
 * @param cur 0 で通常カーソルに戻る */

void __mWindowSetCursor(mWindow *win,mCursor cur)
{
	if(win->win.sys->cursorCur != cur)
	{
		win->win.sys->cursorCur = cur;
	
		XDefineCursor(XDISP, WINDOW_XID(win), cur);
		XFlush(XDISP);
	}
}

/** アイコンイメージをセット
 *
 * @param img 32bit イメージ */

void __mWindowSetIcon(mWindow *win,mImageBuf *img)
{
	unsigned long *buf,*pd;
	uint8_t *ps;
	int pixnum,i;

	pixnum = img->w * img->h;

	buf = (unsigned long *)mMalloc((2 + pixnum) * sizeof(long), FALSE);
	if(!buf) return;

	/* [0] 幅 [1] 高さ [2-] イメージ(long) */

	pd = buf;
	ps = img->buf;

	*(pd++) = img->w;
	*(pd++) = img->h;

	for(i = pixnum; i > 0; i--, ps += 4)
		*(pd++) = M_RGBA(ps[0], ps[1], ps[2], ps[3]);
	
	//セット
	
	XChangeProperty(XDISP, WINDOW_XID(win),
			mX11GetAtom("_NET_WM_ICON"), XA_CARDINAL, 32, PropModeReplace,
			(unsigned char *)buf, 2 + pixnum);
 
	mFree(buf);
}


//===============================
// mWindow-
//===============================


/** @addtogroup window
@{ */

/** ウィンドウタイトルセット
 *
 * @param title NULL で空文字列 */

void mWindowSetTitle(mWindow *p,const char *title)
{
	int len;

	len = (title)? strlen(title): 0;

	XChangeProperty(XDISP, WINDOW_XID(p), mX11GetAtom("_NET_WM_NAME"),
		MAPP_SYS->atoms[MX11_ATOM_UTF8_STRING], 8, PropModeReplace, (unsigned char *)title, len);

	XChangeProperty(XDISP, WINDOW_XID(p), mX11GetAtom("_NET_WM_ICON_NAME"),
		MAPP_SYS->atoms[MX11_ATOM_UTF8_STRING], 8, PropModeReplace, (unsigned char *)title, len);
}

/** D&D を有効にする */

void mWindowEnableDND(mWindow *p)
{
	Atom ver = 5;

	//"XdndAware" プロパティに DND バージョンセット

	XChangeProperty(XDISP, WINDOW_XID(p),
		mX11GetAtom("XdndAware"), XA_ATOM, 32,
		PropModeReplace, (unsigned char *)&ver, 1);
}

/** ペンタブレットからの情報を受け取る */

void mWindowEnablePenTablet(mWindow *p)
{
#if defined(MLIB_ENABLE_PENTABLET) && defined(HAVE_XEXT_XINPUT2)

	if(MAPP_SYS->xi2_opcode != -1)
		mX11XI2_pt_selectEvent(WINDOW_XID(p));

#endif
}

/** ウィンドウをアクティブにする */

void mWindowActivate(mWindow *p)
{
	mX11WindowSetUserTime(p, MAPP_SYS->timeUser);
	
	XSetInputFocus(XDISP, WINDOW_XID(p), RevertToParent, MAPP_SYS->timeLast);
}

/** 最小化されているか */

mBool mWindowIsMinimized(mWindow *p)
{
	return ((p->win.sys->fStateReal & MX11_WIN_STATE_REAL_HIDDEN) != 0);
}

/** 最大化されているか */

mBool mWindowIsMaximized(mWindow *p)
{
	return ((p->win.sys->fStateReal & MX11_WIN_STATE_REAL_MAXIMIZED_HORZ)
			&& (p->win.sys->fStateReal & MX11_WIN_STATE_REAL_MAXIMIZED_VERT));
}

/** 常に最前面 */

void mWindowKeepAbove(mWindow *p,int type)
{
	if(type)
	{
		if(p->win.sys->fStateReal & MX11_WIN_STATE_REAL_MAP)
			mX11Send_NET_WM_STATE(WINDOW_XID(p), 1, MAPP_SYS->atoms[MX11_ATOM_NET_WM_STATE_ABOVE], 0);
		else
			p->win.sys->fMapRequest |= MX11_WIN_MAP_REQUEST_ABOVE;
	}
	else
		mX11Send_NET_WM_STATE(WINDOW_XID(p), 0, MAPP_SYS->atoms[MX11_ATOM_NET_WM_STATE_ABOVE], 0);
}

/** 全画面 */

mBool mWindowFullScreen(mWindow *p,int type)
{
	Atom atom = mX11GetAtom("_NET_WM_STATE_FULLSCREEN");

	if(type)
	{
		if(p->win.sys->fStateReal & MX11_WIN_STATE_REAL_MAP)
			mX11Send_NET_WM_STATE(WINDOW_XID(p), 1, atom, 0);
		else
			p->win.sys->fMapRequest |= MX11_WIN_MAP_REQUEST_FULLSCREEN;
	}
	else
		mX11Send_NET_WM_STATE(WINDOW_XID(p), 0, atom, 0);

	return TRUE;
}

/** ウィンドウ枠の各幅を取得 */

void mWindowGetFrameWidth(mWindow *p,mRect *rc)
{
	long val[4];
	
	if(mX11GetProperty32Array(WINDOW_XID(p),
		MAPP_SYS->atoms[MX11_ATOM_NET_FRAME_EXTENTS], XA_CARDINAL, val, 4))
	{
		rc->x1 = val[0];
		rc->x2 = val[1];
		rc->y1 = val[2];
		rc->y2 = val[3];
	}
	else
		rc->x1 = rc->y1 = rc->x2 = rc->y2 = 0;
}

/** ウィンドウ枠のルート座標位置を取得 */

void mWindowGetFrameRootPos(mWindow *p,mPoint *pt)
{
	Window frame,parent;
	int x,y;
	
	frame = mX11GetFrameWindow(WINDOW_XID(p));

	XTranslateCoordinates(XDISP, frame, MAPP_SYS->root_window, 0, 0, &x, &y, &parent);

	pt->x = x, pt->y = y;
}

/** 設定保存用のウィンドウ位置とサイズ取得
 *
 * 位置は枠を含み、サイズは枠を含まない。\n
 * 最大化中の場合は、元のサイズとなる。 */

void mWindowGetSaveBox(mWindow *p,mBox *box)
{
	mPoint pt;

	mWindowGetFrameRootPos(p, &pt);
	box->x = pt.x;
	box->y = pt.y;

	//サイズ

	if(mWindowIsMaximized(p))
	{
		box->w = p->win.sys->normalW;
		box->h = p->win.sys->normalH;
	}
	else
	{
		box->w = p->wg.w;
		box->h = p->wg.h;
	}
}

/** @} */


//=============================
// mX11Window-
//=============================


/** X イベントマスクセット
 * 
 * @param append TRUE で現在のマスクに追加。FALSE でそのままセット。 */

void mX11WindowSetEventMask(mWindow *p,long mask,mBool append)
{
	if(append)
		p->win.sys->eventmask |= mask;
	else
		p->win.sys->eventmask = mask;
	
	XSelectInput(XDISP, WINDOW_XID(p), p->win.sys->eventmask);
}

/** ウィンドウタイプをプロパティにセット */

void mX11WindowSetWindowType(mWindow *p,const char *name)
{
	Atom type;
	
	type = mX11GetAtom(name);
	
	mX11SetProperty32(WINDOW_XID(p), "_NET_WM_WINDOW_TYPE", XA_ATOM, &type, 1);
}

/** ユーザータイムをセット */

void mX11WindowSetUserTime(mWindow *p,unsigned long time)
{
	if(p->win.sys->usertime_xid)
	{
		mX11SetPropertyCARDINAL(p->win.sys->usertime_xid,
			MAPP_SYS->atoms[MX11_ATOM_NET_WM_USER_TIME], &time, 1);
	}
}

/** ウィンドウ装飾セット */

void mX11WindowSetDecoration(mWindow *p)
{
	/* [0]flags [1]functions [2]decorations [3]inputmode */
	long val[4];
	uint32_t style;
	Atom prop;
	
	style = p->win.fStyle;

	val[0] = MWM_HINTS_FUNCTIONS | MWM_HINTS_DECORATIONS | MWM_HINTS_INPUT_MODE;
	val[1] = MWM_FUNC_MOVE;
	val[2] = 0;
	val[3] = MWM_INPUT_MODELESS;

	if(!(style & MWINDOW_S_NO_RESIZE))
		val[1] |= MWM_FUNC_RESIZE;

	if(style & MWINDOW_S_TITLE)
		val[2] |= MWM_DECOR_TITLE;

	if(style & MWINDOW_S_CLOSE)
		val[1] |= MWM_FUNC_CLOSE;

	if(style & MWINDOW_S_BORDER)
		val[2] |= MWM_DECOR_BORDER;

	if(style & MWINDOW_S_SYSMENU)
		val[2] |= MWM_DECOR_MENU;

	if(style & MWINDOW_S_MINIMIZE)
	{
		val[2] |= MWM_DECOR_MINIMIZE;
		val[1] |= MWM_FUNC_MINIMIZE;
	}

	if((style & MWINDOW_S_MAXIMIZE) &&
		!(style & MWINDOW_S_NO_RESIZE))
	{
		val[2] |= MWM_DECOR_MAXIMIZE;
		val[1] |= MWM_FUNC_MAXIMIZE;
	}

	//

	prop = mX11GetAtom("_MOTIF_WM_HINTS");

	XChangeProperty(XDISP, WINDOW_XID(p), prop, prop, 32,
		PropModeReplace, (unsigned char *)val, 4);
}

