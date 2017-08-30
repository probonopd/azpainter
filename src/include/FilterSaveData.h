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

/***************************************
 * フィルタのデータ保存管理
 ***************************************/

#ifndef FILTER_SAVE_DATA_H
#define FILTER_SAVE_DATA_H

typedef struct _mIniRead mIniRead;

void FilterSaveData_free();

void FilterSaveData_getConfig(mIniRead *ini);
void FilterSaveData_setConfig(void *fp_ptr);

char *FilterSaveData_getText(int cmdid);
mBool FilterSaveData_getValue(char *text,char id,int *dst);
void FilterSaveData_setData(int cmdid,const char *text);

#endif
