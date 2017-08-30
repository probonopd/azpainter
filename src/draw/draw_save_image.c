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

/************************************
 * DrawData
 *
 * PNG/JPEG/BMP 保存
 ************************************/

#include <string.h>

#include "mDef.h"
#include "mSaveImage.h"
#include "mPopupProgress.h"

#include "defDraw.h"
#include "defConfig.h"
#include "defFileFormat.h"

#include "ImageBufRGB16.h"


//--------------------

typedef struct
{
	mSaveImage base;

	mPopupProgress *prog;
	uint8_t *curbuf;
	int format,
		pitch_src;
}_saveimg_norm;

//--------------------


/** 保存設定 */

static int _norm_set_option(mSaveImage *p)
{
	_saveimg_norm *pp = (_saveimg_norm *)p;
	ConfigSaveOption *opt = &APP_CONF->save;

	if(pp->format & FILEFORMAT_PNG)
	{
		//PNG

		mSaveImagePNG_setCompressionLevel(p, opt->png_complevel);
	}
	else if(pp->format & FILEFORMAT_JPEG)
	{
		//JPEG

		mSaveImageJPEG_setQuality(p, opt->jpeg_quality);
		mSaveImageJPEG_setSamplingFactor(p, opt->jpeg_sampling_factor);

		if(opt->flags & CONFIG_SAVEOPTION_F_JPEG_PROGRESSIVE)
			mSaveImageJPEG_setProgression(p);
	}

	return MSAVEIMAGE_ERR_OK;
}

/** Y1行を送る */

static int _norm_send_row(mSaveImage *info,uint8_t *buf,int pitch)
{
	_saveimg_norm *p = (_saveimg_norm *)info;

	memcpy(buf, p->curbuf, pitch);

	p->curbuf += p->pitch_src;

	mPopupProgressThreadIncSubStep(p->prog);

	return MSAVEIMAGE_ERR_OK;
}

/** blendimg (RGB/RGBA 8bit) を通常画像 (RGB) に保存 */

mBool drawFile_save_image(DrawData *p,const char *filename,int format,mPopupProgress *prog)
{
	_saveimg_norm *info;
	mSaveImageFunc func;
	mBool ret;

	//情報

	info = (_saveimg_norm *)mSaveImage_create(sizeof(_saveimg_norm));
	if(!info) return FALSE;

	info->prog = prog;
	info->curbuf = (uint8_t *)p->blendimg->buf;
	info->format = format;

	info->base.output.type = MSAVEIMAGE_OUTPUT_TYPE_PATH;
	info->base.output.filename = filename;
	info->base.width = p->imgw;
	info->base.height = p->imgh;
	info->base.sample_bits = 8;
	info->base.resolution_unit = MSAVEIMAGE_RESOLITION_UNIT_DPI;
	info->base.resolution_horz = p->imgdpi;
	info->base.resolution_vert = p->imgdpi;
	info->base.send_row = _norm_send_row;
	info->base.set_option = _norm_set_option;

	if(format & FILEFORMAT_ALPHA_CHANNEL)
	{
		info->base.coltype = MSAVEIMAGE_COLTYPE_RGBA;
		info->pitch_src = p->imgw << 2;
	}
	else
	{
		info->base.coltype = MSAVEIMAGE_COLTYPE_RGB;
		info->pitch_src = p->imgw * 3;
	}

	//保存関数

	if(format & FILEFORMAT_PNG)
		func = mSaveImagePNG;
	else if(format & FILEFORMAT_JPEG)
		func = mSaveImageJPEG;
	else
		func = mSaveImageBMP;

	//保存

	mPopupProgressThreadBeginSubStep_onestep(prog, 20, p->imgh);

	ret = (func)(M_SAVEIMAGE(info));

	mSaveImage_free(M_SAVEIMAGE(info));

	return ret;
}

