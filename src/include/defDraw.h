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

/******************************
 * DrawData 定義
 ******************************/

#ifndef DEF_DRAW_H
#define DEF_DRAW_H

#include "mStrDef.h"

#include "defDrawGlobal.h"
#include "defCanvasInfo.h"
#include "defTool.h"
#include "ColorValue.h"

typedef struct _ImageBufRGB16 ImageBufRGB16;
typedef struct _ImageBuf8   ImageBuf8;
typedef struct _TileImage   TileImage;
typedef struct _LayerList   LayerList;
typedef struct _LayerItem   LayerItem;
typedef struct _FillPolygon FillPolygon;
typedef struct _DrawFont DrawFont;


#define COLORMASK_MAXNUM  6		//色マスクの色最大数
#define COLPAL_GRADNUM    3		//中間色バーの数
#define LAYERCOLPAL_NUM   30	//レイヤ色パレットの数
#define RULE_RECORD_NUM   10	//定規の記録数
#define HEADTAIL_RECORD_NUM 10	//入り抜き記録数


/*---- data ----*/


/** 描画用ポイントデータ */

typedef struct _DrawPoint
{
	double x,y,pressure;
}DrawPoint;

/** 定規の記録データ */

typedef struct
{
	uint8_t type;
	double d[4];
}RuleRecord;


/*---- sub data ----*/

/** ツールデータ */

typedef struct
{
	int no;			//現在のツール
	uint8_t subno[TOOL_NUM],	//ツールのサブ情報
		opt_move,
		opt_stamp;

	uint16_t opt_fillpoly,
		opt_fillpoly_erase,
		opt_magicwand;
	uint32_t opt_grad,
		opt_fill;
}DrawToolData;

/** 色関連データ */

typedef struct
{
	uint32_t drawcol,	//描画色(8bit)
		bkgndcol,		//背景色
		gradcol[COLPAL_GRADNUM][2],		//中間色、左右の色 (左の色の上位8bitは段階数)
		layercolpal[LAYERCOLPAL_NUM];	//レイヤ色パレット
	int32_t colmask_col[COLORMASK_MAXNUM + 1];	//マスク色。-1 で終端
	uint16_t colpal_max_column;	//カラーパレットの横最大数
	uint8_t colmask_type,		//色マスクタイプ (0:OFF 1:MASK 2:REV)
		colmask_num,			//色マスク、色数
		hlspal_sel,				//HLSパレット:H選択位置
		hlspal_palx,			//HLSパレット:パレット部分 XY 位置
		hlspal_paly,
		colpal_sel,			//カラーパレットのタブ選択
		colpal_cellw,		//カラーパレットの表示幅
		colpal_cellh;

	RGBFix15 rgb15DrawCol,	//描画色/背景色 15bitカラー
		rgb15BkgndCol,
		rgb15Colmask[COLORMASK_MAXNUM + 1];	//マスク色(15bit) r=0xffff で終端
}DrawColorData;

/** 定規データ */

typedef struct
{
	int type,
		ntmp;
	double parallel_rd,	//平行線角度(ラジアン)
		grid_rd,		//グリッド線角度
		ellipse_rd,		//楕円角度
		ellipse_yx,		//楕円扁平率 (y/x)
		dtmp[3];
	mDoublePoint press_pos,	//ボタン押し時のイメージ位置
		conc_pos,			//集中線の中心位置
		circle_pos,
		ellipse_pos;

	RuleRecord record[RULE_RECORD_NUM];  //記録データ

	void (*funcGetPoint)(DrawData *,double *,double *);	//点を補正する関数
}DrawRuleData;

/** 入り抜きデータ */

typedef struct
{
	uint8_t selno;  	//0:直線 1:ベジェ曲線
	uint32_t curval[2],	//現在の値 (上位16bit:入り 下位:抜き)
		record[HEADTAIL_RECORD_NUM];  //登録リスト
}DrawHeadTailData;

/** テキスト描画データ */

typedef struct
{
	DrawFont *font;		//テキスト描画用フォント

	mStr strText,		//現在の文字列
		strName,		//フォント名
		strStyle;		//フォントスタイル
	int size,
		char_space,
		line_space;
	uint8_t weight,
		slant,
		hinting,
		flags,
		in_dialog;		//ダイアログ表示中か
}DrawTextData;

/** 選択範囲用データ */

typedef struct
{
	mRect rcsel;	//選択のおおよその範囲 (tileimgSel != NULL の時)
	
	TileImage *tileimgCopy;	//コピーされたイメージ (ファイル出力失敗時)
	uint32_t col_copyimg;	//コピーされたイメージの線の色
}DrawSelectData;

/** 矩形編集用データ */

typedef struct
{
	mBox box;			//範囲 (w == 0 で範囲なし)
	uint8_t cursor_resize;	//現在のリサイズカーソル状態(フラグ)
}DrawBoxEditData;

/** スタンプ用データ */

typedef struct
{
	TileImage *img;	//スタンプ画像
	mSize size;		//スタンプ画像のサイズ
}DrawStampData;

/** 作業用データ */

typedef struct
{
	int ntmp[3],
		optype,				//現在の操作タイプ
		opsubtype,			//操作タイプのサブ情報
		optoolno,			//現在操作中のツール動作
		press_btt,			//操作開始時に押されたボタン (M_BTT_*)
		release_only_btt,	//ボタン番号。ボタン離し時は指定されたボタンのみ処理する
		drag_cursor_type;	//(ボタン押し時) グラブ中のみカーソルを変更する場合、APP_CURSOR_* を指定。
							//-1 でカーソルはそのまま

	uint32_t press_state;	//操作開始時の装飾キー (M_MODS_*)

	uint8_t opflags,		//操作用オプションフラグ
		drawinfo_flags,		//drawOpSub_setDrawInfo() でセットされるフラグ
		in_filter_dialog,	//フィルタダイアログ中か (TRUE 時、ptmp = ダイアログのポインタ)
		hide_canvas_select;	//キャンバス描画で選択範囲の枠を非表示にするか (範囲イメージ移動で、移動中枠を表示しない場合)

	mPoint ptLastArea,		//前回のカーソル位置 (int)
		pttmp[4];
	mDoublePoint ptd_tmp[1];
	mRect rcdraw,			//描画範囲
		rctmp[1];
	mBox boxtmp[1];

	DrawPoint dptAreaPress,	//操作開始時のカーソル位置
		dptAreaCur,			//現在のカーソル位置
		dptAreaLast;		//前回のカーソル位置

	void *ptmp;
	uint64_t sec_midcol;	//中間色作成の最初の押し時間 (sec)

	RGBAFix15 rgbaDraw;		//描画色

	TileImage *dstimg,		//描画先イメージ
		*tileimg_filterprev;	//フィルタのキャンバスプレビュー時の挿入イメージ (NULL でなし)
	LayerItem *layer;		//リンクの先頭レイヤ

	FillPolygon *fillpolygon;

	void (*funcMotion)(DrawData *,uint32_t);		//ポインタ移動時の処理関数。NULL で何もしない
	mBool (*funcRelease)(DrawData *);				//ボタン離し時の処理関数。NULL でグラブ解放のみ行う
	void (*funcPressInGrab)(DrawData *p,uint32_t);	//グラブ中にボタンが押された時の処理関数
	mBool (*funcAction)(DrawData *p,int action);	//右ボタンやキーなどが押された時の処理関数
}DrawWorkData;


/*---- DrawData ----*/


/** DrawData */

typedef struct _DrawData
{
	int imgw,imgh,imgdpi;	//イメージ幅、高さ、DPI

	/* キャンバス用 */

	int canvas_zoom,		//表示倍率 (100% = 1000)
		canvas_angle;		//表示角度 (1度 = 100)
	uint8_t canvas_mirror,	//左右反転表示
		bCanvasLowQuality;	//キャンバス表示を低品質で行うか
	mSize szCanvas;			//キャンバスの領域サイズ
	mPoint ptScroll;		//キャンバススクロール位置
	double imgoriginX,		//イメージの原点位置
		imgoriginY;

	RGBFix15 rgb15ImgBkgnd,	//イメージ背景色
		rgb15BkgndPlaid[2];	//背景チェック柄の色

	CanvasViewParam viewparam;	//座標計算用パラメータ

	//

	DrawWorkData w;
	DrawColorData col;
	DrawToolData tool;
	DrawRuleData rule;
	DrawHeadTailData headtail;
	DrawStampData stamp;
	DrawSelectData sel;
	DrawBoxEditData boxedit;
	DrawTextData drawtext;

	//

	mStr strOptTexPath;		//オプションテクスチャのファイルパス

	ImageBufRGB16 *blendimg;	//全レイヤ合成後のイメージ
	ImageBuf8 *img8OptTex;		//オプションテクスチャの現在イメージ

	TileImage *tileimgSel,	//選択範囲用 (1bit)
		*tileimgDraw,		//描画用作業イメージ (描画部分の元イメージ保存用)
		*tileimgDraw2,		//(ブラシ描画時の描画濃度バッファ)
		*tileimgTmp;		//操作中のみの作業用イメージ

	LayerList *layerlist;	//レイヤリスト
	LayerItem *curlayer,	//カレントレイヤ
		*masklayer;			//マスクレイヤ
}DrawData;


/** DrawTextData::flags */

enum
{
	DRAW_DRAWTEXT_F_PREVIEW = 1<<0,
	DRAW_DRAWTEXT_F_VERT = 1<<1,
	DRAW_DRAWTEXT_F_ANTIALIAS = 1<<2,
	DRAW_DRAWTEXT_F_SIZE_PIXEL = 1<<3
};

#endif
