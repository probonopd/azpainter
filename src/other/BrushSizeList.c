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
 * ブラシサイズリストデータ
 *****************************************/

#include <stdlib.h>

#include "mDef.h"
#include "mMemAuto.h"
#include "mUtilStr.h"


//-----------------

typedef struct
{
	mMemAuto mem;
	int num;      //現在のデータ個数
}_brushsize;

static _brushsize g_dat;

//-----------------

#define _MAXNUM      500
#define _EXPAND_SIZE (16 * 2)
#define _SIZE_MIN    3
#define _SIZE_MAX    6000

//-----------------


/** 解放 */

void BrushSizeList_free()
{
	mMemAutoFree(&g_dat.mem);
}

/** 初期化 */

void BrushSizeList_init()
{
	mMemzero(&g_dat, sizeof(_brushsize));

	//バッファ確保

	mMemAutoAlloc(&g_dat.mem, 64, _EXPAND_SIZE);
}

/** 指定個数分を確保 (ファイルからの読み込み時) */

mBool BrushSizeList_alloc(int num)
{
	if(num > _MAXNUM) num = _MAXNUM;

	return mMemAutoAlloc(&g_dat.mem, num << 1, _EXPAND_SIZE);
}

/** 個数をセット (ファイルからの読み込み時) */

void BrushSizeList_setNum(int num)
{
	g_dat.num = num;
	g_dat.mem.curpos = num << 1;
}

/** 個数を取得 */

int BrushSizeList_getNum()
{
	return g_dat.num;
}

/** バッファポインタを取得 */

uint16_t *BrushSizeList_getBuf()
{
	return (uint16_t *)g_dat.mem.buf;
}

/** 指定位置のデータを取得 */

int BrushSizeList_getValue(int pos)
{
	return *((uint16_t *)g_dat.mem.buf + pos);
}

/** 文字列から追加
 *
 * '/' で区切って複数 */

void BrushSizeList_addFromText(const char *text)
{
	const char *top,*end;
	uint16_t *pbuf;
	int size,pos,i,n;

	for(top = text; mGetStrNextSplit(&top, &end, '/'); top = end)
	{
		//サイズ値

		size = (int)(atof(top) * 10 + 0.5);

		if(size < _SIZE_MIN)
			size = _SIZE_MIN;
		else if(size > _SIZE_MAX)
			size = _SIZE_MAX;

		//挿入位置
		/* 値の小さい順。同じ値の場合は追加しない。 */

		pbuf = (uint16_t *)g_dat.mem.buf;
		pos = g_dat.num;

		for(i = 0; i < g_dat.num; i++)
		{
			n = *(pbuf++);

			//同じ値は無視
			if(n == size) { pos = -1; break; }

			//挿入位置
			if(n > size) { pos = i; break; }
		}

		if(pos == -1) continue;

		if(g_dat.num >= _MAXNUM) return;

		//一つ分を確保

		if(!mMemAutoAppend(&g_dat.mem, &size, 2))
			return;

		//挿入位置を空けてセット

		pbuf = (uint16_t *)g_dat.mem.buf + g_dat.num;

		for(i = g_dat.num - pos; i > 0; i--, pbuf--)
			*pbuf = *(pbuf - 1);

		*pbuf = size;

		g_dat.num++;
	}
}

/** 指定位置のデータを削除 */

void BrushSizeList_deleteAtPos(int pos)
{
	uint16_t *p;
	int i;

	p = (uint16_t *)g_dat.mem.buf + pos;

	for(i = g_dat.num - pos - 1; i > 0; i--, p++)
		*p = p[1];

	g_dat.num--;

	mMemAutoBack(&g_dat.mem, 2);
}
