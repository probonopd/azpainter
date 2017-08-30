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
 * キャンバスキーコマンドID
 ***************************************/

#ifndef DEF_CANVASKEY_ID_H
#define DEF_CANVASKEY_ID_H

enum
{
	CANVASKEYID_CMD_TOOL = 1,		//ツール変更
	CANVASKEYID_CMD_DRAWTYPE = 30,	//描画タイプ変更
	CANVASKEYID_CMD_OTHER = 60,		//他コマンド
	CANVASKEYID_OP_TOOL = 100,		//+キー:ツール動作
	CANVASKEYID_OP_BRUSHDRAW = 130,	//+キー:ブラシ描画
	CANVASKEYID_OP_SELECT = 150,	//+キー:選択範囲ツール
	CANVASKEYID_OP_OTHER = 160,		//+キー:他

	CANVASKEY_CMD_OTHER_NUM = 12,	//他コマンドの数
	CANVASKEY_OP_OTHER_NUM = 9		//+キー:他の数
};

//+キー操作のコマンドか
#define CANVASKEY_IS_PLUS_MOTION(id)  ((id) >= CANVASKEYID_OP_TOOL && (id) < CANVASKEYID_OP_OTHER + CANVASKEY_OP_OTHER_NUM)

#define CANVASKEYID_CMD_OTHER_ZOOM_UP    (CANVASKEYID_CMD_OTHER + 4)
#define CANVASKEYID_CMD_OTHER_ZOOM_DOWN  (CANVASKEYID_CMD_OTHER + 5)

#endif
