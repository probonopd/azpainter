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
 * APD v3 読込/保存
 ************************************/

#ifndef FILE_APD_V3_H
#define FILE_APD_V3_H

typedef struct _loadAPDv3 loadAPDv3;
typedef struct _saveAPDv3 saveAPDv3;
typedef struct _LayerItem LayerItem;
typedef struct _mPopupProgress mPopupProgress;

typedef struct
{
	int width,
		height,
		dpi;
}loadapdv3_info;


/* load */

loadAPDv3 *loadAPDv3_open(const char *filename,loadapdv3_info *info,mPopupProgress *prog);
void loadAPDv3_close(loadAPDv3 *p);

void loadAPDv3_endThunk(loadAPDv3 *p);

mBool loadAPDv3_readLayerHeader(loadAPDv3 *p,int *layernum,int *curno);
mBool loadAPDv3_readLayerTree_andAddLayer(loadAPDv3 *p);

mBool loadAPDv3_beginLayerInfo(loadAPDv3 *p);
mBool loadAPDv3_readLayerInfo(loadAPDv3 *p,LayerItem *item);

mBool loadAPDv3_beginLayerTile(loadAPDv3 *p);
mBool loadAPDv3_readLayerTile(loadAPDv3 *p,LayerItem *item);

/* save */

saveAPDv3 *saveAPDv3_open(const char *filename,mPopupProgress *prog);
void saveAPDv3_close(saveAPDv3 *p);

void saveAPDv3_writeEnd(saveAPDv3 *p);

void saveAPDv3_writeLayerHeader(saveAPDv3 *p,int num,int curno);
void saveAPDv3_writeLayerTree(saveAPDv3 *p);

void saveAPDv3_endThunk(saveAPDv3 *p);

void saveAPDv3_beginLayerInfo(saveAPDv3 *p);
void saveAPDv3_writeLayerInfo(saveAPDv3 *p,LayerItem *item);

void saveAPDv3_beginLayerTile(saveAPDv3 *p);
void saveAPDv3_writeLayerTile(saveAPDv3 *p,LayerItem *item);

void saveAPDv3_writeBlendImage(saveAPDv3 *p);

#endif
