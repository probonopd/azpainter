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
 * DrawPointBuf
 *
 * 自由線描画用のポイントデータ
 *****************************************/
/*
 * - リング状に 32 個のポイントデータ。
 * - 手ブレ補正も行う。
 */

#include <math.h>

#include "mDef.h"

#include "DrawPointBuf.h"


//---------------------

typedef struct
{
	DrawPointBuf_pt pt[32];	//ポイントデータ
	int num,			//現在のポイント数
		sm_type,		//補正タイプ
		sm_radius,		//補正強さ (半径)
		sm_ptnum,		//補正時に参照するポイントの数
		finish_num;		//描画終了時に処理する数
}DrawPointBuf;

static DrawPointBuf g_ptbuf;

//---------------------


//============================
// sub
//============================


/** ポイント追加 */

static DrawPointBuf_pt *_add_point()
{
	DrawPointBuf_pt *pt;

	pt = g_ptbuf.pt + (g_ptbuf.num & 31);

	g_ptbuf.num++;

	return pt;
}

/** 現在位置からの相対位置でポイント取得 */

static DrawPointBuf_pt *_get_pt(int pos)
{
	return g_ptbuf.pt + ((g_ptbuf.num + pos) & 31);
}

/** 最後の点を取得 */

static void _get_last_point(DrawPointBuf_pt *dst)
{
	*dst = g_ptbuf.pt[(g_ptbuf.num - 1) & 31];
}

/** 最後の点を指定数追加 */

static void _add_point_last(int cnt)
{
	DrawPointBuf_pt pt,*p;

	_get_last_point(&pt);

	for(; cnt > 0; cnt--)
	{
		p = _add_point();
		*p = pt;
	}
}

/** 補正後の位置取得 */

static mBool _get_drawpoint(DrawPointBuf_pt *dst,int radius)
{
	int i,pos;
	DrawPointBuf_pt *pt;
	double d,w,wsum,x,y;

	pos = -g_ptbuf.sm_ptnum;

	x = y = wsum = 0;

	switch(g_ptbuf.sm_type)
	{
		//ガウス重み
		case DRAWPOINTBUF_TYPE_GAUSS:
			d = (radius + 1) * 0.5;
			d = 1.0 / (2 * d * d);

			for(i = -radius; i <= radius; i++)
			{
				pt = _get_pt(pos++);

				w = exp(-(i * i) * d);

				x += pt->x * w;
				y += pt->y * w;

				wsum += w;
			}
			break;

		//線形
		case DRAWPOINTBUF_TYPE_LINEAR:
			d = 1.0 / radius * 0.9;
			
			for(i = -radius; i <= radius; i++)
			{
				pt = _get_pt(pos++);

				w = 1.0 - (i < 0? -i: i) * d;

				x += pt->x * w;
				y += pt->y * w;

				wsum += w;
			}
			break;

		//平均
		default:
			for(i = -radius; i <= radius; i++)
			{
				pt = _get_pt(pos++);

				x += pt->x;
				y += pt->y;

				wsum += 1.0;
			}
			break;
	}

	dst->x = x / wsum;
	dst->y = y / wsum;
	dst->pressure = _get_pt(-radius - 1)->pressure;

	return TRUE;
}


//============================
// main
//============================


/** 手ぶれ補正のセット
 *
 * [!] ポイントをセットする前に実行すること。 */

void DrawPointBuf_setSmoothing(int type,int strength)
{
	//強さ 0 で補正なし

	if(strength == 0)
		type = DRAWPOINTBUF_TYPE_NONE;
	else if(type == DRAWPOINTBUF_TYPE_NONE)
		strength = 0;

	g_ptbuf.sm_type    = type;
	g_ptbuf.sm_radius  = strength;
	g_ptbuf.sm_ptnum   = strength * 2 + 1;
	g_ptbuf.finish_num = strength;
}

/** 最初のポイントをセット */

void DrawPointBuf_setFirstPoint(double x,double y,double pressure)
{
	g_ptbuf.pt[0].x = x;
	g_ptbuf.pt[0].y = y;
	g_ptbuf.pt[0].pressure = pressure;
	g_ptbuf.num = 1;

	if(g_ptbuf.sm_type)
		_add_point_last(g_ptbuf.sm_radius - 1);
}

/** ポイントを追加 */

void DrawPointBuf_addPoint(double x,double y,double pressure)
{
	DrawPointBuf_pt *pt = _add_point();

	pt->x = x;
	pt->y = y;
	pt->pressure = pressure;
}

/** 描画位置を取得
 *
 * @return FALSE で点が足りない */

mBool DrawPointBuf_getPoint(DrawPointBuf_pt *dst)
{
	if(g_ptbuf.sm_type == DRAWPOINTBUF_TYPE_NONE)
	{
		//----- 補正無し => 最後の点をそのまま返す
		
		_get_last_point(dst);
		return TRUE;
	}
	else
	{
		//----- 補正あり
	
		if(g_ptbuf.num == g_ptbuf.sm_radius)
		{
			//最初の点はそのまま返す

			*dst = g_ptbuf.pt[0];
			return TRUE;
		}
		else if(g_ptbuf.num < g_ptbuf.sm_ptnum)
			//点が足りない
			return FALSE;
		else
			return _get_drawpoint(dst, g_ptbuf.sm_radius);
	}
}

/** 描画終了時の描画位置を取得
 *
 * FALSE が返るまで複数回行う。
 *
 * @return FALSE で描画する点がない */

mBool DrawPointBuf_getFinishPoint(DrawPointBuf_pt *dst)
{
	if(g_ptbuf.finish_num == 0)
		//終了
		return FALSE;
	else if(g_ptbuf.finish_num == 1)
		//最後の点はそのまま返す
		_get_last_point(dst);
	else
	{
		//最小個数に達していない場合は最後の点を追加

		if(g_ptbuf.num < g_ptbuf.sm_ptnum)
			_add_point_last(g_ptbuf.sm_ptnum - g_ptbuf.num);

		//最後の点を追加して取得

		_add_point_last(1);

		DrawPointBuf_getPoint(dst);
	}

	g_ptbuf.finish_num--;

	return TRUE;
}

/** 描画位置を mPoint で取得 */

mBool DrawPointBuf_getPoint_int(mPoint *dst)
{
	DrawPointBuf_pt pt;

	if(!DrawPointBuf_getPoint(&pt))
		return FALSE;
	else
	{
		dst->x = floor(pt.x);
		dst->y = floor(pt.y);
		return TRUE;
	}
}

/** 描画終了時の描画位置を取得 (mPoint) */

mBool DrawPointBuf_getFinishPoint_int(mPoint *dst)
{
	DrawPointBuf_pt pt;

	if(!DrawPointBuf_getFinishPoint(&pt))
		return FALSE;
	else
	{
		dst->x = floor(pt.x);
		dst->y = floor(pt.y);
		return TRUE;
	}
}
