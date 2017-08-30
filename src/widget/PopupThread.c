/*$
 Copyright (C) 2013-2017 Azel.

 This file is part of AzPainter.

 AzPainter is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 AzPainter is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
$*/

/*****************************************
 * PopupThread
 *
 * スレッド処理中にポップアップのプログレスバーを表示する
 *****************************************/
/*
 * - スレッド中、
 *  ステータスバーのヘルプ部分か
 *  メインウィンドウ左下 (ステータスバー非表示時) に
 *  プログレスバーを表示する。
 */

#include "mDef.h"
#include "mPopupProgress.h"
#include "mWidget.h"

#include "MainWindow.h"


//-------------------------

typedef struct _PopupThread
{
	mWidget wg;
	mContainerData ct;
	mWindowData win;
	mPopupProgressData pg;

	void *databuf;
	int (*func)(mPopupProgress *,void *);
	int ret;
}PopupThread;

//-------------------------


/** スレッド関数 */

static void _thread_func(mThread *th)
{
	PopupThread *p = (PopupThread *)th->param;

	p->ret = (p->func)(M_POPUPPROGRESS(p), p->databuf);

	mPopupProgressThreadEnd();
}


/** スレッド実行
 *
 * @param data  スレッド関数に渡すデータバッファ
 * @param func  スレッド関数
 * @return スレッド関数の戻り値。-1 でエラー */

int PopupThread_run(void *data,int (*func)(mPopupProgress *,void *))
{
	PopupThread *p;
	mPoint pt;
	int ret;

	p = (PopupThread *)mPopupProgressNew(sizeof(PopupThread), 130, MPROGRESSBAR_S_FRAME);
	if(!p) return -1;

	p->databuf = data;
	p->func = func;

	//スレッド実行

	MainWindow_getProgressBarPos(&pt);

	pt.y -= M_WIDGET(p)->hintH;

	mPopupProgressRun(M_POPUPPROGRESS(p), pt.x, pt.y, _thread_func);

	//終了

	ret = p->ret;

	mWidgetDestroy(M_WIDGET(p));

	return ret;
}
