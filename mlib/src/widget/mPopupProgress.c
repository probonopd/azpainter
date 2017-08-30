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
 * mPopupProgress
 *****************************************/

#include "mDef.h"

#include "mPopupProgress.h"

#include "mGui.h"
#include "mWidget.h"
#include "mWindow.h"
#include "mThread.h"



/**
@defgroup popupprogress mPopupProgress
@brief ポップアップ型のプログレスバー

- スレッドで処理をしている間に表示するポップアップ。
- 表示している間、すべてのウィンドウはマウスやキー押しなどのユーザーアクションを受け付けない。

@ingroup group_widget
@{

@file mPopupProgress.h
@def M_POPUPPROGRESS(p)
@struct mPopupProgress
@struct mPopupProgressData
*/


/** 作成
 *
 * @param w バーの幅
 * @param progress_style プログレスバーのスタイル */

mPopupProgress *mPopupProgressNew(int size,int w,uint32_t progress_style)
{
	mPopupProgress *p;
	mProgressBar *pg;

	if(size < sizeof(mPopupProgress)) size = sizeof(mPopupProgress);
	
	p = (mPopupProgress *)mWindowNew(size, NULL,
			MWINDOW_S_POPUP | MWINDOW_S_NO_IM);
	if(!p) return NULL;

	//プログレスバー

	pg = mProgressBarNew(0, M_WIDGET(p), progress_style);

	if(!pg)
	{
		mWidgetDestroy(M_WIDGET(p));
		return NULL;
	}

	p->pg.progress = pg;

	pg->wg.fLayout = MLF_EXPAND_W;

	//リサイズ

	mGuiCalcHintSize();

	mWidgetResize(M_WIDGET(p), w, p->wg.hintH);

	return p;
}

/** 実行
 *
 * mThread の param に mPopupProgress のポインタが入っている。\n
 * スレッド内で mPopupProgressThreadEnd() を呼ぶと終了する。\n
 * mPopupProgress はこの後で削除すること。 */

void mPopupProgressRun(mPopupProgress *p,int x,int y,void (*threadfunc)(mThread *))
{
	mThread *th;

	th = mThreadNew(0, threadfunc, (intptr_t)p);
	if(!th) return;

	mWidgetMove(M_WIDGET(p), x, y);
	mWidgetShow(M_WIDGET(p), 1);

	//実行

	mAppBlockUserAction(TRUE);

	mThreadRun(th);
	mAppRun();

	mAppBlockUserAction(FALSE);

	//終了

	mThreadWait(th);
	mThreadDestroy(th);
}

/** 終了 (スレッド側から) */

void mPopupProgressThreadEnd()
{
	mAppMutexLock();
	mAppQuit();
	mAppWakeUpEvent();
	mAppMutexUnlock();
}

/** 最大値をセット (スレッド側から) */

void mPopupProgressThreadSetMax(mPopupProgress *p,int max)
{
	if(p)
	{
		mAppMutexLock();

		(p->pg.progress)->pb.max = max;

		mAppMutexUnlock();
	}
}

/** プログレスバー位置をセット (スレッド側から) */

void mPopupProgressThreadSetPos(mPopupProgress *p,int pos)
{
	if(p)
	{
		mAppMutexLock();

		if(mProgressBarSetPos(p->pg.progress, pos))
			mAppWakeUpEvent();

		mAppMutexUnlock();
	}
}

/** プログレスバー位置を+1 (スレッド側から) */

void mPopupProgressThreadIncPos(mPopupProgress *p)
{
	if(p)
	{
		mAppMutexLock();
		mProgressBarIncPos(p->pg.progress);
		mAppWakeUpEvent();
		mAppMutexUnlock();
	}
}

/** 現在位置に加算 */

void mPopupProgressThreadAddPos(mPopupProgress *p,int add)
{
	if(p)
	{
		mAppMutexLock();

		if(mProgressBarAddPos(p->pg.progress, add))
			mAppWakeUpEvent();

		mAppMutexUnlock();
	}
}

/** サブステップ開始 */

void mPopupProgressThreadBeginSubStep(mPopupProgress *p,int stepnum,int max)
{
	if(p)
	{
		mAppMutexLock();
		mProgressBarBeginSubStep(p->pg.progress, stepnum, max);
		mAppMutexUnlock();
	}
}

/** サブステップ開始 (１ステップのみ) */

void mPopupProgressThreadBeginSubStep_onestep(mPopupProgress *p,int stepnum,int max)
{
	if(p)
	{
		mAppMutexLock();
		mProgressBarBeginSubStep_onestep(p->pg.progress, stepnum, max);
		mAppMutexUnlock();
	}
}

/** サブステップのカウントを +1 */

void mPopupProgressThreadIncSubStep(mPopupProgress *p)
{
	if(p)
	{
		mAppMutexLock();

		if(mProgressBarIncSubStep(p->pg.progress))
			mAppWakeUpEvent();

		mAppMutexUnlock();
	}
}

/** @} */
