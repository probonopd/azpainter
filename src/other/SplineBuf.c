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
 * SplineBuf
 *
 * スプラインのポイントバッファ
 *****************************************/

#include <math.h>

#include "mDef.h"
#include "mMemAuto.h"

#include "defDrawGlobal.h"
#include "draw_calc.h"


//----------------------

typedef struct
{
	int x,y;			//キャンバス位置
	uint8_t pressure;	//筆圧 (0 or 1)

	double dx,dy, //イメージ位置
		len,      //次の点との長さ
		a,b,c,
		wx,wy;
}_point;

//----------------------

typedef struct
{
	mMemAuto mem;	//ポイントバッファ (_point)
	int num,		//ポイント数
		cur;		//現在の取得ポイント番号
}SplineBuf;

static SplineBuf g_dat;

//----------------------

#define _GETLASTPT(p)   (((_point *)p->mem.buf) + p->num - 1)
#define _GETTOP(p)      ((_point *)p->mem.buf)

//----------------------



/** 初期化 */

void SplineBuf_init()
{
	mMemzero(&g_dat, sizeof(SplineBuf));
}

/** 解放 */

void SplineBuf_free()
{
	mMemAutoFree(&g_dat.mem);
}


/** 操作開始時 */

mBool SplineBuf_start()
{
	g_dat.num = 0;

	//確保
	
	return mMemAutoAlloc(&g_dat.mem, sizeof(_point) * 20, sizeof(_point) * 20);
}

/** ポイント追加
 *
 * @param pressure 0 or 1 */

mBool SplineBuf_addPoint(mPoint *pt,uint8_t pressure)
{
	SplineBuf *p = &g_dat;
	_point *buf,dat;

	if(p->num >= 100) return FALSE;

	//前回の位置と同じ

	if(p->num)
	{
		buf = _GETLASTPT(p);

		if(pt->x == buf->x && pt->y == buf->y)
			return FALSE;
	}

	//追加

	dat.x = pt->x;
	dat.y = pt->y;
	dat.pressure = pressure;

	if(!mMemAutoAppend(&p->mem, &dat, sizeof(_point)))
		return FALSE;

	p->num++;

	return TRUE;
}

/** 最後の点を削除して一つ戻す
 *
 * @param ptlast 削除後の最後の点が入る
 * @param 点が一つだった場合 TRUE */

mBool SplineBuf_deleteLastPoint(mPoint *ptlast)
{
	if(g_dat.num == 1)
		return TRUE;
	else
	{
		SplineBuf *p = &g_dat;
		_point *buf;
	
		mMemAutoBack(&p->mem, sizeof(_point));
		p->num--;

		//最後の点取得

		buf = _GETLASTPT(p);

		ptlast->x = buf->x;
		ptlast->y = buf->y;
		
		return FALSE;
	}
}

/** すべてのポイントの座標をスクロール */

void SplineBuf_scrollPoint(mPoint *pt)
{
	_point *buf;
	int i,xx,yy;

	buf = (_point *)g_dat.mem.buf;

	xx = -(pt->x);
	yy = -(pt->y);

	for(i = g_dat.num; i > 0; i--, buf++)
	{
		buf->x += xx;
		buf->y += yy;
	}
}

/** ポイント取得開始 (再描画用) */

void SplineBuf_beginGetPoint()
{
	g_dat.cur = 0;
}

/** ポイント取得
 *
 * @return [0]終了 [1]点あり [2]点あり(筆圧0) */

int SplineBuf_getPoint(mPoint *pt)
{
	SplineBuf *p = &g_dat;
	_point *buf;

	if(p->cur >= p->num) return 0;

	buf = _GETTOP(p) + p->cur;

	pt->x = buf->x;
	pt->y = buf->y;

	p->cur++;

	return (buf->pressure == 0)? 2: 1;
}


/** 描画用、初期化 */

mBool SplineBuf_initDraw()
{
	SplineBuf *p = &g_dat;
	_point *topbuf,*endbuf,*buf;
	int i;
	double a,b,aa,bb;

	if(p->num < 2) return FALSE;

	topbuf = _GETTOP(p);
	endbuf = topbuf + p->num - 1;

	//キャンバス座標 -> イメージ座標に変換

	for(i = p->num, buf = topbuf; i; i--, buf++)
		drawCalc_areaToimage_double(APP_DRAW, &buf->dx, &buf->dy, buf->x, buf->y);

	//長さ

	for(i = p->num - 1, buf = topbuf; i; i--, buf++)
	{
		a = buf->dx - buf[1].dx;
		b = buf->dy - buf[1].dy;

		buf->len = sqrt(a * a + b * b);
	}

	//パラメータ

	topbuf->a = 0, topbuf->b = 1, topbuf->c = 0.5;
	topbuf->wx = (3 / (2 * topbuf->len)) * (topbuf[1].dx - topbuf->dx);
	topbuf->wy = (3 / (2 * topbuf->len)) * (topbuf[1].dy - topbuf->dy);

	endbuf->a = 1, endbuf->b = 2, endbuf->c = 0;
	endbuf->wx = (3 / endbuf[-1].len) * (endbuf->dx - endbuf[-1].dx);
	endbuf->wy = (3 / endbuf[-1].len) * (endbuf->dy - endbuf[-1].dy);

	for(i = p->num - 2, buf = topbuf + 1; i; i--, buf++)
	{
		a = buf[-1].len;
		b = buf->len;

		buf->a = b;
		buf->b = 2 * (a + b);
		buf->c = a;

		aa = 3 * a * a;
		bb = 3 * b * b;

		buf->wx = (aa * (buf[1].dx - buf->dx) + bb * (buf->dx - buf[-1].dx)) / (a * b);
		buf->wy = (aa * (buf[1].dy - buf->dy) + bb * (buf->dy - buf[-1].dy)) / (a * b);
	}

	for(i = p->num - 1, buf = topbuf + 1; i; i--, buf++)
	{
		a = buf[-1].b / buf->a;

		buf->b = buf->b * a - buf[-1].c;
		buf->c *= a;

		buf->wx = buf->wx * a - buf[-1].wx;
		buf->wy = buf->wy * a - buf[-1].wy;

		b = 1.0 / buf->b;

		buf->c  *= b;
		buf->wx *= b;
		buf->wy *= b;
		buf->b = 1;
	}

	for(i = p->num - 1, buf = endbuf - 1; i; i--, buf--)
	{
		buf->wx -= buf->c * buf[1].wx;
		buf->wy -= buf->c * buf[1].wy;
	}

	//

	p->cur = 0;
	
	return TRUE;
}

/** 次の描画用パラメータ取得
 *
 * @return FALSE で終了 */

mBool SplineBuf_getNextDraw(mDoublePoint *pt,uint8_t *pressure)
{
	SplineBuf *p = &g_dat;
	_point *buf;
	double xx,yy,a,aa,aaa;

	if(p->cur >= p->num - 1) return FALSE;

	buf = (_point *)p->mem.buf + p->cur;

	xx = buf[1].dx - buf->dx;
	yy = buf[1].dy - buf->dy;

	a = buf->len;
	aa = a * a;
	aaa = -2 / (a * aa);

	//

	pt->x = buf->dx;
	pt->y = buf->dy;

	pt[1].x = buf->wx;
	pt[1].y = buf->wy;

	pt[2].x = xx * 3 / aa - (buf[1].wx + 2 * buf->wx) / a;
	pt[2].y = yy * 3 / aa - (buf[1].wy + 2 * buf->wy) / a;

	pt[3].x = xx * aaa + (buf[1].wx + buf->wx) / aa;
	pt[3].y = yy * aaa + (buf[1].wy + buf->wy) / aa;

	pt[4].x = buf->len;

	pressure[0] = buf->pressure;
	pressure[1] = buf[1].pressure;

	p->cur++;
	
	return TRUE;
}
