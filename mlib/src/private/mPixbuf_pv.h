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

#ifndef MLIB_PIXBUF_PV_H
#define MLIB_PIXBUF_PV_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	uint8_t flags;
	int offsetX,offsetY;
	mRect clip,clipMaster;
}mPixbufPrivate;

typedef struct
{
	mPixbuf b;
	mPixbufPrivate p;
}mPixbufBase;

typedef struct
{
	int dx,dy,sx,sy,w,h;
}mPixbufBltInfo;


#define M_PIXBUFBASE(p)  ((mPixbufBase *)(p))

#define MPIXBUF_F_CLIP        1
#define MPIXBUF_F_CLIP_MASTER 2


void __mPixbufSetClipMaster(mPixbuf *p,mRect *rc);
mBool __mPixbufBltClip(mPixbufBltInfo *p,mPixbuf *dst,int dx,int dy,int sx,int sy,int w,int h);

#ifdef __cplusplus
}
#endif

#endif
