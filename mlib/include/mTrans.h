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

#ifndef MLIB_TRANS_H
#define MLIB_TRANS_H

#define M_TR_G(id)      mTransSetGroup(id)
#define M_TR_T(id)      mTransGetText(id)
#define M_TR_T2(gid,id) mTransGetText2(gid, id)

#define M_TRGROUP_SYS   0xffff

enum M_TRSYS
{
	M_TRSYS_OK = 1,
	M_TRSYS_CANCEL,
	M_TRSYS_YES,
	M_TRSYS_NO,
	M_TRSYS_SAVE,
	M_TRSYS_SAVENO,
	M_TRSYS_ABORT,
	M_TRSYS_NOTSHOW_THISMES,
	M_TRSYS_OPEN,
	M_TRSYS_FILENAME,
	M_TRSYS_TYPE,
	M_TRSYS_PREVIEW,
	M_TRSYS_FONTSTYLE,
	M_TRSYS_FONTSIZE,
	M_TRSYS_FONTWEIGHT,
	M_TRSYS_FONTSLANT,
	M_TRSYS_FONTRENDER,
	M_TRSYS_FONTPREVIEWTEXT,
	M_TRSYS_FILESIZE,
	M_TRSYS_FILEMODIFY,
	M_TRSYS_SHOW_HIDDEN_FILES,

	M_TRSYS_TITLE_OPENFILE = 1000,
	M_TRSYS_TITLE_SAVEFILE,
	M_TRSYS_TITLE_SELECTDIR,
	M_TRSYS_TITLE_SELECTCOLOR,
	M_TRSYS_TITLE_SELECTFONT,

	M_TRSYS_MES_OVERWRITE = 2000,
	M_TRSYS_MES_FILENAME_INCORRECT
};

#ifdef __cplusplus
extern "C" {
#endif

void mTransSetGroup(uint16_t groupid);
const char *mTransGetText(uint16_t strid);
const char *mTransGetTextRaw(uint16_t strid);
const char *mTransGetTextDef(uint16_t strid);
const char *mTransGetText2(uint16_t groupid,uint16_t strid);
const char *mTransGetText2Raw(uint16_t groupid,uint16_t strid);

#ifdef __cplusplus
}
#endif

#endif
