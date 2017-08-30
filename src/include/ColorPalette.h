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
 * カラーパレットデータ
 ***************************/

#ifndef COLORPALETTE_H
#define COLORPALETTE_H

#define COLORPALETTE_NUM  4
#define COLORPALETTEDAT_MAXNUM  4096

typedef struct
{
	uint8_t *buf;
	int num;
}ColorPaletteDat;

typedef struct
{
	ColorPaletteDat pal[COLORPALETTE_NUM];
	mBool exit_save;
}ColorPalette;


extern ColorPalette *g_colorpalette;
#define APP_COLPAL   g_colorpalette


void ColorPalette_init();
void ColorPalette_free();

void ColorPalette_load();
void ColorPalette_savefile();

void ColorPalette_convert_from_ver1();

void ColorPaletteDat_free(ColorPaletteDat *p);
mBool ColorPaletteDat_alloc(ColorPaletteDat *p,int num);
mBool ColorPaletteDat_resize(ColorPaletteDat *p,int num);

void ColorPaletteDat_fillWhite(ColorPaletteDat *p);
uint32_t ColorPaletteDat_getColor(ColorPaletteDat *p,int no);
void ColorPaletteDat_setColor(ColorPaletteDat *p,int no,uint32_t col);
void ColorPaletteDat_gradation(ColorPaletteDat *p,int no1,int no2);

void ColorPaletteDat_createFromImage(ColorPaletteDat *p,uint8_t *buf,int w,int h);

mBool ColorPaletteDat_loadFile(ColorPaletteDat *p,const char *filename);
mBool ColorPaletteDat_saveFile_apl(ColorPaletteDat *p,const char *filename);
mBool ColorPaletteDat_saveFile_aco(ColorPaletteDat *p,const char *filename);

#endif
