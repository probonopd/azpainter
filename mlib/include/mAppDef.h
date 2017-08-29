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

#ifndef MLIB_APPDEF_H
#define MLIB_APPDEF_H

typedef struct _mAppPrivate mAppPrivate;
typedef struct _mAppSystem  mAppSystem;

typedef struct _mApp
{
	mAppPrivate *pv;
	mAppSystem  *sys;

	mWidget *widgetRoot,
			*widgetOver,
			*widgetGrab,
			*widgetGrabKey;
	
	mFont *fontDefault;

	char *pathConfig,
		*pathData;

	int depth;
	uint32_t maskR,maskG,maskB;
	uint16_t filedialog_config[3];
	uint8_t flags,
		r_shift_left,
		g_shift_left,
		b_shift_left,
		r_shift_right,
		g_shift_right,
		b_shift_right;
}mApp;

extern mApp *g_mApp;


enum MAPP_FLAGS
{
	MAPP_FLAGS_DEBUG_EVENT  = 1<<0,
	MAPP_FLAGS_DISABLE_GRAB = 1<<1,
	MAPP_FLAGS_BLOCK_USER_ACTION = 1<<2
};

#define MAPP     (g_mApp)
#define MAPP_SYS (g_mApp->sys)
#define MAPP_PV  (g_mApp->pv)

#endif
