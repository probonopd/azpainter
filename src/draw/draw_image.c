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
 * DrawData
 *
 * イメージ関連
 *****************************************/

#include "mDef.h"
#include "mPopupProgress.h"

#include "defMacros.h"
#include "defDraw.h"
#include "defScalingType.h"
#include "AppErr.h"

#include "ImageBufRGB16.h"
#include "ImageBuf8.h"

#include "defTileImage.h"
#include "TileImage.h"
#include "TileImageDrawInfo.h"
#include "LayerList.h"
#include "LayerItem.h"
#include "Undo.h"

#include "PopupThread.h"
#include "Docks_external.h"

#include "draw_main.h"
#include "draw_select.h"
#include "draw_boxedit.h"



//===================
// sub
//===================


/** イメージサイズ変更時 */

static void _change_imagesize(DrawData *p)
{
	g_tileimage_dinfo.imgw = p->imgw;
	g_tileimage_dinfo.imgh = p->imgh;
}


//=========================
// サイズ変更
//=========================


/** イメージ変更時やイメージサイズ変更時
 *
 * @param change_file 編集ファイルが変わった時 TRUE。イメージサイズのみの変更時は FALSE */

void drawImage_afterChange(DrawData *p,mBool change_file)
{
	//選択範囲解除

	drawSel_release(p, FALSE);

	//矩形編集の枠解除

	drawBoxEdit_clear_noupdate(p);

	//

	_change_imagesize(p);

	if(change_file)
		drawCanvas_fitWindow(p);
	else
		//リサイズ時はスクロールをリセットするだけ
		drawCanvas_setScrollDefault(p);

	drawUpdate_all_layer(p);

	DockCanvasView_changeImageSize();
}

/** 途中でキャンバスイメージを変更する時 */

mBool drawImage_changeImageSize(DrawData *p,int w,int h)
{
	//作業用イメージ再作成

	ImageBufRGB16_free(p->blendimg);

	if(!(p->blendimg = ImageBufRGB16_new(w, h)))
	{
		//失敗した場合、元のサイズで
		p->blendimg = ImageBufRGB16_new(p->imgw, p->imgh);
		return FALSE;
	}

	//

	p->imgw = w;
	p->imgh = h;

	return TRUE;
}


//=========================
//
//=========================


/** 新規イメージ作成
 *
 * @param dpi      0 以下でデフォルト値
 * @param coltype  レイヤ追加時のカラータイプ。負の値で作成しない */

mBool drawImage_new(DrawData *p,int w,int h,int dpi,int coltype)
{
	if(w < 1) w = 1;
	if(h < 1) h = 1;

	//undo & レイヤをクリア

	Undo_deleteAll();
	Undo_clearUpdateFlag();

	LayerList_clear(p->layerlist);

	//情報

	p->imgw = w;
	p->imgh = h;
	p->imgdpi = (dpi <= 0)? 96: dpi;
	p->curlayer = NULL;
	p->masklayer = NULL;

	_change_imagesize(p);

	//作業用イメージ

	ImageBufRGB16_free(p->blendimg);

	if(!(p->blendimg = ImageBufRGB16_new(w, h)))
		return FALSE;

	//レイヤ追加

	if(coltype >= 0)
	{
		if(!(p->curlayer = LayerList_addLayer_newimage(p->layerlist, coltype)))
			return FALSE;
	}

	return TRUE;
}

/** 画像読み込みエラー時の処理
 *
 * @return FALSE:現状のまま変更なし TRUE:更新 */

mBool drawImage_onLoadError(DrawData *p)
{
	LayerItem *top;

	top = LayerList_getItem_top(p->layerlist);

	if(top && p->curlayer)
	{
		//drawImage_new() が実行される前にエラーが出た場合は現状維持

		return FALSE;
	}
	else
	{
		/* レイヤが一つもない
		 * (drawImage_new() が実行されたが、レイヤが作成される前にエラー)
		 *
		 * => 新規イメージ */
	
		if(!top)
			drawImage_new(p, 400, 400, -1, TILEIMAGE_COLTYPE_RGBA);

		/* レイヤはあるがカレントレイヤが指定されていない
		 * (レイヤ読み込み途中でエラー)
		 *
		 * => 読み込んだレイヤはそのままでカレントレイヤセット */

		if(!p->curlayer)
			p->curlayer = top;

		return TRUE;
	}

}

/** 画像ファイルを読み込んで作成
 *
 * @return エラーコード */

int drawImage_loadFile(DrawData *p,const char *filename,
	uint32_t format,mBool ignore_alpha,
	mPopupProgress *prog,char **errmes)
{
	TileImage *img;
	LayerItem *li;
	TileImageLoadFileInfo info;

	*errmes = NULL;

	//TileImage に読み込み

	img = TileImage_loadFile(filename, format,
		ignore_alpha, IMAGE_SIZE_MAX, &info, prog, errmes);

	if(!img) return APPERR_LOAD;

	//新規キャンバス

	if(!drawImage_new(p, info.width, info.height, info.dpi, -1))
		goto ERR;

	//レイヤ作成

	li = LayerList_addLayer(p->layerlist, NULL);
	if(!li) goto ERR;

	LayerItem_replaceImage(li, img);

	LayerItem_setName_layerno(li, 0);

	//カレントレイヤ

	p->curlayer = li;

	return APPERR_OK;

ERR:
	TileImage_free(img);
	return APPERR_ALLOC;
}


//=============================
// キャンバスサイズ変更
//=============================


typedef struct
{
	int w,h,offx,offy;
}_thdata_resizecanvas;


static int _thread_resize_canvas(mPopupProgress *prog,void *data)
{
	_thdata_resizecanvas *p = (_thdata_resizecanvas *)data;
	LayerItem *pi;
	TileImage *img;

	mPopupProgressThreadSetMax(prog, LayerList_getNum(APP_DRAW->layerlist));

	for(pi = LayerList_getItem_top(APP_DRAW->layerlist); pi; pi = LayerItem_getNext(pi))
	{
		if(pi->img)
		{
			img = TileImage_createCropImage(pi->img, p->offx, p->offy, p->w, p->h);

			if(img)
				LayerItem_replaceImage(pi, img);
		}

		mPopupProgressThreadIncPos(prog);
	}

	return TRUE;
}

/** キャンバスサイズ変更 */

mBool drawImage_resizeCanvas(DrawData *p,int w,int h,int movx,int movy,mBool crop)
{
	_thdata_resizecanvas dat;
	int ret,bw,bh;

	bw = p->imgw, bh = p->imgh;

	//作業用、サイズ変更

	if(!drawImage_changeImageSize(p, w, h))
		return FALSE;

	//

	if(!crop)
	{
		//----- 切り取らない
		
		//undo

		Undo_addResizeCanvas_moveOffset(movx, movy, bw, bh);

		//各レイヤのオフセット位置を相対移動

		LayerList_moveOffset_rel_all(p->layerlist, movx, movy);

		ret = TRUE;
	}
	else
	{
		//----- 範囲外を切り取り
		/* 新しいサイズが元より大きくなる場合でも、
		 * レイヤイメージ内には新しいサイズの範囲外にイメージが残る場合があるので、
		 * 常に処理する。 */

		//undo

		Undo_addResizeCanvas_crop(bw, bh);

		//

		dat.w = w, dat.h = h;
		dat.offx = movx;
		dat.offy = movy;

		ret = PopupThread_run(&dat, _thread_resize_canvas);
		if(ret == -1) ret = FALSE;
	}

	return ret;
}


//============================
// キャンバス拡大縮小
//============================


typedef struct
{
	int w,h,type;
}_thdata_scalecanvas;


static int _thread_scale_canvas(mPopupProgress *prog,void *data)
{
	_thdata_scalecanvas *p = (_thdata_scalecanvas *)data;
	LayerItem *pi;
	TileImage *img,*srcimg;
	mSize sizeSrc,sizeDst;

	sizeSrc.w = APP_DRAW->imgw;
	sizeSrc.h = APP_DRAW->imgh;
	sizeDst.w = p->w;
	sizeDst.h = p->h;

	mPopupProgressThreadSetMax(prog,
		LayerList_getNormalLayerNum(APP_DRAW->layerlist) * 6);

	for(pi = LayerList_getItem_top(APP_DRAW->layerlist); pi; pi = LayerItem_getNext(pi))
	{
		if(pi->img)
		{
			srcimg = pi->img;
		
			img = TileImage_new(srcimg->coltype, sizeDst.w, sizeDst.h);
			if(!img) return FALSE;

			img->rgb = srcimg->rgb;

			if(!TileImage_scaling(img, srcimg, p->type, &sizeSrc, &sizeDst, prog))
			{
				TileImage_free(img);
				return FALSE;
			}

			LayerItem_replaceImage(pi, img);
		}
	}

	return TRUE;
}

/** キャンバス拡大縮小 */

mBool drawImage_scaleCanvas(DrawData *p,int w,int h,int dpi,int type)
{
	_thdata_scalecanvas dat;
	int ret;

	dat.w = w;
	dat.h = h;
	dat.type = type;

	//undo

	Undo_addScaleCanvas();

	//スレッド

	ret = PopupThread_run(&dat, _thread_scale_canvas);
	if(ret != 1) return FALSE;

	//作業用変更

	if(!drawImage_changeImageSize(p, w, h))
		return FALSE;

	//DPI 変更

	if(dpi != -1)
		p->imgdpi = dpi;

	return TRUE;
}


//========================


/** 表示用ではなく保存時など用にレイヤ合成 (p->blendimg に) */

void drawImage_blendImage_real(DrawData *p)
{
	mBox box;
	RGBFix15 col;
	LayerItem *pi;

	//白クリア

	col.r = col.g = col.b = 0x8000;
	ImageBufRGB16_fill(p->blendimg, &col);

	//レイヤ合成

	box.x = box.y = 0;
	box.w = p->imgw, box.h = p->imgh;

	pi = LayerList_getItem_bottomVisibleImage(p->layerlist);

	for(; pi; pi = LayerItem_getPrevVisibleImage(pi))
	{
		TileImage_blendToImageRGB16(pi->img, p->blendimg,
			&box, LayerItem_getOpacity_real(pi), pi->blendmode, pi->img8texture);
	}
}

/** blendimg に RGB 8bit として合成 */

mBool drawImage_blendImage_RGB8(DrawData *p)
{
	drawImage_blendImage_real(p);

	if(!ImageBufRGB16_toRGB8(p->blendimg))
	{
		drawUpdate_blendImage_full(p);
		return FALSE;
	}

	return TRUE;
}

/** blendimg に RGBA 8bit として合成
 *
 * アルファ付き PNG 保存用。
 * 合成モードはすべて「通常」とする。 */

void drawImage_blendImage_RGBA8(DrawData *p)
{
	LayerItem *pi;
	uint8_t *pd;
	RGBAFix15 pixres,pixsrc;
	int ix,iy,i;

	pd = (uint8_t *)p->blendimg->buf;

	for(iy = 0; iy < p->imgh; iy++)
	{
		for(ix = 0; ix < p->imgw; ix++, pd += 4)
		{
			pixres.v64 = 0;

			//各レイヤ合成

			pi = LayerList_getItem_bottomVisibleImage(p->layerlist);

			for( ; pi; pi = LayerItem_getPrevVisibleImage(pi))
			{
				TileImage_getPixel(pi->img, ix, iy, &pixsrc);

				if(pixsrc.a)
				{
					if(pi->img8texture)
						pixsrc.a = pixsrc.a * ImageBuf8_getPixel_forTexture(pi->img8texture, ix, iy) / 255;

					pixsrc.a = pixsrc.a * LayerItem_getOpacity_real(pi) >> 7;

					if(pixsrc.a)
						TileImage_colfunc_normal(pi->img, &pixres, &pixsrc, NULL);
				}
			}

			//セット

			if(pixres.a == 0)
				*((uint32_t *)pd) = 0;
			else
			{
				for(i = 0; i < 4; i++)
					pd[i] = (pixres.c[i] * 255 + 0x4000) >> 15;
			}
		}
	}
}

/** 指定位置の全レイヤ合成後の色を取得 */

void drawImage_getBlendColor_atPoint(DrawData *p,int x,int y,RGBAFix15 *dst)
{
	LayerItem *pi;
	RGBFix15 pix;

	pix.r = pix.g = pix.b = 0x8000;

	pi = LayerList_getItem_bottomVisibleImage(p->layerlist);

	for( ; pi; pi = LayerItem_getPrevVisibleImage(pi))
	{
		TileImage_getBlendPixel(pi->img, x, y, &pix,
			LayerItem_getOpacity_real(pi), pi->blendmode, pi->img8texture);
	}

	dst->r = pix.r;
	dst->g = pix.g;
	dst->b = pix.b;
	dst->a = 0x8000;
}
