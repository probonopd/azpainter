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
 * mRect, mBox [矩形]
 *****************************************/

#include "mDef.h"


//================================
// mRect
//================================


/********************//**

@defgroup rect mRect
@brief 矩形構造体 (x1,y1,x2,y2)

基本的に、(x1 \> x2 || y1 \> y2) の場合は範囲なしとみなす。

@ingroup group_data
@{

@file mRectBox.h
@struct mRect

************************/


/** 範囲がない (x1 @> x2 || y1 @> y2) 状態か */

mBool mRectIsEmpty(mRect *rc)
{
	return (rc->x1 > rc->x2 || rc->y1 > rc->y2);
}

/** 空にセットする */

void mRectEmpty(mRect *rc)
{
	rc->x1 = rc->y1 = 0;
	rc->x2 = rc->y2 = -1;
}

/** 値セット (バイト値パック)
 *
 * @param val 上位バイトから順に x1,y1,x2,y2 */

void mRectSetByPack(mRect *rc,uint32_t val)
{
	rc->x1 = (uint8_t)(val >> 24);
	rc->y1 = (uint8_t)(val >> 16);
	rc->x2 = (uint8_t)(val >> 8);
	rc->y2 = (uint8_t)val;
}

/** 値セット */

void mRectSetBox_d(mRect *rc,int x,int y,int w,int h)
{
	rc->x1 = x, rc->y1 = y;
	rc->x2 = x + w - 1, rc->y2 = y + h - 1;
}

/** mBox から値セット */

void mRectSetByBox(mRect *rc,mBox *box)
{
	rc->x1 = box->x, rc->y1 = box->y;
	rc->x2 = box->x + box->w - 1;
	rc->y2 = box->y + box->h - 1;
}

/** mPoint １点からセット */

void mRectSetByPoint(mRect *rc,mPoint *pt)
{
	rc->x1 = rc->x2 = pt->x;
	rc->y1 = rc->y2 = pt->y;
}

/** mPoint の２点の最小値・最大値からセット */

void mRectSetByPoint_minmax(mRect *rc,mPoint *pt1,mPoint *pt2)
{
	if(pt1->x < pt2->x)
		rc->x1 = pt1->x, rc->x2 = pt2->x;
	else
		rc->x1 = pt2->x, rc->x2 = pt1->x;

	if(pt1->y < pt2->y)
		rc->y1 = pt1->y, rc->y2 = pt2->y;
	else
		rc->y1 = pt2->y, rc->y2 = pt1->y;
}

/** mRect を mBox に変換
 *
 * rc が範囲なしの場合、w,h は０になる */

void mRectToBox(mBox *box,mRect *rc)
{
	box->x = rc->x1;
	box->y = rc->y1;
	
	if(mRectIsEmpty(rc))
		box->w = box->h = 0;
	else
	{
		box->w = rc->x2 - rc->x1 + 1;
		box->h = rc->y2 - rc->y1 + 1;
	}
}

/** x1 @< x2, y1 @< y2 になるように入れ替え */

void mRectSwap(mRect *rc)
{
	int n;
	
	if(rc->x1 > rc->x2)
		n = rc->x1, rc->x1 = rc->x2, rc->x2 = n;
	
	if(rc->y1 > rc->y2)
		n = rc->y1, rc->y1 = rc->y2, rc->y2 = n;
}

/** x1 @< x2, y1 @< y2 になるように入れ替え (src → dst) */

void mRectSwapTo(mRect *dst,mRect *src)
{
	*dst = *src;
	mRectSwap(dst);
}

/** src と dst の範囲を結合 */

void mRectUnion(mRect *dst,mRect *src)
{
	if(mRectIsEmpty(src))
		return;
	else if(mRectIsEmpty(dst))
		*dst = *src;
	else
	{
		if(dst->x1 > src->x1) dst->x1 = src->x1;
		if(dst->y1 > src->y1) dst->y1 = src->y1;
		if(dst->x2 < src->x2) dst->x2 = src->x2;
		if(dst->y2 < src->y2) dst->y2 = src->y2;
	}
}

/** dst と mBox の範囲を結合 */

void mRectUnion_box(mRect *dst,mBox *box)
{
	mRect rc;

	rc.x1 = box->x, rc.y1 = box->y;
	rc.x2 = box->x + box->w - 1;
	rc.y2 = box->y + box->h - 1;

	mRectUnion(dst, &rc);
}

/** 指定範囲に収まるようにクリッピング
 * 
 * 範囲外の場合、x1 @> x2, y1 @> y2 になる。
 * 
 * @retval FALSE rc は範囲外
 * @retval TRUE  範囲がある */

mBool mRectClipBox_d(mRect *rc,int x,int y,int w,int h)
{
	int x2,y2;
	
	x2 = x + w - 1;
	y2 = y + h - 1;

	if(w < 1 || h < 1 || mRectIsEmpty(rc)
		|| rc->x2 < x || rc->y2 < y || rc->x1 > x2 || rc->y1 > y2)
	{
		rc->x1 = rc->y1 = 0;
		rc->x2 = rc->y2 = -1;
	
		return FALSE;
	}
	else
	{
		if(rc->x1 < x) rc->x1 = x;
		if(rc->y1 < y) rc->y1 = y;
		if(rc->x2 > x2) rc->x2 = x2;
		if(rc->y2 > y2) rc->y2 = y2;

		return TRUE;
	}
}

/** クリッピング (rc を clip の範囲内に) */

mBool mRectClipRect(mRect *rc,mRect *clip)
{
	if(mRectIsEmpty(rc) || mRectIsEmpty(clip))
		return FALSE;

	if(rc->x2 < clip->x1 || rc->y2 < clip->y1 || rc->x1 > clip->x2 || rc->y1 > clip->y2)
	{
		rc->x1 = rc->y1 = 0;
		rc->x2 = rc->y2 = -1;
		return FALSE;
	}
	else
	{
		if(rc->x1 < clip->x1) rc->x1 = clip->x1;
		if(rc->y1 < clip->y1) rc->y1 = clip->y1;
		if(rc->x2 > clip->x2) rc->x2 = clip->x2;
		if(rc->y2 > clip->y2) rc->y2 = clip->y2;
		
		return TRUE;
	}
}

/** 範囲に指定した位置を含めるようにする */

void mRectIncPoint(mRect *rc,int x,int y)
{
	if(mRectIsEmpty(rc))
	{
		rc->x1 = rc->x2 = x;
		rc->y1 = rc->y2 = y;
	}
	else
	{
		if(rc->x1 > x) rc->x1 = x;
		if(rc->y1 > y) rc->y1 = y;
		if(rc->x2 < x) rc->x2 = x;
		if(rc->y2 < y) rc->y2 = y;
	}
}

/** 範囲を拡張する */

void mRectDeflate(mRect *rc,int dx,int dy)
{
	rc->x1 -= dx, rc->y1 -= dy;
	rc->x2 += dx, rc->y2 += dy;
}

/** 位置の相対移動 */

void mRectRelMove(mRect *rc,int mx,int my)
{
	rc->x1 += mx;
	rc->x2 += mx;
	rc->y1 += my;
	rc->y2 += my;
}

/* @} */


//================================
// mBox
//================================

/*******************//**

@defgroup box mBox
@brief 矩形範囲構造体 (x,y,w,h)

@ingroup group_data
@{

@file mRectBox.h
@struct mBox

***********************/


/** ２点から範囲セット */

void mBoxSetByPoint(mBox *box,mPoint *pt1,mPoint *pt2)
{
	int x1,y1,x2,y2;

	if(pt1->x < pt2->x)
		x1 = pt1->x, x2 = pt2->x;
	else
		x1 = pt2->x, x2 = pt1->x;

	if(pt1->y < pt2->y)
		y1 = pt1->y, y2 = pt2->y;
	else
		y1 = pt2->y, y2 = pt1->y;

	box->x = x1, box->y = y1;
	box->w = x2 - x1 + 1;
	box->h = y2 - y1 + 1;
}

/** mRect からセット */

void mBoxSetByRect(mBox *box,mRect *rc)
{
	box->x = rc->x1, box->y = rc->y1;
	box->w = rc->x2 - rc->x1 + 1;
	box->h = rc->y2 - rc->y1 + 1;
}

/** mBox 同士を範囲結合 */

void mBoxUnion(mBox *dst,mBox *src)
{
	mRect rc1,rc2;

	mRectSetByBox(&rc1, dst);
	mRectSetByBox(&rc2, src);

	mRectUnion(&rc1, &rc2);

	dst->x = rc1.x1, dst->y = rc1.y1;
	dst->w = rc1.x2 - rc1.x1 + 1;
	dst->h = rc1.y2 - rc1.y1 + 1;
}

/** 現在の範囲を、縦横比を維持して指定サイズ内に拡大縮小する
 *
 * @param noscaleup  拡大はしない (拡大する代わりに余白をつける) */

void mBoxScaleKeepAspect(mBox *box,int boxw,int boxh,mBool noscaleup)
{
	int x,y,w,h,dw,dh;

	x = box->x, y = box->y;
	w = box->w, h = box->h;

	if(noscaleup && w <= boxw && h <= boxh)
	{
		//ボックスサイズより小さければ拡大せず余白を付ける

		x = (boxw - w) >> 1;
		y = (boxh - h) >> 1;
	}
	else
	{
		//縦横比維持で拡大縮小

		dw = (int)((double)boxh * w / h + 0.5);
		dh = (int)((double)boxw * h / w + 0.5);

		if(dw < boxw)
		{
			//縦長

			x = (boxw - dw) >> 1, y = 0;
			w = dw, h = boxh;
		}
		else
		{
			//横長

			x = 0, y = (boxh - dh) >> 1;
			w = boxw, h = dh;
		}
	}

	box->x = x, box->y = y;
	box->w = w, box->h = h;
}

/** すべての点を含む範囲をセット */

void mBoxSetByPoints(mBox *box,mPoint *pt,int num)
{
	int x1,y1,x2,y2,xx,yy;

	if(num <= 0) return;

	x1 = x2 = pt->x;
	y1 = y2 = pt->y;

	for(pt++; num > 1; num--, pt++)
	{
		xx = pt->x, yy = pt->y;

		if(xx < x1) x1 = xx;
		if(yy < y1) y1 = yy;
		if(xx > x2) x2 = xx;
		if(yy > y2) y2 = yy;
	}

	box->x = x1, box->y = y1;
	box->w = x2 - x1 + 1, box->h = y2 - y1 + 1;
}

/** 2つの範囲が重なっているか */

mBool mBoxIsCross(mBox *box1,mBox *box2)
{
	if(box1->w == 0 || box1->h == 0 || box2->w == 0 || box2->h == 0)
		return FALSE;

	return (box1->x < box2->x + box2->w
		&& box1->y < box2->y + box2->h
		&& box1->x + box1->w - 1 >= box2->x
		&& box1->y + box1->h - 1 >= box2->y);
}

/** 指定位置が範囲内にあるか */

mBool mBoxIsPointIn(mBox *box,int x,int y)
{
	return (box->w && box->h
		&& box->x <= x && x < box->x + box->w
		&& box->y <= y && y < box->y + box->h);
}

/** @} */
