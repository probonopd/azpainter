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
 * FreeType - GSUB テーブル
 *****************************************/
/*
 * GSUB テーブルは、グリフ置き換えの情報を定義するテーブル。
 * ScriptList、FeatureList、LookupList の３つのブロックがある。
 *
 * http://www.microsoft.com/typography/otspec/chapter2.htm
 * http://www.microsoft.com/typography/otspec/gsub.htm
 */

#include <string.h>

#include "mDef.h"

#include "mBufOperate.h"
#include "mMemAuto.h"


//--------------------

#define ENDMARK  255

#define SRC_GLYPH_TYPE_LIST  1
#define SRC_GLYPH_TYPE_RANGE 2

#define DST_GLYPH_TYPE_ADD  1
#define DST_GLYPH_TYPE_LIST 2

//--------------------


//===============================
// 解析
//===============================


typedef struct
{
	mBufOperate op;
	mMemAuto mem;
	int featurePos,lookupPos;
}parseGSUBDat;


/** Coverage (単独置き換え時) */

/* [format = 1]
 * uint16 format
 * uint16 count
 * uint16 glyph[count] : グリフIDリスト
 *
 * [format = 2]
 * uint16 format
 * uint16 count
 * RangeRecord record[count]
 *
 * <RangeRecord>
 *  uint16 start : 先頭のグリフID
 *  uint16 end   : 終端のグリフID
 *  uint16 startCoverageIndex : グリフIDが start 時のテーブルの先頭インデックス位置
 */

static void _parseCoverage(parseGSUBDat *dat,mBufOperate *op,int offset)
{
	intptr_t bkpos;
	uint16_t format,num,wd;
	int i;

	bkpos = mBufOpSetPosRet(op, offset);

	mBufOpRead16(op, &format);
	mBufOpRead16(op, &num);

	if(format == 1 || format == 2)
	{
		//置換え元の検索用データ
	
		mMemAutoAppendByte(&dat->mem, format);
		mMemAutoAppend(&dat->mem, &num, 2);

		i = (format == 2)? num * 3: num;

		for(; i > 0; i--)
		{
			mBufOpRead16(op, &wd);
			mMemAutoAppend(&dat->mem, &wd, 2);
		}
	}

	mBufOpSetPos(op, bkpos);
}

/** Lookup サブテーブル */

/* [format = 1]
 * uint16 format
 * uint16 offsetCoverage
 * int16  deltaGlyph : 元のグリフに増減される値
 *
 * [format = 2]
 * uint16 format
 * uint16 offsetCoverage
 * uint16 count
 * uint16 glyph[count] : グリフIDのリスト
 */

static void _parseLookupSubTable(parseGSUBDat *dat,mBufOperate *op,int offset)
{
	intptr_t bkpos;
	uint16_t format,pos,wd,num;

	bkpos = mBufOpSetPosRet(op, offset);

	mBufOpRead16(op, &format);
	mBufOpRead16(op, &pos);

	if(format == 1 || format == 2)
	{
		//Coverage (置換元のグリフのデータ)

		_parseCoverage(dat, op, offset + pos);

		//置換後グリフのデータ

		mMemAutoAppendByte(&dat->mem, format);

		if(format == 1)
		{
			//1: 元のグリフ値に差分値を加算する

			mBufOpRead16(op, &wd);
			mMemAutoAppend(&dat->mem, &wd, 2);
		}
		else
		{
			//2: グリフリスト

			mBufOpRead16(op, &num);
			mMemAutoAppend(&dat->mem, &num, 2);

			for(; num > 0; num--)
			{
				mBufOpRead16(op, &wd);
				mMemAutoAppend(&dat->mem, &wd, 2);
			}
		}
	}

	mBufOpSetPos(op, bkpos);
}

/** Lookup テーブル */

/* uint16 type
 * uint16 flag
 * uint16 subtableCount
 * uint16 offset[subtableCount]
 * uint16 markFilteringSet
 */

static void _parseLookupTable(parseGSUBDat *dat,mBufOperate *op,int offset)
{
	intptr_t bkpos;
	int i;

	bkpos = mBufOpSetPosRet(op, offset);

	if(mBufOpGet16(op) == 1)
	{
		//type == 1 : 単独置き換え

		mBufOpSeek(op, 2);

		for(i = mBufOpGet16(op); i > 0; i--)
			_parseLookupSubTable(dat, op, offset + mBufOpGet16(op));
	}

	mBufOpSetPos(op, bkpos);
}

/** LookupList 解析 */

/* [LookupList]
 *  uint16 count : データの数
 *  uint16 offset[count] : LookupList を先頭としたオフセット
 */

static void _parseLookup(parseGSUBDat *dat,mBufOperate *op,int index)
{
	int num;
	intptr_t bkpos;

	bkpos = mBufOpSetPosRet(op, dat->lookupPos);

	num = mBufOpGet16(op);

	if(index < num)
	{
		mBufOpSeek(op, index * 2);

		_parseLookupTable(dat, op, dat->lookupPos + mBufOpGet16(op));
	}

	mBufOpSetPos(op, bkpos);
}

/** FeatureList 解析 */

/* [FeatureList]
 *  uint16 count : レコードの数
 *  FeatureRecord record[count] : レコードの配列
 *
 * [FeatureRecord]
 *  uint32 tag : 識別子
 *  uint16 offset : FeatureList を先頭としたオフセット
 *
 * [FeatureTable]
 *  uint16 params : 予約
 *  uint16 count : LookupList インデックスの数
 *  uint16 index[count] : LookupList のインデックス番号の配列
 */

static void _parseFeature(parseGSUBDat *dat)
{
	mBufOperate *op = &dat->op;
	int i,j;
	intptr_t bkpos;
	uint32_t tag;
	uint16_t offset,lookup_index,lookup_prev = 0xffff;

	mBufOpSetPos(op, dat->featurePos);

	for(i = mBufOpGet16(op); i > 0; i--)
	{
		mBufOpRead32(op, &tag);
		mBufOpRead16(op, &offset);

		// "vert" か "vrt2" のみ。

		if(tag == 0x76657274 || tag == 0x76727432)
		{
			//FeatureTable

			bkpos = mBufOpSetPosRet(op, dat->featurePos + offset);

			mBufOpSeek(op, 2);

			for(j = mBufOpGet16(op); j > 0; j--)
			{
				mBufOpRead16(op, &lookup_index);

				/* Lookup のインデックスが前回と同じなら処理なし
				 * (複数の "vert" で同じデータが続く場合があるので除外) */

				if(lookup_index != lookup_prev)
				{
					_parseLookup(dat, op, lookup_index);

					lookup_prev = lookup_index;
				}
			}

			mBufOpSetPos(op, bkpos);
		}
	}

	//終了
	mMemAutoAppendByte(&dat->mem, ENDMARK);
}


//===============================
// メイン
//===============================


/*************//**

@defgroup ftgsub mFTGSUB
@brief FreeType GSUB テーブル

@ingroup group_lib

@{
@file mFTGSUB.h

****************/

/** GSUB テーブルから縦書き文字置換用のデータを抽出
 *
 * @return mFree() で解放すること */

void *mFTGSUB_getVertTable(void *gsub)
{
	parseGSUBDat dat;
	void *buf = NULL;

	mMemAutoInit(&dat.mem);

	mBufOpInit(&dat.op, gsub, -1, MBUFOPERATE_ENDIAN_BIG);

	//------- GSUB ヘッダ

	/* uint32 version
	 * uint16 ScriptList_offset
	 * uint16 FeatureList_offset
	 * uint16 LookupList_offset */

	//GSUB バージョン

	if(mBufOpGet32(&dat.op) != 0x10000) return FALSE;

	//各ブロックのオフセット

	mBufOpSeek(&dat.op, 2);
	dat.featurePos = mBufOpGet16(&dat.op);
	dat.lookupPos  = mBufOpGet16(&dat.op);

	//------- 解析

	if(!mMemAutoAlloc(&dat.mem, 1024, 1024)) return FALSE;

	_parseFeature(&dat);

	//結果をコピー

	if(dat.mem.curpos > 1)
	{
		buf = mMalloc(dat.mem.curpos, FALSE);
		if(buf) memcpy(buf, dat.mem.buf, dat.mem.curpos);
	}

	mMemAutoFree(&dat.mem);

	return buf;
}

/** グリフを縦書き用グリフに置換
 *
 * @param glyph FT_Get_Char_Index() で取得したグリフ番号
 * @return 縦書き用がなければそのまま返る */

uint32_t mFTGSUB_getVertGlyph(void *table,uint32_t glyph)
{
	uint8_t *ps = (uint8_t *)table;
	uint16_t *pw;
	int type,num,index,i;

	if(!table) return glyph;

	while(1)
	{
		type = *(ps++);
		if(type == ENDMARK) break;
	
		//------ 置き換え元のグリフ検索

		num = *((uint16_t *)ps);
		ps += 2;

		pw = (uint16_t *)ps;
		index = -1;

		if(type == SRC_GLYPH_TYPE_LIST)
		{
			//グリフIDのリスト
			
			for(i = 0; i < num; i++, pw++)
			{
				if(*pw == glyph)
				{
					index = i;
					break;
				}
			}

			ps += num << 1;
		}
		else
		{
			//範囲

			for(i = 0; i < num; i++, pw += 3)
			{
				if(pw[0] <= glyph && glyph <= pw[1])
				{
					index = glyph - pw[0] + pw[2];
					break;
				}
			}

			ps += num * (3 * 2);
		}

		//------- 置き換え後のグリフ

		type = *(ps++);

		if(index < 0)
		{
			//見つからなかった場合、次へ

			if(type == DST_GLYPH_TYPE_ADD)
				ps += 2;
			else
				ps += 2 + *((uint16_t *)ps) * 2;
		}
		else
		{
			//見つかった

			if(type == DST_GLYPH_TYPE_ADD)
				return glyph + *((int16_t *)ps);
			else
			{
				num = *((uint16_t *)ps);

				if(index < num)
					return *((uint16_t *)(ps + 2) + index);
				else
					return glyph;
			}
		}
	}

	return glyph;
}

/** @} */
