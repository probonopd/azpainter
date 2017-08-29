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

#ifndef MLIB_EVENT_H
#define MLIB_EVENT_H

typedef struct
{
	uint32_t code,
		sys_vcode,
		state;
	int rawcode;
	mBool bGrab;
}mEventKey;

typedef struct
{
	char ch;
}mEventChar;

typedef struct
{
	int flags,x,y,w,h;
}mEventConfigure;

typedef struct
{
	int type,x,y,rootx,rooty,btt;
	uint32_t state;
}mEventPointer;

typedef struct
{
	int dir;
	uint32_t state;
}mEventScroll;

typedef struct
{
	int bOut,by;
}mEventFocus;

typedef struct
{
	mWidget *widgetFrom;
	int type,id;
	intptr_t param1,param2;
}mEventNotify;

typedef struct
{
	uint32_t id;
	intptr_t param;
}mEventTimer;

typedef struct
{
	mMenu *menu;
	int itemID;
	mBool bMenuBar;
}mEventMenuPopup;

typedef struct
{
	int id,by;
	intptr_t param;
}mEventCommand;

typedef struct
{
	int len;
}mEventString;

typedef struct
{
	int type,btt,device_id;
	uint32_t state,flags;
	double x,y,rootx,rooty,pressure;
	mBool bPenTablet;
}mEventPenTablet;

/*------------*/

typedef struct _mEvent
{
	int type;
	mWidget *widget;
	void *data;
	
	union {
		mEventKey key;
		mEventChar ch;
		mEventString str;
		mEventPointer pt;
		mEventScroll scr;
		mEventConfigure config;
		mEventFocus focus;
		mEventTimer timer;
		mEventNotify notify;
		mEventCommand cmd;
		mEventMenuPopup popup;
		mEventPenTablet pen;
	};
}mEvent;

/*------------*/

enum MEVENT_TYPE
{
	MEVENT_QUIT = 1,
	MEVENT_CLOSE,
	MEVENT_MAP,
	MEVENT_UNMAP,
	MEVENT_CONFIGURE,
	MEVENT_FOCUS,
	MEVENT_ENTER,
	MEVENT_LEAVE,
	MEVENT_KEYDOWN,
	MEVENT_KEYUP,
	MEVENT_CHAR,
	MEVENT_STRING,
	MEVENT_POINTER,
	MEVENT_POINTER_MODAL,
	MEVENT_SCROLL,
	MEVENT_TIMER,
	MEVENT_NOTIFY,
	MEVENT_COMMAND,
	MEVENT_MENU_POPUP,
	MEVENT_CONSTRUCT,
	MEVENT_PENTABLET,
	MEVENT_PENTABLET_MODAL,

	MEVENT_USER = 10000
};

enum MEVENT_POINTER_TYPE
{
	MEVENT_POINTER_TYPE_MOTION = 0,
	MEVENT_POINTER_TYPE_PRESS,
	MEVENT_POINTER_TYPE_RELEASE,
	MEVENT_POINTER_TYPE_DBLCLK
};

enum MEVENT_CONFIGURE_FLAGS
{
	MEVENT_CONFIGURE_FLAGS_MOVE = 1,
	MEVENT_CONFIGURE_FLAGS_SIZE = 2
};

enum MEVENT_SCROLL_DIR
{
	MEVENT_SCROLL_DIR_UP,
	MEVENT_SCROLL_DIR_DOWN,
	MEVENT_SCROLL_DIR_LEFT,
	MEVENT_SCROLL_DIR_RIGHT
};

enum MEVENT_FOCUS_BY
{
	MEVENT_FOCUS_BY_UNKNOWN,
	MEVENT_FOCUS_BY_WINDOW,
	MEVENT_FOCUS_BY_TABMOVE,
	MEVENT_FOCUS_BY_UNGRAB
};

enum MEVENT_COMMAND_BY
{
	MEVENT_COMMAND_BY_UNKNOWN,
	MEVENT_COMMAND_BY_MENU,
	MEVENT_COMMAND_BY_ACCEL,
	MEVENT_COMMAND_BY_ICONBUTTONS_BUTTON,
	MEVENT_COMMAND_BY_ICONBUTTONS_DROP
};

enum MEVENT_PENTABLET_FLAGS
{
	MEVENT_PENTAB_FLAGS_STYLUS = 1<<0,
	MEVENT_PENTAB_FLAGS_ERASER = 1<<1
};

#endif
