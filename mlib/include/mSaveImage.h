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

#ifndef MLIB_SAVEIMAGE_H
#define MLIB_SAVEIMAGE_H

typedef struct _mSaveImage mSaveImage;
typedef mBool (*mSaveImageFunc)(mSaveImage *);

#define M_SAVEIMAGE(p)  ((mSaveImage *)(p))


/*---- enum ----*/

enum MSAVEIMAGE_OUTPUT_TYPE
{
	MSAVEIMAGE_OUTPUT_TYPE_PATH,
	MSAVEIMAGE_OUTPUT_TYPE_STDIO
};

enum MSAVEIMAGE_RESOLITION_UNIT
{
	MSAVEIMAGE_RESOLITION_UNIT_NONE,
	MSAVEIMAGE_RESOLITION_UNIT_ASPECT,
	MSAVEIMAGE_RESOLITION_UNIT_DPI,
	MSAVEIMAGE_RESOLITION_UNIT_DPM
};

enum MSAVEIMAGE_COLTYPE
{
	MSAVEIMAGE_COLTYPE_PALETTE,
	MSAVEIMAGE_COLTYPE_GRAY,
	MSAVEIMAGE_COLTYPE_GRAY_ALPHA,
	MSAVEIMAGE_COLTYPE_RGB,
	MSAVEIMAGE_COLTYPE_RGBA
};

enum MSAVEIMAGE_ERR
{
	MSAVEIMAGE_ERR_OK,
	MSAVEIMAGE_ERR_OPENFILE,
	MSAVEIMAGE_ERR_ALLOC,
	MSAVEIMAGE_ERR_FORMAT,

	MSAVEIMAGE_ERR_MAX
};


/*---- struct ----*/

typedef struct _mSaveImageOutput
{
	int type;
	union
	{
		const char *filename;
		void *fp;
	};
}mSaveImageOutput;

struct _mSaveImage
{
	mSaveImageOutput output;

	uint8_t *palette_buf;	//R-G-B-A
	char *message;
	void *param1,
		*internal;
	
	int send_raw_format,
		sample_bits,	//8,16
		coltype,
		bits,			//palette bits (1,2,4,8)
		width,
		height,
		palette_num,
		resolution_unit,
		resolution_horz,
		resolution_vert;

	int (*set_option)(mSaveImage *p);
	int (*send_row)(mSaveImage *p,uint8_t *buf,int pitch);
};


/*---- function ----*/

#ifdef __cplusplus
extern "C" {
#endif

mSaveImage *mSaveImage_create(int size);
void mSaveImage_free(mSaveImage *p);

void mSaveImage_setMessage(mSaveImage *p,const char *mes);
void mSaveImage_setMessage_errno(mSaveImage *p,int err);

mBool mSaveImage_getResolution_dpm(mSaveImage *p,int *horz,int *vert);
mBool mSaveImage_getResolution_dpi(mSaveImage *p,int *horz,int *vert);

void *mSaveImage_openFILE(mSaveImageOutput *output);
uint8_t *mSaveImage_convertPalette_RGB(mSaveImage *p);


mBool mSaveImageBMP(mSaveImage *p);
void mSaveImageBMP_setBottomUp(mSaveImage *p);

mBool mSaveImagePNG(mSaveImage *p);
void mSaveImagePNG_setCompressionLevel(mSaveImage *p,int level);
void mSaveImagePNG_setTransparent_8bit(mSaveImage *p,uint32_t col);

mBool mSaveImageJPEG(mSaveImage *param);
void mSaveImageJPEG_setQuality(mSaveImage *p,int quality);
void mSaveImageJPEG_setProgression(mSaveImage *p);
void mSaveImageJPEG_setSamplingFactor(mSaveImage *p,int factor);

mBool mSaveImageGIF(mSaveImage *param);
void mSaveImageGIF_setTransparent(mSaveImage *p,int index);

#ifdef __cplusplus
}
#endif

#endif
