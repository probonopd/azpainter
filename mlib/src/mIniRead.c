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
 * mIniRead
 *****************************************/

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "mDef.h"

#include "mIniRead.h"
#include "mList.h"
#include "mStr.h"
#include "mUtil.h"
#include "mUtilStr.h"
#include "mUtilFile.h"
#include "mMemAuto.h"


//--------------------

#define _GROUPITEM(p) ((mIniReadGroup *)(p))
#define _ITEM(p)      ((mIniReadItem *)(p))


typedef struct
{
	mListItem i;
	char *key,
		*param;
	uint32_t hash;
}mIniReadItem;

typedef struct
{
	mListItem i;
	char *name;
	mIniReadItem *itemTop,*itemEnd; //項目がなければNULL
}mIniReadGroup;


struct _mIniRead
{
	char *buf;
	mList listGroup,
		listItem;

	mIniReadGroup *curgroup;
	mIniReadItem *curitem;
};

//--------------------


//=========================
// リスト作成
//=========================


/** ハッシュ値取得 */

static uint32_t _getHash(const char *pc)
{
	uint32_t h = 0;

	for(; *pc; pc++)
		h = h * 37 + (*pc);

	return h;
}

/** グループ追加 */

static mIniReadGroup *_addGroup(mList *list,char *pc)
{
	mIniReadGroup *pi;
	char *p2;

	p2 = strchr(pc, ']');
	if(p2) *p2 = 0;

	//グループ名が空

	if(*pc == 0) return NULL;

	//追加

	pi = (mIniReadGroup *)mListAppendNew(list, sizeof(mIniReadGroup), NULL);

	if(pi)
		pi->name = pc;

	return pi;
}

/** グループの情報セット */

static void _setGroupInfo(mIniRead *p,mIniReadGroup *group,mListItem *lastitem)
{
	if(!group) return;

	if(lastitem == p->listItem.bottom)
		//一つも項目がなかった場合、NULL
		group->itemTop = group->itemEnd = NULL;
	else
	{
		//項目の先頭と終端をセット
		
		group->itemTop = (lastitem)? _ITEM(lastitem->next): _ITEM(p->listItem.top);
		group->itemEnd = _ITEM(p->listItem.bottom);
	}
}

/** 項目追加 */

static void _addItem(mList *list,char *pc)
{
	char *p2;
	mIniReadItem *pi;

	//大文字に変換

	for(p2 = pc; *p2 && *p2 != '='; p2++)
	{
		if(*p2 >= 'a' && *p2 <= 'z')
			*p2 -= 0x20;
	}

	//'=' がない場合は無効

	if(*p2 != '=') return;
	*p2 = 0;

	//キー名が空

	if(*pc == 0) return;

	//追加

	pi = (mIniReadItem *)mListAppendNew(list, sizeof(mIniReadItem), NULL);
	if(!pi) return;

	pi->key = pc;
	pi->param = p2 + 1;
	pi->hash = _getHash(pc);
}


/** リスト作成 */

static void _createList(mIniRead *p)
{
	char *pc,*next,c;
	mIniReadGroup *piGroup = NULL;
	mListItem *piItemLast = NULL;

	for(pc = p->buf; 1; pc = next)
	{
		//行取得
		
		next = mGetStrNextLine(pc, TRUE);
		if(!next) break;

		//空行

		if(*pc == 0) continue;

		//----- 処理

		c = *pc;

		if(c == '[')
		{
			//---- グループ

			//前のグループの情報をセット

			_setGroupInfo(p, piGroup, piItemLast);

			//グループ追加

			piGroup = _addGroup(&p->listGroup, pc + 1);

			//最後の項目を記録

			piItemLast = p->listItem.bottom;
		}
		else if(isalnum(c) || c == '_')
		{
			//----- 項目

			_addItem(&p->listItem, pc);
		}
	}

	//最後のグループの情報セット

	_setGroupInfo(p, piGroup, piItemLast);
}


//=========================
// 検索
//=========================


/** カレントグループからキーを検索
 *
 * @return 存在しない場合 NULL */

char *_searchKey(mIniRead *p,const char *key)
{
	mStr strkey = MSTR_INIT;
	uint32_t hash;
	mIniReadItem *pi;
	int flag = 0;

	if(!p->curgroup) return NULL;

	//キー名を大文字に変換

	mStrSetText(&strkey, key);
	mStrUpper(&strkey);

	hash = _getHash(strkey.buf);

	//検索

	for(pi = p->curgroup->itemTop; pi; pi = _ITEM(pi->i.next))
	{
		if(pi->hash == hash && strcmp(pi->key, strkey.buf) == 0)
		{
			flag = 1;
			break;
		}
	
		if(pi == p->curgroup->itemEnd) break;
	}

	mStrFree(&strkey);

	//結果

	return (flag)? pi->param: NULL;
}


//=========================
// メイン
//=========================


/*******************//**

@defgroup iniread mIniRead
@brief INI ファイル読み込み

@ingroup group_etc
@{

@file mIniRead.h

*************************/


/** 終了 */

void mIniReadEnd(mIniRead *p)
{
	if(p)
	{
		mFree(p->buf);
		mListDeleteAll(&p->listGroup);
		mListDeleteAll(&p->listItem);

		mFree(p);
	}
}

/** ファイルから読み込み
 *
 * 読み込めなかった場合も mIniRead の確保はされる。 */

mIniRead *mIniReadLoadFile(const char *filename)
{
	mIniRead *ini;
	mBuf buf;

	//確保

	ini = (mIniRead *)mMalloc(sizeof(mIniRead), TRUE);
	if(!ini) return NULL;

	//読み込み

	if(!mReadFileFull(filename, MREADFILEFULL_ADD_NULL, &buf))
		return ini;

	//初期化

	ini->buf = (char *)buf.buf;

	_createList(ini);

	ini->curitem = _ITEM(ini->listItem.top);

/*
	//debug

	mIniReadGroup *pig;
	mIniReadItem *pii;

	mDebug("[group]\n");

	for(pig = _GROUPITEM(ini->listGroup.top); pig; pig = _GROUPITEM(pig->i.next))
		mDebug("%s top:%p end:%p\n", pig->name, pig->itemTop, pig->itemEnd);

	mDebug("[item]\n");

	for(pii = _ITEM(ini->listItem.top); pii; pii = _ITEM(pii->i.next))
		mDebug("%p key:%s param:%s hash:0x%x\n", pii, pii->key, pii->param, pii->hash);
*/

	return ini;
}

/** ファイルから読み込み
 *
 * @param path ディレクトリ (NULL 可)
 * @param filename path からの相対パス */

mIniRead *mIniReadLoadFile2(const char *path,const char *filename)
{
	mStr str = MSTR_INIT;
	mIniRead *p;

	if(path)
	{
		mStrSetText(&str, path);
		mStrPathAdd(&str, filename);
	}
	else
		mStrSetText(&str, filename);

	p = mIniReadLoadFile(str.buf);

	mStrFree(&str);

	return p;
}

/** データを空にする */

void mIniReadEmpty(mIniRead *p)
{
	mFree(p->buf);
	mListDeleteAll(&p->listGroup);
	mListDeleteAll(&p->listItem);

	p->buf = NULL;
	p->curgroup = NULL;
	p->curitem = NULL;
}


//=======================
// グループ
//=======================


/** グループセット時
 *
 * @return グループ名 */

static const char *_setGroup(mIniRead *p)
{
	if(p->curgroup)
	{
		p->curitem = p->curgroup->itemTop;
		return p->curgroup->name;
	}
	else
	{
		p->curitem = NULL;
		return NULL;
	}
}

/** グループ名からカレントグループセット */

mBool mIniReadSetGroup(mIniRead *p,const char *name)
{
	mIniReadGroup *pi;

	p->curgroup = NULL;
	p->curitem = NULL;

	for(pi = _GROUPITEM(p->listGroup.top); pi; pi = _GROUPITEM(pi->i.next))
	{
		if(strcasecmp(name, pi->name) == 0)
		{
			p->curgroup = pi;
			_setGroup(p);

			return TRUE;
		}
	}

	return FALSE;
}

/** 番号からカレントグループセット */

mBool mIniReadSetGroup_int(mIniRead *p,int no)
{
	char m[32];

	mIntToStr(m, no);

	return mIniReadSetGroup(p, m);
}

/** 最初のグループにセット */

const char *mIniReadSetFirstGroup(mIniRead *p)
{
	p->curgroup = _GROUPITEM(p->listGroup.top);

	return _setGroup(p);
}

/** 次のグループにセット
 *
 * @return グループ名。NULL で終了 */

const char *mIniReadSetNextGroup(mIniRead *p)
{
	if(!p->curgroup)
		return NULL;
	else
	{
		p->curgroup = _GROUPITEM((p->curgroup)->i.next);

		return _setGroup(p);
	}
}

/** 現在のグループの項目数取得 */

int mIniReadGetGroupItemNum(mIniRead *p)
{
	mIniReadItem *pi;
	int num;

	if(!p->curgroup) return 0;

	for(pi = p->curgroup->itemTop, num = 0; pi; pi = _ITEM(pi->i.next))
	{
		num++;
		if(pi == p->curgroup->itemEnd) break;
	}

	return num;
}

/** 現在のグループ内に指定キーの値があるか */

mBool mIniReadIsHaveKey(mIniRead *p,const char *key)
{
	return (_searchKey(p, key) != NULL);
}


//======================
// 値取得
//======================


/** グループ内の次の項目を取得 */

mBool mIniReadGetNextItem(mIniRead *p,const char **key,const char **param)
{
	if(!p->curitem)
		return FALSE;
	else
	{
		*key = p->curitem->key;
		*param = p->curitem->param;

		//次へ

		if(p->curgroup && p->curgroup->itemEnd == p->curitem)
			//グループの最後なら次で終了
			p->curitem = NULL;
		else
			//次へ
			p->curitem = _ITEM(p->curitem->i.next);

		return TRUE;
	}
}

/** グループ内の次の項目からキー番号と 32bit 数値取得 */

mBool mIniReadGetNextItem_nonum32(mIniRead *p,int *keyno,void *buf,mBool hex)
{
	const char *key,*param;

	if(!mIniReadGetNextItem(p, &key, &param))
		return FALSE;
	else
	{
		*keyno = atoi(key);

		if(hex)
			*((uint32_t *)buf) = strtoul(param, NULL, 16);
		else
			*((int32_t *)buf) = atoi(param);

		return TRUE;
	}
}

/** int 値取得 */

int mIniReadInt(mIniRead *p,const char *key,int def)
{
	char *pc = _searchKey(p, key);

	if(pc)
		return atoi(pc);
	else
		return def;
}

/** 16進数数値取得 */

uint32_t mIniReadHex(mIniRead *p,const char *key,uint32_t def)
{
	char *pc = _searchKey(p, key);

	if(pc)
		return (uint32_t)strtoul(pc, NULL, 16);
	else
		return def;
}

/** 文字列取得
 *
 * @return 見つかった場合、mIniRead バッファ内のポインタが返るので解放しないこと */

const char *mIniReadText(mIniRead *p,const char *key,const char *def)
{
	char *pc = _searchKey(p, key);

	return (pc)? pc: def;
}

/** バッファに文字列取得
 *
 * @param size バッファのサイズ
 * @param def  NULL で空文字列
 * @return セットされた文字列の長さ */

int mIniReadTextBuf(mIniRead *p,const char *key,char *buf,int size,const char *def)
{
	char *pc = _searchKey(p, key);
	int len;

	if(!pc) pc = (char *)def;

	if(!pc)
	{
		//空文字列
		buf[0] = 0;
		return 0;
	}
	else
	{
		len = strlen(pc);
		if(len >= size) len = size - 1;

		memcpy(buf, pc, len);
		buf[len] = 0;

		return len;
	}
}

/** 文字列を mStr で取得
 *
 * @param def NULL で空文字列 */

mBool mIniReadStr(mIniRead *p,const char *key,mStr *str,const char *def)
{
	char *pc = _searchKey(p, key);

	mStrSetText(str, (pc)? pc: def);

	return (pc != NULL);
}

/** キー名が番号の文字列取得 */

mBool mIniReadNoStr(mIniRead *p,int keyno,mStr *str,const char *def)
{
	char m[16];

	mIntToStr(m, keyno);

	return mIniReadStr(p, m, str, def);
}

/** キー名が番号の文字列配列取得
 *
 * デフォルトは空文字列。 */

void mIniReadNoStrs(mIniRead *p,int keytop,mStr *array,int maxnum)
{
	int i;

	for(i = 0; i < maxnum; i++)
		mIniReadNoStr(p, keytop + i, array + i, NULL);
}

/** カンマで区切られた複数数値取得
 *
 * @param bufnum 最大個数
 * @param bytes １つのデータのバイト数
 * @param hex 16進数
 * @return 取得された数 */

int mIniReadNums(mIniRead *p,const char *key,void *buf,int bufnum,int bytes,mBool hex)
{
	uint8_t *pd = (uint8_t *)buf;
	char *srcbuf,*ps;
	int n,num = 0;

	srcbuf = mGetSplitCharReplaceStr(mIniReadText(p, key, ""), ',');
	if(!srcbuf) return 0;

	for(ps = srcbuf; *ps && num < bufnum; ps += strlen(ps) + 1)
	{
		if(hex)
			n = strtoul(ps, NULL, 16);
		else
			n = atoi(ps);

		//セット

		switch(bytes)
		{
			case 2:
				*((uint16_t *)pd) = n;
				pd += 2;
				break;
			case 4:
				*((int32_t *)pd) = n;
				pd += 4;
				break;
			default:
				*(pd++) = n;
				break;
		}

		num++;
	}

	mFree(srcbuf);

	return num;
}

/** カンマで区切られた値を配列としてメモリ確保して取得
 *
 * @param append_bytes 終端に追加するバイト数
 * @param psize NULL 以外の場合、確保したバイト数が入る
 * @return 確保したバッファ (空の場合は NULL) */

void *mIniReadNums_alloc(mIniRead *p,const char *key,int bytes,mBool hex,int append_bytes,int *psize)
{
	mMemAuto mem;
	char *srcbuf,*ps;
	uint8_t *dstbuf;
	int32_t n;
	uint16_t n16;
	mBool ret;

	if(psize) *psize = 0;

	//カンマを NULL に置き換えた文字列

	srcbuf = mGetSplitCharReplaceStr(mIniReadText(p, key, ""), ',');
	if(!srcbuf) return NULL;

	//作業用バッファ

	mMemAutoInit(&mem);
	if(!mMemAutoAlloc(&mem, bytes * 64, bytes * 64)) goto ERR;

	//データ取得

	for(ps = srcbuf; *ps; ps += strlen(ps) + 1)
	{
		if(hex)
			n = strtoul(ps, NULL, 16);
		else
			n = atoi(ps);

		switch(bytes)
		{
			case 2:
				n16 = n;
				ret = mMemAutoAppend(&mem, &n16, 2);
				break;
			case 4:
				ret = mMemAutoAppend(&mem, &n, 4);
				break;
			default:
				ret = mMemAutoAppendByte(&mem, n);
				break;
		}

		if(!ret) goto ERR;
	}

	mFree(srcbuf);
	srcbuf = NULL;

	//追加サイズ

	if(append_bytes > 0
		&& !mMemAutoAppendZero(&mem, append_bytes))
		goto ERR;

	//結果

	if(mem.curpos)
		dstbuf = (uint8_t *)mMemdup(mem.buf, mem.curpos);
	else
		dstbuf = NULL;

	if(psize && dstbuf)
		*psize = mem.curpos;

	mMemAutoFree(&mem);

	return dstbuf;

ERR:
	mFree(srcbuf);
	mMemAutoFree(&mem);
	return NULL;
}

/** カンマで区切られた mPoint 取得 */

mBool mIniReadPoint(mIniRead *p,const char *key,mPoint *pt,int defx,int defy)
{
	int n[2];

	if(mIniReadNums(p, key, n, 2, 4, FALSE) < 2)
	{
		pt->x = defx;
		pt->y = defy;
		return FALSE;
	}
	else
	{
		pt->x = n[0];
		pt->y = n[1];
		return TRUE;
	}
}

/** カンマで区切られた mSize 取得 */

mBool mIniReadSize(mIniRead *p,const char *key,mSize *size,int defw,int defh)
{
	int n[2];

	if(mIniReadNums(p, key, n, 2, 4, FALSE) < 2)
	{
		size->w = defw;
		size->h = defh;
		return FALSE;
	}
	else
	{
		size->w = n[0];
		size->h = n[1];
		return TRUE;
	}
}

/** カンマで区切られた mBox 取得 */

mBool mIniReadBox(mIniRead *p,const char *key,mBox *box,int defx,int defy,int defw,int defh)
{
	int n[4];

	if(mIniReadNums(p, key, n, 4, 4, FALSE) < 4)
	{
		box->x = defx;
		box->y = defy;
		box->w = defw;
		box->h = defh;
		return FALSE;
	}
	else
	{
		box->x = n[0];
		box->y = n[1];
		box->w = n[2];
		box->h = n[3];
		return TRUE;
	}
}

/** Base64 コードから取得
 *
 * @param psize  NULL 以外で、データのサイズが入る
 * @return 確保されたバッファが返る */

void *mIniReadBase64(mIniRead *p,const char *key,int *psize)
{
	char *pc = _searchKey(p, key);
	int srcsize;
	uint8_t *buf;

	if(!pc) return NULL;

	srcsize = atoi(pc);

	if(srcsize <= 0) return NULL;

	//データ位置

	for(; *pc && *pc != ':'; pc++);

	if(*pc == 0) return NULL;

	pc++;

	//確保

	buf = (uint8_t *)mMalloc(srcsize, FALSE);
	if(!buf) return NULL;

	//デコード

	if(mBase64Decode(buf, srcsize, pc, -1) < 0)
	{
		mFree(buf);
		return NULL;
	}

	if(psize) *psize = srcsize;

	return buf;
}

/** ファイルダイアログ設定を読み込み */

mBool mIniReadFileDialogConfig(mIniRead *p,const char *key,void *val)
{
	mMemzero(val, 3 * 2);

	return mIniReadNums(p, key, val, 3, 2, TRUE);
}

/** @} */
