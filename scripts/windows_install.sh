#!/bin/bash
. .install
PREFIX="${PREFIX:-`pwd`/build}"
if [ "$1" = "--help" ] || [ "$2" = "--help" ] || [ "$3" = "--help" ] || [ "$4" = "--help" ]; then
    echo "windows_install.sh"
    echo -e "Usage:\t" $0 "[--debug][--disable-gtk2][--disable-gtk3]"
    exit
fi
mkdir -p $PREFIX/plugins
mkdir -p $PREFIX/pixmaps
mkdir -p $PREFIX/doc
mkdir -p $PREFIX/share/themes $PREFIX/share/icons
mkdir -p $PREFIX/config

rm -f $PREFIX/plugins/*.dll
cp ./.libs/deadbeef.exe $PREFIX/ 2>>/dev/null
cp ./plugins/nullout/.libs/nullout.dll $PREFIX/plugins/ 2>>/dev/null
cp ./plugins/cdda/.libs/cdda.dll $PREFIX/plugins/ 2>>/dev/null
cp ./plugins/flac/.libs/flac.dll $PREFIX/plugins/ 2>>/dev/null
cp ./plugins/alsa/.libs/alsa.dll $PREFIX/plugins/ 2>>/dev/null
cp ./plugins/sndio/.libs/sndio.dll $PREFIX/plugins/ 2>>/dev/null
cp ./plugins/mp3/.libs/mp3.dll $PREFIX/plugins/ 2>>/dev/null
cp ./plugins/hotkeys/.libs/hotkeys.dll $PREFIX/plugins/ 2>>/dev/null
cp ./plugins/vtx/.libs/vtx.dll $PREFIX/plugins/ 2>>/dev/null
cp ./plugins/ffap/.libs/ffap.dll $PREFIX/plugins/ 2>>/dev/null
cp ./plugins/wavpack/.libs/wavpack.dll $PREFIX/plugins/ 2>>/dev/null
cp ./plugins/vorbis/.libs/vorbis.dll $PREFIX/plugins/ 2>>/dev/null
cp ./plugins/oss/.libs/oss.dll $PREFIX/plugins/ 2>>/dev/null
cp ./plugins/vfs_curl/.libs/vfs_curl.dll $PREFIX/plugins/ 2>>/dev/null
cp ./plugins/ffmpeg/.libs/ffmpeg.dll $PREFIX/plugins/ 2>>/dev/null
cp ./plugins/lastfm/.libs/lastfm.dll $PREFIX/plugins/ 2>>/dev/null
cp ./plugins/sid/.libs/sid.dll $PREFIX/plugins/ 2>>/dev/null
cp ./plugins/adplug/.libs/adplug.dll $PREFIX/plugins/ 2>>/dev/null
cp ./plugins/sndfile/.libs/sndfile.dll $PREFIX/plugins/ 2>>/dev/null
cp ./plugins/pulse/.libs/pulse.dll $PREFIX/plugins/ 2>>/dev/null
cp ./plugins/artwork-legacy/.libs/artwork.dll $PREFIX/plugins/ 2>>/dev/null
cp ./plugins/artwork/.libs/artwork.dll $PREFIX/plugins/ 2>>/dev/null
cp ./plugins/supereq/.libs/supereq.dll $PREFIX/plugins/ 2>>/dev/null
cp ./plugins/gme/.libs/gme.dll $PREFIX/plugins/ 2>>/dev/null
cp ./plugins/dumb/.libs/ddb_dumb.dll $PREFIX/plugins/ 2>>/dev/null
cp ./plugins/notify/.libs/notify.dll $PREFIX/plugins/ 2>>/dev/null
cp ./plugins/musepack/.libs/musepack.dll $PREFIX/plugins/ 2>>/dev/null
cp ./plugins/wildmidi/.libs/wildmidi.dll $PREFIX/plugins/ 2>>/dev/null
cp ./plugins/tta/.libs/tta.dll $PREFIX/plugins/ 2>>/dev/null
cp ./plugins/dca/.libs/dca.dll $PREFIX/plugins/ 2>>/dev/null
cp ./plugins/aac/.libs/aac.dll $PREFIX/plugins/ 2>>/dev/null
cp ./plugins/mms/.libs/mms.dll $PREFIX/plugins/ 2>>/dev/null
cp ./plugins/shn/.libs/ddb_shn.dll $PREFIX/plugins/ 2>>/dev/null
cp ./plugins/psf/.libs/psf.dll $PREFIX/plugins/ 2>>/dev/null
cp ./plugins/shellexec/.libs/shellexec.dll $PREFIX/plugins/ 2>>/dev/null
cp ./plugins/dsp_libsrc/.libs/dsp_libsrc.dll $PREFIX/plugins/ 2>>/dev/null
cp ./plugins/m3u/.libs/m3u.dll $PREFIX/plugins/ 2>>/dev/null
cp ./plugins/converter/.libs/converter.dll $PREFIX/plugins/ 2>>/dev/null
cp ./plugins/soundtouch/ddb_soundtouch.dll $PREFIX/plugins/ 2>>/dev/null
cp ./plugins/vfs_zip/.libs/vfs_zip.dll $PREFIX/plugins/ 2>>/dev/null
cp ./plugins/medialib/.libs/medialib.dll $PREFIX/plugins/ 2>>/dev/null
cp ./plugins/mono2stereo/.libs/ddb_mono2stereo.dll $PREFIX/plugins/ 2>>/dev/null
cp ./plugins/alac/.libs/alac.dll $PREFIX/plugins/ 2>>/dev/null
cp ./plugins/wma/.libs/wma.dll $PREFIX/plugins/ 2>>/dev/null
cp ./plugins/coreaudio/.libs/coreaudio.dll $PREFIX/plugins/ 2>>/dev/null
cp ./plugins/sc68/.libs/in_sc68.dll $PREFIX/plugins/ 2>>/dev/null
cp ./plugins/statusnotifier/.libs/statusnotifier.dll $PREFIX/plugins/ 2>>/dev/null
cp ./plugins/rg_scanner/.libs/rg_scanner.dll $PREFIX/plugins/ 2>>/dev/null
cp ./plugins/portaudio/.libs/portaudio.dll $PREFIX/plugins/ 2>>/dev/null
cp ./plugins/waveout/.libs/waveout.dll $PREFIX/plugins/ 2>>/dev/null

if [ "$1" != "--disable-gtk2" ] && [ "$2" != "--disable-gtk2" ] && [ "$3" != "--disable-gtk2" ]; then
    mkdir -p $PREFIX/etc/gtk-2.0 $PREFIX/lib/gtk-2.0/2.10.0/engines/
    cp ./plugins/gtkui/.libs/ddb_gui_GTK2.dll $PREFIX/plugins/ 2>>/dev/null
    cp ./plugins/shellexecui/.libs/shellexecui_gtk2.dll $PREFIX/plugins/ 2>>/dev/null
    cp ./plugins/converter/.libs/converter_gtk2.dll $PREFIX/plugins/ 2>>/dev/null
    cp ./plugins/pltbrowser/.libs/pltbrowser_gtk2.dll $PREFIX/plugins/ 2>>/dev/null
fi
if [ "$1" != "--disable-gtk3" ] && [ "$2" != "--disable-gtk3" ] && [ "$3" != "--disable-gtk3" ]; then
    mkdir -p $PREFIX/etc/gtk-3.0
    cp ./plugins/gtkui/.libs/ddb_gui_GTK3.dll $PREFIX/plugins/ 2>>/dev/null
    cp ./plugins/shellexecui/.libs/shellexecui_gtk3.dll $PREFIX/plugins/ 2>>/dev/null
    cp ./plugins/converter/.libs/converter_gtk3.dll $PREFIX/plugins/ 2>>/dev/null
    cp ./plugins/pltbrowser/.libs/pltbrowser_gtk3.dll $PREFIX/plugins/ 2>>/dev/null
fi


# libs
cp $(ldd $PREFIX/plugins/*.dll .libs/deadbeef.exe | awk 'NF == 4 {print $3}; NF == 2 {print $1}' |grep -i -v "System32" | grep -i -v "WinSxS" |sort -u) $PREFIX/

# pixmaps
for i in pause_16.png play_16.png noartwork.png buffering_16.png; do
    cp ./pixmaps/$i $PREFIX/pixmaps/
done

# docs
for i in ChangeLog help.txt COPYING.GPLv2 COPYING.LGPLv2.1 about.txt translators.txt; do
    cp ./$i $PREFIX/doc/
done

# icon
cp ./icons/32x32/deadbeef.png $PREFIX/

# converter presets
cp -r plugins/converter/convpresets $PREFIX/plugins/convpresets

# sc68data
mkdir -p  $PREFIX/plugins/data68/Replay
cp -r plugins/sc68/file68/data68/Replay/*.bin $PREFIX/plugins/data68/Replay/

# translations
mkdir -p $PREFIX/locale
for i in po/*.gmo ; do
    base=`basename po/$i .gmo`
    mkdir -p $PREFIX/locale/$base/LC_MESSAGES
    cp $i $PREFIX/locale/$base/LC_MESSAGES/deadbeef.mo
done
cp translation/help.pt_BR.txt $PREFIX/doc/
cp translation/help.ru.txt $PREFIX/doc/
cp translation/help.zh_TW.txt $PREFIX/doc/

# strip
if [ "$1" != "--debug" ] && [ "$2" != "--debug" ] && [ "$3" != "--debug" ]; then
	strip --strip-unneeded $PREFIX/deadbeef.exe
	for i in $PREFIX/plugins/*.dll ; do strip --strip-unneeded $i ; done
fi

if [ "$1" != "--disable-gtk2" ] && [ "$2" != "--disable-gtk2" ] && [ "$3" != "--disable-gtk2" ]; then
    # MS-Windows theme (GTK2)
    for i in /mingw32 /mingw64 /usr; do
        cp -r $i/share/themes/MS-Windows $PREFIX/share/themes/ 2>>/dev/null
        cp $i/lib/gtk-2.0/2.10.0/engines/libwimp.dll $PREFIX/lib/gtk-2.0/2.10.0/engines 2>>/dev/null
    done
    # set default gtk2 theme
    touch $PREFIX/etc/gtk-2.0/settings.ini
    echo -e "[Settings]\r\ngtk-theme-name = MS-Windows\n" > $PREFIX/etc/gtk-2.0/settings.ini

fi

if [ "$1" != "--disable-gtk3" ] && [ "$2" != "--disable-gtk3" ] && [ "$3" != "--disable-gtk3" ]; then
    # Adwaita icons (GTK3)
    #for i in /mingw32 /mingw64 /usr; do
    #    cp -r $i/share/icons/Adwaita $PREFIX/share/icons/ 2>>/dev/null
    #done

    # Hicolor icons (GTK3)
    for i in /mingw32 /mingw64 /usr; do
        cp -r $i/share/icons/hicolor $PREFIX/share/icons/ 2>>/dev/null
    done

    # glib-2.0 share folder(GTK3)
    for i in /mingw32 /mingw64 /usr; do
        cp -r $i/share/glib-2.0 $PREFIX/share/ 2>>/dev/null
    done

    # Windows-10 theme and icons (can be obtained from https://github.com/B00merang-Project/Windows-10 and https://github.com/B00merang-Project/Windows-10-Icons)
    for i in /mingw32 /mingw64 /usr; do
        cp -r $i/share/themes/Windows-10 $PREFIX/share/themes 2>>/dev/null
        cp -r $i/share/icons/Windows-10-Icons $PREFIX/share/icons/ 2>>/dev/null
    done

    # set default gtk3 theme
    touch $PREFIX/etc/gtk-3.0/settings.ini
    echo -e "[Settings]\r\ngtk-theme-name = Windows-10\r\ngtk-icon-theme-name = Windows-10-Icons" > $PREFIX/etc/gtk-3.0/settings.ini
fi

# set default output plugin
echo "output_plugin PortAudio output plugin" > $PREFIX/config/config
