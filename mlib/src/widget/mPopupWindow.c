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
 * mPopupWindow
 *****************************************/

#include "mDef.h"

#include "mPopupWindow.h"

#include "mWidget.h"
#include "mWindow.h"
#include "mGui.h"
#include "mEvent.h"
#include "mKeyDef.h"


/*********************//**

@defgroup popupwindow mPopupWindow
@brief ポップアップウィンドウ

<h3>動作について</h3>
- カーソルがウィンドウ外にある場合は、ウィンドウ外でのボタン押しを検知するため、ポインタをグラブする。
- カーソルがウィンドウ内にある場合は、内部ウィジェットを動作させるために、グラブは解除する。
- ウィンドウ外がクリックされた時、または ESC キーで終了。

<h3>quit ハンドラ</h3>
ウィンドウ外でクリックされたり ESC キーが押された時に、独自の処理を行いたい場合は、mPopupWindowData::quit にハンドラ関数をセットする。\n
ハンドラの戻り値は TRUE で終了、FALSE で終了しない。\n
quit が NULL の場合は常に終了 (デフォルト)。

<h3>継承</h3>
mWidget \> mContainer \> mWindow \> mPopupWindow

@ingroup group_window
@{

@file mPopupWindow.h
@def M_POPUPWINDOW(p)
@struct _mPopupWindow
@struct mPopupWindowData

*************************/



/** 作成
 *
 * @param style ウィンドウスタイル。MWINDOW_S_POPUP,MWINDOW_S_NO_IM は常に ON。 */

mPopupWindow *mPopupWindowNew(int size,mWindow *owner,uint32_t style)
{
	mPopupWindow *p;
	
	if(size < sizeof(mPopupWindow)) size = sizeof(mPopupWindow);
	
	p = (mPopupWindow *)mWindowNew(size, owner,
		style | MWINDOW_S_POPUP | MWINDOW_S_NO_IM);
	if(!p) return NULL;
	
	p->wg.event = mPopupWindowEventHandle;
	p->wg.fEventFilter |= MWIDGET_EVENTFILTER_POINTER | MWIDGET_EVENTFILTER_KEY;

	return p;
}

/** 実行
 *
 * @attention 表示と移動は行うが、サイズ変更は行わない。*/

void mPopupWindowRun(mPopupWindow *p,int rootx,int rooty)
{
	mWidgetMove(M_WIDGET(p), rootx, rooty);
	mWidgetShow(M_WIDGET(p), 1);

	mAppRunPopup(M_WINDOW(p));
}

/** 実行 (移動は行わず、表示のみ) */

void mPopupWindowRun_show(mPopupWindow *p)
{
	mWidgetShow(M_WIDGET(p), 1);

	mAppRunPopup(M_WINDOW(p));
}

/** ポップアップを終了
 *
 * quit() ハンドラがある場合はその戻り値で終了を判定する。\n
 * ハンドラが NULL の場合は常に終了。 */

void mPopupWindowQuit(mPopupWindow *p,mBool bCancel)
{
	if(!p->pop.quit || (p->pop.quit)(p, bCancel))
	{
		/* ungrab 時に LEAVE イベントが起こるので、
		 * 再びグラブされるのを防ぐため、フラグをセット */
	
		p->pop.bEnd = TRUE;
	
		mWidgetUngrabPointer(M_WIDGET(p));
		mAppQuit();
	}
}


//========================
// ハンドラ
//========================


/** グラブ */

static void _grab_onoff(mWidget *p,mBool on)
{
	if(M_POPUPWINDOW(p)->pop.bEnd) return;

	if(on)
		mWidgetGrabPointer(p);
	else
		mWidgetUngrabPointer(p);
}

/** カーソル位置によってグラブをON/OFF */

static void _grab_cur(mWidget *wg)
{
	_grab_onoff(wg, !mWidgetIsCursorIn(wg));
}

/** イベント */

int mPopupWindowEventHandle(mWidget *wg,mEvent *ev)
{
	switch(ev->type)
	{
		case MEVENT_POINTER:
			if(ev->pt.type == MEVENT_POINTER_TYPE_MOTION)
			{
				_grab_onoff(wg, !mWidgetIsContain(wg, ev->pt.x, ev->pt.y));
			}
			else if(ev->pt.type == MEVENT_POINTER_TYPE_PRESS)
			{
				//ウィンドウ外でクリックされた時は終了

				if(!mWidgetIsContain(wg, ev->pt.x, ev->pt.y))
					mPopupWindowQuit(M_POPUPWINDOW(wg), TRUE);
			}
			break;

		case MEVENT_ENTER:
			_grab_cur(wg);
			break;
		case MEVENT_LEAVE:
			_grab_cur(wg);
			break;
		
		case MEVENT_KEYDOWN:
			//ESC キーで終了
			if(ev->key.code == MKEY_ESCAPE)
				mPopupWindowQuit(M_POPUPWINDOW(wg), TRUE);
			break;

		//表示時
		case MEVENT_MAP:
			_grab_cur(wg);
			break;
		default:
			return FALSE;
	}

	return TRUE;
}

/** @} */
