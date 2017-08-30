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
 * <X11> mPixbuf
 *****************************************/

#include <stdlib.h>

#define MINC_X11_UTIL
#define MINC_X11_XSHM
#include "mSysX11.h"

#include "mPixbuf.h"
#include "mPixbuf_pv.h"

#include "mWindowDef.h"

#ifdef HAVE_XEXT_XSHM
#include <sys/ipc.h>
#include <sys/shm.h>
#endif

//-------------------------

typedef struct
{
	mPixbuf b;
	mPixbufPrivate p;

	XImage *ximg;
#ifdef HAVE_XEXT_XSHM
	XShmSegmentInfo xshminfo;
#endif
}__mPixbufX11;

#define _PIXBUF(p)  ((__mPixbufX11 *)(p))

//-------------------------


/** 構造体確保 */

mPixbuf *__mPixbufAlloc(void)
{
	return (mPixbuf *)mMalloc(sizeof(__mPixbufX11), TRUE);
}

/** イメージ解放 */

void __mPixbufFree(mPixbuf *p)
{
	__mPixbufX11 *px11 = (__mPixbufX11 *)p;

#ifdef HAVE_XEXT_XSHM

	if(px11->xshminfo.shmaddr)
	{
		XShmDetach(XDISP, &px11->xshminfo);
	
		XDestroyImage(px11->ximg);
	
		shmdt(px11->xshminfo.shmaddr);

		px11->ximg = NULL;
		px11->xshminfo.shmaddr = NULL;
	}

#endif

	if(px11->ximg)
	{
		XDestroyImage(px11->ximg);
		px11->ximg = NULL;
	}
	
	p->buf = p->buftop = NULL;
}

/** イメージ作成 */

int __mPixbufCreate(mPixbuf *p,int w,int h)
{
	__mPixbufX11 *px11;
	XImage *ximg = NULL;
	
	__mPixbufFree(p);
	
	px11 = _PIXBUF(p);

	//作成 (XShm)

#ifdef HAVE_XEXT_XSHM

	if(MAPP_SYS->fSupport & MX11_SUPPORT_XSHM)
	{
		ximg = XShmCreateImage(XDISP, CopyFromParent,
					MAPP->depth, ZPixmap, NULL, &px11->xshminfo, w, h);
		
		if(ximg)
		{
			px11->xshminfo.shmid = shmget(IPC_PRIVATE, ximg->bytes_per_line * h, IPC_CREAT | 0777);
			
			if(px11->xshminfo.shmid < 0)
			{
				XDestroyImage(ximg);
				ximg = NULL;
			}
			else
			{
				px11->xshminfo.shmaddr = ximg->data = (char *)shmat(px11->xshminfo.shmid, 0, 0);
				px11->xshminfo.readOnly = 0;
				
				XShmAttach(XDISP, &px11->xshminfo);
			}
		}
	}
	
	if(!ximg) px11->xshminfo.shmaddr = NULL;
	
#endif

	//作成 (通常)
	
	if(!ximg)
	{
		//XImage
	
		ximg = XCreateImage(XDISP, CopyFromParent,
					MAPP->depth, ZPixmap, 0, NULL, w, h, 32, 0);
		
		if(!ximg) return -1;
		
		/* イメージバッファ確保
		 * [!] XDestroyImage() で free() を使って解放されるので、
		 *     通常の malloc() を使う。*/
		
		ximg->data = (char *)malloc(ximg->bytes_per_line * h);
		
		if(!ximg->data)
		{
			XDestroyImage(ximg);
			return -1;
		}
	}

	//情報
	
	px11->ximg = ximg;

	p->w     = w;
	p->h     = h;
	p->bpp   = (ximg->bits_per_pixel + 7) >> 3;
	p->pitch = p->pitch_dir = ximg->bytes_per_line;
	p->buf   = p->buftop = (unsigned char *)ximg->data;

	return 0;
}

/** ウィンドウに転送 */

void mPixbufRenderWindow(mPixbuf *p,mWindow *win,mBox *box)
{
#ifdef HAVE_XEXT_XSHM

	if(_PIXBUF(p)->xshminfo.shmaddr)
	{
		XShmPutImage(XDISP, WINDOW_XID(win), MAPP_SYS->gc_def,
			_PIXBUF(p)->ximg,
			box->x, box->y, box->x, box->y, box->w, box->h, False);
	}
	else
	{
		XPutImage(XDISP, WINDOW_XID(win), MAPP_SYS->gc_def,
			_PIXBUF(p)->ximg,
			box->x, box->y, box->x, box->y, box->w, box->h);
	}

#else

	XPutImage(XDISP, WINDOW_XID(win), MAPP_SYS->gc_def,
		_PIXBUF(p)->ximg,
		box->x, box->y, box->x, box->y, box->w, box->h);

#endif
}

