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

/***************************
 * テキスト描画用フォント
 ***************************/

#ifndef DRAWFONT_H
#define DRAWFONT_H

typedef struct _DrawFont DrawFont;
typedef struct _mFontInfo mFontInfo;


typedef struct
{
	int char_space,
		line_space,
		flags;
	void (*setpixel)(int,int,int,void *);
	void *param;
}DrawFontInfo;

enum
{
	DRAWFONT_F_VERT = 1<<0
};


mBool DrawFont_init();
void DrawFont_finish();

void DrawFont_free(DrawFont *p);
DrawFont *DrawFont_create(mFontInfo *info);

void DrawFont_setHinting(DrawFont *p,int type);

void DrawFont_drawText(DrawFont *p,int x,int y,const char *text,DrawFontInfo *info);

#endif
