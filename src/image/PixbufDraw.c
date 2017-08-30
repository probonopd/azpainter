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
 * mPixbuf への描画関数
 *****************************************/

#include <math.h>

#include "mDef.h"
#include "mPixbuf.h"
#include "mSysCol.h"
#include "mRectBox.h"

#include "defCanvasInfo.h"




//===========================
// sub
//===========================


/** クリッピング付き直線描画 (blend A=0-128) */

/* クリッピング範囲が異なっても、常に同じ位置に点が打たれるようにする。
 * でないと、キャンバス全体を更新した時と描画時に範囲更新した時とで
 * グリッドの線がずれる場合がある。 */

static void _drawline_blend_clip(mPixbuf *p,int x1,int y1,int x2,int y2,uint32_t col,
	mRect *rc)
{
	int dx,dy,tmp;
	int64_t f,inc;
	mRect rcb;

	//線からなる矩形が範囲外か

	if(x1 < x2)
		rcb.x1 = x1, rcb.x2 = x2;
	else
		rcb.x1 = x2, rcb.x2 = x1;

	if(y1 < y2)
		rcb.y1 = y1, rcb.y2 = y2;
	else
		rcb.y1 = y2, rcb.y2 = y1;

	if(rcb.x2 < rc->x1 || rcb.x1 > rc->x2 || rcb.y2 < rc->y1 || rcb.y1 > rc->y2)
		return;

	//

	if(x1 == x2 && y1 == y2)
	{
		//1px

		mPixbufSetPixel_blend128(p, x1, y1, col);
	}
	else if(y1 == y2)
	{
		//横線

		x1 = rcb.x1, x2 = rcb.x2;

		if(x1 < rc->x1) x1 = rc->x1;
		if(x2 > rc->x2) x2 = rc->x2;

		mPixbufLineH_blend(p, x1, y1, x2 - x1 + 1, col, (col >> 24) << 1); //A=0-256
	}
	else if(x1 == x2)
	{
		//縦線

		y1 = rcb.y1, y2 = rcb.y2;

		if(y1 < rc->y1) y1 = rc->y1;
		if(y2 > rc->y2) y2 = rc->y2;

		mPixbufLineV_blend(p, x1, y1, y2 - y1 + 1, col, (col >> 24) << 1);
	}
	else
	{
		//直線

		dx = rcb.x2 - rcb.x1;
		dy = rcb.y2 - rcb.y1;

		if(dx > dy)
		{
			//----- 横長

			//x1 -> x2 の方向にする

			if(x1 > x2)
			{
				x1 = rcb.x1, x2 = rcb.x2;
				tmp = y1, y1 = y2, y2 = tmp;
			}

			inc = ((int64_t)(y2 - y1) << 16) / dx;
			f = (int64_t)y1 << 16;

			//X クリッピング

			if(x1 < rc->x1)
			{
				f += inc * (rc->x1 - x1);
				x1 = rc->x1;
			}

			if(x2 > rc->x2) x2 = rc->x2;

			//

			tmp = (f + (1<<15)) >> 16;

			if(y1 < y2)
			{
				//----- Y が下方向

				//開始位置が範囲外

				if(tmp > rc->y2) return;

				//範囲内まで進める

				while(tmp < rc->y1)
				{
					x1++;
					f += inc;

					tmp = (f + (1<<15)) >> 16;
				}

				//描画

				for(; x1 <= x2; x1++, f += inc)
				{
					tmp = (f + (1<<15)) >> 16;
					if(tmp > rc->y2) break;

					mPixbufSetPixel_blend128(p, x1, tmp, col);
				}
			}
			else
			{
				//----- Y が上方向

				//開始位置が範囲外

				if(tmp < rc->y1) return;

				//範囲内まで進める

				while(tmp > rc->y2)
				{
					x1++;
					f += inc;

					tmp = (f + (1<<15)) >> 16;
				}

				//描画

				for(; x1 <= x2; x1++, f += inc)
				{
					tmp = (f + (1<<15)) >> 16;
					if(tmp < rc->y1) break;

					mPixbufSetPixel_blend128(p, x1, tmp, col);
				}
			}
		}
		else
		{
			//------ 縦長

			//y1 -> y2 の方向にする

			if(y1 > y2)
			{
				y1 = rcb.y1, y2 = rcb.y2;
				tmp = x1, x1 = x2, x2 = tmp;
			}

			inc = ((int64_t)(x2 - x1) << 16) / dy;
			f = (int64_t)x1 << 16;

			//Y クリッピング

			if(y1 < rc->y1)
			{
				f += inc * (rc->y1 - y1);
				y1 = rc->y1;
			}

			if(y2 > rc->y2) y2 = rc->y2;

			//

			tmp = (f + (1<<15)) >> 16;

			if(x1 < x2)
			{
				//----- X が右方向

				//開始位置が範囲外

				if(tmp > rc->x2) return;

				//範囲内まで進める

				while(tmp < rc->x1)
				{
					y1++;
					f += inc;

					tmp = (f + (1<<15)) >> 16;
				}

				//描画

				for(; y1 <= y2; y1++, f += inc)
				{
					tmp = (f + (1<<15)) >> 16;
					if(tmp > rc->x2) break;

					mPixbufSetPixel_blend128(p, tmp, y1, col);
				}
			}
			else
			{
				//----- X が左方向

				//開始位置が範囲外

				if(tmp < rc->x1) return;

				//範囲内まで進める

				while(tmp > rc->x2)
				{
					y1++;
					f += inc;

					tmp = (f + (1<<15)) >> 16;
				}

				//描画

				for(; y1 <= y2; y1++, f += inc)
				{
					tmp = (f + (1<<15)) >> 16;
					if(tmp < rc->x1) break;

					mPixbufSetPixel_blend128(p, tmp, y1, col);
				}
			}
		}
	}
}


//===========================
//
//===========================


/** 白黒破線の十字線描画
 *
 * キャンバスビューのルーペ時。
 *
 * @param x    左側の垂直線の位置
 * @param y    上側の水平線の位置
 * @param size ２つの線の間隔
 * @param box  描画範囲 */

void pixbufDraw_cross_dash(mPixbuf *pixbuf,int x,int y,int size,mBox *boxarea)
{
	mPixCol col[2];
	mBox box;
	uint8_t *pd;
	int i,n;

	if(!mPixbufGetClipBox_box(pixbuf, &box, boxarea))
		return;

	col[0] = 0;
	col[1] = MSYSCOL(WHITE);

	//水平線、上 (実際には y - 1 の位置)

	n = y - 1;

	if(box.y <= n && n < box.y + box.h)
	{
		pd = mPixbufGetBufPtFast(pixbuf, box.x, n);
		n = box.x;

		for(i = box.w; i; i--, pd += pixbuf->bpp, n++)
		{
			if(n < x || n >= x + size)
				(pixbuf->setbuf)(pd, col[(n >> 1) & 1]);
		}
	}

	//水平線、下

	n = y + size;

	if(box.y <= n && n < box.y + box.h)
	{
		pd = mPixbufGetBufPtFast(pixbuf, box.x, n);
		n = box.x;

		for(i = box.w; i; i--, pd += pixbuf->bpp, n++)
		{
			if(n < x || n >= x + size)
				(pixbuf->setbuf)(pd, col[(n >> 1) & 1]);
		}
	}

	//垂直線、左

	n = x - 1;

	if(box.x <= n && n < box.x + box.w)
	{
		pd = mPixbufGetBufPtFast(pixbuf, n, box.y);
		n = box.y;

		for(i = box.h; i; i--, pd += pixbuf->pitch_dir, n++)
		{
			if(n < y || n >= y + size)
				(pixbuf->setbuf)(pd, col[(n >> 1) & 1]);
		}
	}

	//垂直線、右

	n = x + size;

	if(box.x <= n && n < box.x + box.w)
	{
		pd = mPixbufGetBufPtFast(pixbuf, n, box.y);
		n = box.y;

		for(i = box.h; i; i--, pd += pixbuf->pitch_dir, n++)
		{
			if(n < y || n >= y + size)
				(pixbuf->setbuf)(pd, col[(n >> 1) & 1]);
		}
	}
}

/** 白黒破線の矩形枠描画 (フィルタプレビュー用) */

void pixbufDraw_dashBox_mono(mPixbuf *pixbuf,int x,int y,int w,int h)
{
	mPixCol col[2];
	int i,n,pos = 0;

	col[0] = 0;
	col[1] = MSYSCOL(WHITE);

	//上

	for(i = 0; i < w; i++, pos++)
		mPixbufSetPixel(pixbuf, x + i, y, col[(pos >> 1) & 1]);

	//右

	n = x + w - 1;

	for(i = 1; i < h; i++, pos++)
		mPixbufSetPixel(pixbuf, n, y + i, col[(pos >> 1) & 1]);

	//下

	n = y + h - 1;

	for(i = w - 2; i >= 0; i--, pos++)
		mPixbufSetPixel(pixbuf, x + i, n, col[(pos >> 1) & 1]);

	//左

	for(i = h - 2; i > 0; i--, pos++)
		mPixbufSetPixel(pixbuf, x, y + i, col[(pos >> 1) & 1]);
}

/** キャンバスにグリッド線描画
 *
 * @param boxdst 描画先の範囲
 * @param boximg boxdst の範囲に相当するイメージ座標での範囲
 * @param col  上位8bitに濃度 (0-128) */

void pixbufDrawGrid(mPixbuf *pixbuf,mBox *boxdst,mBox *boximg,
	int gridw,int gridh,uint32_t col,CanvasDrawInfo *info)
{
	mRect rcclip;
	int top,bottom,i;
	double inc[4],x1,y1,x2,y2,incx,incy;

	//描画先のクリッピング範囲 (rect)
	
	mRectSetByBox(&rcclip, boxdst);

	//イメージ座標 +1 時のキャンバス座標変化値

	CanvasDrawInfo_getImageIncParam(info, inc);

	//--------- 縦線

	top = boximg->x / gridw * gridw;
	bottom = (boximg->x + boximg->w - 1) / gridw * gridw;;
		
	CanvasDrawInfo_imageToarea(info, top, 0, &x1, &y1);
	CanvasDrawInfo_imageToarea(info, top, info->imgh, &x2, &y2);

	incx = inc[0] * gridw;
	incy = inc[1] * gridw;

	for(i = top; i <= bottom; i += gridw)
	{
		_drawline_blend_clip(pixbuf, x1, y1, x2, y2, col, &rcclip);

		x1 += incx, y1 += incy;
		x2 += incx, y2 += incy;
	}

	//--------- 横線

	top = boximg->y / gridh * gridh;
	bottom = (boximg->y + boximg->h - 1) / gridh * gridh;

	CanvasDrawInfo_imageToarea(info, 0, top, &x1, &y1);
	CanvasDrawInfo_imageToarea(info, info->imgw, top, &x2, &y2);

	incx = inc[2] * gridh;
	incy = inc[3] * gridh;

	for(i = top; i <= bottom; i += gridh)
	{
		_drawline_blend_clip(pixbuf, x1, y1, x2, y2, col, &rcclip);

		x1 += incx, y1 += incy;
		x2 += incx, y2 += incy;
	}
}

/** 選択範囲輪郭用に点描画 */

void pixbufDraw_setPixelSelectEdge(mPixbuf *pixbuf,int x,int y,mRect *rcclip)
{
	uint8_t pat[8] = { 0xf0,0x78,0x3c,0x1e,0x0f,0x87,0xc3,0xe1 };

	if(x >= rcclip->x1 && x <= rcclip->x2 && y >= rcclip->y1 && y <= rcclip->y2)
	{
		mPixbufSetPixel(pixbuf, x, y,
			(pat[y & 7] & (1 << (x & 7)))? 0: MSYSCOL(WHITE));
	}
}

/** 選択範囲輪郭用に線描画 */

void pixbufDraw_lineSelectEdge(mPixbuf *pixbuf,int x1,int y1,int x2,int y2,mRect *rcclip)
{
	int dx,dy,f,d,add = 1;

	if(x1 == x2 && y1 == y2)
	{
		pixbufDraw_setPixelSelectEdge(pixbuf, x1, y1, rcclip);
		return;
	}

	dx = (x1 < x2)? x2 - x1: x1 - x2;
	dy = (y1 < y2)? y2 - y1: y1 - y2;

	if(dx > dy)
	{
		f = y1 << 16;
		d = ((y2 - y1) << 16) / (x2 - x1);
		if(x1 > x2) add = -1, d = -d;

		while(x1 != x2)
		{
			pixbufDraw_setPixelSelectEdge(pixbuf, x1, (f + 0x8000) >> 16, rcclip);
			f  += d;
			x1 += add;
		}
	}
	else
	{
		f = x1 << 16;
		d = ((x2 - x1) << 16) / (y2 - y1);
		if(y1 > y2) add = -1, d = -d;

		while(y1 != y2)
		{
			pixbufDraw_setPixelSelectEdge(pixbuf, (f + 0x8000) >> 16, y1, rcclip);
			f  += d;
			y1 += add;
		}
	}
}

/** 矩形編集用の選択範囲の枠を描画 */

void pixbufDrawBoxEditFrame(mPixbuf *pixbuf,mBox *boximg,CanvasDrawInfo *info)
{
	mRect rcclip;
	mPoint pt[4];
	double x1,y1,x2,y2,dtmp;
	uint32_t col;
	int i,subx;

	//描画先のクリッピング範囲 (rect)
	
	mRectSetByBox(&rcclip, &info->boxdst);

	/* 範囲の外側 3px 分に線を引く */

	x1 = boximg->x, y1 = boximg->y;
	x2 = boximg->x + boximg->w, y2 = boximg->y + boximg->h;

	if(info->param->rd == 0)
	{
		//------ 回転なし

		CanvasDrawInfo_imageToarea_pt(info, x1, y1, pt);
		CanvasDrawInfo_imageToarea_pt(info, x2, y2, pt + 1);

		subx = (info->mirror)? 1: -1;

		//左上を -1px

		pt[0].x += subx;
		pt[0].y--;

		//描画

		for(i = 0; i < 3; i++)
		{
			col = (i == 1)? 0x80ffffff: 0x80000000;
		
			_drawline_blend_clip(pixbuf, pt[0].x, pt[0].y, pt[1].x, pt[0].y, col, &rcclip);
			_drawline_blend_clip(pixbuf, pt[1].x, pt[0].y, pt[1].x, pt[1].y, col, &rcclip);
			_drawline_blend_clip(pixbuf, pt[1].x, pt[1].y, pt[0].x, pt[1].y, col, &rcclip);
			_drawline_blend_clip(pixbuf, pt[0].x, pt[1].y, pt[0].x, pt[0].y, col, &rcclip);

			pt[0].x += subx, pt[0].y--;
			pt[1].x -= subx, pt[1].y++;
		}
	}
	else
	{
		//------- 回転あり

		dtmp = info->param->scalediv;

		//左上を -1px するため、イメージ座標でキャンバス 1px に相当する値を引く

		x1 -= dtmp;
		y1 -= dtmp;

		//描画

		for(i = 0; i < 3; i++)
		{
			//４隅の点をキャンバス座標に変換

			CanvasDrawInfo_imageToarea_pt(info, x1, y1, pt);
			CanvasDrawInfo_imageToarea_pt(info, x2, y1, pt + 1);
			CanvasDrawInfo_imageToarea_pt(info, x2, y2, pt + 2);
			CanvasDrawInfo_imageToarea_pt(info, x1, y2, pt + 3);

			//描画

			col = (i == 1)? 0x80ffffff: 0x80000000;
		
			_drawline_blend_clip(pixbuf, pt[0].x, pt[0].y, pt[1].x, pt[1].y, col, &rcclip);
			_drawline_blend_clip(pixbuf, pt[1].x, pt[1].y, pt[2].x, pt[2].y, col, &rcclip);
			_drawline_blend_clip(pixbuf, pt[2].x, pt[2].y, pt[3].x, pt[3].y, col, &rcclip);
			_drawline_blend_clip(pixbuf, pt[3].x, pt[3].y, pt[0].x, pt[0].y, col, &rcclip);

			//1px 拡大

			x1 -= dtmp;
			y1 -= dtmp;
			x2 += dtmp;
			y2 += dtmp;
		}
	}
}
