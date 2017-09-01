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
 * GIF 保存
 *****************************************/

#include <stdio.h>

#include "mDef.h"
#include "mSaveImage.h"
#include "mGIFEncode.h"


//--------------------

typedef struct
{
	mSaveImage *param;

	mGIFEncode *gifenc;
	uint8_t *rowbuf,
		*palbuf;

	int transparent;
}SAVEGIF;

//--------------------


/** 初期化 */

static int _init(SAVEGIF *p,mSaveImage *param)
{
	mGIFEncodeGlobalInfo global;
	mGIFEncodeImage img;
	int ret;

	//パレット

	p->palbuf = mSaveImage_convertPalette_RGB(param);
	if(!p->palbuf) return MSAVEIMAGE_ERR_ALLOC;

	//GIF エンコーダ作成

	global.width = param->width;
	global.height = param->height;
	global.bkgnd_index = 0;
	global.bits = mGIFEncode_colnumToBits(param->palette_num);
	global.palette_buf = p->palbuf;
	global.palette_num = param->palette_num;

	p->gifenc = mGIFEncode_create(&global);
	if(!p->gifenc) return MSAVEIMAGE_ERR_ALLOC;

	//開く

	switch(param->output.type)
	{
		case MSAVEIMAGE_OUTPUT_TYPE_PATH:
			if(!mGIFEncode_open_filename(p->gifenc, param->output.filename))
				return MSAVEIMAGE_ERR_OPENFILE;
			break;
		case MSAVEIMAGE_OUTPUT_TYPE_STDIO:
			mGIFEncode_open_stdio(p->gifenc, param->output.fp);
			break;
	}

	//ヘッダ

	mGIFEncode_writeHeader(p->gifenc);

	//設定

	p->transparent = -1;

	if((ret = (param->set_option)(param)))
		return ret;

	//画像制御ブロック (透過色)

	if(p->transparent >= 0)
		mGIFEncode_writeGrpCtrl(p->gifenc, p->transparent, 0, 0, FALSE);

	//画像開始

	img.offsetx = img.offsety = 0;
	img.width = param->width;
	img.height = param->height;
	img.palette_buf = NULL;
	img.palette_num = 0;

	mGIFEncode_startImage(p->gifenc, &img);

	//Y1行バッファ確保

	p->rowbuf = (uint8_t *)mMalloc(param->width, FALSE);
	if(!p->rowbuf) return MSAVEIMAGE_ERR_ALLOC;

	return 0;
}

/** メイン処理 */

static int _main_proc(SAVEGIF *p)
{
	mSaveImage *param = p->param;
	int ret,i;

	//初期化

	if((ret = _init(p, param)))
		return ret;

	//画像

	for(i = param->height; i > 0; i--)
	{
		//取得

		ret = (param->send_row)(param, p->rowbuf, param->width);
		if(ret) return ret;

		//書き込み

		mGIFEncode_writeImage(p->gifenc, p->rowbuf, param->width);
	}

	mGIFEncode_endImage(p->gifenc);
	mGIFEncode_writeEnd(p->gifenc);

	return 0;
}


/**

@ingroup saveimage
@{

*/


/** GIF ファイルに保存
 *
 * 入力データは 8bit パレットのみ。 */

mBool mSaveImageGIF(mSaveImage *param)
{
	SAVEGIF *p;
	int ret;

	//確保

	p = (SAVEGIF *)mMalloc(sizeof(SAVEGIF), TRUE);
	if(!p) return FALSE;

	p->param = param;
	param->internal = p;

	//処理

	ret = _main_proc(p);

	if(ret)
	{
		if(ret > 0)
			mSaveImage_setMessage_errno(param, ret);
	}

	//解放

	mGIFEncode_close(p->gifenc);
	
	mFree(p->rowbuf);
	mFree(p->palbuf);
	mFree(p);

	return (ret == 0);
}

/** 透過色のインデックスをセット */

void mSaveImageGIF_setTransparent(mSaveImage *p,int index)
{
	((SAVEGIF *)p->internal)->transparent = index;
}

/* @} */
