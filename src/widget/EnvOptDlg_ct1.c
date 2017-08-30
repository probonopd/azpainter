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
 * 環境設定ダイアログ 内容
 *****************************************/

#include <stdio.h>

#include "mDef.h"
#include "mGui.h"
#include "mWidget.h"
#include "mContainer.h"
#include "mLabel.h"
#include "mCheckButton.h"
#include "mColorButton.h"
#include "mLineEdit.h"
#include "mButton.h"
#include "mComboBox.h"
#include "mGroupBox.h"
#include "mFontButton.h"
#include "mTrans.h"
#include "mEvent.h"
#include "mStr.h"
#include "mSysDialog.h"
#include "mMessageBox.h"

#include "defMacros.h"
#include "defConfig.h"

#include "PrevCursor.h"
#include "ImageBuf24.h"

#include "trgroup.h"

#include "EnvOptDlg_pv.h"


//----------------------

mBool ToolBarCustomizeDlg_run(mWindow *owner,uint8_t **ppdst,int *pdstsize);

//----------------------



//=============================
// 設定1
//=============================


enum
{
	OPT1_EDIT_INIT_W,
	OPT1_EDIT_INIT_H,
	OPT1_EDIT_INIT_DPI,
	OPT1_EDIT_UNDO_NUM,
	OPT1_EDIT_UNDO_BUFSIZE,

	OPT1_EDIT_NUM
};

typedef struct
{
	mWidget wg;
	mContainerData ct;

	mLineEdit *edit[OPT1_EDIT_NUM];
	mColorButton *colbtt[3];
}_ct_opt1;

#define OPT1_WID_UNDO_BUFSIZE_HELP  100


/** アンドゥバッファサイズ取得 */

static int _opt1_get_bufsize(_ct_opt1 *p)
{
	mStr str = MSTR_INIT;
	double d;
	char c;
	int ret;

	//double 値取得

	d = mLineEditGetDouble(p->edit[OPT1_EDIT_UNDO_BUFSIZE]);
	if(d < 0) d = 0;

	//末尾の単位

	mLineEditGetTextStr(p->edit[OPT1_EDIT_UNDO_BUFSIZE], &str);

	c = mStrGetLastChar(&str);

	if(c == 'K' || c == 'k')
		d *= 1024;
	else if(c == 'M' || c == 'm')
		d *= 1024 * 1024;
	else if(c == 'G' || c == 'g')
		d *= 1024 * 1024 * 1024;

	//最大1G

	if(d > 1024 * 1024 * 1024)
		d = 1024 * 1024 * 1024;
	
	ret = (int)(d + 0.5);

	mStrFree(&str);

	return ret;
}

/** アンドゥバッファサイズ、単位付きのテキストセット */

static void _opt1_set_undobufsize(mLineEdit *le,int size)
{
	mStr str = MSTR_INIT;

	if(size < 1024)
		mStrSetInt(&str, size);
	else if(size < 1024 * 1024)
		mStrSetFormat(&str, "%.2FK", size * 100 / 1024);
	else if(size < 1024 * 1024 * 1024)
		mStrSetFormat(&str, "%.2FM", (int)((double)size / (1024 * 1024) * 100 + 0.5));
	else
		mStrSetFormat(&str, "%.2FG", (int)((double)size / (1024 * 1024 * 1024) * 100 + 0.5));

	mLineEditSetText(le, str.buf);

	mStrFree(&str);
}

/** 値取得 */

static void _opt1_getval(mWidget *wg,EnvOptDlgCurData *pd)
{
	_ct_opt1 *p = (_ct_opt1 *)wg;
	int i;

	for(i = 0; i < 3; i++)
		pd->canvcol[i] = mColorButtonGetColor(p->colbtt[i]);

	pd->initw = mLineEditGetNum(p->edit[0]);
	pd->inith = mLineEditGetNum(p->edit[1]);
	pd->init_dpi = mLineEditGetNum(p->edit[2]);
	pd->undo_maxnum = mLineEditGetNum(p->edit[3]);
	pd->undo_maxbufsize = _opt1_get_bufsize(p);
}

/** イベント */

static int _opt1_event(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY
		&& ev->notify.id == OPT1_WID_UNDO_BUFSIZE_HELP)
	{
		//アンドゥバッファヘルプ
		
		mMessageBox(wg->toplevel, "help",
			M_TR_T2(TRGROUP_DLG_ENV_OPTION, TRID_MES_UNDOBUFSIZE), MMESBOX_OK, MMESBOX_OK);
	}

	return 1;
}

/** 作成 */

mWidget *EnvOptDlg_create_option1(mWidget *parent,EnvOptDlgCurData *dat)
{
	_ct_opt1 *p;
	mWidget *ct;
	mLineEdit *le;
	int i,j,no,min,max,val,id;
	uint16_t trid[7] = {
		TRID_OPT1_INIT_W, TRID_OPT1_INIT_H, TRID_OPT1_INIT_DPI,
		TRID_OPT1_BKGND_CANVAS, TRID_OPT1_BKGND_PLAID,
		TRID_OPT1_UNDO_NUM, TRID_OPT1_UNDO_BUFSIZE
	};

	p = (_ct_opt1 *)EnvOptDlg_createContentsBase(parent, sizeof(_ct_opt1), _opt1_getval);

	p->wg.event = _opt1_event;

	mContainerSetTypeGrid(M_CONTAINER(p), 2, 6, 7);

	//

	for(i = 0; i < 7; i++)
	{
		id = trid[i];
		
		//ラベル
		
		mLabelCreate(M_WIDGET(p), 0, MLF_MIDDLE, 0, M_TR_T(id));

		//---- 値

		if(id == TRID_OPT1_BKGND_CANVAS)
		{
			//キャンバス背景色

			p->colbtt[0] = mColorButtonCreate(M_WIDGET(p), 0, MCOLORBUTTON_S_DIALOG,
				MLF_MIDDLE, 0, dat->canvcol[0]);
		}
		else if(id == TRID_OPT1_BKGND_PLAID)
		{
			//チェック柄背景色

			ct = mContainerCreate(M_WIDGET(p), MCONTAINER_TYPE_HORZ, 0, 5, 0);

			for(j = 0; j < 2; j++)
			{
				p->colbtt[1 + j] = mColorButtonCreate(ct, 0, MCOLORBUTTON_S_DIALOG,
					MLF_MIDDLE, 0, dat->canvcol[1 + j]);
			}
		}
		else if(id == TRID_OPT1_UNDO_BUFSIZE)
		{
			//アンドゥバッファサイズ

			ct = mContainerCreate(M_WIDGET(p), MCONTAINER_TYPE_HORZ, 0, 5, 0);

			le = p->edit[OPT1_EDIT_UNDO_BUFSIZE] = mLineEditCreate(ct, 0, 0, 0, 0);

			mLineEditSetWidthByLen(le, 10);

			_opt1_set_undobufsize(le, dat->undo_maxbufsize);

			mButtonCreate(ct, OPT1_WID_UNDO_BUFSIZE_HELP, MBUTTON_S_REAL_W, 0, 0, "?");
		}
		else
		{
			//数値エディット

			no = (id == TRID_OPT1_UNDO_NUM)? OPT1_EDIT_UNDO_NUM: i;

			p->edit[no] = le = mLineEditCreate(M_WIDGET(p), 0, MLINEEDIT_S_SPIN, 0, 0);

			if(id == TRID_OPT1_INIT_DPI)
				//起動時DPI
				min = 1, max = 9999, val = dat->init_dpi;
			else if(id == TRID_OPT1_UNDO_NUM)
				//アンドゥ最大回数
				min = 2, max = 400, val = dat->undo_maxnum;
			else
			{
				//起動時のイメージ幅/高さ
				min = 1, max = IMAGE_SIZE_MAX;
				val = (no == 0)? dat->initw: dat->inith;
			}

			mLineEditSetWidthByLen(le, 8);
			mLineEditSetNumStatus(le, min, max, 0);
			mLineEditSetNum(le, val);
		}
	}

	return (mWidget *)p;
}


//=============================
// 設定2
//=============================


#define OPT2_CHECK_NUM  4

typedef struct
{
	mWidget wg;
	mContainerData ct;

	mCheckButton *ck[OPT2_CHECK_NUM];
}_ct_opt2;


/** 値取得 */

static void _opt2_getval(mWidget *wg,EnvOptDlgCurData *pd)
{
	_ct_opt2 *p = (_ct_opt2 *)wg;
	int i;
	uint32_t flags = 0;

	for(i = 0; i < OPT2_CHECK_NUM; i++)
	{
		if(mCheckButtonIsChecked(p->ck[i]))
			flags |= 1 << i;
	}

	pd->optflags = flags;
}

/** 作成 */

mWidget *EnvOptDlg_create_option2(mWidget *parent,EnvOptDlgCurData *dat)
{
	_ct_opt2 *p;
	int i;
	uint32_t flags = dat->optflags;

	p = (_ct_opt2 *)EnvOptDlg_createContentsBase(parent, sizeof(_ct_opt2), _opt2_getval);

	for(i = 0; i < OPT2_CHECK_NUM; i++)
	{
		p->ck[i] = mCheckButtonCreate(M_WIDGET(p), 0, 0, 0, 0,
			M_TR_T(TRID_OPT2_TOP + i), flags & (1 << i));
	}

	return (mWidget *)p;
}


//=============================
// 増減幅
//=============================


typedef struct
{
	mWidget wg;
	mContainerData ct;

	mLineEdit *edit[4];
}_ct_step;


/** 値取得 */

static void _step_getval(mWidget *wg,EnvOptDlgCurData *pd)
{
	_ct_step *p = (_ct_step *)wg;
	int i;

	for(i = 0; i < 4; i++)
		pd->step[i] = mLineEditGetNum(p->edit[i]);
}

/** 作成 */

mWidget *EnvOptDlg_create_step(mWidget *parent,EnvOptDlgCurData *dat)
{
	_ct_step *p;
	mLineEdit *le;
	int i,max[4] = { 100, 1000, 180, 1000 };

	p = (_ct_step *)EnvOptDlg_createContentsBase(parent, sizeof(_ct_step), _step_getval);

	mContainerSetTypeGrid(M_CONTAINER(p), 2, 6, 7);

	for(i = 0; i < 4; i++)
	{
		//ラベル
		
		mLabelCreate(M_WIDGET(p), 0, MLF_MIDDLE, 0, M_TR_T(TRID_STEP_TOP + i));

		//エディット

		le = p->edit[i] = mLineEditCreate(M_WIDGET(p), 0, MLINEEDIT_S_SPIN, 0, 0);

		mLineEditSetWidthByLen(le, 7);
		mLineEditSetNumStatus(le, 1, max[i], (i == 3)? 1: 0);
		mLineEditSetNum(le, dat->step[i]);
	}

	return (mWidget *)p;
}


//=============================
// インターフェイス
//=============================


typedef struct
{
	mWidget wg;
	mContainerData ct;

	mFontButton *fontbtt[2];
	mComboBox *cb_icon[3];
	mLineEdit *edit_theme;

	EnvOptDlgCurData *dat;
}_ct_interface;

enum
{
	INTERFACE_WID_TOOLBAR_CUSTOM = 100,
	INTERFACE_WID_THEME_BTT
};


/** 値取得 */

static void _interface_getval(mWidget *wg,EnvOptDlgCurData *pd)
{
	_ct_interface *p = (_ct_interface *)wg;
	int i;

	//フォント

	for(i = 0; i < 2; i++)
		mFontButtonGetInfoFormat_str(p->fontbtt[i], pd->strFontStyle + i);

	//テーマ

	mLineEditGetTextStr(p->edit_theme, &pd->strThemeFile);

	//アイコン

	for(i = 0; i < 3; i++)
		pd->iconsize[i] = mComboBoxGetItemParam(p->cb_icon[i], -1);
}

/** テーマファイル参照 */

static void _interface_theme_openfile(_ct_interface *p)
{
	mStr str = MSTR_INIT,sysdir = MSTR_INIT;

	mAppGetDataPath(&sysdir, "theme");

	if(mSysDlgOpenFile(p->wg.toplevel, "All files\t*", 0, sysdir.buf, 0, &str))
	{
		if(mStrPathCompareDir(&str, sysdir.buf))
		{
			mStrPathGetFileName(&sysdir, str.buf);
			mStrSetText(&str, "!/theme/");
			mStrAppendStr(&str, &sysdir);
		}
		
		mLineEditSetText(p->edit_theme, str.buf);
	}

	mStrFree(&str);
	mStrFree(&sysdir);
}

/** イベント */

static int _interface_event(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY)
	{
		_ct_interface *p = (_ct_interface *)wg;
	
		switch(ev->notify.id)
		{
			//ツールバーカスタマイズ
			case INTERFACE_WID_TOOLBAR_CUSTOM:
				if(ToolBarCustomizeDlg_run(wg->toplevel, &p->dat->toolbar_btts, &p->dat->toolbar_btts_size))
					p->dat->fchange |= ENVOPTDLG_CHANGE_F_TOOLBAR_BTTS;
				break;

			//テーマ参照
			case INTERFACE_WID_THEME_BTT:
				_interface_theme_openfile(p);
				break;
		}
	}

	return 1;
}

/** 作成 */

mWidget *EnvOptDlg_create_interface(mWidget *parent,EnvOptDlgCurData *dat)
{
	_ct_interface *p;
	mWidget *ct;
	mComboBox *cb;
	int i,j,n;
	char m[16];
	uint8_t iconsize[3][4] = {
		{16,20,24,0}, {13,16,0}, {16,20,24,0}
	};

	p = (_ct_interface *)EnvOptDlg_createContentsBase(parent, sizeof(_ct_interface), _interface_getval);

	p->wg.fLayout = MLF_EXPAND_W;
	p->wg.event = _interface_event;
	p->ct.sepW = 10;

	p->dat = dat;

	//----- フォント

	ct = (mWidget *)mGroupBoxCreate(M_WIDGET(p), 0, MLF_EXPAND_W, 0, M_TR_T(TRID_INTERFACE_FONT));

	mContainerSetTypeGrid(M_CONTAINER(ct), 2, 6, 7);

	for(i = 0; i < 2; i++)
	{
		mLabelCreate(ct, 0, MLF_MIDDLE, 0, M_TR_T(TRID_INTERFACE_FONT_DEFAULT + i));

		p->fontbtt[i] = mFontButtonCreate(ct, 0, 0, MLF_EXPAND_W, 0);

		mFontButtonSetInfoFormat(p->fontbtt[i], dat->strFontStyle[i].buf);
	}

	//----- アイコン

	ct = (mWidget *)mGroupBoxCreate(M_WIDGET(p), 0, MLF_EXPAND_W, 0, M_TR_T(TRID_INTERFACE_ICON));

	mContainerSetTypeGrid(M_CONTAINER(ct), 2, 6, 7);

	for(i = 0; i < 3; i++)
	{
		//ラベル

		mLabelCreate(ct, 0, MLF_MIDDLE, 0, M_TR_T(TRID_INTERFACE_ICON_TOOLBAR + i));

		//combobox

		cb = p->cb_icon[i] = mComboBoxCreate(ct, 0, 0, MLF_MIDDLE, 0);

		for(j = 0; iconsize[i][j]; j++)
		{
			n = iconsize[i][j];
			snprintf(m, 16, "%dx%d", n, n);
			mComboBoxAddItem(cb, m, n);
		}

		mComboBoxSetWidthAuto(cb);
		mComboBoxSetSel_findParam_notfind(cb, dat->iconsize[i], 0);
	}

	//----- テーマ

	ct = mContainerCreate(M_WIDGET(p), MCONTAINER_TYPE_HORZ, 0, 6, MLF_EXPAND_W);

	mLabelCreate(ct, 0, MLF_MIDDLE, 0, M_TR_T(TRID_INTERFACE_THEME));

	//

	p->edit_theme = mLineEditCreate(ct, 0, 0, MLF_EXPAND_W | MLF_MIDDLE, 0);

	mLineEditSetText(p->edit_theme, dat->strThemeFile.buf);

	//

	mButtonCreate(ct, INTERFACE_WID_THEME_BTT, MBUTTON_S_REAL_W, MLF_MIDDLE, 0, "...");

	//----- ツールバーカスタマイズ

	mButtonCreate(M_WIDGET(p), INTERFACE_WID_TOOLBAR_CUSTOM,
		0, 0, 0, M_TR_T(TRID_INTERFACE_CUSTOMIZE_TOOLBAR));
		
	return (mWidget *)p;
}


//=============================
// システム
//=============================


typedef struct
{
	mWidget wg;
	mContainerData ct;

	mLineEdit *edit[3];
}_ct_system;

#define SYSTEM_WID_BTT_TOP 200


/** 値取得 */

static void _system_getval(mWidget *wg,EnvOptDlgCurData *pd)
{
	_ct_system *p = (_ct_system *)wg;
	int i;

	for(i = 0; i < 3; i++)
		mLineEditGetTextStr(p->edit[i], pd->strSysDir + i);
}

/** イベント */

static int _system_event(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY
		&& ev->notify.id >= SYSTEM_WID_BTT_TOP && ev->notify.id < SYSTEM_WID_BTT_TOP + 3)
	{
		//参照ボタン
		
		mStr str = MSTR_INIT;
		mLineEdit *edit = ((_ct_system *)wg)->edit[ev->notify.id - SYSTEM_WID_BTT_TOP];

		mLineEditGetTextStr(edit, &str);

		if(mSysDlgSelectDir(wg->toplevel, str.buf, 0, &str))
			mLineEditSetText(edit, str.buf);

		mStrFree(&str);
	}

	return 1;
}

/** 作成 */

mWidget *EnvOptDlg_create_system(mWidget *parent,EnvOptDlgCurData *dat)
{
	_ct_system *p;
	mWidget *ct;
	int i;

	p = (_ct_system *)EnvOptDlg_createContentsBase(parent, sizeof(_ct_system), _system_getval);

	p->wg.fLayout = MLF_EXPAND_W;
	p->wg.event = _system_event;
	p->ct.sepW = 5;

	for(i = 0; i < 3; i++)
	{
		mLabelCreate(M_WIDGET(p), 0, 0, 0, M_TR_T(TRID_SYSTEM_TEMPDIR + i));

		//

		ct = mContainerCreate(M_WIDGET(p), MCONTAINER_TYPE_HORZ, 0, 6, MLF_EXPAND_W);

		p->edit[i] = mLineEditCreate(ct, 0, 0, MLF_EXPAND_W | MLF_MIDDLE, 0);
		mLineEditSetText(p->edit[i], dat->strSysDir[i].buf);

		mButtonCreate(ct, SYSTEM_WID_BTT_TOP + i, MBUTTON_S_REAL_W, MLF_MIDDLE, 0, "..."); 
	}
	
	return (mWidget *)p;
}


//=============================
// カーソル
//=============================


typedef struct
{
	mWidget wg;
	mContainerData ct;

	PrevCursor *prev;
	mLineEdit *edit[2];

	EnvOptDlgCurData *dat;
}_ct_cursor;

enum
{
	CURSOR_WID_LOADIMG = 200,
	CURSOR_WID_DEFAULT
};


/** ImageBuf24 からカーソルデータ作成 */

static void _cursor_img_to_data(EnvOptDlgCurData *dat,ImageBuf24 *img)
{
	uint8_t *ps,*pd_col,*pd_mask,*pd_colY,*pd_maskY,f;
	int ix,iy,w,h,pitchd,pitchs,col;

	w = img->w, h = img->h;

	if(w > 32) w = 32;
	if(h > 32) h = 32;

	pitchd = (w + 7) >> 3;

	//確保

	mFree(dat->cursor_buf);

	dat->cursor_bufsize = 4 + pitchd * h * 2;

	dat->cursor_buf = (uint8_t *)mMalloc(dat->cursor_bufsize, TRUE);
	if(!dat->cursor_buf) return;

	//

	dat->fchange |= ENVOPTDLG_CHANGE_F_CURSOR;

	//幅、高さセット

	ps = dat->cursor_buf;

	ps[0] = w;
	ps[1] = h;

	//変換

	ps = img->buf;
	pd_colY = dat->cursor_buf + 4;
	pd_maskY = pd_colY + pitchd * h;

	pitchs = img->pitch - w * 3;

	for(iy = h; iy > 0; iy--)
	{
		pd_col = pd_colY;
		pd_mask = pd_maskY;
		f = 1;
	
		for(ix = w; ix > 0; ix--, ps += 3)
		{
			col = (ps[0] << 16) | (ps[1] << 8) | ps[0];

			if(col == 0 || col == 0xffffff)
			{
				if(col == 0) *pd_col |= f;
				*pd_mask |= f;
			}

			f <<= 1;
			if(f == 0) f = 1, pd_col++, pd_mask++;
		}

		ps += pitchs;
		pd_colY += pitchd;
		pd_maskY += pitchd;
	}
}

/** 画像読み込み */

static void _cursor_load_image(_ct_cursor *p)
{
	mStr str = MSTR_INIT;
	ImageBuf24 *img;

	if(mSysDlgOpenFile(p->wg.toplevel, FILEFILTER_NORMAL_IMAGE, 0, NULL, 0, &str))
	{
		img = ImageBuf24_loadFile(str.buf);
		if(img)
		{
			_cursor_img_to_data(p->dat, img);
		
			ImageBuf24_free(img);

			PrevCursor_setBuf(p->prev, p->dat->cursor_buf);
		}
	}

	mStrFree(&str);
}

/** イベント */

static int _cursor_event(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY)
	{
		_ct_cursor *p = (_ct_cursor *)wg;
	
		switch(ev->notify.id)
		{
			//画像読み込み
			case CURSOR_WID_LOADIMG:
				_cursor_load_image(p);
				break;

			//デフォルト
			case CURSOR_WID_DEFAULT:
				if(p->dat->cursor_buf)
				{
					mFree(p->dat->cursor_buf);
					p->dat->cursor_buf = NULL;
					p->dat->cursor_bufsize = 0;

					p->dat->fchange |= ENVOPTDLG_CHANGE_F_CURSOR;

					PrevCursor_setBuf(p->prev, NULL);
				}
				break;
		}
	}

	return 1;
}

/** 値取得 */

static void _cursor_getval(mWidget *wg,EnvOptDlgCurData *pd)
{
	_ct_cursor *p = (_ct_cursor *)wg;
	int x,y;
	uint8_t *buf;

	//ホットスポット位置

	buf = pd->cursor_buf;

	if(buf)
	{
		x = mLineEditGetNum(p->edit[0]);
		y = mLineEditGetNum(p->edit[1]);

		if(x >= buf[0]) x = buf[0] - 1;
		if(y >= buf[1]) y = buf[1] - 1;
		
		/* ホットスポット位置だけ変更した場合も
		 * データ変更とみなす。 */

		if(x != buf[2] || y != buf[3])
		{
			buf[2] = x;
			buf[3] = y;

			pd->fchange |= ENVOPTDLG_CHANGE_F_CURSOR;
		}
	}
}

/** 作成 */

mWidget *EnvOptDlg_create_cursor(mWidget *parent,EnvOptDlgCurData *dat)
{
	_ct_cursor *p;
	mWidget *ct;
	mLineEdit *le;
	char m[2] = {0,0};
	uint8_t *cursor_buf;
	int i;

	p = (_ct_cursor *)EnvOptDlg_createContentsBase(parent, sizeof(_ct_cursor), _cursor_getval);

	p->wg.event = _cursor_event;
	p->ct.sepW = 8;
	p->dat = dat;

	cursor_buf = dat->cursor_buf;  //NULL の場合あり

	//プレビュー

	p->prev = PrevCursor_new(M_WIDGET(p));

	PrevCursor_setBuf(p->prev, cursor_buf);

	//ボタン

	ct = mContainerCreate(M_WIDGET(p), MCONTAINER_TYPE_HORZ, 0, 5, 0);

	mButtonCreate(ct, CURSOR_WID_LOADIMG, 0, 0, 0, M_TR_T(TRID_CURSOR_LOAD));
	mButtonCreate(ct, CURSOR_WID_DEFAULT, 0, 0, 0, M_TR_T(TRID_CURSOR_DEFAULT));

	//ホットスポット位置

	mLabelCreate(M_WIDGET(p), 0, 0, M_MAKE_DW4(0,6,0,0), M_TR_T(TRID_CURSOR_HOTSPOT));

	ct = mContainerCreate(M_WIDGET(p), MCONTAINER_TYPE_HORZ, 0, 7, 0);

	for(i = 0; i < 2; i++)
	{
		m[0] = 'X' + i;
		mLabelCreate(ct, 0, MLF_MIDDLE, 0, m);

		le = p->edit[i] = mLineEditCreate(ct, 0, MLINEEDIT_S_SPIN, 0, 0);

		mLineEditSetWidthByLen(le, 4);
		mLineEditSetNumStatus(le, 0, 31, 0);
		mLineEditSetNum(le, (cursor_buf)? cursor_buf[2 + i]: 0);
	}

	//ヘルプ

	mLabelCreate(M_WIDGET(p), MLABEL_S_BORDER, 0, M_MAKE_DW4(0,6,0,0), M_TR_T(TRID_CURSOR_HELP));

	return (mWidget *)p;
}
