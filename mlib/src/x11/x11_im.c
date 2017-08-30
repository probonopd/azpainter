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
 * (X11) 入力メソッド
 *****************************************/
/*
 * [!] XFilterEvent() では KeyRelease はフィルタリングされないので、
 *     前編集中のキー離しイベントはトップウィンドウに送られてくる。
 */


#include "mSysX11.h"

#include "mWindowDef.h"
#include "mStr.h"
#include "mWidget.h"
#include "mWindow.h"
#include "mFont.h"
#include "mPixbuf.h"
#include "mGui.h"
#include "mUtilCharCode.h"
#include "mSysCol.h"

#include "x11_im.h"
#include "x11_window.h"


//-------------------

#define _IM_WINDOW(p)  ((mX11IMWindow *)(p))
#define _WINSYS(p)     ((p)->win.sys)

//-------------------

typedef struct _mX11IMWindow
{
	mWidget wg;
	mContainerData ct;
	mWindowData win;
	
	mStr str;
}mX11IMWindow;

//-------------------

static void _edit_done(mWindow *win);
static void _move_imwindow(mWindow *win);
static void _get_imwindow_pos(mWindow *win,mPoint *pt);

//-------------------



//************************************
// mX11IMWindow
// (前編集用のポップアップウィンドウ)
//************************************


/** 破棄ハンドラ */

static void _destroy_imwindow(mWidget *wg)
{
	mStrFree(&_IM_WINDOW(wg)->str);
}

/** 作成 */

mX11IMWindow *mX11IMWindow_new(mWindow *client)
{
	mX11IMWindow *p;
	
	p = (mX11IMWindow *)mWindowNew(sizeof(mX11IMWindow),
			NULL, MWINDOW_S_POPUP | MWINDOW_S_NO_IM);

	if(p)
	{
		p->wg.destroy = _destroy_imwindow;
		p->wg.draw = NULL;

		//exposure イベントのみ受け取る

		mX11WindowSetEventMask(M_WINDOW(p), ExposureMask, FALSE);

		//フォントをクライアントのフォーカスウィジェットと同じに

		p->wg.font = mWidgetGetFont(
			(client->win.focus_widget)? client->win.focus_widget: M_WIDGET(client));
	}
	
	return p;
}

/** テキスト描画
 *
 * @return 次の描画位置 */

static int _imwindow_drawText(mPixbuf *pixbuf,mFont *font,
	int x,char *text,int len,XIMFeedback fb)
{
	int tw;
	uint32_t col;
	
	tw = mFontGetTextWidth(font, text, len);

	//反転

	if(fb & XIMReverse)
	{
		mPixbufFillBox(pixbuf, x, 0, tw, font->height, 0);
		col = 0xffffff;
	}
	else
		col = 0;

	//下線
	
	if(fb & XIMUnderline)
		mPixbufLineH(pixbuf, x, font->height, tw, 0);

	//テキスト
	
	mFontDrawText(font, pixbuf, x, 0, text, len, col);
	
	return x + tw;
}

/** 描画 */

static void _imwindow_draw(mX11IMWindow *p,mFont *font,XIMFeedback *feedback,int fblen)
{
	mPixbuf *pixbuf;
	int i,start,pos,x;
	XIMFeedback fb,last_fb;
	
	if(mStrIsEmpty(&p->str)) return;

	pixbuf = p->win.pixbuf;

	//背景
	
	mPixbufFillBox(pixbuf, 0, 0, p->wg.w, p->wg.h, MSYSCOL(WHITE));
		
	//テキスト
	
	if(!feedback)
		mFontDrawText(font, pixbuf, 0, 0, p->str.buf, p->str.len, 0);
	else
	{
		//属性あり
	
		x = 0;
		pos = 0;
		start = -1;
		last_fb = 0;
	
		for(i = 0; i < fblen; i++)
		{
			fb = feedback[i] & (XIMReverse | XIMUnderline);
			
			if(fb != last_fb)
			{
				if(start >= 0)
				{
					x = _imwindow_drawText(pixbuf, font, x,
							p->str.buf + start, pos - start, last_fb);
				}
				
				last_fb = fb;
				start = pos;
			}
			
			pos += mUTF8CharWidth(p->str.buf + pos);
		}

		if(start >= 0)
		{
			_imwindow_drawText(pixbuf, font, x,
				p->str.buf + start, pos - start, last_fb);
		}
	}
}


//************************************
// IM/IC コールバック
//************************************


/** IM が使用可能な状態になった時 */

static void _cb_instantiate(Display *disp,XPointer client,XPointer data)
{
	mX11IM_init();
	
	//コールバック登録解除
	
	XUnregisterIMInstantiateCallback(disp, NULL, NULL, NULL, _cb_instantiate, data);
}

/** IM が何らかの理由で停止した時
 * 
 * [!] これ以降、XCloseIM() や XDestroyIC() を呼んではならない */

static void _cb_im_destroy(XIM *xim,XPointer client,XPointer empty)
{
	MAPP_SYS->im_xim = NULL;
}

/** 前編集開始
 *
 * return : 前編集文字列の最大長 (-1で無限) */

static int _cb_ic_start(XIC *xic,XPointer client,XPointer empty)
{
	mWindow *win = M_WINDOW(client);
	
	_edit_done(win);
	
	_WINSYS(win)->im_editwin = mX11IMWindow_new(win);

	return -1;
}

/** 前編集終了 */

static void _cb_ic_done(XIC *xic,XPointer client,XPointer empty)
{
	_edit_done(M_WINDOW(client));
}

/** 描画 */
/*
  [!] 変換中に確定キーを押さずに次の文字を入力した場合は、done が来ずに draw が続く。

	[XIMPreeditDrawCallbackStruct]
		int caret;      //文字列内でのカーソル位置 (文字数)
		int chg_first;  //変換対象部分の開始位置
		int chg_length; //変換対象部分の文字数
		XIMText *text;  //文字 (NULLで削除)

	[XIMText]
		unsigned short length;  //テキスト文字数
		XIMFeedback * feedback; //各文字における表示タイプの配列
		Bool encoding_is_wchar; //ワイド文字か
		union {
			char * multi_byte;
			wchar_t * wide_char;
		} string;
*/

static void _cb_ic_draw(XIC *xic,XPointer client,XIMPreeditDrawCallbackStruct *dat)
{
	mWindow *win;
	mX11IMWindow *editwin;
	mFont *font;
	mPoint pt;

	win = M_WINDOW(client);
	editwin = _WINSYS(win)->im_editwin;
	
	if(!editwin) return;
	
	//テキストがない場合非表示

	if(!dat->text || !dat->text->string.multi_byte)
	{
		mWidgetShow(M_WIDGET(editwin), 0);
		return;
	}
	
	//テキストを UTF-8 変換
	
	if(dat->text->encoding_is_wchar)
		mStrSetTextWide(&editwin->str, dat->text->string.wide_char, dat->text->length);
	else
		mStrSetTextLocal(&editwin->str, dat->text->string.multi_byte, -1);

	//サイズ

	font = mWidgetGetFont(M_WIDGET(editwin));
	
	mWidgetResize(M_WIDGET(editwin),
		mFontGetTextWidth(font, editwin->str.buf, editwin->str.len),
		font->height + 1);

	//ウィンドウ位置
	
	_get_imwindow_pos(win, &pt);

	mWindowMoveAdjust(M_WINDOW(editwin), pt.x, pt.y, TRUE);
	
	//描画
	
	_imwindow_draw(editwin, font, dat->text->feedback, dat->text->length);
	
	mWidgetUpdate(M_WIDGET(editwin));

	//表示

	mWidgetShow(M_WIDGET(editwin), (editwin->str.len != 0));
}

/** テキスト挿入位置移動 */

static void _cb_ic_caret(XIC *xic,XPointer client,XIMPreeditCaretCallbackStruct *calldat)
{

}


//************************************
// XIM
// (アプリケーションに一つ)
//************************************


/** IM 閉じる */

void mX11IM_close(void)
{
	if(MAPP_SYS->im_xim)
		XCloseIM(MAPP_SYS->im_xim);
}

/** IM 初期化 */

void mX11IM_init(void)
{
	XIM xim;
	XIMStyles *styles;
	XIMCallback cbDestroy;
	int i,j,k;
	XIMStyle style,
		style_pe[3] = {XIMPreeditCallbacks, XIMPreeditNothing, XIMPreeditNone},
		style_st[2] = {XIMStatusNothing, XIMStatusNone};

	//IM 開く

	xim = XOpenIM(XDISP, NULL, NULL, NULL);
	if(!xim)
	{
		XRegisterIMInstantiateCallback(XDISP, NULL, NULL, NULL, _cb_instantiate, NULL);
		return;
	}

	//サポートされているスタイル

	if(XGetIMValues(xim, XNQueryInputStyle, &styles, (void *)0) != NULL)
	{
		XCloseIM(xim);
		return;
	}
	
	for(i = 0; i < 3; i++)
	{
		for(j = 0; j < 2; j++)
		{
			style = style_pe[i] | style_st[j];
			
			for(k = 0; k < styles->count_styles; k++)
			{
				if(styles->supported_styles[k] == style)
					goto ENDSTYLE;
			}
		}
	}
	
	style = 0;

ENDSTYLE:

	XFree(styles);

	if(style == 0)
	{
		XCloseIM(xim);
		return;
	}
	
	//破棄コールバック

	cbDestroy.callback    = (XIMProc)_cb_im_destroy;
	cbDestroy.client_data = NULL;

	XSetIMValues(xim,
		XNDestroyCallback, &cbDestroy,
		(void *)0);

	//

	MAPP_SYS->im_xim   = xim;
	MAPP_SYS->im_style = style;
}


//************************************
// XIC
// (各トップウィンドウに一つ)
//************************************


/** IC 削除 */

void mX11IC_destroy(mWindow *win)
{
	mWindowSysDat *sys = win->win.sys;

	//編集ウィンドウ

	if(sys->im_editwin)
		mWidgetDestroy(M_WIDGET(sys->im_editwin));

	//[!] IM が停止している場合は破棄処理は行なってはいけない

	if(sys->im_xic && MAPP_SYS->im_xim)
		XDestroyIC(sys->im_xic);
}

/** IC 初期化 */

void mX11IC_init(mWindow *win)
{
	mAppSystem *appsys;
	XIC xic;
	Window winid;
	XIMCallback cb[4];
	XVaNestedList listp;
	int i;
	char *pc;
	long mask;
	
	appsys = MAPP_SYS;

	if(!appsys->im_xim) return;
	
	winid = WINDOW_XID(win);
	
	if(appsys->im_style & XIMPreeditCallbacks)
	{
		//-------- コールバック

		//前編集属性

		cb[0].callback = (XIMProc)_cb_ic_start;
		cb[1].callback = (XIMProc)_cb_ic_done;
		cb[2].callback = (XIMProc)_cb_ic_draw;
		cb[3].callback = (XIMProc)_cb_ic_caret;

		for(i = 0; i < 4; i++)
			cb[i].client_data = (XPointer)win;

		listp = XVaCreateNestedList(0,
					XNPreeditStartCallback, cb,
					XNPreeditDoneCallback,  cb + 1,
					XNPreeditDrawCallback,  cb + 2,
					XNPreeditCaretCallback, cb + 3,
					(void *)0);

		//作成

		xic = XCreateIC(appsys->im_xim,
					XNInputStyle,    appsys->im_style,
					XNClientWindow,  winid,
					XNFocusWindow,   winid,
					XNPreeditAttributes, listp,
					(void *)0);

		XFree(listp);
	}
	else
	{
		//前編集なし

		xic = XCreateIC(appsys->im_xim,
					XNInputStyle,   appsys->im_style,
					XNClientWindow, winid,
					XNFocusWindow,  winid,
					(void *)0);
	}

	if(!xic) return;

	//リセット

	pc = XmbResetIC(xic);
	if(pc) XFree(pc);

	//ウィンドウが必要な X イベントマスク追加
	/* [!] 64bit OS の場合、上位 32bit に値がセットされないので mask をクリア */

	mask = 0;
	XGetICValues(xic, XNFilterEvents, &mask, (void *)0);

	mX11WindowSetEventMask(win, mask, TRUE);

	//
	
	_WINSYS(win)->im_xic = xic;
}

/** ウィンドウのフォーカスIN/OUT 時 */

void mX11IC_setFocus(mWindow *win,int focusin)
{
	mWindowSysDat *sys = win->win.sys;

	if(MAPP_SYS->im_xim && sys->im_xic)
	{
		if(focusin)
		{
			//フォーカスIN
		
			XSetICFocus(sys->im_xic);
			
			if(sys->im_editwin)
			{
				_move_imwindow(win);
				
				mWidgetShow(M_WIDGET(sys->im_editwin), 1);
			}
		}
		else
		{
			//フォーカスOUT

			XUnsetICFocus(sys->im_xic);
			
			if(sys->im_editwin)
				mWidgetShow(M_WIDGET(sys->im_editwin), 0);
		}
	}
}

/** 前編集終了 */

void _edit_done(mWindow *win)
{
	if(_WINSYS(win)->im_editwin)
	{
		mWidgetDestroy(M_WIDGET(_WINSYS(win)->im_editwin));
		
		_WINSYS(win)->im_editwin = NULL;
	}
}

/** 編集ウィンドウ位置移動 */

void _move_imwindow(mWindow *win)
{
	mPoint pt;
	
	_get_imwindow_pos(win, &pt);
	
	mWidgetMove(M_WIDGET(win->win.sys->im_editwin), pt.x, pt.y);
}

/** 前編集時のウィンドウ表示位置取得 */

void _get_imwindow_pos(mWindow *win,mPoint *pt)
{
	mWidget *wg;
	
	wg = (win->win.focus_widget)? win->win.focus_widget: M_WIDGET(win);
	
	pt->x = wg->imX;
	pt->y = wg->imY;
	
	mWidgetMapPoint(wg, NULL, pt);
}

