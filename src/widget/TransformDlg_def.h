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
 * 変形ダイアログ 定義
 *****************************************/

/** TransformViewArea : ビューのエリア部分 */

typedef struct
{
	mWidget wg;

	TileImage *imgcopy,	//変形元イメージ
		*imgxor;		//キャンバスのXOR描画用イメージ

	double angle,		//[affine] 角度 (ラジアン)
		scalex,			//[affine] 拡大縮小倍率
		scaley,
		scalex_div,
		scaley_div,
		dcos,			//[affine] 角度のcos,sin
		dsin,
		centerx,		//ソース画像における中心位置
		centery,
		canv_origx,		//キャンバス、イメージの原点位置
		canv_origy,
		dtmp[4],		//ドラッグ時の作業用
		homog_param[9];	//[射影変換] 逆変換のパラメータ

	mPoint ptmov,			//[共通] 平行移動
		ptCornerCanv[4],	//[共通] 4隅のキャンバス座標
		ptLastCanv;			//ドラッグ時、前回のキャンバス位置

	mDoublePoint dptCornerSrc[4],	//[射影変換] 4隅のイメージ位置
		dptCornerDst[4];

	mBox boxsrc;		//元範囲

	int type,			//変形タイプ
		canv_zoom,		//キャンバス表示倍率 (100=100%)
		canv_scrx,		//キャンバススクロール位置
		canv_scry,
		fdrag,
		dragbtt,
		cur_area;		//現在のカーソル下のエリア
	mBool keep_aspect,	//拡大縮小時、縦横比維持
		low_quality;	//ビューを低品質表示 (バーのドラッグ中はONにする)

	CanvasDrawInfo canvinfo;
	CanvasViewParam viewparam;
}TransformViewArea;


//---------------------

enum
{
	TRANSFORM_TYPE_NORMAL,		//通常
	TRANSFORM_TYPE_TRAPEZOID	//遠近
};

enum
{
	TRANSFORM_DSTAREA_NONE,
	TRANSFORM_DSTAREA_LEFT_TOP,
	TRANSFORM_DSTAREA_RIGHT_TOP,
	TRANSFORM_DSTAREA_RIGHT_BOTTOM,
	TRANSFORM_DSTAREA_LEFT_BOTTOM,
	TRANSFORM_DSTAREA_IN_IMAGE,
	TRANSFORM_DSTAREA_OUT_IMAGE
};

enum
{
	TRANSFORM_UPDATE_WITH_CANVAS = 1,
	TRANSFORM_UPDATE_WITH_TRANSPARAM = 2
};

enum
{
	TRANSFORM_AREA_NOTIFY_CHANGE_AFFINE_PARAM = 1,
	TRANSFORM_AREA_NOTIFY_CHANGE_CANVAS_ZOOM
};


#define TRANSFORM_CONFIG_F_KEEPASPECT  1	//XYの倍率を同じにする

#define TRANSFORM_CANVZOOM_MIN    1
#define TRANSFORM_CANVZOOM_MAX    800
#define TRANSFORM_CANVZOOM_UPSTEP 50	//100%以上の時のステップ数 (バー上のみ)

//-----------------


/* TransformViewArea.c */

TransformViewArea *TransformViewArea_create(mWidget *parent,int wid);

void TransformViewArea_init(TransformViewArea *p);
void TransformViewArea_setCanvasZoom(TransformViewArea *p,int zoom);
void TransformViewArea_resetTransformParam(TransformViewArea *p);
void TransformViewArea_update(TransformViewArea *p,uint8_t flags);
void TransformViewArea_startCanvasZoomDrag(TransformViewArea *p);

/* TransformViewArea_sub.c */

void TransformViewArea_image_to_canv_pt(TransformViewArea *p,mPoint *pt,double x,double y);
void TransformViewArea_canv_to_image_dpt(TransformViewArea *p,mDoublePoint *pt,double x,double y);

void TransformViewArea_affine_source_to_canv_pt(TransformViewArea *p,mPoint *pt,double x,double y);
void TransformViewArea_affine_canv_to_source_dpt(TransformViewArea *p,mDoublePoint *ptdst,int x,int y);
void TransformViewArea_affine_canv_to_source_pt(TransformViewArea *p,mPoint *ptdst,int x,int y);
double TransformViewArea_affine_get_canvas_angle(TransformViewArea *p,int x,int y);
void TransformViewArea_affine_canv_to_source_for_drag(TransformViewArea *p,mDoublePoint *ptdst,int x,int y);

mBool TransformViewArea_homog_getDstCanvasBox(TransformViewArea *p,mBox *box);
void TransformViewArea_homog_getDstImageRect(TransformViewArea *p,mRect *rcdst);

void TransformViewArea_onMotion_nodrag(TransformViewArea *p,int x,int y);
void TransformViewArea_onPress_affineZoom(TransformViewArea *p,int cx,int cy);
mBool TransformViewArea_onMotion_homog(TransformViewArea *p,int x,int y);
