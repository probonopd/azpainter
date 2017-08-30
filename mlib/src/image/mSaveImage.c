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
 * mSaveImage
 *****************************************/

#include <stdio.h>

#include "mDef.h"
#include "mSaveImage.h"
#include "mUtilStdio.h"
#include "mUtil.h"


/**************//**

@defgroup saveimage mSaveImage
@brief 画像フォーマット保存

イメージデータを渡す場合は R-G-B-[A] の順。

@ingroup group_image
@{

@file mSaveImage.h

@struct _mSaveImage
@struct mSaveImageOutput

@enum MSAVEIMAGE_OUTPUT_TYPE
@enum MSAVEIMAGE_RESOLITION_UNIT
@enum MSAVEIMAGE_COLTYPE
@enum MSAVEIMAGE_ERR

********************/



/** mSaveImage 構造体作成 */

mSaveImage *mSaveImage_create(int size)
{
	if(size < sizeof(mSaveImage))
		size = sizeof(mSaveImage);

	return (mSaveImage *)mMalloc(size, TRUE);
}

/** mSaveImage 解放 */

void mSaveImage_free(mSaveImage *p)
{
	if(p)
	{
		mFree(p->message);
		mFree(p);
	}
}

/** エラー文字列セット */

void mSaveImage_setMessage(mSaveImage *p,const char *mes)
{
	mFree(p->message);

	p->message = mStrdup(mes);
}

/** エラー番号からエラー文字列セット */

void mSaveImage_setMessage_errno(mSaveImage *p,int err)
{
	const char *mes[] = {
		"file could not be opened",
		"memory allocation to fail",
		"can not write this format"
	};

	if(err > 0 && err < MSAVEIMAGE_ERR_MAX)
		mSaveImage_setMessage(p, mes[err - 1]);
}

/** mSaveImage の解像度を dpm 単位で取得
 *
 * @return FALSE で dpm で取得できなかった */

mBool mSaveImage_getResolution_dpm(mSaveImage *p,int *horz,int *vert)
{
	int h,v;

	h = p->resolution_horz;
	v = p->resolution_vert;
	
	switch(p->resolution_unit)
	{
		case MSAVEIMAGE_RESOLITION_UNIT_DPI:
			*horz = mDPItoDPM(h);
			*vert = mDPItoDPM(v);
			return TRUE;
		case MSAVEIMAGE_RESOLITION_UNIT_DPM:
			*horz = h;
			*vert = v;
			return TRUE;
	}

	return FALSE;
}

/** mSaveImage の解像度を dpi 単位で取得
 *
 * @return FALSE で dpi で取得できなかった */

mBool mSaveImage_getResolution_dpi(mSaveImage *p,int *horz,int *vert)
{
	int h,v;

	h = p->resolution_horz;
	v = p->resolution_vert;
	
	switch(p->resolution_unit)
	{
		case MSAVEIMAGE_RESOLITION_UNIT_DPI:
			*horz = h;
			*vert = v;
			return TRUE;
		case MSAVEIMAGE_RESOLITION_UNIT_DPM:
			*horz = mDPMtoDPI(h);
			*vert = mDPMtoDPI(v);
			return TRUE;
	}

	return FALSE;
}

/** mSaveImageOutput から FILE* を取得 */

void *mSaveImage_openFILE(mSaveImageOutput *output)
{
	switch(output->type)
	{
		case MSAVEIMAGE_OUTPUT_TYPE_PATH:
			return mFILEopenUTF8(output->filename, "wb");
		case MSAVEIMAGE_OUTPUT_TYPE_STDIO:
			return output->fp;
	}

	return NULL;
}

/** パレットデータを R-G-B (3byte) のバッファに変換して作成 */

uint8_t *mSaveImage_convertPalette_RGB(mSaveImage *p)
{
	int num,i;
	uint8_t *buf,*ps,*pd;

	if(!p->palette_buf) return NULL;

	num = p->palette_num;

	buf = (uint8_t *)mMalloc(num * 3, FALSE);
	if(!buf) return NULL;

	//RGBA => RGB 変換

	ps = p->palette_buf;
	pd = buf;

	for(i = num; i > 0; i--, ps += 4, pd += 3)
	{
		pd[0] = ps[0];
		pd[1] = ps[1];
		pd[2] = ps[2];
	}

	return buf;
}
