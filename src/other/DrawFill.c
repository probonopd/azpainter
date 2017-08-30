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
 * DrawFill
 *
 * 塗りつぶし描画処理
 *****************************************/
/*
 * - 不透明範囲の判定時は、判定元は常に描画先 (判定元レイヤ無効)。
 * 
 * - RGB で判定する場合、判定元は最初の一つのみ有効。
 *
 * - アルファ値で判定する場合は、すべての判定元のアルファ値を合成した値で比較。
 *
 * - 塗りつぶしの判定元は imgref が先頭で、以降は TileImage::link でリンクされている。
 *
 * - rcref は塗りつぶしの判定を行う範囲。
 *   この範囲外は境界色で囲まれているとみなす。
 *
 * - imgtmp_ref は塗りつぶしの判定元をすべて合成したイメージ。最初は空。
 *   判定時に色を取得する際にタイルがなければ、その都度タイルを作成していく。
 *
 * - imgtmp_draw (1bit) は塗りつぶす部分に点を置いておくイメージ。
 *   直接描画先に描画するのではなく、まずはここに点を置いて描画する点を決める。
 *
 */

#include "mDef.h"

#include "TileImage.h"
#include "defTileImage.h"

#include "DrawFill.h"

#include "defDraw.h"
#include "draw_main.h"


//------------------

typedef struct
{
	int lx,rx,y,oy;
}_fillbuf;

struct _DrawFill
{
	_fillbuf *buf;		//塗りつぶし用バッファ

	TileImage *imgdst,		//描画先
			*imgref,		//判定元の先頭 (複数の場合はリンクされている)
			*imgtmp_ref,	//作業用 (判定元用イメージ)
			*imgtmp_draw,	//作業用 (塗りつぶす部分:A1)
			*imgtmp_draw2,	//作業用 (塗りつぶす部分<アンチエイリアス自動用>:A1)
			*imgtarget;		//作業時の描画先イメージ

	int type,				//処理タイプ
		color_diff,			//許容色差 (0-0x8000)
		draw_opacity,		//描画濃度 (0-0x8000)
		refimage_multi;		//判定元が複数あるか
	mPoint ptStart;			//開始点
	mRect rcref;			//判定する範囲
	RGBAFix15 pix_start;	//開始点の色

	mBool (*compare)(DrawFill *,int,int);	//ピクセル値比較関数
};

//------------------

static void _run_normal(DrawFill *p);
static void _run_auto_horz(DrawFill *p);
static void _run_auto_vert(DrawFill *p);

static int _getpixelref_alpha(DrawFill *p,int x,int y);

static mBool _compare_rgb(DrawFill *p,int x,int y);
static mBool _compare_alpha(DrawFill *p,int x,int y);
static mBool _compare_alpha_0(DrawFill *p,int x,int y);
static mBool _compare_opaque(DrawFill *p,int x,int y);

//------------------

#define _FILLBUF_NUM  8192

#define _BLEND_ALPHA(a,b)  ((a) + (b) - ((a) * (b) >> 15))


//------------------



/** 作業用バッファ解放 (描画時に不要なもの) */

static void _free_tmp(DrawFill *p)
{
	TileImage_free(p->imgtmp_ref);
	TileImage_free(p->imgtmp_draw2);

	mFree(p->buf);

	//

	p->imgtmp_ref = p->imgtmp_draw2 = NULL;
	p->buf = NULL;
}

/** 解放 */

void DrawFill_free(DrawFill *p)
{
	if(p)
	{
		_free_tmp(p);
		
		TileImage_free(p->imgtmp_draw);

		mFree(p);
	}
}


//============================
// 初期化
//============================


/** 開始点の色を取得 */

static void _get_start_pixel(DrawFill *p,int x,int y)
{
	TileImage *img = p->imgref;
	RGBAFix15 pix,pix2;

	if(p->type == DRAWFILL_TYPE_CANVAS)
		//キャンバス色時
		drawImage_getBlendColor_atPoint(APP_DRAW, x, y, &pix);
	else
	{
		//判定元先頭の色

		TileImage_getPixel(img, x, y, &pix);

		//複数の判定元を参照する場合、アルファ値を合成

		if(p->refimage_multi)
		{
			for(img = img->link; img; img = img->link)
			{
				TileImage_getPixel(img, x, y, &pix2);
				pix.a = _BLEND_ALPHA(pix.a, pix2.a);
			}
		}
	}

	p->pix_start = pix;
}

/** 各タイプ別の初期化 (開始点のピクセル判定 + 比較関数セット)
 *
 * @return TRUE で描画する必要がない */

static mBool _init_type(DrawFill *p)
{
	switch(p->type)
	{
		//RGB、キャンバス色
		case DRAWFILL_TYPE_RGB:
		case DRAWFILL_TYPE_CANVAS:
			p->compare = _compare_rgb;
			return FALSE;

		//アルファ値
		case DRAWFILL_TYPE_ALPHA:
			p->compare = _compare_alpha;
			return FALSE;
	
		//アンチエイリアス自動
		case DRAWFILL_TYPE_AUTO_ANTIALIAS:
			p->compare = _compare_alpha_0;
			return (p->pix_start.a != 0);

		//不透明範囲
		default:
			p->compare = _compare_opaque;
			return (p->pix_start.a == 0);
	}
}

/** 初期化 */

static mBool _init(DrawFill *p)
{
	//判定元を複数参照するか
	//(塗りつぶし判定元が複数あって、アルファ値で判定する場合)

	p->refimage_multi = (p->imgref->link != 0
		&& p->type != DRAWFILL_TYPE_RGB && p->type != DRAWFILL_TYPE_CANVAS);

	//判定範囲をセット

	if(p->type == DRAWFILL_TYPE_OPAQUE)
	{
		//[不透明範囲] 描画先の描画可能範囲
		
		TileImage_getCanDrawRect_pixel(p->imgdst, &p->rcref);
	}
	else
	{
		/* [通常塗りつぶし]
		 *
		 * 両方共にイメージの範囲。
		 * イメージ範囲外に余白部分があるとその部分も塗りつぶされてしまうので、
		 * イメージ範囲内に限定する。 */

		p->rcref.x1 = p->rcref.y1 = 0;
		p->rcref.x2 = APP_DRAW->imgw - 1;
		p->rcref.y2 = APP_DRAW->imgh - 1;
	}

	//開始点が判定範囲内にあるか
	//(範囲外なら描画の必要なし)

	if(p->ptStart.x < p->rcref.x1 || p->ptStart.y < p->rcref.y1
		|| p->ptStart.x > p->rcref.x2 || p->ptStart.y > p->rcref.y2)
		return FALSE;

	//開始点の色を取得
	
	_get_start_pixel(p, p->ptStart.x, p->ptStart.y);

	//タイプ別の初期化と開始点判定

	if(_init_type(p))
		return FALSE;

	//------- 確保

	//バッファ確保

	p->buf = (_fillbuf *)mMalloc(sizeof(_fillbuf) * _FILLBUF_NUM, FALSE);
	if(!p->buf) return FALSE;

	//描画用イメージ (1bit)

	p->imgtmp_draw = TileImage_newFromRect(TILEIMAGE_COLTYPE_ALPHA1, &p->rcref);
	if(!p->imgtmp_draw) return FALSE;

	//判定元合成イメージ (アルファ値複数判定かキャンバス判定時)

	if(p->refimage_multi || p->type == DRAWFILL_TYPE_CANVAS)
	{
		p->imgtmp_ref = TileImage_newFromRect(
			(p->refimage_multi)? TILEIMAGE_COLTYPE_ALPHA16: TILEIMAGE_COLTYPE_RGBA, &p->rcref);

		if(!p->imgtmp_ref) return FALSE;
	}

	//アンチエイリアス自動用 (1bit)

	if(p->type == DRAWFILL_TYPE_AUTO_ANTIALIAS)
	{
		p->imgtmp_draw2 = TileImage_newFromRect(TILEIMAGE_COLTYPE_ALPHA1, &p->rcref);
		if(!p->imgtmp_draw2) return FALSE;
	}

	return TRUE;
}

/** 作成と初期化
 *
 * @return NULL で確保に失敗、または描画する必要なし */

DrawFill *DrawFill_new(TileImage *imgdst,TileImage *imgref,mPoint *ptstart,
	int type,int color_diff,int draw_opacity)
{
	DrawFill *p;

	//確保

	p = (DrawFill *)mMalloc(sizeof(DrawFill), TRUE);
	if(!p) return NULL;

	//値セット

	p->imgdst = imgdst;
	p->imgref = imgref;
	p->ptStart = *ptstart;
	p->type = type;
	p->color_diff = (int)(color_diff / 100.0 * 0x8000 + 0.5);
	p->draw_opacity = (int)(draw_opacity / 100.0 * 0x8000 + 0.5);

	//初期化

	if(_init(p))
		return p;
	else
	{
		DrawFill_free(p);
		return NULL;
	}
}


//===============================
// 実行
//===============================


/** 実行 */

void DrawFill_run(DrawFill *p,RGBAFix15 *pixdraw)
{
	//----- 実行
	//(imgtmp_draw に塗りつぶす部分をセット)

	p->imgtarget = p->imgtmp_draw;

	if(p->type != DRAWFILL_TYPE_AUTO_ANTIALIAS)
		//通常塗りつぶし
		_run_normal(p);
	else
	{
		//---- アンチエイリアス自動

		//水平 : imgtmp_draw に描画
		
		_run_auto_horz(p);

		//垂直 : imgtmp_draw2 に描画
		
		p->imgtarget = p->imgtmp_draw2;

		_run_auto_vert(p);

		//結合 (draw = draw + draw2)

		TileImage_combine_forA1(p->imgtmp_draw, p->imgtmp_draw2);
	}

	//作業用バッファ解放

	_free_tmp(p);

	//imgtmp_draw を参照して実際に描画

	TileImage_drawPixels_fromA1(p->imgdst, p->imgtmp_draw, pixdraw);
}


//===============================
// スキャン
//===============================


/** 水平スキャン */

static void _scan_horz(DrawFill *p,
	int lx,int rx,int y,int oy,_fillbuf **pped,
	mBool (*compfunc)(DrawFill *,int,int))
{
	while(lx <= rx)
	{
		//左の非境界点

		for(; lx < rx; lx++)
		{
			if(!compfunc(p, lx, y)) break;
		}

		if(compfunc(p, lx, y)) break;

		(*pped)->lx = lx;

		//右の境界点

		for(; lx <= rx; lx++)
		{
			if(compfunc(p, lx, y)) break;
		}

		(*pped)->rx = lx - 1;
		(*pped)->y  = y;
		(*pped)->oy = oy;

		if(++(*pped) == p->buf + _FILLBUF_NUM)
			(*pped) = p->buf;
	}
}


/** 垂直スキャン */

static void _scan_vert(DrawFill *p,
	int ly,int ry,int x,int ox,_fillbuf **pped,
	mBool (*compfunc)(DrawFill *,int,int))
{
	while(ly <= ry)
	{
		//開始点

		for(; ly < ry; ly++)
		{
			if(!compfunc(p, x, ly)) break;
		}

		if(compfunc(p, x, ly)) break;

		(*pped)->lx = ly;

		//終了点

		for(; ly <= ry; ly++)
		{
			if(compfunc(p, x, ly)) break;
		}

		(*pped)->rx = ly - 1;
		(*pped)->y  = x;
		(*pped)->oy = ox;

		if(++(*pped) == p->buf + _FILLBUF_NUM)
			(*pped) = p->buf;
	}
}


//============================
// 通常塗りつぶし処理
//============================


/** 通常塗りつぶし処理 実行 */

void _run_normal(DrawFill *p)
{
	int lx,rx,ly,oy,_lx,_rx;
	_fillbuf *pst,*ped;
	mBool (*compfunc)(DrawFill *,int,int) = p->compare;

	pst = p->buf;
	ped = p->buf + 1;

	pst->lx = pst->rx = p->ptStart.x;
	pst->y  = pst->oy = p->ptStart.y;

	//

	do
	{
		lx = pst->lx;
		rx = pst->rx;
		ly = pst->y;
		oy = pst->oy;

		_lx = lx - 1;
		_rx = rx + 1;

		if(++pst == p->buf + _FILLBUF_NUM)
			pst = p->buf;

		//現在の点が境界点か

		if(compfunc(p, lx, ly)) continue;

		//右方向の境界点を探す

		for(; rx < p->rcref.x2; rx++)
		{
			if(compfunc(p, rx + 1, ly)) break;
		}

		//左方向の境界点を探す

		for(; lx > p->rcref.x1; lx--)
		{
			if(compfunc(p, lx - 1, ly)) break;
		}

		//lx-rx の水平線描画
		//(描画に失敗したらエラー)

		if(!TileImage_drawLineH_forA1(p->imgtmp_draw, lx, rx, ly))
			return;

		//真上の走査

		if(ly - 1 >= p->rcref.y1)
		{
			if(ly - 1 == oy)
			{
				_scan_horz(p, lx, _lx, ly - 1, ly, &ped, compfunc);
				_scan_horz(p, _rx, rx, ly - 1, ly, &ped, compfunc);
			}
			else
				_scan_horz(p, lx, rx, ly - 1, ly, &ped, compfunc);
		}

		//真下の走査

		if(ly + 1 <= p->rcref.y2)
		{
			if(ly + 1 == oy)
			{
				_scan_horz(p, lx, _lx, ly + 1, ly, &ped, compfunc);
				_scan_horz(p, _rx, rx, ly + 1, ly, &ped, compfunc);
			}
			else
				_scan_horz(p, lx, rx, ly + 1, ly, &ped, compfunc);
		}

	} while(pst != ped);
}


//===============================
// アンチエイリアス自動判定
//===============================
/*
 * 境界を見つける時に最大アルファ値を記憶しておき、アルファ値が下がった点を境界とする。
 * これを水平方向、垂直方向の両方で実行する。
 * 結果をそれぞれ別のイメージに描画して、最後に２つのイメージを結合する。
 */

/** 水平走査 */

void _run_auto_horz(DrawFill *p)
{
	int lx,rx,ly,oy,_lx,_rx,a,max,lx2,rx2,flag,draw_opacity;
	_fillbuf *pst,*ped;
	mBool (*compfunc)(DrawFill *,int,int) = p->compare;

	pst = p->buf;
	ped = p->buf + 1;

	pst->lx = pst->rx = p->ptStart.x;
	pst->y  = pst->oy = p->ptStart.y;

	draw_opacity = p->draw_opacity;

	//

	do
	{
		lx = pst->lx;
		rx = pst->rx;
		ly = pst->y;
		oy = pst->oy;

		_lx = lx - 1;
		_rx = rx + 1;

		if(++pst == p->buf + _FILLBUF_NUM)
			pst = p->buf;

		//透明ではないか

		if(compfunc(p, lx, ly)) continue;

		//右方向

		rx2 = rx;

		for(max = 0, flag = 1; rx < p->rcref.x2; rx++)
		{
			if(TileImage_isPixelOpaque(p->imgtarget, rx + 1, ly)) break;

			a = _getpixelref_alpha(p, rx + 1, ly);

			if(a) flag = 0;
			if(a >= draw_opacity || a < max) break;

			max = a;
			if(flag) rx2++;
		}

		//左方向

		lx2 = lx;

		for(max = 0, flag = 1; lx > p->rcref.x1; lx--)
		{
			if(TileImage_isPixelOpaque(p->imgtarget, lx - 1, ly)) break;

			a = _getpixelref_alpha(p, lx - 1, ly);

			if(a) flag = 0;
			if(a >= draw_opacity || a < max) break;

			max = a;
			if(flag) lx2--;
		}

		//lx-rx の水平線描画

		if(!TileImage_drawLineH_forA1(p->imgtarget, lx, rx, ly))
			return;

		//真上の走査

		if(ly - 1 >= p->rcref.y1)
		{
			if(ly - 1 == oy)
			{
				_scan_horz(p, lx2, _lx, ly - 1, ly, &ped, compfunc);
				_scan_horz(p, _rx, rx2, ly - 1, ly, &ped, compfunc);
			}
			else
				_scan_horz(p, lx2, rx2, ly - 1, ly, &ped, compfunc);
		}

		//真下の走査

		if(ly + 1 <= p->rcref.y2)
		{
			if(ly + 1 == oy)
			{
				_scan_horz(p, lx2, _lx, ly + 1, ly, &ped, compfunc);
				_scan_horz(p, _rx, rx2, ly + 1, ly, &ped, compfunc);
			}
			else
				_scan_horz(p, lx2, rx2, ly + 1, ly, &ped, compfunc);
		}

	} while(pst != ped);
}

/** 垂直走査 */

void _run_auto_vert(DrawFill *p)
{
	int ly,ry,xx,ox,_ly,_ry,a,max,ly2,ry2,flag,draw_opacity;
	_fillbuf *pst,*ped;
	mBool (*compfunc)(DrawFill *,int,int) = p->compare;

	pst = p->buf;
	ped = p->buf + 1;

	pst->lx = pst->rx = p->ptStart.y;
	pst->y  = pst->oy = p->ptStart.x;

	draw_opacity = p->draw_opacity;

	//

	do
	{
		ly = pst->lx;
		ry = pst->rx;
		xx = pst->y;
		ox = pst->oy;

		_ly = ly - 1;
		_ry = ry + 1;

		if(++pst == p->buf + _FILLBUF_NUM)
			pst = p->buf;

		//透明ではないか

		if(compfunc(p, xx, ly)) continue;

		//下方向

		ry2 = ry;

		for(max = 0, flag = 1; ry < p->rcref.y2; ry++)
		{
			if(TileImage_isPixelOpaque(p->imgtarget, xx, ry + 1)) break;

			a = _getpixelref_alpha(p, xx, ry + 1);

			if(a) flag = 0;
			if(a >= draw_opacity || a < max) break;

			max = a;
			if(flag) ry2++;
		}

		//上方向

		ly2 = ly;

		for(max = 0, flag = 1; ly > p->rcref.y1; ly--)
		{
			if(TileImage_isPixelOpaque(p->imgtarget, xx, ly - 1)) break;

			a = _getpixelref_alpha(p, xx, ly - 1);

			if(a) flag = 0;
			if(a >= draw_opacity || a < max) break;

			max = a;
			if(flag) ly2--;
		}

		//ly-ry の垂直線描画

		if(!TileImage_drawLineV_forA1(p->imgtarget, ly, ry, xx))
			return;

		//左の走査

		if(xx - 1 >= p->rcref.x1)
		{
			if(xx - 1 == ox)
			{
				_scan_vert(p, ly2, _ly, xx - 1, xx, &ped, compfunc);
				_scan_vert(p, _ry, ry2, xx - 1, xx, &ped, compfunc);
			}
			else
				_scan_vert(p, ly2, ry2, xx - 1, xx, &ped, compfunc);
		}

		//右の走査

		if(xx + 1 <= p->rcref.x2)
		{
			if(xx + 1 == ox)
			{
				_scan_vert(p, ly2, _ly, xx + 1, xx, &ped, compfunc);
				_scan_vert(p, _ry, ry2, xx + 1, xx, &ped, compfunc);
			}
			else
				_scan_vert(p, ly2, ry2, xx + 1, xx, &ped, compfunc);
		}

	} while(pst != ped);
}


//=============================
// 判定元アルファ値取得
//=============================


/** 判定元イメージ用のタイルをセット
 *
 * @return 作成されたタイルのポインタ。TILEIMAGE_TILE_EMPTY で透明タイル */

static uint8_t *_set_tile_ref(DrawFill *p,int topx,int topy,uint8_t **pptile)
{
	TileImage *imgsrc;
	uint16_t *pd;
	uint32_t *ptile32;
	int ix,iy,da;
	RGBAFix15 pix;

	//トップのレイヤは、アルファ値をそのままセット

	imgsrc = p->imgref;
	pd = (uint16_t *)*pptile;

	for(iy = 0; iy < 64; iy++)
	{
		for(ix = 0; ix < 64; ix++)
		{
			TileImage_getPixel(imgsrc, topx + ix, topy + iy, &pix);

			*(pd++) = pix.a;
		}
	}

	//以降のリンクはアルファ値を合成

	for(imgsrc = imgsrc->link; imgsrc; imgsrc = imgsrc->link)
	{
		pd = (uint16_t *)*pptile;

		for(iy = 0; iy < 64; iy++)
		{
			for(ix = 0; ix < 64; ix++)
			{
				da = *pd;
				TileImage_getPixel(imgsrc, topx + ix, topy + iy, &pix);

				*(pd++) = _BLEND_ALPHA(da, pix.a);
			}
		}
	}

	//タイルがすべて透明なら、解放して TILEIMAGE_TILE_EMPTY をセット

	ptile32 = (uint32_t *)*pptile;

	for(ix = 64 * 64 * 2 / 4; ix && !(*ptile32); ix--, ptile32++);

	if(ix == 0)
	{
		TileImage_freeTile(pptile);
		*pptile = TILEIMAGE_TILE_EMPTY;
	}

	return *pptile;
}

/** [アルファ値判定時] 判定元をすべて合成したアルファ値を取得
 *
 * imgtmp_ref にタイルがなければ作成。 */

int _getpixelref_alpha(DrawFill *p,int x,int y)
{
	TileImage *img;
	uint8_t **pptile,*tile;
	int tx,ty,topx,topy;
	RGBAFix15 pix;

	//判定元が単体なら直接取得

	if(!p->refimage_multi)
	{
		TileImage_getPixel(p->imgref, x, y, &pix);
		return pix.a;
	}

	//------- 判定元複数時

	img = p->imgtmp_ref;

	//タイル配列位置

	if(!TileImage_pixel_to_tile(img, x, y, &tx, &ty))
		return 0;

	pptile = TILEIMAGE_GETTILE_BUFPT(img, tx, ty);

	//タイル

	tile = *pptile;

	if(tile == TILEIMAGE_TILE_EMPTY)
		//透明タイル
		return 0;
	else if(!tile)
	{
		//タイル作成

		*pptile = TileImage_allocTile(img, FALSE);
		if(!(*pptile)) return 0;

		TileImage_tile_to_pixel(img, tx, ty, &topx, &topy);
		
		tile = _set_tile_ref(p, topx, topy, pptile);
		if(tile == TILEIMAGE_TILE_EMPTY) return 0;
	}

	//存在するタイルからピクセル値取得

	x = (x - img->offx) & 63;
	y = (y - img->offy) & 63;

	return *((uint16_t *)tile + (y << 6) + x);
}


//=============================
// キャンバス判定元色取得
//=============================


/** キャンバスの判定元色取得
 *
 * @return FALSE で取得に失敗 */

mBool _getpixelreg_canvas(DrawFill *p,int x,int y,RGBAFix15 *pixdst)
{
	TileImage *img;
	int tx,ty,topx,topy,ix,iy;
	uint8_t **pptile,*tile;
	RGBAFix15 *pd;

	img = p->imgtmp_ref;

	//タイル配列位置

	if(!TileImage_pixel_to_tile(img, x, y, &tx, &ty))
		return FALSE;

	pptile = TILEIMAGE_GETTILE_BUFPT(img, tx, ty);

	//タイルが未確保なら作成してキャンバス色セット

	tile = *pptile;

	if(!tile)
	{
		tile = *pptile = TileImage_allocTile(img, FALSE);
		if(!(*pptile)) return FALSE;

		TileImage_tile_to_pixel(img, tx, ty, &topx, &topy);

		pd = (RGBAFix15 *)tile;
		
		for(iy = 0; iy < 64; iy++)
		{
			for(ix = 0; ix < 64; ix++, pd++)
				drawImage_getBlendColor_atPoint(APP_DRAW, topx + ix, topy + iy, pd);
		}
	}

	//存在するタイルからピクセル値取得

	x = (x - img->offx) & 63;
	y = (y - img->offy) & 63;

	*pixdst = *((RGBAFix15 *)tile + (y << 6) + x);

	return TRUE;
}


//===================================
// ピクセル値の比較関数
//===================================
/*
 * 開始点と異なる色 (塗りつぶさない部分) なら TRUE を返す。
 * imgtarget にすでに点がある場合も境界とみなす。
 */


/** RGB */

mBool _compare_rgb(DrawFill *p,int x,int y)
{
	RGBAFix15 pix;
	int diff;

	if(TileImage_isPixelOpaque(p->imgtarget, x, y))
		return TRUE;

	//判定元の色

	if(p->type == DRAWFILL_TYPE_RGB)
		TileImage_getPixel(p->imgref, x, y, &pix);
	else
	{
		if(!_getpixelreg_canvas(p, x, y, &pix))
			pix.r = pix.g = pix.b = pix.a = 0x8000;
	}

	//

	if(p->pix_start.a == 0)
		//開始点が透明なら、不透明が境界
		return (pix.a != 0);
	else if(pix.a == 0)
		//開始点が透明でなく、対象が透明なら、境界
		return TRUE;
	else
	{
		//色差が範囲外なら境界

		diff = p->color_diff;

		if(diff == 0)
		{
			return (pix.r != p->pix_start.r || pix.g != p->pix_start.g || pix.b != p->pix_start.b);
		}
		else
		{
			return (pix.r < p->pix_start.r - diff || pix.r > p->pix_start.r + diff
				|| pix.g < p->pix_start.g - diff || pix.g > p->pix_start.g + diff
				|| pix.b < p->pix_start.b - diff || pix.b > p->pix_start.b + diff);
		}
	}
}

/** アルファ値 */

mBool _compare_alpha(DrawFill *p,int x,int y)
{
	int a;

	if(TileImage_isPixelOpaque(p->imgtarget, x, y))
		return TRUE;
	
	a = _getpixelref_alpha(p, x, y);

	return (a < p->pix_start.a - p->color_diff || a > p->pix_start.a + p->color_diff);
}

/** A=0 */

mBool _compare_alpha_0(DrawFill *p,int x,int y)
{
	return (TileImage_isPixelOpaque(p->imgtarget, x, y)
		|| _getpixelref_alpha(p, x, y) != 0);
}

/** 不透明 */

mBool _compare_opaque(DrawFill *p,int x,int y)
{
	return (TileImage_isPixelOpaque(p->imgtarget, x, y)
		|| _getpixelref_alpha(p, x, y) == 0);
}
