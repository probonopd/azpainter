/*$
Copyright (c) 2014-2017, Azel
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
$*/

/***************************************************
 * mtrconv.c
 * テキストからバイナリの翻訳ファイル (mtr) へ変換
 ***************************************************
 *
 * Convert text to mtr file.
 * mtr is binary translation file.
 *
 *  Usage
 * ----------------------
 *
 * mtrconv [-h] inputfile outputfile
 *
 * -h : to output header file for embedding into executable file.
 *
 * 
 *  Text file format
 * ----------------------
 *
 * [GroupID]
 * <id>=<string>
 * +=<string>
 * ...
 *
 * > UTF-8
 * > group or string ID : 0..65535
 * > The beginning of line is not '[' or numbers or '+', Comments.
 * > '\\' => '\', '\n' => Enter、'\t' => Tab
 *
 * 
 *  Compile by gcc
 * ------------------------
 * 
 * $ gcc -Wall -O2 -s -o mtrconv mtrconv.c
 *
 ****************************************/
/*
 * ver 1.01 : 行末の空白は文字列から除外するようにした
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


//------------------

#define _RET_OK  0
#define _RET_ERR 1

//------------------

typedef struct _ITEM_STR   ITEM_STR;
typedef struct _ITEM_GROUP ITEM_GROUP;

struct _ITEM_STR
{
	ITEM_STR *next;

	char *text;
	uint16_t id,len;
};

struct _ITEM_GROUP
{
	ITEM_GROUP *next;

	ITEM_STR *str_top,
		*str_last;

	uint16_t id;
	int itemnum;
	uint32_t strsize;
};


typedef struct
{
	ITEM_GROUP *group_top,
		*group_last;
	int groupnum;
	int curid;
}CONVERTDAT;

//-----------------

int g_bOutputHeader = 0;

//-----------------


//=========================
// sub
//=========================


/** テキストファイルをバッファに読み込み */

static char *_read_text(char *filename)
{
	FILE *fp;
	long size;
	char *buf;

	fp = fopen(filename, "rb");
	if(!fp) return NULL;

	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	buf = (char *)malloc(size + 1);
	if(!buf)
	{
		fclose(fp);
		return NULL;
	}

	fread(buf, 1, size, fp);
	*(buf + size) = 0;

	fclose(fp);

	return buf;
}

/** 改行を NULL に変換して、次の行のポインタを取得 */

static char *_get_nextline(char *buf)
{
	char *pc,*pc2;

	if(!*buf) return NULL;

	//改行か終端まで移動

	for(pc = buf; *pc && *pc != '\r' && *pc != '\n'; pc++);

	//行末の空白は削除

	for(pc2 = pc - 1; pc2 > buf && *pc2 == ' '; *(pc2--) = 0);

	//

	if(*pc == '\r' && pc[1] == '\n')
	{
		*pc = 0, pc[1] = 0;
		pc += 2;
	}
	else if(*pc)
	{
		*pc = 0;
		pc++;
	}

	return pc;
}

/** エスケープシーケンスの変換 */

static void _convert_str_escape(char *text)
{
	char c,*ps,*pd;

	ps = pd = text;

	while(*ps)
	{
		c = *(ps++);
	
		if(c == '\\')
		{
			c = *(ps++);

			if(c == 0)
				break;
			else if(c == 'n')
				c = '\n';
			else if(c == 't')
				c = '\t';
		}

		*(pd++) = c;
	}

	*pd = 0;
}

/** バッファに 16bit 値追加 (BE) */

static void _addbuf16(uint8_t **pp,uint16_t val)
{
	uint8_t *pd = *pp;

	pd[0] = (uint8_t)(val >> 8);
	pd[1] = (uint8_t)val;

	*pp += 2;
}

/** バッファに 32bit 値追加 (BE) */

static void _addbuf32(uint8_t **pp,uint32_t val)
{
	uint8_t *pd = *pp;

	pd[0] = (uint8_t)(val >> 24);
	pd[1] = (uint8_t)(val >> 16);
	pd[2] = (uint8_t)(val >> 8);
	pd[3] = (uint8_t)val;

	*pp += 4;
}


//=========================
// リスト作成
//=========================


/** グループ開始 */

static int _start_group(CONVERTDAT *p,char *line)
{
	ITEM_GROUP *pi;
	int id;
	char *pc;

	//']' までが数字のみか

	for(pc = line + 1; *pc != ']' && *pc >= '0' && *pc <= '9'; pc++);

	if(*pc != ']')
	{
		printf("error: group name '%s'\n", line);
		return _RET_ERR;
	}

	//グループID

	id = atoi(line + 1);

	if(id > 65535)
	{
		printf("error: group id [%d]\n", id);
		return _RET_ERR;
	}

	//グループ追加

	pi = (ITEM_GROUP *)malloc(sizeof(ITEM_GROUP));
	if(!pi) return _RET_ERR;

	memset(pi, 0, sizeof(ITEM_GROUP));

	pi->id = id;

	//リンク

	if(!p->group_top)
		p->group_top = pi;

	if(p->group_last)
		p->group_last->next = pi;

	p->group_last = pi;

	//グループ数

	p->groupnum++;

	return _RET_OK;
}

/** 項目の追加 */

static int _add_stritem(CONVERTDAT *p,char *line)
{
	char *pc;
	ITEM_STR *pi;
	ITEM_GROUP *pg;
	int id;

	//先頭が正しいか。また、pc を値部分まで進める。

	pc = line;

	if(*pc == '+')
	{
		//"+=" : 前回の ID を +1 した値
		
		if(pc[1] != '=')
		{
			printf("error: invalid line '%s'\n", line);
			return _RET_ERR;
		}

		pc += 2;
		id = ++(p->curid);
	}
	else
	{
		//"数値="
		
		for(; *pc >= '0' && *pc <= '9'; pc++);

		if(*pc != '=')
		{
			printf("error: invalid line '%s'\n", line);
			return _RET_ERR;
		}

		pc++;
		id = p->curid = atoi(line);
	}

	//'=' 以降の文字列調整

	_convert_str_escape(pc);

	//追加

	pi = (ITEM_STR *)malloc(sizeof(ITEM_STR));
	if(!pi) return _RET_ERR;

	pi->next = 0;
	pi->id   = id;
	pi->text = strdup(pc);
	pi->len  = strlen(pc) + 1;

	//リンク

	pg = p->group_last;

	if(!pg->str_top)
		pg->str_top = pi;
	
	if(pg->str_last)
		pg->str_last->next = pi;

	pg->str_last = pi;

	//グループの項目数、文字列サイズ

	pg->itemnum++;
	pg->strsize += pi->len;

	return _RET_OK;
}

/** リスト作成 */

static int _create_list(CONVERTDAT *p,char *txtbuf)
{
	char *pc,*next;

	for(pc = txtbuf; 1; pc = next)
	{
		//1行取得
		
		next = _get_nextline(pc);
		if(!next) break;

		//

		if(*pc == '[')
		{
			//グループ開始

			if(_start_group(p, pc) != _RET_OK)
				return _RET_ERR;

			p->curid = -1;
		}
		else if(((*pc >= '0' && *pc <= '9') || *pc == '+')
			&& strchr(pc, '=')
			&& p->group_last)
		{
			//文字列項目
			/* 先頭が数字か '+' で、'=' があり、グループが開始されている */

			if(_add_stritem(p, pc) != _RET_OK)
				return _RET_ERR;
		}
	}

	return _RET_OK;
}


//=========================
// 出力
//=========================


/** バイナリファイルに出力 */

static int _write_binary(char *filename,uint8_t *buf,uint32_t size)
{
	FILE *fp;
	uint8_t ver = 0;

	fp = fopen(filename, "wb");
	if(!fp) return _RET_ERR;

	fputs("MLIBTR", fp);

	fwrite(&ver, 1, 1, fp);

	fwrite(buf, 1, size, fp);

	fclose(fp);

	return _RET_OK;
}

/** ヘッダファイルとして出力 */

static int _write_headerfile(char *filename,uint8_t *buf,uint32_t size)
{
	FILE *fp;
	int i,j,remain;
	uint8_t *ps;

	fp = fopen(filename, "wb");
	if(!fp) return _RET_ERR;

	fputs("const unsigned char g_deftransdat[] = {\n", fp);

	ps = buf;
	remain = size;

	for(i = (remain + 15) >> 4; i > 0; i--)
	{
		for(j = 0; j < 16 && remain; j++, remain--)
		{
			fprintf(fp, "0x%02x", *(ps++));

			if(remain != 1) fputc(',', fp);
		}

		fputc('\n', fp);
	}

	fputs("};\n", fp);

	fclose(fp);

	return _RET_OK;
}


//=========================
// 変換
//=========================


/** リストデータ解放 */

static void _delete_list(CONVERTDAT *p)
{
	ITEM_GROUP *pg,*pg_next;
	ITEM_STR *pi,*pi_next;

	//グループ

	for(pg = p->group_top; pg; pg = pg_next)
	{
		pg_next = pg->next;
	
		//文字列
	
		for(pi = pg->str_top; pi; pi = pi_next)
		{
			pi_next = pi->next;
			
			free(pi->text);
			free(pi);
		}

		free(pg);
	}
}

/** メモリに全体データ出力 (BigEndian) */

static uint8_t *_create_buf(CONVERTDAT *p,uint32_t *bufsize)
{
	ITEM_GROUP *pg;
	ITEM_STR *pi;
	uint8_t *buf,*pd;
	uint32_t size,strsize,itemnum,offset;

	//(ヘッダを除く) 全体のサイズ計算

	strsize = itemnum = 0;

	for(pg = p->group_top; pg; pg = pg->next)
	{
		strsize += pg->strsize;
		itemnum += pg->itemnum;
	}

	size = 4 * 3 + p->groupnum * 8 + itemnum * 6 + strsize;

	//メモリ確保

	buf = (uint8_t *)malloc(size);
	if(!buf) return NULL;

	*bufsize = size;

	//------- 出力

	pd = buf;

	//グループ数、項目全体数、全文字列サイズ

	_addbuf32(&pd, p->groupnum);
	_addbuf32(&pd, itemnum);
	_addbuf32(&pd, strsize);

	//グループリスト

	offset = 0;

	for(pg = p->group_top; pg; pg = pg->next)
	{
		_addbuf16(&pd, pg->id);       //ID
		_addbuf16(&pd, pg->itemnum);  //項目数
		_addbuf32(&pd, offset);       //先頭のオフセット位置

		offset += pg->itemnum * 6;
	}

	//文字列項目リスト

	offset = 0;

	for(pg = p->group_top; pg; pg = pg->next)
	{
		for(pi = pg->str_top; pi; pi = pi->next)
		{
			_addbuf16(&pd, pi->id);  //ID
			_addbuf32(&pd, offset);  //オフセット位置

			offset += pi->len;
		}
	}

	//文字列データ

	for(pg = p->group_top; pg; pg = pg->next)
	{
		for(pi = pg->str_top; pi; pi = pi->next)
		{
			memcpy(pd, pi->text, pi->len);

			pd += pi->len;
		}
	}

	return buf;
}

/** ファイル変換 */

static int _convert_file(char *inputname,char *outputname)
{
	CONVERTDAT dat;
	char *txtbuf;
	uint8_t *datbuf;
	uint32_t bufsize;
	int ret;

	memset(&dat, 0, sizeof(CONVERTDAT));

	//テキスト読み込み

	txtbuf = _read_text(inputname);
	if(!txtbuf)
	{
		printf("error: can't open '%s'\n", inputname);
		return _RET_ERR;
	}

	//リスト作成 (=> テキストバッファ解放)

	ret = _create_list(&dat, txtbuf);

	free(txtbuf);

	if(ret != _RET_OK)
	{
		_delete_list(&dat);
		return _RET_ERR;
	}

	//全体データをメモリに書き出し (=> リストデータ解放)

	datbuf = _create_buf(&dat, &bufsize);

	_delete_list(&dat);

	if(!datbuf) return _RET_ERR;

	//出力

	if(g_bOutputHeader)
		ret = _write_headerfile(outputname, datbuf, bufsize);
	else
		ret = _write_binary(outputname, datbuf, bufsize);

	free(datbuf);

	if(ret != _RET_OK)
		printf("error: can't write '%s'\n", outputname);

	return ret;
}


//===========================
// main
//===========================


/** コマンドライン引数取得
 *
 * return: 入力ファイルの位置。-1 でエラー */

static int _get_param(int argc,char **argv)
{
	int i;

	//オプション

	for(i = 1; i < argc; i++)
	{
		if(argv[i][0] == '-')
		{
			if(strcmp(argv[i], "-h") == 0)
				g_bOutputHeader = 1;
			else
				return -1;
		}
		else
			break;
	}

	//足りない

	if(i + 2 > argc) return -1;

	return i;
}

/** メイン */

int main(int argc,char **argv)
{
	int no;

	//コマンドライン引数

	no = _get_param(argc, argv);

	if(no < 0)
	{
		printf("<Usage> %s [-h] inputfile outputfile\n"
			"  -h : to output header file for embedding into executable file.\n"
			, argv[0]);

		return 1;
	}

	//変換

	if(_convert_file(argv[no], argv[no + 1]) == _RET_OK)
		printf("-OK\n");
	else
		printf("-Failed\n");

	return 0;
}
