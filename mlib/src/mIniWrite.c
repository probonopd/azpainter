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
 * mIniWrite
 *****************************************/

#include <stdio.h>

#include "mDef.h"
#include "mStr.h"
#include "mUtilStdio.h"
#include "mUtil.h"


/*******************//**

@defgroup iniwrite mIniWrite
@brief INI書き込み

@ingroup group_etc
@{

@file mIniWrite.h

************************/


/** ファイル開く */

FILE *mIniWriteOpenFile(const char *filename)
{
	return mFILEopenUTF8(filename, "wb");
}

/** ファイル開く */

FILE *mIniWriteOpenFile2(const char *path,const char *filename)
{
	mStr str = MSTR_INIT;
	FILE *fp;

	if(path)
	{
		mStrSetText(&str, path);
		mStrPathAdd(&str, filename);
	}
	else
		mStrSetText(&str, filename);

	fp = mIniWriteOpenFile(str.buf);

	mStrFree(&str);

	return fp;
}

/** グループ名出力 */

void mIniWriteGroup(FILE *fp,const char *name)
{
	fprintf(fp, "[%s]\n", name);
}

/** グループ名を番号から出力 */

void mIniWriteGroup_int(FILE *fp,int no)
{
	fprintf(fp, "[%d]\n", no);
}

/** int 値出力 */

void mIniWriteInt(FILE *fp,const char *key,int num)
{
	fprintf(fp, "%s=%d\n", key, num);
}

/** 番号をキー名にして int 値出力 */

void mIniWriteNoInt(FILE *fp,int keyno,int num)
{
	fprintf(fp, "%d=%d\n", keyno, num);
}

/** 16進数値出力 */

void mIniWriteHex(FILE *fp,const char *key,uint32_t num)
{
	fprintf(fp, "%s=%x\n", key, num);
}

/** 番号をキー名にして16進数値出力 */

void mIniWriteNoHex(FILE *fp,int keyno,uint32_t num)
{
	fprintf(fp, "%d=%x\n", keyno, num);
}

/** 文字列出力 */

void mIniWriteText(FILE *fp,const char *key,const char *text)
{
	fprintf(fp, "%s=%s\n", key, (text)? text: "");
}

/** 番号をキー名にして文字列出力 */

void mIniWriteNoText(FILE *fp,int keyno,const char *text)
{
	fprintf(fp, "%d=%s\n", keyno, (text)? text: "");
}

/** mStr の文字列出力 */

void mIniWriteStr(FILE *fp,const char *key,mStr *str)
{
	fprintf(fp, "%s=%s\n", key, (str && str->buf)? str->buf: "");
}

/** mStr の配列から文字列出力 */

void mIniWriteNoStrs(FILE *fp,int keytop,mStr *array,int strnum)
{
	int i;

	for(i = 0; i < strnum; i++)
	{
		if(!mStrIsEmpty(array + i))
			fprintf(fp, "%d=%s\n", keytop + i, array[i].buf);
	}
}

/** mPoint をカンマで区切って出力 */

void mIniWritePoint(FILE *fp,const char *key,mPoint *pt)
{
	fprintf(fp, "%s=%d,%d\n", key, pt->x, pt->y);
}

/** mSize をカンマで区切って出力 */

void mIniWriteSize(FILE *fp,const char *key,mSize *size)
{
	fprintf(fp, "%s=%d,%d\n", key, size->w, size->h);
}

/** mBox をカンマで区切って出力 */

void mIniWriteBox(FILE *fp,const char *key,mBox *box)
{
	fprintf(fp, "%s=%d,%d,%d,%d\n",
		key, box->x, box->y, box->w, box->h);
}

/** 数値配列をカンマで区切って出力
 *
 * @param bytes 値のバイト数 (1,2,4)
 * @param hex   16進数にするか */

void mIniWriteNums(FILE *fp,const char *key,const void *buf,int numcnt,int bytes,mBool hex)
{
	const uint8_t *ps = (const uint8_t *)buf;
	uint32_t n;

	//データがない場合も出力する

	fprintf(fp, "%s=", key);

	if(buf)
	{
		for(; numcnt > 0; numcnt--)
		{
			switch(bytes)
			{
				case 2:
					n = *((uint16_t *)ps);
					ps += 2;
					break;
				case 4:
					n = *((uint32_t *)ps);
					ps += 4;
					break;
				default:
					n = *(ps++);
					break;
			}

			if(hex)
				fprintf(fp, "%x", n);
			else
				fprintf(fp, "%u", n);

			if(numcnt > 1) fputc(',', fp);
		}
	}

	fputc('\n', fp);
}

/** Base64 コードで出力
 *
 * key=size:base64 */

void mIniWriteBase64(FILE *fp,const char *key,const void *buf,int size)
{
	int encsize;
	char *encbuf;

	if(!buf || size == 0) return;

	encsize = mBase64GetEncodeSize(size);

	encbuf = (char *)mMalloc(encsize + 1, FALSE);
	if(!encbuf) return;

	mBase64Encode(encbuf, encsize + 1, buf, size);

	fprintf(fp, "%s=%d:%s\n", key, size, encbuf);

	mFree(encbuf);
}

/** ファイルダイアログ設定を出力 */

void mIniWriteFileDialogConfig(FILE *fp,const char *key,const void *buf)
{
	mIniWriteNums(fp, key, buf, 3, 2, TRUE);
}

/** @} */
