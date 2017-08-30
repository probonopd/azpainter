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

/**************************************
 * フィルタ処理
 *
 * アンチエイリアシング
 **************************************/
/*
 * TANE 氏のソースを参考にしています。
 *
 * http://www5.ocn.ne.jp/~tane/
 *
 * layman-0.62.tar.gz : lervise.c
 */

#include "mDef.h"

#include "TileImage.h"
#include "TileImageDrawInfo.h"

#include "FilterDrawInfo.h"
#include "filter_sub.h"


/** 色の比較 */

static mBool _comp_pix(RGBAFix15 *p1,RGBAFix15 *p2)
{
	return ((p1->a == 0 && p2->a == 0)
		|| (p1->a && p2->a
				&& (p1->r >> 9) == (p2->r >> 9)
				&& (p1->g >> 9) == (p2->g >> 9)
				&& (p1->b >> 9) == (p2->b >> 9)));
}

/** 色の比較 (位置) */

static mBool _comp_pixel(TileImage *img,int x1,int y1,int x2,int y2)
{
	RGBAFix15 pix1,pix2;

	TileImage_getPixel(img, x1, y1, &pix1);
	TileImage_getPixel(img, x2, y2, &pix2);

	return _comp_pix(&pix1, &pix2);
}

/** 点の描画 */

static void _setpixel(FilterDrawInfo *info,int sx,int sy,int dx,int dy,int opacity)
{
	RGBAFix15 pix,pix1,pix2;
	int i;
	double sa,da,na;

	if(opacity <= 0) return;

	TileImage_getPixel(info->imgsrc, sx, sy, &pix1);
	TileImage_getPixel(info->imgsrc, dx, dy, &pix2);

	if(_comp_pix(&pix1, &pix2)) return;

	//合成

	sa = (double)(pix1.a * opacity >> 15) / 0x8000;
	da = (double)pix2.a / 0x8000;
	na = sa + da - sa * da;

	pix.a = (int)(na * 0x8000 + 0.5);

	if(pix.a == 0)
		pix.v64 = 0;
	else
	{
		da = da * (1 - sa);
		na = 1.0 / na;

		for(i = 0; i < 3; i++)
			pix.c[i] = (int)((pix1.c[i] * sa + pix2.c[i] * da) * na + 0.5);
	}

	(g_tileimage_dinfo.funcDrawPixel)(info->imgdst, dx, dy, &pix);
}

/** 描画 */

static void _aa_draw(FilterDrawInfo *info,int x,int y,mBool right,mRect *rc)
{
	int x1,y1,x2,y2,i,a,cnt;

	//上

	x1 = x2 = x;
	if(right) x1++; else x2++;

	if(rc->y1 == 0)
		_setpixel(info, x1, y, x2, y, info->ntmp[0] >> 1);
	else
	{
		cnt = (rc->y1 + 1) >> 1;
		if(y - cnt + 1 < info->rc.y1) cnt = y - info->rc.y1 + 1;

		a = info->ntmp[0];
		if(cnt < 4) a -= 1966;

		for(i = 0; i <= cnt; i++)
			_setpixel(info, x1, y, x2, y - i, a - (a * i / cnt));
	}

	//下

	x1 = x2 = x;
	y1 = y2 = y + 1;
	if(right) x2++; else x1++;

	if(rc->y2 == 0)
		_setpixel(info, x1, y1, x2, y2, info->ntmp[0] >> 1);
	else
	{
		cnt = (rc->y2 + 1) >> 1;
		if(y + cnt > info->rc.y2) cnt = info->rc.y2 - y;

		a = info->ntmp[0];
		if(cnt < 4) a -= 1966;

		for(i = 0; i <= cnt; i++)
			_setpixel(info, x1, y1, x2, y2 + i, a - (a * i / cnt));
	}

	//左

	y1 = y2 = y;
	if(right) y1++; else y2++;

	if(rc->x1)
	{
		cnt = (rc->x1 + 1) >> 1;
		if(x - cnt + 1 < info->rc.x1) cnt = x - info->rc.x1 + 1;

		a = info->ntmp[0];
		if(cnt < 4) a -= 1966;

		for(i = 0; i <= cnt; i++)
			_setpixel(info, x, y1, x - i, y2, a - (a * i / cnt));
	}

	//右

	x1 = x2 = x + 1;
	y1 = y2 = y;
	if(right) y2++; else y1++;

	if(rc->x2)
	{
		cnt = (rc->x2 + 1) >> 1;
		if(x + cnt > info->rc.x2) cnt = info->rc.x2 - x;

		a = info->ntmp[0];
		if(cnt < 4) a -= 1966;

		for(i = 0; i <= cnt; i++)
			_setpixel(info, x1, y1, x2 + i, y2, a - (a * i / cnt));
	}
}

/** 判定 */

static void _aa_func(FilterDrawInfo *info,int x,int y,mBool right)
{
	TileImage *imgsrc = info->imgsrc;
	mRect rc;
	int i,x1,x2,y1,y2;
	RGBAFix15 pix,pix2,pix3;

	rc.x1 = rc.y1 = rc.x2 = rc.y2 = 0;

	//上に伸びているか

	x1 = x2 = x;
	if(right) x1++; else x2++;

	if(!_comp_pixel(imgsrc, x1, y, x2, y))
	{
		TileImage_getPixel(imgsrc, x1, y, &pix);

		for(i = y - 1; i >= info->rc.y1; i--)
		{
			TileImage_getPixel(imgsrc, x1, i, &pix2);
			TileImage_getPixel(imgsrc, x2, i, &pix3);

			if(_comp_pix(&pix, &pix2) && !_comp_pix(&pix, &pix3))
				rc.y1++;
			else
				break;
		}
	}

	//下に伸びているか

	x1 = x2 = x;
	y1 = y + 1;
	if(right) x2++; else x1++;

	if(!_comp_pixel(imgsrc, x1, y1, x2, y1))
	{
		TileImage_getPixel(imgsrc, x1, y1, &pix);

		for(i = y + 2; i <= info->rc.y2; i++)
		{
			TileImage_getPixel(imgsrc, x1, i, &pix2);
			TileImage_getPixel(imgsrc, x2, i, &pix3);

			if(_comp_pix(&pix, &pix2) && !_comp_pix(&pix, &pix3))
				rc.y2++;
			else
				break;
		}
	}

	//左に伸びているか

	y1 = y2 = y;
	if(right) y1++; else y2++;

	if(!_comp_pixel(imgsrc, x, y1, x, y2))
	{
		TileImage_getPixel(imgsrc, x, y1, &pix);

		for(i = x - 1; i >= info->rc.x1; i--)
		{
			TileImage_getPixel(imgsrc, i, y1, &pix2);
			TileImage_getPixel(imgsrc, i, y2, &pix3);

			if(_comp_pix(&pix, &pix2) && !_comp_pix(&pix, &pix3))
				rc.x1++;
			else
				break;
		}
	}

	//右に伸びているか

	x1 = x + 1;
	y1 = y2 = y;
	if(right) y2++; else y1++;

	if(!_comp_pixel(imgsrc, x1, y1, x1, y2))
	{
		TileImage_getPixel(imgsrc, x1, y1, &pix);

		for(i = x + 2; i <= info->rc.x2; i++)
		{
			TileImage_getPixel(imgsrc, i, y1, &pix2);
			TileImage_getPixel(imgsrc, i, y2, &pix3);

			if(_comp_pix(&pix, &pix2) && !_comp_pix(&pix, &pix3))
				rc.x2++;
			else
				break;
		}
	}

	_aa_draw(info, x, y, right, &rc);
}

/** アンチエイリアシング */

mBool FilterDraw_antialiasing(FilterDrawInfo *info)
{
	TileImage *imgsrc;
	int ix,iy,f;
	RGBAFix15 pix,pixx,pixy,pixxy;

	info->ntmp[0] = (info->val_bar[0] << 14) / 100;

	FilterSub_copyImage_forPreview(info);

	imgsrc = info->imgsrc;

	//[!] 右端、下端は処理しない

	FilterSub_progBeginOneStep(info, 50, info->box.h - 1);

	for(iy = info->rc.y1; iy < info->rc.y2; iy++)
	{
		for(ix = info->rc.x1; ix < info->rc.x2; ix++)
		{
			TileImage_getPixel(imgsrc, ix, iy, &pix);
			TileImage_getPixel(imgsrc, ix + 1, iy, &pixx);
			TileImage_getPixel(imgsrc, ix, iy + 1, &pixy);
			TileImage_getPixel(imgsrc, ix + 1, iy + 1, &pixxy);

			f = _comp_pix(&pix, &pixxy)
				| (_comp_pix(&pixx, &pixy) << 1)
				| (_comp_pix(&pix, &pixy) << 2);

			if(f == 7) continue;

			if(f & 1)
				_aa_func(info, ix, iy, FALSE);

			if(f & 2)
				_aa_func(info, ix, iy, TRUE);
		}

		FilterSub_progIncSubStep(info);
	}

	return TRUE;
}
