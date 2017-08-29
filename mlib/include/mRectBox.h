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

#ifndef MLIB_RECTBOX_H
#define MLIB_RECTBOX_H

#ifdef __cplusplus
extern "C" {
#endif

mBool mRectIsEmpty(mRect *rc);
void mRectEmpty(mRect *rc);
void mRectSetByPack(mRect *rc,uint32_t val);
void mRectSetBox_d(mRect *rc,int x,int y,int w,int h);
void mRectSetByBox(mRect *rc,mBox *box);
void mRectSetByPoint(mRect *rc,mPoint *pt);
void mRectSetByPoint_minmax(mRect *rc,mPoint *pt1,mPoint *pt2);
void mRectToBox(mBox *box,mRect *rc);
void mRectSwap(mRect *rc);
void mRectSwapTo(mRect *dst,mRect *src);
void mRectUnion(mRect *dst,mRect *src);
void mRectUnion_box(mRect *dst,mBox *box);
mBool mRectClipBox_d(mRect *rc,int x,int y,int w,int h);
mBool mRectClipRect(mRect *rc,mRect *clip);
void mRectIncPoint(mRect *rc,int x,int y);
void mRectDeflate(mRect *rc,int dx,int dy);
void mRectRelMove(mRect *rc,int mx,int my);

void mBoxSetByPoint(mBox *box,mPoint *pt1,mPoint *pt2);
void mBoxSetByRect(mBox *box,mRect *rc);
void mBoxUnion(mBox *dst,mBox *src);
void mBoxScaleKeepAspect(mBox *box,int boxw,int boxh,mBool noscaleup);
void mBoxSetByPoints(mBox *box,mPoint *pt,int num);
mBool mBoxIsCross(mBox *box1,mBox *box2);
mBool mBoxIsPointIn(mBox *box,int x,int y);

#ifdef __cplusplus
}
#endif

#endif
