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

#ifndef MLIB_ICONBUTTONS_H
#define MLIB_ICONBUTTONS_H

#include "mWidgetDef.h"
#include "mListDef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_ICONBUTTONS(p)  ((mIconButtons *)(p))

typedef struct _mToolTip mToolTip;

typedef struct
{
	uint32_t style;
	mList list;
	mImageList *imglist;
	void *itemOn;
	mToolTip *tooltip;
	int fpress;
	uint16_t trgroupid;
}mIconButtonsData;

typedef struct _mIconButtons
{
	mWidget wg;
	mIconButtonsData ib;
}mIconButtons;


enum MICONBUTTONS_STYLE
{
	MICONBUTTONS_S_TOOLTIP    = 1<<0,
	MICONBUTTONS_S_AUTO_WRAP  = 1<<1,
	MICONBUTTONS_S_VERT       = 1<<2,
	MICONBUTTONS_S_SEP_BOTTOM = 1<<3,
	MICONBUTTONS_S_DESTROY_IMAGELIST = 1<<4
};

enum MICONBUTTONS_ITEM_FLAGS
{
	MICONBUTTONS_ITEMF_BUTTON = 0,
	MICONBUTTONS_ITEMF_CHECKBUTTON = 1<<0,
	MICONBUTTONS_ITEMF_CHECKGROUP  = 1<<1,
	MICONBUTTONS_ITEMF_SEP      = 1<<2,
	MICONBUTTONS_ITEMF_DROPDOWN = 1<<3,
	MICONBUTTONS_ITEMF_WRAP     = 1<<4,
	MICONBUTTONS_ITEMF_DISABLE  = 1<<5,
	MICONBUTTONS_ITEMF_CHECKED  = 1<<6
};


void mIconButtonsDestroyHandle(mWidget *p);
void mIconButtonsDrawHandle(mWidget *p,mPixbuf *pixbuf);
int mIconButtonsEventHandle(mWidget *wg,mEvent *ev);

mIconButtons *mIconButtonsNew(int size,mWidget *parent,uint32_t style);

void mIconButtonsSetImageList(mIconButtons *p,mImageList *img);
void mIconButtonsReplaceImageList(mIconButtons *p,mImageList *img);
void mIconButtonsSetTooltipTrGroup(mIconButtons *p,uint16_t gid);
void mIconButtonsCalcHintSize(mIconButtons *p);

void mIconButtonsDeleteAll(mIconButtons *p);
void mIconButtonsAdd(mIconButtons *p,int id,int img,int tooltip,uint32_t flags);
void mIconButtonsAddSep(mIconButtons *p);

void mIconButtonsSetCheck(mIconButtons *p,int id,int type);
mBool mIconButtonsIsCheck(mIconButtons *p,int id);
void mIconButtonsSetEnable(mIconButtons *p,int id,int type);

void mIconButtonsGetItemBox(mIconButtons *p,int id,mBox *box,mBool bRoot);

#ifdef __cplusplus
}
#endif

#endif
