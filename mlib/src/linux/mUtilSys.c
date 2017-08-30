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

/*****************************************
 * <Linux> システム関連ユーティリティ
 *****************************************/

#include <stdlib.h>
#include <unistd.h>

#include "mDef.h"

#include "mUtilCharCode.h"


/***************//**

@defgroup util_sys mUtilSys
@brief システム関連ユーティリティ

@ingroup group_util
@{

@file mUtilSys.h

****************/

/** コマンド実行 */

mBool mExec(const char *cmd)
{
	char *path;
	pid_t pid;
	mBool ret = TRUE;

	path = mUTF8ToLocal_alloc(cmd, -1, NULL);
	if(!path) return FALSE;
	
	//プロセス複製

	pid = fork();

	if(pid < 0)
		ret = FALSE;
	else if(pid == 0)
	{
		execl("/bin/sh", "sh", "-c", path, (char *)0);
		exit(-1);
	}

	mFree(path);

	return ret;
}

/** @} */
