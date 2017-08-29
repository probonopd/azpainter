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

#ifndef MLIB_LOADIMAGE_H
#define MLIB_LOADIMAGE_H

typedef struct _mIOFileBuf mIOFileBuf;
typedef struct _mLoadImage mLoadImage;
typedef mBool (*mLoadImageFunc)(mLoadImage *);

#define M_LOADIMAGE(p)   ((mLoadImage *)(p))


/*---- enum ----*/

enum MLOADIMAGE_FORMAT
{
	MLOADIMAGE_FORMAT_RAW,
	MLOADIMAGE_FORMAT_RGB,
	MLOADIMAGE_FORMAT_RGBA
};

enum MLOADIMAGE_FLAGS
{
	MLOADIMAGE_FLAGS_ALLOW_16BIT = 1<<0,
	MLOADIMAGE_FLAGS_LEAVE_TRANSPARENT = 1<<1
};

enum MLOADIMAGE_SOURCE_TYPE
{
	MLOADIMAGE_SOURCE_TYPE_PATH,
	MLOADIMAGE_SOURCE_TYPE_BUF,
	MLOADIMAGE_SOURCE_TYPE_STDIO
};

enum MLOADIMAGE_RESOLITION_UNIT
{
	MLOADIMAGE_RESOLITION_UNIT_NONE,
	MLOADIMAGE_RESOLITION_UNIT_UNKNOWN,
	MLOADIMAGE_RESOLITION_UNIT_ASPECT,
	MLOADIMAGE_RESOLITION_UNIT_DPI,
	MLOADIMAGE_RESOLITION_UNIT_DPCM,
	MLOADIMAGE_RESOLITION_UNIT_DPM
};

enum MLOADIMAGE_COLTYPE
{
	MLOADIMAGE_COLTYPE_PALETTE,
	MLOADIMAGE_COLTYPE_GRAY,
	MLOADIMAGE_COLTYPE_GRAY_ALPHA,
	MLOADIMAGE_COLTYPE_RGB,
	MLOADIMAGE_COLTYPE_RGBA
};

enum MLOADIMAGE_CHECKFORMAT_FLAGS
{
	MLOADIMAGE_CHECKFORMAT_F_BMP  = 1<<0,
	MLOADIMAGE_CHECKFORMAT_F_PNG  = 1<<1,
	MLOADIMAGE_CHECKFORMAT_F_JPEG = 1<<2,
	MLOADIMAGE_CHECKFORMAT_F_GIF  = 1<<3,

	MLOADIMAGE_CHECKFORMAT_F_ALL = 0xff
};


/*---- error ----*/

enum MLOADIMAGE_ERR
{
	MLOADIMAGE_ERR_OK,
	MLOADIMAGE_ERR_OPENFILE,
	MLOADIMAGE_ERR_CORRUPT,
	MLOADIMAGE_ERR_UNSUPPORTED,
	MLOADIMAGE_ERR_INVALID_VALUE,
	MLOADIMAGE_ERR_ALLOC,
	MLOADIMAGE_ERR_DECODE,
	MLOADIMAGE_ERR_LARGE_SIZE,
	MLOADIMAGE_ERR_NONE_IMAGE,

	MLOADIMAGE_ERR_MAX
};


/*---- struct ----*/

typedef struct _mLoadImageSource
{
	int type;
	uint32_t bufsize;
	union
	{
		const char *filename;
		void *fp;
		const void *buf;
	};
}mLoadImageSource;

typedef struct _mLoadImageInfo
{
	int width,
		height,
		palette_num,
		raw_pitch,
		resolution_unit,
		resolution_horz,
		resolution_vert,
		transparent_col;	//-1=none
	uint8_t bottomup,
		sample_bits,		//8,16
		raw_sample_bits,	//8,16
		raw_bits,			//(for 8bit) 0,1,2,4,8,16,24,32
		raw_coltype;
	uint8_t *palette_buf;	//R-G-B-A 4byte x palette_num
}mLoadImageInfo;

struct _mLoadImage
{
	int format,
		max_width,		//0=disable
		max_height;
	uint32_t flags;
	mLoadImageSource src;
	mLoadImageInfo info;
	char *message;		//error message
	void *param1;

	int (*getinfo)(mLoadImage *,mLoadImageInfo *);
	int (*getrow)(mLoadImage *,uint8_t *buf,int pitch);
};


/*---- function ----*/

#ifdef __cplusplus
extern "C" {
#endif

mLoadImage *mLoadImage_create(int size);
void mLoadImage_free(mLoadImage *p);

void mLoadImage_setMessage(mLoadImage *p,const char *mes);
void mLoadImage_setMessage_errno(mLoadImage *p,int err);
const char *mLoadImage_getErrMessage(int err);

mIOFileBuf *mLoadImage_openIOFileBuf(mLoadImageSource *src);
int mLoadImage_getPitch(mLoadImage *p);
mBool mLoadImage_getResolution_dpi(mLoadImage *p,int *phorz,int *pvert);

mBool mLoadImage_checkFormat(mLoadImageSource *src,mLoadImageFunc *pfunc,uint32_t flags);

mBool mLoadImageBMP(mLoadImage *param);
mBool mLoadImagePNG(mLoadImage *param);
mBool mLoadImageGIF(mLoadImage *param);
mBool mLoadImageJPEG(mLoadImage *param);

#ifdef __cplusplus
}
#endif

#endif
