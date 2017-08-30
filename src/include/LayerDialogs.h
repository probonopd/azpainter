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

/*****************************
 * レイヤ関連ダイアログ
 *****************************/

#ifndef LAYER_DIALOGS_H
#define LAYER_DIALOGS_H

typedef struct
{
	mStr strname;
	int coltype,
		blendmode;
	uint32_t col;
}LayerNewDlgInfo;

#define LAYERCOMBINEDLG_GET_TARGET(n)   (n & 3)
#define LAYERCOMBINEDLG_IS_NEWLAYER(n)  ((n & (1<<2)) != 0)
#define LAYERCOMBINEDLG_GET_COLTYPE(n)  ((n >> 3) & 3)


mBool LayerNewDlg_run(mWindow *owner,LayerNewDlgInfo *info);
mBool LayerColorDlg_run(mWindow *owner,uint32_t *col);
mBool LayerOptionDlg_run(mWindow *owner,mStr *strname,int *blendmode,uint32_t *col);
int LayerTypeDlg_run(mWindow *owner,int curtype);
int LayerCombineDlg_run(mWindow *owner,mBool cur_folder,mBool have_checked);

mBool LayerColorDlg_run(mWindow *owner,uint32_t *col);

#endif
