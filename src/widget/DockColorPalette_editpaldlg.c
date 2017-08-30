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
 * カラーパレット編集ダイアログ
 *****************************************/

#include <string.h>

#include "mDef.h"
#include "mDialog.h"
#include "mWidget.h"
#include "mContainer.h"
#include "mWindow.h"
#include "mLabel.h"
#include "mButton.h"
#include "mLineEdit.h"
#include "mScrollBar.h"
#include "mEvent.h"
#include "mTrans.h"
#include "mPixbuf.h"
#include "mSysCol.h"
#include "mStr.h"

#include "defDraw.h"
#include "ColorPalette.h"

#include "trgroup.h"


//----------------------

typedef struct
{
	uint8_t *palbuf;	//作業用パレットバッファ (最大数分確保)
	int curno,		//カーソル位置
		seltop,		//-1 で選択なし
		selend,
		palnum;
}_data;

typedef struct
{
	mWidget wg;

	_data *pdat;
	mScrollBar *scr;

	int xnum,
		fdrag,
		dragno;
}_palarea;

typedef struct
{
	mWidget wg;
	mContainerData ct;
	mWindowData win;
	mDialogData dlg;

	_palarea *area;
	mScrollBar *scr;
	mLineEdit *edit_num,
		*edit_rgb;

	_data dat;
}_dialog;

//----------------------

enum
{
	TRID_TITLE,
	TRID_DELETE,
	TRID_GRADATION,
	TRID_NUM,
	TRID_ADD,
	TRID_INSERT,
	TRID_RGB_LABEL,
	TRID_HELP,
};

enum
{
	WID_SCROLL = 100,
	WID_BTT_DELETE,
	WID_BTT_GRADATION,
	WID_EDIT_NUM,
	WID_BTT_ADD,
	WID_BTT_INSERT,
	WID_EDIT_RGB
};

enum
{
	_DRAGF_NONE,
	_DRAGF_PRESS,
	_DRAGF_DRAG
};

#define _CELLW   17
#define _CELLH   17
#define _COL_SEL       0x6E51FF
#define _COL_CURFRAME  0xff0000
#define _COL_INSERT    0xff0000

//----------------------



/***********************************
 * データ処理
 ***********************************/


/** 処理範囲取得 */

static void _data_get_area(_data *p,int *top,int *end)
{
	if(p->seltop == -1)
		*top = *end = p->curno;
	else
		*top = p->seltop, *end = p->selend;
}

/** 位置がカーソルか選択範囲内か */

static mBool _data_is_in_cursel(_data *p,int no)
{
	return (no == p->curno
		|| (p->seltop != -1 && p->seltop <= no && no <= p->selend));
}

/** +Shift 選択 (カーソル位置から指定位置まで) */

static void _data_select(_data *p,int no)
{
	int top,end;

	if(no == p->curno)
		//カーソル位置なら選択解除
		p->seltop = -1;
	else
	{
		if(no <= p->curno)
			top = no, end = p->curno;
		else
			top = p->curno, end = no;

		p->seltop = top;
		p->selend = end;
	}
}

/** 挿入先位置調整 */

static int _data_adjust_insert(_data *p,int no)
{
	/* カーソルor選択内、
	 * カーソルor選択終端の次の位置の場合はなし */

	if(_data_is_in_cursel(p, no)
		|| (p->seltop != -1 && p->selend + 1 == no)
		|| (p->seltop == -1 && p->curno + 1 == no))
		return -1;
	else
		return no;
}

/** 追加 */

static mBool _data_append(_data *p,int num)
{
	int last;

	last = p->palnum;

	//追加数調整

	if(last + num > COLORPALETTEDAT_MAXNUM)
		num = COLORPALETTEDAT_MAXNUM - last;

	if(num == 0) return FALSE;

	//追加＆クリア

	p->palnum += num;

	memset(p->palbuf + last * 3, 255, num * 3);

	return TRUE;
}

/** 挿入 (カーソル位置からｎ個) */

static mBool _data_insert(_data *p,int num)
{
	int last,i;
	uint8_t *p1,*p2;

	last = p->palnum;

	//追加数調整

	if(last + num > COLORPALETTEDAT_MAXNUM)
		num = COLORPALETTEDAT_MAXNUM - last;

	if(num == 0) return FALSE;

	//追加

	p->palnum += num;

	//挿入位置を空ける

	p1 = p->palbuf + (p->palnum - 1) * 3;
	p2 = p->palbuf + (last - 1) * 3;

	for(i = last - p->curno; i >= 0; i--)
	{
		p1[0] = p2[0];
		p1[1] = p2[1];
		p1[2] = p2[2];
	
		p1 -= 3;
		p2 -= 3;
	}

	//新規分をクリア

	memset(p->palbuf + p->curno * 3, 255, num * 3);

	//

	p->curno += num;
	p->seltop = -1;

	return TRUE;
}

/** 削除 */

static mBool _data_delete(_data *p)
{
	uint8_t *p1,*p2;
	int top,end,num,delnum;

	num = p->palnum;

	if(num == 1) return FALSE;

	_data_get_area(p, &top, &end);

	//すべて削除の場合一つ残す

	if(top == 0 && end == num - 1)
		top = 1;

	//詰める

	end++;
	delnum = end - top;

	p1 = p->palbuf + top * 3;
	p2 = p->palbuf + end * 3;

	for(; end < num; end++)
	{
		p1[0] = p2[0];
		p1[1] = p2[1];
		p1[2] = p2[2];
	
		p1 += 3;
		p2 += 3;
	}

	p->palnum -= delnum;

	//

	p->curno = (top >= p->palnum)? p->palnum - 1: top;
	p->seltop = -1;

	return TRUE;
}

/** グラデーション */

static mBool _data_gradation(_data *p)
{
	int top,end,i,j,len,diff[3];
	uint8_t *p1,*p2;
	double d;

	_data_get_area(p, &top, &end);

	//長さ

	len = end - top;
	if(len < 2) return FALSE;

	//RGB値の差

	p1 = p->palbuf + top * 3;
	p2 = p->palbuf + end * 3;

	for(i = 0; i < 3; i++)
		diff[i] = p2[i] - p1[i];

	//グラデーション

	p2 = p1 + 3;

	for(i = 1; i < len; i++, p2 += 3)
	{
		d = (double)i / len;
	
		for(j = 0; j < 3; j++)
			p2[j] = (int)(diff[j] * d + p1[j] + 0.5);
	}

	return TRUE;
}

/** D&D移動 */

static void _data_dnd_move(_data *p,int dstno)
{
	uint8_t *buf,*p1,*p2;
	int top,end,num;

	_data_get_area(p, &top, &end);

	num = end - top + 1;

	//コピー

	buf = (uint8_t *)mMalloc(num * 3, FALSE);
	if(!buf) return;

	memcpy(buf, p->palbuf + top * 3, num * 3);

	//削除

	_data_delete(p);

	//挿入

	if(dstno > top)
		dstno -= num;

	p->curno = dstno;

	_data_insert(p, num);

	//挿入位置に貼り付け

	p1 = p->palbuf + dstno * 3;
	p2 = buf;

	for(; num > 0; num--)
	{
		p1[0] = p2[0];
		p1[1] = p2[1];
		p1[2] = p2[2];
	
		p1 += 3;
		p2 += 3;
	}

	//

	mFree(buf);

	p->curno = dstno;
}


/***********************************
 * 操作エリア
 ***********************************/


/** スクロール情報セット */

static void _area_set_scroll(_palarea *p)
{
	int page;

	page = p->wg.h / _CELLH;
	if(page <= 0) page = 1;

	mScrollBarSetStatus(p->scr, 0, (p->pdat->palnum + p->xnum - 1) / p->xnum, page);
}

/** カーソル位置からパレット番号取得 */

static int _get_palno_at_pt(_palarea *p,int x,int y)
{
	int no;

	if(x < 0 || y < 0) return -1;

	x = x / _CELLW;
	y = y / _CELLH + p->scr->sb.pos;

	if(x >= p->xnum) return -1;

	no = x + y * p->xnum;

	if(no >= p->pdat->palnum) return -1;

	return no;
}

/** カーソル位置変更 */

static void _area_set_curpos(_palarea *p,int no)
{
	p->pdat->curno = no;
	p->pdat->seltop = -1;

	mWidgetUpdate(M_WIDGET(p));
}


//========================
// ハンドラ
//========================


/** 描画 */

static void _area_draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	_palarea *p = (_palarea *)wg;
	uint8_t *ps;
	int ix,h,x,y,no,num,seltop,selend,curno,dragno;
	mPixCol selcol;

	h = wg->h;

	//背景

	mPixbufFillBox(pixbuf, 0, 0, wg->w, h, mGraytoPix(0xaa));

	//パレット

	no = p->scr->sb.pos * p->xnum;
	ps = p->pdat->palbuf + no * 3;
	num = p->pdat->palnum;

	curno = p->pdat->curno;
	seltop = p->pdat->seltop;
	selend = p->pdat->selend;
	dragno = (p->fdrag == _DRAGF_DRAG)? p->dragno: -1;

	selcol = mRGBtoPix(_COL_SEL);

	for(y = 0; y < h; y += _CELLH)
	{
		for(ix = p->xnum, x = 0; ix > 0; ix--, x += _CELLW, ps += 3, no++)
		{
			if(no >= num)
			{
				//挿入先が終端
				
				if(no == dragno)
					mPixbufLineV(pixbuf, x, y, _CELLH, mRGBtoPix(_COL_INSERT));

				return;
			}

			//選択背景

			if(seltop != -1 && seltop <= no && no <= selend)
				mPixbufFillBox(pixbuf, x, y, _CELLW, _CELLH, selcol);

			//カーソル枠

			if(no == curno)
				mPixbufBox(pixbuf, x + 1, y + 1, _CELLW - 2, _CELLH - 2, mRGBtoPix(_COL_CURFRAME));

			//ドラッグ挿入先

			if(no == dragno)
				mPixbufLineV(pixbuf, x, y, _CELLH, mRGBtoPix(_COL_INSERT));

			//枠

			mPixbufBox(pixbuf, x + 2, y + 2, _CELLW - 4, _CELLH - 4, 0);

			//色

			mPixbufFillBox(pixbuf, x + 3, y + 3, _CELLW - 6, _CELLH - 6,
				mRGBtoPix2(ps[0], ps[1], ps[2]));
		}
	}
}

/** ボタン押し時 */

static void _area_on_press(_palarea *p,mEvent *ev)
{
	int no;

	no = _get_palno_at_pt(p, ev->pt.x, ev->pt.y);
	if(no == -1) return;

	if(ev->pt.state & M_MODS_SHIFT)
	{
		//+Shift: 選択

		_data_select(p->pdat, no);
		mWidgetUpdate(M_WIDGET(p));
	}
	else
	{
		//カーソル位置変更

		if(!_data_is_in_cursel(p->pdat, no))
			_area_set_curpos(p, no);

		//D&D判定開始

		p->fdrag = _DRAGF_PRESS;
		p->dragno = no;

		mWidgetGrabPointer(M_WIDGET(p));
	}
}

/** 移動時 */

static void _area_on_motion(_palarea *p,int x,int y)
{
	int no;

	no = _get_palno_at_pt(p, x, y);

	//D&D開始の判定中
	/* 押し時の位置からはずれたらD&Dへ */

	if(p->fdrag == _DRAGF_PRESS && no != p->dragno)
	{
		p->fdrag = _DRAGF_DRAG;
		p->dragno = -1;
	}

	//D&D中

	if(p->fdrag == _DRAGF_DRAG)
	{
		//範囲外は終端へ
		
		if(no == -1) no = p->pdat->palnum;

		//挿入先調整

		no = _data_adjust_insert(p->pdat, no);

		//挿入先変更
		
		if(no != p->dragno)
		{
			p->dragno = no;
			mWidgetUpdate(M_WIDGET(p));
		}
	}
}

/** グラブ解放 */

static void _area_ungrab(_palarea *p)
{
	if(p->fdrag)
	{
		if(p->fdrag == _DRAGF_PRESS)
		{
			//選択内を押した場合はカーソル位置が変わっていないので変更

			if(p->pdat->curno != p->dragno)
				_area_set_curpos(p, p->dragno);
		}
		else if(p->fdrag == _DRAGF_DRAG)
		{
			//D&D

			if(p->dragno != -1)
			{
				_data_dnd_move(p->pdat, p->dragno);
				mWidgetUpdate(M_WIDGET(p));
			}
		}

		//
	
		p->fdrag = 0;
		mWidgetUngrabPointer(M_WIDGET(p));
	}
}

/** イベントハンドラ */

static int _area_event_handle(mWidget *wg,mEvent *ev)
{
	_palarea *p = (_palarea *)wg;

	switch(ev->type)
	{
		case MEVENT_POINTER:
			if(ev->pt.type == MEVENT_POINTER_TYPE_MOTION)
			{
				if(p->fdrag)
					_area_on_motion(p, ev->pt.x, ev->pt.y);
			}
			else if(ev->pt.type == MEVENT_POINTER_TYPE_PRESS)
			{
				if(ev->pt.btt == M_BTT_LEFT && p->fdrag == 0)
					_area_on_press(p, ev);
			}
			else if(ev->pt.type == MEVENT_POINTER_TYPE_RELEASE)
			{
				if(ev->pt.btt == M_BTT_LEFT)
					_area_ungrab(p);
			}
			break;

		case MEVENT_SCROLL:
			if(ev->scr.dir == MEVENT_SCROLL_DIR_UP
				|| ev->scr.dir == MEVENT_SCROLL_DIR_DOWN)
			{
				if(mScrollBarMovePos(p->scr,
					(ev->scr.dir == MEVENT_SCROLL_DIR_UP)? -3: 3))
					mWidgetUpdate(wg);
			}
			break;

		case MEVENT_FOCUS:
			if(ev->focus.bOut)
				_area_ungrab(p);
			break;
	}

	return 1;
}

/** サイズ変更時 */

static void _area_on_size(mWidget *wg)
{
	_palarea *p = (_palarea *)wg;

	p->xnum = wg->w / _CELLW;
	if(p->xnum < 1) p->xnum = 1;

	_area_set_scroll(p);
}

/** 作成 */

static _palarea *_create_palarea(mWidget *parent)
{
	_palarea *p;

	p = (_palarea *)mWidgetNew(sizeof(_palarea), parent);

	p->wg.fLayout = MLF_EXPAND_WH;
	p->wg.fEventFilter |= MWIDGET_EVENTFILTER_POINTER | MWIDGET_EVENTFILTER_SCROLL;
	p->wg.event = _area_event_handle;
	p->wg.draw = _area_draw_handle;
	p->wg.onSize = _area_on_size;
	p->wg.initW = _CELLW * 16;
	p->wg.initH = _CELLH * 16;

	return p;
}


/***********************************
 * ダイアログ
 ***********************************/


//========================
// コマンド
//========================


/** 編集後のエリア更新 */

static void _update_area(_dialog *p)
{
	_area_set_scroll(p->area);

	mWidgetUpdate(M_WIDGET(p->area));
}

/** 追加、挿入 */

static void _cmd_add_insert(_dialog *p,mBool insert)
{
	int num;
	mBool ret;

	num = mLineEditGetNum(p->edit_num);

	if(insert)
		ret = _data_insert(&p->dat, num);
	else
		ret = _data_append(&p->dat, num);

	if(ret)
		_update_area(p);
}

/** RGB値セット */

static void _cmd_set_rgb(_dialog *p)
{
	mStr str = MSTR_INIT;
	int c[3] = {0,0,0};
	uint8_t *pd;

	//値取得

	mLineEditGetTextStr(p->edit_rgb, &str);

	mStrToIntArray_range(&str, c, 3, 0, 0, 255);

	mStrFree(&str);

	//エディットクリア

	mLineEditSetText(p->edit_rgb, NULL);

	//セット

	pd = p->dat.palbuf + p->dat.curno * 3;

	pd[0] = c[0];
	pd[1] = c[1];
	pd[2] = c[2];

	//カーソルを進める

	if(p->dat.curno < p->dat.palnum - 1)
		p->dat.curno++;

	mWidgetUpdate(M_WIDGET(p->area));
}

/** OK 時、パレットデータセット */

static void _set_palette(_data *p)
{
	ColorPaletteDat *colpal;

	colpal = APP_COLPAL->pal + APP_DRAW->col.colpal_sel;

	//数が異なるなら再確保

	if(p->palnum != colpal->num)
	{
		if(!ColorPaletteDat_alloc(colpal, p->palnum))
			return;
	}

	//データコピー

	memcpy(colpal->buf, p->palbuf, p->palnum * 3);

	APP_COLPAL->exit_save = TRUE;
}


//========================
//
//========================


/** イベントハンドラ */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY)
	{
		_dialog *p = (_dialog *)wg;
	
		switch(ev->notify.id)
		{
			//スクロール
			case WID_SCROLL:
				if(ev->notify.param2 & MSCROLLBAR_N_HANDLE_F_CHANGE)
					mWidgetUpdate((mWidget *)p->area);
				break;
			//削除
			case WID_BTT_DELETE:
				if(_data_delete(&p->dat))
					_update_area(p);
				break;
			//グラデーション
			case WID_BTT_GRADATION:
				if(_data_gradation(&p->dat))
					_update_area(p);
				break;
			//追加
			case WID_BTT_ADD:
				_cmd_add_insert(p, FALSE);
				break;
			//挿入
			case WID_BTT_INSERT:
				_cmd_add_insert(p, TRUE);
				break;

			//RGB値セット
			case WID_EDIT_RGB:
				if(ev->notify.type == MLINEEDIT_N_ENTER)
					_cmd_set_rgb(p);
				break;
		}
	}
	
	return mDialogEventHandle_okcancel(wg, ev);
}

/** 破棄ハンドラ */

static void _destroy_handle(mWidget *wg)
{
	mFree(((_dialog *)wg)->dat.palbuf);
}

/** 作成 */

static _dialog *_create_dlg(mWindow *owner)
{
	_dialog *p;
	mWidget *ct,*ct2,*cttop;
	mLineEdit *le;
	ColorPaletteDat *colpal;

	//作成
	
	p = (_dialog *)mDialogNew(sizeof(_dialog), owner, MWINDOW_S_DIALOG_NORMAL);
	if(!p) return NULL;

	p->wg.event = _event_handle;
	p->wg.destroy = _destroy_handle;

	//編集用バッファ作成

	p->dat.palbuf = (uint8_t *)mMalloc(COLORPALETTEDAT_MAXNUM * 3, FALSE);
	if(!p->dat.palbuf)
	{
		mWidgetDestroy(M_WIDGET(p));
		return NULL;
	}

	colpal = APP_COLPAL->pal + APP_DRAW->col.colpal_sel;

	p->dat.palnum = colpal->num;

	memcpy(p->dat.palbuf, colpal->buf, colpal->num * 3);

	//

	M_TR_G(TRGROUP_DLG_PALETTE_EDIT);

	mContainerSetPadding_one(M_CONTAINER(p), 8);
	p->ct.sepW = 15;

	mWindowSetTitle(M_WINDOW(p), M_TR_T(TRID_TITLE));

	//=========== ウィジェット

	cttop = mContainerCreate(M_WIDGET(p), MCONTAINER_TYPE_HORZ, 0, 0, MLF_EXPAND_WH);

	//----- パレット表示

	ct = mContainerCreate(cttop, MCONTAINER_TYPE_HORZ, 0, 0, MLF_EXPAND_WH);

	//エリア

	p->area = _create_palarea(ct);
	p->area->pdat = &p->dat;

	//スクロール

	p->scr = mScrollBarNew(0, ct, MSCROLLBAR_S_VERT);
	p->scr->wg.id = WID_SCROLL;
	p->scr->wg.fLayout = MLF_EXPAND_H;

	p->area->scr = p->scr;

	//------ 右側

	ct = mContainerCreate(cttop, MCONTAINER_TYPE_VERT, 0, 2, 0);
	ct->margin.left = 15;

	//ボタン

	mButtonCreate(ct, WID_BTT_DELETE, 0, MLF_EXPAND_W, 0, M_TR_T(TRID_DELETE));
	mButtonCreate(ct, WID_BTT_GRADATION, 0, MLF_EXPAND_W, 0, M_TR_T(TRID_GRADATION));

	//個数

	ct2 = mContainerCreate(ct, MCONTAINER_TYPE_HORZ, 0, 6, 0);
	ct2->margin.top = 13;

	mLabelCreate(ct2, 0, MLF_MIDDLE, 0, M_TR_T(TRID_NUM));

	p->edit_num = le = mLineEditCreate(ct2, WID_EDIT_NUM, MLINEEDIT_S_SPIN, 0, 0);
	mLineEditSetNumStatus(le, 1, COLORPALETTEDAT_MAXNUM, 0);
	mLineEditSetNum(le, 1);
	mLineEditSetWidthByLen(le, 5);

	//追加、挿入

	mButtonCreate(ct, WID_BTT_ADD, 0, MLF_EXPAND_W, M_MAKE_DW4(0,6,0,0), M_TR_T(TRID_ADD));
	mButtonCreate(ct, WID_BTT_INSERT, 0, MLF_EXPAND_W, 0, M_TR_T(TRID_INSERT));

	//RGB

	mLabelCreate(ct, 0, 0, M_MAKE_DW4(0,15,0,0), M_TR_T(TRID_RGB_LABEL));

	p->edit_rgb = mLineEditCreate(ct, WID_EDIT_RGB,  MLINEEDIT_S_NOTIFY_ENTER, MLF_EXPAND_W, 0);

	//------ 下部分

	ct = mContainerCreate(M_WIDGET(p), MCONTAINER_TYPE_HORZ, 0, 0, MLF_EXPAND_W);

	//ヘルプ

	mLabelCreate(ct, 0, MLF_EXPAND_X, M_MAKE_DW4(0,0,15,0), M_TR_T(TRID_HELP));

	//OK,CANCEL

	M_TR_G(M_TRGROUP_SYS);
	
	mButtonCreate(ct, M_WID_OK, 0, MLF_BOTTOM, M_MAKE_DW4(0,0,2,0), M_TR_T(M_TRSYS_OK));
	mButtonCreate(ct, M_WID_CANCEL, 0, MLF_BOTTOM, 0, M_TR_T(M_TRSYS_CANCEL));
	
	return p;
}

/** 編集ダイアログ実行 */

mBool DockColorPalette_runEditPalette(mWindow *owner)
{
	_dialog *p;
	int ret;

	p = _create_dlg(owner);
	if(!p) return FALSE;

	mWindowMoveResizeShow_hintSize(M_WINDOW(p));

	ret = mDialogRun(M_DIALOG(p), FALSE);

	if(ret)
		_set_palette(&p->dat);

	mWidgetDestroy(M_WIDGET(p));

	return ret;
}
