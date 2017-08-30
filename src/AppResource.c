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
 * アプリリソース関連関数
 *****************************************/

#include "mDef.h"
#include "mStr.h"
#include "mImageList.h"

//-----------------

static const uint8_t g_toolbar_btts[] = {
	0,1,15,254,
	2,3,4,254,
	5,6,7,8,9,254,
	10,254,
	11,12,254,
	13,254,
	14,
	255
};

//-----------------


/** アイコン用のイメージリスト読み込み */

mImageList *AppResource_loadIconImage(const char *filename,int size)
{
	mStr str = MSTR_INIT;
	mImageList *img;

	//指定サイズ読み込み

	mStrSetFormat(&str, "!/%dx%d/%s", size, size, filename);

	img = mImageListLoadPNG(str.buf, size, 0x00ff00);

	//失敗時は16x16

	if(!img)
	{
		mStrSetFormat(&str, "!/16x16/%s", filename);

		img = mImageListLoadPNG(str.buf, 16, 0x00ff00);
	}

	mStrFree(&str);

	return img;
}

/** ツールバーのボタンデータのデフォルト値取得 */

const uint8_t *AppResource_getToolbarBtt_default()
{
	return g_toolbar_btts;
}
