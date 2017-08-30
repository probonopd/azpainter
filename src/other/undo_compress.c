/*$
 Copyright (C) 2013-2017 Azel.

 This file is part of AzPainter.

 AzPainter is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 AzPainter is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
$*/

/*****************************************
 * アンドゥ用圧縮/展開
 *****************************************/

#include <string.h>

#include "mDef.h"



//=========================
// 1byte 単位
//=========================


/** 圧縮 (1byte) */

int UndoByteEncode(uint8_t *dst,uint8_t *src,int srcsize)
{
	uint8_t *ps,*psend,*ptop,val;
	int encsize = 0,num;

	ps = src;
	psend = src + srcsize;

	while(ps < psend)
	{
		if(ps == psend - 1 || *ps != ps[1])
		{
			//非連続

			for(ptop = ps; ps < psend - 1 && *ps != ps[1] && ps - ptop < 128; ps++);

			if(ps == psend - 1) ps++;

			num = ps - ptop;

			if(encsize + 1 + num >= srcsize) return srcsize;

			*dst = num - 1;
			memcpy(dst + 1, ptop, num);

			dst += 1 + num;
			encsize += 1 + num;
		}
		else
		{
			//連続

			val = *ps;
			ptop = ps;

			for(ps += 2; ps < psend && *ps == val && ps - ptop < 128; ps++);

			if(encsize + 2 >= srcsize) return srcsize;

			*dst = (char)(-(ps - ptop) + 1);
			dst[1] = val;

			dst += 2;
			encsize += 2;
		}
	}

	return encsize;
}

/** 展開 (1byte) */

void UndoByteDecode(uint8_t *dst,uint8_t *src,int srcsize)
{
	uint8_t *ps,*psend;
	int len;
	char lenb;

	ps = src;
	psend = src + srcsize;

	while(ps < psend)
	{
		lenb = *((char *)ps);
		ps++;

		if(lenb >= 0)
		{
			//非連続

			len = lenb + 1;

			memcpy(dst, ps, len);

			ps += len;
		}
		else
		{
			//連続

			len = -lenb + 1;

			memset(dst, *ps, len);

			ps++;
		}

		dst += len;
	}
}


//=========================
// 2byte 単位
//=========================


/** 圧縮 (2byte) */

int UndoWordEncode(uint8_t *dst,uint8_t *src,int srcsize)
{
	uint16_t *ps,*psend,*ptop,val;
	int encsize = 0,num,addsize;

	ps = (uint16_t *)src;
	psend = (uint16_t *)(src + srcsize);

	while(ps < psend)
	{
		if(ps == psend - 1 || *ps != ps[1])
		{
			//非連続

			for(ptop = ps; ps < psend - 1 && *ps != ps[1] && ps - ptop < 128; ps++);

			if(ps == psend - 1) ps++;

			num = ps - ptop;
			addsize = 1 + (num << 1);

			if(encsize + addsize >= srcsize) return srcsize;

			*dst = num - 1;
			memcpy(dst + 1, ptop, num << 1);

			dst += addsize;
			encsize += addsize;
		}
		else
		{
			//連続

			val = *ps;
			ptop = ps;

			for(ps += 2; ps < psend && *ps == val && ps - ptop < 128; ps++);

			if(encsize + 3 >= srcsize) return srcsize;

			*dst = (char)(-(ps - ptop) + 1);
			*((uint16_t *)(dst + 1)) = val;

			dst += 3;
			encsize += 3;
		}
	}

	return encsize;
}

/** 展開 (2byte) */

void UndoWordDecode(uint8_t *dst,uint8_t *src,int srcsize)
{
	uint8_t *ps,*psend;
	uint16_t *pd,val;
	int len,size;
	char lenb;

	pd = (uint16_t *)dst;
	ps = src;
	psend = src + srcsize;

	while(ps < psend)
	{
		lenb = *((char *)ps);
		ps++;

		if(lenb >= 0)
		{
			//非連続

			len = lenb + 1;
			size = len << 1;

			memcpy(pd, ps, size);

			ps += size;
			pd += len;
		}
		else
		{
			//連続

			len = -lenb + 1;

			val = *((uint16_t *)ps);

			for(; len > 0; len--)
				*(pd++) = val;

			ps += 2;
		}
	}
}

