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

#ifndef ENVOPTDLG_H
#define ENVOPTDLG_H

int EnvOptionDlg_run(mWindow *owner);

enum
{
	ENVOPTDLG_F_UPDATE_CANVAS = 1<<0,
	ENVOPTDLG_F_UPDATE_ALL = 1<<1,
	ENVOPTDLG_F_CURSOR = 1<<2,
	ENVOPTDLG_F_ICONSIZE = 1<<3,
	ENVOPTDLG_F_TOOLBAR_BTT = 1<<4,
	ENVOPTDLG_F_THEME = 1<<5
};

#endif
