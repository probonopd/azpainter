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
 * mNanoTime - 共通
 *****************************************/

#include "mDef.h"
#include "mNanoTime.h"


/**
@defgroup nanotime mNanoTime
@brief ナノ時間関数

@ingroup group_util
@{

@file mNanoTime.h
*/


/** 指定ナノ秒を追加 */

void mNanoTimeAdd(mNanoTime *nt,uint64_t nsec)
{
	nsec += nt->nsec;
	
	nt->sec += nsec / (1000 * 1000 * 1000);
	nt->nsec = nsec % (1000 * 1000 * 1000);
}

/** 指定ミリ秒を追加 */

void mNanoTimeAddMilliSec(mNanoTime *nt,int msec)
{
	mNanoTimeAdd(nt, (uint64_t)msec * 1000 * 1000);
}

/** 時間を比較
 * 
 * @retval -1 nt1 の方が小さい
 * @retval 1  nt1 の方が大きい
 * @retval 0  nt1 == nt2 */

int mNanoTimeCompare(mNanoTime *nt1,mNanoTime *nt2)
{
	if(nt1->sec < nt2->sec)
		return -1;
	else if(nt1->sec > nt2->sec)
		return 1;
	else
	{
		//秒が同じ

		if(nt1->nsec < nt2->nsec)
			return -1;
		else if(nt1->nsec > nt2->nsec)
			return 1;
		else
			return 0;
	}
}

/** nt1 から nt2 を引いた値を取得
 * 
 * nt1 の方が小さい場合は失敗する。
 * 
 * @return nt1 の方が小さい場合 FALSE */

mBool mNanoTimeSub(mNanoTime *dst,mNanoTime *nt1,mNanoTime *nt2)
{
	if(mNanoTimeCompare(nt1, nt2) < 0)
		return FALSE;
	else
	{
		dst->sec = nt1->sec - nt2->sec;
		
		if(nt1->nsec >= nt2->nsec)
			dst->nsec = nt1->nsec - nt2->nsec;
		else
		{
			dst->sec--;
			dst->nsec = (int64_t)(nt1->nsec - nt2->nsec) + 1000 * 1000 * 1000;
		}
		
		return TRUE;
	}
}

/* @} */
