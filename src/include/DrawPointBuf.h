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

/************************************
 * 自由線描画時のポイントデータ処理
 ************************************/

#ifndef DRAWPOINTBUF_H
#define DRAWPOINTBUF_H

typedef struct
{
	double x,y,pressure;
}DrawPointBuf_pt;

//手ブレ補正タイプ
enum
{
	DRAWPOINTBUF_TYPE_NONE,
	DRAWPOINTBUF_TYPE_GAUSS,
	DRAWPOINTBUF_TYPE_LINEAR,
	DRAWPOINTBUF_TYPE_AVERAGE
};


void DrawPointBuf_setFirstPoint(double x,double y,double pressure);
void DrawPointBuf_addPoint(double x,double y,double pressure);

void DrawPointBuf_setSmoothing(int type,int strength);

mBool DrawPointBuf_getPoint(DrawPointBuf_pt *dst);
mBool DrawPointBuf_getFinishPoint(DrawPointBuf_pt *dst);

mBool DrawPointBuf_getPoint_int(mPoint *dst);
mBool DrawPointBuf_getFinishPoint_int(mPoint *dst);

#endif
