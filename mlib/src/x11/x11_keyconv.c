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
 * <X11> キーコード変換
 *****************************************/

#include <string.h>

#define MINC_X11_XKB
#include "mSysX11.h"

#include <X11/keysym.h>

#include "mKeyDef.h"


/** KeySym から MKEY_* のキーコードに変換 */

uint32_t __mSystemKeyConvert(uint32_t key)
{
	uint32_t k;
	
	if(key >= '0' && key <= '9')
		return key;
	
	if(key >= 'A' && key <= 'Z')
		return key;
	
	if(key >= 'a' && key <= 'z')
		return 'A' + (key - 'a');
	
	if(key >= XK_KP_0 && key <= XK_KP_9)
		return MKEY_NUM0 + (key - XK_KP_0);
	
	if(key >= XK_F1 && key <= XK_F35)
		return (key <= XK_F24)? MKEY_F1 + (key - XK_F1): MKEY_UNKNOWN;
	
	switch(key)
	{
		case XK_space:
		case XK_KP_Space:
			k = MKEY_SPACE; break;

		case XK_Shift_L:
		case XK_Shift_R:
			k = MKEY_SHIFT; break;
		case XK_Control_L:
		case XK_Control_R:
			k = MKEY_CONTROL; break;
		case XK_Alt_L:
		case XK_Alt_R:
			k = MKEY_ALT; break;
		case XK_Super_L:
		case XK_Super_R:
			k = MKEY_SUPER; break;
	
		case XK_Return:
		case XK_KP_Enter:
			k = MKEY_ENTER; break;
		case XK_BackSpace:
			k = MKEY_BACKSPACE; break;
		case XK_Tab:
		case XK_KP_Tab:
			k = MKEY_TAB; break;
		case XK_Delete:
		case XK_KP_Delete:
			k = MKEY_DELETE; break;
		case XK_Insert:
		case XK_KP_Insert:
			k = MKEY_INSERT; break;

		case XK_Escape:
			k = MKEY_ESCAPE; break;

		case XK_Page_Up:
		case XK_KP_Page_Up:
			k = MKEY_PAGEUP; break;
		case XK_Page_Down:
		case XK_KP_Page_Down:
			k = MKEY_PAGEDOWN; break;
		case XK_End:
		case XK_KP_End:
			k = MKEY_END; break;
		case XK_Home:
		case XK_KP_Home:
			k = MKEY_HOME; break;
		case XK_Left:
		case XK_KP_Left:
			k = MKEY_LEFT; break;
		case XK_Up:
		case XK_KP_Up:
			k = MKEY_UP; break;
		case XK_Right:
		case XK_KP_Right:
			k = MKEY_RIGHT; break;
		case XK_Down:
		case XK_KP_Down:
			k = MKEY_DOWN; break;

		case XK_KP_Multiply:
			k = MKEY_NUM_MUL; break;
		case XK_KP_Add:
			k = MKEY_NUM_ADD; break;
		case XK_KP_Separator:
			k = MKEY_NUM_SEP; break;
		case XK_KP_Subtract:
			k = MKEY_NUM_SUB; break;
		case XK_KP_Decimal:
			k = MKEY_NUM_DECIMAL; break;
		case XK_KP_Divide:
			k = MKEY_NUM_DIV; break;

		case XK_Menu:
			k = MKEY_MENU; break;

		case XK_Num_Lock:
			k = MKEY_NUM_LOCK; break;
		case XK_Scroll_Lock:
			k = MKEY_SCROLL_LOCK; break;
		case XK_Caps_Lock:
			k = MKEY_CAPS_LOCK; break;
		
		case XK_Break:
			k = MKEY_BREAK; break;
		case XK_Pause:
			k = MKEY_PAUSE; break;
		case XK_Clear:
			k = MKEY_CLEAR; break;
		case XK_Print:
			k = MKEY_PRINTSCREEN; break;

		default:
			k = MKEY_UNKNOWN; break;
	}
	
	return k;
}

/** 生のキーコードから mlib のキーコード取得
 *
 * @ingroup main */

uint32_t mKeyRawToCode(uint32_t key)
{
	KeySym ks;

	ks = XkbKeycodeToKeysym(XDISP, key, 0, 0);

	if(ks == NoSymbol)
		return MKEY_UNKNOWN;
	else
		return __mSystemKeyConvert(ks);
}

/** 生のキーコードからキー名を取得
 *
 * @ingroup main
 * @return 文字数 */

int mRawKeyCodeToName(int key,char *buf,int bufsize)
{
	KeySym ks;
	char *name;
	int len;

	buf[0] = 0;

	//キーコード => KeySym

	ks = XkbKeycodeToKeysym(XDISP, key, 0, 0);

	if(ks == NoSymbol) return 0;

	//KeySym => 文字列

	name = XKeysymToString(ks);
	if(!name) return 0;

	len = strlen(name);
	if(len >= bufsize) len = bufsize - 1;

	memcpy(buf, name, len);
	buf[len] = 0;

	return len;
}
