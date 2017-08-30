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
 * 減色 (メディアンカット)
 **************************************/

#include "mDef.h"
#include "mList.h"

#include "TileImage.h"

#include "FilterDrawInfo.h"
#include "filter_sub.h"


//-------------------

#define _RGB_SHIFT  2
#define _COLBITS    (8 - _RGB_SHIFT)
#define _COLNUM     (256 >> _RGB_SHIFT)
#define _GETINDEX(r,g,b)  (((r) << (_COLBITS + _COLBITS)) + ((g) << _COLBITS) + (b))

typedef struct
{
	mListItem i;
	
	uint8_t min[3],max[3],	//ブロック内の RGB の最小と最大
		avg[3],				//ブロック内の RGB の平均色
		maxside_ch;			//辺 (max - min) が一番長いチャンネル
	int maxside_diff;		//一番長い辺の長さ
}_block;

typedef struct
{
	mList list;
	uint32_t *hist;	//R,G,B のヒストグラム
		/* (0,0,0) - (_COLNUM-1, _COLNUM-1, _COLNUM-1) の各色の使用数 */

	uint8_t *palbuf,
		*palindex;
}_mediancut;

//-------------------



//=======================
// ブロック
//=======================


/** 初期化
 *
 * @return 範囲内に点があるか */

static mBool _block_init(_mediancut *p,_block *pi)
{
	int i,r,g,b,min[3],max[3];
	uint32_t *hist,cnt,pixnum = 0;
	double avg[3],d;

	for(i = 0; i < 3; i++)
	{
		min[i] = 256;
		max[i] = -1;
		avg[i] = 0;
	}

	/* p->min, p->max の範囲を対象に、
	 * ヒストグラムから各色が使われているかを判断し、
	 * 色が使われていれば最小、最大の判定を行う。 */

	hist = p->hist;

	for(r = pi->min[0]; r <= pi->max[0]; r++)
	{
		for(g = pi->min[1]; g <= pi->max[1]; g++)
		{
			for(b = pi->min[2]; b <= pi->max[2]; b++)
			{
				cnt = hist[_GETINDEX(r, g, b)];
				
				if(cnt)
				{
					//最小、最大
					
					if(r < min[0]) min[0] = r;
					if(r > max[0]) max[0] = r;

					if(g < min[1]) min[1] = g;
					if(g > max[1]) max[1] = g;

					if(b < min[2]) min[2] = b;
					if(b > max[2]) max[2] = b;

					//平均色用の総和

					d = cnt * (1.0 / _COLNUM);

					avg[0] += r * d;
					avg[1] += g * d;
					avg[2] += b * d;

					pixnum += cnt;
				}
			}
		}
	}

	if(pixnum == 0) return FALSE;

	//セット

	r = -1;
	d = (double)_COLNUM / pixnum;

	for(i = 0; i < 3; i++)
	{
		pi->min[i] = min[i];
		pi->max[i] = max[i];
		pi->avg[i] = (int)(avg[i] * d + 0.5);

		//max - min が一番長いチャンネル

		g = max[i] - min[i];
		if(g > r)
		{
			r = g;
			b = i;
		}
	}

	pi->maxside_ch = b;
	pi->maxside_diff = r;

	return TRUE;
}

/** 分割ブロック追加 (辺の長さが長い順に) */

static mBool _add_divide_block(_mediancut *p,_block *src,int ch,int min,int max)
{
	_block *pi,*ins;
	int i;

	//作成

	pi = (_block *)mMalloc(sizeof(_block), TRUE);
	if(!pi) return FALSE;

	for(i = 0; i < 3; i++)
	{
		if(i == ch)
		{
			pi->min[i] = min;
			pi->max[i] = max;
		}
		else
		{
			pi->min[i] = src->min[i];
			pi->max[i] = src->max[i];
		}
	}

	if(!_block_init(p, pi))
	{
		mFree(pi);
		return TRUE;
	}

	//挿入位置

	for(ins = (_block *)p->list.top; ins; ins = (_block *)ins->i.next)
	{
		if(pi->maxside_diff > ins->maxside_diff)
			break;
	}

	mListInsert(&p->list, M_LISTITEM(ins), M_LISTITEM(pi));

	return TRUE;
}


//=======================


/** ヒストグラム生成 */

static mBool _get_histogram(_mediancut *p,TileImage *img,mRect *rc)
{
	int ix,iy,f = 0,idx,r,g,b;
	RGBAFix15 pix;
	uint32_t *hist;

	hist = p->hist;

	for(iy = rc->y1; iy <= rc->y2; iy++)
	{
		for(ix = rc->x1; ix <= rc->x2; ix++)
		{
			TileImage_getPixel(img, ix, iy, &pix);
			
			if(pix.a)
			{
				r = RGBCONV_FIX15_TO_8(pix.r);
				g = RGBCONV_FIX15_TO_8(pix.g);
				b = RGBCONV_FIX15_TO_8(pix.b);
			
				idx = (r >> _RGB_SHIFT << (_COLBITS + _COLBITS))
					+ (g >> _RGB_SHIFT << _COLBITS)
					+ (b >> _RGB_SHIFT);

				hist[idx]++;
				f = 1;
			}
		}
	}

	return f;
}

/** パレット生成 */

static mBool _create_palette(_mediancut *p)
{
	_block *pi;
	uint8_t *pal,*index,palno;
	int i,n,r,g,b;

	//確保
	
	p->palbuf = (uint8_t *)mMalloc(4 * p->list.num, FALSE);
	p->palindex = (uint8_t *)mMalloc(_COLNUM * _COLNUM * _COLNUM, TRUE);
	
	if(!p->palbuf || !p->palindex) return FALSE;

	//

	pal = p->palbuf;
	index = p->palindex;
	palno = 0;

	for(pi = (_block *)p->list.top; pi; pi = (_block *)pi->i.next, palno++)
	{
		//パレット色

		for(i = 0; i < 3; i++)
		{
			n = pi->avg[i];

			if(n == _COLNUM - 1)
				n = 255;
			else
				n <<= _RGB_SHIFT;

			pal[i] = n;
		}

		pal += 4;

		//パレットのインデックステーブル

		for(r = pi->min[0]; r <= pi->max[0]; r++)
		{
			for(g = pi->min[1]; g <= pi->max[1]; g++)
			{
				for(b = pi->min[2]; b <= pi->max[2]; b++)
				{
					index[_GETINDEX(r, g, b)] = palno;
				}
			}
		}
	}

	//解放

	mListDeleteAll(&p->list);

	return TRUE;
}

/** メディアンカット メイン処理 */

static mBool _mediancut_main(_mediancut *p,int palnum)
{
	_block *pi,block;
	int i,ch,mid;

	//最初のブロックをセット

	pi = (_block *)mListAppendNew(&p->list, sizeof(_block), NULL);
	if(!pi) return FALSE;

	for(i = 0; i < 3; i++)
		pi->max[i] = _COLNUM - 1;

	_block_init(p, pi);

	//ブロック数が色数と同じになるまで繰り返す

	while(p->list.num < palnum)
	{
		//先頭のブロックを取り出す

		block = *((_block *)p->list.top);

		//分割不可能な場合、終了

		if(block.maxside_diff < 1) break;

		mListDelete(&p->list, p->list.top);

		//RGB のうち辺の長さが一番大きいものを対象にして、平均色で分割

		ch = block.maxside_ch;
		mid = block.avg[ch];

		if(!_add_divide_block(p, &block, ch, block.min[ch], mid)
			|| !_add_divide_block(p, &block, ch, mid, block.max[ch]))
			return FALSE;
	}

	//ヒストグラム解放

	mFree(p->hist);
	p->hist = NULL;

	//パレット生成

	return _create_palette(p);
}

/** 解放 */

static void _mediancut_free(_mediancut *p)
{
	mListDeleteAll(&p->list);
	mFree(p->hist);
	mFree(p->palbuf);
	mFree(p->palindex);
	mFree(p);
}

/** パレット割り当て */

static void _set_palette(_mediancut *p,FilterDrawInfo *info)
{
	RGBAFix15 pix;
	uint8_t *pal;
	int ix,iy,r,g,b;
	TileImageSetPixelFunc setpix;

	FilterSub_getPixelFunc(&setpix);

	for(iy = info->rc.y1; iy <= info->rc.y2; iy++)
	{
		for(ix = info->rc.x1; ix <= info->rc.x2; ix++)
		{
			TileImage_getPixel(info->imgsrc, ix, iy, &pix);

			if(pix.a)
			{
				r = RGBCONV_FIX15_TO_8(pix.r) >> _RGB_SHIFT;
				g = RGBCONV_FIX15_TO_8(pix.g) >> _RGB_SHIFT;
				b = RGBCONV_FIX15_TO_8(pix.b) >> _RGB_SHIFT;
			
				pal = p->palbuf + (p->palindex[_GETINDEX(r, g, b)] << 2);

				pix.r = RGBCONV_8_TO_FIX15(pal[0]);
				pix.g = RGBCONV_8_TO_FIX15(pal[1]);
				pix.b = RGBCONV_8_TO_FIX15(pal[2]);

				(setpix)(info->imgdst, ix, iy, &pix);
			}
		}

		FilterSub_progIncSubStep(info);
	}
}

/** 減色 */

mBool FilterDraw_mediancut(FilterDrawInfo *info)
{
	_mediancut *p;

	//確保

	p = (_mediancut *)mMalloc(sizeof(_mediancut), TRUE);
	if(!p) return FALSE;

	p->hist = (uint32_t *)mMalloc(_COLNUM * _COLNUM * _COLNUM * 4, TRUE);
	if(!p->hist)
	{
		mFree(p);
		return FALSE;
	}

	//ヒストグラム生成 (すべて透明なら処理なし)

	if(!_get_histogram(p, info->imgsrc, &info->rc))
	{
		_mediancut_free(p);
		return TRUE;
	}

	//メディアンカット

	if(!_mediancut_main(p, info->val_bar[0]))
	{
		_mediancut_free(p);
		return FALSE;
	}

	//パレット割り当て

	_set_palette(p, info);

	//

	_mediancut_free(p);

	return TRUE;
}
