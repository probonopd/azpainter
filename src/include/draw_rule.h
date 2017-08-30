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
 * DrawData 定規関連
 **********************************/

#ifndef DRAW_RULE_H
#define DRAW_RULE_H

enum
{
	RULE_TYPE_OFF,
	RULE_TYPE_PARALLEL_LINE,
	RULE_TYPE_GRID_LINE,
	RULE_TYPE_CONC_LINE,
	RULE_TYPE_CIRCLE,
	RULE_TYPE_ELLIPSE,

	RULE_TYPE_NUM
};


void drawRule_setType(DrawData *p,int type);
void drawRule_onPress(DrawData *p,mBool dotpen);
mBool drawRule_onPress_setting(DrawData *p);

void drawRule_setting_line(DrawData *p);
void drawRule_setting_ellipse(DrawData *p);

void drawRule_setRecord(DrawData *p,int no);
void drawRule_callRecord(DrawData *p,int no);

#endif
