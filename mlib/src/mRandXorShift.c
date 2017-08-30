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
 * 乱数生成 - XorShift (128bit)
 *****************************************/

#include <stdint.h>

static uint32_t g_xorshift[4] = {123456789, 362436069, 521288629, 88675123};


/****************//**

@defgroup rand_xorshift mRandXorShift
@brief 乱数生成 (XorShift 128bit)

@ingroup group_etc
@{

@file mRandXorShift.h

****************/


/** 初期化 */

void mRandXorShift_init(uint32_t seed)
{
	int i;

	g_xorshift[0] = 1812433253U * (seed ^ (seed >> 30)) + 1;

	for(i = 0; i <= 2; i++)
		g_xorshift[i + 1] = 1812433253U * (g_xorshift[i] ^ (g_xorshift[i] >> 30)) + (i + 2);
}

/** 32bit 値取得 */

uint32_t mRandXorShift_getUint32()
{
	uint32_t t;

	t = g_xorshift[0] ^ (g_xorshift[0] << 11);
	g_xorshift[0] = g_xorshift[1];
	g_xorshift[1] = g_xorshift[2];
	g_xorshift[2] = g_xorshift[3];
	g_xorshift[3] = (g_xorshift[3] ^ (g_xorshift[3] >> 19)) ^ (t ^ (t >> 8));

	return g_xorshift[3];
}

/** 0.0〜1.0 未満の double 値取得 */

double mRandXorShift_getDouble()
{
	return mRandXorShift_getUint32() * (1.0 / (UINT32_MAX + 1.0));
}

/** 指定範囲の int 値取得 */

int mRandXorShift_getIntRange(int min,int max)
{
	if(min >= max)
		return min;
	else
		return (int)(mRandXorShift_getDouble() * (max - min + 1)) + min;
}

/* @} */
