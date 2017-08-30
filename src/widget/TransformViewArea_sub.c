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

/***********************************************
 * 変形ダイアログ - TransformViewArea サブ関数
 ***********************************************/

#include <math.h>

#include "mDef.h"
#include "mWidgetDef.h"
#include "mWidget.h"
#include "mRectBox.h"

#include "defDraw.h"
#include "AppCursor.h"

#include "TransformDlg_def.h"


//==============================
// イメージ/キャンバス座標変換
//==============================


/** イメージ座標 -> キャンバス座標変換 */

void TransformViewArea_image_to_canv_pt(TransformViewArea *p,mPoint *pt,double x,double y)
{
	pt->x = floor((x - p->canv_origx) * p->viewparam.scale - p->canv_scrx + (p->wg.w >> 1));
	pt->y = floor((y - p->canv_origy) * p->viewparam.scale - p->canv_scry + (p->wg.h >> 1));
}

/** キャンバス座標 -> イメージ座標変換 */

void TransformViewArea_canv_to_image_dpt(TransformViewArea *p,mDoublePoint *pt,double x,double y)
{
	pt->x = (x + p->canv_scrx - (p->wg.w >> 1)) * p->viewparam.scalediv + p->canv_origx;
	pt->y = (y + p->canv_scry - (p->wg.h >> 1)) * p->viewparam.scalediv + p->canv_origy;
}


//==============================
// アフィン変換
//==============================


/** ソース画像座標 -> アフィン変換 -> キャンバス座標変換 */

void TransformViewArea_affine_source_to_canv_pt(TransformViewArea *p,mPoint *pt,double x,double y)
{
	double xx,yy;

	x = (x - p->centerx) * p->scalex;
	y = (y - p->centery) * p->scaley;

	xx = (x * p->dcos - y * p->dsin) + p->centerx + p->boxsrc.x + p->ptmov.x;
	yy = (x * p->dsin + y * p->dcos) + p->centery + p->boxsrc.y + p->ptmov.y;

	TransformViewArea_image_to_canv_pt(p, pt, xx, yy);
}

/** キャンバス座標 -> ソース画像座標 (mDoublePoint) */

void TransformViewArea_affine_canv_to_source_dpt(TransformViewArea *p,mDoublePoint *ptdst,int x,int y)
{
	mDoublePoint pt;
	double xx,yy;

	//キャンバス -> イメージ座標変換

	TransformViewArea_canv_to_image_dpt(p, &pt, x, y);

	//イメージ座標から逆アフィン変換

	xx = pt.x - p->boxsrc.x - p->ptmov.x - p->centerx;
	yy = pt.y - p->boxsrc.y - p->ptmov.y - p->centery;

	ptdst->x = ( xx * p->dcos + yy * p->dsin) * p->scalex_div + p->centerx;
	ptdst->y = (-xx * p->dsin + yy * p->dcos) * p->scaley_div + p->centery;
}

/** キャンバス座標 -> ソース画像座標 (mPoint) */

void TransformViewArea_affine_canv_to_source_pt(TransformViewArea *p,mPoint *ptdst,int x,int y)
{
	mDoublePoint dpt;

	TransformViewArea_affine_canv_to_source_dpt(p, &dpt, x, y);

	ptdst->x = floor(dpt.x);
	ptdst->y = floor(dpt.y);
}

/** カーソルと矩形範囲中央の角度取得 */

double TransformViewArea_affine_get_canvas_angle(TransformViewArea *p,int x,int y)
{
	mDoublePoint dpt;

	//イメージ座標で計算

	TransformViewArea_canv_to_image_dpt(p, &dpt, x, y);

	return atan2(dpt.y - p->ptmov.y - (p->boxsrc.y + p->centery),
		dpt.x - p->ptmov.x - (p->boxsrc.x + p->centerx));
}

/** キャンバス座標 -> ソース座標 (拡大縮小ドラッグ時) */

void TransformViewArea_affine_canv_to_source_for_drag(TransformViewArea *p,mDoublePoint *ptdst,int x,int y)
{
	mDoublePoint pt;
	double xx,yy;

	//キャンバス -> イメージ座標変換

	TransformViewArea_canv_to_image_dpt(p, &pt, x, y);

	//イメージ座標から逆アフィン変換
	/* ドラッグ開始時の倍率での座標が必要なので、倍率は保存した値を使う */

	xx = pt.x - p->boxsrc.x - p->ptmov.x - p->centerx;
	yy = pt.y - p->boxsrc.y - p->ptmov.y - p->centery;

	ptdst->x = ( xx * p->dcos + yy * p->dsin) * p->dtmp[2] + p->centerx;
	ptdst->y = (-xx * p->dsin + yy * p->dcos) * p->dtmp[3] + p->centery;
}


//==============================
// 射影変換
//==============================


/** 点と辺のベクトル外積 */

static mBool _vector_product(mPoint *pt,mPoint *pt1,mPoint *pt2)
{
	return (((pt2->x - pt1->x) * (pt->y - pt2->y) - (pt->x - pt2->x) * (pt2->y - pt1->y)) >= 0);
}

/** 点と辺のベクトル外積 */

static mBool _homog_judge(mDoublePoint *pt,mDoublePoint *pt1,mDoublePoint *pt2)
{
	return (((pt2->x - pt1->x) * (pt->y - pt2->y) - (pt->x - pt2->x) * (pt2->y - pt1->y)) < 0);
}

/** ポイントが変換後イメージの範囲内か */

static mBool _homog_isPointInImage(TransformViewArea *p,mPoint *pt)
{
	//0-1-3 の三角形の内側か

	if(_vector_product(pt, p->ptCornerCanv, p->ptCornerCanv + 1)
		&& _vector_product(pt, p->ptCornerCanv + 1, p->ptCornerCanv + 3)
		&& _vector_product(pt, p->ptCornerCanv + 3, p->ptCornerCanv))
		return TRUE;

	//1-2-3 の三角形の内側か

	return (_vector_product(pt, p->ptCornerCanv + 1, p->ptCornerCanv + 2)
		&& _vector_product(pt, p->ptCornerCanv + 2, p->ptCornerCanv + 3)
		&& _vector_product(pt, p->ptCornerCanv + 3, p->ptCornerCanv + 1));
}

/** 変換後の描画範囲を取得 (キャンバス座標) */

mBool TransformViewArea_homog_getDstCanvasBox(TransformViewArea *p,mBox *box)
{
	mRect rc;
	int i,x,y;

	rc.x1 = rc.x2 = p->ptCornerCanv[0].x;
	rc.y1 = rc.y2 = p->ptCornerCanv[0].y;

	for(i = 1; i < 4; i++)
	{
		x = p->ptCornerCanv[i].x;
		y = p->ptCornerCanv[i].y;

		if(x < rc.x1) rc.x1 = x;
		else if(x > rc.x2) rc.x2 = x;

		if(y < rc.y1) rc.y1 = y;
		else if(y > rc.y2) rc.y2 = y;
	}

	rc.x1--, rc.y1--;
	rc.x2++, rc.y2++;

	//クリッピング

	if(!mRectClipBox_d(&rc, 0, 0, p->wg.w, p->wg.h))
		return FALSE;
	else
	{
		mBoxSetByRect(box, &rc);
		return TRUE;
	}
}

/** 変換後の描画範囲取得 (イメージ座標) */

void TransformViewArea_homog_getDstImageRect(TransformViewArea *p,mRect *rcdst)
{
	mRect rc;
	int i,x,y;

	rc.x1 = rc.x2 = floor(p->dptCornerDst[0].x + p->ptmov.x);
	rc.y1 = rc.y2 = floor(p->dptCornerDst[0].y + p->ptmov.y);

	for(i = 1; i < 4; i++)
	{
		x = floor(p->dptCornerDst[i].x + p->ptmov.x);
		y = floor(p->dptCornerDst[i].y + p->ptmov.y);

		if(x < rc.x1) rc.x1 = x;
		else if(x > rc.x2) rc.x2 = x;

		if(y < rc.y1) rc.y1 = y;
		else if(y > rc.y2) rc.y2 = y;
	}

	rc.x1--, rc.y1--;
	rc.x2++, rc.y2++;

	*rcdst = rc;
}


//=========================
// 操作
//=========================


/** カーソル位置が変換後のどの範囲内か */

static int _get_point_dstarea(TransformViewArea *p,int x,int y)
{
	int i;
	mPoint pt;

	//4隅 (共通)

	for(i = 0; i < 4; i++)
	{
		pt = p->ptCornerCanv[i];

		if(pt.x - 4 <= x && x <= pt.x + 4
			&& pt.y - 4 <= y && y <= pt.y + 4)
			return TRANSFORM_DSTAREA_LEFT_TOP + i;
	}

	//変換後のイメージ範囲内/外

	if(p->type == TRANSFORM_TYPE_NORMAL)
	{
		//アフィン変換
		
		TransformViewArea_affine_canv_to_source_pt(p, &pt, x, y);

		if(pt.x < 0 || pt.y < 0 || pt.x >= p->boxsrc.w || pt.y >= p->boxsrc.h)
			return TRANSFORM_DSTAREA_OUT_IMAGE;
		else
			return TRANSFORM_DSTAREA_IN_IMAGE;
	}
	else
	{
		//射影変換

		pt.x = x;
		pt.y = y;

		return _homog_isPointInImage(p, &pt)? TRANSFORM_DSTAREA_IN_IMAGE: TRANSFORM_DSTAREA_NONE;
	}
}

/** 非ドラッグの移動時 */

void TransformViewArea_onMotion_nodrag(TransformViewArea *p,int x,int y)
{
	int n,cur;

	n = _get_point_dstarea(p, x, y);

	//エリアが変わったら、カーソル変更

	if(n != p->cur_area)
	{
		p->cur_area = n;

		if(n == TRANSFORM_DSTAREA_NONE)
			cur = -1;
		else if(n == TRANSFORM_DSTAREA_IN_IMAGE)
			cur = APP_CURSOR_HAND;
		else if(n == TRANSFORM_DSTAREA_OUT_IMAGE)
			cur = APP_CURSOR_ROTATE;
		else
		{
			//4隅
			
			if(p->type == TRANSFORM_TYPE_TRAPEZOID)
				cur = APP_CURSOR_MOVE;
			else
			{
				if(n == TRANSFORM_DSTAREA_LEFT_TOP || n == TRANSFORM_DSTAREA_RIGHT_BOTTOM)
					cur = APP_CURSOR_LEFT_TOP;
				else
					cur = APP_CURSOR_RIGHT_TOP;
			}
		}
		
		mWidgetSetCursor(M_WIDGET(p), (cur == -1)? 0: AppCursor_getForDialog(cur));
	}
}

/** アフィン変換(拡大縮小)のドラッグ開始時 */

void TransformViewArea_onPress_affineZoom(TransformViewArea *p,int cx,int cy)
{
	mDoublePoint dpt;

	//押し位置のソース座標

	TransformViewArea_affine_canv_to_source_dpt(p, &dpt, cx, cy);

	p->dtmp[0] = (dpt.x - p->centerx) / p->scalex;
	p->dtmp[1] = (dpt.y - p->centery) / p->scaley;

	//開始時の逆倍率

	p->dtmp[2] = p->scalex_div;
	p->dtmp[3] = p->scaley_div;
}

/** 射影変換の4隅の点移動時 */

mBool TransformViewArea_onMotion_homog(TransformViewArea *p,int x,int y)
{
	mDoublePoint pt,*ppt;
	mBool f = TRUE;

	TransformViewArea_canv_to_image_dpt(p, &pt, x, y);

	pt.x -= p->ptmov.x;
	pt.y -= p->ptmov.y;

	ppt = p->dptCornerDst;

	//調整

	switch(p->fdrag)
	{
		//左上
		case TRANSFORM_DSTAREA_LEFT_TOP:
			f = (_homog_judge(&pt, ppt + 3, ppt + 1)
				&& _homog_judge(&pt, ppt + 2, ppt + 1)
				&& _homog_judge(&pt, ppt + 3, ppt + 2));
			break;
		//右上
		case TRANSFORM_DSTAREA_RIGHT_TOP:
			f = (_homog_judge(&pt, ppt, ppt + 2)
				&& _homog_judge(&pt, ppt, ppt + 3)
				&& _homog_judge(&pt, ppt + 3, ppt + 2));
			break;
		//右下
		case TRANSFORM_DSTAREA_RIGHT_BOTTOM:
			f = (_homog_judge(&pt, ppt + 1, ppt + 3)
				&& _homog_judge(&pt, ppt + 1, ppt)
				&& _homog_judge(&pt, ppt, ppt + 3));
			break;
		//左下
		default:
			f = (_homog_judge(&pt, ppt + 2, ppt)
				&& _homog_judge(&pt, ppt + 1, ppt)
				&& _homog_judge(&pt, ppt + 2, ppt + 1));
			break;
	}

	//変更

	if(f)
	{
		//座標値が 0 の場合は少しずらす

		if(pt.x == 0) pt.x = 0.001;
		if(pt.y == 0) pt.y = 0.001;

		//セット

		p->dptCornerDst[p->fdrag - TRANSFORM_DSTAREA_LEFT_TOP] = pt;
	}

	return f;
}
