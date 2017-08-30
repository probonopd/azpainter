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
 * x11_clipboard
 *****************************************/

#include <string.h>

#define MINC_X11_ATOM
#include "mSysX11.h"

#include "mWindowDef.h"
#include "x11_clipboard.h"
#include "x11_util.h"


//-------------------------

struct _mX11Clipboard
{
	int dattype;
	void *datbuf;
	uint32_t datsize;
	
	Atom *atoms;
	int atomNum;
	
	int (*callback)(mX11Clipboard *,mX11Clipboard_cb *);
};

#define _BASEATOM_NUM  2

static int _sendDefaultText(mX11Clipboard *p,mX11Clipboard_cb *dat);

//-------------------------


//========================
// mClipboard.h
//========================


/** クリップボードからテキスト取得 */

char *mClipboardGetText(mWidget *wg)
{
	return mX11ClipboardGetText(MAPP_SYS->clipb, wg->toplevel);
}

/** クリップボードにテキストセット
 * 
 * @param size NULL 文字も含めたサイズ */

mBool mClipboardSetText(mWidget *wg,const char *text,int size)
{
	if(!text || size <= 0) return FALSE;

	return mX11ClipboardSetDat(MAPP_SYS->clipb, wg->toplevel,
		MX11CLIPB_DATTYPE_TEXT, text, size,
		NULL, 0, NULL);
}

/** 終了後もデータを扱えるように保存 */

mBool mClipboardSave(mWidget *wg)
{
	return mX11ClipboardSaveManager(MAPP_SYS->clipb, wg->toplevel, 0);
}


//========================
// sub
//========================


/** アトムリストセット
 * 
 * @param atoms  NULL でデフォルト */

static mBool _setAtomList(mX11Clipboard *p,Atom *atoms,int atom_num)
{
	Atom *pd;

	//タイプ別のデフォルトアトム数
	
	if(!atoms)
	{
		if(p->dattype == MX11CLIPB_DATTYPE_TEXT)
			atom_num = 2;
		else
			return FALSE;
	}
	
	//確保
	
	p->atoms = (Atom *)mMalloc(sizeof(Atom) * (atom_num + _BASEATOM_NUM), FALSE);
	if(!p->atoms) return FALSE;
	
	p->atomNum = atom_num;
	
	//セット
	
	pd = p->atoms;
	
	if(atoms)
	{
		memcpy(pd, atoms, sizeof(Atom) * atom_num);
		pd += atom_num;
	}
	else
	{
		//デフォルト
		
		switch(p->dattype)
		{
			case MX11CLIPB_DATTYPE_TEXT:
				*(pd++) = MX11ATOM(UTF8_STRING);
				*(pd++) = MX11ATOM(COMPOUND_TEXT);
				break;
		}
	}

	//ベースのアトム
	
	*(pd++) = MX11ATOM(TARGETS);
	*(pd++) = MX11ATOM(MULTIPLE);
	
	return TRUE;
}


//========================


/** 解放 */

void mX11ClipboardDestroy(mX11Clipboard *p)
{
	if(p)
	{
		mX11ClipboardFreeDat(p);
		mFree(p);
	}
}

/** データ解放 */

void mX11ClipboardFreeDat(mX11Clipboard *p)
{
	M_FREE_NULL(p->datbuf);
	M_FREE_NULL(p->atoms);
	
	p->dattype = MX11CLIPB_DATTYPE_NONE;
}

/** 作成 */

mX11Clipboard *mX11ClipboardNew(void)
{
	return (mX11Clipboard *)mMalloc(sizeof(mX11Clipboard), TRUE);
}

/** クリップボードにデータをセット */

mBool mX11ClipboardSetDat(mX11Clipboard *p,
	mWindow *win,
	int dattype,const void *datbuf,uint32_t datsize,
	Atom *atom_list,int atom_num,
	int (*callback)(mX11Clipboard *,mX11Clipboard_cb *))
{
	mX11ClipboardFreeDat(p);
	
	//データコピー
	
	p->datbuf = mMalloc(datsize, FALSE);
	if(!p->datbuf) return FALSE;
	
	memcpy(p->datbuf, datbuf, datsize);
	
	//

	p->dattype = dattype;
	p->datsize = datsize;
	p->callback = callback;
	
	//アトムリストセット
	
	if(!_setAtomList(p, atom_list, atom_num))
	{
		mX11ClipboardFreeDat(p);
		return FALSE;
	}
	
	//デフォルトコールバックセット
	
	if(!callback)
	{
		if(dattype == MX11CLIPB_DATTYPE_TEXT)
			p->callback = _sendDefaultText;
	}
	
	//クリップボード所有権セット

	XSetSelectionOwner(XDISP, MX11ATOM(CLIPBOARD), WINDOW_XID(win), MAPP_SYS->timeLast);

	return TRUE;
}

/** クリップボードマネージャにデータを保存
 * 
 * データはアプリケーション自身が保持しているので、アプリケーションが終了した場合は
 * クリップボードデータは消えることになる。 @n
 * アプリケーションが終了した後もクリップボードにデータを置いておきたい場合は、
 * この関数でクリップボードマネージャの方にデータを保存させる。 @n
 * @n
 * [!] 各対応データ形式ごとにデータが保存されるので、コールバック関数が複数回来る。 @n
 * [!] コールバック呼び出し時は、multiple が 1。
 * 
 * @param dattype 0 で現在のデータ。それ以外で指定タイプの場合のみ保存。
 */

mBool mX11ClipboardSaveManager(mX11Clipboard *p,mWindow *win,int dattype)
{
	Window id;

	if(p->dattype == MX11CLIPB_DATTYPE_NONE)
		return FALSE;
	
	if(dattype && dattype != p->dattype)
		return FALSE;
	
	//マネージャの所有者がいない

	if(XGetSelectionOwner(XDISP, MX11ATOM(CLIPBOARD_MANAGER)) == None)
		return FALSE;

	/* 保存要求
	 * [!] 先にプロパティを消しておかないと MULTIPLE で property=0 になる */

	id = WINDOW_XID(win);

	XDeleteProperty(XDISP, id, MX11ATOM(MLIB_SELECTION));

	XSetSelectionOwner(XDISP, MX11ATOM(CLIPBOARD), id, MAPP_SYS->timeLast);

	XConvertSelection(XDISP,MX11ATOM(CLIPBOARD_MANAGER),
		mX11GetAtom("SAVE_TARGETS"), MX11ATOM(MLIB_SELECTION),
		id, MAPP_SYS->timeLast);

	XSync(XDISP, False);

	return TRUE;
}


//=============================
// データ取得
//=============================


/** クリップボードからテキスト取得
 * 
 * @return 確保された文字列のポインタ。mFree() で解放する。 */

char *mX11ClipboardGetText(mX11Clipboard *p,mWindow *win)
{
	Atom atom[3],type;

	if(p->dattype == MX11CLIPB_DATTYPE_TEXT)
	{
		//自身がデータを持っている場合
		
		return mStrdup((char *)p->datbuf);
	}
	else
	{
		//------- 他クライアントから
		
		//利用可能なテキストタイプ
		
		atom[0] = MX11ATOM(UTF8_STRING);
		atom[1] = MX11ATOM(COMPOUND_TEXT);
		atom[2] = XA_STRING;
		
		type = mX11GetSelectionTargetType(WINDOW_XID(win),
			MX11ATOM(CLIPBOARD), atom, 3);
		
		if(type == 0) return NULL;
		
		//テキスト取得
		
		if(type == MX11ATOM(COMPOUND_TEXT))
		{
			return mX11GetSelectionCompoundText(WINDOW_XID(win),
				MX11ATOM(CLIPBOARD));
		}
		else
		{
			//UTF-8/ASCII
			
			return mX11GetSelectionDat(WINDOW_XID(win),
				MX11ATOM(CLIPBOARD), type, TRUE, NULL);
		}
	}
	
	return NULL;
}


//=============================
// X イベント
//=============================


/** テキストのデータを送る
 * 
 * NULL 文字は送らない。 */

int _sendDefaultText(mX11Clipboard *p,mX11Clipboard_cb *dat)
{
	if(dat->type == MX11ATOM(COMPOUND_TEXT))
	{
		mX11SetPropertyCompoundText(dat->winid, dat->prop,
			(char *)dat->buf, dat->size - 1);
	}
	else
	{
		mX11SetProperty8(dat->winid, dat->prop, dat->type,
			dat->buf, dat->size - 1, FALSE);
	}

	return 1;
}

/** 指定フォーマットのデータを送る */

static Atom _sendDat(mX11Clipboard *p,
	Window winid,Atom prop,Atom type,mBool multiple)
{
	mX11Clipboard_cb dat;
	int i,f;
	
	dat.buf = p->datbuf;
	dat.size = p->datsize;
	dat.winid = winid;
	dat.prop = prop;
	dat.type = type;
	dat.multiple = multiple;
	
	//指定フォーマットに対応しているか
	
	for(i = 0, f = 0; i < p->atomNum; i++)
	{
		if(p->atoms[i] == type) { f = 1; break; }
	}
	
	if(!f) return 0;
	
	//データ送る
	
	if(p->callback)
		f = (p->callback)(p, &dat);
	else
		f = 0;
	
	return (f)? prop: 0;
}

/** 複数タイプのデータセット */

static void _sendMultiple(mX11Clipboard *p,Window id,Atom prop)
{
	Atom *atoms;
	int num,i;
	
	//プロパティとタイプのペアを取得し、各タイプごとにセット
	
	atoms = mX11GetProperty32(id, prop, mX11GetAtom("ATOM_PAIR"), &num);
	if(!atoms) return;
	
	for(i = 0; i < num; i += 2)
		_sendDat(p, id, atoms[i], atoms[i + 1], TRUE);
	
	mFree(atoms);
}

/** SelectionRequest イベント時の処理 (selection="CLIPBOARD") */

void mX11ClipboardSelectionRequest(mX11Clipboard *p,
	XSelectionRequestEvent *ev)
{
	XSelectionEvent xev;
	Atom prop;

	prop = ev->property;

	if(p->dattype == MX11CLIPB_DATTYPE_NONE)
		prop = 0;
	else if(ev->target == MX11ATOM(TARGETS))
	{
		/* 他クライアントからの、対応可能なフォーマットタイプの要求 */
		
		mX11SetPropertyAtom(ev->requestor, ev->property,
			p->atoms, p->atomNum + _BASEATOM_NUM);
	}
	else if(ev->target == MX11ATOM(MULTIPLE))
	{
		/* クリップボードマネージャへのデータ保存時 */
		
		_sendMultiple(p, ev->requestor, ev->property);
	}
	else
	{
		/* 他クライアントからの、指定フォーマットによるデータ要求 */
		
		prop = _sendDat(p, ev->requestor, ev->property, ev->target, FALSE);
	}

	//------ 返信イベント送信

	xev.type       = SelectionNotify;
	xev.send_event = 1;
	xev.display    = XDISP;
	xev.requestor  = ev->requestor;
	xev.selection  = ev->selection;
	xev.target     = ev->target;
	xev.property   = prop;
	xev.time       = ev->time;

	XSendEvent(xev.display, xev.requestor, False, 0, (XEvent *)&xev);
}
