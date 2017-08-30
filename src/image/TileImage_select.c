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
 * TileImage
 *
 * 選択範囲関連
 *****************************************/

#include <string.h>

#include "mDef.h"
#include "mRectBox.h"
#include "mPopupProgress.h"

#include "defTileImage.h"
#include "TileImage.h"
#include "TileImageDrawInfo.h"

#include "defCanvasInfo.h"

#include "PixbufDraw.h"


//=====================================
//
//=====================================


/** 選択範囲反転 */

/* p は選択範囲イメージ。
 * 現在の選択範囲の描画可能範囲 (最小はキャンバス範囲) が対象。
 * 自動選択ツール時はキャンバス範囲のみの選択となるので、
 * キャンバス範囲外を対象に含めると、範囲外が選択されてしまう。
 * なので、キャンバス範囲内のみ反転、キャンバス範囲外は常にクリアする。 */

void TileImage_inverseSelect(TileImage *p)
{
	int ix,iy,cw,ch,fy;
	mRect rc;
	RGBAFix15 pix;

	TileImage_getCanDrawRect_pixel(p, &rc);

	cw = g_tileimage_dinfo.imgw;
	ch = g_tileimage_dinfo.imgh;

	pix.v64 = 0;

	g_tileimage_dinfo.funcColor = TileImage_colfunc_inverse_alpha;

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		fy = (iy >= 0 && iy < ch);
		
		for(ix = rc.x1; ix <= rc.x2; ix++)
		{
			//キャンバス範囲内なら反転、範囲外ならクリア
			
			pix.a = (ix >= 0 && ix < cw && fy);

			TileImage_setPixel_subdraw(p, ix, iy, &pix);
		}
	}
}

/** 選択範囲 (1bit)、点がある範囲を正確に取得 (空ではない状態) */

void TileImage_getSelectHaveRect_real(TileImage *p,mRect *dst)
{
	uint8_t **pp,*ps8,f;
	uint64_t *ps64;
	int i,ix,iy,cur;
	mRect rc;

	//タイルの範囲取得

	mRectEmpty(&rc);

	pp = p->ppbuf;

	for(iy = 0; iy < p->tileh; iy++)
	{
		for(ix = 0; ix < p->tilew; ix++, pp++)
		{
			if(*pp)
				mRectIncPoint(&rc, ix, iy);
		}
	}

	//上 (点がある一番上のY)

	pp = TILEIMAGE_GETTILE_BUFPT(p, rc.x1, rc.y1);
	cur = 63;

	for(i = rc.x2 - rc.x1 + 1; i > 0; i--, pp++)
	{
		if(!(*pp)) continue;

		ps64 = (uint64_t *)*pp;
		
		for(iy = 0; iy < 64 && iy < cur; iy++, ps64++)
		{
			if(*ps64) cur = iy;
		}
	}

	dst->y1 = p->offy + (rc.y1 << 6) + cur;

	//下

	pp = TILEIMAGE_GETTILE_BUFPT(p, rc.x1, rc.y2);
	cur = 0;

	for(i = rc.x2 - rc.x1 + 1; i > 0; i--, pp++)
	{
		if(!(*pp)) continue;

		ps64 = (uint64_t *)(*pp + 8 * 63);
		
		for(iy = 63; iy >= 0 && iy > cur; iy--, ps64--)
		{
			if(*ps64) cur = iy;
		}
	}

	dst->y2 = p->offy + (rc.y2 << 6) + cur;

	//左

	pp = TILEIMAGE_GETTILE_BUFPT(p, rc.x1, rc.y1);
	cur = 63;

	for(i = rc.y2 - rc.y1 + 1; i > 0; i--, pp += p->tilew)
	{
		if(!(*pp)) continue;
	
		for(ix = 0, f = 0x80; ix < 64 && ix < cur; ix++)
		{
			ps8 = *pp + (ix >> 3);
		
			for(iy = 0; iy < 64; iy++, ps8 += 8)
			{
				if(*ps8 & f)
				{
					cur = ix;
					break;
				}
			}

			f >>= 1;
			if(f == 0) f = 0x80;
		}
	}

	dst->x1 = p->offx + (rc.x1 << 6) + cur;

	//右

	pp = TILEIMAGE_GETTILE_BUFPT(p, rc.x2, rc.y1);
	cur = 0;

	for(i = rc.y2 - rc.y1 + 1; i > 0; i--, pp += p->tilew)
	{
		if(!(*pp)) continue;

		for(ix = 63, f = 1; ix >= 0 && ix > cur; ix--)
		{
			ps8 = *pp + (ix >> 3);
		
			for(iy = 0; iy < 64; iy++, ps8 += 8)
			{
				if(*ps8 & f)
				{
					cur = ix;
					break;
				}
			}

			if(f == 0x80)
				f = 1;
			else
				f <<= 1;
		}
	}

	dst->x2 = p->offx + (rc.x2 << 6) + cur;
}


//=====================================
// 選択範囲 拡張/縮小、輪郭描画用サブ
//=====================================


/* 64x64タイルの左上を(0,0)とした時の位置
 * (x が -1 の時 /8 が -1 になるように >>3 とする) */

#define _TILEEX2_BUFPOS(x,y)  (((y) + 2) * 10 + ((x) >> 3) + 1)


/** 指定タイルの周囲 2px を含む範囲のデータをバッファに取得
 *
 * @return 一つでも点があるか */

static mBool _gettilebuf_ex2(TileImage *p,uint8_t *dstbuf,uint8_t **pptile,int tx,int ty)
{
	uint8_t *ps,*pd,flags;
	uint32_t *p32;
	int i;

	//クリア

	memset(dstbuf, 0, 10 * 68);

	//指定タイルのデータをコピー (8byte 単位)

	ps = *pptile;
	pd = dstbuf + _TILEEX2_BUFPOS(0,0);

	for(i = 64; i; i--)
	{
		*((uint64_t *)pd) = *((uint64_t *)ps);

		ps += 8;
		pd += 10;
	}

	//一つ上下左右のタイルが範囲内かどうかのフラグ

	flags = (tx != 0) | ((tx < p->tilew - 1) << 1)
		| ((ty != 0) << 2) | ((ty < p->tileh - 1) << 3);

	//一つ左のタイルの右端 2px

	if((flags & 1) && *(pptile - 1))
	{
		ps = *(pptile - 1) + 7;
		pd = dstbuf + _TILEEX2_BUFPOS(-1,0);

		for(i = 64; i; i--, ps += 8, pd += 10)
			*pd = *ps;
	}

	//一つ右のタイルの左端 2px

	if((flags & 2) && *(pptile + 1))
	{
		ps = *(pptile + 1);
		pd = dstbuf + _TILEEX2_BUFPOS(64,0);

		for(i = 64; i; i--, ps += 8, pd += 10)
			*pd = *ps;
	}

	//一つ上のタイルの下端 2px

	if((flags & 4) && *(pptile - p->tilew))
	{
		ps = *(pptile - p->tilew) + 62 * 8;
		pd = dstbuf + _TILEEX2_BUFPOS(0,-2);

		for(i = 2; i; i--, ps += 8, pd += 10)
			*((uint64_t *)pd) = *((uint64_t *)ps);
	}

	//一つ下のタイルの上端 2px

	if((flags & 8) && *(pptile + p->tilew))
	{
		ps = *(pptile + p->tilew);
		pd = dstbuf + _TILEEX2_BUFPOS(0,64);

		for(i = 2; i; i--, ps += 8, pd += 10)
			*((uint64_t *)pd) = *((uint64_t *)ps);
	}

	//(-1,-1)

	if((flags & 5) == 5)
	{
		ps = *(pptile - 1 - p->tilew);

		if(ps && (ps[8 * 64 - 1] & 1))
			dstbuf[_TILEEX2_BUFPOS(-1,-1)] |= 1;
	}

	//(-1,64)

	if((flags & 9) == 9)
	{
		ps = *(pptile - 1 + p->tilew);

		if(ps && (ps[7] & 1))
			dstbuf[_TILEEX2_BUFPOS(-1,64)] |= 1;
	}

	//(64,-1)

	if((flags & 6) == 6)
	{
		ps = *(pptile + 1 - p->tilew);

		if(ps && (ps[8 * 63] & 0x80))
			dstbuf[_TILEEX2_BUFPOS(64,-1)] |= 0x80;
	}

	//(64,64)

	if((flags & 10) == 10)
	{
		ps = *(pptile + 1 + p->tilew);

		if(ps && (*ps & 0x80))
			dstbuf[_TILEEX2_BUFPOS(64,64)] |= 0x80;
	}

	//一つでも点があるか判定

	p32 = (uint32_t *)dstbuf;

	for(i = 10 * 68 / 4; i && *p32 == 0; i--, p32++);

	return (i != 0);
}


//=====================================
// 選択範囲 拡張/縮小
//=====================================


/** タイル 1px 拡張 */

static void _expand_tile_inflate(TileImage *p,uint8_t *buf,int px,int py)
{
	uint8_t *ps,f;
	int ix,iy,n;
	RGBAFix15 pix;

	pix.v64 = 0;
	pix.a = 0x8000;

	/* 周囲1px分を含む範囲で、点がなくて上下左右いずれかに点がある場合、点を追加 */

	ps = buf + _TILEEX2_BUFPOS(-1, -1);

	for(iy = -1; iy <= 64; iy++, ps++)
	{
		for(ix = -1, f = 1; ix <= 64; ix++)
		{
			if(!(*ps & f))
			{
				//上下

				n = (ps[-10] & f) | (ps[10] & f);

				//左
			
				n |= (f == 0x80)? ps[-1] & 1: *ps & (f << 1);

				//右

				n |= (f == 1)? ps[1] & 0x80: *ps & (f >> 1);

				//上下左右いずれかに点があればセット

				if(n)
					TileImage_setPixel_subdraw(p, px + ix, py + iy, &pix);
			}

			f >>= 1;
			if(!f) f = 0x80, ps++;
		}
	}
}

/** タイル 1px 縮小 */

static void _expand_tile_deflate(TileImage *p,uint8_t *buf,int px,int py)
{
	uint8_t *ps,f;
	int ix,iy,n;
	RGBAFix15 pix;

	pix.v64 = 0;

	/* 64x64 の範囲で、点があって上下左右いずれかに点がない場合、点を消去 */

	ps = buf + _TILEEX2_BUFPOS(0, 0);

	for(iy = 0; iy < 64; iy++, ps += 2)
	{
		for(ix = 0, f = 0x80; ix < 64; ix++)
		{
			if(*ps & f)
			{
				//上下

				n = !(ps[-10] & f) | !(ps[10] & f);

				//左
			
				n |= !((f == 0x80)? ps[-1] & 1: *ps & (f << 1));

				//右

				n |= !((f == 1)? ps[1] & 0x80: *ps & (f >> 1));

				//上下左右いずれかに点がない場合消去

				if(n)
					TileImage_setPixel_subdraw(p, px + ix, py + iy, &pix);
			}

			f >>= 1;
			if(!f) f = 0x80, ps++;
		}
	}
}

/** 選択範囲 拡張/縮小 */

void TileImage_expandSelect(TileImage *p,int pxcnt,mPopupProgress *prog)
{
	TileImage *tmp;
	uint8_t **pptile,*buf;
	int i,ix,iy,w,h,px,py;

	//作業用タイルバッファ

	buf = (uint8_t *)mMalloc(10 * 68, FALSE);
	if(!buf) return;

	//

	i = (pxcnt < 0)? -pxcnt: pxcnt;

	mPopupProgressThreadSetMax(prog, i * 5);

	for(; i > 0; i--)
	{
		//判定元用にコピー

		tmp = TileImage_newClone(p);
		if(!tmp) break;

		//tmp を元にタイルごとに処理して p に点セット

		pptile = tmp->ppbuf;
		w = tmp->tilew;
		h = tmp->tileh;

		mPopupProgressThreadBeginSubStep(prog, 5, h);

		for(iy = 0, py = tmp->offy; iy < h; iy++, py += 64)
		{
			for(ix = 0, px = tmp->offx; ix < w; ix++, px += 64, pptile++)
			{
				if(*pptile && _gettilebuf_ex2(tmp, buf, pptile, ix, iy))
				{
					if(pxcnt > 0)
						_expand_tile_inflate(p, buf, px, py);
					else
						_expand_tile_deflate(p, buf, px, py);
				}
			}

			mPopupProgressThreadIncSubStep(prog);
		}

		//

		TileImage_free(tmp);
	}

	mFree(buf);
}


//=====================================
// 選択範囲 輪郭描画
//=====================================
/*
 * 作業用バッファは、64x64 + 周囲2px の 1bit データ。
 * 左端は 2bit 分余白。
 * [2bit余白][一つ左2px]+[タイル64]+[一つ右2px][2bit余白] で横は 10 byte。
 */

typedef struct
{
	CanvasDrawInfo *cdinfo;
	mRect rcclipimg,
		rcclipcanv;
	double dadd[4];
	uint8_t fpixelbox;	//拡大表示時、ピクセルごとに枠を描画
}_drawselinfo;



/** 拡大表示時の1点の輪郭描画 */

static void _drawsel_drawedge_point(mPixbuf *pixbuf,
	double dx,double dy,double *padd,int flags,mRect *rcclip)
{
	dx += 0.5;
	dy += 0.5;

	if(flags & 1)
		pixbufDraw_lineSelectEdge(pixbuf, dx, dy, dx + padd[0], dy + padd[1], rcclip);

	if(flags & 2)
		pixbufDraw_lineSelectEdge(pixbuf, dx + padd[2], dy + padd[3], dx + padd[0] + padd[2], dy + padd[1] + padd[3], rcclip);

	if(flags & 4)
		pixbufDraw_lineSelectEdge(pixbuf, dx, dy, dx + padd[2], dy + padd[3], rcclip);

	if(flags & 8)
		pixbufDraw_lineSelectEdge(pixbuf, dx + padd[0], dy + padd[1], dx + padd[0] + padd[2], dy + padd[1] + padd[3], rcclip);
}

/** タイルデータから輪郭描画
 *
 * @param px,py     タイルの左上イメージ位置
 * @param rcclip    イメージのクリッピング範囲
 * @param rcclipdst キャンバス描画範囲のクリッピング */

static void _drawsel_drawedge(mPixbuf *pixbuf,uint8_t *tilebuf,
	int px,int py,_drawselinfo *info)
{
	int sx,sy,w,h,img_right,img_bottom,ix,iy,x,y;
	uint8_t *ps,*psY,stf,f,flags,fpixelbox,flags_y,tmp;
	double ddx,ddy,ddx2,ddy2,*dadd;
	mRect rc;

	//描画するタイル範囲をイメージ座標からクリッピング
	/* sx,sy : tilebuf (ここでは左上を(0,0)と仮定する) */

	sx = sy = 0;
	w = h = 66;
	rc = info->rcclipimg;	//x2,y2 は +1

	if(px < rc.x1) w += px - rc.x1, sx += rc.x1 - px, px = rc.x1;
	if(py < rc.y1) h += py - rc.y1, sy += rc.y1 - py, py = rc.y1;
	if(px + w > rc.x2) w = rc.x2 - px;
	if(py + h > rc.y2) h = rc.y2 - py;

	if(w <= 0 || h <= 0) return;

	//イメージの左/下端

	img_right = g_tileimage_dinfo.imgw - 1;
	img_bottom = g_tileimage_dinfo.imgh - 1;

	//タイルデータの開始位置
	//(周囲1px分も描画するので、位置は -1)

	sx--, sy--;

	ps = tilebuf + _TILEEX2_BUFPOS(sx, sy);

	stf = 1 << (7 - (sx & 7));

	//

	CanvasDrawInfo_imageToarea(info->cdinfo, px, py, &ddx2, &ddy2);

	fpixelbox = info->fpixelbox;
	dadd = info->dadd;

	//

	y = py;

	for(iy = 0; iy < h; iy++, y++)
	{
		x = px;
		f = stf;
		psY = ps;

		flags_y = (y == 0) | ((y == img_bottom) << 1);

		ddx = ddx2;
		ddy = ddy2;

		for(ix = 0; ix < w; ix++, x++)
		{
			if(*ps & f)
			{
				//------- 点がある場合、イメージ全体の端だった場合は輪郭描画
				
				//端の位置かどうかのフラグ
				
				flags = flags_y | ((x == 0) << 2) | ((x == img_right) << 3);

				//

				if(flags)
				{
					if(fpixelbox)
						_drawsel_drawedge_point(pixbuf, ddx, ddy, dadd, flags, &info->rcclipcanv);
					else
						pixbufDraw_setPixelSelectEdge(pixbuf, (int)(ddx + 0.5), (int)(ddy + 0.5), &info->rcclipcanv);
				}
			}
			else
			{
				//------ 点がない場合、上下左右いずれかに点があれば輪郭描画

				flags = 0;

				//上

				if(ps[-10] & f) flags |= 1;

				//下

				if(ps[10] & f) flags |= 2;

				//左

				tmp = (f == 0x80)? ps[-1] & 1: *ps & (f << 1);

				if(tmp) flags |= 4;

				//右

				tmp = (f == 1)? ps[1] & 0x80: *ps & (f >> 1);

				if(tmp) flags |= 8;

				//描画

				if(flags)
				{
					if(fpixelbox)
						_drawsel_drawedge_point(pixbuf, ddx, ddy, dadd, flags, &info->rcclipcanv);
					else
						pixbufDraw_setPixelSelectEdge(pixbuf, (int)(ddx + 0.5), (int)(ddy + 0.5), &info->rcclipcanv);
				}
			}

			f >>= 1;
			if(!f) f = 0x80, ps++;

			ddx += dadd[0];
			ddy += dadd[1];
		}

		ps = psY + 10;

		ddx2 += dadd[2];
		ddy2 += dadd[3];
	}
}

/** キャンバスに選択範囲の輪郭を描画
 *
 * @param boximg キャンバス描画範囲に相当するイメージ範囲 */

void TileImage_drawSelectEdge(TileImage *p,mPixbuf *pixbuf,CanvasDrawInfo *cdinfo,mBox *boximg)
{
	_drawselinfo info;
	TileImageTileRectInfo tinfo;
	uint8_t **pptile,*buf;
	int ix,iy,px,py;

	if(!p->ppbuf) return;

	//タイル範囲

	pptile = TileImage_getTileRectInfo(p, &tinfo, boximg);
	if(!pptile) return;

	//作業用バッファ

	buf = (uint8_t *)mMalloc(10 * 68, FALSE);
	if(!buf) return;

	//情報

	info.cdinfo = cdinfo;
	info.rcclipimg = tinfo.rcclip;

	mRectSetByBox(&info.rcclipcanv, &cdinfo->boxdst);

	info.dadd[0] = cdinfo->param->scale * cdinfo->param->cos;
	info.dadd[1] = cdinfo->param->scale * cdinfo->param->sin;
	info.dadd[2] = -info.dadd[1];
	info.dadd[3] = info.dadd[0];

	if(cdinfo->mirror)
	{
		info.dadd[0] = -info.dadd[0];
		info.dadd[1] = -info.dadd[1];
	}

	info.fpixelbox = (cdinfo->param->scale > 1);

	//タイルごとに処理

	for(iy = tinfo.rctile.y1, py = tinfo.pxtop.y; iy <= tinfo.rctile.y2; iy++, py += 64, pptile += tinfo.pitch)
	{
		for(ix = tinfo.rctile.x1, px = tinfo.pxtop.x; ix <= tinfo.rctile.x2; ix++, px += 64, pptile++)
		{
			if(*pptile)
			{
				//一つも点がない場合は何もしない
			
				if(_gettilebuf_ex2(p, buf, pptile, ix, iy))
					_drawsel_drawedge(pixbuf, buf, px - 1, py - 1, &info);
			}
		}
	}
	
	//

	mFree(buf);
}
