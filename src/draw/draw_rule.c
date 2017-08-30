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
 * 定規関連
 *****************************************/
/*
 * - 角度などのパラメータは、すべてイメージに対する値
 */

#include <math.h>

#include "mDef.h"

#include "defDraw.h"

#include "draw_calc.h"
#include "draw_rule.h"
#include "draw_op_def.h"
#include "draw_op_sub.h"
#include "draw_op_func.h"



//==================================
// sub
//==================================


/** 角度 rd の cos,sin 値を dtmp[0,1] にセット */

static void _set_tmp_sincos(DrawData *p,double rd)
{
	p->rule.dtmp[0] = cos(rd);
	p->rule.dtmp[1] = sin(rd);
}

/** mDoublePoint の位置を小数点切り捨て +0.5 にする */

static void _set_doublepoint_dotmiddle(mDoublePoint *pt)
{
	pt->x = floor(pt->x) + 0.5;
	pt->y = floor(pt->y) + 0.5;
}

/** w.pttmp[0] の領域位置から、中心イメージ位置取得 */

static void _get_imagepos_middle(DrawData *p,mDoublePoint *dst)
{
	drawCalc_areaToimage_double(p,
		&dst->x, &dst->y, p->w.pttmp[0].x, p->w.pttmp[0].y);

	_set_doublepoint_dotmiddle(dst);
}



//==================================
// 補正位置取得関数
//==================================


/** 平行線/グリッド/集中線 */

static void _getpoint_line(DrawData *p,double *px,double *py)
{
	DrawRuleData *rule = &p->rule;
	double xx,yy,vi,len,dcos,dsin;

	dcos = rule->dtmp[0];
	dsin = rule->dtmp[1];

	xx = *px - rule->press_pos.x;
	yy = *py - rule->press_pos.y;

	len = sqrt(xx * xx + yy * yy);

	//ベクトルの内積 = cos の値
	/* (v1x * v2x + v1y * v2y) / (v1len * v2len)
	 * v2 は長さ 1 の指定角度の線とするので、v2x = cos, v2y = sin */

	if(len == 0)
		vi = 0;
	else
		vi = (xx * dcos + yy * dsin) / len;

	//グリッド時は、ある程度長さが出るまで角度を確定しない

	if(rule->type == RULE_TYPE_GRID_LINE && !rule->ntmp)
	{
		if(len >= 3)
		{
			if(vi > -0.7 && vi < 0.7)
				_set_tmp_sincos(p, rule->grid_rd + M_MATH_PI / 2);
			
			rule->ntmp = 1;
		}

		*px = rule->press_pos.x;
		*py = rule->press_pos.y;

		return;
	}

	//内積が負なら 180 度反転

	if(vi < 0) dcos = -dcos, dsin = -dsin;

	*px = len * dcos + rule->press_pos.x;
	*py = len * dsin + rule->press_pos.y;
}

/** 正円 */

static void _getpoint_circle(DrawData *p,double *px,double *py)
{
	double rd;

	rd = atan2(*py - p->rule.circle_pos.y, *px - p->rule.circle_pos.x);

	*px = p->rule.dtmp[0] * cos(rd) + p->rule.circle_pos.x;
	*py = p->rule.dtmp[0] * sin(rd) + p->rule.circle_pos.y;
}

/** 楕円 */

static void _getpoint_ellipse(DrawData *p,double *px,double *py)
{
	DrawRuleData *rule = &p->rule;
	double xx,yy,x,y,rd;

	//楕円を正円の状態に戻して、カーソル位置の角度取得

	xx = *px - rule->ellipse_pos.x;
	yy = *py - rule->ellipse_pos.y;

	x = (xx * rule->dtmp[0] - yy * rule->dtmp[1]) * rule->ellipse_yx;
	y = xx * rule->dtmp[1] + yy * rule->dtmp[0];

	rd = atan2(y, x);

	//求めた角度の位置を取得

	xx = rule->dtmp[2] / rule->ellipse_yx * cos(rd);
	yy = rule->dtmp[2] * sin(rd);

	x =  xx * rule->dtmp[0] + yy * rule->dtmp[1];
	y = -xx * rule->dtmp[1] + yy * rule->dtmp[0];

	*px = x + rule->ellipse_pos.x;
	*py = y + rule->ellipse_pos.y;
}



//==================================
// main
//==================================


/** 定規タイプ変更 */

void drawRule_setType(DrawData *p,int type)
{
	void (*func_getpoint[])(DrawData *,double *,double *) = {
		NULL, _getpoint_line, _getpoint_line, _getpoint_line,
		_getpoint_circle, _getpoint_ellipse
	};

	p->rule.type = type;

	p->rule.funcGetPoint = func_getpoint[type];
}

/** 自由線、ボタン押し時 */

void drawRule_onPress(DrawData *p,mBool dotpen)
{
	DrawPoint dpt;
	double x,y,xx,yy;

	//押し時のイメージ位置

	drawOpSub_getDrawPoint(p, &dpt);

	if(dotpen)
	{
		dpt.x = floor(dpt.x) + 0.5;
		dpt.y = floor(dpt.y) + 0.5;
	}

	p->rule.press_pos.x = dpt.x;
	p->rule.press_pos.y = dpt.y;

	//各タイプ

	switch(p->rule.type)
	{
		//平行線
		case RULE_TYPE_PARALLEL_LINE:
			_set_tmp_sincos(p, p->rule.parallel_rd);
			break;
		//グリッド
		case RULE_TYPE_GRID_LINE:
			p->rule.ntmp = 0;
			_set_tmp_sincos(p, p->rule.grid_rd);
			break;
		//集中線
		case RULE_TYPE_CONC_LINE:
			_set_tmp_sincos(p,
				atan2(dpt.y - p->rule.conc_pos.y, dpt.x - p->rule.conc_pos.x));
			break;
		//正円 (dtmp[0] = 半径)
		case RULE_TYPE_CIRCLE:
			x = dpt.x - p->rule.circle_pos.x;
			y = dpt.y - p->rule.circle_pos.y;

			p->rule.dtmp[0] = sqrt(x * x + y * y);
			break;
		//楕円 (dtmp[2] = 半径)
		case RULE_TYPE_ELLIPSE:
			_set_tmp_sincos(p, p->rule.ellipse_rd);
		
			x = dpt.x - p->rule.ellipse_pos.x;
			y = dpt.y - p->rule.ellipse_pos.y;

			xx = (x * p->rule.dtmp[0] - y * p->rule.dtmp[1]) * p->rule.ellipse_yx;
			yy = x * p->rule.dtmp[1] + y * p->rule.dtmp[0];

			p->rule.dtmp[2] = sqrt(xx * xx + yy * yy);
			break;
	}
}


//==========================
// 定規設定
//==========================


/** +Ctrl での定規の設定開始時
 *
 * @return グラブするか */

mBool drawRule_onPress_setting(DrawData *p)
{
	switch(p->rule.type)
	{
		//集中線
		case RULE_TYPE_CONC_LINE:
			drawOpSub_getImagePoint_double(p, &p->rule.conc_pos);
			_set_doublepoint_dotmiddle(&p->rule.conc_pos);

			return drawOpXor_rulepoint_press(p);
			
		//同心円(正円)
		case RULE_TYPE_CIRCLE:
			drawOpSub_getImagePoint_double(p, &p->rule.circle_pos);
			_set_doublepoint_dotmiddle(&p->rule.circle_pos);

			return drawOpXor_rulepoint_press(p);

		//同心円(楕円)
		case RULE_TYPE_ELLIPSE:
			return drawOpXor_ellipse_press(p, DRAW_OPSUB_RULE_SETTING);

		//平行線、グリッド
		default:
			return drawOpXor_line_press(p, DRAW_OPSUB_RULE_SETTING);
	}
}

/** 定規設定 : 平行線、グリド */

void drawRule_setting_line(DrawData *p)
{
	double rd;

	rd = drawCalc_getLineRadian_forImage(p);

	switch(p->rule.type)
	{
		//平行線
		case RULE_TYPE_PARALLEL_LINE:
			p->rule.parallel_rd = rd;
			break;
		//グリッド
		case RULE_TYPE_GRID_LINE:
			p->rule.grid_rd = rd;
			break;
	}
}

/** 定規設定 : 楕円 */

void drawRule_setting_ellipse(DrawData *p)
{
	DrawRuleData *rule = &p->rule;
	double rx,ry;

	//位置

	_get_imagepos_middle(p, &rule->ellipse_pos);

	//半径

	rx = p->w.pttmp[1].x * p->viewparam.scalediv;
	ry = p->w.pttmp[1].y * p->viewparam.scalediv;

	if(rx == 0) rx = 0.1;
	if(ry == 0) ry = 0.1;

	//パラメータ

	rule->ellipse_yx = ry / rx;
	rule->ellipse_rd = p->viewparam.rd;

	//左右反転表示の場合、x を反転した角度

	if(p->canvas_mirror)
	{
		rx = cos(p->viewparam.rd);
		ry = sin(p->viewparam.rd);

		rule->ellipse_rd = atan2(ry, -rx);
	}
}


//==========================
// 記録
//==========================


/** 現在のデータを記録する */

void drawRule_setRecord(DrawData *p,int no)
{
	RuleRecord *pd;
	DrawRuleData *rule = &p->rule;

	pd = p->rule.record + no;

	pd->type = rule->type;

	switch(rule->type)
	{
		case RULE_TYPE_OFF:
			break;
		case RULE_TYPE_PARALLEL_LINE:
			pd->d[0] = rule->parallel_rd;
			break;
		case RULE_TYPE_GRID_LINE:
			pd->d[0] = rule->grid_rd;
			break;
		case RULE_TYPE_CONC_LINE:
			pd->d[0] = rule->conc_pos.x;
			pd->d[1] = rule->conc_pos.y;
			break;
		case RULE_TYPE_CIRCLE:
			pd->d[0] = rule->circle_pos.x;
			pd->d[1] = rule->circle_pos.y;
			break;
		case RULE_TYPE_ELLIPSE:
			pd->d[0] = rule->ellipse_pos.x;
			pd->d[1] = rule->ellipse_pos.y;
			pd->d[2] = rule->ellipse_yx;
			pd->d[3] = rule->ellipse_rd;
			break;
	}
}

/** 記録データを呼び出し */

void drawRule_callRecord(DrawData *p,int no)
{
	RuleRecord *ps;
	DrawRuleData *rule = &p->rule;

	ps = p->rule.record + no;

	switch(ps->type)
	{
		case RULE_TYPE_OFF:
			break;
		case RULE_TYPE_PARALLEL_LINE:
			rule->parallel_rd = ps->d[0];
			break;
		case RULE_TYPE_GRID_LINE:
			rule->grid_rd = ps->d[0];
			break;
		case RULE_TYPE_CONC_LINE:
			rule->conc_pos.x = ps->d[0];
			rule->conc_pos.y = ps->d[1];
			break;
		case RULE_TYPE_CIRCLE:
			rule->circle_pos.x = ps->d[0];
			rule->circle_pos.y = ps->d[1];
			break;
		case RULE_TYPE_ELLIPSE:
			rule->ellipse_pos.x = ps->d[0];
			rule->ellipse_pos.y = ps->d[1];
			rule->ellipse_yx = ps->d[2];
			rule->ellipse_rd = ps->d[3];
			break;
	}

	drawRule_setType(p, ps->type);
}
