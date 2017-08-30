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

#include <string.h>

#include "mDef.h"
#include "mPixbuf.h"
#include "mPixbuf_pv.h"

#include "mAppDef.h"
#include "mSysCol.h"
#include "mRectBox.h"
#include "mLoadImage.h"


//--------------------------

mPixbuf *__mPixbufAlloc(void);
void __mPixbufFree(mPixbuf *p);
int __mPixbufCreate(mPixbuf *p,int w,int h);

//--------------------------


/*****************************//**

@defgroup pixbuf mPixbuf
@brief 画面に転送可能なピクセルバッファ

ディスプレイ環境により 16/24/32 bit イメージのいずれか。\n
ビットオーダーは OS に依存。

@ingroup group_image
@{

@file mPixbuf.h
@struct _mPixbuf
@enum MPIXBUF_DRAWBTT_FLAGS
@enum MPIXBUF_DRAWCKBOX_FLAGS

@def MPIXBUF_COL_XOR
XOR で塗る場合の色値

**********************************/


//====================================
// ピクセルバッファ位置に色をセット
//====================================


static void _setbuf_32bit(uint8_t *pd,mPixCol col)
{
	if(col == MPIXBUF_COL_XOR)
		*((uint32_t *)pd) ^= 0xffffffff;
	else
		*((uint32_t *)pd) = col;
}

static void _setbuf_24bit(uint8_t *pd,mPixCol col)
{
	if(col == MPIXBUF_COL_XOR)
	{
		pd[0] ^= 0xff;
		pd[1] ^= 0xff;
		pd[2] ^= 0xff;
	}
	else
	{
	#ifdef MLIB_BIGENDIAN
		pd[0] = (uint8_t)(col >> 16);
		pd[1] = (uint8_t)(col >> 8);
		pd[2] = (uint8_t)col;
	#else
		pd[0] = (uint8_t)col;
		pd[1] = (uint8_t)(col >> 8);
		pd[2] = (uint8_t)(col >> 16);
	#endif
	}
}

static void _setbuf_16bit(uint8_t *pd,mPixCol col)
{
	if(col == MPIXBUF_COL_XOR)
		*((uint16_t *)pd) ^= 0xffff;
	else
		*((uint16_t *)pd) = col;
}


//=============================


/** 解放 */

void mPixbufFree(mPixbuf *p)
{
	if(p)
	{
		__mPixbufFree(p);
		mFree(p);
	}
}

/** 作成 */

mPixbuf *mPixbufCreate(int w,int h)
{
	mPixbuf *p;
	
	if(w < 1) w = 1;
	if(h < 1) h = 1;
	
	//
	
	p = __mPixbufAlloc();
	if(!p) return NULL;
		
	if(__mPixbufCreate(p, w, h))
	{
		mFree(p);
		return NULL;
	}
	
	//
		
	if(p->bpp == 4)
		p->setbuf = _setbuf_32bit;
	else if(p->bpp == 3)
		p->setbuf = _setbuf_24bit;
	else
		p->setbuf = _setbuf_16bit;

	return p;
}

/** サイズ変更 */

int mPixbufResizeStep(mPixbuf *p,int w,int h,int stepw,int steph)
{
	int neww,newh;

	if(w < 1) w = 1;
	if(h < 1) h = 1;
	
	neww = (w + stepw - 1) / stepw * stepw;
	newh = (h + steph - 1) / steph * steph;
	
	if(p->w != neww || p->h != newh)
	{
		if(__mPixbufCreate(p, neww, newh))
			return -1;
	}

	return 0;
}


//=========================
// 画像読み込み
//=========================


typedef struct
{
	mLoadImage base;
	mPixbuf *img;
	uint8_t *dstbuf;
}LOADIMAGE_INFO;


/** 情報取得 */

static int _loadimage_getinfo(mLoadImage *load,mLoadImageInfo *info)
{
	LOADIMAGE_INFO *p = (LOADIMAGE_INFO *)load;

	p->img = mPixbufCreate(info->width, info->height);
	if(!p->img) return MLOADIMAGE_ERR_ALLOC;

	p->dstbuf = p->img->buftop;
	if(info->bottomup) p->dstbuf += (info->height - 1) * p->img->pitch_dir;

	return MLOADIMAGE_ERR_OK;
}

/** Y1行取得 */

static int _loadimage_getrow(mLoadImage *load,uint8_t *buf,int pitch)
{
	LOADIMAGE_INFO *p = (LOADIMAGE_INFO *)load;
	mPixbuf *img;
	uint8_t *ps,*pd;
	int i;

	img = p->img;
	ps = buf;
	pd = p->dstbuf;

	for(i = load->info.width; i > 0; i--, ps += 3, pd += img->bpp)
		(img->setbuf)(pd, mRGBtoPix2(ps[0], ps[1], ps[2]));

	p->dstbuf += (load->info.bottomup)? -(p->img->pitch_dir): p->img->pitch_dir;

	return MLOADIMAGE_ERR_OK;
}

/** ファイルまたはバッファから画像読み込み */

mPixbuf *mPixbufLoadImage(mLoadImageSource *src,mDefEmptyFunc loadfunc,char **errmes)
{
	LOADIMAGE_INFO *p;
	mPixbuf *img;
	mBool ret;

	//mLoadImage

	p = (LOADIMAGE_INFO *)mLoadImage_create(sizeof(LOADIMAGE_INFO));
	if(!p) return NULL;

	p->base.format = MLOADIMAGE_FORMAT_RGB;
	p->base.src = *src;
	p->base.getinfo = _loadimage_getinfo;
	p->base.getrow = _loadimage_getrow;

	//読み込み関数

	ret = ((mLoadImageFunc)loadfunc)(M_LOADIMAGE(p));

	//結果

	img = p->img;

	if(!ret)
	{
		mPixbufFree(img);
		img = NULL;

		if(errmes) *errmes = mStrdup(p->base.message);
	}

	mLoadImage_free(M_LOADIMAGE(p));

	return img;
}


//======================


/** 指定位置のピクセルバッファ位置取得
 * 
 * @return 範囲外は NULL */

uint8_t *mPixbufGetBufPt(mPixbuf *p,int x,int y)
{
	x += M_PIXBUFBASE(p)->p.offsetX;
	y += M_PIXBUFBASE(p)->p.offsetY;

	if(x >= 0 && x < p->w && y >= 0 && y < p->h)
		return p->buftop + y * p->pitch_dir + x * p->bpp;
	else
		return NULL;
}

/** 指定位置のピクセルバッファ位置取得 (範囲判定なし) */

uint8_t *mPixbufGetBufPtFast(mPixbuf *p,int x,int y)
{
	return p->buftop + (y + M_PIXBUFBASE(p)->p.offsetY) * p->pitch_dir
			+ (x + M_PIXBUFBASE(p)->p.offsetX) * p->bpp;
}

/** 指定位置のピクセルバッファ位置取得 (絶対位置)
 * 
 * @return 範囲外は NULL */

uint8_t *mPixbufGetBufPt_abs(mPixbuf *p,int x,int y)
{
	if(x >= 0 && x < p->w && y >= 0 && y < p->h)
		return p->buftop + y * p->pitch_dir + x * p->bpp;
	else
		return NULL;
}

/** 指定位置のピクセルバッファ位置取得 (絶対位置・範囲判定なし) */

uint8_t *mPixbufGetBufPtFast_abs(mPixbuf *p,int x,int y)
{
	return p->buftop + y * p->pitch_dir + x * p->bpp;
}

/** 指定位置の色 (RGB) を取得 */

mRgbCol mPixbufGetPixelRGB(mPixbuf *p,int x,int y)
{
	uint8_t *pd;
	uint32_t col = 0;

	x += M_PIXBUFBASE(p)->p.offsetX;
	y += M_PIXBUFBASE(p)->p.offsetY;

	if(x < 0 || x >= p->w || y < 0 || y >= p->h)
		return 0;
	else
	{
		pd = p->buftop + y * p->pitch_dir + x * p->bpp;
		
		switch(p->bpp)
		{
			case 4:
				col = *((uint32_t *)pd);
				break;
			case 3:
			#ifdef MLIB_BIGENDIAN
				col = ((uint32_t)pd[0] << 16) | (pd[1] << 8) | pd[2];
			#else
				col = ((uint32_t)pd[2] << 16) | (pd[1] << 8) | pd[0];
			#endif
				break;
			case 2:
				col = *((uint16_t *)pd);
				break;
		}
		
		return mPixtoRGB(col);
	}
}

/** バッファ位置からRGB値を取得 */

void mPixbufGetPixelBufRGB(mPixbuf *p,uint8_t *buf,int *pr,int *pg,int *pb)
{
	uint32_t col;

	switch(p->bpp)
	{
		case 4:
			col = *((uint32_t *)buf);
			break;
		case 3:
		#ifdef MLIB_BIGENDIAN
			col = ((uint32_t)buf[0] << 16) | (buf[1] << 8) | buf[2];
		#else
			col = ((uint32_t)buf[2] << 16) | (buf[1] << 8) | buf[0];
		#endif
			break;
		case 2:
			col = *((uint16_t *)buf);
			break;
	}

	col = mPixtoRGB(col);

	*pr = M_GET_R(col);
	*pg = M_GET_G(col);
	*pb = M_GET_B(col);
}

/** オフセット位置セット
 * 
 * @param pt 元のオフセット位置が入る。NULL 可。 */

void mPixbufSetOffset(mPixbuf *p,int x,int y,mPoint *pt)
{
	if(pt)
	{
		pt->x = M_PIXBUFBASE(p)->p.offsetX;
		pt->y = M_PIXBUFBASE(p)->p.offsetY;
	}

	M_PIXBUFBASE(p)->p.offsetX = x;
	M_PIXBUFBASE(p)->p.offsetY = y;
}


//============================
// クリッピング
//============================
 

/** (内部用) マスタークリッピング範囲セット */

void __mPixbufSetClipMaster(mPixbuf *p,mRect *rc)
{
	if(rc)
	{
		M_PIXBUFBASE(p)->p.clipMaster = *rc;
		M_PIXBUFBASE(p)->p.clip = *rc;
		M_PIXBUFBASE(p)->p.flags |= MPIXBUF_F_CLIP | MPIXBUF_F_CLIP_MASTER;
	}
	else
	{
		M_PIXBUFBASE(p)->p.flags &= ~(MPIXBUF_F_CLIP | MPIXBUF_F_CLIP_MASTER);
	}
}


/** クリッピングをなしに */

void mPixbufClipNone(mPixbuf *p)
{
	if(M_PIXBUFBASE(p)->p.flags & MPIXBUF_F_CLIP_MASTER)
		M_PIXBUFBASE(p)->p.clip = M_PIXBUFBASE(p)->p.clipMaster;
	else
		M_PIXBUFBASE(p)->p.flags &= ~MPIXBUF_F_CLIP;
}

/** クリッピング範囲セット
 *
 * @return 描画範囲があるか */

mBool mPixbufSetClipBox_d(mPixbuf *p,int x,int y,int w,int h)
{
	mPixbufBase *pb = M_PIXBUFBASE(p);
	mBool ret;

	mRectSetBox_d(&pb->p.clip, x + pb->p.offsetX, y + pb->p.offsetY, w, h);

	if(pb->p.flags & MPIXBUF_F_CLIP_MASTER)
		ret = mRectClipRect(&pb->p.clip, &pb->p.clipMaster);
	else
		ret = mRectClipBox_d(&pb->p.clip, 0, 0, p->w, p->h);

	pb->p.flags |= MPIXBUF_F_CLIP;

	return ret;
}

/** クリッピング範囲セット (mBox)
 *
 * @return 描画範囲があるか */

mBool mPixbufSetClipBox_box(mPixbuf *p,mBox *box)
{
	return mPixbufSetClipBox_d(p, box->x, box->y, box->w, box->h);
}

/** 点がクリッピング範囲内にあるか (絶対位置) */

mBool mPixbufIsInClip_abs(mPixbuf *p,int x,int y)
{
	if(M_PIXBUFBASE(p)->p.flags & MPIXBUF_F_CLIP)
	{
		if(mRectIsEmpty(&M_PIXBUFBASE(p)->p.clip))
			return FALSE;
		else
			return (M_PIXBUFBASE(p)->p.clip.x1 <= x && x <= M_PIXBUFBASE(p)->p.clip.x2
				&& M_PIXBUFBASE(p)->p.clip.y1 <= y && y <= M_PIXBUFBASE(p)->p.clip.y2);
	}
	else
	{
		return (x >= 0 && x < p->w && y >= 0 && y < p->h);
	}
}

/** クリッピングを行った範囲を取得 (mRect で) */

mBool mPixbufGetClipRect_d(mPixbuf *p,mRect *rc,int x,int y,int w,int h)
{
	mRect rc1;
	int offx,offy;
	mBool ret;

	offx = M_PIXBUFBASE(p)->p.offsetX;
	offy = M_PIXBUFBASE(p)->p.offsetY;

	rc1.x1 = x + offx;
	rc1.y1 = y + offy;
	rc1.x2 = rc1.x1 + w - 1;
	rc1.y2 = rc1.y1 + h - 1;

	if(M_PIXBUFBASE(p)->p.flags & MPIXBUF_F_CLIP)
		ret = mRectClipRect(&rc1, &M_PIXBUFBASE(p)->p.clip);
	else
		ret = mRectClipBox_d(&rc1, 0, 0, p->w, p->h);

	if(ret)
	{
		rc1.x1 -= offx;
		rc1.x2 -= offx;
		rc1.y1 -= offy;
		rc1.y2 -= offy;

		*rc = rc1;
	}

	return ret;
}

/** クリッピングを行った範囲を取得 (mBox で) */

mBool mPixbufGetClipBox_d(mPixbuf *p,mBox *box,int x,int y,int w,int h)
{
	mRect rc;

	if(!mPixbufGetClipRect_d(p, &rc, x, y, w, h))
		return FALSE;
	else
	{
		box->x = rc.x1, box->y = rc.y1;
		box->w = rc.x2 - rc.x1 + 1;
		box->h = rc.y2 - rc.y1 + 1;

		return TRUE;
	}
}

/** クリッピングを行った範囲を取得 (mBox -> mBox) */

mBool mPixbufGetClipBox_box(mPixbuf *p,mBox *box,mBox *boxsrc)
{
	return mPixbufGetClipBox_d(p, box,
			boxsrc->x, boxsrc->y, boxsrc->w, boxsrc->h);
}


//=============================
// 描画 (バッファに直接)
//=============================


/** 水平線描画 */

void mPixbufRawLineH(mPixbuf *p,uint8_t *buf,int w,mPixCol pix)
{
	uint32_t *p32;
	uint16_t *p16;
	uint8_t c1,c2,c3;

	if(pix == MPIXBUF_COL_XOR)
	{
		//XOR

		switch(p->bpp)
		{
			case 4:
				for(p32 = (uint32_t *)buf; w > 0; w--)
					*(p32++) ^= 0xffffffff;
				break;
			case 3:
				for(; w > 0; w--, buf += 3)
				{
					buf[0] ^= 0xff;
					buf[1] ^= 0xff;
					buf[2] ^= 0xff;
				}
				break;
			case 2:
				for(p16 = (uint16_t *)buf; w > 0; w--)
					*(p16++) ^= 0xffff;
				break;
		}
	}
	else
	{
		//通常

		switch(p->bpp)
		{
			case 4:
				for(p32 = (uint32_t *)buf; w > 0; w--)
					*(p32++) = pix;
				break;
			case 3:
			#ifdef MLIB_BIGENDIAN
				c1 = (uint8_t)(pix >> 16);
				c2 = (uint8_t)(pix >> 8);
				c3 = (uint8_t)pix;
			#else
				c1 = (uint8_t)pix;
				c2 = (uint8_t)(pix >> 8);
				c3 = (uint8_t)(pix >> 16);
			#endif
			
				for(; w > 0; w--, buf += 3)
					buf[0] = c1, buf[1] = c2, buf[2] = c3;
				break;
			case 2:
				for(p16 = (uint16_t *)buf; w > 0; w--)
					*(p16++) = (uint16_t)pix;
				break;
		}
	}
}

/** 垂直線描画 */

void mPixbufRawLineV(mPixbuf *p,uint8_t *buf,int h,mPixCol pix)
{
	uint8_t c1,c2,c3;
	int pitch;
	
	pitch = p->pitch_dir;

	if(pix == MPIXBUF_COL_XOR)
	{
		//XOR

		switch(p->bpp)
		{
			case 4:
				for(; h > 0; h--, buf += pitch)
					*((uint32_t *)buf) ^= 0xffffffff;
				break;
			case 3:
				for(; h > 0; h--, buf += pitch)
				{
					buf[0] ^= 0xff;
					buf[1] ^= 0xff;
					buf[2] ^= 0xff;
				}
				break;
			case 2:
				for(; h > 0; h--, buf += pitch)
					*((uint16_t *)buf) ^= 0xffff;
				break;
		}
	}
	else
	{
		//通常
	
		switch(p->bpp)
		{
			case 4:
				for(; h > 0; h--, buf += pitch)
					*((uint32_t *)buf) = pix;
				break;
			case 3:
			#ifdef MLIB_BIGENDIAN
				c1 = (uint8_t)(pix >> 16);
				c2 = (uint8_t)(pix >> 8);
				c3 = (uint8_t)pix;
			#else
				c1 = (uint8_t)pix;
				c2 = (uint8_t)(pix >> 8);
				c3 = (uint8_t)(pix >> 16);
			#endif
			
				for(; h > 0; h--, buf += pitch)
					buf[0] = c1, buf[1] = c2, buf[2] = c3;
				break;
			case 2:
				for(; h > 0; h--, buf += pitch)
					*((uint16_t *)buf) = (uint16_t)pix;
				break;
		}
	}
}


//=============================
// 描画
//=============================


/** 全体を埋める */

void mPixbufFill(mPixbuf *p,mPixCol pix)
{
	uint8_t *pd;
	int i;
	
	//Y1行分を埋める
	
	mPixbufRawLineH(p, p->buf, p->w, pix);
	
	//残りは上1行を順にコピー
	
	pd = p->buf;
	
	for(i = p->h - 1; i > 0; i--, pd += p->pitch)
		memcpy(pd + p->pitch, pd, p->pitch);
}

/** 点を置く */

void mPixbufSetPixel(mPixbuf *p,int x,int y,mPixCol pix)
{
	x += M_PIXBUFBASE(p)->p.offsetX;
	y += M_PIXBUFBASE(p)->p.offsetY;

	if(mPixbufIsInClip_abs(p, x, y))
		(p->setbuf)(p->buftop + y * p->pitch_dir + x * p->bpp, pix);
}

/** 点を置く (RGB) */

void mPixbufSetPixelRGB(mPixbuf *p,int x,int y,mRgbCol rgb)
{
	x += M_PIXBUFBASE(p)->p.offsetX;
	y += M_PIXBUFBASE(p)->p.offsetY;

	if(mPixbufIsInClip_abs(p, x, y))
		(p->setbuf)(p->buftop + y * p->pitch_dir + x * p->bpp, mRGBtoPix(rgb));
}

/** 複数の点を置く */

void mPixbufSetPixels(mPixbuf *p,mPoint *pt,int ptcnt,mPixCol pix)
{
	int x,y;
	
	for(; ptcnt > 0; ptcnt--, pt++)
	{
		x = pt->x + M_PIXBUFBASE(p)->p.offsetX;
		y = pt->y + M_PIXBUFBASE(p)->p.offsetY;

		if(mPixbufIsInClip_abs(p, x, y))
			(p->setbuf)(p->buftop + y * p->pitch_dir + x * p->bpp, pix);
	}
}

/** RGB合成して点をセット
 *
 * @param col 下位は RGB。24bitより上位が不透明度 (0-128) */

void mPixbufSetPixel_blend128(mPixbuf *p,int x,int y,uint32_t col)
{
	uint8_t *pd;
	int r,g,b,a;

	x += M_PIXBUFBASE(p)->p.offsetX;
	y += M_PIXBUFBASE(p)->p.offsetY;

	if(!mPixbufIsInClip_abs(p, x, y)) return;

	if((pd = mPixbufGetBufPt_abs(p, x, y)))
	{
		mPixbufGetPixelBufRGB(p, pd, &r, &g, &b);

		a = col >> 24;
		r = ((M_GET_R(col) - r) * a >> 7) + r;
		g = ((M_GET_G(col) - g) * a >> 7) + g;
		b = ((M_GET_B(col) - b) * a >> 7) + b;
		
		(p->setbuf)(pd, mRGBtoPix2(r, g, b));
	}
}

/** RGB合成して点をセット (バッファ位置) */

void mPixbufSetPixelBuf_blend128(mPixbuf *p,uint8_t *buf,uint32_t col)
{
	int r,g,b,a;

	mPixbufGetPixelBufRGB(p, buf, &r, &g, &b);

	a = col >> 24;
	r = ((M_GET_R(col) - r) * a >> 7) + r;
	g = ((M_GET_G(col) - g) * a >> 7) + g;
	b = ((M_GET_B(col) - b) * a >> 7) + b;
	
	(p->setbuf)(buf, mRGBtoPix2(r, g, b));
}


//========================


/** 水平線描画 */

void mPixbufLineH(mPixbuf *p,int x,int y,int w,mPixCol pix)
{
	mRect rc;

	if(mPixbufGetClipRect_d(p, &rc, x, y, w, 1))
	{
		mPixbufRawLineH(p,
			mPixbufGetBufPtFast(p, rc.x1, rc.y1), rc.x2 - rc.x1 + 1, pix);
	}
}

/** 垂直線描画 */

void mPixbufLineV(mPixbuf *p,int x,int y,int h,mPixCol pix)
{
	mRect rc;

	if(mPixbufGetClipRect_d(p, &rc, x, y, 1, h))
	{
		mPixbufRawLineV(p,
			mPixbufGetBufPtFast(p, rc.x1, rc.y1), rc.y2 - rc.y1 + 1, pix);
	}
}

/** 水平線描画 (指定不透明度で合成)
 *
 * @param opacity 256 = 1.0 */

void mPixbufLineH_blend(mPixbuf *p,int x,int y,int w,mRgbCol rgb,int opacity)
{
	mRect rc;
	uint8_t *pd;
	int ix,r,g,b,sr,sg,sb;

	if(!mPixbufGetClipRect_d(p, &rc, x, y, w, 1))
		return;
	
	pd = mPixbufGetBufPtFast(p, rc.x1, rc.y1);

	sr = M_GET_R(rgb);
	sg = M_GET_G(rgb);
	sb = M_GET_B(rgb);

	for(ix = rc.x1; ix <= rc.x2; ix++, pd += p->bpp)
	{
		mPixbufGetPixelBufRGB(p, pd, &r, &g, &b);
	
		r = ((sr - r) * opacity >> 8) + r;
		g = ((sg - g) * opacity >> 8) + g;
		b = ((sb - b) * opacity >> 8) + b;
	
		(p->setbuf)(pd, mRGBtoPix2(r, g, b));
	}
}

/** 垂直線描画 (指定不透明度で合成) */

void mPixbufLineV_blend(mPixbuf *p,int x,int y,int h,mRgbCol rgb,int opacity)
{
	mRect rc;
	uint8_t *pd;
	int iy,r,g,b,sr,sg,sb;

	if(!mPixbufGetClipRect_d(p, &rc, x, y, 1, h))
		return;
	
	pd = mPixbufGetBufPtFast(p, rc.x1, rc.y1);

	sr = M_GET_R(rgb);
	sg = M_GET_G(rgb);
	sb = M_GET_B(rgb);

	for(iy = rc.y1; iy <= rc.y2; iy++, pd += p->pitch_dir)
	{
		mPixbufGetPixelBufRGB(p, pd, &r, &g, &b);
	
		r = ((sr - r) * opacity >> 8) + r;
		g = ((sg - g) * opacity >> 8) + g;
		b = ((sb - b) * opacity >> 8) + b;
	
		(p->setbuf)(pd, mRGBtoPix2(r, g, b));
	}
}

/** 直線描画 */

static void _draw_line(mPixbuf *p,int x1,int y1,int x2,int y2,mPixCol pix,mBool noend)
{
	int sx,sy,dx,dy,a,a1,e;

	if(x1 == x2 && y1 == y2 && !noend)
	{
		mPixbufSetPixel(p, x1, y1, pix);
		return;
	}

	//

	dx = (x1 < x2)? x2 - x1: x1 - x2;
	dy = (y1 < y2)? y2 - y1: y1 - y2;
	sx = (x1 <= x2)? 1: -1;
	sy = (y1 <= y2)? 1: -1;

	if(dx >= dy)
	{
		a  = 2 * dy;
		a1 = a - 2 * dx;
		e  = a - dx;

		while(x1 != x2)
		{
			mPixbufSetPixel(p, x1, y1, pix);

			if(e >= 0)
				y1 += sy, e += a1;
			else
				e += a;

			x1 += sx;
		}
	}
	else
	{
		a  = 2 * dx;
		a1 = a - 2 * dy;
		e  = a - dy;

		while(y1 != y2)
		{
			mPixbufSetPixel(p, x1, y1, pix);

			if(e >= 0)
				x1 += sx, e += a1;
			else
				e += a;

			y1 += sy;
		}
	}

	if(!noend) mPixbufSetPixel(p, x1, y1, pix);
}

/** 直線描画 */

void mPixbufLine(mPixbuf *p,int x1,int y1,int x2,int y2,mPixCol pix)
{
	_draw_line(p, x1, y1, x2, y2, pix, FALSE);
}

/** 直線描画 (終点を描画しない) */

void mPixbufLine_noend(mPixbuf *p,int x1,int y1,int x2,int y2,mPixCol pix)
{
	_draw_line(p, x1, y1, x2, y2, pix, TRUE);
}

/** 複数点をつなげた直線を描画
 *
 * [0]-[1], [1]-[2], ... [n-2]-[n-1]
 * 
 * @param num 点の数 */

void mPixbufLines(mPixbuf *p,mPoint *pt,int num,mPixCol pix)
{
	for(; num > 1; num--, pt++)
		mPixbufLine(p, pt->x, pt->y, pt[1].x, pt[1].y, pix);
}

/** 四角形枠 */

void mPixbufBox(mPixbuf *p,int x,int y,int w,int h,mPixCol pix)
{
	mRect rc;
	int ww,hh;

	if(!mPixbufGetClipRect_d(p, &rc, x, y, w, h)) return;
	
	ww = rc.x2 - rc.x1 + 1;
	hh = rc.y2 - rc.y1 + 1;

	//左
	
	if(rc.x1 == x)
		mPixbufRawLineV(p, mPixbufGetBufPtFast(p, rc.x1, rc.y1), hh, pix);
	
	//右
	
	if(rc.x2 == x + w - 1)
		mPixbufRawLineV(p, mPixbufGetBufPtFast(p, rc.x2, rc.y1), hh, pix);
	
	//上
		
	if(rc.y1 == y)
		mPixbufRawLineH(p, mPixbufGetBufPtFast(p, rc.x1, rc.y1), ww, pix);
	
	//下
	
	if(rc.y2 == y + h - 1)
		mPixbufRawLineH(p, mPixbufGetBufPtFast(p, rc.x1, rc.y2), ww, pix);
}

/** 四角形枠 (低速版)
 *
 * XOR で描画する場合はこちらを使う */

void mPixbufBoxSlow(mPixbuf *p,int x,int y,int w,int h,mPixCol pix)
{
	int i,n;

	//横線

	n = y + h - 1;

	for(i = 0; i < w; i++)
	{
		mPixbufSetPixel(p, x + i, y, pix);

		if(h > 1) mPixbufSetPixel(p, x + i, n, pix);
	}

	//縦線

	n = x + w - 1;

	for(i = 1; i < h - 1; i++)
	{
		mPixbufSetPixel(p, x, y + i, pix);

		if(w > 1) mPixbufSetPixel(p, n, y + i, pix);
	}
}

/** 四角形枠・破線 (1px) */

void mPixbufBoxDash(mPixbuf *p,int x,int y,int w,int h,mPixCol pix)
{
	int i,n,f = 1;
	
	//上
	
	for(i = 0; i < w; i++, f ^= 1)
	{
		if(f) mPixbufSetPixel(p, x + i, y, pix);
	}
	
	//右
	
	for(i = 1, n = x + w - 1; i < h; i++, f ^= 1)
	{
		if(f) mPixbufSetPixel(p, n, y + i, pix);
	}

	//下
	
	for(i = w - 2, n = y + h - 1; i >= 0; i--, f ^= 1)
	{
		if(f) mPixbufSetPixel(p, x + i, n, pix);
	}
	
	//左
	
	for(i = h - 2; i > 0; i--, f ^= 1)
	{
		if(f) mPixbufSetPixel(p, x, y + i, pix);
	}
}

/** 四角形塗りつぶし */

void mPixbufFillBox(mPixbuf *p,int x,int y,int w,int h,mPixCol pix)
{
	mBox box;
	uint8_t *pd;
	int i;
	
	if(!mPixbufGetClipBox_d(p, &box, x, y, w, h)) return;

	//Y1列分を塗りつぶし
	
	pd = mPixbufGetBufPtFast(p, box.x, box.y);

	mPixbufRawLineH(p, pd, box.w, pix);

	//以降、前の列をコピー

	w = box.w * p->bpp;

	for(i = box.h - 1; i > 0; i--, pd += p->pitch_dir)
		memcpy(pd + p->pitch_dir, pd, w);
}

/** 2x2 のパターンで四角形塗りつぶし */

void mPixbufFillBox_pat2x2(mPixbuf *p,int x,int y,int w,int h,mPixCol pix)
{
	mRect rc;
	uint8_t *pd;
	int ix,iy,pitch,fy;
	
	if(!mPixbufGetClipRect_d(p, &rc, x, y, w, h)) return;
	
	pd = mPixbufGetBufPtFast(p, rc.x1, rc.y1);
	pitch = p->pitch_dir - (rc.x2 - rc.x1 + 1) * p->bpp;

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		fy = iy & 1;
		
		for(ix = rc.x1; ix <= rc.x2; ix++, pd += p->bpp)
		{
			if(!((ix & 1) ^ fy))
				(p->setbuf)(pd, pix);
		}

		pd += pitch;
	}
}

/** 4x4の格子柄で四角形塗りつぶし (描画開始位置を原点とする) */

void mPixbufFillBox_checkers4x4_originx(mPixbuf *p,
	int x,int y,int w,int h,mPixCol pix1,mPixCol pix2)
{
	mBox box;
	int ix,iy,fy,pitch;
	uint8_t *pd;
	mPixCol col[2];
	
	if(!mPixbufGetClipBox_d(p, &box, x, y, w, h))
		return;

	pd = mPixbufGetBufPtFast(p, box.x, box.y);
	pitch = p->pitch_dir - box.w * p->bpp;

	col[0] = pix1, col[1] = pix2;

	for(iy = 0; iy < box.h; iy++)
	{
		fy = (iy & 7) >> 2;
	
		for(ix = 0; ix < box.w; ix++, pd += p->bpp)
			(p->setbuf)(pd, col[fy ^ ((ix & 7) >> 2)]);

		pd += pitch;
	}
}

/** 楕円 */

void mPixbufEllipse(mPixbuf *p,int x1,int y1,int x2,int y2,mPixCol pix)
{
	double dx,dy,e,e2;
	int a,b,b1;

	if(x1 == x2 && y1 == y2)
	{
		mPixbufSetPixel(p, x1, y1, pix);
		return;
	}

	a = (x1 < x2)? x2 - x1: x1 - x2;
	b = (y1 < y2)? y2 - y1: y1 - y2;

	if(a >= b)
	{
		//横長

		b1 = b & 1;
		dx = 4.0 * (1 - a) * b * b;
		dy = 4.0 * (b1 + 1) * a * a;
		e = dx + dy + b1 * a * a;

		if(x1 > x2) x1 = x2, x2 += a;
		if(y1 > y2) y1 = y2;

		y1 += (b + 1) / 2;
		y2 = y1 - b1;

		a *= 8 * a;
		b1 = 8 * b * b;

		do
		{
			mPixbufSetPixel(p, x2, y1, pix);
			if(x1 != x2) mPixbufSetPixel(p, x1, y1, pix);
			if(y1 != y2) mPixbufSetPixel(p, x1, y2, pix);
			if(x1 != x2 && y1 != y2) mPixbufSetPixel(p, x2, y2, pix);

			e2 = 2 * e;
			if(e2 <= dy) { y1++; y2--; e += dy += a; }
			if(e2 >= dx || 2 * e > dy) { x1++; x2--; e += dx += b1; }
		}while(x1 <= x2);
	}
	else
	{
		//縦長

		b1 = a & 1;
		dy = 4.0 * (1 - b) * a * a;
		dx = 4.0 * (b1 + 1) * b * b;
		e = dx + dy + b1 * b * b;

		if(y1 > y2) y1 = y2, y2 += b;
		if(x1 > x2) x1 = x2;

		x1 += (a + 1) / 2;
		x2 = x1 - b1;

		b *= 8 * b;
		b1 = 8 * a * a;

		do
		{
			mPixbufSetPixel(p, x2, y1, pix);
			if(x1 != x2) mPixbufSetPixel(p, x1, y1, pix);
			if(y1 != y2) mPixbufSetPixel(p, x1, y2, pix);
			if(x1 != x2 && y1 != y2) mPixbufSetPixel(p, x2, y2, pix);

			e2 = 2 * e;
			if(e2 <= dx) { x1++; x2--; e += dx += b; }
			if(e2 >= dy || 2 * e > dx) { y1++; y2--; e += dy += b1; }
		}while(y1 <= y2);
	}
}


//==========================
// 転送
//==========================


/** クリッピング */

mBool __mPixbufBltClip(mPixbufBltInfo *p,
	mPixbuf *dst,int dx,int dy,int sx,int sy,int w,int h)
{
	mBox box;

	//クリッピング範囲適用

	if(!mPixbufGetClipBox_d(dst, &box, dx, dy, w, h)) return FALSE;

	//左上がずれた分はソース位置もずらす

	sx += box.x - dx;
	sy += box.y - dy;

	//セット

	p->dx = box.x, p->dy = box.y;
	p->w = box.w, p->h = box.h;
	p->sx = sx, p->sy = sy;

	return TRUE;
}


/** mPixbuf から転送
 *
 * @param w,h 負の値で src の幅・高さ */

void mPixbufBlt(mPixbuf *dst,int dx,int dy,mPixbuf *src,int sx,int sy,int w,int h)
{
	mPixbufBltInfo info;
	int iy,size;
	uint8_t *ps,*pd;

	if(w < 0) w = src->w;
	if(h < 0) h = src->h;

	if(!__mPixbufBltClip(&info, dst, dx, dy, sx, sy, w, h)) return;

	pd = mPixbufGetBufPtFast(dst, info.dx, info.dy);
	ps = mPixbufGetBufPt(src, info.sx, info.sy);
	size = dst->bpp * info.w;

	for(iy = info.h; iy > 0; iy--)
	{
		memcpy(pd, ps, size);

		pd += dst->pitch_dir;
		ps += src->pitch_dir;
	}
}

/** グレイスケールのイメージデータから転送 */

void mPixbufBltFromGray(mPixbuf *p,int dx,int dy,
	const uint8_t *srcimg,int sx,int sy,int w,int h,int srcw)
{
	mPixbufBltInfo info;
	uint8_t *pd;
	const uint8_t *ps;
	int pitchs,pitchd,ix,iy,c;

	if(!__mPixbufBltClip(&info, p, dx, dy, sx, sy, w, h)) return;

	pd = mPixbufGetBufPtFast(p, info.dx, info.dy);
	ps = srcimg + info.sy * srcw + info.sx;
	pitchd = p->pitch_dir - info.w * p->bpp;
	pitchs = srcw - info.w;

	for(iy = info.h; iy; iy--)
	{
		for(ix = info.w; ix; ix--, pd += p->bpp)
		{
			c = *(ps++);
			
			(p->setbuf)(pd, mRGBtoPix2(c,c,c));
		}

		pd += pitchd;
		ps += pitchs;
	}
}

/** 1bit パターンから転送 */

void mPixbufBltFrom1bit(mPixbuf *p,int dx,int dy,
	const uint8_t *pat,int sx,int sy,int w,int h,int patw,mPixCol col_1,mPixCol col_0)
{
	mPixbufBltInfo info;
	uint8_t *pd,fleft,f;
	const uint8_t *ps,*psY;
	int ix,iy,pitchDst,pitchSrc;
	
	if(!__mPixbufBltClip(&info, p, dx, dy, sx, sy, w, h)) return;
	
	pd = mPixbufGetBufPtFast(p, info.dx, info.dy);

	pitchDst = p->pitch_dir - info.w * p->bpp;
	pitchSrc = (patw + 7) >> 3;

	psY = pat + info.sy * pitchSrc + (info.sx >> 3);
	fleft = 1 << (info.sx & 7);
	
	for(iy = info.h; iy; iy--, pd += pitchDst, psY += pitchSrc)
	{
		ps = psY;
		f = fleft;
	
		for(ix = info.w; ix; ix--, pd += p->bpp)
		{
			(p->setbuf)(pd, (*ps & f)? col_1: col_0);

			f <<= 1;
			if(!f) f = 1, ps++;
		}
	}
}

/** 2bit パターンから転送 (最大4色) */

void mPixbufBltFrom2bit(mPixbuf *p,int dx,int dy,
	const uint8_t *pat,int sx,int sy,int w,int h,int patw,mPixCol *col)
{
	mPixbufBltInfo info;
	uint8_t *pd;
	const uint8_t *ps,*ps_y;
	int ix,iy,pitchDst,pitchSrc,bit_left,bit;
	
	if(!__mPixbufBltClip(&info, p, dx, dy, sx, sy, w, h)) return;
	
	pd = mPixbufGetBufPtFast(p, info.dx, info.dy);

	pitchDst = p->pitch_dir - info.w * p->bpp;
	pitchSrc = (patw + 3) >> 2;

	ps_y = pat + info.sy * pitchSrc + (info.sx >> 2);
	bit_left = (info.sx & 3) << 1;
	
	for(iy = info.h; iy; iy--, pd += pitchDst, ps_y += pitchSrc)
	{
		ps = ps_y;
		bit = bit_left;
	
		for(ix = info.w; ix; ix--, pd += p->bpp)
		{
			(p->setbuf)(pd, col[(*ps >> bit) & 3]);
			
			bit += 2;
			if(bit == 8) { bit = 0; ps++; }
		}
	}
}


//==========================
// 図形描画
//==========================


/** 3D枠描画 */

void mPixbufDraw3DFrame(mPixbuf *p,int x,int y,int w,int h,mPixCol col_lt,mPixCol col_dk)
{
	mPixbufLineH(p, x, y, w, col_lt);
	mPixbufLineV(p, x, y + 1, h - 1, col_lt);

	mPixbufLineH(p, x + 1, y + h - 1, w - 1, col_dk);
	mPixbufLineV(p, x + w - 1, y + 1, h - 2, col_dk);
}

/** ビットパターンから描画
 *
 * ビット順は下位 -> 上位の順。\n
 * bit=1 は col で描画。bit=0 は描画しない。 */

void mPixbufDrawBitPattern(mPixbuf *p,int x,int y,
	const uint8_t *pat,int patw,int path,mPixCol col)
{
	mBox box;
	uint8_t *pd,f,fleft;
	const uint8_t *ps,*ps_y;
	int ix,iy,pitchDst,pitchSrc;
	
	if(!mPixbufGetClipBox_d(p, &box, x, y, patw, path)) return;
	
	pd = mPixbufGetBufPtFast(p, box.x, box.y);

	pitchDst = p->pitch_dir - box.w * p->bpp;
	pitchSrc = (patw + 7) >> 3;

	x = box.x - x;
	y = box.y - y;

	ps_y = pat + y * pitchSrc + (x >> 3);
	fleft = 1 << (x & 7);
	
	for(iy = box.h; iy; iy--, pd += pitchDst, ps_y += pitchSrc)
	{
		ps = ps_y;
		f  = fleft;
	
		for(ix = box.w; ix; ix--, pd += p->bpp)
		{
			if(*ps & f)
				(p->setbuf)(pd, col);
			
			f <<= 1;
			if(!f) f = 1, ps++;
		}
	}
}

/** 横にひと続きのビットパターンから指定番号のパターンを複数描画 (1bit)
 *
 * @param number 255 で終了 */

void mPixbufDrawBitPatternSum(mPixbuf *p,int x,int y,
	const uint8_t *pat,int patw,int path,int eachw,const uint8_t *number,mPixCol col)
{
	const uint8_t *ps,*psY;
	int ix,iy,pitchs,sx,f,fleft;

	pitchs = (patw + 7) >> 3;

	for(; *number != 255; x += eachw)
	{
		sx = *(number++) * eachw;
		if(sx >= patw) break;
		
		psY = pat + (sx >> 3);
		fleft = 1 << (sx & 7);

		for(iy = 0; iy < path; iy++, psY += pitchs)
		{
			for(ix = 0, ps = psY, f = fleft; ix < eachw; ix++)
			{
				if(*ps & f)
					mPixbufSetPixel(p, x + ix, y + iy, col);

				f <<= 1;
				if(f == 256) f = 1, ps++;
			}
		}
	}
}

/** 2bit のイメージパターンから描画
 *
 * @param pcol 4色分の色。全ビットが 1 で透過。 */

void mPixbufDraw2bitPattern(mPixbuf *p,int x,int y,const uint8_t *pat,int patw,int path,mPixCol *pcol)
{
	mPixbufBltInfo info;
	uint8_t *pd;
	const uint8_t *ps,*ps_y;
	int ix,iy,pitchDst,pitchSrc,bit_left,bit;
	mPixCol col;
	
	if(!__mPixbufBltClip(&info, p, x, y, 0, 0, patw, path)) return;
	
	pd = mPixbufGetBufPtFast(p, info.dx, info.dy);

	pitchDst = p->pitch_dir - info.w * p->bpp;
	pitchSrc = (patw + 3) >> 2;

	ps_y = pat + info.sy * pitchSrc + (info.sx >> 2);
	bit_left = (info.sx & 3) << 1;
	
	for(iy = info.h; iy; iy--, pd += pitchDst, ps_y += pitchSrc)
	{
		ps = ps_y;
		bit = bit_left;
	
		for(ix = info.w; ix; ix--, pd += p->bpp)
		{
			col = pcol[(*ps >> bit) & 3];

			if(col != (mPixCol)-1)
				(p->setbuf)(pd, col);
			
			bit += 2;
			if(bit == 8) { bit = 0; ps++; }
		}
	}
}

/** 横にひと続きのビットパターンから指定番号のパターンを複数描画 (2bit)
 *
 * @param number 255 で終了
 * @param pcol  全ビットONで透過 */

void mPixbufDraw2BitPatternSum(mPixbuf *p,int x,int y,
	const uint8_t *pat,int patw,int path,int eachw,const uint8_t *number,mPixCol *pcol)
{
	const uint8_t *ps,*psY;
	int ix,iy,pitchs,sx,bitleft,bit;
	mPixCol col;

	pitchs = (patw + 3) >> 2;

	for(; *number != 255; x += eachw)
	{
		sx = *(number++) * eachw;
		if(sx >= patw) break;

		//
		
		psY = pat + (sx >> 2);
		bitleft = (sx & 3) << 1;

		for(iy = 0; iy < path; iy++, psY += pitchs)
		{
			ps = psY;
			bit = bitleft;
			
			for(ix = 0; ix < eachw; ix++)
			{
				col = pcol[(*ps >> bit) & 3];

				if(col != (mPixCol)-1)
					mPixbufSetPixel(p, x + ix, y + iy, col);

				bit += 2;
				if(bit == 8) { bit = 0; ps++; }
			}
		}
	}
}

/** 数字とドットをテキストから描画
 *
 * 数字は 5x7。ドットは 3x7。 */

void mPixbufDrawNumber_5x7(mPixbuf *p,int x,int y,const char *text,mPixCol col)
{
	uint8_t pat[42] = {
		0x6f,0xff,0xf9,0xff,0xff,0x00, 0x49,0x88,0x19,0x91,0x99,0x00,
		0x49,0x88,0x19,0x91,0x99,0x00, 0x49,0xff,0xff,0x8f,0xff,0x00,
		0x49,0x81,0x88,0x89,0x89,0x00, 0x49,0x81,0x88,0x89,0x89,0x03,
		0xef,0xff,0xf8,0x8f,0x8f,0x03 };
	int w,sx,ix,iy;
	uint8_t f,fleft,*ps,*psY;

	for(; *text; text++)
	{
		if(*text == '.')
			sx = 4 * 10, w = 2;
		else if(*text >= '0' && *text <= '9')
			sx = (*text - '0') * 4, w = 4;
		else
			continue;

		//

		psY = pat + (sx >> 3);
		fleft = 1 << (sx & 7);

		for(iy = 0; iy < 7; iy++)
		{
			ps = psY;
			f = fleft;
			
			for(ix = 0; ix < w; ix++)
			{
				if(*ps & f)
					mPixbufSetPixel(p, x + ix, y + iy, col);

				f <<= 1;
				if(f == 0) f = 1, ps++;
			}

			psY += 6;
		}

		x += w + 1;
	}
}

/** チェック描画 (10x9) */

void mPixbufDrawChecked(mPixbuf *p,int x,int y,mPixCol col)
{
	uint8_t pat[] = {
		0x00,0x02,0x00,0x03,0x80,0x03,0xc3,0x03,0xe7,0x01,0xff,0x00,0x7e,0x00,0x3c,0x00,0x18,0x00
	};
	
	mPixbufDrawBitPattern(p, x, y, pat, 10, 9, col);
}

/** メニュー用チェック描画 (8x7) */

void mPixbufDrawMenuChecked(mPixbuf *p,int x,int y,mPixCol col)
{
	uint8_t pat[] = { 0x80,0xc0,0x60,0x31,0x1b,0x0e,0x04 };
	
	mPixbufDrawBitPattern(p, x, y, pat, 8, 7, col);
}

/** メニュー用ラジオ描画 (5x5) */

void mPixbufDrawMenuRadio(mPixbuf *p,int x,int y,mPixCol col)
{
	uint8_t pat[] = { 0x0e,0x1f,0x1f,0x1f,0x0e };
	
	mPixbufDrawBitPattern(p, x, y, pat, 5, 5, col);
}

/** ボタン描画 */

void mPixbufDrawButton(mPixbuf *p,int x,int y,int w,int h,uint32_t flags)
{
	mPixCol col;

	//背景

	mPixbufFillBox(p, x, y, w, h,
		(flags & MPIXBUF_DRAWBTT_FOCUS)? MSYSCOL(FACE_FOCUS): MSYSCOL(FACE_DARK));

	//枠色

	if(flags & MPIXBUF_DRAWBTT_DISABLE)
		col = MSYSCOL(FRAME_LIGHT);
	else if(flags & (MPIXBUF_DRAWBTT_DEFAULT_BUTTON | MPIXBUF_DRAWBTT_FOCUS))
		col = MSYSCOL(FRAME_FOCUS);
	else
		col = MSYSCOL(FRAME);

	//枠とフォーカス

	if(flags & MPIXBUF_DRAWBTT_PRESS)
	{
		mPixbufLineH(p, x, y, w, col);
		mPixbufLineV(p, x, y + 1, h - 1, col);
	}
	else
	{
		mPixbufBox(p, x, y, w, h, col);

		if(flags & MPIXBUF_DRAWBTT_FOCUS)
			mPixbufBoxDash(p, x + 2, y + 2, w - 4, h - 4, MSYSCOL(TEXT));
	}
}

/** チェックボックス描画 (13x13) */

void mPixbufDrawCheckBox(mPixbuf *p,int x,int y,uint32_t flags)
{
	mPixCol colframe,col;
	mPoint pt[4];

	colframe = (flags & MPIXBUF_DRAWCKBOX_DISABLE)?
			MSYSCOL(FRAME_LIGHT): MSYSCOL(FRAME_DARK);

	//背景
	
	mPixbufFillBox(p, x + 1, y + 1, 11, 11, MSYSCOL(FACE_LIGHTEST));

	//
	
	col = (flags & MPIXBUF_DRAWCKBOX_DISABLE)? MSYSCOL(TEXT_DISABLE): MSYSCOL(TEXT);

	if(flags & MPIXBUF_DRAWCKBOX_RADIO)
	{
		//------ ラジオ
		
		//枠

		mPixbufLineH(p, x + 2, y, 9, colframe);
		mPixbufLineH(p, x + 2, y + 12, 9, colframe);
		mPixbufLineV(p, x, y + 2, 9, colframe);
		mPixbufLineV(p, x + 12, y + 2, 9, colframe);
		
		pt[0].x = x + 1;  pt[0].y = y + 1;
		pt[1].x = x + 11; pt[1].y = y + 1;
		pt[2].x = x + 11; pt[2].y = y + 11;
		pt[3].x = x + 1;  pt[3].y = y + 11;
		
		mPixbufSetPixels(p, pt, 4, colframe);
		
		//
		
		if(flags & MPIXBUF_DRAWCKBOX_CHECKED)
		{
			mPixbufFillBox(p, x + 2, y + 3, 9, 7, col);
			mPixbufLineH(p, x + 3, y + 2, 7, col);
			mPixbufLineH(p, x + 3, y + 10, 7, col);
		}
	}
	else
	{
		//----- 通常
	
		mPixbufBox(p, x, y, 13, 13, colframe);

		if(flags & MPIXBUF_DRAWCKBOX_CHECKED)
			mPixbufDrawChecked(p, x + 2, y + 2, col);
	}
}

/** 下矢印描画 (5x3) */

void mPixbufDrawArrowDown(mPixbuf *p,int ctx,int cty,mPixCol col)
{
	uint8_t pat[] = { 0x1f,0x0e,0x04 };
	
	mPixbufDrawBitPattern(p, ctx - 2, cty - 1, pat, 5, 3, col);
}

/** 上矢印描画 (5x3) */

void mPixbufDrawArrowUp(mPixbuf *p,int ctx,int cty,mPixCol col)
{
	uint8_t pat[] = { 0x04,0x0e,0x1f };
	
	mPixbufDrawBitPattern(p, ctx - 2, cty - 1, pat, 5, 3, col);
}

/** 左矢印描画 (3x5) */

void mPixbufDrawArrowLeft(mPixbuf *p,int ctx,int cty,mPixCol col)
{
	uint8_t pat[] = { 0x04,0x06,0x07,0x06,0x04 };
	
	mPixbufDrawBitPattern(p, ctx - 1, cty - 2, pat, 3, 5, col);
}

/** 右矢印描画 (3x5) */

void mPixbufDrawArrowRight(mPixbuf *p,int ctx,int cty,mPixCol col)
{
	uint8_t pat[] = { 0x01,0x03,0x07,0x03,0x01 };
	
	mPixbufDrawBitPattern(p, ctx - 1, cty - 2, pat, 3, 5, col);
}

/** 下矢印(小)描画 (3x2)*/

void mPixbufDrawArrowDown_small(mPixbuf *p,int ctx,int y,mPixCol col)
{
	uint8_t pat[] = {0x07,0x02};

	mPixbufDrawBitPattern(p, ctx - 1, y, pat, 3, 2, col);
}

/** 上矢印(小)描画 (3x2)*/

void mPixbufDrawArrowUp_small(mPixbuf *p,int ctx,int y,mPixCol col)
{
	uint8_t pat[] = {0x02,0x07};

	mPixbufDrawBitPattern(p, ctx - 1, y, pat, 3, 2, col);
}

/** 下矢印描画 (7x4) */

void mPixbufDrawArrowDown_7x4(mPixbuf *p,int x,int y,mPixCol col)
{
	uint8_t pat[] = { 0x7f,0x3e,0x1c,0x08 };
	
	mPixbufDrawBitPattern(p, x, y, pat, 7, 4, col);
}

/** 右矢印描画 (4x7) */

void mPixbufDrawArrowRight_4x7(mPixbuf *p,int x,int y,mPixCol col)
{
	uint8_t pat[] = { 0x01,0x03,0x07,0x0f,0x07,0x03,0x01 };
	
	mPixbufDrawBitPattern(p, x, y, pat, 4, 7, col);
}

/** 下矢印描画 (13x7) */

void mPixbufDrawArrowDown_13x7(mPixbuf *p,int ctx,int cty,mPixCol col)
{
	uint8_t pat[] = { 0xff,0x1f,0xfe,0x0f,0xfc,0x07,0xf8,0x03,0xf0,0x01,0xe0,0x00,0x40,0x00 };
	
	mPixbufDrawBitPattern(p, ctx - 6, cty - 3, pat, 13, 7, col);
}

/** 下矢印描画 (13x7) */

void mPixbufDrawArrowUp_13x7(mPixbuf *p,int ctx,int cty,mPixCol col)
{
	uint8_t pat[] = { 0x40,0x00,0xe0,0x00,0xf0,0x01,0xf8,0x03,0xfc,0x07,0xfe,0x0f,0xff,0x1f };

	mPixbufDrawBitPattern(p, ctx - 6, cty - 3, pat, 13, 7, col);
}

/** 左矢印描画 (7x13) */

void mPixbufDrawArrowLeft_7x13(mPixbuf *p,int ctx,int cty,mPixCol col)
{
	uint8_t pat[] = { 0x40,0x60,0x70,0x78,0x7c,0x7e,0x7f,0x7e,0x7c,0x78,0x70,0x60,0x40 };

	mPixbufDrawBitPattern(p, ctx - 3, cty - 6, pat, 7, 13, col);
}

/** 右矢印描画 (7x13) */

void mPixbufDrawArrowRight_7x13(mPixbuf *p,int ctx,int cty,mPixCol col)
{
	uint8_t pat[] = { 0x01,0x03,0x07,0x0f,0x1f,0x3f,0x7f,0x3f,0x1f,0x0f,0x07,0x03,0x01 };

	mPixbufDrawBitPattern(p, ctx - 3, cty - 6, pat, 7, 13, col);
}

/** @} */
