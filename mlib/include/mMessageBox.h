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

#ifndef MLIB_MESSAGEBOX_H
#define MLIB_MESSAGEBOX_H

#ifdef __cplusplus
extern "C" {
#endif

enum MMESBOX_BUTTONS
{
	MMESBOX_OK      = 1<<0,
	MMESBOX_CANCEL  = 1<<1,
	MMESBOX_YES     = 1<<2,
	MMESBOX_NO      = 1<<3,
	MMESBOX_SAVE    = 1<<4,
	MMESBOX_SAVENO  = 1<<5,
	MMESBOX_ABORT   = 1<<6,
	MMESBOX_NOTSHOW = 1<<30,

	MMESBOX_OKCANCEL = MMESBOX_OK | MMESBOX_CANCEL,
	MMESBOX_YESNO = MMESBOX_YES | MMESBOX_NO
};

uint32_t mMessageBox(mWindow *owner,const char *title,const char *message,
	uint32_t btts,uint32_t defbtt);

void mMessageBoxErr(mWindow *owner,const char *message);
void mMessageBoxErrTr(mWindow *owner,uint16_t groupid,uint16_t strid);

#ifdef __cplusplus
}
#endif

#endif
