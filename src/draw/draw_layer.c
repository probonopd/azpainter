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
 * レイヤ関連
 *****************************************/

#include <string.h>

#include "mDef.h"
#include "mStr.h"
#include "mRectBox.h"
#include "mTrans.h"

#include "defDraw.h"
#include "defFileFormat.h"
#include "AppErr.h"

#include "MainWinCanvas.h"
#include "PopupThread.h"
#include "Docks_external.h"

#include "LayerList.h"
#include "LayerItem.h"
#include "TileImage.h"
#include "Undo.h"

#include "draw_main.h"
#include "draw_op_sub.h"

#include "trgroup.h"
#include "trid_word.h"


//---------------

#define _CURSOR_WAIT    MainWinCanvasArea_setCursor_wait();
#define _CURSOR_RESTORE MainWinCanvasArea_restoreCursor();

//---------------


/** 新規レイヤ
 *
 * @param pi_above NULL 以外で、指定レイヤの上に置く */

mBool drawLayer_newLayer(DrawData *p,
	const char *name,int type,int blendmode,uint32_t col,LayerItem *pi_above)
{
	LayerItem *pi;
	mBool expand;

	//追加時に pi_above フォルダが展開されてしまうので、追加前の状態を記録

	if(pi_above)
		expand = LAYERITEM_IS_EXPAND(pi_above);

	//追加

	pi = LayerList_addLayer_image(p->layerlist, (pi_above)? pi_above: p->curlayer, type);
	if(!pi) return FALSE;

	mStrdup_ptr(&pi->name, name);

	LayerItem_setLayerColor(pi, col);

	pi->blendmode = blendmode;

	//指定レイヤの上に移動

	if(pi_above)
	{
		LayerList_moveitem(p->layerlist, pi, pi_above, FALSE);

		//フォルダ非展開に戻す

		if(!expand)
			pi_above->flags &= ~LAYERITEM_F_FOLDER_EXPAND;
	}

	//

	p->curlayer = pi;

	//

	Undo_addLayerNew(FALSE);

	DockLayer_update_all();

	return TRUE;
}

/** 新規フォルダ */

mBool drawLayer_newFolder(DrawData *p,const char *name,LayerItem *pi_above)
{
	LayerItem *pi;
	mBool expand;

	//追加時に pi_above フォルダが展開されてしまうので、追加前の状態を記録

	if(pi_above)
		expand = LAYERITEM_IS_EXPAND(pi_above);

	//追加

	pi = LayerList_addLayer(p->layerlist, (pi_above)? pi_above: p->curlayer);
	if(!pi) return FALSE;

	mStrdup_ptr(&pi->name, name);

	pi->flags |= LAYERITEM_F_FOLDER_EXPAND;

	//指定レイヤの上に移動

	if(pi_above)
	{
		LayerList_moveitem(p->layerlist, pi, pi_above, FALSE);

		//フォルダ非展開に戻す

		if(!expand)
			pi_above->flags &= ~LAYERITEM_F_FOLDER_EXPAND;
	}

	//

	p->curlayer = pi;

	//

	Undo_addLayerNew(FALSE);

	DockLayer_update_all();

	return TRUE;
}

/** ダイアログなしでの新規レイヤ作成
 *
 * @param type 負の値でフォルダ */

void drawLayer_newLayer_direct(DrawData *p,int type)
{
	if(type < 0)
		drawLayer_newFolder(p, M_TR_T2(TRGROUP_WORD, TRID_WORD_FOLDER), NULL);
	else
	{
		mStr str = MSTR_INIT;

		mStrSetFormat(&str, "%s%d",
			M_TR_T2(TRGROUP_WORD, TRID_WORD_LAYER), LayerList_getNum(p->layerlist));

		drawLayer_newLayer(p, str.buf, type, 0, 0, NULL);

		mStrFree(&str);
	}
}

/** 指定イメージから新規レイヤ作成 (イメージ置き換え)
 *
 * 選択範囲:新規レイヤ貼り付け用 */

mBool drawLayer_newLayer_fromImage(DrawData *p,TileImage *img,uint32_t col)
{
	LayerItem *pi;

	//作成

	pi = LayerList_addLayer(p->layerlist, p->curlayer);
	if(!pi) return FALSE;

	LayerItem_replaceImage(pi, img);
	LayerItem_setLayerColor(pi, col);

	LayerList_setItemName_curlayernum(p->layerlist, pi);

	//カレントレイヤ

	p->curlayer = pi;

	//undo

	Undo_addLayerNew(TRUE);

	//更新

	drawUpdate_rect_imgcanvas_canvasview_inLayerHave(p, pi);

	DockLayer_update_all();

	return TRUE;
}


//===============================
// ファイルから読み込み
//===============================


typedef struct
{
	const char *filename;
	TileImage *img;
	LayerItem *item;
	uint32_t format;
	mBool ignore_alpha;
}_thdata_loadfile;


/** [スレッド] ファイルから読み込み */

static int _thread_loadfile(mPopupProgress *prog,void *data)
{
	_thdata_loadfile *p = (_thdata_loadfile *)data;

	if(p->format & FILEFORMAT_APD_v3)
	{
		//APD v3

		p->item = LayerList_addLayer_apdv3(APP_DRAW->layerlist, APP_DRAW->curlayer,
				p->filename, prog);

		return (p->item)? APPERR_OK: APPERR_LOAD;
	}
	else
	{
		//画像ファイル

		p->img = TileImage_loadFile(p->filename, 0,
			p->ignore_alpha, 20000, NULL, prog, NULL);

		return (p->img)? APPERR_OK: APPERR_LOAD;
	}
}

/** ファイルから読み込んで新規レイヤ
 *
 * [!] APD ver 3 からの読み込みの場合、先頭レイヤがフォルダの場合は
 *     フォルダとして読み込まれるので注意。
 *
 * @param rcupdate 更新範囲が入る。複数ファイルの同時読み込み用 (NULL で単体更新)
 * @return APPERR */

int drawLayer_newLayer_file(DrawData *p,const char *filename,
	mBool ignore_alpha,mRect *rcupdate)
{
	_thdata_loadfile dat;
	LayerItem *item;
	int err;
	uint32_t format;
	mRect rc;
	mStr str = MSTR_INIT;

	//ファイルフォーマット (通常画像 or APD v3 のみ読み込み可)

	format = FileFormat_getbyFileHeader(filename);

	if(!(format & FILEFORMAT_OPEN_LAYERIMAGE))
	{
		return (format & FILEFORMAT_APD)? APPERR_LOAD_ONLY_APDv3: APPERR_UNSUPPORTED_FORMAT;
	}

	//スレッド用データ

	mMemzero(&dat, sizeof(_thdata_loadfile));

	dat.filename = filename;
	dat.format = format;
	dat.ignore_alpha = ignore_alpha;

	//読み込み

	err = PopupThread_run(&dat, _thread_loadfile);
	if(err == -1) err = APPERR_ALLOC;

	if(err != APPERR_OK) return err;

	//レイヤ追加

	if(format & FILEFORMAT_APD_v3)
		item = dat.item;
	else
	{
		//------ 画像ファイルから
	
		item = LayerList_addLayer(p->layerlist, p->curlayer);
		if(!item)
		{
			TileImage_free(dat.img);
			return APPERR_ALLOC;
		}

		LayerItem_replaceImage(item, dat.img);

		//名前 (ファイル名から)

		mStrPathGetFileNameNoExt(&str, filename);

		mStrdup_ptr(&item->name, str.buf);

		mStrFree(&str);
	}

	//

	p->curlayer = item;

	/* 読み込まれた結果フォルダの場合があるので注意 */

	//undo

	Undo_addLayerNew((item->img != NULL));

	//更新など

	TileImage_getHaveImageRect_pixel(item->img, &rc, NULL);

	if(rcupdate)
		mRectUnion(rcupdate, &rc);
	else
	{
		drawUpdate_rect_imgcanvas_canvasview_fromRect(p, &rc);
		DockLayer_update_all();
	}

	return APPERR_OK;
}


//===============================
// コマンド
//===============================


/** カレントレイヤを複製 */

void drawLayer_copy(DrawData *p)
{
	LayerItem *pi;

	//フォルダは不可

	if(LAYERITEM_IS_FOLDER(p->curlayer)) return;

	//複製

	pi = LayerList_dupLayer(p->layerlist, p->curlayer);
	if(!pi) return;

	p->curlayer = pi;

	//undo

	Undo_addLayerCopy();

	//更新

	drawUpdate_rect_imgcanvas_canvasview_inLayerHave(p, pi);
	DockLayer_update_all();
}

/** カレントレイヤを削除 */

void drawLayer_delete(DrawData *p,mBool update)
{
	mRect rc;
	LayerItem *cur;

	cur = p->curlayer;

	/* 親がルートで、前後にレイヤがない場合 (そのレイヤがフォルダで、子がある場合も含む)、
	 * 削除するとレイヤが一つもなくなるので、削除しない */

	if(!(cur->i.parent) && !(cur->i.prev) && !(cur->i.next))
		return;

	//undo、更新範囲

	if(update)
	{
		Undo_addLayerDelete();

		LayerItem_getVisibleImageRect(cur, &rc);
	}

	//削除

	p->curlayer = LayerList_deleteLayer(p->layerlist, cur);

	//更新

	if(update)
	{
		drawUpdate_rect_imgcanvas_canvasview_fromRect(p, &rc);
		DockLayer_update_all();
	}
}

/** カレントレイヤを消去 */

void drawLayer_erase(DrawData *p)
{
	mRect rc;

	if(drawOpSub_canDrawLayer(p)) return;

	Undo_addLayerClearImage();

	LayerItem_getVisibleImageRect(p->curlayer, &rc);

	//

	TileImage_clear(p->curlayer->img);

	//更新

	drawUpdate_rect_imgcanvas_canvasview_fromRect(p, &rc);
	DockLayer_update_curlayer(FALSE);
}

/** レイヤタイプ変更 */

void drawLayer_setColorType(DrawData *p,LayerItem *item,int type,mBool bLumtoAlpha)
{
	mRect rc;

	_CURSOR_WAIT

	//undo

	Undo_addLayerSetColorType(item, type, bLumtoAlpha);

	//変換

	LayerItem_getVisibleImageRect(item, &rc);

	TileImage_convertType(item->img, type, bLumtoAlpha);

	item->coltype = type;

	//更新

	drawUpdate_rect_imgcanvas_canvasview_fromRect(p, &rc);
	DockLayer_update_layer(item);

	_CURSOR_RESTORE
}

/** イメージ全体の編集 (反転/回転)
 *
 * フォルダの場合は下位レイヤもすべて処理される。 */

void drawLayer_editFullImage(DrawData *p,int type)
{
	mRect rc;

	if(LayerItem_editFullImage(p->curlayer, type, &rc))
	{
		//undo

		Undo_addLayerFullEdit(type);

		//更新

		drawUpdate_rect_imgcanvas_canvasview_fromRect(p, &rc);

		if(LAYERITEM_IS_IMAGE(p->curlayer))
			DockLayer_update_curlayer(FALSE);
		else
			DockLayer_update_all();
	}
}


//===============================
// 下レイヤ結合/移す
//===============================


typedef struct
{
	LayerItem *item_src,*item_dst;
	int opacity_dst;
	mRect rcarea;
}_thdata_combine;


/** [スレッド] 下レイヤに結合/移す */

static int _thread_combine(mPopupProgress *prog,void *data)
{
	_thdata_combine *p = (_thdata_combine *)data;

	TileImage_combine(p->item_dst->img, p->item_src->img, &p->rcarea,
		p->item_src->opacity, p->opacity_dst, p->item_src->blendmode,
		p->item_src->img8texture, prog);

	return 1;
}

/** 下レイヤに結合 or 移す */

void drawLayer_combine(DrawData *p,mBool drop)
{
	LayerItem *item_src,*item_dst;
	mRect rcsrc,rcdst;
	_thdata_combine dat;

	//結合元と結合先レイヤ (フォルダは除く)

	item_src = p->curlayer;
	if(LAYERITEM_IS_FOLDER(item_src)) return;

	item_dst = (LayerItem *)item_src->i.next;
	if(!item_dst || LAYERITEM_IS_FOLDER(item_dst)) return;

	//イメージ範囲

	TileImage_getHaveImageRect_pixel(item_src->img, &rcsrc, NULL);
	TileImage_getHaveImageRect_pixel(item_dst->img, &rcdst, NULL);

	//"移す"で、元イージが空ならそのまま

	if(drop && mRectIsEmpty(&rcsrc)) return;

	//処理する px 範囲
	/* "結合"の場合、結合先のレイヤ不透明度を結合後イメージに適用させた後、
	 * レイヤ不透明度を 100% に変更するので、結合先全体も処理範囲に含める。 */

	if(!drop)
		mRectUnion(&rcsrc, &rcdst);
	
	//undo

	Undo_addLayerDropAndCombine(drop);

	//結合処理

	if(!mRectIsEmpty(&rcsrc))
	{
		dat.item_dst = item_dst;
		dat.item_src = item_src;
		dat.opacity_dst = (drop)? LAYERITEM_OPACITY_MAX: item_dst->opacity;
		dat.rcarea = rcsrc;

		if(PopupThread_run(&dat, _thread_combine) == -1)
			return;
	}

	//

	if(drop)
	{
		//"移す" => カレントレイヤを消去

		TileImage_clear(item_src->img);

		DockLayer_update_curlayer(FALSE);
		DockLayer_update_layer(item_dst);
	}
	else
	{
		//"結合"

		item_dst->opacity = LAYERITEM_OPACITY_MAX;

		//カレントレイヤを削除
		drawLayer_delete(p, FALSE);

		DockLayer_update_all();
	}

	drawUpdate_rect_imgcanvas_canvasview_fromRect(p, &rcsrc);
}


//===============================
// 複数レイヤ結合
//===============================


typedef struct
{
	TileImage *imgdst;
	LayerItem *topitem;
	mRect rcupdate;
}_thdata_combinemulti;


/** [スレッド] 複数レイヤ結合 */

static int _thread_combine_multi(mPopupProgress *prog,void *data)
{
	_thdata_combinemulti *p = (_thdata_combinemulti *)data;
	LayerItem *pi;
	int num;
	mRect rc;

	mRectEmpty(&p->rcupdate);

	//結合数

	for(pi = p->topitem, num = 0; pi; pi = pi->link, num++);

	//結合

	mPopupProgressThreadSetMax(prog, num);

	for(pi = p->topitem; pi; pi = pi->link)
	{
		if(TileImage_getHaveImageRect_pixel(pi->img, &rc, NULL))
		{
			TileImage_combine(p->imgdst, pi->img, &rc,
				LayerItem_getOpacity_real(pi), LAYERITEM_OPACITY_MAX,
				pi->blendmode, pi->img8texture, NULL);

			mRectUnion(&p->rcupdate, &rc);
		}

		mPopupProgressThreadIncPos(prog);
	}

	return 1;
}

/** 複数レイヤ結合 */

void drawLayer_combineMulti(DrawData *p,int target,mBool newlayer,int coltype)
{
	LayerItem *top,*newitem,*pi;
	TileImage *img;
	_thdata_combinemulti dat;

	//リンクをセット (top が NULL の場合あり)

	top = LayerList_setLink_combineMulti(p->layerlist, target, p->curlayer);

	//結合用イメージ作成

	img = TileImage_new(coltype, 1, 1);
	if(!img) return;

	//img にイメージ結合

	if(!top)
		mRectEmpty(&dat.rcupdate);
	else
	{
		dat.imgdst = img;
		dat.topitem = top;

		if(PopupThread_run(&dat, _thread_combine_multi) == -1)
		{
			TileImage_free(img);
			return;
		}
	}

	//undo (新規レイヤでない場合)

	if(!newlayer)
	{
		if(target == 0)
			Undo_addLayerCombineAll();
		else
			Undo_addLayerCombineFolder();
	}

	//各レイヤ処理

	switch(target)
	{
		//すべての表示レイヤ
		case 0:
			if(!newlayer)
				LayerList_clear(p->layerlist);

			newitem = LayerList_addLayer(p->layerlist, NULL);

			if(newlayer)
				LayerList_moveitem(p->layerlist, newitem, LayerList_getItem_top(p->layerlist), FALSE);

			LayerList_setItemName_curlayernum(p->layerlist, newitem);
			break;
		//フォルダ内
		case 1:
			newitem = LayerList_addLayer(p->layerlist, NULL);

			LayerList_moveitem(p->layerlist, newitem, p->curlayer, FALSE);

			mStrdup_ptr(&newitem->name, p->curlayer->name);

			if(!newlayer)
				drawLayer_delete(p, FALSE);
			break;
		//チェック (チェックの一番上のレイヤの上へ)
		default:
			for(pi = top; pi && pi->link; pi = pi->link);

			newitem = LayerList_addLayer(p->layerlist, pi);

			LayerList_setItemName_curlayernum(p->layerlist, newitem);
			break;
	}

	LayerItem_replaceImage(newitem, img);

	p->curlayer = newitem;

	//undo (新規レイヤ時)

	if(newlayer)
		Undo_addLayerNew(TRUE);

	//更新

	drawUpdate_rect_imgcanvas_canvasview_fromRect(p, &dat.rcupdate);
	DockLayer_update_all();
}


//===============================
// 画像の統合
//===============================


/** [スレッド] 画像の統合 */

static int _thread_blendall(mPopupProgress *prog,void *data)
{
	TileImage *img = (TileImage *)data;

	//ImageBufRGB16 に合成

	drawImage_blendImage_real(APP_DRAW);

	//ImageBufRGB16 からイメージ作成

	TileImage_setImage_fromImageBufRGB16(img, APP_DRAW->blendimg, prog);
	
	return 1;
}

/** 画像の統合 (すべて合成) */

void drawLayer_blendAll(DrawData *p)
{
	TileImage *img;
	LayerItem *item;
	
	//統合後のイメージを作成

	img = TileImage_new(TILEIMAGE_COLTYPE_RGBA, p->imgw, p->imgh);
	if(!img) return;

	//実行

	if(PopupThread_run(img, _thread_blendall) == -1)
	{
		TileImage_free(img);
		return;
	}

	//undo

	Undo_addLayerCombineAll();

	//レイヤクリア & 追加

	LayerList_clear(p->layerlist);

	item = LayerList_addLayer(p->layerlist, NULL);

	//

	LayerList_setItemName_curlayernum(p->layerlist, item);

	LayerItem_replaceImage(item, img);

	p->curlayer = item;

	//更新

	drawUpdate_all_layer();
}


//===============================
// 状態など変更
//===============================


/** カレントレイヤ変更
 *
 * レイヤ一覧上で、変更前のレイヤと変更後のレイヤが更新される。
 * 変更前のレイヤを更新させない場合は DrawData::curlayer を NULL にしておく。
 *
 * @return 変更されたか */

mBool drawLayer_setCurrent(DrawData *p,LayerItem *item)
{
	if(item == p->curlayer)
		return FALSE;
	else
	{
		LayerItem *last = p->curlayer;

		p->curlayer = item;

		DockLayer_update_layer(last);
		DockLayer_update_curlayer(TRUE);

		return TRUE;
	}
}

/** カレントレイヤ変更 (一覧上でカレントが見える状態にする)
 *
 * - item = カレントの場合でも、スクロールは行う。
 * - item がフォルダに隠れて見えない場合は親を展開。
 * - 一覧上で範囲外の位置にあるならスクロール。 */

void drawLayer_setCurrent_visibleOnList(DrawData *p,LayerItem *item)
{
	LayerItem *last;
	mBool update_all = FALSE;

	//以前のカレント (変わらないなら NULL)

	last = p->curlayer;
	if(last == item) last = NULL;

	//カレント変更

	p->curlayer = item;

	//閉じたフォルダに隠れている場合は親を展開

	if(!LayerItem_isVisibleOnList(item))
	{
		LayerItem_setExpandParent(item);
		update_all = TRUE;
	}

	//更新

	DockLayer_update_changecurrent_visible(last, update_all);
}

/** テクスチャセット */

void drawLayer_setTexture(DrawData *p,const char *path)
{
	LayerItem_setTexture(p->curlayer, path);

	drawUpdate_rect_imgcanvas_canvasview_inLayerHave(p, p->curlayer);
}

/** 線の色変更 */

void drawLayer_setLayerColor(DrawData *p,LayerItem *item,uint32_t col)
{
	if(LayerItem_isType_layercolor(item))
	{
		LayerItem_setLayerColor(item, col);

		drawUpdate_rect_imgcanvas_canvasview_inLayerHave(p, item);
		DockLayer_update_layer(item);
	}
}

/** フォルダの展開状態を反転 */

void drawLayer_revFolderExpand(DrawData *p,LayerItem *item)
{
	item->flags ^= LAYERITEM_F_FOLDER_EXPAND;

	DockLayer_update_all();
}

/** 表示/非表示を反転 */

void drawLayer_revVisible(DrawData *p,LayerItem *item)
{
	mRect rc;

	if(LAYERITEM_IS_VISIBLE(item))
	{
		//表示 => 非表示

		LayerItem_getVisibleImageRect(item, &rc);

		item->flags ^= LAYERITEM_F_VISIBLE;

		drawUpdate_rect_imgcanvas_canvasview_fromRect(p, &rc);
	}
	else
	{
		//非表示 => 表示

		item->flags ^= LAYERITEM_F_VISIBLE;

		drawUpdate_rect_imgcanvas_canvasview_inLayerHave(p, item);
	}
}

/** ロック状態を反転 */

void drawLayer_revLock(DrawData *p,LayerItem *item)
{
	item->flags ^= LAYERITEM_F_LOCK;

	//undo

	Undo_addLayerFlags(item, LAYERITEM_F_LOCK);

	//更新

	DockLayer_update_layer(item);
}

/** 塗りつぶし判定元フラグ反転 */

void drawLayer_revFillRef(DrawData *p,LayerItem *item)
{
	item->flags ^= LAYERITEM_F_FILLREF;

	DockLayer_update_layer(item);
}

/** チェックフラグ反転 */

void drawLayer_revChecked(DrawData *p,LayerItem *item)
{
	item->flags ^= LAYERITEM_F_CHECKED;

	DockLayer_update_layer(item);
}

/** レイヤマスク
 *
 * @param type -1:ON/OFF -2:下レイヤマスクON/OFF */

void drawLayer_setLayerMask(DrawData *p,LayerItem *item,int type)
{
	int under_on,mask_on;
	LayerItem *update_item = NULL;

	mask_on = (item == p->masklayer);
	under_on = item->flags & LAYERITEM_F_MASK_UNDER;

	switch(type)
	{
		//通常クリック
		/* マスクレイヤ、下レイヤマスクを OFF or マスクレイヤ ON */
		case -1:
			if(mask_on)
				p->masklayer = NULL;
			else if(under_on)
				item->flags ^= LAYERITEM_F_MASK_UNDER;
			else
			{
				//他のレイヤがマスクレイヤなら、そのレイヤも更新
				if(p->masklayer) update_item = p->masklayer;
				
				p->masklayer = item;
			}
			break;

		//下レイヤマスクON/OFF
		case -2:
			if(mask_on) p->masklayer = NULL;

			item->flags ^= LAYERITEM_F_MASK_UNDER;
			break;
	}

	//更新

	DockLayer_update_layer(item);
	if(update_item) DockLayer_update_layer(update_item);
}


//=========================
// 表示
//=========================


/** すべて表示/非表示/カレントのみ
 *
 * @param type [0]非表示 [1]表示 [2]カレントのみ */

void drawLayer_showAll(DrawData *p,int type)
{
	LayerList_setVisible_all(p->layerlist, (type == 1));

	if(type == 2)
		LayerItem_setVisible(p->curlayer);

	drawUpdate_all_layer(p);
}

/** チェックレイヤの表示反転 */

void drawLayer_showRevChecked(DrawData *p)
{
	mRect rc;

	LayerList_showRevChecked(p->layerlist, &rc);

	drawUpdate_rect_imgcanvas_canvasview_fromRect(p, &rc);
	DockLayer_update_all();
}

/** フォルダを除くレイヤの表示反転 */

void drawLayer_showRevImage(DrawData *p)
{
	mRect rc;

	LayerList_showRevImage(p->layerlist, &rc);

	drawUpdate_rect_imgcanvas_canvasview_fromRect(p, &rc);
	DockLayer_update_all();
}

/** すべてのレイヤの指定フラグを OFF */

void drawLayer_allFlagsOff(DrawData *p,uint32_t flags)
{
	LayerList_allFlagsOff(p->layerlist, flags);

	DockLayer_update_all();
}


//==============================
// フォルダ
//==============================


/** すべてのフォルダを閉じる */

void drawLayer_closeAllFolders(DrawData *p)
{
	LayerList_closeAllFolders(p->layerlist, p->curlayer);

	DockLayer_update_all();
}


//==============================
// 移動
//==============================


/** カレントレイヤを上下に移動 (同じフォルダ内で) */

void drawLayer_moveUpDown(DrawData *p,mBool bUp)
{
	int parent,no;

	LayerList_getItemPos_forParent(p->layerlist, p->curlayer, &parent, &no);

	if(LayerList_moveitem_updown(p->layerlist, p->curlayer, bUp))
	{
		//undo

		Undo_addLayerMoveList(p->curlayer, parent, no);

		//更新

		drawUpdate_rect_imgcanvas_canvasview_inLayerHave(p, p->curlayer);
		DockLayer_update_all();
	}
}

/** D&D によるレイヤ移動
 *
 * @param type [0]挿入 [1]フォルダ */

void drawLayer_moveDND(DrawData *p,LayerItem *dst,int type)
{
	LayerItem *cur;
	mTreeItem *parent,*prev;
	int pno,no;

	cur = p->curlayer;
	parent = cur->i.parent;
	prev = cur->i.prev;

	LayerList_getItemPos_forParent(p->layerlist, cur, &pno, &no);

	//移動

	LayerList_moveitem(p->layerlist, cur, dst, (type == 1));

	//位置が変わっていない

	if(cur->i.parent == parent && cur->i.prev == prev)
		return;

	//undo

	Undo_addLayerMoveList(cur, pno, no);

	//更新

	DockLayer_update_all();
	drawUpdate_rect_imgcanvas_canvasview_inLayerHave(p, cur);
}

/** チェックレイヤを指定フォルダに移動 */

void drawLayer_moveCheckedToFolder(DrawData *p,LayerItem *dst)
{
	mRect rc;
	int16_t *buf;
	int num;

	buf = LayerList_moveitem_checkedToFolder(p->layerlist, dst, &num, &rc);

	if(buf)
	{
		//UNDO

		Undo_addLayerMoveList_multi(num, buf);
		mFree(buf);

		//更新
	
		DockLayer_update_all();
		drawUpdate_rect_imgcanvas_canvasview_fromRect(p, &rc);
	}
}


//==============================
// アンドゥ実行用
//==============================


/** レイヤ削除 (UNDO 実行用) */

void drawLayer_deleteForUndo(DrawData *p,LayerItem *item)
{
	LayerItem *cur;

	if(!item) return;

	//削除後のカレントレイヤ
	/* item が「現在のカレントレイヤ」または「現在のカレントレイヤが item の子」の場合、
	 * カレントレイヤが削除されることになるので、item の前後をカレントにする。 */

	cur = p->curlayer;

	if(item == cur || LayerItem_isChildItem(cur, item))
	{
		cur = LayerItem_getNextPass(item);
		if(!cur) cur = LayerItem_getPrevExpand(item);
	}

	//レイヤ削除

	LayerList_deleteLayer(APP_DRAW->layerlist, item);

	//カレントレイヤ

	p->curlayer = cur;
}

/** アンドゥでレイヤ順番移動をした後の処理 */

void drawLayer_afterMoveList_forUndo(DrawData *p)
{
	/* 現在のカレントがリスト上で見えない場合、
	 * (移動後に非展開のフォルダの子になった場合)
	 * カレントの親をすべて展開して見えるようにする。 */

	if(!LayerItem_isVisibleOnList(p->curlayer))
		LayerItem_setExpandParent(p->curlayer);
}


//==============================
// ほか
//==============================


/** レイヤの選択を一つ上下に移動 */

void drawLayer_currentSelUpDown(DrawData *p,mBool up)
{
	LayerItem *cur = p->curlayer;

	if(up)
		cur = LayerItem_getPrevExpand(cur);
	else
		cur = LayerItem_getNextExpand(cur);

	if(cur)
		drawLayer_setCurrent(p, cur);
}

/** 押し位置に点がある最上位のレイヤを選択 */

void drawLayer_selectPixelTopLayer(DrawData *p)
{
	LayerItem *item;
	mPoint pt;

	drawOpSub_getImagePoint_int(p, &pt);

	item = LayerList_getItem_topPixelLayer(p->layerlist, pt.x, pt.y);

	if(item)
		drawLayer_setCurrent_visibleOnList(p, item);
}

