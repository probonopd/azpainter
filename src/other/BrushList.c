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
 * ブラシリストデータ
 *****************************************/
/*
 * [BrushList::cur]
 * 
 *  現在の描画用ブラシパラメータのデータ。
 *  ブラシ描画時はこのパラメータを使う。
 *  ブラシの選択が変更されたらここにデータがコピーされる。
 *  ウィジェット上で値が変更されたら、常にこの値も更新する。
 *
 * [BrushList::dp_cur, dp_reg]
 *
 *  描画時に使われるパラメータ値。
 *  ブラシ選択変更時や項目値の変更時は、常にこの値も更新する。
 */

#include <stdio.h>

#include "mDef.h"
#include "mList.h"
#include "mUtilStdio.h"
#include "mUtilStr.h"
#include "mStr.h"
#include "mGui.h"

#include "BrushList.h"
#include "BrushItem.h"
#include "BrushDrawParam.h"
#include "BrushSizeList.h"

#include "MaterialImgList.h"
#include "defPixelMode.h"
#include "defMacros.h"


//-------------------

typedef struct
{
	mList list;		//グループリスト

	BrushItem cur,		//編集用の現在データ
		*selitem,		//選択ブラシ
		*regitem,		//登録ブラシ
		*toggleitem;	//ブラシ切り替え用

	BrushDrawParam dp_cur,
		dp_reg,
		dp_filter;		//フィルタの漫画描画用

	int reg_radius;		//キャンバスキーの「登録ブラシのサイズを変更」時の一時的な現在半径値
						//0 でなし
}BrushList;

static BrushList *g_dat = NULL;

//-------------------

#define GROUP_MAXNUM		100
#define BRUSHITEM_MAXNUM 	500

//-------------------



//*********************************************
// BrushGroupItem : グループアイテム
//*********************************************


/** 破棄ハンドラ */

static void _groupitem_destroy(mListItem *li)
{
	BrushGroupItem *p = BRUSHGROUPITEM(li);

	//ブラシデータリスト
	mListDeleteAll(&p->list);

	//名前
	mFree(p->name);
}

/** 先頭ブラシアイテム取得 */

BrushItem *BrushGroup_getTopItem(BrushGroupItem *p)
{
	return (BrushItem *)(p->list.top);
}

/** グループ内のインデックス番号からアイテム取得 */

BrushItem *BrushGroup_getItemAtIndex(BrushGroupItem *p,int index)
{
	return (BrushItem *)mListGetItemByIndex(&p->list, index);
}

/** 表示時の縦アイテム数取得 */

int BrushGroup_getViewVertNum(BrushGroupItem *p)
{
	return (p->list.num + p->colnum - 1) / p->colnum;
}



//*******************************************
// BrushList : ブラシリスト
//*******************************************


//==========================
// sub
//==========================


/** 選択アイテムが連動保存タイプか */

static mBool _is_sel_linksave()
{
	return (g_dat->selitem && (g_dat->selitem->flags & BRUSHITEM_F_AUTO_SAVE));
}

/** BrushDrawParam:登録ブラシ用の画像を解放 */

static void _release_drawparam_reg_image()
{
	MaterialImgList_releaseImage(MATERIALIMGLIST_TYPE_BRUSH, g_dat->dp_reg.img_shape);
	MaterialImgList_releaseImage(MATERIALIMGLIST_TYPE_TEXTURE, g_dat->dp_reg.img_texture);

	g_dat->dp_reg.img_shape = NULL;
	g_dat->dp_reg.img_texture = NULL;
}

/** 選択アイテム変更時の更新 */

static void _update_selitem(mBool set_regsize)
{
	//編集用にデータコピー

	BrushItem_copy(&g_dat->cur, g_dat->selitem);

	/* 登録ブラシに変更された時、
	 * キャンバスキーでのブラシサイズ変更を編集用データに適用させる。
	 * [!] 保存せずに選択を変更すると元に戻る。 */

	if(g_dat->selitem == g_dat->regitem && g_dat->reg_radius)
	{
		if(set_regsize)
			g_dat->cur.radius = g_dat->reg_radius;

		g_dat->reg_radius = 0;
	}

	//描画用パラメータセット

	BrushList_setBrushDrawParam(FALSE);
}

/** 描画用の形状の硬さ取得 */

static double _get_drawparam_shape_hard(int val)
{
	if(val <= 50)
		return (-0.4 - 0.7) * (val * 0.02) + 0.7;
	else
		return (-5 + 0.4) * ((val - 50) * 0.02) - 0.4;
}

/** 描画用の形状荒さ取得 */

static int _get_drawparam_rough(int val)
{
	if(val == 0)
		return 0;
	else if(val <= 50)
		return val * 2;
	else if(val <= 80)
		return 100 + (val - 50) * 10;
	else if(val <= 90)
		return 300 + (val - 80) * 400;
	else
		return 5000 + (val - 90) * 1000;
}

/** 描画用の筆圧補正値セット */

static void _set_drawparam_pressure(BrushDrawParam *p,int type,uint32_t val)
{
	int i,pt[4];
	double d[4];

	switch(type)
	{
		//曲線
		case 0:
			if(val == 100)
				p->flags &= ~BRUSHDP_F_PRESSURE;
			else
			{
				p->flags |= BRUSHDP_F_PRESSURE;
				p->pressure_val[0] = val * 0.01;
			}
			break;

		//線形1点
		case 1:
			for(i = 0; i < 2; i++, val >>= 8)
			{
				pt[i] = (uint8_t)val;
				d[i] = pt[i] * 0.01;
			}

			if((pt[0] == 0 && pt[1] == 0) || (pt[0] == 100 && pt[1] == 100))
				p->flags &= ~BRUSHDP_F_PRESSURE;
			else
			{
				p->flags |= BRUSHDP_F_PRESSURE;
			
				p->pressure_val[0] = d[0];
				p->pressure_val[1] = d[1];
				p->pressure_val[2] = 1 - d[0];
				p->pressure_val[3] = 1 - d[1];
			}
			break;

		//線形2点
		default:
			for(i = 0; i < 4; i++, val >>= 8)
			{
				pt[i] = (uint8_t)val;
				d[i] = pt[i] * 0.01;

				p->pressure_val[i] = d[i];
			}

			p->pressure_val[4] = d[2] - d[0];	//in2 - in1
			p->pressure_val[5] = d[3] - d[1];	//out2 - out1
			p->pressure_val[6] = 1 - d[2];		//1 - in2
			p->pressure_val[7] = 1 - d[3];		//1 - out2

			p->flags |= BRUSHDP_F_PRESSURE;
			break;
	}
}


//==========================
//
//==========================


/** 解放 */

void BrushList_free()
{
	//編集用ブラシデータ
	BrushItem_destroy_handle(M_LISTITEM(&g_dat->cur));

	mListDeleteAll(&g_dat->list);
	
	mFree(g_dat);
}

/** 確保 */

mBool BrushList_new()
{
	//確保

	g_dat = (BrushList *)mMalloc(sizeof(BrushList), TRUE);
	if(!g_dat) return FALSE;

	//デフォルト値セット

	BrushItem_setDefault(&g_dat->cur);

	BrushList_setBrushDrawParam(FALSE);

	return TRUE;
}


//========================
// 取得
//========================


/** グループ数取得 */

int BrushList_getGroupNum()
{
	return g_dat->list.num;
}

/** 先頭グループ取得 */

BrushGroupItem *BrushList_getTopGroup()
{
	return BRUSHGROUPITEM(g_dat->list.top);
}

/** 編集用ブラシ取得 */

BrushItem *BrushList_getEditItem()
{
	return &g_dat->cur;
}

/** 選択ブラシ取得 */

BrushItem *BrushList_getSelItem()
{
	return g_dat->selitem;
}

/** 登録ブラシ取得 */

BrushItem *BrushList_getRegisteredItem()
{
	return g_dat->regitem;
}

/** 描画開始時、使用するブラシアイテムと描画パラメータ取得
 *
 * @param ppparam NULL で取得しない */

void BrushList_getDrawInfo(
	BrushItem **ppitem,BrushDrawParam **ppparam,mBool registered)
{
	BrushList *p = g_dat;

	if(registered && p->regitem && p->selitem != p->regitem)
	{
		//登録ブラシ (現在の選択 = 登録ブラシの場合は除く)

		*ppitem = p->regitem;
		if(ppparam) *ppparam = &p->dp_reg;
	}
	else
	{
		//編集用データ

		*ppitem = &p->cur;
		if(ppparam) *ppparam = &p->dp_cur;
	}
}

/** フィルタ描画用のパラメータ取得 (漫画用、線描画)
 *
 * 通常円形(硬さ最大)、濃度最大、間隔 0.4 */

BrushDrawParam *BrushList_getBrushDrawParam_forFilter()
{
	BrushDrawParam *p = &g_dat->dp_filter;

	mMemzero(p, sizeof(BrushDrawParam));

	p->opacity = 1;
	p->interval = 0.4;
	p->flags = BRUSHDP_F_SHAPE_HARD_MAX;

	return p;
}

/** キャンバスキーのブラシサイズ変更時の情報取得
 *
 * @return 現在の半径値 */

int BrushList_getChangeBrushSizeInfo(BrushItem **ppitem,mBool registered)
{
	BrushItem *item;

	BrushList_getDrawInfo(&item, NULL, registered);

	*ppitem = item;

	if(item == &g_dat->cur)
		//編集データ
		return item->radius;
	else
	{
		//登録ブラシ (カレントでない場合)
		/* 前回のブラシサイズ変更時の値があればそれを使う */

		if(g_dat->reg_radius)
			return g_dat->reg_radius;
		else
			return item->radius;
	}
}


//======================
// セット
//======================


/** 選択アイテムセット
 *
 * @param item  NULL で選択なしに
 * @return 選択が変更されたか */

mBool BrushList_setSelItem(BrushItem *item)
{
	BrushList *p = g_dat;

	if(item == p->selitem)
		return FALSE;
	else
	{
		/* 現在の選択が登録ブラシで、別のブラシに変わる場合、
		 * 登録ブラシの描画用パラメータを確定 */

		if(p->regitem && p->selitem == p->regitem)
			BrushList_setBrushDrawParam(TRUE);

		//選択変更

		p->toggleitem = p->selitem;
	
		p->selitem = item;

		if(item) _update_selitem(TRUE);
	
		return TRUE;
	}
}

/** 登録ブラシセット
 *
 * @param item  NULL で解除 */

void BrushList_setRegisteredItem(BrushItem *item)
{
	BrushList *p = g_dat;

	if(item != p->regitem)
	{
		/* 現在の BrushDrawParam の登録ブラシ用データを解放。
		 * 
		 * [!] item != NULL なら BrushList_setBrushDrawParam() 時に
		 *     行われるので必要ないが、item == NULL の場合は必要なので実行しておく */
		
		_release_drawparam_reg_image(p);
	
		p->regitem = item;
		p->reg_radius = 0;

		if(item) BrushList_setBrushDrawParam(TRUE);
	}
}

/** 直前のブラシと切り替え */

mBool BrushList_toggleLastItem()
{
	BrushList *p = g_dat;

	if(p->toggleitem)
		return BrushList_setSelItem(p->toggleitem);
	else
		return FALSE;
}

/** 選択ブラシを前後に移動 */

mBool BrushList_moveSelect(mBool next)
{
	BrushItem *pi = g_dat->selitem;

	if(pi)
	{
		if(next)
			pi = BRUSHITEM(pi->i.next);
		else
			pi = BRUSHITEM(pi->i.prev);

		if(pi)
			return BrushList_setSelItem(pi);
	}

	return FALSE;
}

/** ブラシデータから描画用パラメータセット
 *
 * @param registered 登録ブラシ用にセット。FALSE でカレントデータ */

void BrushList_setBrushDrawParam(mBool registered)
{
	BrushItem *pi;
	BrushDrawParam *pd;

	if(registered)
	{
		pd = &g_dat->dp_reg;
		pi = g_dat->regitem;

		//登録ブラシの画像解放

		_release_drawparam_reg_image();
	}
	else
		pd = &g_dat->dp_cur, pi = &g_dat->cur;

	//--------

	pd->radius = pi->radius * 0.1;
	pd->opacity = pi->opacity * 0.01;
	pd->min_size = pi->min_size * 0.001;
	pd->min_opacity = pi->min_opacity * 0.001;
	pd->interval_src = pi->interval * 0.01;

	//

	pd->flags = 0;

	if(!(pi->flags & BRUSHITEM_F_ANTIALIAS)) pd->flags |= BRUSHDP_F_NO_ANTIALIAS;
	if(pi->flags & BRUSHITEM_F_CURVE) pd->flags |= BRUSHDP_F_CURVE;

	//矩形上書き

	if(pi->pixelmode == PIXELMODE_OVERWRITE_SQUARE)
		pd->flags |= BRUSHDP_F_OVERWRITE_TP;

	//ランダム

	if(pi->rand_size_min != 1000)
	{
		pd->flags |= BRUSHDP_F_RANDOM_SIZE;
		pd->random_size_min = pi->rand_size_min * 0.001;
	}

	if(pi->rand_pos_range)
	{
		pd->flags |= BRUSHDP_F_RANDOM_POS;
		pd->random_pos_len = pi->rand_pos_range * 0.02;
	}

	//水彩

	if(pi->type == BRUSHITEM_TYPE_WATER)
		pd->flags |= BRUSHDP_F_WATER;

	pd->water_param[0] = pi->water_param[0] * 0.001;
	pd->water_param[1] = pi->water_param[1] * 0.001;
	pd->water_param[2] = pi->water_param[2] * 0.001;

	//ブラシ形状

	pd->shape_hard = _get_drawparam_shape_hard(pi->hardness);
	pd->rough = _get_drawparam_rough(pi->roughness);

	if(pi->hardness == 100)
		pd->flags |= BRUSHDP_F_SHAPE_HARD_MAX;

	pd->angle = (pi->rotate_angle << 9) / 360;  //0-511
	pd->angle_random = (pi->rotate_rand_w << 8) / 180;

	if(pi->flags & BRUSHITEM_F_ROTATE_TRAVELLING_DIR)
		pd->flags |= BRUSHDP_F_TRAVELLING_DIR;

	//筆圧

	pd->pressure_type = pi->pressure_type;

	_set_drawparam_pressure(pd, pi->pressure_type, pi->pressure_val);

	//画像
	//[!] 登録ブラシの場合は常に画像を保持する

	pd->img_shape = MaterialImgList_getImage(MATERIALIMGLIST_TYPE_BRUSH_SHAPE,
		pi->shape_path, registered);

	pd->img_texture = MaterialImgList_getImage(MATERIALIMGLIST_TYPE_BRUSH_TEXTURE,
		pi->texture_path, registered);
}


//======================
// コマンド
//======================


/** 新規ブラシ作成 */

BrushItem *BrushList_newBrush(BrushGroupItem *group)
{
	BrushItem *pi;

	if(group->list.num >= BRUSHITEM_MAXNUM) return NULL;

	pi = (BrushItem *)mListAppendNew(&group->list, sizeof(BrushItem), BrushItem_destroy_handle);
	if(pi)
	{
		pi->group = group;
		
		BrushItem_setDefault(pi);
	}

	return pi;
}

/** 新規グループ */

BrushGroupItem *BrushList_newGroup(const char *name,int colnum)
{
	BrushGroupItem *pg;

	if(g_dat->list.num >= GROUP_MAXNUM) return NULL;

	pg = (BrushGroupItem *)mListAppendNew((mList *)g_dat, sizeof(BrushGroupItem), _groupitem_destroy);
	if(pg)
	{
		pg->name = mStrdup(name);
		pg->colnum = colnum;
		pg->expand = 1;
	}

	return pg;
}

/** グループを上下に移動 */

void BrushList_moveGroup_updown(BrushGroupItem *group,mBool up)
{
	mListMoveUpDown((mList *)g_dat, M_LISTITEM(group), up);
}

/** グループを削除 */

void BrushList_deleteGroup(BrushGroupItem *group)
{
	BrushList *p = g_dat;

	//このグループ内に選択ブラシがある場合、選択ブラシをなしに

	if(p->selitem && p->selitem->group == group)
		BrushList_setSelItem(NULL);

	//このグループ内に登録ブラシがある場合、登録ブラシを解除

	if(p->regitem && p->regitem->group == group)
		BrushList_setRegisteredItem(NULL);

	//このグループ内に切り替えブラシがある場合

	if(p->toggleitem && p->toggleitem->group == group)
		p->toggleitem = NULL;

	//削除

	mListDelete((mList *)p, M_LISTITEM(group));
}

/** ブラシアイテム削除
 *
 * 選択アイテムだった場合、削除後は、グループ内の前後のアイテムが選択される。
 *
 * @return TRUE で選択アイテムが変更された */

mBool BrushList_deleteItem(BrushItem *item)
{
	BrushList *p = g_dat;
	BrushItem *sel = NULL;

	//削除後の選択アイテム

	if(item == p->selitem)
	{
		sel = BRUSHITEM(item->i.next);
		if(!sel) sel = BRUSHITEM(item->i.prev);
	}

	//登録ブラシの場合、解除

	if(item == p->regitem)
		BrushList_setRegisteredItem(NULL);

	//切り替え用ブラシの場合

	if(item == p->toggleitem)
		p->toggleitem = NULL;
	
	//削除

	mListDelete(&item->group->list, M_LISTITEM(item));

	//選択変更

	if(item == p->selitem)
	{
		BrushList_setSelItem(sel);
		return TRUE;
	}
	else
		return FALSE;
}

/** ブラシ位置移動 (D&D)
 *
 * @param dst_group NULL で dst の前に挿入。NULL 以外でグループの終端へ。 */

void BrushList_moveItem(BrushItem *src,BrushGroupItem *dst_group,BrushItem *dst)
{
	if(src == dst) return;

	//src のグループから src を外す

	mListRemove(&src->group->list, M_LISTITEM(src));

	//dst のグループにリンクし直す

	if(dst_group)
	{
		mListAppend(&dst_group->list, M_LISTITEM(src));

		src->group = dst_group;
	}
	else
	{
		mListInsert(&dst->group->list, M_LISTITEM(dst), M_LISTITEM(src));

		src->group = dst->group;
	}
}

/** ブラシ位置を上下移動 */

void BrushList_moveItem_updown(BrushItem *item,mBool up)
{
	mListMoveUpDown(&item->group->list, M_LISTITEM(item), up);
}

/** テキストフォーマットから貼り付け */

mBool BrushList_pasteItem_text(BrushGroupItem *group,BrushItem *item,char *text)
{
	BrushList *p = g_dat;

	//グループに新規作成

	if(group)
	{
		item = BrushList_newBrush(group);
		if(!item) return FALSE;
	}

	//セット

	BrushItem_setFromText(item, text);

	//

	if(group)
		//新規時、選択する
		BrushList_setSelItem(item);
	else
	{
		//上書き時

		if(item == p->selitem)
			//選択アイテムならデータ更新
			_update_selitem(FALSE);
		else if(item == p->regitem)
			//登録ブラシ (非選択) なら描画用パラメータ更新
			BrushList_setBrushDrawParam(TRUE);
	}

	return TRUE;
}

/** 新規貼り付け */

BrushItem *BrushList_pasteItem_new(BrushGroupItem *group,BrushItem *src)
{
	BrushItem *item;

	//新規

	item = BrushList_newBrush(group);
	if(!item) return NULL;

	//コピー

	BrushItem_copy(item, src);

	return item;
}


//============================
// ブラシ項目値変更
//============================
/*
 * - 連携保存時 (手動保存 OFF 時) は、選択アイテムの値も変更。
 * - dp_cur の描画用パラメータも更新する。
 */


/** dp_cur.flags のフラグ ON/OFF
 *
 * @return on を返す */

static mBool _dpcur_flag_onoff(uint32_t f,mBool on)
{
	if(on)
		g_dat->dp_cur.flags |= f;
	else
		g_dat->dp_cur.flags &= ~f;

	return on;
}

/** 半径値変更 */

static void _change_radius(int val)
{
	g_dat->cur.radius = val;
	if(_is_sel_linksave()) g_dat->selitem->radius = val;

	g_dat->dp_cur.radius = val * 0.1;
}


//-----------

/** 手動保存 実行 */

void BrushList_saveitem_manual()
{
	if(g_dat->selitem)
	{
		BrushItem_copy(g_dat->selitem, &g_dat->cur);

		//キャンバスキーでの登録ブラシサイズ変更の値をリセット

		if(g_dat->selitem == g_dat->regitem)
			g_dat->reg_radius = 0;
	}
}

/** 指定ブラシのブラシ名変更
 *
 * @return item が選択ブラシか */

mBool BrushList_setBrushName(BrushItem *item,const char *name)
{
	mStrdup_ptr(&item->name, name);

	//選択アイテムなら、カレントデータも変更
	/* [!] 手動保存で保存する際には名前もコピーされるので、
	 *     ここで変更しておかないと、手動保存の実行時に以前の名前がコピーされてしまう */

	if(item != g_dat->selitem)
		return FALSE;
	else
	{
		mStrdup_ptr(&g_dat->cur.name, name);
		return TRUE;
	}
}

/** ブラシリスト編集後 */

void BrushList_afterBrushListEdit()
{
	//選択アイテムの名前が変更された場合、編集用データの名前も変更する

	if(g_dat->selitem)
		mStrdup_ptr(&g_dat->cur.name, g_dat->selitem->name);
}

/** タイプ */

void BrushList_updateval_type(int val)
{
	BrushItem *cur = &g_dat->cur;
	int n;

	cur->type = val;
	if(_is_sel_linksave()) g_dat->selitem->type = val;

	//塗りタイプ調整

	if(cur->pixelmode >= BrushItem_getPixelModeNum(cur))
	{
		cur->pixelmode = 0;
		if(_is_sel_linksave()) g_dat->selitem->pixelmode = 0;
	}

	//半径値調整 (指先 <-> 他の変更時)

	n = BrushItem_adjustRadius(cur, cur->radius);

	if(n != cur->radius)
		_change_radius(n);

	//水彩フラグ

	_dpcur_flag_onoff(BRUSHDP_F_WATER, (val == BRUSHITEM_TYPE_WATER));
}

/** 半径
 *
 * @return 調整された値 */

int BrushList_updateval_radius(int val)
{
	val = BrushItem_adjustRadius(&g_dat->cur, val);

	_change_radius(val);

	return val;
}

/** 登録ブラシの半径 (キャンバスキーでのサイズ変更時)
 *
 * 登録ブラシが選択ブラシでない状態の時。
 * val : 指先の場合はそのままの値。 */

void BrushList_updateval_radius_reg(int val)
{
	if(g_dat->regitem->flags & BRUSHITEM_F_AUTO_SAVE)
	{
		g_dat->regitem->radius = val;
		g_dat->reg_radius = 0;
	}
	else
		g_dat->reg_radius = val;
	
	g_dat->dp_reg.radius = val * 0.1;
}

/** 濃度 */

void BrushList_updateval_opacity(int val)
{
	g_dat->cur.opacity = val;
	if(_is_sel_linksave()) g_dat->selitem->opacity = val;

	g_dat->dp_cur.opacity = val * 0.01;
}

/** 塗りタイプ */

void BrushList_updateval_pixelmode(int val)
{
	g_dat->cur.pixelmode = val;
	if(_is_sel_linksave()) g_dat->selitem->pixelmode = val;

	//ブラシ形状の透明部分も描画するか

	_dpcur_flag_onoff(BRUSHDP_F_OVERWRITE_TP, (val == PIXELMODE_OVERWRITE_SQUARE));
}

/** 手ブレ補正タイプ */

void BrushList_updateval_hosei_type(int val)
{
	g_dat->cur.hosei_type = val;
	if(_is_sel_linksave()) g_dat->selitem->hosei_type = val;
}

/** 手ブレ補正強さ */

void BrushList_updateval_hosei_str(int val)
{
	g_dat->cur.hosei_strong = val;
	if(_is_sel_linksave()) g_dat->selitem->hosei_strong = val;
}

/** サイズ最小 */

void BrushList_updateval_min_size(int val)
{
	g_dat->cur.min_size = val;
	if(_is_sel_linksave()) g_dat->selitem->min_size = val;

	g_dat->dp_cur.min_size = val * 0.001;
}

/** 濃度最小 */

void BrushList_updateval_min_opacity(int val)
{
	g_dat->cur.min_opacity = val;
	if(_is_sel_linksave()) g_dat->selitem->min_opacity = val;

	g_dat->dp_cur.min_opacity = val * 0.001;
}

/** 間隔 */

void BrushList_updateval_interval(int val)
{
	g_dat->cur.interval = val;
	if(_is_sel_linksave()) g_dat->selitem->interval = val;

	g_dat->dp_cur.interval_src = val * 0.01;
}

/** ランダムサイズ */

void BrushList_updateval_random_size(int val)
{
	g_dat->cur.rand_size_min = val;
	if(_is_sel_linksave()) g_dat->selitem->rand_size_min = val;

	//

	if(_dpcur_flag_onoff(BRUSHDP_F_RANDOM_SIZE, (val != 1000)))
		g_dat->dp_cur.random_size_min = val * 0.001;
}

/** ランダム位置 */

void BrushList_updateval_random_pos(int val)
{
	g_dat->cur.rand_pos_range = val;
	if(_is_sel_linksave()) g_dat->selitem->rand_pos_range = val;

	//

	if(_dpcur_flag_onoff(BRUSHDP_F_RANDOM_POS, (val != 0)))
		g_dat->dp_cur.random_pos_len = val * 0.02;
}

/** 水彩パラメータ */

void BrushList_updateval_waterparam(int no,int val)
{
	g_dat->cur.water_param[no] = val;
	if(_is_sel_linksave()) g_dat->selitem->water_param[no] = val;

	g_dat->dp_cur.water_param[no] = val * 0.001;
}

/** 形状画像 */

void BrushList_updateval_shape_path(const char *path)
{
	mStrdup_ptr(&g_dat->cur.shape_path, path);

	if(_is_sel_linksave())
		mStrdup_ptr(&g_dat->selitem->shape_path, path);

	g_dat->dp_cur.img_shape = MaterialImgList_getImage(MATERIALIMGLIST_TYPE_BRUSH_SHAPE, path, FALSE);
}

/** 形状、硬さ */

void BrushList_updateval_shape_hard(int val)
{
	g_dat->cur.hardness = val;
	if(_is_sel_linksave()) g_dat->selitem->hardness = val;

	//

	g_dat->dp_cur.shape_hard = _get_drawparam_shape_hard(val);

	_dpcur_flag_onoff(BRUSHDP_F_SHAPE_HARD_MAX, (val == 100));
}

/** 形状、荒さ */

void BrushList_updateval_shape_rough(int val)
{
	g_dat->cur.roughness = val;
	if(_is_sel_linksave()) g_dat->selitem->roughness = val;

	g_dat->dp_cur.rough = _get_drawparam_rough(val);
}

/** 進行方向 */

void BrushList_updateval_travelling_dir()
{
	g_dat->cur.flags ^= BRUSHITEM_F_ROTATE_TRAVELLING_DIR;

	if(_is_sel_linksave())
		g_dat->selitem->flags ^= BRUSHITEM_F_ROTATE_TRAVELLING_DIR;

	g_dat->dp_cur.flags ^= BRUSHDP_F_TRAVELLING_DIR;
}

/** 角度 */

void BrushList_updateval_angle(int val)
{
	g_dat->cur.rotate_angle = val;
	if(_is_sel_linksave()) g_dat->selitem->rotate_angle = val;

	g_dat->dp_cur.angle = val * 512 / 360;
}

/** 角度ランダム */

void BrushList_updateval_angle_radom(int val)
{
	g_dat->cur.rotate_rand_w = val;
	if(_is_sel_linksave()) g_dat->selitem->rotate_rand_w = val;

	g_dat->dp_cur.angle_random = val * 256 / 180;
}

/** 筆圧補正タイプ */

void BrushList_updateval_pressure_type(int type)
{
	uint32_t def,defval[3] = {
		100, 50|(50<<8), 30|(30<<8)|(60<<16)|(60<<24)
	};

	//タイプ

	g_dat->cur.pressure_type = type;
	if(_is_sel_linksave()) g_dat->selitem->pressure_type = type;

	g_dat->dp_cur.pressure_type = type;

	//値 (各タイプのデフォルト値に初期化)

	def = defval[type];

	g_dat->cur.pressure_val = def;
	if(_is_sel_linksave()) g_dat->selitem->pressure_val = def;

	_set_drawparam_pressure(&g_dat->dp_cur, type, def);
}

/** 筆圧補正、値 */

void BrushList_updateval_pressure_value(uint32_t val)
{
	g_dat->cur.pressure_val = val;
	if(_is_sel_linksave()) g_dat->selitem->pressure_val = val;

	_set_drawparam_pressure(&g_dat->dp_cur, g_dat->cur.pressure_type, val);
}

/** テクスチャ画像 */

void BrushList_updateval_texture_path(const char *path)
{
	mStrdup_ptr(&g_dat->cur.texture_path, path);

	if(_is_sel_linksave())
		mStrdup_ptr(&g_dat->selitem->texture_path, path);

	g_dat->dp_cur.img_texture = MaterialImgList_getImage(MATERIALIMGLIST_TYPE_BRUSH_TEXTURE, path, FALSE);
}

/** アンチエイリアス */

void BrushList_updateval_antialias(BrushList *p)
{
	g_dat->cur.flags ^= BRUSHITEM_F_ANTIALIAS;
	if(_is_sel_linksave()) g_dat->selitem->flags ^= BRUSHITEM_F_ANTIALIAS;

	g_dat->dp_cur.flags ^= BRUSHDP_F_NO_ANTIALIAS;
}

/** 曲線補間 */

void BrushList_updateval_curve(BrushList *p)
{
	g_dat->cur.flags ^= BRUSHITEM_F_CURVE;
	if(_is_sel_linksave()) g_dat->selitem->flags ^= BRUSHITEM_F_CURVE;

	g_dat->dp_cur.flags ^= BRUSHDP_F_CURVE;
}

/** 詳細設定のダイアログ後
 *
 * 値は常に連動保存させる。 */

void BrushList_updateval_detail()
{
	BrushItem *sel,*cur;

	cur = &g_dat->cur;

	//連動保存

	sel = g_dat->selitem;

	if(sel)
	{
		//自動保存

		if(cur->flags & BRUSHITEM_F_AUTO_SAVE)
			sel->flags |= BRUSHITEM_F_AUTO_SAVE;
		else
			sel->flags &= ~BRUSHITEM_F_AUTO_SAVE;

		//半径 (指先以外)

		if(cur->type != BRUSHITEM_TYPE_FINGER)
		{
			sel->radius = cur->radius;
			sel->radius_min = cur->radius_min;
			sel->radius_max = cur->radius_max;
		}
	}

	//描画用 (半径が調整された場合があるため)

	g_dat->dp_cur.radius = cur->radius * 0.1;
}


/*********** ファイル処理 ***************************/


//===============================
// sub
//===============================


/** 設定ファイル開く */

static FILE *_open_configfile(const char *fname,mBool write)
{
	mStr str = MSTR_INIT;
	FILE *fp;

	mAppGetConfigPath(&str, fname);

	fp = mFILEopenUTF8(str.buf, (write)? "wb": "rb");

	mStrFree(&str);

	return fp;
}


//===============================
// ファイル読み込み
//===============================


/** 文字列読み込み */

static mBool _read_string(FILE *fp,char **ppstr)
{
	char *name;

	if(mFILEreadStr_len16BE(fp, &name) == -1)
		return FALSE;
	else
	{
		mStrdup_ptr(ppstr, name);
		mFree(name);

		return TRUE;
	}
}

/** ブラシアイテム読み込み
 *
 * @return -1 でエラー、それ以外でフラグ */

static int _read_brushitem(FILE *fp,BrushItem *pi)
{
	uint8_t flags;
	uint16_t exsize;

	if(!mFILEreadByte(fp, &flags)
		|| !mFILEread16BE(fp, &exsize)
		
		|| !mFILEread32BE(fp, &pi->pressure_val)
		|| mFILEreadArray16BE(fp, &pi->radius, BRUSHITEM_16BITVAL_NUM) != BRUSHITEM_16BITVAL_NUM
		|| fread(&pi->type, 1, BRUSHITEM_8BITVAL_NUM, fp) != BRUSHITEM_8BITVAL_NUM

		|| !_read_string(fp, &pi->name)
		|| !_read_string(fp, &pi->shape_path)
		|| !_read_string(fp, &pi->texture_path))
		return -1;

	//拡張データ

	fseek(fp, exsize, SEEK_CUR);

	return flags;
}

/** ブラシデータ読み込み */

static void _read_brushdata(BrushList *p,FILE *fp)
{
	uint8_t type,flags,colnum;
	uint16_t size;
	long pos;
	int ret;
	char *name;
	BrushGroupItem *group = NULL;
	BrushItem *pi,*pi_sel = NULL,*pi_reg = NULL;

	while(mFILEreadByte(fp, &type) && type != 255)
	{
		//サイズ

		if(!mFILEread16BE(fp, &size)) break;

		pos = ftell(fp);
		
		//

		if(type == 0)
		{
			//---- ブラシ

			if(!group) break;

			//作成

			pi = BrushList_newBrush(group);
			if(!pi) break;

			//読み込み

			ret = _read_brushitem(fp, pi);
			if(ret == -1) break;

			//フラグ

			if(ret & 1) pi_sel = pi;
			if(ret & 2) pi_reg = pi;
		}
		else if(type == 1)
		{
			//---- グループ
			
			if(!mFILEreadByte(fp, &flags)
				|| !mFILEreadByte(fp, &colnum)
				|| mFILEreadStr_len16BE(fp, &name) == -1)
				break;

			group = BrushList_newGroup(name, colnum);

			mFree(name);

			if(!group) break;

			group->expand = flags & 1;
		}

		//次へ

		fseek(fp, pos + size, SEEK_SET);
	}

	//選択、登録アイテム

	BrushList_setSelItem(pi_sel);
	BrushList_setRegisteredItem(pi_reg);
}

/** ブラシの設定ファイル読み込み */

static void _load_configfile()
{
	FILE *fp;
	uint8_t ver,subver;
	uint16_t wd;

	//開く

	fp = _open_configfile(CONFIG_FILENAME_BRUSH, FALSE);
	if(!fp) return;

	//ヘッダ

	if(!mFILEreadCompareStr(fp, "AZPLBRD")) goto ERR;
	
	//バージョン

	if(!mFILEreadByte(fp, &ver)
		|| ver != 1
		|| !mFILEreadByte(fp, &subver))
		goto ERR;

	//サイズリスト

	if(!mFILEread16BE(fp, &wd)) goto ERR;

	if(wd)
	{
		if(!BrushSizeList_alloc(wd)
			|| mFILEreadArray16BE(fp, BrushSizeList_getBuf(), wd) != wd)
			goto ERR;

		BrushSizeList_setNum(wd);
	}

	//ブラシデータ

	_read_brushdata(g_dat, fp);

ERR:
	fclose(fp);
}

/** ブラシの設定ファイルを読み込み (初期化時) */

void BrushList_loadconfigfile()
{
	_load_configfile();

	//グループが一つもない場合は、新規作成

	if(g_dat->list.num == 0)
		BrushList_newGroup("group", 2);
}


//===============================
// ファイル保存
//===============================


/** ブラシデータ書き込み */

static void _write_brushitem(BrushList *p,FILE *fp,BrushItem *pi)
{
	int n,size;
	fpos_t pos;

	//type (0=brush)
	mFILEwriteByte(fp, 0);

	//size
	fgetpos(fp, &pos);
	mFILEwrite16BE(fp, 0);

	//----------

	size = 1 + 2 + 4 + 2 * BRUSHITEM_16BITVAL_NUM + BRUSHITEM_8BITVAL_NUM;

	//flags (0bit:select 1bit:登録ブラシ)

	n = (pi == p->selitem);
	if(pi == p->regitem) n |= 2;

	mFILEwriteByte(fp, n);

	//拡張データサイズ

	mFILEwrite16BE(fp, 0);

	//基本データ

	mFILEwrite32BE(fp, pi->pressure_val);

	mFILEwriteArray16BE(fp, &pi->radius, BRUSHITEM_16BITVAL_NUM);
	fwrite(&pi->type, 1, BRUSHITEM_8BITVAL_NUM, fp);

	size += mFILEwriteStr_len16BE(fp, pi->name, -1);
	size += mFILEwriteStr_len16BE(fp, pi->shape_path, -1);
	size += mFILEwriteStr_len16BE(fp, pi->texture_path, -1);

	//サイズ

	fsetpos(fp, &pos);
	mFILEwrite16BE(fp, size);
	fseek(fp, 0, SEEK_END);
}

/** 設定ファイルに保存 */

void BrushList_saveconfigfile()
{
	FILE *fp;
	BrushGroupItem *gi;
	BrushItem *pi;
	int n;

	//開く

	fp = _open_configfile(CONFIG_FILENAME_BRUSH, TRUE);
	if(!fp) return;

	//ヘッダ

	fputs("AZPLBRD", fp);
	mFILEwriteByte(fp, 1);	//ver = 1
	mFILEwriteByte(fp, 0);	//sub version

	//サイズリスト

	n = BrushSizeList_getNum();

	mFILEwrite16BE(fp, n);
	mFILEwriteArray16BE(fp, BrushSizeList_getBuf(), n);

	//ブラシデータ

	for(gi = BRUSHGROUPITEM(g_dat->list.top); gi; gi = BRUSHGROUPITEM(gi->i.next))
	{
		//----- グループデータ

		n = mStrlen(gi->name);
		if(n > 0xffff) n = 0xffff;

		//type (1=group)
		mFILEwriteByte(fp, 1);
		//size
		mFILEwrite16BE(fp, 1 + 1 + 2 + n);

		//flags (0bit:expand)
		mFILEwriteByte(fp, gi->expand);
		//横に並べる数
		mFILEwriteByte(fp, gi->colnum);
		//名前
		mFILEwriteStr_len16BE(fp, gi->name, n);

		//------ ブラシアイテム

		for(pi = BRUSHITEM(gi->list.top); pi; pi = BRUSHITEM(pi->i.next))
			_write_brushitem(g_dat, fp, pi);
	}

	//ブラシ終了
	mFILEwriteByte(fp, 255);

	fclose(fp);
}


//===============================
// ver.1 => ver.2 ファイル変換
//===============================


/** ブラシデータ変換 */

static mBool _convert_brush(FILE *fpin,FILE *fpout,uint8_t flags)
{
	char *name,*brush_path = NULL,*tex_path = NULL;
	uint8_t bt,bt2,btbuf[6];
	uint16_t wbuf[15];
	int len[3];
	mBool ret = FALSE;

	//------ 読み込み

	//名前

	len[0] = mFILEreadStr_variableLen(fpin, &name);
	if(len[0] < 0) return FALSE;

	//ブラシ画像パス

	len[1] = mFILEreadStr_variableLen(fpin, &brush_path);
	if(len[1] < 0) goto ERR;

	//テクスチャ画像パス

	len[2] = mFILEreadStr_variableLen(fpin, &tex_path);
	if(len[2] < 0) goto ERR;

	//2byte、1byte データ

	if(fread(wbuf, 1, 15 * 2, fpin) != 15 * 2
		|| fread(btbuf, 1, 6, fpin) != 6)
		goto ERR;

	//======= 書き込み

	//type (0=brush)

	mFILEwriteByte(fpout, 0);

	//データサイズ

	mFILEwrite16BE(fpout,
		1 + 2 + 4 + 2 * BRUSHITEM_16BITVAL_NUM + BRUSHITEM_8BITVAL_NUM
		+ 2 * 3 + len[0] + len[1] + len[2]);

	//flags (0bit:select 1bit:登録ブラシ)

	mFILEwriteByte(fpout, ((flags & 0x40) != 0) | ((flags & 0x80) >> 6));
	
	//拡張データサイズ

	mFILEwrite16BE(fpout, 0);

	//筆圧値 (32bit)

	mFILEwriteZero(fpout, 2);
	fwrite(wbuf + 13, 1, 2, fpout);

	//---- 16bit値 (BE)
	/* 0:半径 1:半径最大 2:最小サイズ 3:最小濃度 4:間隔
	 * 5:ランダムサイズ 6:ランダム位置 7,8,9:水彩1..3
	 * 10:回転角度 11:回転ランダム 12:荒さ 13:筆圧サイズ 14:筆圧濃度 */

	//半径

	fwrite(wbuf, 1, 2, fpout);

	//半径最小/最大

	mFILEwrite16BE(fpout, BRUSHITEM_RADIUS_MIN);
	
	fwrite(wbuf + 1, 1, 2, fpout);

	//最小サイズ/最小濃度/間隔/ランダムサイズ/ランダム位置

	fwrite(wbuf + 2, 1, 2 * 5, fpout);

	//回転角度

	fwrite(wbuf + 10, 1, 2, fpout);

	//水彩

	fwrite(wbuf + 7, 1, 2 * 3, fpout);

	//----- 8bit値
	/* 0:濃度
	 * 1:塗りタイプ (0:重ね 1:A比較 2:上書き 3:消しゴム 4:覆い焼き 5:焼き込み 6:加算 7:ぼかし)
	 * 2:補正タイプ(なし/強/中/弱) 3:補正強さ 4:硬さ
	 * 5:フラグ (0:手動保存 1:水彩 2:進行方向 3:アンチエイリアス 4:曲線補間) */

	//ブラシタイプ/塗りタイプ

	bt2 = 0;

	if(btbuf[5] & (1<<1))
		bt = BRUSHITEM_TYPE_WATER;
	else if(btbuf[1] == 7)
		bt = BRUSHITEM_TYPE_BLUR;
	else if(btbuf[1] == 3)
		bt = BRUSHITEM_TYPE_ERASE;
	else
	{
		bt = 0;

		switch(btbuf[1])
		{
			case 0: bt2 = PIXELMODE_BLEND_PIXEL; break;
			case 1: bt2 = PIXELMODE_COMPARE_A; break;
			case 2: bt2 = PIXELMODE_OVERWRITE_STYLE; break;
			case 4: bt2 = PIXELMODE_DODGE; break;
			case 5: bt2 = PIXELMODE_BURN; break;
			case 6: bt2 = PIXELMODE_ADD; break;
		}
	}

	//ブラシタイプ

	fwrite(&bt, 1, 1, fpout);

	//濃度

	fwrite(btbuf, 1, 1, fpout);

	//塗りタイプ

	fwrite(&bt2, 1, 1, fpout);

	//補正タイプ

	bt = btbuf[2];
	if(bt == 1) bt = 3;
	else if(bt == 3) bt = 1;

	fwrite(&bt, 1, 1, fpout);

	//補正強さ

	mFILEwriteByte(fpout, btbuf[3] + 1);

	//硬さ

	fwrite(btbuf + 4, 1, 1, fpout);

	//荒さ (2byte -> 1byte)

	fwrite((uint8_t *)wbuf + 12 * 2 + 1, 1, 1, fpout);

	//回転ランダム (2byte -> 1byte)

	fwrite((uint8_t *)wbuf + 11 * 2 + 1, 1, 1, fpout);

	//筆圧補正タイプ

	mFILEwriteByte(fpout, 0);

	//フラグ

	bt = 0;
	bt2 = btbuf[5];

	if(!(bt2 & 1)) bt |= BRUSHITEM_F_AUTO_SAVE;
	if(bt2 & 4) bt |= BRUSHITEM_F_ROTATE_TRAVELLING_DIR;
	if(bt2 & 8) bt |= BRUSHITEM_F_ANTIALIAS;
	if(bt2 & 16) bt |= BRUSHITEM_F_CURVE;

	fwrite(&bt, 1, 1, fpout);
	
	//----- 文字列

	mFILEwriteStr_len16BE(fpout, name, len[0]);
	mFILEwriteStr_len16BE(fpout, brush_path, len[1]);
	mFILEwriteStr_len16BE(fpout, tex_path, len[2]);

	//----- 終了

	ret = TRUE;

ERR:	
	mFree(name);
	mFree(brush_path);
	mFree(tex_path);

	return ret;
}

/** ver.1 の設定ファイルから変換 */

void BrushList_convert_from_ver1()
{
	FILE *fpin,*fpout;
	uint8_t bt,*buf;
	uint16_t wd;
	int size,type;

	//読み込み

	fpin = _open_configfile("brush.dat", FALSE);
	if(!fpin) return;

	if(!mFILEreadCompareStr(fpin, "AZPLBRD")
		|| !mFILEreadByte(fpin, &bt)
		|| bt != 0
		|| !mFILEreadByte(fpin, &bt)
		|| bt != 0)
	{
		fclose(fpin);
		return;
	}

	//書き込み

	fpout = _open_configfile(CONFIG_FILENAME_BRUSH, TRUE);
	if(!fpout) return;

	fputs("AZPLBRD", fpout);
	mFILEwriteByte(fpout, 1);
	mFILEwriteByte(fpout, 0);

	//サイズリスト

	if(!mFILEread16BE(fpin, &wd)) goto END;

	if(wd)
	{
		size = wd << 1;

		buf = (uint8_t *)mMalloc(size, FALSE);
		if(!buf) goto END;

		if(fread(buf, 1, size, fpin) != size)
		{
			mFree(buf);
			goto END;
		}

		mFILEwrite16BE(fpout, wd);
		fwrite(buf, 1, size, fpout);

		mFree(buf);
	}

	//ブラシデータ

	while(mFILEreadByte(fpin, &bt))
	{
		type = bt & 0x3f;
		if(type >= 2) break;

		if(type == 1)
		{
			//---- グループ
		
			//type (1=group)
			mFILEwriteByte(fpout, 1);
			//size
			mFILEwrite16BE(fpout, 1 + 1 + 2 + 5);

			//flags (0bit:expand)
			mFILEwriteByte(fpout, ((bt & 0x40) != 0));
			//横に並べる数
			mFILEwriteByte(fpout, 2);
			//名前
			mFILEwrite16BE(fpout, 5);
			fputs("group", fpout);
		}
		else
		{
			//ブラシ

			if(!_convert_brush(fpin, fpout, bt))
				break;
		}
	}

	mFILEwriteByte(fpout, 255);

	//

END:
	fclose(fpin);
	fclose(fpout);
}
