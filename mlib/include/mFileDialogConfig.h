/*$
Copyright (c) 2017, Azel
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERSAND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
$*/

#ifndef MLIB_FILEDIALOGCONFIG_H
#define MLIB_FILEDIALOGCONFIG_H

/* mAppDef.h, mIniRead.c, mIniWrite.c */

enum MFILEDIALOGCONFIG_VAL
{
	MFILEDIALOGCONFIG_VAL_FLAGS,
	MFILEDIALOGCONFIG_VAL_WIN_WIDTH,
	MFILEDIALOGCONFIG_VAL_WIN_HEIGHT,

	MFILEDIALOGCONFIG_VAL_NUM
};

enum MFILEDIALOGCONFIG_SORTTYPE
{
	MFILEDIALOGCONFIG_SORTTYPE_FILENAME,
	MFILEDIALOGCONFIG_SORTTYPE_FILESIZE,
	MFILEDIALOGCONFIG_SORTTYPE_MODIFY
};

enum MFILEDIALOGCONFIG_FLAGS
{
	MFILEDIALOGCONFIG_FLAGS_SORT_UP = 1<<3,
	MFILEDIALOGCONFIG_FLAGS_SHOW_HIDDEN_FILES = 1<<4
};

#define MFILEDIALOGCONFIG_GET_SORT_TYPE(n)  ((n) & 7)

#endif
