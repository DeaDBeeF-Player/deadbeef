#!/bin/sh

# This script does things needed after building deadbeef from source.

if [ -z $1 ]; then
	echo "windows_postbuild.sh"
	echo "Usage: $0 [deadbeef binary folder]"
	exit
fi

# Create essential directories
mkdir -pv $1/plugins
mkdir -pv $1/pixmaps
mkdir -pv $1/doc
mkdir -pv $1/share/themes $1/share/icons
mkdir -pv $1/config
mkdir -pv $1/lib
mkdir -pv $1/locale

# Doc files
cp -uv ChangeLog help.txt COPYING.GPLv2 COPYING.LGPLv2.1 about.txt translators.txt  $1/doc/

# Translations
for i in po/*.gmo ; do
	base=`basename $i .gmo`
	mkdir -pv $1/locale/$base/LC_MESSAGES
	cp -uv $i $1/locale/$base/LC_MESSAGES/deadbeef.mo
done
cp -uv translation/help.ru.txt  $1/doc/

# Libraries
rm -fv $1/plugins/*.lib | true
rm -fv $1/libwin.lib | true

ldd $1/plugins/*.dll $1/deadbeef.exe | awk 'NF == 4 {print $3}; NF == 2 {print $1}' | grep -iv "???" | grep -iv "System32" | grep -iv "WinSxS" | grep -iv "ConEmu" | sort -u > .libraries.tmp
cp -uv `cat .libraries.tmp` $1/


# gdk_pixbuf libs
for i in /mingw32 /mingw64 /usr; do
	cp -ru $i/lib/gdk-pixbuf-2.0 $1/lib/gdk-pixbuf-2.0 2>>/dev/null
done

# gtk2 theme
mkdir -pv $1/lib/gtk-2.0/2.10.0/engines

for i in /mingw32 /mingw64 /usr; do
	cp -ru $i/share/themes/MS-Windows $1/share/themes/ 2>>/dev/null
	cp -ru $i/lib/gtk-2.0/2.10.0/engines/libwimp.dll $1/lib/gtk-2.0/2.10.0/engines 2>>/dev/null
done

mkdir -pv $1/etc $1/etc/gtk-2.0
touch $1/etc/gtk-2.0/settings.ini
echo -e "[Settings]\r\ngtk-theme-name = MS-Windows\n" > $1/etc/gtk-2.0/settings.ini

# gtk3 misc
mkdir -pv $1/etc $1/etc/gtk-3.0
touch $1/etc/gtk-3.0/settings.ini
echo -e "[Settings]\r\ngtk-theme-name = Windows-10\r\ngtk-icon-theme-name = Windows-10-Icons" > $1/etc/gtk-3.0/settings.ini

for i in /mingw32 /mingw64 /usr; do
	cp -ru $i/share/icons/hicolor $1/share/icons/
	cp -ru $i/share/glib-2.0 $1/share/
done

# Windows-10 theme and icons can be obtained from https://github.com/B00merang-Project/Windows-10 and https://github.com/B00merang-Project/Windows-10-Icons)
for i in /mingw32 /mingw64 /usr; do
	cp -ru $i/share/icons/Windows-10-Icons $1/share/icons/ 2>>/dev/null
	cp -ru $i/share/themes/Windows-10 $1/share/themes/ 2>>/dev/null
done

# Adwaita is not necessary anymore
# for i in /mingw32 /mingw64 /usr; do
# 	 cp -ru $i/share/icons/Adwaita $1/share/icons/ 2>>/dev/null
# done

echo "output_plugin PortAudio output plugin" > $1/config/config
echo "gui_plugin GTK3" >> $1/config/config

exit 0