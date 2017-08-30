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
 * DrawData 操作関数
 **********************************/

#ifndef DRAW_OP_FUNC_H
#define DRAW_OP_FUNC_H

/* brush */

mBool drawOp_brush_free_press(DrawData *p,mBool registered,mBool pressure_max);
mBool drawOp_dotpen_free_press(DrawData *p,mBool registered);
mBool drawOp_finger_free_press(DrawData *p,mBool registered);

void drawOpDraw_brush_line(DrawData *p);
void drawOpDraw_brush_box(DrawData *p);
void drawOpDraw_brush_ellipse(DrawData *p);
void drawOpDraw_brush_lineSuccConc(DrawData *p);
void drawOpDraw_brush_spline(DrawData *p);

mBool drawOp_xorline_to_bezier(DrawData *p);

mBool drawOp_dragBrushSize_press(DrawData *p,mBool registered);

/* func1 */

mBool drawOp_common_norelease(DrawData *p);

void drawOpDraw_fillBox(DrawData *p);
void drawOpDraw_fillEllipse(DrawData *p);
void drawOpDraw_fillPolygon(DrawData *p);
void drawOpDraw_gradation(DrawData *p);

mBool drawOp_fill_press(DrawData *p);
mBool drawOp_movetool_press(DrawData *p);

mBool drawOp_stamp_press(DrawData *p,int subno);
void drawOp_setStampImage(DrawData *p);

mBool drawOp_canvasMove_press(DrawData *p);
mBool drawOp_canvasRotate_press(DrawData *p);
mBool drawOp_canvasZoom_press(DrawData *p);

mBool drawOp_spoit_press(DrawData *p,mBool enable_alt);

mBool drawOp_intermediateColor_press(DrawData *p);
mBool drawOp_replaceColor_press(DrawData *p,mBool rep_tp);

mBool drawOp_drawtext_press(DrawData *p);

/* func2 */

void drawOp_setSelect(DrawData *p,int type);

mBool drawOp_magicwand_press(DrawData *p);
mBool drawOp_selmove_press(DrawData *p);
mBool drawOp_selimgmove_press(DrawData *p,mBool cut);

void drawOp_boxedit_nograb_motion(DrawData *p);
mBool drawOp_boxedit_press(DrawData *p);

/* xor */

mBool drawOpXor_line_press(DrawData *p,int opsubtype);
mBool drawOpXor_boxarea_press(DrawData *p,int opsubtype);
mBool drawOpXor_boximage_press(DrawData *p,int opsubtype);
mBool drawOpXor_ellipse_press(DrawData *p,int opsubtype);
mBool drawOpXor_sumline_press(DrawData *p,int opsubtype);
mBool drawOpXor_polygon_press(DrawData *p,int opsubtype);
mBool drawOpXor_lasso_press(DrawData *p,int opsubtype);
mBool drawOpXor_rulepoint_press(DrawData *p);

void drawOpXor_drawline(DrawData *p);
void drawOpXor_drawBox_area(DrawData *p);
void drawOpXor_drawBox_image(DrawData *p);
void drawOpXor_drawEllipse(DrawData *p);
void drawOpXor_drawBezier(DrawData *p,mBool erase);
void drawOpXor_drawPolygon(DrawData *p);
void drawOpXor_drawLasso(DrawData *p,mBool erase);
void drawOpXor_drawCrossPoint(DrawData *p);
void drawOpXor_drawBrushSizeCircle(DrawData *p,mBool erase);

/* spline */

mBool drawOp_spline_press(DrawData *p);

#endif
