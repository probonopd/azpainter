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

#ifndef MLIB_PSD_LOAD_H
#define MLIB_PSD_LOAD_H

#include "mPSDDef.h"

typedef struct
{
	int err,
		width,
		height,
		bits,
		img_channels,
		colmode,
		layernum;
}mPSDLoad;

typedef struct
{
	mBox box_img,
		box_mask;
	uint32_t blendmode;
	char *name;
	uint8_t opacity,
		group,
		flags,
		layermask_defcol,
		layermask_flags;
}mPSDLoadLayerInfo;

enum MPSDLOAD_LAYERINFO_FLAGS
{
	MPSDLOAD_LAYERINFO_F_HIDE = 1<<0
};

enum MPSDLOAD_LAYERGROUP
{
	MPSDLOAD_LAYERGROUP_NONE,
	MPSDLOAD_LAYERGROUP_EXPAND,
	MPSDLOAD_LAYERGROUP_FOLDED,
	MPSDLOAD_LAYERGROUP_END
};

enum MPSDLOAD_LAYERMASK_FLAGS
{
	MPSDLOAD_LAYERMASK_F_RELATIVE = 1<<0,
	MPSDLOAD_LAYERMASK_F_DISABLE  = 1<<1,
	MPSDLOAD_LAYERMASK_F_REVERSE  = 1<<2
};

enum MPSDLOAD_ERR
{
	MPSDLOAD_ERR_OK = 0,
	MPSDLOAD_ERR_ALLOC,
	MPSDLOAD_ERR_OPENFILE,
	MPSDLOAD_ERR_NOT_PSD,
	MPSDLOAD_ERR_CORRUPTED,
	MPSDLOAD_ERR_INVALID_VALUE,
	MPSDLOAD_ERR_UNSUPPORTED,

	MPSDLOAD_ERR_NUM
};


#ifdef __cplusplus
extern "C" {
#endif

void mPSDLoad_close(mPSDLoad *p);
mPSDLoad *mPSDLoad_new();

mBool mPSDLoad_openFile(mPSDLoad *p,const char *filename);

mBool mPSDLoad_isHaveLayer(mPSDLoad *p);
uint8_t *mPSDLoad_getLineImageBuf(mPSDLoad *p);
const char *mPSDLoad_getErrorMessage(int err);

mBool mPSDLoad_readResource_currentLayer(mPSDLoad *p,int *pno);
mBool mPSDLoad_readResource_resolution_dpi(mPSDLoad *p,int *horz,int *vert);

mBool mPSDLoad_beginLayer(mPSDLoad *p);
mBool mPSDLoad_getLayerInfo(mPSDLoad *p,int no,mPSDLoadLayerInfo *info);
void mPSDLoad_getLayerImageMaxSize(mPSDLoad *p,mSize *size);
mBool mPSDLoad_beginLayerImage(mPSDLoad *p,mPSDLoadLayerInfo *info,mBool skip_group);
int mPSDLoad_beginLayerImageChannel(mPSDLoad *p,uint16_t channel_id);
mBool mPSDLoad_readLayerImageChannelLine(mPSDLoad *p);
mBool mPSDLoad_readLayerImageChannelLine_mask(mPSDLoad *p);
void mPSDLoad_endLayerImage(mPSDLoad *p);
void mPSDLoad_endLayer(mPSDLoad *p);

mBool mPSDLoad_beginImage(mPSDLoad *p);
mBool mPSDLoad_beginImageChannel(mPSDLoad *p);
mBool mPSDLoad_readImageChannelLine(mPSDLoad *load);

#ifdef __cplusplus
}
#endif

#endif
