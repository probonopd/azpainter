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

/**********************************
 * ツールオプションデータのマクロ
 **********************************/

#ifndef MACRO_TOOL_OPTION_H
#define MACRO_TOOL_OPTION_H

/* <fill polygon>
 * [11](1)antialias | [7](4)pixelmode | [0](7)opacity {0-100}
 */

#define FILLPOLY_OPT_DEFAULT  ((1<<11) | 100)

#define FILLPOLY_GET_OPACITY(n)  (n & 127)
#define FILLPOLY_GET_PIXMODE(n)  ((n >> 7) & 15)
#define FILLPOLY_IS_ANTIALIAS(n) ((n & (1<<11)) != 0)

#define FILLPOLY_SET_OPACITY(p,n)    *(p) = (*(p) & ~127) | (n)
#define FILLPOLY_SET_PIXMODE(p,n)    *(p) = (*(p) & ~(15<<7)) | ((n) << 7)
#define FILLPOLY_TOGGLE_ANTIALIAS(p) *(p) ^= (1<<11)

/* <fill>
 * [22](1)disable ref | [18](4)type | [14](4)pixelmode | [7](7)diff | [0](7)opacity
 */

#define FILL_OPT_DEFAULT  100

#define FILL_GET_OPACITY(n)     (n & 127)
#define FILL_GET_COLOR_DIFF(n)  ((n >> 7) & 127)
#define FILL_GET_PIXMODE(n)     ((n >> 14) & 15)
#define FILL_GET_TYPE(n)        ((n >> 18) & 15)
#define FILL_IS_DISABLE_REF(n)  ((n & (1<<22)) != 0)

#define FILL_SET_OPACITY(p,n)       *(p) = (*(p) & ~127) | (n)
#define FILL_SET_COLOR_DIFF(p,n)    *(p) = (*(p) & ~(127<<7)) | ((n) << 7)
#define FILL_SET_PIXMODE(p,n)       *(p) = (*(p) & ~(15<<14)) | ((n) << 14)
#define FILL_SET_TYPE(p,n)          *(p) = (*(p) & ~(15<<18)) | ((n) << 18)
#define FILL_TOGGLE_DISABLE_REF(p)  *(p) ^= (1<<22)

/* <gradation>
 * [21](1) loop | [20](1)reverse | [19](1)custom | [11](8)selno | [7](4)pixmode | [0](7)opacity
 */

#define GRAD_OPT_DEFAULT  100

#define GRAD_GET_OPACITY(n)   (n & 127)
#define GRAD_GET_PIXMODE(n)   ((n >> 7) & 15)
#define GRAD_GET_SELNO(n)     ((n >> 11) & 255)
#define GRAD_IS_NOT_CUSTOM(n) (!(n & (1<<19)))
#define GRAD_IS_CUSTOM(n)     ((n & (1<<19)) != 0)
#define GRAD_IS_REVERSE(n)    ((n & (1<<20)) != 0)
#define GRAD_IS_LOOP(n)       ((n & (1<<21)) != 0)

#define GRAD_SET_OPACITY(p,n)  *(p) = (*(p) & ~127) | (n)
#define GRAD_SET_PIXMODE(p,n)  *(p) = (*(p) & ~(15<<7)) | ((n) << 7)
#define GRAD_SET_SELNO(p,n)    *(p) = (*(p) & ~(255<<11)) | ((n) << 11)
#define GRAD_SET_CUSTOM_OFF(p) *(p) &= ~(1<<19)
#define GRAD_SET_CUSTOM_ON(p)  *(p) |= (1<<19)
#define GRAD_TOGGLE_REVERSE(p) *(p) ^= (1<<20)
#define GRAD_TOGGLE_LOOP(p)    *(p) ^= (1<<21)

/* <magicwand>
 * [11](1)disable ref | [7](4)type | [0](7)diff
 */

#define MAGICWAND_OPT_DEFAULT  0

#define MAGICWAND_GET_COLOR_DIFF(n)  (n & 127)
#define MAGICWAND_GET_TYPE(n)        ((n >> 7) & 15)
#define MAGICWAND_IS_DISABLE_REF(n)  ((n & (1<<11)) != 0)

#define MAGICWAND_SET_COLOR_DIFF(p,n)   *(p) = (*(p) & ~127) | (n)
#define MAGICWAND_SET_TYPE(p,n)         *(p) = (*(p) & ~(15<<7)) | ((n) << 7)
#define MAGICWAND_TOGGLE_DISABLE_REF(p) *(p) ^= (1<<11)

/* <stamp> */

#define STAMP_OPT_DEFAULT 0

#define STAMP_IS_OVERWRITE(n)  ((n & 1) != 0)
#define STAMP_IS_LEFTTOP(n)    ((n & 2) != 0)

#define STAMP_TOGGLE_OVERWRITE(p)  *(p) ^= 1
#define STAMP_TOGGLE_LEFTTOP(p)    *(p) ^= 2

#endif
