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

/*****************************************
 * UndoItem
 *
 * 基本処理 (確保と読み書き)
 *****************************************/

#include <stdio.h>
#include <string.h>	//memcpy

#include "mDef.h"
#include "mStr.h"
#include "mUndo.h"
#include "mUtilFile.h"
#include "mUtilStdio.h"

#include "Undo.h"
#include "UndoItem.h"
#include "UndoMaster.h"

#include "defConfig.h"


//------------------

//#define _PUT_DEBUG  1

#define _UNDO g_app_undo

enum
{
	_WRITETYPE_FILE,
	_WRITETYPE_MEM_FIX,
	_WRITETYPE_MEM_VARIABLE
};

//------------------



//=========================
// sub
//=========================


/** バッファ確保 */

static mBool _alloc_buf(UndoItem *p,int size)
{
	p->buf = (uint8_t *)mMalloc(size, FALSE);
	if(!p->buf) return FALSE;

	p->size = size;
	p->fileno = -1;

	_UNDO->used_bufsize += size;

#ifdef _PUT_DEBUG
	mDebug("mem:+%d -> %u\n", size, _UNDO->used_bufsize);
#endif

	return TRUE;
}

/** ファイル確保 (ファイル番号のみセット) */

static mBool _alloc_file(UndoItem *p)
{
	//空文字列で作業用ディレクトリ使用不可

	if(mStrIsEmpty(&APP_CONF->strTempDirProc)) return FALSE;

	//ファイル番号

	p->fileno = _UNDO->cur_fileno;

	_UNDO->cur_fileno = (_UNDO->cur_fileno + 1) & 0xffff;

#ifdef _PUT_DEBUG
	mDebug("file:%d\n", p->fileno);
#endif

	return TRUE;
}

/** ファイル名取得 */

static void _get_filename(mStr *str,int no)
{
	char m[16];

	snprintf(m, 16, "undo%04x", no);

	mStrCopy(str, &APP_CONF->strTempDirProc);
	mStrPathAdd(str, m);
}

/** ファイル書き込み開く */

static mBool _openfile_write(UndoItem *p)
{
	mStr str = MSTR_INIT;
	mBool ret;

	_get_filename(&str, p->fileno);

	_UNDO->writefp = mFILEopenUTF8(str.buf, "wb");

	ret = (_UNDO->writefp != NULL);

	mStrFree(&str);

	return ret;
}

/** ファイル出力に切り替え */

static mBool _change_to_file(UndoItem *p,const void *buf,int size)
{
	_UNDO->write_type = _WRITETYPE_FILE;

	//ファイル確保、開く

	if(!_alloc_file(p)
		|| !_openfile_write(p))
		return FALSE;

	//一時バッファの内容を書き込み

	if(fwrite(_UNDO->writetmpbuf, 1, _UNDO->write_tmpsize, _UNDO->writefp) != _UNDO->write_tmpsize)
		return FALSE;

	//データ書き込み

	return (fwrite(buf, 1, size, _UNDO->writefp) == size);
}


//=========================
// main
//=========================


/** 解放 */

void UndoItem_free(UndoItem *p)
{
	/* [!] データは val の値のみの場合があるので、
	 *     p->buf の値で判定せずに fileno で判定する。 */

	if(p->fileno == -1)
	{
		//バッファ
		
		if(p->buf)
		{
			mFree(p->buf);

			_UNDO->used_bufsize -= p->size;
		}
	}
	else
	{
		//ファイル削除

		mStr str = MSTR_INIT;

		_get_filename(&str, p->fileno);

		mDeleteFile(str.buf);

		mStrFree(&str);
	}
}

/** 確保
 *
 * @param size 1 以上でメモリ優先固定サイズ、-1 で常にファイル、-2 でメモリ優先で可変 */

mBool UndoItem_alloc(UndoItem *p,int size)
{
	int type,remain;

	//アンドゥバッファの残りサイズ
	/* バッファサイズが 0 で常にファイル。
	 * 環境設定でバッファサイズを変更した後は、使用済みサイズの方が
	 * 大きい場合があるので注意。 */

	if(APP_CONF->undo_maxbufsize <= _UNDO->used_bufsize)
		remain = 0;
	else
		remain = APP_CONF->undo_maxbufsize - _UNDO->used_bufsize;

	//出力タイプ

	if(size == UNDO_ALLOC_FILE || remain == 0)
		//ファイル
		type = _WRITETYPE_FILE;
	else if(size > 0)
	{
		//固定サイズ
		/* バッファに収まればメモリ、超えればファイル */

		type = (size <= remain)? _WRITETYPE_MEM_FIX: _WRITETYPE_FILE;
	}
	else
	{
		//可変サイズ
		/* 一時バッファに出力し、バッファに出力可能なサイズを超えた時点で
		 * ファイル出力に切り替え。 */

		type = _WRITETYPE_MEM_VARIABLE;

		_UNDO->write_tmpsize = 0;
		_UNDO->write_remain = (remain < UNDO_WRITETEMPBUFSIZE)? remain: UNDO_WRITETEMPBUFSIZE;
	}

	_UNDO->write_type = type;

	//確保

	if(type == _WRITETYPE_FILE)
		return _alloc_file(p);
	else if(type == _WRITETYPE_MEM_FIX)
		return _alloc_buf(p, size);
	else
		return TRUE;
}

/** 先頭から指定サイズデータ読み込み */

uint8_t *UndoItem_readData(UndoItem *p,int size)
{
	uint8_t *buf;
	mBool ret = FALSE;

	buf = (uint8_t *)mMalloc(size, FALSE);
	if(!buf) return NULL;

	if(UndoItem_openRead(p))
	{
		ret = UndoItem_read(p, buf, size);

		UndoItem_closeRead(p);
	}

	if(!ret)
	{
		mFree(buf);
		buf = NULL;
	}

	return buf;
}

/** データをコピー (データサイズ固定時) */

mBool UndoItem_copyData(UndoItem *src,UndoItem *dst,int size)
{
	uint8_t *buf;
	mBool ret = FALSE;

	if(!UndoItem_alloc(dst, size))
		return FALSE;

	//コピー

	if(dst->buf && src->buf)
	{
		//両方バッファ
		
		memcpy(dst->buf, src->buf, size);
		return TRUE;
	}
	else
	{
		//一方または両方がファイルの場合

		buf = (uint8_t *)mMalloc(size, FALSE);
		if(!buf) return FALSE;

		if(UndoItem_openRead(src) && UndoItem_openWrite(dst))
		{
			if(UndoItem_read(src, buf, size))
			{
				UndoItem_write(dst, buf, size);
				ret = TRUE;
			}
		}

		UndoItem_closeRead(src);
		UndoItem_closeWrite(dst);

		mFree(buf);

		return ret;
	}
}


//=========================
// 書き込み
//=========================


/** 書き込み開く */

mBool UndoItem_openWrite(UndoItem *p)
{
	switch(_UNDO->write_type)
	{
		//バッファ
		case _WRITETYPE_MEM_FIX:
			_UNDO->write_dst = p->buf;
			break;

		//ファイル
		case _WRITETYPE_FILE:
			return _openfile_write(p);
	}

	return TRUE;
}

/** 書き込み */

mBool UndoItem_write(UndoItem *p,const void *buf,int size)
{
	//データなし
	
	if(!buf || size <= 0) return TRUE;

	//

	switch(_UNDO->write_type)
	{
		//バッファ
		case _WRITETYPE_MEM_FIX:
			memcpy(_UNDO->write_dst, buf, size);

			_UNDO->write_dst += size;
			break;

		//ファイル
		case _WRITETYPE_FILE:
			return (fwrite(buf, 1, size, _UNDO->writefp) == size);

		//可変
		case _WRITETYPE_MEM_VARIABLE:
			if(size <= _UNDO->write_remain)
			{
				//残りサイズ内に収まる場合は一時バッファに

				memcpy(_UNDO->writetmpbuf + _UNDO->write_tmpsize, buf, size);

				_UNDO->write_tmpsize += size;
				_UNDO->write_remain -= size;
			}
			else
			{
				//一時バッファを超える場合はファイル出力に切り替え

				return _change_to_file(p, buf, size);
			}
			break;
	}

	return TRUE;
}

/** 書き込み閉じる */

mBool UndoItem_closeWrite(UndoItem *p)
{
	switch(_UNDO->write_type)
	{
		//ファイル
		case _WRITETYPE_FILE:
			if(_UNDO->writefp)
			{
				fclose(_UNDO->writefp);
				_UNDO->writefp = NULL;
			}
			break;
		//可変サイズ
		/* 終了時点でファイル出力に切り替わっていない場合は、
		 * 一時バッファからメモリ出力。 */
		case _WRITETYPE_MEM_VARIABLE:
			if(!_alloc_buf(p, _UNDO->write_tmpsize))
				return FALSE;

			memcpy(p->buf, _UNDO->writetmpbuf, _UNDO->write_tmpsize);
			break;
	}

	return TRUE;
}

/** ファイル書き込み時、現在位置保存+圧縮サイズ(仮)書き込み */

mBool UndoItem_writeEncSize_temp()
{
	uint32_t v = 0;

	fgetpos(_UNDO->writefp, &_UNDO->writefpos);

	return (fwrite(&v, 1, 4, _UNDO->writefp) == 4);
}

/** ファイル書き込み時、実際の圧縮サイズ書き込み */

void UndoItem_writeEncSize_real(uint32_t size)
{
	fsetpos(_UNDO->writefp, &_UNDO->writefpos);

	fwrite(&size, 4, 1, _UNDO->writefp);

	fseek(_UNDO->writefp, 0, SEEK_END);
}


//=========================
// 読み込み
//=========================


/** 読み込み開く */

mBool UndoItem_openRead(UndoItem *p)
{
	mBool ret = TRUE;

	if(p->fileno == -1)
	{
		//バッファ

		_UNDO->read_dst = p->buf;
	}
	else
	{
		//ファイル

		mStr str = MSTR_INIT;

		_get_filename(&str, p->fileno);

		_UNDO->readfp = mFILEopenUTF8(str.buf, "rb");

		ret = (_UNDO->readfp != NULL);

		mStrFree(&str);
	}

	return ret;
}

/** 読み込み */

mBool UndoItem_read(UndoItem *p,void *buf,int size)
{
	if(p->fileno == -1)
	{
		memcpy(buf, _UNDO->read_dst, size);

		_UNDO->read_dst += size;

		return TRUE;
	}
	else
	{
		return (fread(buf, 1, size, _UNDO->readfp) == size);
	}
}

/** 読み込みシーク */

void UndoItem_readSeek(UndoItem *p,int seek)
{
	if(p->fileno == -1)
		_UNDO->read_dst += seek;
	else
		fseek(_UNDO->readfp, seek, SEEK_CUR);
}

/** 読み込み閉じる */

void UndoItem_closeRead(UndoItem *p)
{
	if(_UNDO->readfp)
	{
		fclose(_UNDO->readfp);
		_UNDO->readfp = NULL;
	}
}

