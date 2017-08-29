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

#ifndef MLIB_GIFENCODE_H
#define MLIB_GIFENCODE_H

typedef struct _mGIFEncode mGIFEncode;

typedef struct _mGIFEncodeGlobalInfo
{
	uint16_t width,
		height,
		palette_num;
	uint8_t bits,			//1..8
		bkgnd_index;
	uint8_t *palette_buf;  //R-G-B
}mGIFEncodeGlobalInfo;

typedef struct _mGIFEncodeImage
{
	uint16_t offsetx,
		offsety,
		width,
		height;
	uint8_t *palette_buf;
	int palette_num;
}mGIFEncodeImage;


enum MGIFENCODE_BKGNDTYPE
{
	MGIFENCODE_BKGNDTYPE_NONE,
	MGIFENCODE_BKGNDTYPE_KEEP,
	MGIFENCODE_BKGNDTYPE_FILL_BKGND,
	MGIFENCODE_BKGNDTYPE_RESTORE
};


/*---- function -----*/

#ifdef __cplusplus
extern "C" {
#endif

mGIFEncode *mGIFEncode_create(mGIFEncodeGlobalInfo *info);
void mGIFEncode_close(mGIFEncode *p);

mBool mGIFEncode_open_filename(mGIFEncode *p,const char *filename);
mBool mGIFEncode_open_stdio(mGIFEncode *p,void *fp);

int mGIFEncode_colnumToBits(int num);

void mGIFEncode_writeHeader(mGIFEncode *p);
void mGIFEncode_writeGrpCtrl(mGIFEncode *p,int transparent,int bkgnd_type,uint16_t time,mBool user_input);
void mGIFEncode_writeLoop(mGIFEncode *p,uint16_t loopcnt);
void mGIFEncode_writeEnd(mGIFEncode *p);

void mGIFEncode_startImage(mGIFEncode *p,mGIFEncodeImage *img);
void mGIFEncode_writeImage(mGIFEncode *p,uint8_t *buf,int size);
void mGIFEncode_endImage(mGIFEncode *p);

#ifdef __cplusplus
}
#endif

#endif
