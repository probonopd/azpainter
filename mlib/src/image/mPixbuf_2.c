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
 * mPixbuf
 *****************************************/

#include "mDef.h"
#include "mPixbuf.h"
#include "mPixbuf_pv.h"

#include "mImageBuf.h"


/**
@addtogroup pixbuf
 
@{
*/


/** mImageBuf から転送 */

void mPixbufBltImageBuf(mPixbuf *dst,int dx,int dy,mImageBuf *src,int sx,int sy,int w,int h)
{
	mPixbufBltInfo info;
	uint8_t *ps,*pd;
	int ix,iy,pitchsrc,pitchdst;

	if(w < 0) w = src->w;
	if(h < 0) h = src->h;

	if(!__mPixbufBltClip(&info, dst, dx, dy, sx, sy, w, h)) return;

	//

	pd = mPixbufGetBufPtFast(dst, info.dx, info.dy);
	ps = MIMAGEBUF_GETBUFPT(src, info.sx, info.sy);

	pitchsrc = src->pitch - info.w * src->bpp;
	pitchdst = dst->pitch_dir - info.w * dst->bpp;

	for(iy = info.h; iy > 0; iy--)
	{
		for(ix = info.w; ix > 0; ix--)
		{
			(dst->setbuf)(pd, mRGBtoPix2(ps[0], ps[1], ps[2]));
		
			ps += src->bpp;
			pd += dst->bpp;
		}

		ps += pitchsrc;
		pd += pitchdst;
	}
}

/** mImageBuf 画像をタイル上に並べて描画
 *
 * オフセット位置を原点とする */

void mPixbufTileImageBuf(mPixbuf *dst,int x,int y,int w,int h,mImageBuf *src)
{
	mRect rc;
	uint8_t *pd,*psY,*ps;
	int ix,iy,pitchdst,sx,sxleft,sy;

	if(!dst || !src) return;

	if(!mPixbufGetClipRect_d(dst, &rc, x, y, w, h)) return;

	//

	pd = mPixbufGetBufPtFast(dst, rc.x1, rc.y1);

	pitchdst = dst->pitch_dir - (rc.x2 - rc.x1 + 1) * dst->bpp;

	sxleft = rc.x1 % src->w;
	sy = rc.y1 % src->h;
	psY = src->buf + src->pitch * sy;

	//

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		sx = sxleft;
		ps = psY + sx * src->bpp;
	
		for(ix = rc.x1; ix <= rc.x2; ix++, pd += dst->bpp)
		{
			(dst->setbuf)(pd, mRGBtoPix2(ps[0], ps[1], ps[2]));

			//次のソース位置

			sx++;
			if(sx == src->w)
			{
				sx = 0;
				ps = psY;
			}
			else
				ps += src->bpp;
		}

		pd += pitchdst;

		//次のソースY位置

		sy++;
		if(sy == src->h)
		{
			sy = 0;
			psY = src->buf;
		}
		else
			psY += src->pitch;
	}
}

/** 画像を拡大縮小して描画 (オーバーサンプリング)
 *
 * @param overnum オーバーサンプリング数 */

void mPixbufScaleImageBuf_oversamp(mPixbuf *dst,int x,int y,int w,int h,
	mImageBuf *src,int overnum)
{
	mRect rc;
	int ix,iy,jx,jy,pitchdst,fx,fy,fxleft,fincx,fincy,
		n,f,r,g,b,div,fincx2,fincy2;
	double ratex,ratey;
	uint8_t *pd,*psY,*ps;
	intptr_t *tablebuf;

	if(!dst || !src) return;

	//拡大縮小なしの場合は通常転送

	if(w == src->w && h == src->h)
	{
		mPixbufBltImageBuf(dst, x, y, src, 0, 0, w, h);
		return;
	}

	//クリッピング

	if(!mPixbufGetClipRect_d(dst, &rc, x, y, w, h)) return;

	//

	pd = mPixbufGetBufPtFast(dst, rc.x1, rc.y1);
	pitchdst = dst->pitch_dir - (rc.x2 - rc.x1 + 1) * dst->bpp;

	div = overnum * overnum;

	ratex = (double)src->w / w;
	ratey = (double)src->h / h;

	fxleft = (int)((rc.x1 - x) * ratex * (1 << 16));
	fy = (int)((rc.y1 - y) * ratey * (1 << 16));
	fincx = (int)(ratex * (1 << 16));
	fincy = (int)(ratey * (1 << 16));
	fincx2 = (int)(ratex / overnum * (1 << 16));
	fincy2 = (int)(ratey / overnum * (1 << 16));

	//テーブルバッファ

	tablebuf = (intptr_t *)mMalloc(sizeof(intptr_t) * overnum * 2, FALSE);
	if(!tablebuf) return;

	//

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		fx = fxleft;

		//Y テーブル作成 (Yバッファ位置)

		for(jy = 0, f = fy; jy < overnum; jy++, f += fincy2)
		{
			n = f >> 16;
			if(n >= src->h) n = src->h - 1;
			
			tablebuf[jy] = (intptr_t)(src->buf + src->pitch * n);
		}

		//------ X
	
		for(ix = rc.x1; ix <= rc.x2; ix++, pd += dst->bpp, fx += fincx)
		{
			//X テーブル作成 (Xバイト位置)

			for(jx = 0, f = fx; jx < overnum; jx++, f += fincx2)
			{
				n = f >> 16;
				if(n >= src->w) n = src->w - 1;
				
				tablebuf[overnum + jx] = n * src->bpp;
			}
		
			//オーバーサンプリング

			r = g = b = 0;

			for(jy = 0; jy < overnum; jy++)
			{
				psY = (uint8_t *)tablebuf[jy];
			
				for(jx = 0; jx < overnum; jx++)
				{
					ps = psY + tablebuf[overnum + jx];

					r += ps[0];
					g += ps[1];
					b += ps[2];
				}
			}

			r /= div;
			g /= div;
			b /= div;

			//

			(dst->setbuf)(pd, mRGBtoPix2(r, g, b));
		}

		pd += pitchdst;
		fy += fincy;
	}

	mFree(tablebuf);
}

/** @} */
