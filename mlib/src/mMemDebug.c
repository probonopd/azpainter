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

/****************************************************
 * [メモリデバッグ用関数]
 *
 * - マクロ MLIB_MEMDEBUG が定義されていると有効になる。
 * - malloc,realloc,free,strdup,strndup をデバッグ用関数に置き換える。
 * - アプリ終了時に stderr に結果が出力される。
 * - メモリリーク検出
 * - 二重解放防止
 * - スレッド対応 (pthread)
 *
 *****************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "mConfig.h"

#if defined(HAVE_PTHREAD_H) && !defined(MLIB_NO_THREAD)
#include <pthread.h>
#endif


//-------------------------------

typedef struct _memdebug_list
{
	struct _memdebug_list *prev,*next;
	void *ptr;
	const char *filename;
	int line;
}memdebug_list;

static char g_bFirst = 1;

static memdebug_list *g_list_top = NULL,
					*g_list_bottom = NULL;

//-------------------------------

#if defined(HAVE_PTHREAD_H) && !defined(MLIB_NO_THREAD)

static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

#define MUTEXLOCK	pthread_mutex_lock(&g_mutex)
#define MUTEXUNLOCK	pthread_mutex_unlock(&g_mutex)

#else

#define MUTEXLOCK
#define MUTEXUNLOCK

#endif

//-------------------------------


/** バッファポインタから検索 */

static memdebug_list *_mdebug_search(void *ptr)
{
	memdebug_list *pl;
	
	for(pl = g_list_bottom; pl && pl->ptr != ptr; pl = pl->prev);
	
	if(!pl)
		fprintf(stderr, "## memdebug - unfounded %p\n", ptr);
		
	return pl;
}

/** 終了時 */

static void _mdebug_exit(void)
{
	memdebug_list *pl,*next;
	
	fflush(stdout);

	if(!g_list_top)
		fputs("## memdebug ok\n", stderr);
	else
	{
		//メモリリーク
	
		fputs("## memdebug - memory leak!\n", stderr);
	
		for(pl = g_list_top; pl; pl = next)
		{
			next = pl->next;
		
			fprintf(stderr, "%p <%s> line %d\n", pl->ptr, pl->filename, pl->line);
			
			free(pl);
		}
	}
}


//=======================


/*************//**

@defgroup memdebug mMemDebug
@brief メモリデバッグ用関数

@ingroup group_main
@{

@file mMemDebug.h

******************/


/** free */

void mMemDebug_free(void *ptr)
{
	memdebug_list *pl;
	
	if(!ptr) return;
	
	MUTEXLOCK;
	
	pl = _mdebug_search(ptr);
	
	if(pl)
	{
		//リストからはずす
	
		if(pl->prev)
			(pl->prev)->next = pl->next;
		else
			g_list_top = pl->next;
		
		if(pl->next)
			(pl->next)->prev = pl->prev;
		else
			g_list_bottom = pl->prev;

		//解放

		free(pl);
		free(ptr);
	}

	MUTEXUNLOCK;
}

/** malloc */

void *mMemDebug_malloc(size_t size,const char *filename,int line)
{
	void *ptr;
	memdebug_list *pl;
	
	//初期化
	
	MUTEXLOCK;
	
	if(g_bFirst)
	{
		atexit(_mdebug_exit);
		g_bFirst = 0;
	}
	
	MUTEXUNLOCK;
	
	//確保
	
	ptr = malloc(size);
	if(!ptr) return NULL;
	
	//リスト
	
	pl = (memdebug_list *)malloc(sizeof(memdebug_list));
	if(!pl)
	{
		free(ptr);
		return NULL;
	}
	
	pl->ptr      = ptr;
	pl->filename = filename;
	pl->line     = line;
	
	//リストに追加
	
	MUTEXLOCK;
	
	pl->prev = g_list_bottom;
	
	if(!g_list_top)
		g_list_top = pl;
	else
		g_list_bottom->next = pl;
	
	g_list_bottom = pl;
	pl->next = NULL;
	
	MUTEXUNLOCK;
	
	return ptr;
}

/** realloc */

void *mMemDebug_realloc(void *ptr,size_t size,const char *filename,int line)
{
	memdebug_list *pl;
	void *pnew = NULL;
	
	if(!ptr)
		return mMemDebug_malloc(size, filename, line);
	else if(size == 0)
	{
		mMemDebug_free(ptr);
		return NULL;
	}
	
	//サイズ変更
	
	MUTEXLOCK;
	
	pl = _mdebug_search(ptr);
		
	if(pl)
	{
		pnew = realloc(ptr, size);
	
		if(pnew) pl->ptr = pnew;
	}
	
	MUTEXUNLOCK;
	
	return pnew;
}

/** strdup */

char *mMemDebug_strdup(const char *str,const char *filename,int line)
{
	char *pret;
	int len;

	if(!str)
		return NULL;
	else
	{
		len = strlen(str) + 1;
	
		pret = (char *)mMemDebug_malloc(len, filename, line);
		if(!pret) return NULL;
		
		memcpy(pret, str, len);
		
		return pret;
	}
}

/** strndup */

char *mMemDebug_strndup(const char *str,size_t len,const char *filename,int line)
{
	char *pret;
	int max;

	if(!str)
		return NULL;
	else
	{
		max = strlen(str);
		if(len > max) len = max;
	
		pret = (char *)mMemDebug_malloc(len + 1, filename, line);
		if(!pret) return NULL;
		
		memcpy(pret, str, len);
		pret[len] = 0;
		
		return pret;
	}
}

/*@}*/
