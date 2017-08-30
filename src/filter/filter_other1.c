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
 * いろいろ1 (ピクセレート、輪郭、ほか)
 **************************************/

#include <math.h>

#include "mDef.h"
#include "mRandXorShift.h"

#include "defTileImage.h"
#include "TileImage.h"
#include "TileImageDrawInfo.h"

#include "FilterDrawInfo.h"
#include "filter_sub.h"



//=======================
// ピクセレート
//=======================


/** モザイク */

mBool FilterDraw_mozaic(FilterDrawInfo *info)
{
	int ix,iy,ixx,iyy,size;
	double weight,d[4];
	RGBAFix15 pix;
	TileImageSetPixelFunc setpix;

	FilterSub_getPixelFunc(&setpix);

	size = info->val_bar[0];
	weight = 1.0 / (size * size);

	//

	FilterSub_progBeginOneStep(info, 50, (info->box.h + size - 1) / size);

	for(iy = info->rc.y1; iy <= info->rc.y2; iy += size)
	{
		for(ix = info->rc.x1; ix <= info->rc.x2; ix += size)
		{
			//平均色

			d[0] = d[1] = d[2] = d[3] = 0;

			for(iyy = 0; iyy < size; iyy++)
			{
				for(ixx = 0; ixx < size; ixx++)
				{
					FilterSub_addAdvColor_at(info->imgsrc, ix + ixx, iy + iyy, d, info->clipping);
				}
			}

			FilterSub_getAdvColor(d, weight, &pix);

			//セット

			for(iyy = 0; iyy < size; iyy++)
			{
				if(iy + iyy > info->rc.y2) break;
			
				for(ixx = 0; ixx < size; ixx++)
				{
					if(ix + ixx > info->rc.x2) break;
				
					(setpix)(info->imgdst, ix + ixx, iy + iyy, &pix);
				}
			}
		}

		FilterSub_progIncSubStep(info);
	}

	return TRUE;
}

//=======================
// 水晶
//=======================


typedef struct
{
	int cx,cy,colnum;
	double dcol[4];
	RGBAFix15 pix;
}_crystal_dat;


/** イメージ位置からブロックのポインタ取得
 *
 * (xpos,ypos) のブロックの周囲から、(x,y) と中心の距離が一番近いものを選択 */

static _crystal_dat *_crystal_get_block(_crystal_dat *buf,
	int x,int y,int xpos,int ypos,int xnum,int ynum)
{
	_crystal_dat *p;
	int ix,iy,minx,miny,minlen,len;

	minlen = -1;

	for(iy = ypos - 1; iy <= ypos + 1; iy++)
	{
		if(iy < 0 || iy >= ynum) continue;

		for(ix = xpos - 1; ix <= xpos + 1; ix++)
		{
			if(ix < 0 || ix >= xnum) continue;

			p = buf + iy * xnum + ix;

			len = (p->cx - x) * (p->cx - x) + (p->cy - y) * (p->cy - y);

			if(minlen < 0 || len < minlen)
			{
				minlen = len;
				minx = ix;
				miny = iy;
			}
		}
	}

	return buf + miny * xnum + minx;
}

mBool FilterDraw_crystal(FilterDrawInfo *info)
{
	_crystal_dat *workbuf,*p;
	int size,xnum,ynum,ix,iy,x,y,xx,yy;
	TileImageSetPixelFunc setpix;

	FilterSub_getPixelFunc(&setpix);

	size = info->val_bar[0];

	xnum = (info->box.w + size - 1) / size;
	ynum = (info->box.h + size - 1) / size;

	//作業用データ確保

	workbuf = (_crystal_dat *)mMalloc(sizeof(_crystal_dat) * xnum * ynum, TRUE);
	if(!workbuf) return FALSE;

	//各ブロックの中心位置をランダムで決定

	p = workbuf;

	for(iy = 0, yy = info->box.y; iy < ynum; iy++, yy += size)
	{
		for(ix = 0, xx = info->box.x; ix < xnum; ix++, xx += size, p++)
		{
			x = xx + mRandXorShift_getIntRange(0, size - 1);
			y = yy + mRandXorShift_getIntRange(0, size - 1);

			if(x > info->rc.x2) x = info->rc.x2;
			if(y > info->rc.y2) y = info->rc.y2;

			p->cx = x;
			p->cy = y;
		}
	}

	//平均色を使う場合、各ブロック内の色を加算

	if(info->val_ckbtt[0])
	{
		for(iy = info->rc.y1; iy <= info->rc.y2; iy++)
		{
			yy = (iy - info->rc.y1) / size;
			
			for(ix = info->rc.x1; ix <= info->rc.x2; ix++)
			{
				xx = (ix - info->rc.x1) / size;

				p = _crystal_get_block(workbuf, ix, iy, xx, yy, xnum, ynum);

				FilterSub_addAdvColor_at(info->imgsrc, ix, iy, p->dcol, info->clipping);
				p->colnum++;
			}
		}
	}

	//各ブロックの色を決定

	p = workbuf;

	for(iy = 0; iy < ynum; iy++)
	{
		for(ix = 0; ix < xnum; ix++, p++)
		{
			if(info->val_ckbtt[0])
				FilterSub_getAdvColor(p->dcol, 1.0 / p->colnum, &p->pix);
			else
				TileImage_getPixel(info->imgsrc, p->cx, p->cy, &p->pix);
		}
	}

	//色をセット

	for(iy = info->rc.y1; iy <= info->rc.y2; iy++)
	{
		yy = (iy - info->rc.y1) / size;
		
		for(ix = info->rc.x1; ix <= info->rc.x2; ix++)
		{
			xx = (ix - info->rc.x1) / size;

			p = _crystal_get_block(workbuf, ix, iy, xx, yy, xnum, ynum);

			(setpix)(info->imgdst, ix, iy, &p->pix);
		}

		FilterSub_progIncSubStep(info);
	}

	mFree(workbuf);

	return TRUE;
}

/** ハーフトーン
 *
 * val_bar : [0] サイズ [1..3] 角度 */

mBool FilterDraw_halftone(FilterDrawInfo *info)
{
	int i,c,ix,iy,ixx,iyy;
	double len,len_div,len_half,rr,rrdiv,dcos[3],dsin[3],dx,dy,xx[3],yy[3],dx2,dy2,dyy;
	RGBAFix15 pix;
	TileImageSetPixelFunc setpix;

	FilterSub_getPixelFunc(&setpix);

	len = info->val_bar[0] * 0.1;

	len_div = 1.0 / len;
	len_half = len * 0.5;
	rrdiv = len * len / M_MATH_PI / 0x8000;

	//RGB 各角度の sin,cos 値

	for(i = 0; i < 3; i++)
	{
		c = info->val_bar[1 + i];

		//G,B を R と同じ角度に
		if(i && info->val_ckbtt[0]) c = info->val_bar[1];

		rr = -c / 180.0 * M_MATH_PI;
		dcos[i] = cos(rr);
		dsin[i] = sin(rr);
	}

	//

	for(iy = info->rc.y1; iy <= info->rc.y2; iy++)
	{
		//x 先頭の各位置
	
		dx = info->rc.x1;
		dy = iy;

		for(i = 0; i < 3; i++)
		{
			xx[i] = dx * dcos[i] - dy * dsin[i];
			yy[i] = dx * dsin[i] + dy * dcos[i];
		}

		//
	
		for(ix = info->rc.x1; ix <= info->rc.x2; ix++)
		{
			TileImage_getPixel(info->imgsrc, ix, iy, &pix);

			//透明部分は処理しない

			if(pix.a == 0)
			{
				for(i = 0; i < 3; i++)
				{
					xx[i] += dcos[i];
					yy[i] += dsin[i];
				}
				continue;
			}

			//グレイスケール化

			if(info->val_ckbtt[2])
				pix.r = pix.g = pix.b = RGB15_TO_GRAY(pix.r, pix.g, pix.b);

			//RGB

			for(i = 0; i < 3; i++)
			{
				//ボックスの中心からの距離

				dx = xx[i] - (floor(xx[i] * len_div) * len + len_half);
				dy = yy[i] - (floor(yy[i] * len_div) * len + len_half);

				//RGB 値から半径取得

				rr = sqrt(pix.c[i] * rrdiv);
				rr *= rr;

				//色取得

				if(pix.c[i] == 0x8000)
					c = 0x8000;
				else if(info->val_ckbtt[1])
				{
					//アンチエイリアス
					/* サブピクセルごとに計算しないと、
					 * 次のボックスにまたがっている部分が問題になる */

					c = 0;

					for(iyy = 0, dy2 = dy; iyy < 5; iyy++, dy2 += 0.2)
					{
						if(dy2 >= len_half) dy2 -= len;
						dyy = dy2 * dy2;

						for(ixx = 0, dx2 = dx; ixx < 5; ixx++, dx2 += 0.2)
						{
							if(dx2 >= len_half) dx2 -= len;

							if(dx2 * dx2 + dyy < rr) c += 0x8000;
						}
					}

					c /= 25;
				}
				else
				{
					//非アンチエイリアス

					if(dx * dx + dy * dy < rr)
						c = 0x8000;
					else
						c = 0;
				}

				pix.c[i] = c;

				//次の位置

				xx[i] += dcos[i];
				yy[i] += dsin[i];
			}

			//セット

			(setpix)(info->imgdst, ix, iy, &pix);
		}

		FilterSub_progIncSubStep(info);
	}

	return TRUE;
}


//=======================
// 輪郭
//=======================


/** シャープ */

mBool FilterDraw_sharp(FilterDrawInfo *info)
{
	Filter3x3Info dat;
	int i;
	double d;

	d = info->val_bar[0] * -0.01;

	for(i = 0; i < 9; i++)
		dat.mul[i] = d;

	dat.mul[4] = info->val_bar[0] * 0.08 + 1;
	dat.divmul = 1;
	dat.add = 0;
	dat.grayscale = FALSE;

	return FilterSub_draw3x3(info, &dat);
}

/** アンシャープマスク */

static void _unsharpmask_setpixel(int x,int y,RGBAFix15 *pixg,FilterDrawInfo *info)
{
	RGBAFix15 pix;
	int i,n,amount;

	TileImage_getPixel(info->imgsrc, x, y, &pix);

	if(pix.a == 0) return;

	amount = info->val_bar[1];

	for(i = 0; i < 3; i++)
	{
		n = pix.c[i] + (pix.c[i] - pixg->c[i]) * amount / 100;
		if(n < 0) n = 0;
		else if(n > 0x8000) n = 0x8000;

		pix.c[i] = n;
	}

	(g_tileimage_dinfo.funcDrawPixel)(info->imgdst, x, y, &pix);
}

mBool FilterDraw_unsharpmask(FilterDrawInfo *info)
{
	return FilterSub_gaussblur(info, info->imgsrc, info->val_bar[0],
		_unsharpmask_setpixel, NULL);
}

/** 輪郭抽出 (Sobel) */

mBool FilterDraw_sobel(FilterDrawInfo *info)
{
	TileImage *imgsrc;
	int ix,iy,ixx,iyy,n,i;
	double dx[3],dy[3],d;
	RGBAFix15 pix,pix2;
	TileImageSetPixelFunc setpix;
	char hmask[9] = {-1,-2,-1, 0,0,0, 1,2,1},
		vmask[9] = {-1,0,1, -2,0,2, -1,0,1};

	FilterSub_getPixelFunc(&setpix);

	imgsrc = info->imgsrc;

	for(iy = info->rc.y1; iy <= info->rc.y2; iy++)
	{
		for(ix = info->rc.x1; ix <= info->rc.x2; ix++)
		{
			TileImage_getPixel(imgsrc, ix, iy, &pix);
			if(pix.a == 0) continue;

			//3x3

			dx[0] = dx[1] = dx[2] = 0;
			dy[0] = dy[1] = dy[2] = 0;

			for(iyy = -1, n = 0; iyy <= 1; iyy++)
			{
				for(ixx = -1; ixx <= 1; ixx++, n++)
				{
					TileImage_getPixel_clip(imgsrc, ix + ixx, iy + iyy, &pix2);

					for(i = 0; i < 3; i++)
					{
						d = (double)pix2.c[i] / 0x8000;
					
						dx[i] += d * hmask[n];
						dy[i] += d * vmask[n];
					}
				}
			}

			//RGB

			for(i = 0; i < 3; i++)
			{
				n = (int)(sqrt(dx[i] * dx[i] + dy[i] * dy[i]) * 0x8000 + 0.5);
				if(n > 0x8000) n = 0x8000;

				pix.c[i] = n;
			}

			(setpix)(info->imgdst, ix, iy, &pix);
		}
	}
	
	return TRUE;
}

/** 輪郭抽出 (Laplacian) */

mBool FilterDraw_laplacian(FilterDrawInfo *info)
{
	Filter3x3Info dat;
	int i;

	for(i = 0; i < 9; i++)
		dat.mul[i] = -1;

	dat.mul[4] = 8;
	dat.divmul = 1;
	dat.add = 0x8000;
	dat.grayscale = FALSE;

	return FilterSub_draw3x3(info, &dat);
}

/** ハイパス */

static void _highpass_setpixel(int x,int y,RGBAFix15 *pixg,FilterDrawInfo *info)
{
	RGBAFix15 pix;
	int i,n;

	TileImage_getPixel(info->imgsrc, x, y, &pix);
	if(pix.a == 0) return;

	for(i = 0; i < 3; i++)
	{
		n = pix.c[i] - pixg->c[i] + 0x4000;
		if(n < 0) n = 0;
		else if(n > 0x8000) n = 0x8000;

		pix.c[i] = n;
	}

	(g_tileimage_dinfo.funcDrawPixel)(info->imgdst, x, y, &pix);
}

mBool FilterDraw_highpass(FilterDrawInfo *info)
{
	return FilterSub_gaussblur(info, info->imgsrc, info->val_bar[0], _highpass_setpixel, NULL);
}


//============================
// ほか
//============================


/** 線画抽出 */

static mBool _commfunc_lumtoalpha(FilterDrawInfo *info,int x,int y,RGBAFix15 *pix)
{
	if(pix->a == 0)
		return FALSE;
	else
	{
		pix->a = 0x8000 - RGB15_TO_GRAY(pix->r, pix->g, pix->b);
		pix->r = pix->g = pix->b = 0;

		return TRUE;
	}
}

mBool FilterDraw_lumtoAlpha(FilterDrawInfo *info)
{
	return FilterSub_drawCommon(info, _commfunc_lumtoalpha);
}

/** 1px ドット線の補正 */

mBool FilterDraw_dot_thinning(FilterDrawInfo *info)
{
	TileImage *img = info->imgdst;
	int ix,iy,jx,jy,n,pos,flag;
	RGBAFix15 pixtp,pix;
	uint8_t c[25];
	char erasex,erasey;

	pixtp.v64 = 0;

	FilterSub_progSetMax(info, 50);

	//------ phase1 (3x3 余分な点を消す)

	FilterSub_progBeginSubStep(info, 25, info->box.h);

	for(iy = info->rc.y1; iy <= info->rc.y2; iy++)
	{
		for(ix = info->rc.x1; ix <= info->rc.x2; ix++)
		{
			//透明なら処理なし

			if(TileImage_isPixelTransparent(img, ix, iy)) continue;

			//3x3 範囲 : 透明でないか

			for(jy = -1, pos = 0; jy <= 1; jy++)
				for(jx = -1; jx <= 1; jx++, pos++)
					c[pos] = TileImage_isPixelOpaque(img, ix + jx, iy + jy);

			//上下、左右が 0 と 1 の組み合わせでない場合

			if(!(c[3] ^ c[5]) || !(c[1] ^ c[7]))
				continue;

			//

			flag = FALSE;
			n = c[0] + c[2] + c[6] + c[8];

			if(n == 0)
				//斜めがすべて 0
				flag = TRUE;
			else if(n == 1)
			{
				/* 斜めのうちひとつだけ 1 で他が 0 の場合、
				 * 斜めの点の左右/上下どちらに点があるか */

				if(c[0])
					flag = c[1] ^ c[3];
				else if(c[2])
					flag = c[1] ^ c[5];
				else if(c[6])
					flag = c[3] ^ c[7];
				else
					flag = c[5] ^ c[7];
			}

			//消す

			if(flag)
				TileImage_setPixel_draw_direct(img, ix, iy, &pixtp);
		}

		FilterSub_progIncSubStep(info);
	}

	//------- phase2 (5x5 不自然な線の補正)

	FilterSub_progBeginSubStep(info, 25, info->box.h);

	for(iy = info->rc.y1; iy <= info->rc.y2; iy++)
	{
		for(ix = info->rc.x1; ix <= info->rc.x2; ix++)
		{
			//不透明なら処理なし

			if(TileImage_isPixelOpaque(img, ix, iy)) continue;

			//5x5 範囲

			for(jy = -2, pos = 0; jy <= 2; jy++)
				for(jx = -2; jx <= 2; jx++, pos++)
					c[pos] = TileImage_isPixelOpaque(img, ix + jx, iy + jy);

			//3x3 内に点が 4 つあるか

			n = 0;

			for(jy = 0, pos = 6; jy < 3; jy++, pos += 5 - 3)
				for(jx = 0; jx < 3; jx++, pos++)
					n += c[pos];

			if(n != 4) continue;

			//各判定

			flag = 0;

			if(c[6] + c[7] + c[13] + c[18] == 4)
			{
				if(c[1] + c[2] + c[3] + c[9] + c[14] + c[19] == 0)
				{
					erasex = 1;
					erasey = -1;
					flag = 1;
				}
			}
			else if(c[8] + c[13] + c[16] + c[17] == 4)
			{
				if(c[9] + c[14] + c[19] + c[21] + c[22] + c[23] == 0)
				{
					erasex = 1;
					erasey = 1;
					flag = 1;
				}
			}
			else if(c[7] + c[8] + c[11] + c[16] == 4)
			{
				if(c[1] + c[2] + c[3] + c[5] + c[10] + c[15]  == 0)
				{
					erasex = -1;
					erasey = -1;
					flag = 1;
				}
			}
			else if(c[6] + c[11] + c[17] + c[18] == 4)
			{
				if(c[5] + c[10] + c[15] + c[21] + c[22] + c[23] == 0)
				{
					erasex = -1;
					erasey = 1;
					flag = 1;
				}
			}

			//セット

			if(flag)
			{
				TileImage_getPixel(img, ix + erasex, iy, &pix);
				TileImage_setPixel_draw_direct(img, ix, iy, &pix);

				TileImage_setPixel_draw_direct(img, ix + erasex, iy, &pixtp);
				TileImage_setPixel_draw_direct(img, ix, iy + erasey, &pixtp);
			}
		}

		FilterSub_progIncSubStep(info);
	}

	return TRUE;
}

/** 縁取り */

mBool FilterDraw_hemming(FilterDrawInfo *info)
{
	TileImage *imgsrc,*imgdst,*imgref,*imgdraw;
	int i,ix,iy;
	mRect rc;
	RGBAFix15 pixdraw,pix;
	TileImageSetPixelFunc setpix;

	imgdraw = info->imgdst;

	FilterSub_getDrawColor_fromType(info, info->val_combo[0], &pixdraw);

	//判定元イメージ

	imgref = NULL;

	if(info->val_ckbtt[0])
		imgref = FilterSub_getCheckedLayerImage();

	if(!imgref) imgref = info->imgsrc;

	//準備

	g_tileimage_dinfo.funcColor = TileImage_colfunc_normal;

	setpix = (info->in_dialog)? TileImage_setPixel_new_colfunc: TileImage_setPixel_draw_direct;

	FilterSub_copyImage_forPreview(info);

	//

	i = info->val_bar[0] * 10;
	if(info->val_ckbtt[1]) i += 10;

	FilterSub_progSetMax(info, i);

	//------- 縁取り描画 (imgsrc を参照して、imgdraw と imgdst へ描画)

	imgsrc = imgdst = NULL;

	rc = info->rc;

	for(i = 0; i < info->val_bar[0]; i++)
	{
		//ソース用としてコピー

		TileImage_free(imgsrc);
	
		imgsrc = TileImage_newClone((i == 0)? imgref: imgdst);
		if(!imgsrc) goto ERR;

		//描画先作成

		TileImage_free(imgdst);

		imgdst = TileImage_newFromRect(imgsrc->coltype, &rc);
		if(!imgdst) goto ERR;

		//imgsrc を参照し、点を上下左右に合成

		FilterSub_progBeginSubStep(info, 10, info->box.h);

		for(iy = rc.y1; iy <= rc.y2; iy++)
		{
			for(ix = rc.x1; ix <= rc.x2; ix++)
			{
				TileImage_getPixel(imgsrc, ix, iy, &pix);

				if(pix.a)
				{
					pixdraw.a = pix.a;

					//imgdst

					TileImage_setPixel_new_colfunc(imgdst, ix - 1, iy, &pixdraw);
					TileImage_setPixel_new_colfunc(imgdst, ix + 1, iy, &pixdraw);
					TileImage_setPixel_new_colfunc(imgdst, ix, iy - 1, &pixdraw);
					TileImage_setPixel_new_colfunc(imgdst, ix, iy + 1, &pixdraw);

					//imgdraw

					(setpix)(imgdraw, ix - 1, iy, &pixdraw);
					(setpix)(imgdraw, ix + 1, iy, &pixdraw);
					(setpix)(imgdraw, ix, iy - 1, &pixdraw);
					(setpix)(imgdraw, ix, iy + 1, &pixdraw);
				}
			}

			FilterSub_progIncSubStep(info);
		}
	}

	TileImage_free(imgsrc);
	TileImage_free(imgdst);

	//------- 元画像を切り抜く
	/* info->imgsrc は info->imgdst から複製した状態なので、
	 * カレントレイヤが対象でも問題ない */

	if(info->val_ckbtt[1])
	{
		g_tileimage_dinfo.funcColor = TileImage_colfunc_erase;

		FilterSub_progBeginSubStep(info, 10, info->box.h);

		for(iy = rc.y1; iy <= rc.y2; iy++)
		{
			for(ix = rc.x1; ix <= rc.x2; ix++)
			{
				TileImage_getPixel(imgref, ix, iy, &pix);

				if(pix.a)
					(setpix)(imgdraw, ix, iy, &pix);
			}

			FilterSub_progIncSubStep(info);
		}
	}
	
	return TRUE;

ERR:
	TileImage_free(imgsrc);
	TileImage_free(imgdst);
	return FALSE;
}

/** 立体枠 */

mBool FilterDraw_3Dframe(FilterDrawInfo *info)
{
	TileImage *img = info->imgdst;
	int i,j,w,h,cnt,fw,smooth,vs,v,xx,yy;

	FilterSub_getImageSize(&w, &h);
	
	fw = info->val_bar[0];
	smooth = info->val_ckbtt[0];

	//横線

	vs = (smooth)? 17039: 10485;
	if(info->val_ckbtt[1]) vs = -vs;

	v = vs;

	for(i = 0, cnt = w; i < fw; i++, cnt -= 2)
	{
		if((i << 1) >= h) break;

		if(smooth) v = vs - vs * i / fw;

		//上

		xx = i;
		yy = i;

		for(j = 0; j < cnt; j++)
			TileImage_setPixel_draw_addsub(img, xx + j, yy, v);

		//下

		xx++;
		yy = h - 1 - i;

		for(j = 0; j < cnt - 2; j++)
			TileImage_setPixel_draw_addsub(img, xx + j, yy, -v);
	}

	//縦線

	vs = (smooth)? 13435: 8192;
	if(info->val_ckbtt[1]) vs = -vs;

	v = vs;

	for(i = 0, cnt = h - 1; i < fw; i++, cnt -= 2)
	{
		if((i << 1) >= w) break;

		if(smooth) v = vs - vs * i / fw;

		//上

		xx = i;
		yy = i + 1;

		for(j = 0; j < cnt; j++)
			TileImage_setPixel_draw_addsub(img, xx, yy + j, v);

		//下

		xx = w - 1 - i;

		for(j = 0; j < cnt; j++)
			TileImage_setPixel_draw_addsub(img, xx, yy + j, -v);
	}

	return TRUE;
}

/** シフト */

mBool FilterDraw_shift(FilterDrawInfo *info)
{
	int ix,iy,sx,sy,mx,my,w,h;
	RGBAFix15 pix;
	TileImageSetPixelFunc setpix;

	FilterSub_getPixelFunc(&setpix);

	mx = info->val_bar[0];
	my = info->val_bar[1];

	FilterSub_getImageSize(&w, &h);

	for(iy = 0; iy < h; iy++)
	{
		sy = iy - my;
		while(sy < 0) sy += h;
		while(sy >= h) sy -= h;
	
		for(ix = 0; ix < w; ix++)
		{
			sx = ix - mx;
			while(sx < 0) sx += w;
			while(sx >= w) sx -= w;

			TileImage_getPixel(info->imgsrc, sx, sy, &pix);

			(setpix)(info->imgdst, ix, iy, &pix);
		}

		FilterSub_progIncSubStep(info);
	}

	return TRUE;
}
