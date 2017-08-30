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

/****************************************
 * <X11> ユーティリティ関数
 ****************************************/

#include <stdlib.h>
#include <string.h>
#include <sys/select.h>

#define MINC_X11_ATOM
#define MINC_X11_UTIL
#include "mSysX11.h"

#include "mAppDef.h"
#include "mNanoTime.h"
#include "mUtilCharCode.h"
#include "mStr.h"



/** 名前からアトム識別子取得 */

Atom mX11GetAtom(const char *name)
{
	return XInternAtom(XDISP, name, False);
}


//===================================
// プロパティデータセット
//===================================


/** プロパティに long の値をセット */

void mX11SetProperty32(Window id,const char *name,Atom proptype,void *val,int num)
{
	XChangeProperty(XDISP, id, mX11GetAtom(name), proptype, 32,
		PropModeReplace, (unsigned char *)val, num);
}

/** プロパティに CARDINAL(long) 値をセット */

void mX11SetPropertyCARDINAL(Window id,Atom prop,void *val,int num)
{
	XChangeProperty(XDISP, id, prop, XA_CARDINAL, 32,
		PropModeReplace, (unsigned char *)val, num);
}

/** プロパティに Atom 値をセット */

void mX11SetPropertyAtom(Window id,Atom prop,Atom *atoms,int num)
{
	XChangeProperty(XDISP, id, prop, XA_ATOM, 32,
		PropModeReplace, (unsigned char *)atoms, num);
}

/** プロパティにデータセット (format=8)
 *
 * @param append TRUE で追加。FALSE で置き換え。 */

void mX11SetProperty8(Window id,Atom prop,Atom type,
	const void *buf,long size,mBool append)
{
	int mode;
	unsigned char *ps = (unsigned char *)buf;
	long maxsize,send;

	maxsize = XMaxRequestSize(XDISP) * 4;
	mode    = (append)? PropModeAppend: PropModeReplace;

	while(size)
	{
		send = (size > maxsize)? maxsize: size;

		XChangeProperty(XDISP, id, prop, type, 8, mode, ps, send);

		mode = PropModeAppend;
		ps += send;
		size -= send;
	}
}

/** プロパティに COMPOUND_TEXT 文字列セット
 *
 * @param utf8 UTF-8 文字列 */

void mX11SetPropertyCompoundText(Window id,Atom prop,const char *utf8,int len)
{
	XTextProperty tp;
	wchar_t *pwc;

	pwc = mUTF8ToWide_alloc(utf8, len, NULL);
	if(!pwc) return;

	if(XwcTextListToTextProperty(XDISP, &pwc, 1, XCompoundTextStyle, &tp) == Success)
		XSetTextProperty(XDISP, id, &tp, prop);

	mFree(pwc);

	if(tp.value) XFree(tp.value);
}


//===================================
// プロパティデータ読み込み
//===================================


/** プロパティから long の配列データを指定数読み込み */

mBool mX11GetProperty32Array(Window id,Atom prop,Atom proptype,void *buf,int num)
{
	Atom type;
	int format;
	unsigned long pnum,after;
	unsigned char *pdat;
	mBool ret = FALSE;

	if(XGetWindowProperty(XDISP, id, prop, 0, 1024, False, proptype,
			&type, &format, &pnum, &after, &pdat) == Success)
	{
		if(type == proptype && format == 32 && (int)pnum >= num)
		{
			memcpy(buf, pdat, sizeof(long) * num);
			ret = TRUE;
		}

		XFree(pdat);
	}

	return ret;
}

/** プロパティから long データすべて読み込み
 * 
 * @param proptype 値のタイプ
 * @return 確保したメモリのポインタ (mFree() で解放する) */

void *mX11GetProperty32(Window id,Atom prop,Atom proptype,int *resnum)
{
	Atom type;
	int ret,format;
	unsigned long nitems,after;
	unsigned char *pdat,*buf,*pdst;
	long offset;

	if(prop == 0) return NULL;

	//データサイズ取得 : after

	ret = XGetWindowProperty(XDISP, id, prop, 0, 0,
			0, proptype, &type, &format, &nitems, &after, &pdat);

	if(pdat) XFree(pdat);

	if(ret != Success || format != 32 || type != proptype)
		return NULL;

	//確保
	/* [!] 64bit OS 時の注意
	 * after は (long x データ数) のサイズではなく、(4 x データ数)。
	 * しかし、実際に取得されるデータは (long x データ数) なので注意。
	 * X の内部では 4byte データだが、読み込み時には 4byte -> 8byte 変換される。 */

	after >>= 2;

	buf = (unsigned char *)mMalloc(after * sizeof(long), TRUE);
	if(!buf) return NULL;

	*resnum = after;

	//読み込み
	/* [!] プロパティのデータ位置とサイズは 4byte データとして扱う。 */

	pdst = buf;

	for(offset = 0; after > 0; offset += nitems * 4)
	{
		ret = XGetWindowProperty(XDISP, id, prop, offset / 4, after / 4,
					0, proptype, &type, &format, &nitems, &after, &pdat);

		if(ret != Success)
		{
			mFree(buf);
			return NULL;
		}

		memcpy(pdst, pdat, nitems * sizeof(long));
		pdst += nitems * sizeof(long);

		XFree(pdat);
	}

	return buf;
}

/** プロパティから format=8 データ取得
 * 
 * @param append_null 最後に NULL (1byte) を追加する
 * @param ret_size データのサイズが入る (NULL 可)
 * @return 確保されたデータのポインタ */

void *mX11GetProperty8(Window id,Atom prop,mBool append_null,uint32_t *ret_size)
{
	Atom type;
	int ret,format;
	unsigned long nitems,after,size;
	long offset;
	unsigned char *pdat,*buf;

	if(prop == None) return NULL;

	//データサイズ取得

	ret = XGetWindowProperty(XDISP, id, prop, 0, 0,
				False, AnyPropertyType, &type, &format, &nitems, &after, &pdat);

	if(pdat) XFree(pdat);

	if(ret != Success || type == None || format != 8)
		return NULL;

	//確保
	
	size = after;
	if(append_null) size++;
	
	buf = (unsigned char *)mMalloc(size, FALSE);
	if(!buf) return NULL;
	
	if(ret_size) *ret_size = size;
	
	//読み込み

	for(offset = 0; after > 0; offset += nitems)
	{
		ret = XGetWindowProperty(XDISP, id, prop,
				offset / 4, after / 4 + 1,
				False, AnyPropertyType, &type, &format, &nitems, &after, &pdat);

		if(ret != Success)
		{
			mFree(buf);
			return NULL;
		}

		memcpy(buf + offset, pdat, nitems);
		
		XFree(pdat);
	}
	
	//NULL 追加
	
	if(append_null)
		buf[size - 1] = 0;

	return (void *)buf;
}


//==============================
// イベント関連
//==============================


/** ClientMessage 用にイベント構造体をセット */

void mX11SetEventClientMessage(XEvent *ev,Window win,Atom mestype)
{
	memset(ev, 0, sizeof(XEvent));

	ev->xclient.type         = ClientMessage;
	ev->xclient.display      = XDISP;
	ev->xclient.message_type = mestype;
	ev->xclient.format       = 32;
	ev->xclient.window       = win;
}

/** ルートウィンドウに ClientMessage イベントを送る (format=32)
 * 
 * @param data long 値の配列
 * @param num  data の個数。最大５個 */

void mX11SendClientMessageToRoot(Window id,Atom mestype,void *data,int num)
{
	XEvent ev;
	int i;
	
	if(num > 5) num = 5;

	memset(&ev, 0, sizeof(XEvent));

	ev.xclient.type         = ClientMessage;
	ev.xclient.display      = XDISP;
	ev.xclient.message_type = mestype;
	ev.xclient.format       = 32;
	ev.xclient.window       = id;

	for(i = 0; i < num; i++)
		ev.xclient.data.l[i] = *((long *)data + i);

	XSendEvent(XDISP, XROOTWINDOW, False,
		SubstructureRedirectMask | SubstructureNotifyMask, &ev);
}

/** ルートウィンドウに ClientMessage を使って文字列を送信 (StartupNotify 用) */

void mX11SendClientMessageToRoot_string(const char *mestype,
	const char *mestype_begin,const char *sendstr)
{
	Display *disp = XDISP;
	XSetWindowAttributes attrs;
	XClientMessageEvent ev;
	Window win;
	Atom atom_type,atom_type_begin;
	const char *ps;
	uint8_t *dst;
	int len,size;

	if(!sendstr) return;

	//送信用ウィンドウ作成

	attrs.override_redirect = True;
	attrs.event_mask = PropertyChangeMask | StructureNotifyMask;

	win = XCreateWindow(disp, XROOTWINDOW,
		 -100, -100, 1, 1, 0,
		 CopyFromParent, CopyFromParent,
		 (Visual *)CopyFromParent, CWOverrideRedirect | CWEventMask,
		 &attrs);

	if(!win) return;

	//

	atom_type = mX11GetAtom(mestype);
	atom_type_begin = mX11GetAtom(mestype_begin);

	memset(&ev, 0, sizeof(XClientMessageEvent));

	ev.type = ClientMessage;
	ev.message_type = atom_type_begin;
	ev.display = disp;
	ev.window = win;
	ev.format = 8;

	len = strlen(sendstr) + 1;

	//送信
	/* data 部分が、32bit OS で long x 5 = 20byte あるので、
	 * その部分を使って 20byte ごとにデータを送る。 */

	ps = sendstr;
	dst = (uint8_t *)ev.data.b;

	while(len)
	{
		size = (len <= 20)? len: 20;
		
		memcpy(dst, ps, size);

		if(size < 20)
			memset(dst + size, 0, 20 - size);

		XSendEvent(disp, XROOTWINDOW, False,
			PropertyChangeMask, (XEvent *)&ev);

		ev.message_type = atom_type;

		ps += size;
		len -= size;
	}

	//終了

	XDestroyWindow(disp, win);
	XFlush(disp);
}

/** StartupNotify 通知 (起動完了を示す) */

void mX11SendStartupNotify_complete()
{
	char *pc;
	mStr str = MSTR_INIT;

	//環境変数から ID 文字列取得

	pc = getenv("DESKTOP_STARTUP_ID");

	if(!pc || pc[0] == 0) return;

	//送信文字列

	mStrSetText(&str, "remove: ID=\"");

	for(; *pc; pc++)
	{
		if(*pc == ' ' || *pc == '"' || *pc == '\\')
			mStrAppendChar(&str, '\\');

		mStrAppendChar(&str, *pc);
	}

	mStrAppendChar(&str, '"');

	//送信
	
	mX11SendClientMessageToRoot_string("_NET_STARTUP_INFO",
		"_NET_STARTUP_INFO_BEGIN", str.buf);
	
	mStrFree(&str);

	unsetenv("DESKTOP_STARTUP_ID");
}

/** 指定イベントを受け取る (タイムアウト付き)
 *
 * @param evtype    受け取るイベント
 * @param timeoutms タイムアウト時間 (ミリセカンド)
 * @return 0 で成功。1 でタイムアウト。-1 でエラー。 */

int mX11GetEventTimeout(int evtype,int timeoutms,XEvent *ev)
{
	fd_set fd;
	int xfd,ret;
	struct timeval tv;
	mNanoTime nt_now,nt_end,nt_diff;

	xfd = MAPP_SYS->connection;

	mNanoTimeGet(&nt_end);
	mNanoTimeAddMilliSec(&nt_end, timeoutms);

	while(1)
	{
		//キュー内にあるか

		if(XCheckTypedEvent(XDISP, evtype, ev)) return 0;

		//タイムアウトまでの時間

		mNanoTimeGet(&nt_now);
		if(!mNanoTimeSub(&nt_diff, &nt_end, &nt_now)) return 1;

		//イベントを受け取るまで待つ

		FD_ZERO(&fd);
		FD_SET(xfd, &fd);

		tv.tv_sec  = nt_diff.sec;
		tv.tv_usec = nt_diff.nsec / 1000;

		ret = select(xfd + 1, &fd, NULL, NULL, &tv);
		
		if(ret < 0) return -1;
		else if(ret == 0) return 1;
	}

	return -1;
}


//==============================
// ウィンドウマネージャ関連
//==============================


/** _NET_WM_STATE を送る
 *
 * @param action  [0]はずす [1]追加 [2]トグル */

void mX11Send_NET_WM_STATE(Window id,int action,Atom data1,Atom data2)
{
	long dat[3];
	
	dat[0] = action;
	dat[1] = data1;
	dat[2] = data2;

    mX11SendClientMessageToRoot(id,
		MAPP_SYS->atoms[MX11_ATOM_NET_WM_STATE], dat, 3);
}


//=================================
// セレクション関連
//=================================


/** セレクションから変換要求を行いイベント取得
 * 
 * id のプロパティ "_MLIB_SELECTION" 上にデータが変換される。
 * 
 * @param ev NULL でイベントを格納しない */

mBool mX11SelectionConvert(Window id,Atom selection,Atom target,XEvent *ev)
{
	XEvent xev;

	//変換要求

	XConvertSelection(XDISP, selection, target,
			MAPP_SYS->atoms[MX11_ATOM_MLIB_SELECTION], id, CurrentTime);

	XFlush(XDISP);

	//SelectionNotify イベント取得

	if(mX11GetEventTimeout(SelectionNotify, 2000, &xev))
		return FALSE;
	else
	{
		if(ev) *ev = xev;
		return TRUE;
	}
}

/** セレクションから利用可能なデータタイプ取得
 * 
 * @param target_list  対応可能なアトムのリスト (先頭にあるものほど優先順位が高い)
 * @param target_num   target_list の数
 * @return 実際に利用できるタイプ (0 で失敗) */

Atom mX11GetSelectionTargetType(Window id,
	Atom atom_selection,Atom *target_list,int target_num)
{
	XEvent ev;
	Atom *pAtom,type = 0;
	int i,j,num;
	
	//変換

	if(!mX11SelectionConvert(id, atom_selection,
			MAPP_SYS->atoms[MX11_ATOM_TARGETS], &ev))
		return 0;

	//アトムリスト取得
	
	pAtom = (Atom *)mX11GetProperty32(ev.xselection.requestor,
		ev.xselection.property, XA_ATOM, &num);

	if(!pAtom) return 0;

	//

/*
	char *pc;
	for(i = 0; i < num; i++)
	{
		pc = XGetAtomName(XDISP, pAtom[i]);
		mDebug("- %s\n", pc);
		XFree(pc);
	}
*/

	for(i = 0; i < target_num; i++)
	{
		for(j = 0; j < num; j++)
		{
			if(pAtom[j] == target_list[i])
			{
				type = pAtom[j];
				goto END;
			}
		}
	}

END:
	mFree(pAtom);

	return type;
}

/** セレクションからデータ取得 (format=8) */

void *mX11GetSelectionDat(Window id,Atom selection,Atom target,
	mBool append_null,uint32_t *ret_size)
{
	void *buf;

	//変換

	if(!mX11SelectionConvert(id, selection, target, NULL))
		return NULL;
	
	//取得
	
	buf = mX11GetProperty8(id,
			MAPP_SYS->atoms[MX11_ATOM_MLIB_SELECTION], append_null, ret_size);
	if(!buf) return NULL;

	//プロパティ削除

	XDeleteProperty(XDISP, id, MAPP_SYS->atoms[MX11_ATOM_MLIB_SELECTION]);

	return buf;
}

/** セレクションから COMPOUND_TEXT 文字列を UTF-8 に変換して取得 */

char *mX11GetSelectionCompoundText(Window id,Atom selection)
{
	XTextProperty tp;
	wchar_t **ppwc;
	int num;
	char *utf8 = NULL;
	
	//変換

	if(!mX11SelectionConvert(id, selection,
			MAPP_SYS->atoms[MX11_ATOM_COMPOUND_TEXT], NULL))
		return NULL;

	//文字列リストデータ取得

	XGetTextProperty(XDISP, id, &tp, MAPP_SYS->atoms[MX11_ATOM_MLIB_SELECTION]);

	if(tp.format != 0)
	{
		if(XwcTextPropertyToTextList(XDISP, &tp, &ppwc, &num) == Success)
		{
			utf8 = mWideToUTF8_alloc(*ppwc, -1, NULL);

			XwcFreeStringList(ppwc);
		}

		XFree(tp.value);
	}
	
	//プロパティ削除

	XDeleteProperty(XDISP, id, MAPP_SYS->atoms[MX11_ATOM_MLIB_SELECTION]);

	return utf8;
}


//============================
// ほか
//============================


/** トップレベルウィンドウの枠のウィンドウ取得
 *
 * @return 枠がない場合は同じウィンドウが返る */

Window mX11GetFrameWindow(Window id)
{
	Window root,parent,*child;
	unsigned int num;

	while(1)
	{
		XQueryTree(XDISP, id, &root, &parent, &child, &num);
		if(child) XFree(child);

		if(parent == root) break;

		id = parent;
	}

	return id;
}
