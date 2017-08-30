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
 * TileImage 拡大縮小
 *****************************************/

#include <string.h>
#include <math.h>

#include "mDef.h"
#include "mPopupProgress.h"

#include "defTileImage.h"
#include "TileImage.h"
#include "TileImage_coltype.h"

#include "defScalingType.h"


//-------------------

typedef struct
{
	TileImage *dstimg;
	int type,
		srcw,srch,
		dstw,dsth,
		range,
		tap_x,tap_y;
	double *weight_x,
		*weight_y,
		*dwork,
		scale_x,
		scale_y;
	uint16_t *pos_x,
		*pos_y;
	uint8_t *tilebuf;
	TileImageColFunc_setTileFromRGBA settilefunc;
}_scaling_dat;

//-------------------



//==========================
// sub
//==========================


/** 作業用バッファ解放 */

static void _free_buf(_scaling_dat *p)
{
	mFree(p->weight_x);
	mFree(p->weight_y);
	mFree(p->pos_x);
	mFree(p->pos_y);
	mFree(p->dwork);
	mFree(p->tilebuf);
}

/** 作業用バッファ確保 */

static mBool _alloc_buf(_scaling_dat *p)
{
	int n;

	p->weight_x = (double *)mMalloc(sizeof(double) * 64 * p->tap_x, FALSE);
	p->weight_y = (double *)mMalloc(sizeof(double) * 64 * p->tap_y, FALSE);

	p->pos_x = (uint16_t *)mMalloc(2 * 64 * p->tap_x, FALSE);
	p->pos_y = (uint16_t *)mMalloc(2 * 64 * p->tap_y, FALSE);

	n = p->tap_x;
	if(n < p->tap_y) n = p->tap_y;

	p->dwork = (double *)mMalloc(sizeof(double) * n, FALSE);

	p->tilebuf = (uint8_t *)mMalloc(64 * 64 * 8, FALSE);

	if(p->weight_x && p->weight_y && p->pos_x && p->pos_y && p->dwork && p->tilebuf)
		return TRUE;
	else
	{
		_free_buf(p);
		return FALSE;
	}
}

/** 重み計算 */

static double _get_weight(int type,double d)
{
	d = fabs(d);

	switch(type)
	{
		//mitchell
		case SCALING_TYPE_MITCHELL:
			if (d < 1.0)
				return 7.0 * d * d * d / 6.0 - 2.0 * d * d + 8.0 / 9.0;
			else if(d >= 2.0)
				return 0.0;
			else
				return 2.0 * d * d - 10.0 * d / 3.0 - 7.0 * d * d * d / 18.0 + 16.0 / 9.0;
			break;

		//Lagrange
		case SCALING_TYPE_LANGRANGE:
			if (d < 1.0)
				return 0.5 * (d - 2) * (d + 1) * (d - 1);
			else if(d >= 2.0)
				return 0.0;
			else
				return -(d - 3) * (d - 2) * (d - 1) / 6;
			break;
		
		//Lanczos2
		case SCALING_TYPE_LANCZOS2:
			if(d < 2.2204460492503131e-016)
				return 1.0;
			else if(d >= 2.0)
				return 0.0;
			else
			{
				d *= M_MATH_PI;
				return sin(d) * sin(d / 2.0) / (d * d / 2.0);
			}
			break;

		//Lanczos3
		default:
			if(d < 2.2204460492503131e-016)
				return 1.0;
			else if(d >= 3.0)
				return 0.0;
			else
			{
				d *= M_MATH_PI;
				return sin(d) * sin(d / 3.0) / (d * d / 3.0);
			}
			break;
	}

	return 0;
}

/** 縮小用パラメータセット */

static void _set_param_down(_scaling_dat *p,int top,int dstend,int tap,int srcl,
	double scale,double *pweight,uint16_t *ppos)
{
	double *workbuf,sum,scaler;
	int i,j,pos;

	workbuf = p->dwork;
	scaler = 1.0 / scale;

	for(i = 0; i < 64; i++)
	{
		//出力先の範囲外

		if(top + i >= dstend)
		{
			*ppos = (uint16_t)-1;
			break;
		}
	
		pos = floor((top + i - p->range + 0.5) * scaler + 0.5);
		sum = 0;

		for(j = 0; j < tap; j++, ppos++)
		{
			if(pos < 0)
				*ppos = 0;
			else if(pos >= srcl)
				*ppos = srcl - 1;
			else
				*ppos = pos;

			workbuf[j] = _get_weight(p->type, (pos + 0.5) * scale - (top + i + 0.5));

			sum += workbuf[j];
			pos++;
		}

		for(j = 0; j < tap; j++)
			*(pweight++) = workbuf[j] / sum;
	}
}

/** 拡大用パラメータセット */

static void _set_param_up(_scaling_dat *p,int top,int dstend,int tap,int srcl,
	double scale,double *pweight,uint16_t *ppos)
{
	double *workbuf,sum,scaler,dpos;
	int i,j,pos;

	workbuf = p->dwork;
	scaler = 1.0 / scale;

	for(i = 0; i < 64; i++)
	{
		//出力先の範囲外

		if(top + i >= dstend)
		{
			*ppos = (uint16_t)-1;
			break;
		}

		dpos = (top + i + 0.5) * scaler;
		pos = (int)floor(dpos - 2.5);
		dpos = pos + 0.5 - dpos;
		sum = 0;

		for(j = 0; j < tap; j++, ppos++)
		{
			if(pos < 0)
				*ppos = 0;
			else if(pos >= srcl)
				*ppos = srcl - 1;
			else
				*ppos = pos;

			workbuf[j] = _get_weight(p->type, dpos);

			sum += workbuf[j];

			dpos += 1.0;
			pos++;
		}

		for(j = 0; j < tap; j++)
			*(pweight++) = workbuf[j] / sum;
	}
}


//==========================
//
//==========================


/** タイルごとの処理 */

static mBool _proc_tile(_scaling_dat *dat,uint8_t **pptile,TileImage *src)
{
	int ixx,iyy,ix,iy,n,i,tapx,tapy,tpcnt = 0;
	double cc[3],aa,ww;
	RGBAFix15 pix,*pdst;
	uint16_t *ppos_x,*ppos_y;
	double *pweight_x,*pweight_y;

	tapx = dat->tap_x;
	tapy = dat->tap_y;

	memset(dat->tilebuf, 0, 64 * 64 * 8);

	//------ tilebuf に 64x64 の色をセット

	/* タイルの端が出力先の範囲外に来たら (位置が -1) 終了する。
	 * 例えば 1x1 に縮小する場合、タイルすべての点を処理するとかなり重くなるため。
	 * また、範囲外の部分を透明にするため。 */

	pdst = (RGBAFix15 *)dat->tilebuf;
	ppos_y = dat->pos_y;
	pweight_y = dat->weight_y;

	for(iyy = 0; iyy < 64; iyy++)
	{
		//範囲外
		
		if(*ppos_y == (uint16_t)-1)
		{
			tpcnt += (64 - iyy) << 6;
			break;
		}
	
		ppos_x = dat->pos_x;
		pweight_x = dat->weight_x;

		for(ixx = 0; ixx < 64; ixx++)
		{
			//範囲外

			if(*ppos_x == (uint16_t)-1)
			{
				pdst += 64 - ixx;
				tpcnt += 64 - ixx;
				break;
			}
		
			//
		
			cc[0] = cc[1] = cc[2] = aa = 0;

			for(iy = 0; iy < tapy; iy++)
			{
				for(ix = 0; ix < tapx; ix++)
				{
					TileImage_getPixel(src, ppos_x[ix], ppos_y[iy], &pix);

					ww = ((double)pix.a / 0x8000) * pweight_x[ix] * pweight_y[iy];

					cc[0] += pix.r * ww;
					cc[1] += pix.g * ww;
					cc[2] += pix.b * ww;
					aa += ww;
				}
			}

			//A

			n = (int)(aa * 0x8000 + 0.5);
			if(n < 0) n = 0;
			else if(n > 0x8000) n = 0x8000;

			//RGB

			if(n == 0)
			{
				pix.v64 = 0;
				tpcnt++;
			}
			else
			{
				pix.a = n;

				aa = 1.0 / aa;

				for(i = 0; i < 3; i++)
				{
					n = (int)(cc[i] * aa + 0.5);
					if(n < 0) n = 0;
					else if(n > 0x8000) n = 0x8000;

					pix.c[i] = n;
				}
			}

			*(pdst++) = pix;

			ppos_x += tapx;
			pweight_x += tapx;
		}

		ppos_y += tapy;
		pweight_y += tapy;
	}

	//------ すべて透明でなければ、タイル確保してセット

	if(tpcnt != 64 * 64)
	{
		*pptile = TileImage_allocTile(dat->dstimg, FALSE);
		if(!(*pptile)) return FALSE;
		
		(dat->settilefunc)(*pptile, dat->tilebuf, FALSE);
	}

	return TRUE;
}

/** 拡大縮小、他のタイプ */

static mBool _scaling_etc(TileImage *dst,TileImage *src,
	int type,mSize *size_src,mSize *size_dst,mPopupProgress *prog)
{
	_scaling_dat dat;
	uint8_t **pptile;
	int itx,ity,topx,topy;
	uint8_t param_x,param_y;
	void (*setparam[2])(_scaling_dat *,int,int,int,int,double,double *,uint16_t *) = {
		_set_param_down, _set_param_up
	};

	mMemzero(&dat, sizeof(_scaling_dat));

	//データセット

	dat.dstimg = dst;
	dat.settilefunc = g_tileimage_funcs[dst->coltype].setTileFromRGBA;

	dat.srcw = size_src->w;
	dat.srch = size_src->h;
	dat.dstw = size_dst->w;
	dat.dsth = size_dst->h;

	dat.type = type;
	dat.range = (type == SCALING_TYPE_LANCZOS3)? 3: 2;

	if(dat.srcw < dat.dstw)
		dat.tap_x = 6;
	else
		dat.tap_x = round((double)dat.range * 2 * dat.srcw / dat.dstw);

	if(dat.srch < dat.dsth)
		dat.tap_y = 6;
	else
		dat.tap_y = round((double)dat.range * 2 * dat.srch / dat.dsth);

	dat.scale_x = (double)dat.dstw / dat.srcw;
	dat.scale_y = (double)dat.dsth / dat.srch;

	//バッファ確保

	if(!_alloc_buf(&dat)) return FALSE;

	//--------- タイルごとに処理

	pptile = dst->ppbuf;

	param_x = (dat.srcw < dat.dstw);
	param_y = (dat.srch < dat.dsth);

	mPopupProgressThreadBeginSubStep(prog, 6, dst->tileh);

	for(ity = dst->tileh, topy = 0; ity; ity--, topy += 64)
	{
		(setparam[param_y])(&dat, topy, dat.dsth,
			dat.tap_y, dat.srch, dat.scale_y, dat.weight_y, dat.pos_y);

		for(itx = dst->tilew, topx = 0; itx; itx--, topx += 64, pptile++)
		{
			(setparam[param_x])(&dat, topx, dat.dstw,
				dat.tap_x, dat.srcw, dat.scale_x, dat.weight_x, dat.pos_x);

			if(!_proc_tile(&dat, pptile, src))
				return FALSE;
		}

		mPopupProgressThreadIncSubStep(prog);
	}

	_free_buf(&dat);

	return TRUE;
}

/** ニアレストネイバー */

static mBool _scaling_nearest(TileImage *p,TileImage *src,
	int sw,int sh,int dw,int dh,mPopupProgress *prog)
{
	int x,y,sy;
	RGBAFix15 pix;

	mPopupProgressThreadBeginSubStep(prog, 6, dh);

	for(y = 0; y < dh; y++)
	{
		sy = y * sh / dh;
	
		for(x = 0; x < dw; x++)
		{
			TileImage_getPixel(src, x * sw / dw, sy, &pix);

			if(pix.a)
				TileImage_setPixel_new(p, x, y, &pix);
		}

		mPopupProgressThreadIncSubStep(prog);
	}

	return TRUE;
}

/** src を拡大縮小して dst へ
 *
 * [!] dst は出力サイズ分のタイル配列確保済みであること。 */

mBool TileImage_scaling(TileImage *dst,TileImage *src,
	int type,mSize *size_src,mSize *size_dst,mPopupProgress *prog)
{
	if(type == SCALING_TYPE_NEAREST_NEIGHBOR)
		return _scaling_nearest(dst, src, size_src->w, size_src->h, size_dst->w, size_dst->h, prog);
	else
		return _scaling_etc(dst, src, type, size_src, size_dst, prog);
}
