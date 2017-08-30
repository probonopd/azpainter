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
 * PrevTileImage
 *
 * TileImage プレビューウィジェット
 *****************************************/

#include "mDef.h"
#include "mWidgetDef.h"
#include "mWidget.h"
#include "mPixbuf.h"

#include "TileImage.h"


//----------------------

typedef struct _PrevTileImage
{
	mWidget wg;

	TileImage *img;
	mSize size;
}PrevTileImage;

//----------------------


/** 描画 */

static void _draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	PrevTileImage *p = (PrevTileImage *)wg;

	//枠

	mPixbufDraw3DFrame(pixbuf, 0, 0, wg->w, wg->h, mGraytoPix(130), mGraytoPix(0xff));

	//プレビュー

	if(p->img)
		TileImage_drawPreview(p->img, pixbuf, 1, 1, wg->w - 2, wg->h - 2, p->size.w, p->size.h);
	else
		mPixbufFillBox(pixbuf, 1, 1, wg->w - 2, wg->h - 2, mGraytoPix(210));
}

/** 作成 */

PrevTileImage *PrevTileImage_new(mWidget *parent,int w,int h,int (*dnd_handle)(mWidget *,char **))
{
	PrevTileImage *p;

	p = (PrevTileImage *)mWidgetNew(sizeof(PrevTileImage), parent);

	p->wg.hintW = w;
	p->wg.hintH = h;
	p->wg.draw = _draw_handle;
	p->wg.onDND = dnd_handle;

	if(dnd_handle)
		p->wg.fState |= MWIDGET_STATE_ENABLE_DROP;

	return p;
}

/** 画像をセット */

void PrevTileImage_setImage(PrevTileImage *p,TileImage *img,int w,int h)
{
	p->img = img;
	p->size.w = w;
	p->size.h = h;
	
	mWidgetUpdate(M_WIDGET(p));
}

