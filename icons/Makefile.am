iconspath=@top_srcdir@/icons

# icons should always be installed into gtk prefix (either gtk2 or gtk3 - doesn't
# matter)
GTK_PREFIX=`pkg-config --variable=prefix gtk+-2.0 || pkg-config --variable=prefix gtk+-3.0 || echo /usr`

icon16dir=$(GTK_PREFIX)/share/icons/hicolor/16x16/apps
icon16_DATA=$(iconspath)/16x16/deadbeef.png

icon22dir=$(GTK_PREFIX)/share/icons/hicolor/22x22/apps
icon22_DATA=$(iconspath)/22x22/deadbeef.png

icon24dir=$(GTK_PREFIX)/share/icons/hicolor/24x24/apps
icon24_DATA=$(iconspath)/24x24/deadbeef.png

icon32dir=$(GTK_PREFIX)/share/icons/hicolor/32x32/apps
icon32_DATA=$(iconspath)/32x32/deadbeef.png

icon36dir=$(GTK_PREFIX)/share/icons/hicolor/36x36/apps
icon36_DATA=$(iconspath)/36x36/deadbeef.png

icon48dir=$(GTK_PREFIX)/share/icons/hicolor/48x48/apps
icon48_DATA=$(iconspath)/48x48/deadbeef.png

icon64dir=$(GTK_PREFIX)/share/icons/hicolor/64x64/apps
icon64_DATA=$(iconspath)/64x64/deadbeef.png

icon72dir=$(GTK_PREFIX)/share/icons/hicolor/72x72/apps
icon72_DATA=$(iconspath)/72x72/deadbeef.png

icon96dir=$(GTK_PREFIX)/share/icons/hicolor/96x96/apps
icon96_DATA=$(iconspath)/96x96/deadbeef.png

icon128dir=$(GTK_PREFIX)/share/icons/hicolor/128x128/apps
icon128_DATA=$(iconspath)/128x128/deadbeef.png

icon192dir=$(GTK_PREFIX)/share/icons/hicolor/192x192/apps
icon192_DATA=$(iconspath)/192x192/deadbeef.png

icon256dir=$(GTK_PREFIX)/share/icons/hicolor/256x256/apps
icon256_DATA=$(iconspath)/256x256/deadbeef.png

iconscalabledir=$(GTK_PREFIX)/share/icons/hicolor/scalable/apps
iconscalable_DATA=$(iconspath)/scalable/deadbeef.svg

EXTRA_DIST = $(icon16_DATA) $(icon22_DATA) $(icon24_DATA) $(icon32_DATA) $(icon36_DATA) $(icon48_DATA) $(icon64_DATA) $(icon72_DATA) $(icon96_DATA) $(icon128_DATA) $(icon192_DATA) $(icon256_DATA) $(iconscalable_DATA)
