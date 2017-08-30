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
 * 編集関連
 *****************************************/

#include <string.h>
#include <math.h>

#include "mDef.h"
#include "mRectBox.h"
#include "mPixbuf.h"
#include "mPopupProgress.h"

#include "defTileImage.h"
#include "TileImage.h"
#include "TileImage_pv.h"
#include "TileImage_coltype.h"
#include "TileImageDrawInfo.h"

#include "defCanvasInfo.h"
#include "ImageBuf8.h"
#include "ImageBufRGB16.h"
#include "blendcol.h"



/** タイルタイプ変換
 *
 * @param bLumtoAlpha  色の輝度をアルファ値へ変換 */

void TileImage_convertType(TileImage *p,int newtype,mBool bLumtoAlpha)
{
	uint8_t *tilebuf,**pp;
	uint32_t i;
	int srctype;
	TileImageColFunc_getTileRGBA getTileFunc;
	TileImageColFunc_setTileFromRGBA setTileFunc;
	TileImageColFunc_isTransparentTile isTransparentFunc;

	//作業用タイルバッファ

	tilebuf = (uint8_t *)mMalloc(64 * 64 * 8, FALSE);
	if(!tilebuf) return;

	//タイプ変更

	srctype = p->coltype;

	p->coltype = newtype;

	__TileImage_setTileSize(p);

	//タイル変換

	getTileFunc = g_tileimage_funcs[srctype].getTileRGBA;
	setTileFunc = g_tileimage_funcs[newtype].setTileFromRGBA;
	isTransparentFunc = g_tileimage_funcs[newtype].isTransparentTile;

	pp = p->ppbuf;

	for(i = p->tilew * p->tileh; i; i--, pp++)
	{
		if(*pp)
		{
			//RGBA タイルとして取得
			
			(getTileFunc)(p, tilebuf, *pp);

			//タイル再確保

			mFree(*pp);

			*pp = TileImage_allocTile(p, FALSE);
			if(!(*pp))
			{
				mFree(tilebuf);
				return;
			}

			//RGBA データからセット

			(setTileFunc)(*pp, tilebuf, bLumtoAlpha);

			//すべて透明なら解放

			if((isTransparentFunc)(*pp))
				TileImage_freeTile(pp);
		}
	}

	mFree(tilebuf);
}

/** イメージ結合 */

void TileImage_combine(TileImage *dst,TileImage *src,mRect *rc,int opasrc,int opadst,
	int blendmode,ImageBuf8 *src_texture,mPopupProgress *prog)
{
	int ix,iy,dstrawa;
	RGBAFix15 colsrc,coldst;
	RGBFix15 bsrc,bdst;
	BlendColorFunc blendfunc;

	//配列リサイズ

	if(!TileImage_resizeTileBuf_combine(dst, src)) return;

	//

	blendfunc = g_blendcolfuncs[blendmode];

	mPopupProgressThreadBeginSubStep_onestep(prog, 20, rc->y2 - rc->y1 + 1);

	for(iy = rc->y1; iy <= rc->y2; iy++)
	{
		for(ix = rc->x1; ix <= rc->x2; ix++)
		{
			//----- src 色
			
			TileImage_getPixel(src, ix, iy, &colsrc);

			//テクスチャ適用
			if(src_texture)
				colsrc.a = colsrc.a * ImageBuf8_getPixel_forTexture(src_texture, ix, iy) / 255;

			colsrc.a = colsrc.a * opasrc >> 7;

			//------ dst

			TileImage_getPixel(dst, ix, iy, &coldst);

			dstrawa = coldst.a;

			//両方透明なら処理なし
			
			if(colsrc.a == 0 && coldst.a == 0) continue;

			coldst.a = coldst.a * opadst >> 7;

			//色合成

			if(blendmode)
			{
				bsrc = *((RGBFix15 *)&colsrc);
			
				if(coldst.a == 0)
					bdst.r = bdst.g = bdst.b = 0x8000;
				else
					bdst = *((RGBFix15 *)&coldst);

				(blendfunc)(&bsrc, &bdst);

				*((RGBFix15 *)&colsrc) = bsrc;
			}

			//アルファ合成 => coldst

			TileImage_colfunc_normal(dst, &coldst, &colsrc, NULL);

			//セット
			/* 結合前と結合後がどちらも透明ならそのまま。
			 * opadst の値によっては結合後に透明になる場合があるので、
			 * [結合前=不透明、結合後=透明] なら色をセットしなければならない。 */

			if(coldst.a || dstrawa)
				TileImage_setPixel_new(dst, ix, iy, &coldst);
		}

		mPopupProgressThreadIncSubStep(prog);
	}

	//透明タイルを解放

	TileImage_freeEmptyTiles(dst);
}

/** ImageBufRGB16 からイメージをセット (画像統合時)
 *
 * - p は src と同じサイズで作成済み。
 * - p のタイプは RGBA で固定。
 * - 全体が不透明となる。 */

void TileImage_setImage_fromImageBufRGB16(TileImage *p,
	ImageBufRGB16 *src,mPopupProgress *prog)
{
	uint8_t **pptile;
	int ix,iy;

	mPopupProgressThreadBeginSubStep_onestep(prog, 20, p->tileh);

	pptile = p->ppbuf;

	for(iy = 0; iy < p->tileh; iy++)
	{
		for(ix = 0; ix < p->tilew; ix++, pptile++)
		{
			*pptile = TileImage_allocTile(p, FALSE);
			if(!(*pptile)) return;
		
			ImageBufRGB16_getTileRGBA(src, (RGBAFix15 *)*pptile, ix << 6, iy << 6);
		}

		mPopupProgressThreadIncSubStep(prog);
	}
}

/** 色置き換え (透明色 -> 指定色) */

void TileImage_replaceColor_fromTP(TileImage *p,RGBAFix15 *pixdst)
{
	mRect rc;
	int ix,iy;

	TileImage_getCanDrawRect_pixel(p, &rc);

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		for(ix = rc.x1; ix <= rc.x2; ix++)
		{
			if(TileImage_isPixelTransparent(p, ix, iy))
				TileImage_setPixel_draw_direct(p, ix, iy, pixdst);
		}
	}
}

/** 色置き換え (不透明 -> 指定色 or 透明) */

void TileImage_replaceColor_fromNotTP(TileImage *p,RGBAFix15 *src,RGBAFix15 *dst)
{
	int ix,iy;
	mRect rc;
	RGBAFix15 pix,pixdst;
	TileImageColFunc_isSameColor compfunc = g_tileimage_funcs[p->coltype].isSameRGB;

	pixdst = *dst;

	TileImage_getCanDrawRect_pixel(p, &rc);

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		for(ix = rc.x1; ix <= rc.x2; ix++)
		{
			TileImage_getPixel(p, ix, iy, &pix);

			//不透明で、RGB が src と同じ色なら置換え

			if(pix.a && (compfunc)(src, &pix))
			{
				//置換え先が透明でない場合は、元のアルファ値を維持
				
				if(pixdst.a)
					pixdst.a = pix.a;
				
				TileImage_setPixel_draw_direct(p, ix, iy, &pixdst);
			}
		}
	}
}


//===============================
// イメージ全体の編集
//===============================


/** キャンバスサイズ変更で範囲外を切り取る場合の、切り取り後のイメージ作成 */

TileImage *TileImage_createCropImage(TileImage *src,int offx,int offy,int w,int h)
{
	TileImage *dst;
	int ix,iy;
	RGBAFix15 pix;

	//作成

	dst = TileImage_new(src->coltype, w, h);
	if(!dst) return NULL;

	dst->rgb = src->rgb;

	//コピー

	for(iy = 0; iy < h; iy++)
	{
		for(ix = 0; ix < w; ix++)
		{
			TileImage_getPixel(src, ix - offx, iy - offy, &pix);

			if(pix.a)
				TileImage_setPixel_new(dst, ix, iy, &pix);
		}
	}

	return dst;
}

/** 全体を左右反転 */

void TileImage_fullReverse_horz(TileImage *p)
{
	int ix,iy,cnt;
	uint8_t **pp,**pp2,*ptmp;
	TileImageColFunc_editTile revfunc;

	//offset

	p->offx = (g_tileimage_dinfo.imgw - 1) - p->offx - (p->tilew * 64 - 1);

	//各タイルのイメージを左右反転

	revfunc = g_tileimage_funcs[p->coltype].reverseHorz;

	pp = p->ppbuf;

	for(ix = p->tilew * p->tileh; ix; ix--, pp++)
	{
		if(*pp)
			(revfunc)(*pp);
	}

	//タイル配列を左右反転

	cnt = p->tilew >> 1;
	pp  = p->ppbuf;
	pp2 = pp + p->tilew - 1;

	for(iy = p->tileh; iy; iy--)
	{
		for(ix = cnt; ix; ix--, pp++, pp2--)
		{
			ptmp = *pp;
			*pp  = *pp2;
			*pp2 = ptmp;
		}

		pp  += p->tilew - cnt;
		pp2 += p->tilew + cnt;
	}
}

/** 全体を上下反転 */

void TileImage_fullReverse_vert(TileImage *p)
{
	int ix,iy,ww;
	uint8_t **pp,**pp2,*ptmp;
	TileImageColFunc_editTile revfunc;

	//offset

	p->offy = (g_tileimage_dinfo.imgh - 1) - p->offy - (p->tileh * 64 - 1);

	//各タイルを上下反転

	revfunc = g_tileimage_funcs[p->coltype].reverseVert;

	pp = p->ppbuf;

	for(ix = p->tilew * p->tileh; ix; ix--, pp++)
	{
		if(*pp)
			(revfunc)(*pp);
	}

	//タイル配列を上下反転

	ww = p->tilew << 1;
	pp  = p->ppbuf;
	pp2 = pp + (p->tileh - 1) * p->tilew;

	for(iy = p->tileh >> 1; iy; iy--)
	{
		for(ix = p->tilew; ix; ix--, pp++, pp2++)
		{
			ptmp = *pp;
			*pp  = *pp2;
			*pp2 = ptmp;
		}

		pp2 -= ww;
	}
}

/** 全体を左に90度回転 */

void TileImage_fullRotate_left(TileImage *p)
{
	uint8_t **ppnew,**pp,**pp2;
	int ix,iy,n;
	TileImageColFunc_editTile rotatefunc;

	//新規配列

	ppnew = __TileImage_allocTileBuf_new(p->tileh, p->tilew, FALSE);
	if(!ppnew) return;

	//オフセット

	ix = g_tileimage_dinfo.imgw >> 1;
	iy = g_tileimage_dinfo.imgh >> 1;
	n = p->offx;

	p->offx = p->offy - iy + ix;
	p->offy = iy - (n + p->tilew * 64 - 1 - ix);

	//各タイルを回転

	rotatefunc = g_tileimage_funcs[p->coltype].rotateLeft;

	pp = p->ppbuf;

	for(ix = p->tilew * p->tileh; ix; ix--, pp++)
	{
		if(*pp)
			(rotatefunc)(*pp);
	}

	//配列を回転して配置

	pp = ppnew;
	pp2 = p->ppbuf + p->tilew - 1;
	n = p->tilew * p->tileh + 1;

	for(iy = p->tilew; iy; iy--, pp2 -= n)
	{
		for(ix = p->tileh; ix; ix--, pp2 += p->tilew)
			*(pp++) = *pp2;
	}

	//

	mFree(p->ppbuf);

	n = p->tilew;

	p->ppbuf = ppnew;
	p->tilew = p->tileh;
	p->tileh = n;
}

/** 全体を右に90度回転 */

void TileImage_fullRotate_right(TileImage *p)
{
	uint8_t **ppnew,**pp,**pp2;
	int ix,iy,n;
	TileImageColFunc_editTile rotatefunc;

	//新規配列

	ppnew = __TileImage_allocTileBuf_new(p->tileh, p->tilew, FALSE);
	if(!ppnew) return;

	//オフセット

	ix = g_tileimage_dinfo.imgw >> 1;
	iy = g_tileimage_dinfo.imgh >> 1;
	n = p->offx;

	p->offx = ix - (p->offy + p->tileh * 64 - 1 - iy);
	p->offy = n - ix + iy;

	//各タイルを回転

	rotatefunc = g_tileimage_funcs[p->coltype].rotateRight;

	pp = p->ppbuf;

	for(ix = p->tilew * p->tileh; ix; ix--, pp++)
	{
		if(*pp)
			(rotatefunc)(*pp);
	}

	//配列を回転して配置

	pp = ppnew;
	pp2 = p->ppbuf + (p->tileh - 1) * p->tilew;
	n = p->tilew * p->tileh + 1;

	for(iy = p->tilew; iy; iy--, pp2 += n)
	{
		for(ix = p->tileh; ix; ix--, pp2 -= p->tilew)
			*(pp++) = *pp2;
	}

	//

	mFree(p->ppbuf);

	n = p->tilew;

	p->ppbuf = ppnew;
	p->tilew = p->tileh;
	p->tileh = n;
}


//=========================
// スタンプ
//=========================
/*
 * スタンプ用イメージにおいて、
 * 選択範囲外の色はすべて 0、
 * 範囲内でかつ透明の場合は a=0, r=0x8000。
 *
 * [!] 貼付け時に <上書き> で範囲内に透明部分が含まれる場合は、それも上書きしなければならないため、
 *     範囲外の透明と範囲内の透明は区別できるようにする必要がある。
 */


/** スタンプ用イメージ作成
 *
 * src の rc 範囲のイメージの中で sel の選択範囲に点がある部分をコピー。
 * 結果のイメージは (0,0) から rc の幅と高さの範囲に描画。 */

TileImage *TileImage_createStampImage(TileImage *src,TileImage *sel,mRect *rcsrc)
{
	TileImage *dst;
	int ix,iy;
	mRect rc;
	RGBAFix15 pix;

	rc = *rcsrc;

	//作成

	dst = TileImage_new(src->coltype, rc.x2 - rc.x1 + 1, rc.y2 - rc.y1 + 1);
	if(!dst) return NULL;

	dst->rgb = src->rgb;

	//コピー

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		for(ix = rc.x1; ix <= rc.x2; ix++)
		{
			if(TileImage_isPixelOpaque(sel, ix, iy))
			{
				TileImage_getPixel(src, ix, iy, &pix);

				//範囲内でかつ透明の場合
				
				if(pix.a == 0) pix.r = 0x8000;

				TileImage_setPixel_new(dst, ix - rc.x1, iy - rc.y1, &pix);
			}
		}
	}

	return dst;
}

/** スタンプイメージの貼り付け */

void TileImage_pasteStampImage(TileImage *dst,int x,int y,
	TileImage *src,int srcw,int srch)
{
	int ix,iy;
	RGBAFix15 pix;

	for(iy = 0; iy < srch; iy++)
	{
		for(ix = 0; ix < srcw; ix++)
		{
			TileImage_getPixel(src, ix, iy, &pix);

			//値がすべて 0 なら選択範囲外

			if(pix.v64)
			{
				//a=0,r=0x8000 は範囲内で透明

				if(pix.a == 0) pix.r = 0;
				
				TileImage_setPixel_draw_direct(dst, x + ix, y + iy, &pix);
			}
		}
	}
}


//=====================================
// 選択範囲 編集用
//=====================================
/*
 * [ドラッグでの移動/コピー時]
 *   上書き時用に、選択範囲外は v64 = 0。範囲内で透明は a = 0, r = 0x8000。
 */


/** 選択範囲内のイメージをコピー/切り取りして、作成
 *
 * [ ドラッグでの移動/コピー ] [ 選択範囲コマンドのコピー ]
 * 色関数は <上書き> をセットしておく。
 *
 * @param cut  src の範囲内を透明に
 * @param for_move  移動/コピー用。選択範囲内外の透明色を区別できるようにするか。 */

TileImage *TileImage_createSelCopyImage(TileImage *src,TileImage *sel,
	mRect *rcarea,mBool cut,mBool for_move)
{
	TileImage *dst;
	int ix,iy;
	RGBAFix15 pix,pixe;
	mRect rc;

	//作成

	dst = TileImage_newFromRect(src->coltype, rcarea);
	if(!dst) return NULL;

	dst->rgb = src->rgb;

	//

	rc = *rcarea;
	pixe.v64 = 0;

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		for(ix = rc.x1; ix <= rc.x2; ix++)
		{
			if(TileImage_isPixelOpaque(sel, ix, iy))
			{
				//コピー
				
				TileImage_getPixel(src, ix, iy, &pix);

				if(for_move)
				{
					if(pix.a == 0) pix.r = 0x8000;
				
					TileImage_setPixel_new(dst, ix, iy, &pix);
				}
				else
				{
					//不透明部分のみセット
					
					if(pix.a)
						TileImage_setPixel_new(dst, ix, iy, &pix);
				}

				//消去

				if(cut && pix.a)
					TileImage_setPixel_draw_direct(src, ix, iy, &pixe);
			}
		}
	}

	return dst;
}

/** イメージの貼り付け
 *
 * [ ドラッグでの選択範囲移動/コピー ] */

void TileImage_pasteSelImage(TileImage *dst,TileImage *src,mRect *rcarea)
{
	int ix,iy;
	RGBAFix15 pix;
	mRect rc;

	rc = *rcarea;

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		for(ix = rc.x1; ix <= rc.x2; ix++)
		{
			TileImage_getPixel(src, ix, iy, &pix);

			//pix.v64 == 0 は選択範囲外

			if(pix.v64)
			{
				if(pix.a == 0) pix.r = 0;
			
				TileImage_setPixel_draw_direct(dst, ix, iy, &pix);
			}
		}
	}
}


//================================
// 矩形編集
//================================


/** 矩形範囲内のイメージをコピーして作成
 *
 * 作成されるイメージは、(0,0)-(w x h)。
 * 左右反転、上下反転用。
 * 
 * @param clear  元イメージの範囲を透明にする */

TileImage *TileImage_createCopyImage_box(TileImage *src,mBox *box,mBool clear)
{
	TileImage *dst;
	int ix,iy,x,y,w,h;
	RGBAFix15 pix,pixe;
	TileImageSetPixelFunc setpix = g_tileimage_dinfo.funcDrawPixel;

	w = box->w;
	h = box->h;

	pixe.v64 = 0;

	//作成

	dst = TileImage_new(src->coltype, w, h);
	if(!dst) return NULL;

	dst->rgb = src->rgb;

	//コピー

	for(iy = 0, y = box->y; iy < h; iy++, y++)
	{
		for(ix = 0, x = box->x; ix < w; ix++, x++)
		{
			TileImage_getPixel(src, x, y, &pix);

			if(pix.a)
			{
				TileImage_setPixel_new(dst, ix, iy, &pix);

				if(clear) (setpix)(src, x, y, &pixe);
			}
		}
	}

	return dst;
}

/** 矩形内のイメージをコピー (選択範囲外は透明に) & 範囲内消去
 *
 * 矩形編集の変形時
 *
 * @param sel  NULL の場合あり */

TileImage *TileImage_createCopyImage_forTransform(TileImage *src,TileImage *sel,mBox *box)
{
	TileImage *dst;
	int w,h,ix,iy,sx,sy;
	RGBAFix15 pix,pixe;

	w = box->w;
	h = box->h;

	//作成

	dst = TileImage_new(src->coltype, w, h);
	if(!dst) return NULL;

	dst->rgb = src->rgb;

	//

	pixe.v64 = 0;

	for(iy = 0, sy = box->y; iy < h; iy++, sy++)
	{
		for(ix = 0, sx = box->x; ix < w; ix++, sx++)
		{
			if(!sel || TileImage_isPixelOpaque(sel, sx, sy))
			{
				TileImage_getPixel(src, sx, sy, &pix);

				if(pix.a)
				{
					//コピー
					
					TileImage_setPixel_new(dst, ix, iy, &pix);

					//消去

					TileImage_setPixel_new(src, sx, sy, &pixe);
				}
			}
		}
	}

	return dst;
}

/** 変形の実行前に変形元の範囲内を消去 */

void TileImage_clearImageBox_forTransform(TileImage *p,TileImage *sel,mBox *box)
{
	int ix,iy;
	mRect rc;
	RGBAFix15 pixe;

	mRectSetByBox(&rc, box);

	pixe.v64 = 0;

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		for(ix = rc.x1; ix <= rc.x2; ix++)
		{
			if(!sel || TileImage_isPixelOpaque(sel, ix, iy))
				TileImage_setPixel_draw_direct(p, ix, iy, &pixe);
		}
	}
}

/** src の (0,0)-(w x h) のイメージを dst の (x,y)-(w x h) に戻す
 *
 * 変形ダイアログの終了時 */

void TileImage_restoreCopyImage_box(TileImage *dst,TileImage *src,mBox *boxdst)
{
	int ix,iy;
	mBox box;
	RGBAFix15 pix;

	box = *boxdst;

	for(iy = 0; iy < box.h; iy++)
	{
		for(ix = 0; ix < box.w; ix++)
		{
			TileImage_getPixel(src, ix, iy, &pix);

			if(pix.a)
				TileImage_setPixel_new(dst, box.x + ix, box.y + iy, &pix);
		}
	}
}

/** 矩形範囲の左右反転 */

void TileImage_rectReverse_horz(TileImage *p,mBox *box)
{
	int x1,x2,ix,iy,y,cnt;
	RGBAFix15 pix1,pix2;

	x1 = box->x;
	x2 = box->x + box->w - 1;
	cnt = box->w >> 1;

	for(iy = box->h, y = box->y; iy > 0; iy--, y++)
	{
		for(ix = 0; ix < cnt; ix++)
		{
			//両端の色を入れ替えてセット

			TileImage_getPixel(p, x1 + ix, y, &pix1);
			TileImage_getPixel(p, x2 - ix, y, &pix2);

			TileImage_setPixel_draw_direct(p, x1 + ix, y, &pix2);
			TileImage_setPixel_draw_direct(p, x2 - ix, y, &pix1);
		}
	}
}

/** 矩形範囲の上下反転 */

void TileImage_rectReverse_vert(TileImage *p,mBox *box)
{
	int y1,y2,ix,iy,x;
	RGBAFix15 pix1,pix2;

	y1 = box->y;
	y2 = box->y + box->h - 1;

	for(iy = box->h >> 1; iy > 0; iy--, y1++, y2--)
	{
		for(ix = box->w, x = box->x; ix > 0; ix--, x++)
		{
			TileImage_getPixel(p, x, y1, &pix1);
			TileImage_getPixel(p, x, y2, &pix2);

			TileImage_setPixel_draw_direct(p, x, y1, &pix2);
			TileImage_setPixel_draw_direct(p, x, y2, &pix1);
		}
	}
}

/** 範囲を左に90度回転 */

void TileImage_rectRotate_left(TileImage *p,TileImage *src,mBox *box)
{
	int w,h,sx,sy,dx,dy,ix,iy;
	RGBAFix15 pix;

	if(!src) return;

	w = box->w, h = box->h;

	sx = w - 1;
	dx = box->x + (w >> 1) - (h >> 1);
	dy = box->y + (h >> 1) - (w >> 1);

	for(iy = 0; iy < w; iy++, sx--)
	{
		for(ix = 0, sy = 0; ix < h; ix++, sy++)
		{
			TileImage_getPixel(src, sx, sy, &pix);
			TileImage_setPixel_draw_direct(p, dx + ix, dy + iy, &pix);
		}
	}
}

/** 範囲を右に90度回転 */

void TileImage_rectRotate_right(TileImage *p,TileImage *src,mBox *box)
{
	int w,h,sx,sy,dx,dy,ix,iy;
	RGBAFix15 pix;

	if(!src) return;

	w = box->w, h = box->h;

	sx = 0;
	dx = box->x + (w >> 1) - (h >> 1);
	dy = box->y + (h >> 1) - (w >> 1);

	for(iy = 0; iy < w; iy++, sx++)
	{
		sy = h - 1;
		
		for(ix = 0; ix < h; ix++, sy--)
		{
			TileImage_getPixel(src, sx, sy, &pix);
			TileImage_setPixel_draw_direct(p, dx + ix, dy + iy, &pix);
		}
	}
}


//=============================
// フィルタ用
//=============================


/** src の指定範囲内のイメージを同じ位置にコピー
 * 
 * プレビューイメージ用。
 * [!] dst は空状態である。 */

void TileImage_copyImage_rect(TileImage *dst,TileImage *src,mRect *rcarea)
{
	int ix,iy;
	RGBAFix15 pix;
	mRect rc = *rcarea;

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		for(ix = rc.x1; ix <= rc.x2; ix++)
		{
			TileImage_getPixel(src, ix, iy, &pix);

			if(pix.a)
				TileImage_setPixel_new(dst, ix, iy, &pix);
		}
	}
}


//=============================
// 変形: 共通 sub
//=============================


/** Bicubic 重み取得 */

static double _get_bicubic(double d)
{
	d = fabs(d);

	if(d < 1.0)
		return 1 - 2 * d * d + d * d * d;
	else if(d < 2.0)
		return 4 - 8 * d + 5 * d * d - d * d * d;
	else
		return 0;
}

/** Bicubic 補間の色取得
 *
 * @return FALSE で色のセットなし */

static mBool _getcolor_bicubic(TileImage *src,
	double dx,double dy,int nx,int ny,int sw,int sh,RGBAFix15 *pixdst)
{
	int i,j,x,y;
	double wx[4],wy[4],dc[4],da,w;
	RGBAFix15 pix;

	//ソース範囲外

	if(nx < 0 || ny < 0 || nx >= sw || ny >= sh)
		return FALSE;

	//

	nx--;
	ny--;

	//Bicubic 重みテーブル

	for(i = 0; i < 4; i++)
	{
		wx[i] = _get_bicubic(dx - (nx + i + 0.5));
		wy[i] = _get_bicubic(dy - (ny + i + 0.5));
	}

	//ソースの色 (4x4近傍)

	dc[0] = dc[1] = dc[2] = da = 0;

	for(i = 0; i < 4; i++)
	{
		y = ny + i;
		if(y < 0) y = 0;
		else if(y >= sh) y = sh - 1;
	
		for(j = 0; j < 4; j++)
		{
			x = nx + j;
			if(x < 0) x = 0;
			else if(x >= sw) x = sw - 1;
		
			TileImage_getPixel(src, x, y, &pix);

			if(pix.a)
			{
				w = wx[j] * wy[i] * ((double)pix.a / 0x8000);
				
				dc[0] += pix.r * w;
				dc[1] += pix.g * w;
				dc[2] += pix.b * w;
				da += w;
			}
		}
	}

	//A

	j = (int)(da * 0x8000 + 0.5);
	if(j < 0) j = 0; else if(j > 0x8000) j = 0x8000;

	pix.a = j;

	if(j == 0) return FALSE;

	//RGB

	da = 1 / da;

	for(i = 0; i < 3; i++)
	{
		j = (int)(dc[i] * da + 0.5);
		if(j < 0) j = 0; else if(j > 0x8000) j = 0x8000;

		pix.c[i] = j;
	}

	*pixdst = pix;

	return TRUE;
}


//=============================
// 変形: アフィン変換
//=============================


/** アフィン変換の描画対象範囲取得 */

static void _get_affinetrans_rect(mRect *rcdst,mBox *box,
	double scalex,double scaley,double dcos,double dsin,double movx,double movy)
{
	double cx,cy,xx,yy;
	mDoublePoint pt[4];
	mPoint ptmin,ptmax;
	int i,x,y;

	cx = box->w * 0.5;
	cy = box->h * 0.5;

	//4隅の座標

	pt[0].x = pt[3].x = -cx;
	pt[0].y = pt[1].y = -cy;
	pt[1].x = pt[2].x = -cx + box->w;
	pt[2].y = pt[3].y = -cy + box->h;

	//4隅の点を変換して、最小/最大値取得

	for(i = 0; i < 4; i++)
	{
		xx = pt[i].x * scalex;
		yy = pt[i].y * scaley;
	
		x = floor(xx * dcos - yy * dsin + cx);
		y = floor(xx * dsin + yy * dcos + cy);

		if(i == 0)
		{
			ptmin.x = ptmax.x = x;
			ptmin.y = ptmax.y = y;
		}
		else
		{
			if(x < ptmin.x) ptmin.x = x;
			if(y < ptmin.y) ptmin.y = y;
			if(x > ptmax.x) ptmax.x = x;
			if(y > ptmax.y) ptmax.y = y;
		}
	}

	//セット (念の為範囲拡大)

	rcdst->x1 = box->x + ptmin.x + movx - 2;
	rcdst->y1 = box->y + ptmin.y + movy - 2;
	rcdst->x2 = box->x + ptmax.x + movx + 2;
	rcdst->y2 = box->y + ptmax.y + movy + 2;
}

/** アフィン変換
 *
 * @param src  元イメージ (0,0)-(wxh) */

void TileImage_transformAffine(TileImage *dst,TileImage *src,mBox *box,
	double scalex,double scaley,double dcos,double dsin,double movx,double movy,
	mPopupProgress *prog)
{
	mRect rc;
	int ix,iy,sw,sh,nx,ny;
	double cx,cy,dx,dy,dxx,dyy,scalex_div,scaley_div,addxx,addxy,addyx,addyy;
	RGBAFix15 pix;
	TileImageSetPixelFunc setpix = g_tileimage_dinfo.funcDrawPixel;

	//描画先範囲 + クリッピング

	_get_affinetrans_rect(&rc, box, scalex, scaley, dcos, dsin, movx, movy);

	if(!TileImage_clipCanDrawRect(dst, &rc)) return;

	//

	sw = box->w, sh = box->h;

	cx = sw * 0.5;
	cy = sh * 0.5;

	scalex_div = 1 / scalex;
	scaley_div = 1 / scaley;

	//開始位置のソース座標

	dx = rc.x1 - box->x - movx - cx;
	dy = rc.y1 - box->y - movy - cy;

	dxx = (dx *  dcos + dy * dsin) * scalex_div + cx;
	dyy = (dx * -dsin + dy * dcos) * scaley_div + cy;

	//

	addxx = dcos * scalex_div;
	addxy = -dsin * scaley_div;
	addyx = dsin * scalex_div;
	addyy = dcos * scaley_div;

	//

	mPopupProgressThreadBeginSubStep_onestep(prog, 20, rc.y2 - rc.y1 + 1);

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		dx = dxx;
		dy = dyy;
	
		for(ix = rc.x1; ix <= rc.x2; ix++, dx += addxx, dy += addxy)
		{
			nx = floor(dx);
			ny = floor(dy);

			//セット

			if(_getcolor_bicubic(src, dx, dy, nx, ny, sw, sh, &pix))
				(setpix)(dst, ix, iy, &pix);
		}

		dxx += addyx;
		dyy += addyy;

		mPopupProgressThreadIncSubStep(prog);
	}
}

/** アフィン変換＋キャンバスに合成 (プレビュー用) */

void TileImage_drawPixbuf_affine(TileImage *p,mPixbuf *pixbuf,mBox *boxsrc,
	double scalex,double scaley,double dcos,double dsin,
	mPoint *ptmov,CanvasDrawInfo *canvinfo)
{
	int ix,iy,nx,ny,sw,sh,pitchd,bpp;
	uint32_t col;
	double cx,cy,scalec,dx,dy,dx_xx,dx_yx,dy_xy,dy_yy,add_cos,add_sin;
	mRect rc;
	mBox box;
	RGBAFix15 pix;
	uint8_t *pd;

	//クリッピング

	if(!mPixbufGetClipBox_box(pixbuf, &box, &canvinfo->boxdst)) return;

	mRectSetByBox(&rc, &box);

	//

	sw = boxsrc->w;
	sh = boxsrc->h;
	cx = sw * 0.5;
	cy = sh * 0.5;

	scalec = canvinfo->param->scalediv;

	//キャンバス左上のソース位置 (変換前)

	dx = (rc.x1 + canvinfo->scrollx) * scalec + canvinfo->originx - boxsrc->x - cx - ptmov->x;
	dy = (rc.y1 + canvinfo->scrolly) * scalec + canvinfo->originy - boxsrc->y - cy - ptmov->y;

	//

	dx_xx = dx * dcos;
	dx_yx = dx * -dsin;
	dy_xy = dy * dsin;
	dy_yy = dy * dcos;

	add_cos = scalec * dcos;
	add_sin = scalec * dsin;

	//pixbuf

	pd = mPixbufGetBufPtFast(pixbuf, rc.x1, rc.y1);
	bpp = pixbuf->bpp;
	pitchd = pixbuf->pitch_dir - box.w * bpp;

	//

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		dx = dx_xx;
		dy = dx_yx;
	
		for(ix = rc.x1; ix <= rc.x2; ix++, pd += bpp, dx += add_cos, dy -= add_sin)
		{
			//逆アフィン変換
			/* (sx *  dcos + sy * dsin) * scalex
			 * (sx * -dsin + sy * dcos) * scaley */

			nx = floor((dx + dy_xy) * scalex + cx);
			ny = floor((dy + dy_yy) * scaley + cy);
		
			//ソース範囲外

			if(nx < 0 || ny < 0 || nx >= sw || ny >= sh)
				continue;

			//ソースの色

			TileImage_getPixel(p, nx, ny, &pix);

			//キャンバスに合成

			col = RGBCONV_FIX15_TO_8(pix.r) << 16;
			col |= RGBCONV_FIX15_TO_8(pix.g) << 8;
			col |= RGBCONV_FIX15_TO_8(pix.b);
			col |= pix.a >> (15 - 7) << 24;

			mPixbufSetPixelBuf_blend128(pixbuf, pd, col);
		}

		pd += pitchd;
		dy_xy += add_sin;
		dy_yy += add_cos;
	}
}


//=============================
// 変形: 射影変換
//=============================


/** 射影変換
 *
 * @param param [0..8]:射影変換用パラメータ [9..10]:平行移動 */

void TileImage_transformHomography(TileImage *dst,TileImage *src,mRect *rcdst,
	double *param,int sx,int sy,int sw,int sh,mPopupProgress *prog)
{
	mRect rc;
	int ix,iy,nx,ny;
	double dp_y,dx_y,dy_y,dx,dy,dp,dsx,dsy;
	RGBAFix15 pix;
	TileImageSetPixelFunc setpix = g_tileimage_dinfo.funcDrawPixel;

	//クリッピング

	rc = *rcdst;

	if(!TileImage_clipCanDrawRect(dst, &rc)) return;

	//開始時のパラメータ

	dx = rc.x1 - param[9];
	dy = rc.y1 - param[10];

	dp_y = dx * param[6] + dy * param[7] + param[8];
	dx_y = dx * param[0] + dy * param[1] + param[2];
	dy_y = dx * param[3] + dy * param[4] + param[5];

	//

	mPopupProgressThreadBeginSubStep_onestep(prog, 20, rc.y2 - rc.y1 + 1);

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		dp = dp_y;
		dx = dx_y;
		dy = dy_y;
	
		for(ix = rc.x1; ix <= rc.x2; ix++, dp += param[6], dx += param[0], dy += param[3])
		{
			//逆変換
			/* d = ix * param[6] + iy * param[7] + param[8]
			 * sx = (ix * param[0] + iy * param[1] + param[2]) / d
			 * sy = (ix * param[3] + iy * param[4] + param[5]) / d */

			dsx = dx / dp - sx;
			dsy = dy / dp - sy;
			
			nx = floor(dsx);
			ny = floor(dsy);

			//セット

			if(_getcolor_bicubic(src, dsx, dsy, nx, ny, sw, sh, &pix))
				(setpix)(dst, ix, iy, &pix);
		}

		dp_y += param[7];
		dx_y += param[1];
		dy_y += param[4];

		mPopupProgressThreadIncSubStep(prog);
	}
}

/** 射影変換＋キャンバスに合成 (プレビュー用) */

void TileImage_drawPixbuf_homography(TileImage *p,mPixbuf *pixbuf,mBox *boxdst,
	double *param,mPoint *ptmov,int sx,int sy,int sw,int sh,CanvasDrawInfo *canvinfo)
{
	int ix,iy,nx,ny,pitchd,bpp;
	uint32_t col;
	double scalec,dx,dy,dp,dp_y,dx_y,dy_y,addxp,addxx,addxy,addyp,addyx,addyy;
	mRect rc;
	mBox box;
	RGBAFix15 pix;
	uint8_t *pd;

	//クリッピング

	if(!mPixbufGetClipBox_box(pixbuf, &box, boxdst)) return;

	mRectSetByBox(&rc, &box);

	//

	scalec = canvinfo->param->scalediv;

	//キャンバス左上のパラメータ

	dx = (rc.x1 + canvinfo->scrollx) * scalec + canvinfo->originx - ptmov->x;
	dy = (rc.y1 + canvinfo->scrolly) * scalec + canvinfo->originy - ptmov->y;

	dp_y = dx * param[6] + dy * param[7] + param[8];
	dx_y = dx * param[0] + dy * param[1] + param[2];
	dy_y = dx * param[3] + dy * param[4] + param[5];

	//加算値

	addxp = scalec * param[6];
	addxx = scalec * param[0];
	addxy = scalec * param[3];

	addyp = scalec * param[7];
	addyx = scalec * param[1];
	addyy = scalec * param[4];

	//pixbuf

	pd = mPixbufGetBufPtFast(pixbuf, rc.x1, rc.y1);
	bpp = pixbuf->bpp;
	pitchd = pixbuf->pitch_dir - box.w * bpp;

	//

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		dp = dp_y;
		dx = dx_y;
		dy = dy_y;

		//

		for(ix = rc.x1; ix <= rc.x2; ix++, pd += bpp, dp += addxp, dx += addxx, dy += addxy)
		{
			//逆変換
			/* d = x * param[6] + y * param[7] + param[8]
			 * sx = (x * param[0] + y * param[1] + param[2]) / d
			 * sy = (x * param[3] + y * param[4] + param[5]) / d */

			nx = floor(dx / dp - sx);
			ny = floor(dy / dp - sy);
		
			//ソース範囲外

			if(nx < 0 || ny < 0 || nx >= sw || ny >= sh)
				continue;

			//ソースの色

			TileImage_getPixel(p, nx, ny, &pix);

			//キャンバスに合成

			col = RGBCONV_FIX15_TO_8(pix.r) << 16;
			col |= RGBCONV_FIX15_TO_8(pix.g) << 8;
			col |= RGBCONV_FIX15_TO_8(pix.b);
			col |= pix.a >> (15 - 7) << 24;

			mPixbufSetPixelBuf_blend128(pixbuf, pd, col);
		}

		pd += pitchd;

		dp_y += addyp;
		dx_y += addyx;
		dy_y += addyy;
	}
}
