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
 * メニュー・サブ処理
 *****************************************/

#include "mDef.h"

#include "mMenu.h"
#include "mMenu_pv.h"

#include "mFont.h"
#include "mAccelerator.h"


//-----------------

#define _MENUITEM_SEP_H   5
#define _MENUITEM_SPACE_V 3

//-----------------


/** 選択できないアイテムか */

mBool __mMenuItemIsDisableItem(mMenuItem *p)
{
	return ((p->item.flags & (MMENUITEM_F_DISABLE | MMENUITEM_F_SEP)) != 0);
}

/** 有効なサブメニューか */

mBool __mMenuItemIsEnableSubmenu(mMenuItem *p)
{
	return (p->item.submenu
			&& !(p->item.flags & MMENUITEM_F_DISABLE)
			&& (p->item.submenu)->list.top);
}

/** ラジオグループのチェック */

void __mMenuItemCheckRadio(mMenuItem *item,int check)
{
	mMenuItem *p;
	
	//前方のフラグをOFF
	
	for(p = M_MENUITEM(item->i.prev);
			p && !(p->item.flags & MMENUITEM_F_SEP); p = M_MENUITEM(p->i.prev))
		M_FLAG_OFF(p->item.flags, MMENUITEM_F_CHECKED);

	//後ろのフラグをOFF
	
	for(p = M_MENUITEM(item->i.next);
			p && !(p->item.flags & MMENUITEM_F_SEP); p = M_MENUITEM(p->i.next))
		M_FLAG_OFF(p->item.flags, MMENUITEM_F_CHECKED);
	
	//自身
	
	if(check)
		M_FLAG_ON(item->item.flags, MMENUITEM_F_CHECKED);
	else
		M_FLAG_OFF(item->item.flags, MMENUITEM_F_CHECKED);
}

/** ホットキーからアイテム検索 */

mMenuItem *__mMenuFindByHotkey(mMenu *menu,char key)
{
	mMenuItem *p;
	
	for(p = M_MENUITEM(menu->list.top); p; p = M_MENUITEM(p->i.next))
	{
		if(p->hotkey == key && !__mMenuItemIsDisableItem(p))
			return p;
	}
	
	return NULL;
}


//=======================
// 初期化
//=======================


/** テキスト関連を初期化 */

void __mMenuInitText(mMenuItem *p,int flags)
{
	const char *pc;
	char c;

	//------ ショートカットキー文字列

	if(flags & MMENU_INITTEXT_SHORTCUT)
	{
		if(p->item.shortcutkey)
			p->sctext = mAcceleratorGetKeyText(p->item.shortcutkey);
	}

	//------ ラベル

	if(flags & MMENU_INITTEXT_LABEL)
	{
		//ラベルコピー
		
		if(p->item.label && (p->item.flags & MMENUITEM_F_LABEL_COPY))
		{
			p->labelcopy = mStrdup(p->item.label);
			p->item.label = p->labelcopy;
		}

		//ラベル文字列

		p->labelsrc = (p->item.label)? p->item.label: p->labelcopy;

		//ラベルからホットキー取得

		p->hotkey = 0;

		if(p->labelsrc)
		{
			for(pc = p->labelsrc; *pc; pc++)
			{
				if(*pc == '&')
				{
					c = *(++pc);

					if(c == '&')
						continue;
					else if((c >= '0' && c <= '9') ||
							(c >= 'A' && c <= 'Z') ||
							(c >= 'a' && c <= 'z'))
					{
						if(c >= 'a' && c <= 'z') c = c - 'a' + 'A';
					
						p->hotkey = c;
						break;
					}
				}
			}
		}
	}
}

/** 初期化 - 位置や幅 */

static void _init_size(mMenuItem *p,mFont *font)
{
	if(p->item.flags & MMENUITEM_F_SEP)
	{
		//セパレータ
		
		p->labelLeftW  = 1;
		p->labelRightW = 0;
		p->item.height = _MENUITEM_SEP_H;
	}
	else if(p->item.draw)
	{
		//独自描画
		
		p->labelLeftW = p->labelRightW = 0;
	}
	else if(p->item.label)
	{
		//------- 通常テキスト
				
		//テキスト
		
		p->labelLeftW  = mFontGetTextWidthHotkey(font, p->labelsrc, -1);
		p->labelRightW = mFontGetTextWidth(font, p->sctext, -1);
		
		//高さ
		
		p->item.height = font->height;

		if(p->item.height < 9) p->item.height = 9;
		
		p->item.height += _MENUITEM_SPACE_V * 2;
	}
}

/** ウィンドウ表示前の初期化 */

void __mMenuInit(mMenu *menu,mFont *font)
{
	mMenuItem *p;
	
	for(p = M_MENUITEM(menu->list.top); p; p = M_MENUITEM(p->i.next))
	{
		//描画幅など処理
		
		if(p->fTmp & MMENUITEM_TMPF_INIT_SIZE)
		{
			_init_size(p, font);
		
			M_FLAG_OFF(p->fTmp, MMENUITEM_TMPF_INIT_SIZE);
		}
	}
}
