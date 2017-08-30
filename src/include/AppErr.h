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
 * アプリケーションエラー番号
 *
 * (翻訳データの文字列IDと連動している)
 ***************************************/

#ifndef APPERR_H
#define APPERR_H

enum
{
	APPERR_OK,
	APPERR_FAILED,
	APPERR_ALLOC,
	APPERR_UNSUPPORTED_FORMAT,
	APPERR_LOAD,
	APPERR_LARGE_SIZE,
	APPERR_LOAD_ONLY_APDv3,
	APPERR_USED_KEY,
	APPERR_CANNOTDRAW_FOLDER,
	APPERR_CANNOTDRAW_LOCK,
	APPERR_NONE_CHECKED_LAYER,
	APPERR_NONE_OPT_TEXTURE,
	APPERR_FILTER_COLTYPE
};

#endif
