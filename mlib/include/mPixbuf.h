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

#ifndef MLIB_PIXBUF_H
#define MLIB_PIXBUF_H

typedef struct _mLoadImageSource mLoadImageSource;
typedef struct _mImageBuf mImageBuf;

struct _mPixbuf
{
	uint8_t *buf,*buftop;
	int w,h,bpp,pitch,pitch_dir;
	void (*setbuf)(uint8_t *,mPixCol);
};


#define MPIXBUF_COL_XOR  0xffffffff

enum MPIXBUF_DRAWBTT_FLAGS
{
	MPIXBUF_DRAWBTT_PRESS = 1<<0,
	MPIXBUF_DRAWBTT_FOCUS = 1<<1,
	MPIXBUF_DRAWBTT_DEFAULT_BUTTON = 1<<2,
	MPIXBUF_DRAWBTT_DISABLE = 1<<3
};

enum MPIXBUF_DRAWCKBOX_FLAGS
{
	MPIXBUF_DRAWCKBOX_CHECKED = 1<<0,
	MPIXBUF_DRAWCKBOX_RADIO = 1<<1,
	MPIXBUF_DRAWCKBOX_DISABLE = 1<<2
};

/*---------*/

#ifdef __cplusplus
extern "C" {
#endif

void mPixbufFree(mPixbuf *p);
mPixbuf *mPixbufCreate(int w,int h);
int mPixbufResizeStep(mPixbuf *p,int w,int h,int stepw,int steph);

mPixbuf *mPixbufLoadImage(mLoadImageSource *src,mDefEmptyFunc loadfunc,char **errmes);

void mPixbufRenderWindow(mPixbuf *p,mWindow *win,mBox *box);

uint8_t *mPixbufGetBufPt(mPixbuf *p,int x,int y);
uint8_t *mPixbufGetBufPtFast(mPixbuf *p,int x,int y);
uint8_t *mPixbufGetBufPt_abs(mPixbuf *p,int x,int y);
uint8_t *mPixbufGetBufPtFast_abs(mPixbuf *p,int x,int y);
mRgbCol mPixbufGetPixelRGB(mPixbuf *p,int x,int y);
void mPixbufGetPixelBufRGB(mPixbuf *p,uint8_t *buf,int *pr,int *pg,int *pb);

void mPixbufSetOffset(mPixbuf *p,int x,int y,mPoint *pt);

void mPixbufClipNone(mPixbuf *p);
mBool mPixbufSetClipBox_d(mPixbuf *p,int x,int y,int w,int h);
mBool mPixbufSetClipBox_box(mPixbuf *p,mBox *box);

mBool mPixbufIsInClip_abs(mPixbuf *p,int x,int y);
mBool mPixbufGetClipRect_d(mPixbuf *p,mRect *rc,int x,int y,int w,int h);
mBool mPixbufGetClipBox_d(mPixbuf *p,mBox *box,int x,int y,int w,int h);
mBool mPixbufGetClipBox_box(mPixbuf *p,mBox *box,mBox *boxsrc);

void mPixbufRawLineH(mPixbuf *p,uint8_t *buf,int w,mPixCol pix);
void mPixbufRawLineV(mPixbuf *p,uint8_t *buf,int h,mPixCol pix);

void mPixbufFill(mPixbuf *p,mPixCol pix);
void mPixbufSetPixel(mPixbuf *p,int x,int y,mPixCol pix);
void mPixbufSetPixelRGB(mPixbuf *p,int x,int y,mRgbCol rgb);
void mPixbufSetPixels(mPixbuf *p,mPoint *pt,int ptcnt,mPixCol pix);
void mPixbufSetPixel_blend128(mPixbuf *p,int x,int y,uint32_t col);
void mPixbufSetPixelBuf_blend128(mPixbuf *p,uint8_t *buf,uint32_t col);

void mPixbufLineH(mPixbuf *p,int x,int y,int w,mPixCol pix);
void mPixbufLineV(mPixbuf *p,int x,int y,int h,mPixCol pix);
void mPixbufLineH_blend(mPixbuf *p,int x,int y,int w,mRgbCol rgb,int opacity);
void mPixbufLineV_blend(mPixbuf *p,int x,int y,int h,mRgbCol rgb,int opacity);

void mPixbufLine(mPixbuf *p,int x1,int y1,int x2,int y2,mPixCol pix);
void mPixbufLine_noend(mPixbuf *p,int x1,int y1,int x2,int y2,mPixCol pix);
void mPixbufLines(mPixbuf *p,mPoint *pt,int num,mPixCol pix);
void mPixbufBox(mPixbuf *p,int x,int y,int w,int h,mPixCol pix);
void mPixbufBoxSlow(mPixbuf *p,int x,int y,int w,int h,mPixCol pix);
void mPixbufBoxDash(mPixbuf *p,int x,int y,int w,int h,mPixCol pix);
void mPixbufFillBox(mPixbuf *p,int x,int y,int w,int h,mPixCol pix);
void mPixbufFillBox_pat2x2(mPixbuf *p,int x,int y,int w,int h,mPixCol pix);
void mPixbufFillBox_checkers4x4_originx(mPixbuf *p,int x,int y,int w,int h,mPixCol pix1,mPixCol pix2);
void mPixbufEllipse(mPixbuf *p,int x1,int y1,int x2,int y2,mPixCol pix);

/*------*/

void mPixbufBlt(mPixbuf *dst,int dx,int dy,mPixbuf *src,int sx,int sy,int w,int h);

void mPixbufBltFromGray(mPixbuf *p,int dx,int dy,
	const uint8_t *srcimg,int sx,int sy,int w,int h,int srcw);
void mPixbufBltFrom1bit(mPixbuf *p,int dx,int dy,
	const uint8_t *pat,int sx,int sy,int w,int h,int patw,mPixCol col_1,mPixCol col_0);
void mPixbufBltFrom2bit(mPixbuf *p,int dx,int dy,
	const uint8_t *pat,int sx,int sy,int w,int h,int patw,mPixCol *col);

/*------*/

void mPixbufDraw3DFrame(mPixbuf *p,int x,int y,int w,int h,mPixCol col_lt,mPixCol col_dk);

void mPixbufDrawBitPattern(mPixbuf *p,int x,int y,const uint8_t *pat,int patw,int path,mPixCol col);
void mPixbufDrawBitPatternSum(mPixbuf *p,int x,int y,
	const uint8_t *pat,int patw,int path,int eachw,const uint8_t *number,mPixCol col);
void mPixbufDraw2bitPattern(mPixbuf *p,int x,int y,const uint8_t *pat,int patw,int path,mPixCol *pcol);
void mPixbufDraw2BitPatternSum(mPixbuf *p,int x,int y,
	const uint8_t *pat,int patw,int path,int eachw,const uint8_t *number,mPixCol *pcol);

void mPixbufDrawNumber_5x7(mPixbuf *p,int x,int y,const char *text,mPixCol col);

void mPixbufDrawChecked(mPixbuf *p,int x,int y,mPixCol col);
void mPixbufDrawMenuChecked(mPixbuf *p,int x,int y,mPixCol col);
void mPixbufDrawMenuRadio(mPixbuf *p,int x,int y,mPixCol col);
void mPixbufDrawButton(mPixbuf *p,int x,int y,int w,int h,uint32_t flags);
void mPixbufDrawCheckBox(mPixbuf *p,int x,int y,uint32_t flags);

void mPixbufDrawArrowDown(mPixbuf *p,int ctx,int cty,mPixCol col);
void mPixbufDrawArrowUp(mPixbuf *p,int ctx,int cty,mPixCol col);
void mPixbufDrawArrowLeft(mPixbuf *p,int ctx,int cty,mPixCol col);
void mPixbufDrawArrowRight(mPixbuf *p,int ctx,int cty,mPixCol col);

void mPixbufDrawArrowDown_small(mPixbuf *p,int ctx,int y,mPixCol col);
void mPixbufDrawArrowUp_small(mPixbuf *p,int ctx,int y,mPixCol col);

void mPixbufDrawArrowDown_7x4(mPixbuf *p,int x,int y,mPixCol col);
void mPixbufDrawArrowRight_4x7(mPixbuf *p,int x,int y,mPixCol col);

void mPixbufDrawArrowDown_13x7(mPixbuf *p,int ctx,int cty,mPixCol col);
void mPixbufDrawArrowUp_13x7(mPixbuf *p,int ctx,int cty,mPixCol col);
void mPixbufDrawArrowLeft_7x13(mPixbuf *p,int ctx,int cty,mPixCol col);
void mPixbufDrawArrowRight_7x13(mPixbuf *p,int ctx,int cty,mPixCol col);

/*------*/

void mPixbufBltImageBuf(mPixbuf *dst,int dx,int dy,mImageBuf *src,int sx,int sy,int w,int h);
void mPixbufTileImageBuf(mPixbuf *dst,int x,int y,int w,int h,mImageBuf *src);
void mPixbufScaleImageBuf_oversamp(mPixbuf *dst,int x,int y,int w,int h,mImageBuf *src,int overnum);

#ifdef __cplusplus
}
#endif

#endif
