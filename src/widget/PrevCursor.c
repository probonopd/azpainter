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
 * PrevCursor
 *
 * カーソル画像プレビューウィジェット
 *****************************************/

#include "mDef.h"
#include "mWidgetDef.h"
#include "mWidget.h"
#include "mPixbuf.h"
#include "mSysCol.h"


//----------------------

typedef struct _PrevCursor
{
	mWidget wg;

	uint8_t *buf;
}PrevCursor;

//----------------------


/** プレビュー描画 */

static void _draw_prev(mPixbuf *pixbuf,uint8_t *buf)
{
	uint8_t *ps_col,*ps_colY,*ps_mask,*ps_maskY,*pd,f;
	mBox box;
	int w,h,bpp,pitchd,pitchs,ix,iy,sw,sh;
	mPixCol white;

	sw = buf[0];
	sh = buf[1];

	//クリッピング

	if(!mPixbufGetClipBox_d(pixbuf, &box, 1, 1, sw, sh)) return;

	w = box.w, h = box.h;

	//

	pd = mPixbufGetBufPtFast(pixbuf, box.x, box.y);
	bpp = pixbuf->bpp;
	pitchd = pixbuf->pitch_dir - w * bpp;

	pitchs = (sw + 7) >> 3;
	ps_colY = buf + 4;
	ps_maskY = ps_colY + pitchs * sh;

	white = MSYSCOL(WHITE);

	//

	for(iy = h; iy > 0; iy--)
	{
		ps_col = ps_colY;
		ps_mask = ps_maskY;
		f = 1;
		
		for(ix = w; ix > 0; ix--, pd += bpp)
		{
			if(*ps_mask & f)
				(pixbuf->setbuf)(pd, (*ps_col & f)? 0: white);

			f <<= 1;
			if(f == 0) f = 1, ps_col++, ps_mask++;
		}

		ps_colY += pitchs;
		ps_maskY += pitchs;
		pd += pitchd;
	}
}

/** 描画 */

static void _draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	PrevCursor *p = (PrevCursor *)wg;

	//枠

	mPixbufDraw3DFrame(pixbuf, 0, 0, wg->w, wg->h, mGraytoPix(130), mGraytoPix(0xff));

	//カーソル画像

	mPixbufFillBox(pixbuf, 1, 1, wg->w - 2, wg->h - 2, mGraytoPix(210));

	if(p->buf)
		_draw_prev(pixbuf, p->buf);
}

/** 作成 */

PrevCursor *PrevCursor_new(mWidget *parent)
{
	PrevCursor *p;

	p = (PrevCursor *)mWidgetNew(sizeof(PrevCursor), parent);

	p->wg.hintW = 32 + 2;
	p->wg.hintH = 32 + 2;
	p->wg.draw = _draw_handle;

	return p;
}

/** 画像をセット */

void PrevCursor_setBuf(PrevCursor *p,uint8_t *buf)
{
	p->buf = buf;
	
	mWidgetUpdate(M_WIDGET(p));
}

