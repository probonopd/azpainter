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
 * 読み込みエラー文字列
 ***************************************/

#ifndef DEF_LOADERR_H
#define DEF_LOADERR_H

enum
{
	LOADERR_OK = -1,
	LOADERR_ALLOC = 0,
	LOADERR_CORRUPTED,
	LOADERR_OPENFILE,
	LOADERR_HEADER,
	LOADERR_VERSION,
	LOADERR_INVALID_VALUE,
	LOADERR_DECODE,

	LOADERR_NUM
};

/*-----------*/

#ifdef LOADERR_DEFINE

const char *g_loaderr_str[] = {
	"memory alloc error",
	"corrupted file",
	"open error",
	"invalid header",
	"version error",
	"invalid value",
	"decode error"
};

#else

extern const char *g_loaderr_str[LOADERR_NUM];

#endif

#endif
