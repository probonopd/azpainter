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
 * ブラシアイテム
 *****************************************/

#include "mDef.h"
#include "mList.h"
#include "mStr.h"
#include "mTextParam.h"

#include "BrushItem.h"
#include "BrushList.h"
#include "defPixelMode.h"


/** 破棄ハンドラ */

void BrushItem_destroy_handle(mListItem *item)
{
	BrushItem *p = BRUSHITEM(item);

	mFree(p->name);
	mFree(p->shape_path);
	mFree(p->texture_path);
}


/** デフォルト値セット */

void BrushItem_setDefault(BrushItem *p)
{
	mStrdup_ptr(&p->name, "new");
	mStrdup_ptr(&p->shape_path, NULL);
	mStrdup_ptr(&p->texture_path, "?");

	p->pressure_val = 100;

	p->radius = 40;
	p->radius_min = BRUSHITEM_RADIUS_MIN;
	p->radius_max = 4000;
	p->min_size = 0;
	p->min_opacity = 1000;
	p->interval = 20;
	p->rand_size_min = 1000;
	p->rand_pos_range = 0;
	p->rotate_angle = 0;
	p->water_param[0] = 500;
	p->water_param[1] = 900;
	p->water_param[2] = 400;

	p->type = BRUSHITEM_TYPE_NORMAL;
	p->opacity = 100;
	p->pixelmode = 0;
	p->hosei_type = 1;
	p->hosei_strong = 1;
	p->hardness = 100;
	p->roughness = 0;
	p->rotate_rand_w = 0;
	p->pressure_type = 0;
	p->flags = BRUSHITEM_F_ANTIALIAS;
}

/** 半径値を調整 */

int BrushItem_adjustRadius(BrushItem *p,int n)
{
	if(p->type == BRUSHITEM_TYPE_FINGER)
	{
		//指先
		
		if(n < BRUSHITEM_FINGER_RADIUS_MIN)
			n = BRUSHITEM_FINGER_RADIUS_MIN;
		else if(n > BRUSHITEM_FINGER_RADIUS_MAX)
			n = BRUSHITEM_FINGER_RADIUS_MAX;
	}
	else
	{
		//ほか
		
		if(n < p->radius_min)
			n = p->radius_min;
		else if(n > p->radius_max)
			n = p->radius_max;
	}

	return n;
}

/** 半径最小/最大値を調整 */

void BrushItem_adjustRadiusMinMax(BrushItem *p)
{
	//指先タイプ以外

	if(p->type != BRUSHITEM_TYPE_FINGER
		&& p->radius_min >= p->radius_max)
	{
		p->radius_min = BRUSHITEM_FINGER_RADIUS_MIN;

		if(p->radius_min == p->radius_max)
			p->radius_max = BRUSHITEM_RADIUS_MAX;
	}
}

/** 現在のブラシタイプの塗りタイプの数を取得 */

int BrushItem_getPixelModeNum(BrushItem *p)
{
	switch(p->type)
	{
		case BRUSHITEM_TYPE_ERASE:
			return PIXELMODE_ERASE_NUM;
		case BRUSHITEM_TYPE_DOTPEN:
			return PIXELMODE_DOTPEN_NUM;
		default:
			return PIXELMODE_BRUSH_NUM;
	}
}

/** グループ内での位置取得 */

int BrushItem_getPosInGroup(BrushItem *p)
{
	return mListGetItemIndex(&p->group->list, M_LISTITEM(p));
}

/** 値をコピー */
/* [!] 元の値を維持しなければならないメンバが増えた場合は追記すること */

void BrushItem_copy(BrushItem *dst,BrushItem *src)
{
	BrushItem tmp;

	//解放

	BrushItem_destroy_handle(M_LISTITEM(dst));

	//値コピー

	tmp = *src;

	tmp.i = dst->i;
	tmp.group = dst->group;

	*dst = tmp;

	//文字列コピー

	dst->name = dst->shape_path = dst->texture_path = NULL;

	mStrdup_ptr(&dst->name, src->name);
	mStrdup_ptr(&dst->shape_path, src->shape_path);
	mStrdup_ptr(&dst->texture_path, src->texture_path);
}


//========================
// テキストフォーマット
//========================


/** クリップボード用、テキストフォーマットの文字列取得 */

void BrushItem_getTextFormat(BrushItem *p,mStr *strdst)
{
	mStr str[3];

	mStrArrayInit(str, 3);

	//文字列はパーセントエンコーディングで

	mStrSetPercentEncoding(str, p->name);
	mStrSetPercentEncoding(str + 1, p->shape_path);
	mStrSetPercentEncoding(str + 2, p->texture_path);

	//セット

	mStrSetFormat(strdst,
		"azpainter-v2-brush;name=%t;type=%d;", str, p->type);

	if(p->type == BRUSHITEM_TYPE_FINGER)
		mStrAppendFormat(strdst, "radius=%d;", p->radius);
	else
	{
		mStrAppendFormat(strdst,
			"radius=%.1F;radius_min=%.1F;radius_max=%.1F;",
			p->radius, p->radius_min, p->radius_max);
	}

	mStrAppendFormat(strdst,
		"opacity=%d;pix=%d;sm_type=%d;sm_str=%d;"
		"min_size=%.1F;min_opacity=%.1F;interval=%.2F;"
		"rand_size=%.1F;rand_pos=%.2F;angle=%d;angle_rand=%d;"
		"rough=%d;hard=%d;flags=%d;"
		"water1=%.1F;water2=%.1F;water3=%.1F;"
		"press_type=%d;press_value=%d;"
		"shape=%t;texture=%t;",
		p->opacity, p->pixelmode, p->hosei_type, p->hosei_strong,
		p->min_size, p->min_opacity, p->interval,
		p->rand_size_min, p->rand_pos_range,
		p->rotate_angle, p->rotate_rand_w,
		p->roughness, p->hardness, p->flags,
		p->water_param[0], p->water_param[1], p->water_param[2],
		p->pressure_type, p->pressure_val,
		str + 1, str + 2);

	mStrArrayFree(str, 3);
}

/** 文字列値取得 */

static void _get_textparam(mTextParam *tp,const char *key,mStr *strtmp,char **dst)
{
	char *text;
	
	if(mTextParamGetText_raw(tp, key, &text))
	{
		mStrDecodePercentEncoding(strtmp, text);

		mStrdup_ptr(dst, mStrIsEmpty(strtmp)? NULL: strtmp->buf);
	}
}

/** 数値取得 */

static int _get_intparam(mTextParam *tp,const char *key,int dig,int min,int max,int def)
{
	int n;

	if(dig == 0)
	{
		//通常数値
		
		if(mTextParamGetInt_range(tp, key, &n, min, max))
			return n;
		else
			return def;
	}
	else
	{
		//浮動小数点数
	
		if(mTextParamGetDoubleInt_range(tp, key, &n, dig, min, max))
			return n;
		else
			return def;
	}
}

/** 筆圧補正値調整 */

static void _adjust_pressure(BrushItem *p)
{
	uint32_t val = p->pressure_val;
	int i,n[4];

	if(p->pressure_type == 0)
	{
		//曲線
		
		if(val < BRUSHITEM_PRESSURE_CURVE_MIN)
			val = BRUSHITEM_PRESSURE_CURVE_MIN;
		else if(val > BRUSHITEM_PRESSURE_CURVE_MAX)
			val = BRUSHITEM_PRESSURE_CURVE_MAX;
	}
	else
	{
		//線形

		for(i = 0; i < 4; i++, val >>= 8)
		{
			n[i] = (uint8_t)val;
			if(n[i] > 100) n[i] = 100;
		}

		if(p->pressure_type == 2)
		{
			if(n[2] < n[0]) n[2] = n[0];
			if(n[3] < n[1]) n[3] = n[1];
		}

		val = (n[3] << 24) | (n[2] << 16) | (n[1] << 8) | n[0];
	}

	p->pressure_val = val;
}

/** テキストフォーマットから値をセット
 *
 * [!] ヘッダは確認済み。 */

void BrushItem_setFromText(BrushItem *p,const char *text)
{
	mTextParam *tp;
	mStr str = MSTR_INIT;

	tp = mTextParamCreate(text + 19, ';', '=');
	if(!tp) return;

	_get_textparam(tp, "name", &str, &p->name);
	_get_textparam(tp, "shape", &str, &p->shape_path);
	_get_textparam(tp, "texture", &str, &p->texture_path);

	//パラメータがない時は現在値を維持

	p->type = _get_intparam(tp, "type", 0, 0, BRUSHITEM_TYPE_NUM - 1, p->type);
	p->opacity = _get_intparam(tp, "opacity", 0, 1, 100, p->opacity);
	p->pixelmode = _get_intparam(tp, "pix", 0, 0, PIXELMODE_BRUSH_NUM - 1, p->pixelmode);
	p->hosei_type = _get_intparam(tp, "sm_type", 0, 0, 3, p->hosei_type);
	p->hosei_strong = _get_intparam(tp, "sm_str", 0, 1, BRUSHITEM_HOSEI_STRONG_MAX, p->hosei_strong);
	p->min_size = _get_intparam(tp, "min_size", 1, 0, 1000, p->min_size);
	p->min_opacity = _get_intparam(tp, "min_opacity", 1, 0, 1000, p->min_opacity);
	p->interval = _get_intparam(tp, "interval", 2, BRUSHITEM_INTERVAL_MIN, BRUSHITEM_INTERVAL_MAX, p->interval);
	p->rand_size_min = _get_intparam(tp, "rand_size", 1, 0, 1000, p->rand_size_min);
	p->rand_pos_range = _get_intparam(tp, "rand_pos", 2, 0, BRUSHITEM_RANDOM_POS_MAX, p->rand_pos_range);
	p->rotate_angle = _get_intparam(tp, "angle", 0, 0, 359, p->rotate_angle);
	p->rotate_rand_w = _get_intparam(tp, "angle_rand", 0, 0, 180, p->rotate_rand_w);
	p->roughness = _get_intparam(tp, "rough", 0, 0, 100, p->roughness);
	p->hardness = _get_intparam(tp, "hard", 0, 0, 100, p->hardness);
	p->flags = _get_intparam(tp, "flags", 0, 0, 0xff, p->flags);

	p->water_param[0] = _get_intparam(tp, "water1", 1, 0, 1000, p->water_param[0]);
	p->water_param[1] = _get_intparam(tp, "water2", 1, 0, 1000, p->water_param[1]);
	p->water_param[2] = _get_intparam(tp, "water3", 1, 0, 1000, p->water_param[2]);

	p->pressure_type = _get_intparam(tp, "press_type", 0, 0, 2, p->pressure_type);
	p->pressure_val = _get_intparam(tp, "press_value", 0, 0, INT32_MAX, p->pressure_val);

	if(p->type == BRUSHITEM_TYPE_FINGER)
		p->radius = _get_intparam(tp, "radius", 0, BRUSHITEM_FINGER_RADIUS_MIN, BRUSHITEM_FINGER_RADIUS_MAX, p->radius);
	else
	{
		p->radius = _get_intparam(tp, "radius", 1, BRUSHITEM_RADIUS_MIN, BRUSHITEM_RADIUS_MAX, p->radius);
		p->radius_min = _get_intparam(tp, "radius_min", 1, BRUSHITEM_RADIUS_MIN, BRUSHITEM_RADIUS_MAX, p->radius_min);
		p->radius_max = _get_intparam(tp, "radius_max", 1, BRUSHITEM_RADIUS_MIN, BRUSHITEM_RADIUS_MAX, p->radius_max);
	}

	mTextParamFree(tp);

	mStrFree(&str);

	//半径を調整

	BrushItem_adjustRadiusMinMax(p);

	p->radius = BrushItem_adjustRadius(p, p->radius);

	//塗りタイプ

	if(p->pixelmode >= BrushItem_getPixelModeNum(p))
		p->pixelmode = 0;

	//筆圧

	_adjust_pressure(p);
}
