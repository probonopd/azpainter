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
 * [レイアウト関数]
 *****************************************/

#include "mDef.h"

#include "mContainerDef.h"

#include "mWidget.h"
#include "mWidget_pv.h"



//==================================
// 共通
//==================================


/** コンテナの領域幅取得 (余白は除く) */

static void _get_container_inside_size(mContainer *p,int *pw,int *ph)
{
	int w,h;

	w = p->wg.w - p->ct.padding.left - p->ct.padding.right;
	h = p->wg.h - p->ct.padding.top - p->ct.padding.bottom;
	
	if(w < 1) w = 1;
	if(h < 1) h = 1;
	
	*pw = w, *ph = h;
}

/** 推奨サイズセット */

static void _set_hint_size(mWidget *wg,mSize *hint,mSize *init)
{
	mContainer *p = (mContainer *)wg;
	int pw,ph;

	pw = p->ct.padding.left + p->ct.padding.right;
	ph = p->ct.padding.top + p->ct.padding.bottom;

	wg->hintW = hint->w + pw;
	wg->hintH = hint->h + ph;
	wg->initW = init->w + pw;
	wg->initH = init->h + ph;
}


//==================================
// HORZ : 水平
//==================================


/** 推奨サイズ計算 */

void __mLayoutCalcHorz(mWidget *root)
{
	mContainer *root_ct;
	mWidget *p;
	mSize hint,init,maxhint,maxinit;
	
	root_ct = M_CONTAINER(root);

	maxhint.w = maxhint.h = 0;
	maxinit.w = maxinit.h = 0;

	for(p = root->first; p; p = p->next)
	{
		__mWidgetCalcHint(p);
		
		if(p->fState & MWIDGET_STATE_VISIBLE)
		{
			__mWidgetGetLayoutCalcSize(p, &hint, &init);

			maxhint.w += hint.w;
			maxinit.w += init.w;

			if(p->next)
			{
				maxhint.w += root_ct->ct.sepW;
				maxinit.w += root_ct->ct.sepW;
			}

			if(maxhint.h < hint.h) maxhint.h = hint.h;
			if(maxinit.h < init.h) maxinit.h = init.h;
		}
	}

	_set_hint_size(root, &maxhint, &maxinit);
}

/** 水平レイアウト実行 */

void __mLayoutHorz(mWidget *wg)
{
	mContainer *container = M_CONTAINER(wg);
	mWidget *p,*expand_last_wg;
	int expand_cnt,expand_w_each,expand_w_last;
	int cw,ch,x,y,w,wg_w,wg_h,lw,lh,mw,mh,tmp,left,top;
	uint32_t lflags;
	
	_get_container_inside_size(container, &cw, &ch);
	
	//拡張アイテム情報取得
	
	w = cw;
	expand_cnt = 0;
	expand_last_wg = NULL;
	
	for(p = wg->first; p; p = p->next)
	{
		if(!(p->fState & MWIDGET_STATE_VISIBLE)) continue;
		
		if(p->fLayout & (MLF_EXPAND_X | MLF_EXPAND_W))
		{
			expand_cnt++;
			expand_last_wg = p;
		}
		else
			w -= __mWidgetGetLayoutW_space(p);
		
		if(p->next) w -= container->ct.sepW;
	}
	
	//拡張アイテムの各幅と最後のアイテムの幅
	
	if(expand_cnt)
	{
		if(w <= 0) w = 0;
		expand_w_each = w / expand_cnt;
		expand_w_last = w - expand_w_each * (expand_cnt - 1);
	}
	
	//---------- レイアウト
	
	left = container->ct.padding.left;
	top  = container->ct.padding.top;
	
	for(p = wg->first; p; p = p->next)
	{
		if(!(p->fState & MWIDGET_STATE_VISIBLE)) continue;
		
		lw = __mWidgetGetLayoutW(p);
		lh = __mWidgetGetLayoutH(p);
		lflags = p->fLayout;
		
		mw = p->margin.left + p->margin.right;
		mh = p->margin.top + p->margin.bottom;
		
		x = y = 0;
		wg_w = lw;
		wg_h = lh;
				
		//高さ・拡張
		
		if(lflags & MLF_EXPAND_H)
		{
			wg_h = ch - mh;
			if(wg_h < lh) wg_h = lh;
		}
		
		//Y 位置
		
		if(lflags & MLF_MIDDLE)
			y = (ch - wg_h - mh) >> 1;
		else if(lflags & MLF_BOTTOM)
			y = ch - wg_h - p->margin.bottom;
		
		if(y < 0) y = 0;
		
		y += top + p->margin.top;
		
		//ウィジェット幅
				
		if(!(lflags & (MLF_EXPAND_X | MLF_EXPAND_W)))
			w = lw + mw;
		else
		{
			w = (p == expand_last_wg)? expand_w_last: expand_w_each;

			tmp = w - mw;
			if(tmp < lw) tmp = lw;
			
			if(lflags & MLF_EXPAND_W) wg_w = tmp;
			
			w = tmp + mw;
		}
		
		//X 位置
		
		if(lflags & MLF_CENTER)
			x = (w - wg_w - mw) >> 1;
		else if(lflags & MLF_RIGHT)
			x = w - wg_w - mw;
		
		if(x < 0) x = 0;
		
		x += left + p->margin.left;
		
		//ウィジェット位置・サイズ
		
		mWidgetMoveResize(p, x, y, wg_w, wg_h);

		p->fUI |= MWIDGET_UI_LAYOUTED;
		
		//次の位置
		
		left += w;
		if(p->next) left += container->ct.sepW;
	}
}


//==================================
// VERT : 垂直
//==================================


/** 推奨サイズ計算 */

void __mLayoutCalcVert(mWidget *root)
{
	mContainer *root_ct;
	mWidget *p;
	mSize hint,init,maxhint,maxinit;
	
	root_ct = M_CONTAINER(root);

	maxhint.w = maxhint.h = 0;
	maxinit.w = maxinit.h = 0;
	
	for(p = root->first; p; p = p->next)
	{
		__mWidgetCalcHint(p);
		
		if(p->fState & MWIDGET_STATE_VISIBLE)
		{
			__mWidgetGetLayoutCalcSize(p, &hint, &init);

			maxhint.h += hint.h;
			maxinit.h += init.h;

			if(p->next)
			{
				maxhint.h += root_ct->ct.sepW;
				maxinit.h += root_ct->ct.sepW;
			}

			if(maxhint.w < hint.w) maxhint.w = hint.w;
			if(maxinit.w < init.w) maxinit.w = init.w;
		}
	}
	
	_set_hint_size(root, &maxhint, &maxinit);
}

/** 垂直レイアウト実行 */

void __mLayoutVert(mWidget *wg)
{
	mContainer *container = M_CONTAINER(wg);
	mWidget *p,*expand_last_wg;
	int expand_cnt,expand_w_each,expand_w_last;
	int cw,ch,x,y,h,wg_w,wg_h,lw,lh,mw,mh,tmp,left,top;
	uint32_t lflags;
	
	_get_container_inside_size(container, &cw, &ch);

	//拡張アイテム情報取得
	
	h = ch;
	expand_cnt = 0;
	expand_last_wg = NULL;
	
	for(p = container->wg.first; p; p = p->next)
	{
		if(!(p->fState & MWIDGET_STATE_VISIBLE)) continue;
		
		if(p->fLayout & (MLF_EXPAND_Y | MLF_EXPAND_H))
		{
			expand_cnt++;
			expand_last_wg = p;
		}
		else
			h -= __mWidgetGetLayoutH_space(p);
		
		if(p->next) h -= container->ct.sepW;
	}
	
	//拡張アイテムの各幅と最後のアイテムの幅
	
	if(expand_cnt)
	{
		if(h <= 0) h = 0;
		expand_w_each = h / expand_cnt;
		expand_w_last = h - expand_w_each * (expand_cnt - 1);
	}

	//---------- レイアウト
	
	left = container->ct.padding.left;
	top  = container->ct.padding.top;
	
	for(p = container->wg.first; p; p = p->next)
	{
		if(!(p->fState & MWIDGET_STATE_VISIBLE)) continue;
		
		lw = __mWidgetGetLayoutW(p);
		lh = __mWidgetGetLayoutH(p);
		lflags = p->fLayout;
		
		mw = p->margin.left + p->margin.right;
		mh = p->margin.top + p->margin.bottom;
		
		x = y = 0;
		wg_w = lw;
		wg_h = lh;
				
		//幅・拡張
		
		if(lflags & MLF_EXPAND_W)
		{
			wg_w = cw - mw;
			if(wg_w < lw) wg_w = lw;
		}
		
		//X 位置
		
		if(lflags & MLF_CENTER)
			x = (cw - wg_w - mw) >> 1;
		else if(lflags & MLF_RIGHT)
			x = cw - wg_w - p->margin.right;
		
		if(x < 0) x = 0;
		
		x += left + p->margin.left;
		
		//ウィジェット高さ
				
		if(!(lflags & (MLF_EXPAND_Y | MLF_EXPAND_H)))
			h = lh + mh;
		else
		{
			h = (p == expand_last_wg)? expand_w_last: expand_w_each;

			tmp = h - mh;
			if(tmp < lh) tmp = lh;
			
			if(lflags & MLF_EXPAND_H) wg_h = tmp;
			
			h = tmp + mh;
		}
		
		//Y 位置
		
		if(lflags & MLF_MIDDLE)
			y = (h - wg_h - mh) >> 1;
		else if(lflags & MLF_BOTTOM)
			y = h - wg_h - mh;
		
		if(y < 0) y = 0;
		
		y += top + p->margin.top;
		
		//ウィジェット位置・サイズ
		
		mWidgetMoveResize(p, x, y, wg_w, wg_h);

		p->fUI |= MWIDGET_UI_LAYOUTED;

		//次の位置
		
		top += h;
		if(p->next) top += container->ct.sepW;
	}
}


//==================================
// GRID : グリッド
//==================================


typedef struct
{
	int32_t expand_last_no,
		expand_w_each,
		expand_w_last,
		add_sep;
	uint32_t cell_size[1];   /* 上位2bit はフラグ */
}__grid_info;

#define GRID_CELL_EXPAND    (1L<<31)
#define GRID_CELL_ADD_SEP   (1L<<30)
#define GRID_CELL_SIZE_MASK ((1L<<30) - 1)


/** 次の行へ */

static mWidget *_get_next_row(mContainer *ct,mWidget *p)
{
	int cols = ct->ct.gridCols;

	for(; p && cols; cols--, p = p->next);
	
	return p;
}

/** 推奨サイズ計算 */

void __mLayoutCalcGrid(mWidget *root)
{
	mContainer *root_ct;
	mWidget *p,*p2;
	int maxh,maxi,cnt;
	mSize hint,init,maxhint,maxinit;
	
	root_ct = M_CONTAINER(root);

	maxhint.w = maxhint.h = 0;
	maxinit.w = maxinit.h = 0;
	
	//子のサイズ計算
	
	for(p = root->first; p; p = p->next)
		__mWidgetCalcHint(p);
	
	//幅 (各 X 列の最大幅)
	
	for(p = root->first, cnt = root_ct->ct.gridCols; p && cnt; cnt--, p = p->next)
	{
		maxh = maxi = 0;
		
		for(p2 = p; p2; p2 = _get_next_row(root_ct, p2))
		{
			if(p2->fState & MWIDGET_STATE_VISIBLE)
			{
				__mWidgetGetLayoutCalcSize(p2, &hint, &init);

				if(maxh < hint.w) maxh = hint.w;
				if(maxi < init.w) maxi = init.w;
			}
		}

		if(cnt > 1)
		{
			if(maxh) maxh += root_ct->ct.gridSepCol;
			if(maxi) maxi += root_ct->ct.gridSepCol;
		}

		maxhint.w += maxh;
		maxinit.w += maxi;
	}
	
	//高さ
	
	for(p = root->first; p; )
	{
		maxh = maxi = 0;
	
		for(cnt = root_ct->ct.gridCols; p && cnt; cnt--, p = p->next)
		{
			if(p->fState & MWIDGET_STATE_VISIBLE)
			{
				__mWidgetGetLayoutCalcSize(p, &hint, &init);

				if(maxh < hint.h) maxh = hint.h;
				if(maxi < init.h) maxi = init.h;
			}
		}

		if(p)
		{
			if(maxh) maxh += root_ct->ct.gridSepRow;
			if(maxi) maxi += root_ct->ct.gridSepRow;
		}

		maxhint.h += maxh;
		maxinit.h += maxi;
	}

	//

	_set_hint_size(root, &maxhint, &maxinit);
}


/** X 列の情報作成 */

static __grid_info *_get_grid_col_info(mContainer *ctn,int width,int *ret_rows)
{
	__grid_info *info;
	mWidget *px,*py;
	int i,flags,max,tmp,cols,rows = 0,expand_cnt = 0,expand_last_no = 0;
	uint32_t cell;
	
	cols = ctn->ct.gridCols;

	info = (__grid_info *)mMalloc(sizeof(__grid_info) + (cols - 1) * sizeof(int32_t), FALSE);
	if(!info) return NULL;
	
	//
	
	for(px = ctn->wg.first, i = 0; px && i < cols; px = px->next, i++)
	{
		flags = max = 0;
	
		for(py = px; py; py = _get_next_row(ctn, py))
		{
			if(py->fState & MWIDGET_STATE_VISIBLE)
			{
				flags |= 1;
				
				if(py->fLayout & (MLF_EXPAND_X | MLF_EXPAND_W))
					flags |= 2;
				
				tmp = __mWidgetGetLayoutW_space(py);
				if(tmp > max) max = tmp;
			}
			
			if(i == 0) rows++;
		}
		
		//
		
		cell = max;
		if(flags & 2) cell |= GRID_CELL_EXPAND;
		if((flags & 1) && i < cols - 1) cell |= GRID_CELL_ADD_SEP;
		
		info->cell_size[i] = cell;
		
		//
		
		if(flags)
		{
			if(flags & 2)
			{
				expand_cnt++;
				expand_last_no = i;
			}
			else
				width -= max;
			
			if(cell & GRID_CELL_ADD_SEP) width -= ctn->ct.gridSepCol;
		}
	}
	
	//
	
	if(expand_cnt)
	{
		if(width < 0) width = 0;

		info->expand_last_no = expand_last_no;
		info->expand_w_each  = width / expand_cnt;
		info->expand_w_last  = width - info->expand_w_each * (expand_cnt - 1);
	}
	
	*ret_rows = rows;
	
	return info;
}

/** Y 行の情報作成 */

static __grid_info *_get_grid_row_info(mContainer *ctn,int rows,int height)
{
	__grid_info *info;
	mWidget *p;
	int i,j,flags,max,tmp,cols,expand_cnt = 0,expand_last_no = 0;
	uint32_t cell;
	
	cols = ctn->ct.gridCols;

	info = (__grid_info *)mMalloc(sizeof(__grid_info) + (rows - 1) * sizeof(int32_t), FALSE);
	if(!info) return NULL;
	
	//
	
	for(p = ctn->wg.first, i = 0; p; i++)
	{
		flags = max = 0;
	
		for(j = 0; p && j < cols; j++, p = p->next)
		{
			if(p->fState & MWIDGET_STATE_VISIBLE)
			{
				flags |= 1;
				
				if(p->fLayout & (MLF_EXPAND_Y | MLF_EXPAND_H))
					flags |= 2;
				
				tmp = __mWidgetGetLayoutH_space(p);
				if(tmp > max) max = tmp;
			}
		}
		
		//
		
		cell = max;
		if(flags & 2) cell |= GRID_CELL_EXPAND;
		if((flags & 1) && p) cell |= GRID_CELL_ADD_SEP;
		
		info->cell_size[i] = cell;
		
		//
		
		if(flags)
		{
			if(flags & 2)
			{
				expand_cnt++;
				expand_last_no = i;
			}
			else
				height -= max;
	
			if(cell & GRID_CELL_ADD_SEP) height -= ctn->ct.gridSepRow;
		}
	}
	
	//
	
	if(expand_cnt)
	{
		if(height < 0) height = 0;

		info->expand_last_no = expand_last_no;
		info->expand_w_each  = height / expand_cnt;
		info->expand_w_last  = height - info->expand_w_each * (expand_cnt - 1);
	}
	
	return info;
}

/** グリッドレイアウト実行 */

void __mLayoutGrid(mContainer *wg)
{
	mContainer *container = M_CONTAINER(wg);
	__grid_info *info_col,*info_row;
	mWidget *p;
	int cw,ch,rows,top,left,left_start,row_no,col_no,maxh;
	int lw,lh,lflags,mw,mh,x,y,wg_w,wg_h,w,h,tmp;

	_get_container_inside_size(container, &cw, &ch);
	
	//情報取得
	
	info_col = _get_grid_col_info(container, cw, &rows);
	if(!info_col) return;
	
	info_row = _get_grid_row_info(container, rows, ch);
	if(!info_row)
	{
		mFree(info_col);
		return;
	}
	
	//
	
	left_start = container->ct.padding.left;
	top = container->ct.padding.top;
	
	for(p = container->wg.first, row_no = 0; p; row_no++)
	{
		left = left_start;
		maxh = 0;
		
		for(col_no = 0; p && col_no < container->ct.gridCols; col_no++, p = p->next)
		{
			if(!(p->fState & MWIDGET_STATE_VISIBLE)) continue;
			
			lw = __mWidgetGetLayoutW(p);
			lh = __mWidgetGetLayoutH(p);
			lflags = p->fLayout;
			
			mw = p->margin.left + p->margin.right;
			mh = p->margin.top + p->margin.bottom;
			
			x = y = 0;
			wg_w = lw;
			wg_h = lh;
			
			//ウィジェット幅
			
			tmp = info_col->cell_size[col_no];
			
			if(lflags & MLF_GRID_COL_W)
			{
				w = tmp & GRID_CELL_SIZE_MASK;
				wg_w = w - mw;
			}
			else if(tmp & GRID_CELL_EXPAND)
			{
				//Y 行に一つでも拡張がある場合
				
				tmp &= GRID_CELL_SIZE_MASK;
				
				w = (col_no == info_col->expand_last_no)? info_col->expand_w_last: info_col->expand_w_each;
				if(w < tmp) w = tmp;
				
				if(lflags & MLF_EXPAND_W) wg_w = w - mw;
			}
			else
				w = tmp & GRID_CELL_SIZE_MASK;
			
			//ウィジェット高さ
			
			tmp = info_row->cell_size[row_no];
			
			if(lflags & MLF_GRID_ROW_H)
			{
				h = tmp & GRID_CELL_SIZE_MASK;
				wg_h = h - mh;
			}
			else if(tmp & GRID_CELL_EXPAND)
			{
				tmp &= GRID_CELL_SIZE_MASK;
				
				h = (row_no == info_row->expand_last_no)? info_row->expand_w_last: info_row->expand_w_each;
				if(h < tmp) h = tmp;
				
				if(lflags & MLF_EXPAND_H) wg_h = h - mh;
			}
			else
				h = tmp & GRID_CELL_SIZE_MASK;
			
			//X 位置
			
			if(lflags & MLF_CENTER)
				x = (w - wg_w - mw) >> 1;
			else if(lflags & MLF_RIGHT)
				x = w - wg_w - mw;
			
			if(x < 0) x = 0;
			
			x += left + p->margin.left;
			
			//Y 位置
			
			if(lflags & MLF_MIDDLE)
				y = (h - wg_h - mh) >> 1;
			else if(lflags & MLF_BOTTOM)
				y = h - wg_h - mh;
			
			if(y < 0) y = 0;
			
			y += top + p->margin.top;
			
			//位置・サイズ変更
			
			mWidgetMoveResize(p, x, y, wg_w, wg_h);
			
			p->fUI |= MWIDGET_UI_LAYOUTED;

			//次へ
			
			left += w;
			if(info_col->cell_size[col_no] & GRID_CELL_ADD_SEP) left += container->ct.gridSepCol;
			
			if(h > maxh) maxh = h;
		}
		
		top += maxh;
		if(info_row->cell_size[row_no] & GRID_CELL_ADD_SEP) top += container->ct.gridSepRow;
	}
	
	//
	
	mFree(info_col);
	mFree(info_row);
}
