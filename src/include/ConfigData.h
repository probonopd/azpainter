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
 * ConfigData 関数
 ************************************/

#ifndef CONFIGDATA_H
#define CONFIGDATA_H

mBool ConfigData_new();
void ConfigData_free();

void ConfigData_setTempDir_default();
void ConfigData_createTempDir();
void ConfigData_deleteTempDir();

mBool ConfigData_getTempPath(mStr *str,const char *add);

void ConfigData_waterPreset_default();

#endif
