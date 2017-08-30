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

/************************************
 * 画像ファイルフォーマット定義
 ************************************/

#ifndef DEF_FILEFORMAT_H
#define DEF_FILEFORMAT_H

enum
{
	FILEFORMAT_UNKNOWN = 0,
	
	FILEFORMAT_PNG  = 1<<0,
	FILEFORMAT_JPEG = 1<<1,
	FILEFORMAT_GIF  = 1<<2,
	FILEFORMAT_BMP  = 1<<3,

	FILEFORMAT_APD = 1<<4,
	FILEFORMAT_ADW = 1<<5,
	FILEFORMAT_PSD = 1<<6,

	/* option */

	FILEFORMAT_APD_v1v2 = 1<<16,
	FILEFORMAT_APD_v3   = 1<<17,

	FILEFORMAT_ALPHA_CHANNEL = 1<<18,	//アルファチャンネル (保存時)

	//

	FILEFORMAT_NORMAL_IMAGE = FILEFORMAT_PNG | FILEFORMAT_JPEG | FILEFORMAT_GIF | FILEFORMAT_BMP,
	FILEFORMAT_OPEN_LAYERIMAGE = FILEFORMAT_NORMAL_IMAGE | FILEFORMAT_APD_v3
};

/* MainWindow_file.c */
uint32_t FileFormat_getbyFileHeader(const char *filename);

#endif
