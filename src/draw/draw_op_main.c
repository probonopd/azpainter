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
 * 操作 - メイン
 *****************************************/

#include "mDef.h"
#include "mEvent.h"
#include "mKeyDef.h"
#include "mRectBox.h"

#include "defDraw.h"
#include "defConfig.h"
#include "defCanvasKeyID.h"

#include "draw_main.h"
#include "draw_calc.h"
#include "draw_layer.h"
#include "draw_op_def.h"
#include "draw_op_func.h"
#include "draw_op_sub.h"
#include "draw_rule.h"

#include "BrushItem.h"
#include "BrushList.h"
#include "AppCursor.h"

#include "MainWindow.h"
#include "MainWinCanvas.h"
#include "Docks_external.h"


//---------------------

#define _ONPRESS_BRUSH_F_REGISTERED   1	//登録ブラシ
#define _ONPRESS_BRUSH_F_PRESSURE_MAX 2 //筆圧最大

//---------------------



//============================
// sub
//============================


/** ボタン押し時 : ブラシ、自由線描画 */

static mBool _onPress_brush_free(DrawData *p,uint32_t flags)
{
	BrushItem *item;
	mBool registered;

	//登録ブラシか

	registered = ((flags & _ONPRESS_BRUSH_F_REGISTERED) != 0);

	//-------- 装飾キー

	if(p->w.press_state & M_MODS_ALT)
	{
		//+Alt : スポイト

		return drawOp_spoit_press(p, FALSE);
	}
	else if(p->rule.type && (p->w.press_state & M_MODS_CTRL))
	{
		//+Ctrl : (定規ON時) 定規設定

		return drawRule_onPress_setting(p);
	}
	else if(p->w.press_state == M_MODS_SHIFT)
	{
		//+Shift でブラシサイズ変更 (ドットペン時は除く)

		return drawOp_dragBrushSize_press(p, registered);
	}

	//------------

	//描画不可

	if(drawOpSub_canDrawLayer_mes(p)) return FALSE;

	//ブラシアイテム

	BrushList_getDrawInfo(&item, NULL, registered);

	//定規セット

	drawRule_onPress(p, (item->type == BRUSHITEM_TYPE_DOTPEN));

	//開始

	switch(item->type)
	{
		//ドットペン
		case BRUSHITEM_TYPE_DOTPEN:
			return drawOp_dotpen_free_press(p, registered);
		//指先
		case BRUSHITEM_TYPE_FINGER:
			return drawOp_finger_free_press(p, registered);
		//ブラシ
		default:
			return drawOp_brush_free_press(p, registered, ((flags & _ONPRESS_BRUSH_F_PRESSURE_MAX) != 0));
	}
}

/** ボタン押し時 : ブラシ描画 */

static mBool _onPress_brush(DrawData *p,int subno,uint32_t flags)
{
	//自由線以外時、描画可能かチェック

	if(subno != TOOLSUB_DRAW_FREE && drawOpSub_canDrawLayer_mes(p))
		return FALSE;

	//

	switch(subno)
	{
		//自由線
		case TOOLSUB_DRAW_FREE:
			return _onPress_brush_free(p, flags);

		//直線
		case TOOLSUB_DRAW_LINE:
			return drawOpXor_line_press(p, DRAW_OPSUB_DRAW_LINE);

		//四角形
		case TOOLSUB_DRAW_BOX:
			return drawOpXor_boxarea_press(p, DRAW_OPSUB_DRAW_FRAME);

		//楕円
		case TOOLSUB_DRAW_ELLIPSE:
			return drawOpXor_ellipse_press(p, DRAW_OPSUB_DRAW_FRAME);

		//連続直線
		case TOOLSUB_DRAW_SUCCLINE:
			return drawOpXor_sumline_press(p, DRAW_OPSUB_DRAW_SUCCLINE);

		//集中線
		case TOOLSUB_DRAW_CONCLINE:
			return drawOpXor_sumline_press(p, DRAW_OPSUB_DRAW_CONCLINE);

		//ベジェ曲線
		case TOOLSUB_DRAW_BEZIER:
			return drawOpXor_line_press(p, DRAW_OPSUB_TO_BEZIER);

		//スプライン曲線
		case TOOLSUB_DRAW_SPLINE:
			return drawOp_spline_press(p);
	}

	return FALSE;
}

/** ボタン押し時 : 図形塗り/消し */

static mBool _onPress_fillpoly(DrawData *p,int subno)
{
	if(drawOpSub_canDrawLayer_mes(p)) return FALSE;

	switch(subno)
	{
		//四角形
		case 0:
			return drawOpXor_boxarea_press(p, DRAW_OPSUB_DRAW_FILL);
		//楕円
		case 1:
			return drawOpXor_ellipse_press(p, DRAW_OPSUB_DRAW_FILL);
		//多角形
		case 2:
			return drawOpXor_polygon_press(p, DRAW_OPSUB_DRAW_FILL);
		//フリーハンド
		default:
			return drawOpXor_lasso_press(p, DRAW_OPSUB_DRAW_FILL);
	}

	return FALSE;
}

/** ボタン押し時 : 選択範囲 */

static mBool _onPress_select(DrawData *p,int subno)
{
	switch(subno)
	{
		//四角
		case TOOLSUB_SEL_BOX:
			return drawOpXor_boxarea_press(p, DRAW_OPSUB_SET_SELECT);
		//多角形
		case TOOLSUB_SEL_POLYGON:
			return drawOpXor_polygon_press(p, DRAW_OPSUB_SET_SELECT);
		//フリーハンド
		case TOOLSUB_SEL_LASSO:
			return drawOpXor_lasso_press(p, DRAW_OPSUB_SET_SELECT);
		//イメージ移動/コピー
		case TOOLSUB_SEL_IMGMOVE:
		case TOOLSUB_SEL_IMGCOPY:
			return drawOp_selimgmove_press(p, (subno == 3));
		//範囲移動
		case TOOLSUB_SEL_POSMOVE:
			return drawOp_selmove_press(p);
	}

	return FALSE;
}

/** ボタン押し時 : ツール
 *
 * @param subno 負の値で現在のサブタイプ */

static mBool _onPress_tool(DrawData *p,int toolno,int subno)
{
	if(subno < 0)
		subno = p->tool.subno[toolno];

	p->w.optoolno = toolno;

	mRectEmpty(&p->w.rcdraw);

	switch(toolno)
	{
		//ブラシ
		case TOOL_BRUSH:
			return _onPress_brush(p, subno, 0);

		//図形塗り/消し
		case TOOL_FILL_POLYGON:
		case TOOL_FILL_POLYGON_ERASE:
			return _onPress_fillpoly(p, subno);

		//塗りつぶし
		case TOOL_FILL:
		case TOOL_FILL_ERASE:
			return drawOp_fill_press(p);

		//グラデーション
		case TOOL_GRADATION:
			if(drawOpSub_canDrawLayer_mes(p))
				return FALSE;
			else
				return drawOpXor_line_press(p, DRAW_OPSUB_DRAW_GRADATION);

		//移動
		case TOOL_MOVE:
			return drawOp_movetool_press(p);

		//テキスト
		case TOOL_TEXT:
			return drawOp_drawtext_press(p);

		//自動選択
		case TOOL_MAGICWAND:
			return drawOp_magicwand_press(p);

		//選択範囲
		case TOOL_SELECT:
			return _onPress_select(p, subno);

		//矩形編集
		case TOOL_BOXEDIT:
			return drawOp_boxedit_press(p);

		//スタンプ
		case TOOL_STAMP:
			return drawOp_stamp_press(p, subno);

		//キャンバス回転
		case TOOL_CANVAS_ROTATE:
			return drawOp_canvasRotate_press(p);
		//キャンバス移動
		case TOOL_CANVAS_MOVE:
			return drawOp_canvasMove_press(p);
		//スポイト
		case TOOL_SPOIT:
			return drawOp_spoit_press(p, TRUE);
	}

	return FALSE;
}

/** キャンバスキーのキー＋操作のコマンド取得
 *
 * @return -1 でなし */

static int _get_canvaskey_cmd()
{
	int key,cmd;

	//キャンバス上で現在押されているキー

	key = MainWinCanvasArea_getPressRawKey();
	if(key == -1) return -1;

	//現在のキーにコマンドが設定されているか

	cmd = APP_CONF->canvaskey[key];
	if(cmd == 0) return -1;

	//<キー＋操作> のコマンドかどうか
	/* ここで弾いておかないと、キー押し時に実行されるタイプのコマンドのキーが
	 * 押されている間に左ボタンを押すと、キー押し時とボタン押し時に
	 * ２回コマンドが実行されてしまう。 */

	if(CANVASKEY_IS_PLUS_MOTION(cmd))
		return cmd;
	else
		return -1;
}

/** 左ボタン以外、またはキャンバスキーの動作を判定 & 実行
 *
 * @return グラブするか。-1 で左ボタン動作 */

static int _onPress_command(DrawData *p,int btt,int pentab_eraser,mBool type_pentab)
{
	int cmd,ret,cursor = -1;
	mBool can_draw;

	//====== コマンドID 取得 (CANVASKEYID_*)

	if(btt >= 2)
	{
		//[左ボタン以外のボタン]
		/* 指定なしなら、何もしない。 */
		
		if(btt > CONFIG_POINTERBTT_MAXNO) return 0;

		if(type_pentab)
			cmd = APP_CONF->pentab_btt_cmd[btt];
		else
			cmd = APP_CONF->default_btt_cmd[btt];

		if(!cmd) return 0;
	}
	else if(pentab_eraser && APP_CONF->pentab_btt_cmd[0])
	{
		//[左ボタン:ペンタブの消しゴム側]
		/* 指定がなければ普通の左ボタンとして扱う */
	
		cmd = APP_CONF->pentab_btt_cmd[0];
	}
	else
	{
		//[左ボタン]

		//キャンバスキーのキー＋操作コマンドを優先

		cmd = _get_canvaskey_cmd();
		
		if(cmd == -1)
		{
			//キャンバスキー操作がなかった場合、左ボタンに動作指定があるか
			/* 指定がなければ通常左ボタン動作へ */

			if(type_pentab)
				cmd = APP_CONF->pentab_btt_cmd[1];
			else
				cmd = APP_CONF->default_btt_cmd[1];

			if(!cmd) return -1;
		}
	}

	//======= 各コマンド処理

	//戻り値はデフォルトで何もしない

	ret = 0;

	//描画が可能な状態か (矩形編集ツール中は描画不可とする)

	can_draw = (p->tool.no != TOOL_BOXEDIT);

	//

	if(cmd >= CANVASKEYID_OP_TOOL && cmd < CANVASKEYID_OP_TOOL + TOOL_NUM)
	{
		//ツール動作 (矩形編集ツール中は、キャンバス操作のみ使用可)

		cmd -= CANVASKEYID_OP_TOOL;

		if(!(p->tool.no == TOOL_BOXEDIT && cmd != TOOL_CANVAS_MOVE && cmd != TOOL_CANVAS_ROTATE))
		{
			ret = _onPress_tool(p, cmd, -1);

			cursor = drawCursor_getToolCursor(cmd);
		}
	}
	else if(cmd >= CANVASKEYID_OP_BRUSHDRAW && cmd < CANVASKEYID_OP_BRUSHDRAW + TOOLSUB_DRAW_NUM)
	{
		//ブラシ描画 (描画タイプ指定)

		if(can_draw)
		{
			ret = _onPress_tool(p, TOOL_BRUSH, cmd - CANVASKEYID_OP_BRUSHDRAW);

			cursor = APP_CURSOR_DRAW;
		}
	}
	else if(cmd >= CANVASKEYID_OP_SELECT && cmd < CANVASKEYID_OP_SELECT + TOOLSUB_SEL_NUM)
	{
		//選択範囲ツール動作

		cmd -= CANVASKEYID_OP_SELECT;

		ret = _onPress_select(p, cmd);

		if(cmd == TOOLSUB_SEL_IMGMOVE || cmd == TOOLSUB_SEL_IMGCOPY || cmd == TOOLSUB_SEL_POSMOVE)
			cursor = APP_CURSOR_SEL_MOVE;
		else
			cursor = APP_CURSOR_SELECT;
	}
	else if(cmd >= CANVASKEYID_OP_OTHER && cmd < CANVASKEYID_OP_OTHER + CANVASKEY_OP_OTHER_NUM)
	{
		//他動作

		cmd -= CANVASKEYID_OP_OTHER;

		cursor = APP_CURSOR_DRAW;

		switch(cmd)
		{
			//登録ブラシで自由線描画
			case 0:
				if(can_draw)
					ret = _onPress_brush_free(p, _ONPRESS_BRUSH_F_REGISTERED);
				break;
			//登録ブラシで自由線描画 (筆圧最大)
			case 1:
				if(can_draw)
					ret = _onPress_brush_free(p, _ONPRESS_BRUSH_F_REGISTERED | _ONPRESS_BRUSH_F_PRESSURE_MAX);
				break;
			//表示倍率変更 (上下ドラッグ)
			case 2:
				ret = drawOp_canvasZoom_press(p);
				break;
			//ブラシサイズ変更
			case 3:
				ret = drawOp_dragBrushSize_press(p, FALSE);
				break;
			//登録ブラシのブラシサイズ変更
			case 4:
				ret = drawOp_dragBrushSize_press(p, TRUE);
				break;
			//中間色作成
			case 5:
				ret = drawOp_intermediateColor_press(p);
				break;
			//色置き換え
			case 6:
			case 7:
				ret = drawOp_replaceColor_press(p, (cmd == 7));
				break;
			//掴んだレイヤを選択
			case 8:
				drawLayer_selectPixelTopLayer(p);
				break;
		}
	}
	else if(cmd >= CANVASKEYID_CMD_OTHER && cmd < CANVASKEYID_CMD_OTHER + CANVASKEY_CMD_OTHER_NUM)
	{
		//他コマンド (左ボタン以外のボタン操作時)

		MainWindow_onCanvasKeyCommand(cmd);
	}

	//グラブする場合、カーソル指定がある時はセット

	if(ret > 0 && cursor != -1)
		p->w.drag_cursor_type = cursor;

	return ret;
}


//============================
// メイン処理
//============================


/** ボタン押し時
 *
 * @return グラブするか */

mBool drawOp_onPress(DrawData *p,mEvent *ev)
{
	int ret;

	if(p->w.optype != DRAW_OPTYPE_NONE)
	{
		//======= 操作中

		if(ev->pen.btt == M_BTT_LEFT && p->w.funcPressInGrab)
		{
			//左ボタン押し
			
			p->w.dptAreaCur.x = ev->pen.x;
			p->w.dptAreaCur.y = ev->pen.y;

			(p->w.funcPressInGrab)(p, ev->pen.state);
		}

		return FALSE;
	}
	else
	{
		//======= 操作開始

		p->w.dptAreaPress.x = ev->pen.x;
		p->w.dptAreaPress.y = ev->pen.y;
		p->w.dptAreaPress.pressure = ev->pen.pressure;

		p->w.dptAreaCur = p->w.dptAreaLast = p->w.dptAreaPress;

		p->w.ptLastArea.x = ev->pen.x;
		p->w.ptLastArea.y = ev->pen.y;

		p->w.press_btt = ev->pen.btt;
		p->w.press_state = ev->pen.state & M_MODS_MASK_KEY;

		p->w.opflags = 0;
		p->w.release_only_btt = ev->pen.btt;
		p->w.drag_cursor_type = -1;

		//---- 左ボタン+SPACE 操作

		if(ev->pen.btt == M_BTT_LEFT
			&& MainWinCanvasArea_isPressKey_space())
		{
			if(p->w.press_state == M_MODS_CTRL)
			{
				//+Ctrl : キャンバス回転

				p->w.opflags |= DRAW_OPFLAGS_MOTION_DISABLE_STATE;
				return drawOp_canvasRotate_press(p);
			}
			else if(p->w.press_state == M_MODS_SHIFT)
				//+Shift : キャンバス表示倍率
				return drawOp_canvasZoom_press(p);
			else
				return drawOp_canvasMove_press(p);
		}

		//

		ret = _onPress_command(p, ev->pen.btt,
			(ev->pen.flags & MEVENT_PENTAB_FLAGS_ERASER),
			ev->pen.bPenTablet);

		if(ret == -1)
			//左ボタンの通常動作
			return _onPress_tool(p, p->tool.no, -1);
		else
			return ret;
	}

	return FALSE;
}

/** ボタン離し時
 *
 * @return グラブ解除するか */

mBool drawOp_onRelease(DrawData *p,mEvent *ev)
{
	mBool ungrab = TRUE;

	if(p->w.optype == DRAW_OPTYPE_NONE) return FALSE;

	//位置

	p->w.dptAreaCur.x = ev->pen.x;
	p->w.dptAreaCur.y = ev->pen.y;
	p->w.dptAreaCur.pressure = ev->pen.pressure;

	//右ボタン押し時の処理

	if(ev->pen.btt == M_BTT_RIGHT
		&& p->w.funcAction && (p->w.funcAction)(p, DRAW_FUNCACTION_RBTT_UP))
		goto END;

	//指定ボタンのみ処理する

	if(ev->pen.btt != p->w.release_only_btt)
		return FALSE;

	//処理

	if(p->w.funcRelease)
		ungrab = (p->w.funcRelease)(p);

	//操作終了

END:
	if(ungrab)
		p->w.optype = DRAW_OPTYPE_NONE;

	return ungrab;
}

/** カーソル移動時 */

void drawOp_onMotion(DrawData *p,mEvent *ev)
{
	mPoint pt;
	mBool flag;

	//イメージ位置 (int)

	drawCalc_areaToimage_pt(p, &pt, ev->pen.x, ev->pen.y);

	//キャンバスビューパレット、ルーペモード時

	DockCanvasView_changeLoupePos(&pt);

	//

	if(p->w.optype == DRAW_OPTYPE_NONE)
	{
		//------- 非操作中の処理

		if(p->tool.no == TOOL_BOXEDIT && p->boxedit.box.w)
		{
			//矩形編集ツールで、範囲がある時

			p->w.dptAreaCur.x = ev->pen.x;
			p->w.dptAreaCur.y = ev->pen.y;

			drawOp_boxedit_nograb_motion(p);
		}
	}
	else
	{
		//-------- 操作中の処理
	
		p->w.dptAreaCur.x = ev->pen.x;
		p->w.dptAreaCur.y = ev->pen.y;
		p->w.dptAreaCur.pressure = ev->pen.pressure;

		pt.x = ev->pen.x;
		pt.y = ev->pen.y;

		//位置を int で扱う場合、前回と同じ位置なら処理しない

		flag = ((p->w.opflags & DRAW_OPFLAGS_MOTION_POS_INT)
			&& pt.x == p->w.ptLastArea.x && pt.y == p->w.ptLastArea.y);

		//処理

		if(p->w.funcMotion && !flag)
		{
			(p->w.funcMotion)(p,
				(p->w.opflags & DRAW_OPFLAGS_MOTION_DISABLE_STATE)? 0: ev->pen.state);
		}

		p->w.dptAreaLast = p->w.dptAreaCur;
		p->w.ptLastArea = pt;
	}
}

/** 左ボタン、ダブルクリック時
 *
 * @return TRUE でダブルクリックとして処理し、グラブ解除。
 *         FALSE で通常の押しとして処理する。*/

mBool drawOp_onLBttDblClk(DrawData *p,mEvent *ev)
{
	mBool release = FALSE;

	//現在の操作処理

	if(p->w.optype && p->w.funcAction)
		release = (p->w.funcAction)(p, DRAW_FUNCACTION_LBTT_DBLCLK);

	//解除

	if(release)
		p->w.optype = DRAW_OPTYPE_NONE;

	return release;
}

/** 操作中のキー押し時
 *
 * @return TRUE でグラブ解除 */

mBool drawOp_onKeyDown(DrawData *p,uint32_t key)
{
	mBool release = FALSE;
	int action;

	//現在の操作処理

	if(p->w.funcAction)
	{
		switch(key)
		{
			case MKEY_ENTER: action = DRAW_FUNCACTION_KEY_ENTER; break;
			case MKEY_ESCAPE: action = DRAW_FUNCACTION_KEY_ESC; break;
			case MKEY_BACKSPACE: action = DRAW_FUNCACTION_KEY_BACKSPACE; break;
			default: action = -1; break;
		}

		if(action != -1)
			release = (p->w.funcAction)(p, action);
	}

	//解除

	if(release)
		p->w.optype = DRAW_OPTYPE_NONE;

	return release;
}

/** 想定外のグラブ解除時 */

void drawOp_onUngrab(DrawData *p)
{
	if(p->w.optype)
	{
		if(p->w.funcAction)
			(p->w.funcAction)(p, DRAW_FUNCACTION_UNGRAB);
		else if(p->w.funcRelease)
			(p->w.funcRelease)(p);

		p->w.optype = DRAW_OPTYPE_NONE;
	}
}
