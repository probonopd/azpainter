SUBDIRS=texture

appdatadir=$(datadir)/azpainter

dist_appdata_DATA=\
palette-2.dat \
brush-2.dat \
grad.dat \
sckey-2.conf \
icon.png

# tr

trdatadir=$(appdatadir)/tr

dist_trdata_DATA=\
tr/en.mtr \
tr/ja.mtr \
tr/pt_BR.mtr \
tr/ru.mtr \
tr/uk.mtr \
tr/zh_CN.mtr

# theme

themedir=$(appdatadir)/theme
dist_theme_DATA=theme/black.theme

# image

img13dir=$(appdatadir)/13x13
img16dir=$(appdatadir)/16x16
img20dir=$(appdatadir)/20x20
img24dir=$(appdatadir)/24x24

dist_img13_DATA=13x13/layer.png

dist_img16_DATA=\
16x16/brushedit.png \
16x16/canvasview.png \
16x16/imgviewer.png \
16x16/layer.png \
16x16/tool.png \
16x16/toolbar.png \
16x16/toolsub.png

dist_img20_DATA=\
20x20/brushedit.png \
20x20/canvasview.png \
20x20/imgviewer.png \
20x20/tool.png \
20x20/toolbar.png \
20x20/toolsub.png

dist_img24_DATA=\
24x24/brushedit.png \
24x24/canvasview.png \
24x24/imgviewer.png \
24x24/tool.png \
24x24/toolbar.png \
24x24/toolsub.png

# brush

brushdir=$(appdatadir)/brush

dist_brush_DATA=\
brush/circle1.png \
brush/ellipse.png

# uninstall

uninstall-local:
	-test -d "$(appdatadir)" && rm -rf "$(appdatadir)"
