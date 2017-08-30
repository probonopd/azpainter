/*$
 Copyright (C) 2013-2017 Azel.

 This file is part of AzPainter.

 AzPainter is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 AzPainter is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
$*/

/*****************************************
 * PrevImage8
 *
 * ImageBuf8 イメージのプレビュー
 *****************************************/

#include "mDef.h"
#include "mWidgetDef.h"
#include "mWidget.h"
#include "mPixbuf.h"

#include "ImageBuf8.h"


//------------------------

typedef struct _PrevImage8
{
	mWidget wg;

	ImageBuf8 *img;
	mBool bBrush;
}PrevImage8;

//------------------------


/** 描画 */

static void _draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	PrevImage8 *p = (PrevImage8 *)wg;
	int w,h;

	mPixbufBox(pixbuf, 0, 0, wg->w, wg->h, 0);

	w = wg->w - 2;
	h = wg->h - 2;

	if(!p->img)
		mPixbufFillBox(pixbuf, 1, 1, w, h, mGraytoPix(0xcc));
	else if(p->bBrush)
		ImageBuf8_drawBrushPreview(p->img, pixbuf, 1, 1, w, h);
	else
		ImageBuf8_drawTexturePreview(p->img, pixbuf, 1, 1, w, h);
}

/** 作成
 *
 * @param hintw,hinth  推奨サイズ */

PrevImage8 *PrevImage8_new(mWidget *parent,int hintw,int hinth,uint32_t fLayout)
{
	PrevImage8 *p;
	
	p = (PrevImage8 *)mWidgetNew(sizeof(PrevImage8), parent);
	if(!p) return NULL;
	
	p->wg.draw = _draw_handle;

	p->wg.fLayout = fLayout;
	p->wg.hintW = hintw;
	p->wg.hintH = hinth;
	
	return p;
}

/** イメージをセット
 *
 * @param img  NULL でなし (灰色で塗りつぶされる) */

void PrevImage8_setImage(PrevImage8 *p,ImageBuf8 *img,mBool bBrush)
{
	if(!img && !p->img) return;
	
	p->img = img;
	p->bBrush = bBrush;

	mWidgetUpdate(M_WIDGET(p));
}
