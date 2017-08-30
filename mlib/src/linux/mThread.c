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
 * <Linux> スレッド
 *****************************************/

#include <pthread.h>

#include "mDef.h"
#include "mThread.h"



//*******************************
// mThread
//*******************************

/**
@defgroup thread mThread
@brief スレッド

@ingroup group_system
@{

@file mThread.h
@struct _mThread
*/


/** スレッド関数 */

static void *_thread_func(void *arg)
{
	mThread *p = (mThread *)arg;

	//スレッドID をセット

	mMutexLock(p->mutex);
	p->handle = (intptr_t)pthread_self();
	mMutexUnlock(p->mutex);

	//登録された関数へ

	(p->func)(p);

	return 0;
}


/** 削除 */

void mThreadDestroy(mThread *p)
{
	if(p)
	{
		mMutexDestroy(p->mutex);
	
		mFree(p);
	}
}

/** スレッド用データ作成
 *
 * @param size 構造体の全体サイズ */

mThread *mThreadNew(int size,void (*func)(mThread *),intptr_t param)
{
	mThread *p;

	if(size < sizeof(mThread)) size = sizeof(mThread);

	p = (mThread *)mMalloc(size, TRUE);
	if(!p) return NULL;

	p->func = func;
	p->param = param;
	p->mutex = mMutexNew();

	return p;
}

/** スレッド開始 */

mBool mThreadRun(mThread *p)
{
	pthread_t id;
	int loop = 1;

	if(!p || p->handle) return FALSE;

	//開始

	if(pthread_create(&id, NULL, _thread_func, p) != 0)
		return FALSE;

	//スレッド関数が開始するまで待つ

	while(loop)
	{
		mMutexLock(p->mutex);

		if(p->handle) loop = FALSE;

		mMutexUnlock(p->mutex);
	}

	return TRUE;
}

/** スレッドが終了するまで待つ */

mBool mThreadWait(mThread *p)
{
	if(!p || !p->handle) return TRUE;

	if(pthread_join(p->handle, NULL) != 0)
		return FALSE;
	else
	{
		p->handle = 0;
		return TRUE;
	}
}

/** @} */


//*******************************
// mMutex
//*******************************


#define _MUTEX(p)  ((pthread_mutex_t *)(p))


/**
@defgroup mutex mMutex
@brief ミューテクス (スレッド同期)

@ingroup group_system
@{

@file mThread.h
*/


/** 削除 */

void mMutexDestroy(mMutex p)
{
	if(p)
	{
		pthread_mutex_destroy(_MUTEX(p));

		mFree(p);
	}
}

/** 作成 */

mMutex mMutexNew()
{
	pthread_mutex_t *p;

	//確保

	p = (pthread_mutex_t *)mMalloc(sizeof(pthread_mutex_t), TRUE);
	if(!p) return NULL;

	//初期化

	pthread_mutex_init(p, NULL);

	return (mMutex)p;
}

/** ロック */

void mMutexLock(mMutex p)
{
	if(p)
		pthread_mutex_lock(_MUTEX(p));
}

/** ロック解除 */

void mMutexUnlock(mMutex p)
{
	if(p)
		pthread_mutex_unlock(_MUTEX(p));
}

/** @} */
