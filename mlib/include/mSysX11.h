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

#ifndef MLIB_SYSX11_H
#define MLIB_SYSX11_H

/*---- include ----*/

#include "mConfig.h"

#include <X11/Xlib.h>

#ifdef MINC_X11_ATOM
#include <X11/Xatom.h>
#endif

#ifdef MINC_X11_UTIL
#include <X11/Xutil.h>
#endif

#ifdef MINC_X11_LOCALE
#include <X11/Xlocale.h>
#endif

#ifdef MINC_X11_XKB
#include <X11/XKBlib.h>
#endif

#if defined(MINC_X11_XSHM) && defined(HAVE_XEXT_XSHM)
#include <X11/extensions/XShm.h>
#endif

#if defined(MINC_X11_XI2) && defined(MLIB_ENABLE_PENTABLET) && defined(HAVE_XEXT_XINPUT2)
#include <X11/extensions/XInput2.h>
#endif

#include "mDef.h"
#include "mAppDef.h"
#include "mListDef.h"

#if !defined(MLIB_NO_THREAD)
#include "mThread.h"
#endif

/*---- macro ----*/

#define XDISP           (g_mApp->sys->disp)
#define XROOTWINDOW     (g_mApp->sys->root_window)
#define WINDOW_XID(p)   ((p)->win.sys->xid)
#define MX11ATOM(name)  MAPP_SYS->atoms[MX11_ATOM_ ## name]

enum MX11_ATOM_INDEX
{
	MX11_ATOM_MLIB_SELECTION,
	MX11_ATOM_WM_PROTOCOLS,
	MX11_ATOM_WM_DELETE_WINDOW,
	MX11_ATOM_WM_TAKE_FOCUS,
	MX11_ATOM_NET_WM_PING,
	MX11_ATOM_NET_WM_USER_TIME,
	MX11_ATOM_NET_FRAME_EXTENTS,
	MX11_ATOM_NET_WM_STATE,
	MX11_ATOM_NET_WM_STATE_MAXIMIZED_VERT,
	MX11_ATOM_NET_WM_STATE_MAXIMIZED_HORZ,
	MX11_ATOM_NET_WM_STATE_HIDDEN,
	MX11_ATOM_NET_WM_STATE_MODAL,
	MX11_ATOM_NET_WM_STATE_ABOVE,
	MX11_ATOM_UTF8_STRING,
	MX11_ATOM_COMPOUND_TEXT,
	MX11_ATOM_CLIPBOARD,
	MX11_ATOM_TARGETS,
	MX11_ATOM_MULTIPLE,
	MX11_ATOM_CLIPBOARD_MANAGER,
	MX11_ATOM_XdndEnter,
	MX11_ATOM_XdndPosition,
	MX11_ATOM_XdndLeave,
	MX11_ATOM_XdndDrop,
	MX11_ATOM_XdndStatus,
	MX11_ATOM_XdndActionCopy,
	MX11_ATOM_text_uri_list,

	MX11_ATOM_NUM
};

/*---- struct ----*/

typedef struct FT_LibraryRec_ *FT_Library;
typedef struct _mX11IMWindow mX11IMWindow;
typedef struct _mX11Clipboard mX11Clipboard;
typedef struct _mX11DND mX11DND;

struct _mAppSystem
{
	Display	*disp;
	Window	root_window;
	Visual	*visual;
	Colormap colormap;
	int		screen,
			connection,
			select_fdnum,
			xi2_opcode,
			xi2_grab,
			xi2_grab_deviceid;
	uint32_t fSupport,
		fState;
	GC		gc_def;
	Time	timeUser,
			timeLast;
	
	XIM      im_xim;
	XIMStyle im_style;
	
	Window dbc_window;
	Time dbc_time;
	int dbc_x,dbc_y,dbc_btt;
	
	FT_Library ftlib;
	
	mList listTimer,
		listXiDev;
	
	mX11Clipboard *clipb;
	mX11DND *dnd;

#if !defined(MLIB_NO_THREAD)
	mMutex mutex;
	int fd_pipe[2];
#endif

	Atom atoms[MX11_ATOM_NUM];

	mCursor cursor_hsplit,
		cursor_vsplit;
};

struct _mWindowSysDat
{
	Window xid,
		usertime_xid;
	long eventmask;
	mCursor cursorCur;

	XIC im_xic;
	mX11IMWindow *im_editwin;

	int normalW,normalH;
	uint8_t fStateReal,
		fMapRequest;
};

/*---- enum ----*/

enum MX11_SUPPORT_FLAGS
{
	MX11_SUPPORT_XSHM     = 1<<0,
	MX11_SUPPORT_USERTIME = 1<<1
};

enum MX11_STATE_FLAGS
{
	MX11_STATE_MAP_FIRST_WINDOW = 1<<0
};

enum MX11_WIN_STATE_REAL_FLAGS
{
	MX11_WIN_STATE_REAL_MAP = 1<<0,
	MX11_WIN_STATE_REAL_MAXIMIZED_VERT = 1<<1,
	MX11_WIN_STATE_REAL_MAXIMIZED_HORZ = 1<<2,
	MX11_WIN_STATE_REAL_HIDDEN = 1<<3,
	MX11_WIN_STATE_REAL_ABOVE  = 1<<4,
};

enum MX11_WIN_MAP_REQUEST_FLAGS
{
	MX11_WIN_MAP_REQUEST_MAXIMIZE = 1<<0,
	MX11_WIN_MAP_REQUEST_HIDDEN   = 1<<1,
	MX11_WIN_MAP_REQUEST_MODAL    = 1<<2,
	MX11_WIN_MAP_REQUEST_ABOVE    = 1<<3,
	MX11_WIN_MAP_REQUEST_MOVE     = 1<<4,
	MX11_WIN_MAP_REQUEST_FULLSCREEN = 1<<5
};

#endif
