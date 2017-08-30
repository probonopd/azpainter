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
 * <X11> mWidget 関数
 *****************************************/

#include "mSysX11.h"

#include "mWindowDef.h"
#include "mAppTimer.h"


/**

@addtogroup widget
@{

*/


/** ウィジェット座標を指定ウィジェットの相対座標に変換
 * 
 * from,to : NULL でルート */

void mWidgetMapPoint(mWidget *from,mWidget *to,mPoint *pt)
{
	Window id,id_from;
	int x,y;
	
	if(!from)
		id_from = MAPP_SYS->root_window;
	else
	{
		id_from = WINDOW_XID(from->toplevel);

		pt->x += from->absX;
		pt->y += from->absY;
	}
	
	//
	
	XTranslateCoordinates(XDISP,
		id_from, (to)? WINDOW_XID(to->toplevel): MAPP_SYS->root_window,
		pt->x, pt->y, &x, &y, &id);
		
	if(to)
	{
		x -= to->absX;
		y -= to->absY;
	}

	pt->x = x, pt->y = y;
}


/** タイマー追加
 * 
 * 同じ ID のタイマーが存在する場合は置き換わる。
 * 
 * @param msec 間隔 (ミリセカンド) */

void mWidgetTimerAdd(mWidget *p,uint32_t timerid,uint32_t msec,intptr_t param)
{
	mAppTimerAppend(&MAPP_SYS->listTimer, p, timerid, msec, param);
}

/** 指定タイマーが存在するか */

mBool mWidgetTimerIsExist(mWidget *p,uint32_t timerid)
{
	return mAppTimerIsExist(&MAPP_SYS->listTimer, p, timerid);
}

/** タイマー削除
 * 
 * @return タイマーを削除したか */

mBool mWidgetTimerDelete(mWidget *p,uint32_t timerid)
{
	return mAppTimerDelete(&MAPP_SYS->listTimer, p, timerid);
}

/** タイマーすべて削除 */

void mWidgetTimerDeleteAll(mWidget *p)
{
	mAppTimerDeleteWidget(&MAPP_SYS->listTimer, p);
}

/** 現在のカーソル位置を指定ウィジェットの座標で取得
 *
 * @param p NULL でルート座標 */

void mWidgetGetCursorPos(mWidget *p,mPoint *pt)
{
	Window win;
	int x,y,rx,ry;
	unsigned int btt;

	XQueryPointer(XDISP,
		(p)? WINDOW_XID(p->toplevel): MAPP_SYS->root_window,
		&win, &win, &rx, &ry, &x, &y, &btt);

	if(!p)
	{
		pt->x = rx;
		pt->y = ry;
	}
	else
	{
		pt->x = x - p->absX;
		pt->y = y - p->absY;
	}
}

/** @} */
