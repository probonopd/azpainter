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

/********************************
 * ブラシリストデータ
 ********************************/

#ifndef BRUSHLIST_H
#define BRUSHLIST_H

#include "mListDef.h"

typedef struct _BrushItem BrushItem;
typedef struct _BrushDrawParam BrushDrawParam;

#define BRUSHGROUPITEM(p)  ((BrushGroupItem *)(p))


/** グループアイテム */

typedef struct _BrushGroupItem
{
	mListItem i;

	mList list;		//ブラシリスト
	char *name;		//名前
	int colnum;		//横に並べる数
	uint8_t expand;	//展開
}BrushGroupItem;


/*-- BrushGroup --*/

BrushItem *BrushGroup_getTopItem(BrushGroupItem *p);
BrushItem *BrushGroup_getItemAtIndex(BrushGroupItem *p,int index);
int BrushGroup_getViewVertNum(BrushGroupItem *p);


/*-- BrushList --*/

mBool BrushList_new();
void BrushList_free();

int BrushList_getGroupNum();
BrushGroupItem *BrushList_getTopGroup();

BrushItem *BrushList_getEditItem();
BrushItem *BrushList_getSelItem();
BrushItem *BrushList_getRegisteredItem();

void BrushList_getDrawInfo(
	BrushItem **ppitem,BrushDrawParam **ppparam,mBool registered);
BrushDrawParam *BrushList_getBrushDrawParam_forFilter();
int BrushList_getChangeBrushSizeInfo(BrushItem **ppitem,mBool registered);

BrushItem *BrushList_newBrush(BrushGroupItem *group);
BrushGroupItem *BrushList_newGroup(const char *name,int colnum);
void BrushList_moveGroup_updown(BrushGroupItem *group,mBool up);
void BrushList_deleteGroup(BrushGroupItem *group);
mBool BrushList_deleteItem(BrushItem *item);
void BrushList_moveItem(BrushItem *src,BrushGroupItem *dst_group,BrushItem *dst);
void BrushList_moveItem_updown(BrushItem *item,mBool up);
mBool BrushList_pasteItem_text(BrushGroupItem *group,BrushItem *item,char *text);
BrushItem *BrushList_pasteItem_new(BrushGroupItem *group,BrushItem *src);

mBool BrushList_setSelItem(BrushItem *item);
void BrushList_setRegisteredItem(BrushItem *item);
mBool BrushList_toggleLastItem();
mBool BrushList_moveSelect(mBool next);
void BrushList_setBrushDrawParam(mBool registered);

void BrushList_saveitem_manual();

mBool BrushList_setBrushName(BrushItem *item,const char *name);
void BrushList_afterBrushListEdit();

void BrushList_updateval_type(int val);
int BrushList_updateval_radius(int val);
void BrushList_updateval_radius_reg(int val);
void BrushList_updateval_opacity(int val);
void BrushList_updateval_pixelmode(int val);
void BrushList_updateval_hosei_type(int val);
void BrushList_updateval_hosei_str(int val);
void BrushList_updateval_min_size(int val);
void BrushList_updateval_min_opacity(int val);
void BrushList_updateval_interval(int val);
void BrushList_updateval_random_size(int val);
void BrushList_updateval_random_pos(int val);
void BrushList_updateval_waterparam(int no,int val);
void BrushList_updateval_shape_path(const char *path);
void BrushList_updateval_shape_hard(int val);
void BrushList_updateval_shape_rough(int val);
void BrushList_updateval_travelling_dir();
void BrushList_updateval_angle(int val);
void BrushList_updateval_angle_radom(int val);
void BrushList_updateval_pressure_type(int val);
void BrushList_updateval_pressure_value(uint32_t val);
void BrushList_updateval_texture_path(const char *path);
void BrushList_updateval_antialias();
void BrushList_updateval_curve();
void BrushList_updateval_detail();

void BrushList_loadconfigfile();
void BrushList_saveconfigfile();

void BrushList_convert_from_ver1();

#endif
