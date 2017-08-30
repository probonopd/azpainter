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
 * キー関連
 *****************************************/

#include <stdio.h>
#include <string.h>

#include "mDef.h"
#include "mKeyDef.h"


//-------------------

static const char *keyname1[] = {
	"Break", "Pause", "Clear", "PrintScreen", "Escape",
	"NumLock", "ScrollLock", "CapsLock",
	"Enter", "Backspace", "Tab", "Delete", "Insert",
	"Shift", "Ctrl", "Alt", "Super", "Menu"
},
*keyname2[] = {
	"Space", "PageUp", "PageDown", "End", "Home",
	"Left", "Up", "Right", "Down"
},
*keyname3[] = {
	"Num(*)", "Num(+)", "Num(,)", "Num(-)", "Num(.)", "Num(/)"
};

//-------------------


int mKeyCodeToName(uint32_t c,char *buf,int bufsize)
{
	char m[32];

	m[0] = m[1] = 0;

	if(c >= MKEY_BREAK && c <= MKEY_MENU)
		strcpy(m, keyname1[c - MKEY_BREAK]);
	else if(c >= MKEY_SPACE && c <= MKEY_DOWN)
		strcpy(m, keyname2[c - MKEY_SPACE]);
	else if((c >= MKEY_0 && c <= MKEY_9) || (c >= MKEY_A && c <= MKEY_Z))
		m[0] = c;
	else if(c >= MKEY_NUM0 && c <= MKEY_NUM9)
		sprintf(m, "NUM%d", c - MKEY_NUM0);
	else if(c >= MKEY_NUM_MUL && c <= MKEY_NUM_DIV)
		strcpy(m, keyname3[c - MKEY_NUM_MUL]);
	else if(c >= MKEY_F1 && c <= MKEY_F24)
		sprintf(m, "F%d", c - MKEY_F1 + 1);

	//セット
	
	return mStrcpy(buf, m, bufsize);
}

