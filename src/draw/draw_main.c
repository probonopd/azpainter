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
 * DrawData
 *
 * 初期化など
 *****************************************/

#include <time.h>

#include "mDef.h"
#include "mStr.h"
#include "mGui.h"
#include "mRandXorShift.h"

#include "defMacros.h"
#include "defDraw.h"
#include "defConfig.h"

#include "draw_main.h"
#include "draw_calc.h"
#include "draw_select.h"
#include "draw_boxedit.h"

#include "ImageBufRGB16.h"
#include "ImageBuf8.h"
#include "ImageBuf24.h"
#include "TileImage.h"
#include "LayerList.h"

#include "DrawFont.h"
#include "blendcol.h"
#include "SinTable.h"
#include "AppCursor.h"

#include "MainWinCanvas.h"
#include "StatusBar.h"
#include "Docks_external.h"



//=============================
// 初期化
//=============================


/** 解放 */

void DrawData_free()
{
	DrawData *p = APP_DRAW;

	if(p)
	{
		//テキスト描画
		
		DrawFont_free(p->drawtext.font);

		mStrFree(&p->drawtext.strText);
		mStrFree(&p->drawtext.strName);
		mStrFree(&p->drawtext.strStyle);

		//

		mStrFree(&p->strOptTexPath);
		
		ImageBufRGB16_free(p->blendimg);

		ImageBuf8_free(p->img8OptTex);

		TileImage_free(p->sel.tileimgCopy);

		TileImage_free(p->tileimgSel);
		TileImage_free(p->tileimgDraw);
		TileImage_free(p->tileimgDraw2);
		TileImage_free(p->tileimgTmp);

		TileImage_free(p->stamp.img);

		LayerList_free(p->layerlist);

		mFree(p);
	}

	TileImage_finish();
}

/** DrawData 作成
 *
 * [!] 設定ファイルの読み込み前に行う */

mBool DrawData_new()
{
	DrawData *p;

	p = (DrawData *)mMalloc(sizeof(DrawData), TRUE);
	if(!p) return FALSE;

	APP_DRAW = p;

	//値初期化

	p->canvas_zoom = 1000;
	p->szCanvas.w = p->szCanvas.h = 100;

	drawCalc_setCanvasViewParam(p);

	p->rule.ellipse_yx = 1;

	//初期化

	if(!TileImage_init()) return FALSE;

	BlendColor_setFuncTable();

	SinTable_init();

	//作成

	p->layerlist = LayerList_new();

	//乱数初期化

	mRandXorShift_init(time(NULL));

	return TRUE;
}

/** 設定ファイル読み込み後でウィジェット作成前の初期化 */

void drawInit_beforeCreateWidget()
{
	//オプションテクスチャ画像読み込み
	drawTexture_loadOptionTextureImage(APP_DRAW);
}

/** ウィンドウ表示前の初期化 */

void drawInit_beforeShow()
{
	DrawData *p = APP_DRAW;

	//8bit色値を 15bit 値に変換
	
	RGBtoRGBFix15(&APP_DRAW->col.rgb15DrawCol, p->col.drawcol);
	RGBtoRGBFix15(&APP_DRAW->col.rgb15BkgndCol, p->col.bkgndcol);

	drawColorMask_changeColor(p);

	drawConfig_changeCanvasColor(p);

	//新規イメージ

	drawImage_new(p, APP_CONF->init_imgw, APP_CONF->init_imgh,
		APP_CONF->init_dpi, TILEIMAGE_COLTYPE_RGBA);

	//合成

	drawUpdate_blendImage_full(p);

	//キャンバス状態

	drawCanvas_setScrollDefault(p);
	drawCanvas_setZoomAndAngle(p, 0, 0, 0, FALSE);
}

/** キャンバス関連の色が変更された時 */

void drawConfig_changeCanvasColor(DrawData *p)
{
	//イメージ背景色

	RGBtoRGBFix15(&p->rgb15ImgBkgnd, 0xffffff);

	//背景チェック柄

	RGBtoRGBFix15(p->rgb15BkgndPlaid, APP_CONF->colBkgndPlaid[0]);
	RGBtoRGBFix15(p->rgb15BkgndPlaid + 1, APP_CONF->colBkgndPlaid[1]);
}


//=============================
// ほか
//=============================


/** オプションのテクスチャイメージの読み込み
 *
 * DrawData::strOptTexPath に画像パスをセットしておく。
 *
 * @return イメージが読み込まれたか */

mBool drawTexture_loadOptionTextureImage(DrawData *p)
{
	mStr str = MSTR_INIT,*path;
	ImageBuf24 *img;
	mBool ret = FALSE;

	//イメージ解放
	
	ImageBuf8_free(p->img8OptTex);
	p->img8OptTex = NULL;

	//パスが空ならイメージ無し

	path = &p->strOptTexPath;

	if(mStrIsEmpty(path)) return FALSE;

	//絶対パス取得
	/* 先頭が '/' でデフォルトディレクトリ。
	 * そうでなければ、ユーザーディレクトリ。 */

	if(path->buf[0] == '/')
	{
		mAppGetDataPath(&str, APP_TEXTURE_PATH);
		mStrPathAdd(&str, path->buf + 1);
	}
	else
	{
		mStrCopy(&str, &APP_CONF->strUserTextureDir);
		mStrPathAdd(&str, path->buf);
	}

	//読み込み (24bit)

	img = ImageBuf24_loadFile(str.buf);
	if(!img) goto ERR;

	//8bit イメージ

	p->img8OptTex = ImageBuf8_createFromImageBuf24(img);
	if(!p->img8OptTex) goto ERR;

	ret = TRUE;

ERR:
	ImageBuf24_free(img);
	mStrFree(&str);

	return ret;
}

/** ツールのカーソル番号を取得 */

int drawCursor_getToolCursor(int toolno)
{
	int no;

	switch(toolno)
	{
		case TOOL_TEXT:
			no = APP_CURSOR_TEXT;
			break;
		case TOOL_MOVE:
			no = APP_CURSOR_MOVE;
			break;
		case TOOL_SELECT:
		case TOOL_BOXEDIT:
			no = APP_CURSOR_SELECT;
			break;
		case TOOL_STAMP:
			if(APP_DRAW->stamp.img)
				no = APP_CURSOR_STAMP;
			else
				no = APP_CURSOR_SELECT;
			break;
		case TOOL_CANVAS_ROTATE:
			no = APP_CURSOR_ROTATE;
			break;
		case TOOL_CANVAS_MOVE:
			no = APP_CURSOR_HAND;
			break;
		case TOOL_SPOIT:
			no = APP_CURSOR_SPOIT;
			break;
		default:
			no = APP_CURSOR_DRAW;
			break;
	}

	return no;
}


//=============================
// カラー関連
//=============================


/** 描画色の RGB 値 (8bit) を取得 */

void drawColor_getDrawColor_rgb(int *dst)
{
	uint32_t col = APP_DRAW->col.drawcol;
	
	dst[0] = M_GET_R(col);
	dst[1] = M_GET_G(col);
	dst[2] = M_GET_B(col);
}

/** 描画色を RGBAFix15 で取得 */

void drawColor_getDrawColor_rgbafix(RGBAFix15 *dst)
{
	RGBFix15 *ps = &APP_DRAW->col.rgb15DrawCol;

	dst->r = ps->r;
	dst->g = ps->g;
	dst->b = ps->b;
	dst->a = 0x8000;
}

/** 描画色セット */

void drawColor_setDrawColor(uint32_t col)
{
	APP_DRAW->col.drawcol = col & 0xffffff;

	RGBtoRGBFix15(&APP_DRAW->col.rgb15DrawCol, col);
}

/** 背景色セット */

void drawColor_setBkgndColor(uint32_t col)
{
	APP_DRAW->col.bkgndcol = col & 0xffffff;

	RGBtoRGBFix15(&APP_DRAW->col.rgb15BkgndCol, col);
}

/** 描画色/背景色入れ替え*/

void drawColor_toggleDrawCol(DrawData *p)
{
	uint32_t tmpc;
	RGBFix15 tmprgb;

	tmpc = p->col.drawcol;
	p->col.drawcol = p->col.bkgndcol;
	p->col.bkgndcol = tmpc;

	tmprgb = p->col.rgb15DrawCol;
	p->col.rgb15DrawCol = p->col.rgb15BkgndCol;
	p->col.rgb15BkgndCol = tmprgb;
}


//=============================
// 色マスク関連
//=============================


/** 色マスクの色変更時 */

void drawColorMask_changeColor(DrawData *p)
{
	int i;

	for(i = 0; i < p->col.colmask_num; i++)
		RGBtoRGBFix15(p->col.rgb15Colmask + i, p->col.colmask_col[i]);

	p->col.rgb15Colmask[i].r = 0xffff;
}

/** 色マスクのタイプを変更
 *
 * @param type 0..2:off,mask,rev -1:マスク反転  -2:逆マスク反転 */

void drawColorMask_setType(DrawData *p,int type)
{
	if(type >= 0)
		p->col.colmask_type = type;
	else
	{
		type = -type;
	
		if(p->col.colmask_type == type)
			p->col.colmask_type = 0;
		else
			p->col.colmask_type = type;
	}
}

/** 色マスクに色をセット (1色)
 *
 * @param col  -1 で描画色 */

void drawColorMask_setColor(DrawData *p,int col)
{
	p->col.colmask_num = 1;
	p->col.colmask_col[0] = (col == -1)? p->col.drawcol: col;
	p->col.colmask_col[1] = -1;

	drawColorMask_changeColor(p);
}

/** 色マスクに色を追加
 *
 * @param col  -1 で描画色 */

mBool drawColorMask_addColor(DrawData *p,int col)
{
	int i;

	if(p->col.colmask_num < COLORMASK_MAXNUM)
	{
		if(col == -1) col = p->col.drawcol;
	
		//同じ色が存在するか

		for(i = 0; i < p->col.colmask_num; i++)
		{
			if(p->col.colmask_col[i] == col)
				return FALSE;
		}
		
		//追加
	
		p->col.colmask_col[p->col.colmask_num] = col;
		p->col.colmask_col[p->col.colmask_num + 1] = -1;

		p->col.colmask_num++;

		drawColorMask_changeColor(p);
	}

	return TRUE;
}

/** 色マスクの色を削除
 *
 * @param no  負の値で最後の色 */

mBool drawColorMask_delColor(DrawData *p,int no)
{
	int i;
	int *pcol = p->col.colmask_col;

	//1色しかないならそのまま

	if(p->col.colmask_num == 1) return FALSE;

	if(no < 0) no = p->col.colmask_num - 1;

	//終端の -1 も含め詰める
	
	for(i = no; i < p->col.colmask_num; i++)
		pcol[i] = pcol[i + 1];

	p->col.colmask_num--;

	drawColorMask_changeColor(p);

	return TRUE;
}

/** 色マスクの色を最初の１色のみにする */

void drawColorMask_clear(DrawData *p)
{
	p->col.colmask_col[1] = -1;
	p->col.colmask_num = 1;

	drawColorMask_changeColor(p);
}



//=============================
// ツール
//=============================


/** ツール変更 */

void drawTool_setTool(DrawData *p,int no)
{
	//変更

	if(p->tool.no != no)
	{
		//矩形編集から変更する場合、枠を消す

		if(p->tool.no == TOOL_BOXEDIT)
			drawBoxEdit_setBox(p, NULL);

		//

		p->tool.no = no;

		//各ウィジェット

		DockTool_changeTool();
		DockOption_changeTool();

		StatusBar_setHelp_tool();

		//カーソル

		MainWinCanvasArea_setCursor_forTool();
	}
}

/** ツールサブ変更 */

void drawTool_setToolSubtype(DrawData *p,int subno)
{
	if(subno != p->tool.subno[p->tool.no])
	{
		p->tool.subno[p->tool.no] = subno;

		DockTool_changeToolSub();

		StatusBar_setHelp_tool();
	}
}


//=============================
// スタンプ
//=============================


/** スタンプイメージをクリア */

void drawStamp_clearImage(DrawData *p)
{
	if(p->stamp.img)
	{
		TileImage_free(p->stamp.img);
		p->stamp.img = NULL;

		//オプションのプレビュー更新

		DockOption_changeStampImage();

		//スタンプ画像がない時、カーソル変更

		MainWinCanvasArea_setCursor_forTool();
	}
}

/** 画像ファイルからスタンプイメージ読み込み */

void drawStamp_loadImage(DrawData *p,const char *filename,mBool ignore_alpha)
{
	TileImageLoadFileInfo info;

	TileImage_free(p->stamp.img);

	p->stamp.img = TileImage_loadFile(filename, 0, ignore_alpha,
		10000, &info, NULL, NULL);

	if(p->stamp.img)
	{
		p->stamp.size.w = info.width;
		p->stamp.size.h = info.height;

		//カーソル変更
		
		MainWinCanvasArea_setCursor_forTool();
	}
}
