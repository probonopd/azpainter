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

#ifndef MLIB_PSD_SAVE_H
#define MLIB_PSD_SAVE_H

#include "mPSDDef.h"

typedef struct _mPSDSave mPSDSave;

typedef struct
{
	int width,
		height,
		img_channels,
		bits,		//1,8,16
		colmode;
}mPSDSaveInfo;

typedef struct
{
	int top,left,bottom,right,
		channels;
	uint32_t blendmode;
	const char *name;
	uint8_t opacity,
		hide;
}mPSDSaveLayerInfo;


#ifdef __cplusplus
extern "C" {
#endif

mPSDSave *mPSDSave_openFile(const char *filename,mPSDSaveInfo *info,mBool packbits);
void mPSDSave_close(mPSDSave *p);

uint8_t *mPSDSave_getLineImageBuf(mPSDSave *p);

void mPSDSave_beginResource(mPSDSave *p);
void mPSDSave_writeResource_resolution_dpi(mPSDSave *p,int dpi_horz,int dpi_vert);
void mPSDSave_writeResource_currentLayer(mPSDSave *p,int no);
void mPSDSave_endResource(mPSDSave *p);

void mPSDSave_writeLayerNone(mPSDSave *p);

mBool mPSDSave_beginLayer(mPSDSave *p,int num);
void mPSDSave_writeLayerInfo(mPSDSave *p,mPSDSaveLayerInfo *info);
mBool mPSDSave_endLayerInfo(mPSDSave *p);
mBool mPSDSave_beginLayerImage(mPSDSave *p,mBox *box);
void mPSDSave_beginLayerImageChannel(mPSDSave *p,int channel_id);
void mPSDSave_writeLayerImageChannelLine(mPSDSave *p);
void mPSDSave_endLayerImageChannel(mPSDSave *p);
void mPSDSave_endLayerImage(mPSDSave *p);
void mPSDSave_endLayer(mPSDSave *p);

int mPSDSave_getLayerImageMaxWidth(mPSDSave *p);

mBool mPSDSave_beginImage(mPSDSave *p);
void mPSDSave_beginImageChannel(mPSDSave *p);
void mPSDSave_writeImageChannelLine(mPSDSave *p);
void mPSDSave_endImageChannel(mPSDSave *p);

#ifdef __cplusplus
}
#endif

#endif
