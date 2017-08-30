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
 * APD v3 読み込み/保存
 ************************************/

#include "mDef.h"

#include "defDraw.h"
#include "defConfig.h"

#include "LayerList.h"
#include "LayerItem.h"

#include "draw_main.h"

#include "file_apd_v3.h"


//=========================
// 保存
//=========================


/** APD v3 保存 */

mBool drawFile_save_apd_v3(DrawData *p,const char *filename,mPopupProgress *prog)
{
	saveAPDv3 *sav;
	LayerItem *pi;

	sav = saveAPDv3_open(filename, prog);
	if(!sav) return FALSE;

	saveAPDv3_writeLayerHeader(sav,
		LayerList_getNum(p->layerlist), LayerList_getItemPos(p->layerlist, p->curlayer));

	saveAPDv3_writeLayerTree(sav);

	//レイヤ情報

	saveAPDv3_beginLayerInfo(sav);

	for(pi = LayerList_getItem_top(p->layerlist); pi; pi = LayerItem_getNext(pi))
		saveAPDv3_writeLayerInfo(sav, pi);

	saveAPDv3_endThunk(sav);

	//レイヤタイル

	saveAPDv3_beginLayerTile(sav);

	for(pi = LayerList_getItem_top(p->layerlist); pi; pi = LayerItem_getNext(pi))
		saveAPDv3_writeLayerTile(sav, pi);

	saveAPDv3_endThunk(sav);

	//合成後イメージ

	if(!(APP_CONF->optflags & CONFIG_OPTF_APD_NOINCLUDE_BLENDIMG))
	{
		drawImage_blendImage_RGB8(p);

		saveAPDv3_writeBlendImage(sav);

		drawUpdate_blendImage_full(p);
	}

	//

	saveAPDv3_writeEnd(sav);

	saveAPDv3_close(sav);

	return TRUE;
}


//=========================
// 読み込み
//=========================


/** 読み込み処理 */

static mBool _load_main(DrawData *p,loadAPDv3 *load)
{
	int layernum,layercur;
	LayerItem *pi;

	//レイヤ情報

	if(!loadAPDv3_readLayerHeader(load, &layernum, &layercur))
		return FALSE;

	//レイヤツリー & レイヤ作成

	if(!loadAPDv3_readLayerTree_andAddLayer(load))
		return FALSE;

	//レイヤ情報

	if(!loadAPDv3_beginLayerInfo(load))
		 return FALSE;

	for(pi = LayerList_getItem_top(p->layerlist); pi; pi = LayerItem_getNext(pi))
	{
		if(!loadAPDv3_readLayerInfo(load, pi))
			return FALSE;
	}

	loadAPDv3_endThunk(load);

	//レイヤタイル

	if(!loadAPDv3_beginLayerTile(load))
		 return FALSE;

	for(pi = LayerList_getItem_top(p->layerlist); pi; pi = LayerItem_getNext(pi))
	{
		if(!loadAPDv3_readLayerTile(load, pi))
			return FALSE;
	}

	//カレントレイヤ

	p->curlayer = LayerList_getItem_byPos_topdown(p->layerlist, layercur);

	return TRUE;
}

/** APD v3 読み込み */

mBool drawFile_load_apd_v3(DrawData *p,const char *filename,mPopupProgress *prog)
{
	loadAPDv3 *load;
	loadapdv3_info info;
	mBool ret = FALSE;

	load = loadAPDv3_open(filename, &info, prog);
	if(!load) return FALSE;

	if(drawImage_new(p, info.width, info.height, info.dpi, -1))
		ret = _load_main(p, load);

	loadAPDv3_close(load);

	return ret;
}
