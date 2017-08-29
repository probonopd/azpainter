EXTRA_DIST = azpainter.desktop azpainter.png azpainter.svg \
 application-x-azpainter-apd.png application-x-azpainter-apd.svg azpainter.xml

desktopdir = $(datadir)/applications
desktop_DATA = azpainter.desktop

icondir = $(datadir)/icons/hicolor/48x48/apps
icon_DATA = azpainter.png application-x-azpainter-apd.png

svgdir = $(datadir)/icons/hicolor/scalable/apps
svg_DATA = azpainter.svg application-x-azpainter-apd.svg

mimetypedir = $(datadir)/mime/packages
mimetype_DATA = azpainter.xml

### rule

install-data-hook: update-cache
uninstall-hook: update-cache

update-cache:
	@-if test -z "$(DESTDIR)"; then \
		echo "* Updating cache."; \
		gtk-update-icon-cache -f -t $(datadir)/icons/hicolor; \
		update-mime-database $(datadir)/mime; \
		update-desktop-database $(datadir)/applications; \
	fi
