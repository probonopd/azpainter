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

/********************************
 * アンドゥ管理データ
 ********************************/

typedef struct _mZlibEncode mZlibEncode;
typedef struct _mZlibDecode mZlibDecode;

typedef struct
{
	mUndo undo;

	UndoUpdateInfo update;	//(作業用) 更新情報
	uint8_t change;			//データが変更されたかのフラグに使う

	uint32_t used_bufsize;	//現在使われているアンドゥバッファの総サイズ
	int cur_fileno,			//ファイル番号の現在値
		write_type,			//書き込み時の出力タイプ
		write_tmpsize,		//一時バッファに書き込まれたサイズ
		write_remain;		//可変書き込み時の残りサイズ
	uint8_t *writetmpbuf,	//書き込み時の一時出力バッファ
		*workbuf1,			//タイルイメージ用バッファ
		*workbuf2,
		*workbuf_flags,
		*write_dst,			//書き込み位置
		*read_dst;			//読み込み位置
	FILE *writefp,
		*readfp;
	fpos_t writefpos;		//[file] 書き込み位置記録

	mZlibEncode *zenc;	//[file] レイヤタイル圧縮用
	mZlibDecode *zdec;	//[file] レイヤタイル展開用
}UndoMaster;

extern UndoMaster *g_app_undo;

#define UNDO_WRITETEMPBUFSIZE  (128 * 1024)
