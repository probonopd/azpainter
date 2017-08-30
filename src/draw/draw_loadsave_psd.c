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
 * PSD 読み書き
 *****************************************/

#include <string.h>

#include "mDef.h"
#include "mPopupProgress.h"
#include "mPSDLoad.h"
#include "mPSDSave.h"
#include "mUtilFile.h"

#include "defMacros.h"
#include "defDraw.h"
#include "defConfig.h"

#include "TileImage.h"
#include "LayerList.h"
#include "LayerItem.h"
#include "ImageBufRGB16.h"

#include "draw_main.h"


//--------------------

enum
{
	_TYPE_RGB = 1,
	_TYPE_GRAY,
	_TYPE_MONO
};

#define _TOGRAY(r,g,b)  ((r * 77 + g * 150 + b * 29) >> 8)

//--------------------

static const uint32_t g_blendmode[] = {
	MPSD_BLENDMODE_NORMAL, MPSD_BLENDMODE_MULTIPLY, MPSD_BLENDMODE_LINEAR_DODGE,
	MPSD_BLENDMODE_SUBTRACT, MPSD_BLENDMODE_SCREEN, MPSD_BLENDMODE_OVERLAY,
	MPSD_BLENDMODE_HARD_LIGHT, MPSD_BLENDMODE_SOFT_LIGHT, MPSD_BLENDMODE_DODGE,
	MPSD_BLENDMODE_BURN, MPSD_BLENDMODE_LINEAR_BURN, MPSD_BLENDMODE_VIVID_LIGHT,
	MPSD_BLENDMODE_LINEAR_LIGHT, MPSD_BLENDMODE_PIN_LIGHT, MPSD_BLENDMODE_DARKEN,
	MPSD_BLENDMODE_LIGHTEN, MPSD_BLENDMODE_DIFFERENCE, 0
};

//--------------------


//=============================
// 読み込み
//=============================


/** レイヤ読み込み - チャンネルを読み込み、バッファにセット */

static mBool _load_layer_image_channel(mPSDLoad *psd,mBox *boximg,mPopupProgress *prog,uint8_t *dstbuf)
{
	int i,w;
	uint8_t *ps;

	w = boximg->w;
	ps = mPSDLoad_getLineImageBuf(psd);

	for(i = boximg->h; i > 0; i--)
	{
		if(!mPSDLoad_readLayerImageChannelLine(psd))
			return FALSE;

		memcpy(dstbuf, ps, w);
		dstbuf += w;

		mPopupProgressThreadIncSubStep(prog);
	}

	return TRUE;
}

/** レイヤ読み込み - アルファ値読み込み
 *
 * @return FALSE 時は、psd->err にエラー番号を入れておく */

static mBool _load_layer_image_alpha(mPSDLoad *psd,
	TileImage *img,mPSDLoadLayerInfo *info,mPopupProgress *prog,uint8_t *buf)
{
	uint8_t *ps,*pd;
	int ret,ix,iy;
	mBox box;

	box = info->box_img;

	//アルファチャンネルを buf にセット

	ret = mPSDLoad_beginLayerImageChannel(psd, MPSD_CHANNEL_ID_ALPHA);
	if(!ret) return FALSE;

	if(ret == -1)
	{
		//チャンネルがない場合はすべて不透明

		memset(buf, 255, box.w * box.h);
		mPopupProgressThreadAddPos(prog, 4);
	}
	else
	{
		//読み込み
		
		mPopupProgressThreadBeginSubStep(prog, 4, box.h);

		if(!_load_layer_image_channel(psd, &box, prog, buf))
			return FALSE;
	}

	//レイヤマスクがある場合、アルファ値に適用

	if(!(info->layermask_flags & MPSDLOAD_LAYERMASK_F_DISABLE))
	{
		ret = mPSDLoad_beginLayerImageChannel(psd, MPSD_CHANNEL_ID_MASK);
		if(!ret) return FALSE;

		if(ret != -1)
		{
			pd = buf;
		
			for(iy = box.h; iy > 0; iy--)
			{
				if(!mPSDLoad_readLayerImageChannelLine_mask(psd))
					return FALSE;

				ps = mPSDLoad_getLineImageBuf(psd);

				for(ix = box.w; ix > 0; ix--, ps++, pd++)
				{
					if(*pd > *ps) *pd = *ps;
				}
			}
		}
	}

	//アルファ値セット

	if(TileImage_setChannelImage(img, buf, -1, box.w, box.h))
		return TRUE;
	else
	{
		psd->err = MPSDLOAD_ERR_ALLOC;
		return FALSE;
	}
}

/** レイヤ読み込み - イメージ (RGBA/GRAY) */

static int _load_layer_image(DrawData *p,mPSDLoad *psd,mPopupProgress *prog)
{
	mPSDLoadLayerInfo info;
	LayerItem *item;
	TileImage *img;
	uint8_t *allocbuf = NULL,*buf;
	mSize size;
	int i,chnum,ret;

	chnum = (psd->colmode == MPSD_COLMODE_GRAYSCALE)? 1: 3;

	//1チャンネル分のバッファ確保
	//(blendimg のバッファで足りるならそちらを使う)

	mPSDLoad_getLayerImageMaxSize(psd, &size);

	if(size.w * size.h <= p->imgw * p->imgh * 6)
		buf = (uint8_t *)p->blendimg->buf;
	else
	{
		buf = allocbuf = (uint8_t *)mMalloc(size.w * size.h, FALSE);
		if(!allocbuf) return MPSDLOAD_ERR_ALLOC;
	}

	//---- 各レイヤ読み込み
	/* ファイルの格納順に読み込むので、
	 * 一覧上において下から順にイメージのあるレイヤを読み込み。 */

	mPopupProgressThreadSetMax(prog,
		LayerList_getNormalLayerNum(p->layerlist) * 16);

	item = LayerList_getItem_bottomNormal(p->layerlist);

	for(; item; item = LayerItem_getPrevNormal(item))
	{
		//読み込み開始
		
		if(!mPSDLoad_beginLayerImage(psd, &info, TRUE))
		{
			//空イメージ
			
			mPSDLoad_endLayerImage(psd);
			mPopupProgressThreadAddPos(prog, 16);
			continue;
		}

		img = item->img;

		//A チャンネルを先に読み込み

		if(!_load_layer_image_alpha(psd, img, &info, prog, buf))
			goto ERR;

		//色チャンネル読み込み

		for(i = 0; i < chnum; i++)
		{
			//開始
			
			ret = mPSDLoad_beginLayerImageChannel(psd, i);
			if(!ret)
				goto ERR;
			else if(ret == -1)
			{
				mPopupProgressThreadAddPos(prog, 4);
				continue;
			}

			//buf に読み込み

			mPopupProgressThreadBeginSubStep(prog, (chnum == 1)? 12: 4, info.box_img.h);

			if(!_load_layer_image_channel(psd, &info.box_img, prog, buf))
				goto ERR;

			//セット

			TileImage_setChannelImage(img, buf, i, info.box_img.w, info.box_img.h);
		}

		mPSDLoad_endLayerImage(psd);
	}

	mFree(allocbuf);

	return MPSDLOAD_ERR_OK;

ERR:
	mFree(allocbuf);
	return psd->err;
}

/** レイヤ読み込み */

static int _load_from_layer(DrawData *p,mPSDLoad *psd,mPopupProgress *prog)
{
	mPSDLoadLayerInfo info;
	TileImageInfo tinfo;
	LayerItem *item,*item_parent;
	TileImage *img;
	int layerno,i,coltype;

	//------ レイヤ作成

	item_parent = NULL;
	coltype = (psd->colmode == MPSD_COLMODE_GRAYSCALE)? TILEIMAGE_COLTYPE_GRAY: TILEIMAGE_COLTYPE_RGBA;

	//グループレイヤ構造を維持するため、一覧上において上から順に作成する

	for(layerno = psd->layernum - 1; layerno >= 0; layerno--)
	{
		if(!mPSDLoad_getLayerInfo(psd, layerno, &info))
			return psd->err;

	/*
		mDebug("%s : (%d,%d-%dx%d) m(%d,%d-%dx%d)\n",
			info.name,
			info.box_img.x, info.box_img.y, info.box_img.w, info.box_img.h,
			info.box_mask.x, info.box_mask.y, info.box_mask.w, info.box_mask.h);
	*/

		//フォルダ終了

		if(info.group == MPSDLOAD_LAYERGROUP_END)
		{
			if(item_parent) item_parent = LAYERITEM(item_parent->i.parent);
			continue;
		}
		
		//レイヤ作成

		item = LayerList_addLayer(p->layerlist, NULL);
		if(!item) return MPSDLOAD_ERR_ALLOC;

		//位置移動 (親の最後に)

		LayerList_moveitem_forLoad_topdown(p->layerlist, item, item_parent);

		//名前 (UTF-8 とみなす)

		LayerList_setItemName_utf8(p->layerlist, item, info.name);

		//不透明度

		item->opacity = (int)(info.opacity / 255.0 * 128.0 + 0.5);

		//非表示

		if(info.flags & MPSDLOAD_LAYERINFO_F_HIDE)
			item->flags &= ~LAYERITEM_F_VISIBLE;

		//フォルダ

		if(info.group == MPSDLOAD_LAYERGROUP_EXPAND
			|| info.group == MPSDLOAD_LAYERGROUP_FOLDED)
		{
			item_parent = item;
			item->flags |= LAYERITEM_F_FOLDER_EXPAND;
			continue;
		}

		//イメージ作成

		if(info.box_img.w == 0 || info.box_img.h == 0)
		{
			//空イメージ
			tinfo.offx = tinfo.offy = 0;
			tinfo.tilew = tinfo.tileh = 1;
		}
		else
		{
			tinfo.offx = info.box_img.x;
			tinfo.offy = info.box_img.y;
			tinfo.tilew = (info.box_img.w + 63) / 64;
			tinfo.tileh = (info.box_img.h + 63) / 64;
		}

		img = TileImage_newFromInfo(coltype, &tinfo);
		if(!img) return MPSDLOAD_ERR_ALLOC;

		LayerItem_replaceImage(item, img);

		//合成モード

		for(i = 0; g_blendmode[i]; i++)
		{
			if(g_blendmode[i] == info.blendmode)
			{
				item->blendmode = i;
				break;
			}
		}
	}

	//------ イメージ読み込み

	return _load_layer_image(p, psd, prog);
}

/** 一枚絵イメージから読み込み */

static int _load_from_image(DrawData *p,mPSDLoad *psd,mPopupProgress *prog)
{
	LayerItem *item;
	TileImage *img;
	int type,i,ix,iy,w,h,chnum;
	uint8_t *ps,*pd,f;

	w = psd->width;
	h = psd->height;

	//カラータイプ
	/* 1bit 白黒、8bit グレースケール => GRAY
	 * RGB(A) 8bit => RGBA */

	if(psd->bits == 1)
		type = _TYPE_MONO;
	else if(psd->colmode == MPSD_COLMODE_GRAYSCALE)
		type = _TYPE_GRAY;
	else if(psd->colmode == MPSD_COLMODE_RGB && psd->img_channels >= 3)
		type = _TYPE_RGB;
	else
		return MPSDLOAD_ERR_UNSUPPORTED;

	//レイヤ作成

	item = LayerList_addLayer(p->layerlist, NULL);
	if(!item) return MPSDLOAD_ERR_ALLOC;

	img = TileImage_new(
		(type == _TYPE_RGB)? TILEIMAGE_COLTYPE_RGBA: TILEIMAGE_COLTYPE_GRAY, w, h);

	if(!img) return MPSDLOAD_ERR_ALLOC;

	LayerItem_replaceImage(item, img);
	LayerList_setItemName_curlayernum(p->layerlist, item);

	//------- イメージ

	if(!mPSDLoad_beginImage(psd)) goto PSDERR;

	//mono or gray

	if(type != _TYPE_RGB)
	{
		mPopupProgressThreadSetMax(prog, 40);
		mPopupProgressThreadBeginSubStep(prog, 20, h);

		if(!mPSDLoad_beginImageChannel(psd)) goto PSDERR;
	}

	//blendimg に RGBA 8bit としてセット

	switch(type)
	{
		//1bit 白黒
		case _TYPE_MONO:
			pd = (uint8_t *)p->blendimg->buf;
			
			for(iy = h; iy; iy--)
			{
				if(!mPSDLoad_readImageChannelLine(psd)) goto PSDERR;

				ps = mPSDLoad_getLineImageBuf(psd);

				for(ix = w, f = 0x80; ix; ix--, pd += 4)
				{
					pd[0] = pd[1] = pd[2] = (*ps & f)? 0: 255;
					pd[3] = 255;
				
					f >>= 1;
					if(f == 0) f = 0x80, ps++;
				}

				mPopupProgressThreadIncSubStep(prog);
			}
			break;

		//グレイスケール
		case _TYPE_GRAY:
			pd = (uint8_t *)p->blendimg->buf;
			
			for(iy = h; iy; iy--)
			{
				if(!mPSDLoad_readImageChannelLine(psd)) goto PSDERR;

				ps = mPSDLoad_getLineImageBuf(psd);

				for(ix = w; ix; ix--, pd += 4)
				{
					pd[0] = pd[1] = pd[2] = *(ps++);
					pd[3] = 255;
				}

				mPopupProgressThreadIncSubStep(prog);
			}
			break;

		//RGB,RGBA
		case _TYPE_RGB:
			chnum = psd->img_channels;
			if(chnum > 4) chnum = 4;

			mPopupProgressThreadSetMax(prog, chnum * 10 + 20);

			//RGB(A)

			for(i = 0; i < chnum; i++)
			{
				if(!mPSDLoad_beginImageChannel(psd)) goto PSDERR;

				mPopupProgressThreadBeginSubStep(prog, 10, h);

				pd = (uint8_t *)p->blendimg->buf + i;

				for(iy = h; iy; iy--)
				{
					if(!mPSDLoad_readImageChannelLine(psd)) goto PSDERR;

					ps = mPSDLoad_getLineImageBuf(psd);

					for(ix = w; ix; ix--, pd += 4)
						*pd = *(ps++);

					mPopupProgressThreadIncSubStep(prog);
				}
			}

			//A なしの場合

			if(chnum != 4)
			{
				mPopupProgressThreadBeginSubStep(prog, 10, h);

				pd = (uint8_t *)p->blendimg->buf + 3;

				for(iy = h; iy; iy--)
				{
					for(ix = w; ix; ix--, pd += 4)
						*pd = 255;

					mPopupProgressThreadIncSubStep(prog);
				}
			}
			break;
	}

	//RGBA8 イメージからタイルセット

	TileImage_setImage_fromRGBA8(img, (uint8_t *)p->blendimg->buf, w, h, prog, 20);

	return MPSDLOAD_ERR_OK;

PSDERR:
	return psd->err;
}

/** PSD 読み込み */

mBool drawFile_load_psd(DrawData *p,const char *filename,mPopupProgress *prog,char **errmes)
{
	mPSDLoad *psd;
	mBool ret = FALSE;
	int err = -1,horz,vert;

	/* err: [-1] psd->err [-2] サイズ制限 [それ以外] PSD エラー */

	//作成

	psd = mPSDLoad_new();
	if(!psd)
	{
		err = MPSDLOAD_ERR_ALLOC;
		goto ERR;
	}

	//開く

	if(!mPSDLoad_openFile(psd, filename)) goto ERR;

	//画像サイズ制限

	if(psd->width > IMAGE_SIZE_MAX || psd->height > IMAGE_SIZE_MAX)
	{
		err = -2;
		goto ERR;
	}

	//新規イメージ

	if(!drawImage_new(p, psd->width, psd->height, -1, -1))
	{
		err = MPSDLOAD_ERR_ALLOC;
		goto ERR;
	}

	//画像リソース (DPI)

	if(mPSDLoad_readResource_resolution_dpi(psd, &horz, &vert))
		p->imgdpi = horz;

	//レイヤ or 一枚絵イメージ

	if(mPSDLoad_isHaveLayer(psd))
	{
		//レイヤ読み込み

		if(!mPSDLoad_beginLayer(psd)) goto ERR;

		err = _load_from_layer(p, psd, prog);

		mPSDLoad_endLayer(psd);
	}
	else
	{
		//レイヤなしの場合は一枚絵イメージから読み込み

		err = _load_from_image(p, psd, prog);
	}

	//成功

	if(err == MPSDLOAD_ERR_OK)
	{
		p->curlayer = LayerList_getItem_top(p->layerlist);
		ret = TRUE;
	}

	//---------

ERR:
	//失敗時、エラー文字列
	
	if(!ret)
	{
		if(err == -2)
			*errmes = mStrdup("image size is large");
		else
			*errmes = mStrdup(mPSDLoad_getErrorMessage((err == -1)? psd->err: err));
	}

	mPSDLoad_close(psd);

	return ret;
}


//=============================
// 保存
//=============================


/** 一枚絵イメージ書き込み
 *
 * blendimg に RGB 8bit としてセットされている。 */

static mBool _psdimage_write(DrawData *p,mPSDSave *psd,int type,mPopupProgress *prog)
{
	uint8_t *ps,*pd,*dstbuf,val,f;
	int w,h,i,j,ix,iy;

	if(!mPSDSave_beginImage(psd)) return FALSE;
	
	w = p->imgw;
	h = p->imgh;
	ps = (uint8_t *)p->blendimg->buf;

	dstbuf = mPSDSave_getLineImageBuf(psd);

	switch(type)
	{
		//RGB
		case _TYPE_RGB:
			mPopupProgressThreadBeginSubStep_onestep(prog, 30, h * 3);

			for(i = 0; i < 3; i++)
			{
				mPSDSave_beginImageChannel(psd);

				ps = (uint8_t *)p->blendimg->buf + i;

				for(iy = h; iy > 0; iy--)
				{
					pd = dstbuf;

					for(ix = w; ix > 0; ix--, ps += 3)
						*(pd++) = *ps;

					mPSDSave_writeImageChannelLine(psd);

					mPopupProgressThreadIncSubStep(prog);
				}

				mPSDSave_endImageChannel(psd);
			}
			break;

		//グレイスケール
		case _TYPE_GRAY:
			mPopupProgressThreadBeginSubStep_onestep(prog, 20, h);

			mPSDSave_beginImageChannel(psd);
			
			for(iy = h; iy > 0; iy--)
			{
				pd = dstbuf;

				for(ix = w; ix > 0; ix--, ps += 3)
					*(pd++) = _TOGRAY(ps[0], ps[1], ps[2]);

				mPSDSave_writeImageChannelLine(psd);

				mPopupProgressThreadIncSubStep(prog);
			}

			mPSDSave_endImageChannel(psd);
			break;

		//mono (白=0、それ以外は1)
		case _TYPE_MONO:
			mPopupProgressThreadBeginSubStep_onestep(prog, 20, h);

			mPSDSave_beginImageChannel(psd);
			
			for(iy = h; iy > 0; iy--)
			{
				pd = dstbuf;
				ix = w;

				for(i = (w + 7) >> 3; i > 0; i--)
				{
					val = 0;
					f = 0x80;

					for(j = 8; j && ix; j--, ix--, ps += 3, f >>= 1)
					{
						if(ps[0] != 255 || ps[1] != 255 || ps[2] != 255)
							val |= f;
					}

					*(pd++) = val;
				}

				mPSDSave_writeImageChannelLine(psd);

				mPopupProgressThreadIncSubStep(prog);
			}

			mPSDSave_endImageChannel(psd);
			break;
	}

	return TRUE;
}

/** レイヤ出力 */

static mBool _write_layer(DrawData *p,mPSDSave *psd,int layernum,mPopupProgress *prog)
{
	LayerItem *pi_bottom,*pi;
	mPSDSaveLayerInfo info;
	mRect rc;
	mBox box;
	int ch,ix,iy;
	uint8_t *pd;
	RGBAFix15 pix;

	mPopupProgressThreadSetMax(prog, layernum * 4);

	//書き込む先頭レイヤ

	pi_bottom = LayerList_getItem_bottomNormal(p->layerlist);

	//レイヤ開始

	if(!mPSDSave_beginLayer(psd, layernum)) return FALSE;

	//レイヤ情報

	for(pi = pi_bottom; pi; pi = LayerItem_getPrevNormal(pi))
	{
		mMemzero(&info, sizeof(mPSDSaveLayerInfo));
	
		if(TileImage_getHaveImageRect_pixel(pi->img, &rc, NULL))
		{
			info.left = rc.x1;
			info.top = rc.y1;
			info.right = rc.x2 + 1;
			info.bottom = rc.y2 + 1;
		}

		info.name = pi->name;
		info.opacity = (int)(pi->opacity / 128.0 * 255.0 + 0.5);
		info.hide = !(pi->flags & LAYERITEM_F_VISIBLE);
		info.blendmode = g_blendmode[pi->blendmode];
		info.channels = 4;

		mPSDSave_writeLayerInfo(psd, &info);
	}

	if(!mPSDSave_endLayerInfo(psd)) return FALSE;

	//イメージ

	for(pi = pi_bottom; pi; pi = LayerItem_getPrevNormal(pi))
	{
		if(!mPSDSave_beginLayerImage(psd, &box))
		{
			//空イメージ

			mPopupProgressThreadAddPos(prog, 4);
		}
		else
		{
			//各チャンネル

			for(ch = 0; ch < 4; ch++)
			{
				mPSDSave_beginLayerImageChannel(psd,
					(ch == 3)? MPSD_CHANNEL_ID_ALPHA: ch);
				
				for(iy = 0; iy < box.h; iy++)
				{
					pd = mPSDSave_getLineImageBuf(psd);

					for(ix = 0; ix < box.w; ix++)
					{
						TileImage_getPixel(pi->img, box.x + ix, box.y + iy, &pix);

						*(pd++) = RGBCONV_FIX15_TO_8(pix.c[ch]);
					}

					mPSDSave_writeLayerImageChannelLine(psd);
				}

				mPSDSave_endLayerImageChannel(psd);

				mPopupProgressThreadIncPos(prog);
			}
		}

		mPSDSave_endLayerImage(psd);
	}

	mPSDSave_endLayer(psd);

	return TRUE;
}

/** PSD 保存 (レイヤ維持)
 *
 * [!] フォルダは保存されない。 */

mBool drawFile_save_psd_layer(DrawData *p,const char *filename,mPopupProgress *prog)
{
	mPSDSave *psd;
	mPSDSaveInfo info;
	mBool ret = FALSE;
	int layernum;

	//通常レイヤ数 (フォルダしかない場合はエラー)

	layernum = LayerList_getNormalLayerNum(p->layerlist);

	if(layernum == 0) return FALSE;

	//情報

	info.width = p->imgw;
	info.height = p->imgh;
	info.img_channels = 3;
	info.bits = 8;
	info.colmode = MPSD_COLMODE_RGB;

	//開く

	psd = mPSDSave_openFile(filename, &info,
		((APP_CONF->save.flags & CONFIG_SAVEOPTION_F_PSD_UNCOMPRESSED) == 0));

	if(!psd) return FALSE;

	//画像リソース

	mPSDSave_beginResource(psd);

	mPSDSave_writeResource_resolution_dpi(psd, p->imgdpi, p->imgdpi);
	mPSDSave_writeResource_currentLayer(psd, 0);

	mPSDSave_endResource(psd);

	//レイヤ

	if(_write_layer(p, psd, layernum, prog))
	{
		//一枚絵イメージ

		ret = _psdimage_write(p, psd, _TYPE_RGB, prog);
	}

	mPSDSave_close(psd);

	if(!ret) mDeleteFile(filename);

	return ret;
}

/** PSD 保存 (レイヤなし)
 *
 * @param type 1:8bitRGB 2:grayscale 3:mono */

mBool drawFile_save_psd_image(DrawData *p,int type,const char *filename,mPopupProgress *prog)
{
	mPSDSave *psd;
	mPSDSaveInfo info;
	mBool ret;

	//情報

	info.width = p->imgw;
	info.height = p->imgh;
	info.img_channels = (type == _TYPE_RGB)? 3: 1;
	info.bits = (type == _TYPE_MONO)? 1: 8;

	switch(type)
	{
		case _TYPE_RGB: info.colmode = MPSD_COLMODE_RGB; break;
		case _TYPE_GRAY: info.colmode = MPSD_COLMODE_GRAYSCALE; break;
		default: info.colmode = MPSD_COLMODE_MONO; break;
	}

	//開く

	psd = mPSDSave_openFile(filename, &info,
		((APP_CONF->save.flags & CONFIG_SAVEOPTION_F_PSD_UNCOMPRESSED) == 0));

	if(!psd) return FALSE;

	//画像リソース (DPI)

	mPSDSave_beginResource(psd);

	mPSDSave_writeResource_resolution_dpi(psd, p->imgdpi, p->imgdpi);

	mPSDSave_endResource(psd);
	
	//レイヤなし

	mPSDSave_writeLayerNone(psd);

	//一枚絵イメージ

	ret = _psdimage_write(p, psd, type, prog);

	mPSDSave_close(psd);

	if(!ret) mDeleteFile(filename);

	return ret;
}
