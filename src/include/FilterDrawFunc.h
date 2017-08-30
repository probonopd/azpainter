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

/***************************************
 * フィルタ描画関数
 ***************************************/

/* filter_color.c */

mBool FilterDraw_color_brightcont(FilterDrawInfo *info);
mBool FilterDraw_color_gamma(FilterDrawInfo *info);
mBool FilterDraw_color_level(FilterDrawInfo *info);
mBool FilterDraw_color_rgb(FilterDrawInfo *info);
mBool FilterDraw_color_hsv(FilterDrawInfo *info);
mBool FilterDraw_color_nega(FilterDrawInfo *info);
mBool FilterDraw_color_grayscale(FilterDrawInfo *info);
mBool FilterDraw_color_sepia(FilterDrawInfo *info);
mBool FilterDraw_color_gradmap(FilterDrawInfo *info);
mBool FilterDraw_color_threshold(FilterDrawInfo *info);
mBool FilterDraw_color_threshold_dither(FilterDrawInfo *info);
mBool FilterDraw_color_posterize(FilterDrawInfo *info);

mBool FilterDraw_color_replace_drawcol(FilterDrawInfo *info);
mBool FilterDraw_color_replace(FilterDrawInfo *info);

mBool FilterDraw_alpha_checked(FilterDrawInfo *info);
mBool FilterDraw_alpha_current(FilterDrawInfo *info);

/* filter_mediancut.c */

mBool FilterDraw_mediancut(FilterDrawInfo *info);

/* filter_blur.c */

mBool FilterDraw_blur(FilterDrawInfo *info);
mBool FilterDraw_gaussblur(FilterDrawInfo *info);
mBool FilterDraw_motionblur(FilterDrawInfo *info);
mBool FilterDraw_radialblur(FilterDrawInfo *info);
mBool FilterDraw_lensblur(FilterDrawInfo *info);

/* filter_draw.c */

mBool FilterDraw_cloud(FilterDrawInfo *info);
mBool FilterDraw_amitone1(FilterDrawInfo *info);
mBool FilterDraw_amitone2(FilterDrawInfo *info);
mBool FilterDraw_draw_randpoint(FilterDrawInfo *info);
mBool FilterDraw_draw_edgepoint(FilterDrawInfo *info);
mBool FilterDraw_draw_frame(FilterDrawInfo *info);
mBool FilterDraw_draw_horzvertLine(FilterDrawInfo *info);
mBool FilterDraw_draw_plaid(FilterDrawInfo *info);

/* filter_comic.c */

mBool FilterDraw_comic_concline_flash(FilterDrawInfo *info);
mBool FilterDraw_comic_popupflash(FilterDrawInfo *info);
mBool FilterDraw_comic_uniflash(FilterDrawInfo *info);
mBool FilterDraw_comic_uniflash_wave(FilterDrawInfo *info);

/* filter_effect.c */

mBool FilterDraw_effect_glow(FilterDrawInfo *info);
mBool FilterDraw_effect_rgbshift(FilterDrawInfo *info);
mBool FilterDraw_effect_oilpaint(FilterDrawInfo *info);
mBool FilterDraw_effect_emboss(FilterDrawInfo *info);
mBool FilterDraw_effect_noise(FilterDrawInfo *info);
mBool FilterDraw_effect_diffusion(FilterDrawInfo *info);
mBool FilterDraw_effect_scratch(FilterDrawInfo *info);
mBool FilterDraw_effect_median(FilterDrawInfo *info);
mBool FilterDraw_effect_blurring(FilterDrawInfo *info);

/* filter_transform.c */

mBool FilterDraw_trans_wave(FilterDrawInfo *info);
mBool FilterDraw_trans_ripple(FilterDrawInfo *info);
mBool FilterDraw_trans_polar(FilterDrawInfo *info);
mBool FilterDraw_trans_radial_shift(FilterDrawInfo *info);
mBool FilterDraw_trans_swirl(FilterDrawInfo *info);

/* filter_other1.c */

mBool FilterDraw_mozaic(FilterDrawInfo *info);
mBool FilterDraw_crystal(FilterDrawInfo *info);
mBool FilterDraw_halftone(FilterDrawInfo *info);

mBool FilterDraw_sharp(FilterDrawInfo *info);
mBool FilterDraw_unsharpmask(FilterDrawInfo *info);
mBool FilterDraw_sobel(FilterDrawInfo *info);
mBool FilterDraw_laplacian(FilterDrawInfo *info);
mBool FilterDraw_highpass(FilterDrawInfo *info);

mBool FilterDraw_lumtoAlpha(FilterDrawInfo *info);
mBool FilterDraw_3Dframe(FilterDrawInfo *info);
mBool FilterDraw_shift(FilterDrawInfo *info);
mBool FilterDraw_dot_thinning(FilterDrawInfo *info);
mBool FilterDraw_hemming(FilterDrawInfo *info);

/* filter_antialiasing.c */

mBool FilterDraw_antialiasing(FilterDrawInfo *info);

