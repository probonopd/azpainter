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

#ifndef MLIB_WIDGETDEF_H
#define MLIB_WIDGETDEF_H

struct _mWidget
{
	mWindow *toplevel;
	mWidget *parent,
		*prev,*next,
		*first,*last,
		*notifyWidget;
	mFont *font;
	mCursor cursorCur;
	
	int x,y,w,h,
		id,
		absX,absY,
		hintW,hintH,
		imX,imY;
	uint16_t initW,initH,
		hintOverW,hintOverH,
		hintMinW,hintMinH;
	uint32_t fState,
		fOption,
		fLayout,
		fUI,
		fEventFilter;
	uint8_t fType,
		fAcceptKeys,
		notifyTarget,
		notifyTargetInterrupt;
	intptr_t param;
	
	mWidgetSpacing margin;
	
	void (*destroy)(mWidget *);
	int (*event)(mWidget *,mEvent *);
	void (*draw)(mWidget *,mPixbuf *);
	void (*drawBkgnd)(mWidget *,mPixbuf *,mBox *);
	void (*calcHint)(mWidget *);
	void (*layout)(mWidget *);
	void (*onSize)(mWidget *);
	int (*onDND)(mWidget *,char **files);
};


enum MWIDGET_TYPE_FLAGS
{
	MWIDGET_TYPE_CONTAINER   = 1<<0,
	MWIDGET_TYPE_WINDOW      = 1<<1,
	MWIDGET_TYPE_CHECKBUTTON = 1<<2
};

enum MWIDGET_STATE_FLAGS
{
	MWIDGET_STATE_VISIBLE       = 1<<0,
	MWIDGET_STATE_ENABLED       = 1<<1,
	MWIDGET_STATE_FOCUSED       = 1<<2,
	MWIDGET_STATE_TAKE_FOCUS    = 1<<3,
	MWIDGET_STATE_DEFAULT_FOCUS = 1<<4,
	MWIDGET_STATE_ENTER_DEFAULT = 1<<5,
	MWIDGET_STATE_ENABLE_DROP   = 1<<6
};

enum MWIDGET_OPTION_FLAGS
{
	MWIDGET_OPTION_NO_DRAW_BKGND   = 1<<0,
	MWIDGET_OPTION_DESTROY_CURSOR  = 1<<1,
	MWIDGET_OPTION_ONFOCUS_ALLKEY_TO_WINDOW  = 1<<2,
	MWIDGET_OPTION_ONFOCUS_NORMKEY_TO_WINDOW = 1<<3,
	MWIDGET_OPTION_DISABLE_SCROLL_EVENT = 1<<4
};

enum MWIDGET_UI_FLAGS
{
	MWIDGET_UI_UPDATE       = 1<<0,
	MWIDGET_UI_FOLLOW_DRAW  = 1<<1,
	MWIDGET_UI_DRAW         = 1<<2,
	MWIDGET_UI_FOLLOW_CALC  = 1<<3,
	MWIDGET_UI_CALC         = 1<<4,
	MWIDGET_UI_LAYOUTED     = 1<<5,
	MWIDGET_UI_DRAWED_BKGND = 1<<6
};

enum MWIDGET_EVENTFILTER_FLAGS
{
	MWIDGET_EVENTFILTER_POINTER = 1<<0,
	MWIDGET_EVENTFILTER_SCROLL  = 1<<1,
	MWIDGET_EVENTFILTER_KEY     = 1<<2,
	MWIDGET_EVENTFILTER_CHAR    = 1<<3,
	MWIDGET_EVENTFILTER_STRING  = 1<<4,
	MWIDGET_EVENTFILTER_PENTABLET = 1<<5
};

enum MWIDGET_ACCEPTKEY_FLAGS
{
	MWIDGET_ACCEPTKEY_TAB    = 1<<0,
	MWIDGET_ACCEPTKEY_ENTER  = 1<<1,
	MWIDGET_ACCEPTKEY_ESCAPE = 1<<2,
	
	MWIDGET_ACCEPTKEY_ALL = 0xff
};

enum MWIDGET_NOTIFY_TARGET
{
	MWIDGET_NOTIFYTARGET_PARENT_NOTIFY,
	MWIDGET_NOTIFYTARGET_SELF,
	MWIDGET_NOTIFYTARGET_PARENT,
	MWIDGET_NOTIFYTARGET_PARENT2,
	MWIDGET_NOTIFYTARGET_TOPLEVEL,
	MWIDGET_NOTIFYTARGET_WIDGET
};

enum MWIDGET_NOTIFY_TARGET_INTERRUPT
{
	MWIDGET_NOTIFYTARGET_INT_NONE,
	MWIDGET_NOTIFYTARGET_INT_SELF
};

enum MWIDGET_LAYOUT_FLAGS
{
	MLF_FIX_HINT_W = 0,
	MLF_FIX_HINT_H = 0,
	MLF_FIX_W      = 1<<0,
	MLF_FIX_H      = 1<<1,
	MLF_EXPAND_X   = 1<<2,
	MLF_EXPAND_Y   = 1<<3,
	MLF_EXPAND_W   = 1<<4,
	MLF_EXPAND_H   = 1<<5,

	MLF_LEFT       = 0,
	MLF_TOP        = 0,
	MLF_RIGHT      = 1<<6,
	MLF_BOTTOM     = 1<<7,
	MLF_CENTER     = 1<<8,
	MLF_MIDDLE     = 1<<9,

	MLF_GRID_COL_W = 1<<10,
	MLF_GRID_ROW_H = 1<<11,

	MLF_CENTER_XY = MLF_CENTER | MLF_MIDDLE,
	MLF_EXPAND_XY = MLF_EXPAND_X | MLF_EXPAND_Y,
	MLF_EXPAND_WH = MLF_EXPAND_W | MLF_EXPAND_H,
	MLF_FIX_WH    = MLF_FIX_W | MLF_FIX_H
};

#endif
