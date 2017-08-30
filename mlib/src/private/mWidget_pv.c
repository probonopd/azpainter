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
 * mWidget 内部関数
 *****************************************/

#include "mDef.h"
#include "mAppDef.h"
#include "mWindowDef.h"

#include "mEvent.h"
#include "mWidget.h"
#include "mRectBox.h"
#include "mUtilStr.h"
#include "mFont.h"

#include "mEventList.h"
#include "mWidget_pv.h"



//=======================


/** ウィジェットツリーの次のウィジェット取得 (ルート下全体が対象)
 * 
 * @return NULL で終了 */

mWidget *__mWidgetGetTreeNext(mWidget *p)
{
	if(p->first)
		return p->first;
	else
	{
		do
		{
			if(p->next)
				return p->next;
			else
				p = p->parent;
		}while(p);
		
		return p;
	}
}

/** 次のウィジェット取得
 * 
 * p の子はスキップする。 */

mWidget *__mWidgetGetTreeNextPass(mWidget *p)
{
	do{
		if(p->next)
			return p->next;
		else
			p = p->parent;
	}while(p);
	
	return p;
}

/** ウィジェットツリーの次のウィジェット取得
 * 
 * p の子はスキップする。root の下位のみ取得。 */

mWidget *__mWidgetGetTreeNextPass_root(mWidget *p,mWidget *root)
{
	if(p == root) return NULL;
	
	do{
		if(p->next)
			return p->next;
		else
		{
			p = p->parent;
			if(p == root) return NULL;
		}
	}while(p);
	
	return p;
}

/** ウィジェットツリーの次のウィジェット取得
 * 
 * p の下位も含む。root の下位のみ取得。
 *
 * @param p NULL で root から */

mWidget *__mWidgetGetTreeNext_root(mWidget *p,mWidget *root)
{
	if(!p) p = root;

	if(p->first)
		return p->first;
	else
		return __mWidgetGetTreeNextPass_root(p, root);
}

/** fUI のフラグが ON になっている次のウィジェット取得
 * 
 * @param p NULL でルートウィジェットから */

mWidget *__mWidgetGetTreeNext_follow_ui(mWidget *p,
		uint32_t follow_mask,uint32_t run_mask)
{
	if(!p) p = MAPP->widgetRoot;
	
	while(1)
	{
		//次へ

		if(p->fUI & follow_mask)
		{
			p->fUI &= ~follow_mask;

			p = __mWidgetGetTreeNext(p);
		}
		else
			p = __mWidgetGetTreeNextPass(p);
		
		if(!p) break;
		
		//

		if(p->fUI & run_mask) break;
	}
	
	return p;
}

/** 次の描画対象ウィジェット取得 */

/*
 mWidgetDrawBkgnd() によって背景が描画された場合、MWIDGET_UI_DRAWED_BKGND が ON になる。
 親がすでに全体描画された場合、以降の子は背景を描画する必要がないので、
 mWidgetDrawBkgnd() 内で親の MWIDGET_UI_DRAWED_BKGND が ON の場合は描画しないようにしている。
 なお、MWIDGET_UI_DRAWED_BKGND はこの関数内で常にクリアされた状態で終わるようにしてある。
*/

mWidget *__mWidgetGetTreeNext_follow_uidraw(mWidget *p)
{
	int follow;

	if(!p) p = MAPP->widgetRoot;
	
	while(1)
	{
		//辿るか
	
		follow = p->fUI & MWIDGET_UI_FOLLOW_DRAW;

		p->fUI &= ~MWIDGET_UI_FOLLOW_DRAW;
	
		//次へ

		if(follow && p->first)
			p = p->first;
		else
		{
			do{
				if(p->next)
				{
					p = p->next;
					break;
				}
				else
				{
					p = p->parent;
					if(p) p->fUI &= ~MWIDGET_UI_DRAWED_BKGND;
				}
			}while(p);
		}
		
		if(!p) break;
		
		//

		if(p->fUI & MWIDGET_UI_DRAW)
		{
			p->fUI &= ~MWIDGET_UI_DRAW;
			break;
		}
	}
	
	return p;
}

/** 親方向すべてに fUI のフラグを ON にする */

void __mWidgetSetTreeUpper_ui(mWidget *p,uint32_t flags)
{
	for(; p; p = p->parent)
		p->fUI |= flags;
}

/** fState のフラグが ON の次のウィジェット取得
 * 
 * @param p  NULL で root の子を先頭に */

mWidget *__mWidgetGetTreeNext_state(mWidget *p,mWidget *root,uint32_t mask)
{
	if(p)
		p = __mWidgetGetTreeNext_root(p, root);
	else
		p = root->first;
	
	for(; p; p = __mWidgetGetTreeNext_root(p, root))
	{
		if((p->fState & mask) == mask) return p;
	}
	
	return NULL;
}


//=======================

/** ウィジェット全体のクリッピング範囲取得 (絶対座標)
 *
 * @return FALSE で親の範囲外 */

mBool __mWidgetGetClipRect(mWidget *wg,mRect *rcdst)
{
	mRect rc;
	mWidget *p;

	//wg の範囲

	rc.x1 = wg->absX;
	rc.y1 = wg->absY;
	rc.x2 = rc.x1 + wg->w - 1;
	rc.y2 = rc.y1 + wg->h - 1;

	//親の範囲を適用

	for(p = wg->parent; p->parent; p = p->parent)
	{
		if(!mRectClipBox_d(&rc, p->absX, p->absY, p->w, p->h))
			return FALSE;
	}

	*rcdst = rc;

	return TRUE;
}


//============================
// レイアウト関連
//============================


/** リサイズ時 */

void __mWidgetOnResize(mWidget *p)
{
	/* onSize はレイアウトの前に実行する。
	 * 子アイテムのレイアウト前にレイアウト状態を変更したい場合があるため。 */

	if(p->onSize)
		(p->onSize)(p);

	mWidgetCalcHintSize(p);

	mWidgetLayout(p);

	mWidgetUpdate(p);
}

/** レイアウトサイズ計算 */

void __mWidgetCalcHint(mWidget *p)
{
	if(p->fUI & MWIDGET_UI_CALC)
	{
		if(p->calcHint)
			(p->calcHint)(p);
		else if(p->fType & MWIDGET_TYPE_CONTAINER)
			//コンテナの関数を使う
			(M_CONTAINER(p)->ct.calcHint)(p);

		p->fUI &= ~MWIDGET_UI_CALC;
	}
}

/** レイアウト時の幅取得 */

int __mWidgetGetLayoutW(mWidget *p)
{
	if(p->fLayout & MLF_FIX_W)
		return p->w;
	else if(p->initW && !(p->fUI & MWIDGET_UI_LAYOUTED))
		return p->initW;
	else if(p->hintOverW)
		return p->hintOverW;
	else if(p->hintMinW)
		return (p->hintW < p->hintMinW)? p->hintMinW: p->hintW;
	else
		return p->hintW;
}

/** レイアウト時の高さ取得 */

int __mWidgetGetLayoutH(mWidget *p)
{
	if(p->fLayout & MLF_FIX_H)
		return p->h;
	else if(p->initH && !(p->fUI & MWIDGET_UI_LAYOUTED))
		return p->initH;
	else if(p->hintOverH)
		return p->hintOverH;
	else if(p->hintMinH)
		return (p->hintH < p->hintMinH)? p->hintMinH: p->hintH;
	else
		return p->hintH;
}

/** 余白を含めたレイアウト幅取得 */

int __mWidgetGetLayoutW_space(mWidget *p)
{
	return __mWidgetGetLayoutW(p) + p->margin.left + p->margin.right;
}

/** 余白を含めたレイアウト高さ取得 */

int __mWidgetGetLayoutH_space(mWidget *p)
{
	return __mWidgetGetLayoutH(p) + p->margin.top + p->margin.bottom;
}

/** 推奨サイズ計算時の幅と高さ取得 */

void __mWidgetGetLayoutCalcSize(mWidget *p,mSize *hint,mSize *init)
{
	int hs,is,margin;

	//幅

	margin = p->margin.left + p->margin.right;

	if(p->fLayout & MLF_FIX_W)
		hs = is = p->w;
	else
	{
		is = p->initW;
		hs = (p->hintOverW)? p->hintOverW: p->hintW;

		if(!is) is = hs;
	}

	hint->w = hs + margin;
	init->w = is + margin;

	//高さ

	margin = p->margin.top + p->margin.bottom;

	if(p->fLayout & MLF_FIX_H)
		hs = is = p->h;
	else
	{
		is = p->initH;
		hs = (p->hintOverH)? p->hintOverH: p->hintH;

		if(!is) is = hs;
	}

	hint->h = hs + margin;
	init->h = is + margin;
}


//===============================
// フォーカス関連
//===============================


/** フォーカスセット処理 */

void __mWidgetSetFocus(mWidget *p,int by)
{
	mEvent *ev;

	p->fState |= MWIDGET_STATE_DEFAULT_FOCUS | MWIDGET_STATE_FOCUSED;
	
	ev = mEventListAppend_widget(p, MEVENT_FOCUS);
	if(ev)
	{
		ev->focus.bOut = FALSE;
		ev->focus.by = by;
	}
}

/** フォーカス消去処理 */

void __mWidgetKillFocus(mWidget *p,int by)
{
	mEvent *ev;

	if(p->fState & MWIDGET_STATE_FOCUSED)
	{
		p->fState &= ~MWIDGET_STATE_FOCUSED;
		
		ev = mEventListAppend_widget(p, MEVENT_FOCUS);
		
		ev->focus.bOut = TRUE;
		ev->focus.by = by;
	}
}

/** フォーカスを取り除く
 * 
 * 削除時や非表示時など、フォーカスを無効にする場合に呼ぶ。 */

void __mWidgetRemoveFocus(mWidget *p)
{
	if(p->fState & MWIDGET_STATE_FOCUSED)
		__mWindowSetFocus(p->toplevel, NULL, MEVENT_FOCUS_BY_UNKNOWN);
}

/** p が使用できない状態の場合、フォーカス解除
 *
 * p が非表示または無効の場合、このウィジェット下にフォーカスがあればフォーカスを解除。 */

void __mWidgetRemoveFocus_byDisable(mWidget *p)
{
	mWidget *wg;

	if(!(p->fState & MWIDGET_STATE_VISIBLE)
		|| !(p->fState & MWIDGET_STATE_ENABLED))
	{
		for(wg = p; wg; wg = __mWidgetGetTreeNext_root(wg, p))
		{
			if(wg->fState & MWIDGET_STATE_FOCUSED)
			{
				__mWidgetRemoveFocus(wg);
				break;
			}
		}
	}
}


//===============================
// ウィジェットテキスト共通処理
//===============================


/** 複数行ラベル文字列の各行の情報取得
 *
 * 単一行の場合は NULL となる。
 *
 * @return 確保されたバッファ (最後の pos は -1 となる) */

mWidgetLabelTextRowInfo *__mWidgetGetLabelTextRowInfo(const char *text)
{
	mWidgetLabelTextRowInfo *buf,*pd;
	const char *pc,*pcend;
	int len,lines;
	
	if(!text) return NULL;
	
	//行数
	
	lines = mGetStrLines(text);
	if(lines <= 1) return NULL;

	//確保
	
	buf = (mWidgetLabelTextRowInfo *)mMalloc(
		sizeof(mWidgetLabelTextRowInfo) * (lines + 1), TRUE);
	
	if(!buf) return NULL;
	
	//各行処理

	pd = buf;
	
	for(pc = text; *pc; pd++)
	{
		pcend = mStrchr_end(pc, '\n');

		len = pcend - pc;

		//最後が '\n' + NULL の場合
		
		if(len == 0 && *pcend == 0) break;
		
		//
		
		pd->pos = pc - text;
		pd->len = len;

		pc = pcend;
		if(*pc) pc++;
	}

	pd->pos = -1;

	return buf;
}

/** 複数行ラベルテキストの各行の幅と全体のサイズ取得 */

void __mWidgetGetLabelTextSize(mWidget *p,const char *text,mWidgetLabelTextRowInfo *buf,mSize *dstsize)
{
	mFont *font;
	int w,maxw = 0,lines;
	
	font = mWidgetGetFont(p);

	if(!text || !buf)
	{
		//text != NULL && !buf の場合は単一行
		
		dstsize->w = (text)? mFontGetTextWidth(font, text, -1): 0;
		dstsize->h = font->height;
	}
	else
	{
		for(lines = 0; buf->pos >= 0; buf++, lines++)
		{
			w = mFontGetTextWidth(font, text + buf->pos, buf->len);
			
			buf->width = w;
			
			if(w > maxw) maxw = w;
		}
		
		dstsize->w = maxw;
		dstsize->h = font->height + font->lineheight * (lines - 1);
	}
}


//*****************************
// mWindow
//*****************************


/** デフォルトフォーカスウィジェット取得 */

mWidget *__mWindowGetDefaultFocusWidget(mWindow *win)
{
	mWidget *wg;

	//デフォルトフォーカス

	wg = __mWidgetGetTreeNext_state(NULL, M_WIDGET(win),
			MWIDGET_STATE_TAKE_FOCUS | MWIDGET_STATE_DEFAULT_FOCUS
			| MWIDGET_STATE_VISIBLE | MWIDGET_STATE_ENABLED);
	
	//フォーカスセット可能な最初のウィジェット
	
	if(!wg)
	{
		wg = __mWidgetGetTreeNext_state(NULL, M_WIDGET(win),
				MWIDGET_STATE_TAKE_FOCUS | MWIDGET_STATE_VISIBLE | MWIDGET_STATE_ENABLED);
	}
	
	return wg;
}

/** ウィンドウのフォーカスウィジェット変更
 *
 * @param focus 新しいフォーカス。NULL の場合あり */

mBool __mWindowSetFocus(mWindow *win,mWidget *focus,int by)
{
	mWidget *old = win->win.focus_widget;

	//変更なし

	if(focus == old) return FALSE;

	//フォーカスを受け取り、表示＆有効状態、かつフォーカスがない場合のみ

	if(focus &&
		(focus->fState & (MWIDGET_STATE_TAKE_FOCUS | MWIDGET_STATE_VISIBLE | MWIDGET_STATE_ENABLED | MWIDGET_STATE_FOCUSED))
			!= (MWIDGET_STATE_TAKE_FOCUS | MWIDGET_STATE_VISIBLE | MWIDGET_STATE_ENABLED))
		return FALSE;

	//現在のフォーカスを解除

	if(old)
	{
		__mWidgetKillFocus(old, by);
		old->fState &= ~MWIDGET_STATE_DEFAULT_FOCUS;
	}

	//フォーカスセット
	
	win->win.focus_widget = focus;
	
	if(focus)
		__mWidgetSetFocus(focus, by);

	return TRUE;
}

/** ウィンドウ内の次のフォーカスへ移動 */

mBool __mWindowMoveNextFocus(mWindow *win)
{
	mWidget *wg = NULL;
	
	//フォーカスウィジェットの次
	
	if(win->win.focus_widget)
	{
		wg = __mWidgetGetTreeNext_state(win->win.focus_widget, M_WIDGET(win),
				MWIDGET_STATE_TAKE_FOCUS | MWIDGET_STATE_VISIBLE | MWIDGET_STATE_ENABLED);
	}
	
	//先頭から検索

	if(!wg)
	{
		wg = __mWidgetGetTreeNext_state(NULL, M_WIDGET(win),
				MWIDGET_STATE_TAKE_FOCUS | MWIDGET_STATE_VISIBLE | MWIDGET_STATE_ENABLED);
	}
	
	if(!wg)
		return FALSE;
	else
	{
		__mWindowSetFocus(win, wg, MEVENT_FOCUS_BY_TABMOVE);
		return TRUE;
	}
}
