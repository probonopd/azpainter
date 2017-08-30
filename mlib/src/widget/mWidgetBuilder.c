/*$
Copyright (c) 2017, Azel
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERSAND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
$*/

/*****************************************
 * mWidgetBuilder
 *****************************************/

/*
 * ウィジェット名[#スタイル[,...]]:パラメータ名=パラメータ:...;
 *
 * スペース・タブはスキップする。
 * [ウィジェット名] "-" で親へ戻る ("-" の数分)
 */


#include <stdlib.h>
#include <string.h>

#include "mDef.h"

#include "mStr.h"
#include "mContainerDef.h"

#include "mWidget.h"
#include "mContainer.h"
#include "mLabel.h"
#include "mButton.h"
#include "mCheckButton.h"
#include "mColorButton.h"
#include "mComboBox.h"
#include "mGroupBox.h"
#include "mLineEdit.h"
#include "mListView.h"
#include "mInputAccelKey.h"
#include "mFontButton.h"
#include "mArrowButton.h"
#include "mTab.h"
#include "mSliderBar.h"

#include "mTrans.h"
#include "mUtilStr.h"


//-----------------------

typedef struct
{
	const char *pnext,*ptop,*pend,*pcur;
	mWidget *curparent,
		*curwidget;
	int wgtype;
	mStr strParam;
}mWidgetBuilder;

//-----------------------

enum
{
	_TYPE_UNKNOWN = -1,
	_TYPE_CONTAINER,
	_TYPE_LABEL,
	_TYPE_BUTTON,
	_TYPE_SPACER,
	_TYPE_CHECKBUTTON,
	_TYPE_COMBOBOX,
	_TYPE_GROUPBOX,
	_TYPE_LINEEDIT,
	_TYPE_LISTVIEW,
	_TYPE_INPUTACCELKEY,
	_TYPE_COLORBUTTON,
	_TYPE_ARROWBUTTON,
	_TYPE_FONTBUTTON,
	_TYPE_TAB,
	_TYPE_SLIDER
};

static const char *g_widgetname =
	"ct;lb;bt;sp;ck;cb;gb;le;lv;iak;colbt;arrbt;fontbt;tab;sl";

//-----------------------

enum
{
	_PARAM_ID,
	_PARAM_TEXT,
	_PARAM_TEXT_TR,
	_PARAM_TEXT_TRG,
	_PARAM_LAYOUT_FLAG,
	_PARAM_MARGIN,
	_PARAM_PADDING,
	_PARAM_CONTAINER_SEPW,
	_PARAM_INITSIZE,
	_PARAM_WIDTH_LEN,
	_PARAM_CONTAINER_STYLE,
	_PARAM_RANGE
};

static const char *g_paramname =
	"id;t;tr;TR;lf;mg;pd;sep;init;wlen;cts;range";

//-----------------------



//============================
// sub
//============================


/** 一行範囲取得 */

static mBool _getline(mWidgetBuilder *p)
{
	const char *pc,*pend;

	pc = p->pnext;

	do
	{
		//先頭の空白はスキップ

		for(; *pc && (*pc == ' ' || *pc == '\t'); pc++);

		//終了

		if(!(*pc)) return FALSE;

		//終端位置

		for(pend = pc; *pend && *pend != ';'; pend++);

		if(pc != pend) break;

		//空の場合は次へ

		pc = pend + 1;
		
	} while(1);

	//

	p->ptop = p->pcur = pc;
	p->pend = pend;
	p->pnext = (*pend)? pend + 1: pend;

	return TRUE;
}

/** 次のパラメータ値取得 (strParam に) */

static mBool _getNextParam(mWidgetBuilder *p)
{
	const char *pc,*pend;

	pc   = p->pcur;
	pend = p->pend;

	//終了

	if(pc == pend) return FALSE;

	//取得

	for(; pc != pend && *pc != ':'; pc++);

	mStrSetTextLen(&p->strParam, p->pcur, pc - p->pcur);

	//次の位置

	p->pcur = (pc == pend)? pc: pc + 1;

	return TRUE;
}

/** パラメータ文字列を指定文字で分離
 *
 * @return 分割文字の次の位置 (文字がなければ終端) */

static char *_splitParam(mWidgetBuilder *p,char split)
{
	char *pc,*pc2;

	pc = p->strParam.buf;

	pc2 = strchr(pc, split);

	if(pc2)
		*(pc2++) = 0;
	else
		pc2 = pc + p->strParam.len;

	return pc2;
}

/** カンマで区切られた値を取得
 *
 * @return 値の個数 */

static int _getParamNums(char *param,int *dst,int maxnum)
{
	int num = 0;

	for(; *param && num < maxnum; num++)
	{
		*(dst++) = atoi(param);

		for(; *param && *param != ','; param++);

		if(*param == ',') param++;
	}

	return num;
}

/** 文字の列挙からスタイル値を取得 */

static uint32_t _getStyleNum(char *param,const char *chars)
{
	uint32_t s = 0,f;
	const char *pc;
	char c;

	while(*param)
	{
		c = *(param++);
	
		for(pc = chars, f = 1; *pc && *pc != ','; pc++, f <<= 1)
		{
			if(*pc == c)
			{
				s |= f;
				break;
			}
		}
	}

	return s;
}

/** ウィジェットタイプがコンテナの派生か */

static mBool _is_include_container(mWidgetBuilder *p)
{
	return (p->wgtype == _TYPE_CONTAINER || p->wgtype == _TYPE_GROUPBOX);
}

/** コンテナスタイル取得 */

static void _getContainerStyle(int *dst,char *param)
{
	dst[1] = 0;

	if(*param == 'g')
	{
		dst[0] = MCONTAINER_TYPE_GRID;
		dst[1] = atoi(param + 1);
	}
	else if(*param == 'h')
		dst[0] = MCONTAINER_TYPE_HORZ;
	else
		dst[0] = MCONTAINER_TYPE_VERT;
}


//============================
// パラメータ
//============================


/** テキストセット (t/tr/TR) */

static void _setText(mWidget *wg,int wgtype,int type,char *param)
{
	const char *text;
	int val[2],num;

	//文字列

	switch(type)
	{
		case _PARAM_TEXT:
			text = param;
			break;
		case _PARAM_TEXT_TR:
			text = M_TR_T(atoi(param));
			break;
		case _PARAM_TEXT_TRG:
			num = _getParamNums(param, val, 2);

			//数値が一つなら、グループは M_TRGROUP_SYS
			if(num == 1)
			{
				val[1] = val[0];
				val[0] = M_TRGROUP_SYS;
			}
			
			text = M_TR_T2(val[0], val[1]);
			break;
	}

	//セット

	switch(wgtype)
	{
		case _TYPE_LABEL:
			mLabelSetText(M_LABEL(wg), text);
			break;
		case _TYPE_BUTTON:
			mButtonSetText(M_BUTTON(wg), text);
			break;
		case _TYPE_CHECKBUTTON:
			mCheckButtonSetText(M_CHECKBUTTON(wg), text);
			break;
		case _TYPE_GROUPBOX:
			mGroupBoxSetLabel(M_GROUPBOX(wg), text);
			break;
		case _TYPE_LINEEDIT:
			mLineEditSetText(M_LINEEDIT(wg), text);
			break;
		case _TYPE_COMBOBOX:
			mComboBoxAddItems(M_COMBOBOX(wg), text, 0);
			mComboBoxSetWidthAuto(M_COMBOBOX(wg));
			break;
	}
}

/** マージン・余白セット */

static void _setMarginPadding(mWidgetSpacing *pd,char *param)
{
	int val[4],num;
	char c;

	c = *param;

	if(c == 'l' || c == 't' || c == 'r' || c == 'b')
	{
		//指定値を一つセット
		
		num = atoi(param + 1);

		if(c == 'l') pd->left = num;
		else if(c == 't') pd->top = num;
		else if(c == 'r') pd->right = num;
		else pd->bottom = num;
	}
	else
	{
		//値の数により各値をセット
		
		num = _getParamNums(param, val, 4);

		if(num == 1)
			pd->left = pd->right = pd->top = pd->bottom = val[0];
		else if(num == 2)
		{
			pd->left = pd->right = val[0];
			pd->top = pd->bottom = val[1];
		}
		else if(num == 4)
		{
			pd->left   = val[0];
			pd->top    = val[1];
			pd->right  = val[2];
			pd->bottom = val[3];
		}
	}
}

/** 各パラメータセット */

static void _setParam(mWidgetBuilder *p)
{
	mWidget *wg = p->curwidget;
	char *pc,*param;
	int type,val[2],num;

	while(_getNextParam(p))
	{
		pc = p->strParam.buf;

		if(!(*pc)) continue;
	
		//'='で分離

		param = _splitParam(p, '=');
	
		//パラメータ名 => 番号

		type = mGetEqStringIndex(pc, g_paramname, ';', FALSE);
		
		//処理
	
		switch(type)
		{
			case -1: break;
			
			//ID
			case _PARAM_ID:
				wg->id = atoi(param);
				break;
			//テキスト
			case _PARAM_TEXT:
			case _PARAM_TEXT_TR:
			case _PARAM_TEXT_TRG:
				_setText(wg, p->wgtype, type, param);
				break;
			//レイアウトフラグ
			case _PARAM_LAYOUT_FLAG:
				wg->fLayout = _getStyleNum(param, "WHxywhrbcmCR");
				break;
			//margin
			case _PARAM_MARGIN:
				_setMarginPadding(&wg->margin, param);
				break;
			//コンテナ padding
			case _PARAM_PADDING:
				if(p->wgtype == _TYPE_CONTAINER)
					_setMarginPadding(&M_CONTAINER(wg)->ct.padding, param);
				break;
			//コンテナ 境界余白
			case _PARAM_CONTAINER_SEPW:
				if(_is_include_container(p))
				{
					num = _getParamNums(param, val, 2);

					if(num == 2)
					{
						M_CONTAINER(wg)->ct.gridSepCol = val[0];
						M_CONTAINER(wg)->ct.gridSepRow = val[1];
					}
					else
						M_CONTAINER(wg)->ct.sepW = val[0];
				}
				break;
			//レイアウト初期サイズ
			case _PARAM_INITSIZE:
				num = _getParamNums(param, val, 2);

				wg->initW = val[0];
				if(num == 2) wg->initH = val[1];
				break;
			//幅を文字長さで指定
			case _PARAM_WIDTH_LEN:
				if(p->wgtype == _TYPE_LINEEDIT)
					mLineEditSetWidthByLen(M_LINEEDIT(wg), atoi(param));
				break;
			//コンテナスタイル
			case _PARAM_CONTAINER_STYLE:
				if(_is_include_container(p))
				{
					_getContainerStyle(val, param);
					mContainerSetType(M_CONTAINER(wg), val[0], val[1]);
				}
				break;
			//範囲
			case _PARAM_RANGE:
				_getParamNums(param, val, 2);

				if(p->wgtype == _TYPE_LINEEDIT)
					mLineEditSetNumStatus(M_LINEEDIT(wg), val[0], val[1], 0);
				else if(p->wgtype == _TYPE_SLIDER)
					mSliderBarSetRange(M_SLIDERBAR(wg), val[0], val[1]);
				break;
		}
	}
}


//============================
// ウィジェット作成
//============================


/** スタイル取得 */

static void _getWidgetStyle(mWidgetBuilder *p,int *dst,char *param)
{
	char *pc;
	const char *pfind,*pfind2,
		*label = "rcbmB",
		*button = "wh",
		*checkbutton = "rg",
		*lineedit = "cersf",
		*listview = "MhcRCmads",
		*scrollview = "hvHVfs",
		*colorbutton = "d",
		*tab = "tblrsf",
		*arrowbutton = "ulrf",
		*slider = "vs",
		*acckey = "c";

	dst[0] = dst[1] = 0;
	
	if(p->wgtype == _TYPE_CONTAINER)
	{
		//コンテナ

		_getContainerStyle(dst, param);
	}
	else
	{
		//----- ほか

		pfind = pfind2 = NULL;

		switch(p->wgtype)
		{
			case _TYPE_LABEL: pfind = label; break;
			case _TYPE_BUTTON: pfind = button; break;
			case _TYPE_CHECKBUTTON: pfind = checkbutton; break;
			case _TYPE_LINEEDIT: pfind = lineedit; break;
			case _TYPE_LISTVIEW: pfind = listview; pfind2 = scrollview; break;
			case _TYPE_COLORBUTTON: pfind = colorbutton; break;
			case _TYPE_ARROWBUTTON: pfind = arrowbutton; break;
			case _TYPE_TAB: pfind = tab; break;
			case _TYPE_SLIDER: pfind = slider; break;
			case _TYPE_INPUTACCELKEY: pfind = acckey; break;
		}

		//

		if(pfind)
			dst[0] = _getStyleNum(param, pfind);

		if(pfind2)
		{
			pc = strchr(param, ',');
			if(pc) dst[1] = _getStyleNum(pc + 1, pfind2);
		}
	}
}

/** ウィジェット作成 */

static mBool _createWidget(mWidgetBuilder *p,int type,char *styleparam)
{
	mWidget *wg,*parent;
	int st[2];

	p->wgtype = type;

	//スタイル取得

	_getWidgetStyle(p, st, styleparam);

	//作成

	wg = NULL;
	parent = p->curparent;

	switch(type)
	{
		case _TYPE_CONTAINER:
			wg = (mWidget *)mContainerNew(0, parent);
			if(wg)
			{
				mContainerSetType(M_CONTAINER(wg), st[0], st[1]);
				p->curparent = wg;
			}
			break;
		case _TYPE_LABEL:
			wg = (mWidget *)mLabelNew(0, parent, st[0]);
			break;
		case _TYPE_BUTTON:
			wg = (mWidget *)mButtonNew(0, parent, st[0]);
			break;
		case _TYPE_SPACER:
			wg = mWidgetNew(0, parent);
			break;
		case _TYPE_CHECKBUTTON:
			wg = (mWidget *)mCheckButtonNew(0, parent, st[0]);
			break;
		case _TYPE_COMBOBOX:
			wg = (mWidget *)mComboBoxNew(0, parent, st[0]);
			break;
		case _TYPE_GROUPBOX:
			wg = (mWidget *)mGroupBoxNew(0, parent, st[0]);
			p->curparent = wg;
			break;
		case _TYPE_LINEEDIT:
			wg = (mWidget *)mLineEditNew(0, parent, st[0]);
			break;
		case _TYPE_LISTVIEW:
			wg = (mWidget *)mListViewNew(0, parent, st[0], st[1]);
			break;
		case _TYPE_INPUTACCELKEY:
			wg = (mWidget *)mInputAccelKeyNew(0, parent, st[0]);
			break;
		case _TYPE_COLORBUTTON:
			wg = (mWidget *)mColorButtonNew(0, parent, st[0]);
			break;
		case _TYPE_ARROWBUTTON:
			wg = (mWidget *)mArrowButtonNew(0, parent, st[0]);
			break;
		case _TYPE_FONTBUTTON:
			wg = (mWidget *)mFontButtonNew(0, parent, st[0]);
			break;
		case _TYPE_TAB:
			wg = (mWidget *)mTabNew(0, parent, st[0]);
			break;
		case _TYPE_SLIDER:
			wg = (mWidget *)mSliderBarNew(0, parent, st[0]);
			break;
	}

	//

	p->curwidget = wg;

	return (wg != NULL);
}


//*******************************


/**
@defgroup widgetbuilder mWidgetBuilder
@brief ウィジェットビルダー

テキストで指定したパラメータで複数のウィジェットを作成。

@ingroup group_etc
@{

@file mWidgetBuilder.h

*/

/** 文字列からウィジェット作成 */

mBool mWidgetBuilderCreateFromText(mWidget *parent,const char *text)
{
	mWidgetBuilder wb;
	int type;
	mBool result = TRUE;
	char *pc,*styleparam;

	//初期化

	wb.pnext = text;
	wb.curparent = parent;

	mStrAlloc(&wb.strParam, 64);

	//

	while(_getline(&wb))
	{
		//最初の値 = ウィジェット名＋スタイル
	
		if(!_getNextParam(&wb)) continue;

		pc = wb.strParam.buf;

		//'-' の場合、親へ戻る

		if(*pc == '-')
		{
			for(; *pc == '-' && wb.curparent != parent; pc++)
				wb.curparent = wb.curparent->parent;
		
			continue;
		}

		//ウィジェットタイプ
	
		styleparam = _splitParam(&wb, '#');

		type = mGetEqStringIndex(pc, g_widgetname, ';', FALSE);

		//ウィジェット作成
		
		if(type != _TYPE_UNKNOWN)
		{
			if(!_createWidget(&wb, type, styleparam))
			{
				result = FALSE;
				break;
			}

			//パラメータセット

			_setParam(&wb);
		}
	}

	mStrFree(&wb.strParam);

	return result;
}

/** @} */
