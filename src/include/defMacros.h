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

/**********************
 * 共通マクロ定義
 **********************/

#ifndef DEF_MACROS_H
#define DEF_MACROS_H

#define APPNAME       "AzPainter"

#define IMAGE_SIZE_MAX    20000		//イメージ最大サイズ
#define CANVAS_ZOOM_MIN   10		//キャンバス表示倍率、最小 (1%=10)
#define CANVAS_ZOOM_MAX   10000

#define FILEFILTER_NORMAL_IMAGE   "Image File (BMP/PNG/GIF/JPEG)\t*.bmp;*.png;*.gif;*.jpg;*.jpeg\tAll Files (*)\t*\t"

#define CONFIG_FILENAME_MAIN         "main-2.conf"
#define CONFIG_FILENAME_BRUSH        "brush-2.dat"
#define CONFIG_FILENAME_COLPALETTE   "palette-2.dat"
#define CONFIG_FILENAME_GRADATION    "grad.dat"
#define CONFIG_FILENAME_SHORTCUTKEY  "sckey-2.conf"

#define APP_TEXTURE_PATH  "texture"
#define APP_BRUSH_PATH    "brush"

#endif
