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

/******************************************
 * [Dock]オプションのウィジェット作成関数
 ******************************************/

#ifndef DOCK_OPTION_SUB_H
#define DOCK_OPTION_SUB_H

typedef struct _ValueBar  ValueBar;
typedef struct _mComboBox mComboBox;

/* タブ内容作成 */

mWidget *DockOption_createTab_tool(mWidget *parent);
mWidget *DockOption_createTab_tool_grad(mWidget *parent);

mWidget *DockOption_createTab_texture(mWidget *parent);
mWidget *DockOption_createTab_rule(mWidget *parent);
mWidget *DockOption_createTab_headtail(mWidget *parent);

/* ウィジェット作成 */

mWidget *DockOption_createMainContainer(int size,mWidget *parent,
	int (*event)(mWidget *,mEvent *));

ValueBar *DockOption_createDensityBar(mWidget *parent,int id,int val);
ValueBar *DockOption_createBar(mWidget *parent,int wid,int label_id,int min,int max,int val);

mComboBox *DockOption_createPixelModeCombo(mWidget *parent,int id,uint8_t *dat,int sel);
mComboBox *DockOption_createComboBox(mWidget *parent,int cbid,int trid);

/* ほか */

void DockOption_toolStamp_changeImage(mWidget *wg);

#endif
