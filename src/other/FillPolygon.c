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
 * FillPolygon
 *
 * 多角形塗りつぶし処理
 *****************************************/

#include <math.h>

#include "mDef.h"
#include "mMemAuto.h"


//--------------------

//交点データ

typedef struct
{
	int x;
	char dir;
}EdgeDat;

//処理用データ

typedef struct _FillPolygon
{
	mMemAuto mempt;		//ポイントバッファ
	mMemAuto memedge;	//交点バッファ
	uint16_t *aabuf;	//アンチエイリアス用バッファ
	
	int ptnum,
		edgenum,
		xmin,xmax,ymin,ymax,
		width,
		edgeCurPos,
		edgeCurParam;
}FillPolygon;


#define _EXPAND_PTNUM  1
#define _SUBPIXEL_Y    8
#define _GETPTBUF(p,pos)   ((mDoublePoint *)(p)->mempt.buf + pos)

//--------------------


//===========================
// main
//===========================


/** 解放 */

void FillPolygon_free(FillPolygon *p)
{
	if(p)
	{
		mMemAutoFree(&p->mempt);
		mMemAutoFree(&p->memedge);
		mFree(p->aabuf);
		
		mFree(p);
	}
}

/** 作成 */

FillPolygon *FillPolygon_new()
{
	FillPolygon *p;

	p = (FillPolygon *)mMalloc(sizeof(FillPolygon), TRUE);
	if(!p) return NULL;

	//ポイントバッファ

	if(!mMemAutoAlloc(&p->mempt, sizeof(mDoublePoint) * 20, sizeof(mDoublePoint) * 50))
	{
		mFree(p);
		return NULL;
	}

	return p;
}

/** 頂点を追加
 *
 * @return FALSE で失敗 */

mBool FillPolygon_addPoint(FillPolygon *p,double x,double y)
{
	mDoublePoint *pbuf,pt;
	int nx,ny;

	//前の位置と同じなら追加しない

	if(p->ptnum)
	{
		pbuf = _GETPTBUF(p, p->ptnum - 1);

		if(pbuf->x == x && pbuf->y == y) return TRUE;
	}

	//追加

	pt.x = x, pt.y = y;

	if(!mMemAutoAppend(&p->mempt, &pt, sizeof(mDoublePoint)))
		return FALSE;

	p->ptnum++;

	//位置の最小/最大値

	nx = floor(x);
	ny = floor(y);

	if(p->ptnum == 1)
	{
		p->xmin = p->xmax = nx;
		p->ymin = p->ymax = ny;
	}
	else
	{
		if(nx < p->xmin) p->xmin = nx;
		if(nx > p->xmax) p->xmax = nx;

		if(ny < p->ymin) p->ymin = ny;
		if(ny > p->ymax) p->ymax = ny;
	}

	return TRUE;
}

/** 頂点を閉じる
 *
 * @return FALSE で頂点が足りない */

mBool FillPolygon_closePoint(FillPolygon *p)
{
	mDoublePoint *pbuf;

	if(p->ptnum < 3) return FALSE;

	//始点を追加

	pbuf = (mDoublePoint *)p->mempt.buf;
	FillPolygon_addPoint(p, pbuf->x, pbuf->y);

	//X 範囲を拡張

	p->xmin--;
	p->xmax++;

	return TRUE;
}


//================================
// sub
//================================


/** 交点追加 */

static mBool _add_edge(FillPolygon *p,int x,char dir)
{
	EdgeDat dat;

	dat.x = x;
	dat.dir = dir;

	if(!mMemAutoAppend(&p->memedge, &dat, sizeof(EdgeDat)))
		return FALSE;

	p->edgenum++;

	return TRUE;
}

/** 交点を小さい順に並び替え */

static void _sort_edge(FillPolygon *p)
{
	EdgeDat *pe,*pe2,*pemin,dat;
	int i,j;

	pe = (EdgeDat *)p->memedge.buf;

	for(i = 0; i < p->edgenum - 1; i++, pe++)
	{
		pemin = pe;

		for(j = i + 1, pe2 = pe + 1; j < p->edgenum; j++, pe2++)
		{
			if(pe2->x < pemin->x)
				pemin = pe2;
		}

		//入れ替え

		if(pe != pemin)
		{
			dat = *pe;
			*pe = *pemin;
			*pemin = dat;
		}
	}
}


//================================
// 描画用
//================================


/** 描画開始 */

mBool FillPolygon_beginDraw(FillPolygon *p,mBool bAntiAlias)
{
	//描画幅
	
	p->width = p->xmax - p->xmin + 1;

	//交点バッファ確保

	if(!mMemAutoAlloc(&p->memedge, sizeof(EdgeDat) * 20, sizeof(EdgeDat) * 20))
		return FALSE;

	//

	if(bAntiAlias)
	{
		//アンチエイリアス用バッファ

		p->aabuf = (uint16_t *)mMalloc(p->width * 2, FALSE);
		if(!p->aabuf) return FALSE;
	}
	else
	{
		//非アンチエイリアス時、位置を小数点切り捨て
		/* 同じ位置が続く場合があるので、それは除外 */

		int i,num = 0;
		mDoublePoint *pd,*ps;

		pd = ps = (mDoublePoint *)p->mempt.buf;

		for(i = 0; i < p->ptnum; i++, ps++)
		{
			pd->x = floor(ps->x);
			pd->y = floor(ps->y);

			//前の位置と同じなら削除

			if(!(i && pd->x == pd[-1].x && pd->y == pd[-1].y))
			{
				pd++;
				num++;
			}
		}

		if(num < 3 + _EXPAND_PTNUM) return FALSE;

		p->ptnum = num;
	}

	return TRUE;
}

/** 描画先 y の最小/最大値取得 */

void FillPolygon_getMinMaxY(FillPolygon *p,int *ymin,int *ymax)
{
	*ymin = p->ymin;
	*ymax = p->ymax;
}

/** 描画先の矩形範囲取得 */

void FillPolygon_getDrawRect(FillPolygon *p,mRect *rc)
{
	rc->x1 = p->xmin;
	rc->x2 = p->xmax;
	rc->y1 = p->ymin;
	rc->y2 = p->ymax;
}


//================================
// 非アンチエイリアス
//================================


/** 交点取得 (非アンチエイリアス用) */

mBool FillPolygon_getIntersection_noAA(FillPolygon *p,int yy)
{
	int i,x;
	mDoublePoint *ptbuf,*pt1,*pt2;
	char dir;
	double y;

	//交点クリア

	mMemAutoReset(&p->memedge);
	p->edgenum = 0;

	p->edgeCurPos = 0;
	p->edgeCurParam = 0;

	//各辺から交点取得

	ptbuf = (mDoublePoint *)p->mempt.buf;

	y = yy + 0.1;

	for(i = p->ptnum - _EXPAND_PTNUM; i > 0; i--, ptbuf++)
	{
		pt1 = ptbuf;
		pt2 = ptbuf + 1;
	
		//水平線は除外

		if(pt1->y == pt2->y) continue;

		//方向
	
		dir = (pt1->y < pt2->y)? 1: -1;

		if(dir < 0)
			pt1 = ptbuf + 1, pt2 = ptbuf;

		//y が辺の範囲外

		if(y < pt1->y || y > pt2->y) continue;

		//交点 X

		x = round(pt1->x + (y - pt1->y) / (pt2->y - pt1->y) * (pt2->x - pt1->x));

		if(!_add_edge(p, x, dir))
			return FALSE;
	}

	_sort_edge(p);

	return TRUE;
}

/** 非アンチエイリアス用、次の描画する線取得
 *
 * @return FALSE で終了 */

mBool FillPolygon_getNextLine_noAA(FillPolygon *p,int *x1,int *x2)
{
	EdgeDat *pe;

	if(p->edgeCurPos >= p->edgenum - 1) return FALSE;

	pe = (EdgeDat *)p->memedge.buf + p->edgeCurPos;

	for(; p->edgeCurPos < p->edgenum - 1; p->edgeCurPos++, pe++)
	{
		//規則カウント

		p->edgeCurParam += pe->dir;

		//カウント値が0以外なら交点間を描画

		if(p->edgeCurParam)
		{
			*x1 = pe->x;
			*x2 = pe[1].x;

			p->edgeCurPos++;

			return TRUE;
		}
	}

	return FALSE;
}


//================================
// アンチエイリアス
//================================


/** 交点リスト作成 */

static mBool _get_intersection_aa(FillPolygon *p,double y)
{
	mDoublePoint *ptbuf,pt1,pt2,pttmp;
	int i,x;
	char dir;

	//交点クリア

	mMemAutoReset(&p->memedge);
	p->edgenum = 0;

	//各辺から交点取得

	ptbuf = (mDoublePoint *)p->mempt.buf;

	for(i = p->ptnum - _EXPAND_PTNUM; i > 0; i--, ptbuf++)
	{
		pt1 = *ptbuf;
		pt2 = ptbuf[1];
	
		//水平線は除外

		if(pt1.y == pt2.y) continue;

		//方向
	
		dir = (pt1.y < pt2.y)? 1: -1;

		if(dir < 0)
			pttmp = pt1, pt1 = pt2, pt2 = pttmp;

		//y が辺の範囲外

		if(y < pt1.y || y > pt2.y) continue;

		//交点 X (8bit 固定小数点、xmin からの相対位置)

		x = round(((y - pt1.y) / (pt2.y - pt1.y) * (pt2.x - pt1.x) + pt1.x - p->xmin) * (1<<8));

		if(!_add_edge(p, x, dir))
			return FALSE;
	}

	_sort_edge(p);

	return TRUE;
}

/** アンチエイリアス用バッファにセット */

mBool FillPolygon_setXBuf_AA(FillPolygon *p,int y)
{
	uint16_t *xbuf,*px;
	EdgeDat *pe;
	double dy;
	int iy,i,cnt,x1,x2,nx1,nx2;

	xbuf = p->aabuf;
	mMemzero(xbuf, p->width * 2);

	dy = y + 0.011;  //少しずらす

	for(iy = 0; iy < _SUBPIXEL_Y; iy++, dy += 1.0 / _SUBPIXEL_Y)
	{
		//交点取得

		if(!_get_intersection_aa(p, dy)) return FALSE;

		//交点間

		pe = (EdgeDat *)p->memedge.buf;
		cnt = 0;

		for(i = p->edgenum - 1; i > 0; i--, pe++)
		{
			cnt += pe->dir;

			if(cnt)
			{
				//小数部分を濃度に反映
			
				x1 = pe->x;
				x2 = pe[1].x;

				nx1 = x1 >> 8;
				nx2 = x2 >> 8;

				px = xbuf + nx1;

				if(nx1 == nx2)
					*px += (x2 - x1) & 255;
				else
				{
					*(px++) += 255 - (x1 & 255);

					for(nx1++; nx1 < nx2; nx1++)
						*(px++) += 255;

					*px += x2 & 255;
				}
			}
		}
	}

	return TRUE;
}

/** アンチエイリアスバッファ取得 */

uint16_t *FillPolygon_getAABuf(FillPolygon *p,int *xmin,int *width)
{
	*xmin = p->xmin;
	*width = p->width;

	return p->aabuf;
}
