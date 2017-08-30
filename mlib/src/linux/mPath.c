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
 * <Linux> mPath [パス関連]
 *****************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mDef.h"


/*****************//**

@defgroup path mPath
@brief ファイルパス関連

@ingroup group_system
@{

*********************/


/** ホームディレクトリパスを取得
 *
 * @return メモリ確保された文字列 */

char *mGetHomePath()
{
	char *pc = getenv("HOME");

	if(pc)
		return mStrdup(pc);
	else
		return mStrdup("/");
}

/** デフォルトの作業用ディレクトリパスを取得
 *
 * @return メモリ確保された文字列 */

char *mGetTempPath()
{
	return mStrdup("/tmp");
}

/** 各プロセスで異なる作業用ファイル名を取得 */

char *mGetProcessTempName()
{
	char m[32];

	snprintf(m, 32, "%d", (int)getpid());

	return mStrdup(m);
}

/** パス区切り文字を取得 */

char mPathGetSplitChar()
{
	return '/';
}

/** パスがトップのディレクトリか */

mBool mPathIsTop(const char *path)
{
	return (!path || (path[0] == '/' && path[1] == 0));
}

/** 指定文字がファイル名に使えない文字か */

mBool mPathIsDisableFilenameChar(char c)
{
	return (c == '/');
}

/** ファイル名として正しいか
 *
 * @return ファイル名に使えない文字が含まれていれば FALSE が返る */

mBool mPathIsEnableFilename(const char *filename)
{
	return (filename && !strchr(filename, '/'));
}

/** パス区切り文字の位置を取得
 *
 * @param last 最後に現れた文字位置を取得
 * @return -1 で見つからなかった */

int mPathGetSplitCharPos(const char *path,mBool last)
{
	char *pc;

	if(!path) return -1;

	if(last)
		pc = strrchr(path, '/');
	else
		pc = strchr(path, '/');

	return (pc)? pc - path: -1;
}

/** path の終端にパス区切り文字があればその位置を返す
 *
 * (Linux 時) "/" のみの場合は -1 が返る。
 *
 * @param len 文字長さ (必須)
 * @return -1 でなし */

int mPathGetBottomSplitCharPos(const char *path,int len)
{
	if(len <= 1)
		return -1;
	else
		return (path[len - 1] == '/')? len - 1: -1;
}

/** ２つのパスが同じか比較 */

mBool mPathCompareEq(const char *path1,const char *path2)
{
	return (path1 && path2 && strcmp(path1, path2) == 0);
}

/** @} */
