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

/**********************************
 * DrawData ファイル読み書き
 **********************************/

#ifndef DRAW_FILE_H
#define DRAW_FILE_H

typedef struct _mPopupProgress mPopupProgress;

mBool drawFile_save_image(DrawData *p,const char *filename,int format,mPopupProgress *prog);

int drawFile_load_adw(const char *filename,mPopupProgress *prog);
int drawFile_load_apd_v1v2(const char *filename,mPopupProgress *prog);

mBool drawFile_load_apd_v3(DrawData *p,const char *filename,mPopupProgress *prog);
mBool drawFile_save_apd_v3(DrawData *p,const char *filename,mPopupProgress *prog);

mBool drawFile_load_psd(DrawData *p,const char *filename,mPopupProgress *prog,char **errmes);
mBool drawFile_save_psd_layer(DrawData *p,const char *filename,mPopupProgress *prog);
mBool drawFile_save_psd_image(DrawData *p,int type,const char *filename,mPopupProgress *prog);

#endif
