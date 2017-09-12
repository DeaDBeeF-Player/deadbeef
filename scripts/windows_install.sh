#!/bin/bash
. .install
PREFIX="${PREFIX:-`pwd`/build}"
mkdir -p $PREFIX/plugins
mkdir -p $PREFIX/pixmaps
mkdir -p $PREFIX/doc
mkdir -p $PREFIX/share/themes $PREFIX/share/icons
mkdir -p $PREFIX/etc/gtk-2.0 $PREFIX/lib/gtk-2.0/2.10.0/engines/
mkdir -p $PREFIX/config

rm -f $PREFIX/plugins/*.dll
cp ./.libs/deadbeef.exe $PREFIX/
cp ./plugins/nullout/.libs/nullout.dll $PREFIX/plugins/
cp ./plugins/cdda/.libs/cdda.dll $PREFIX/plugins/
cp ./plugins/flac/.libs/flac.dll $PREFIX/plugins/
cp ./plugins/alsa/.libs/alsa.dll $PREFIX/plugins/
cp ./plugins/sndio/.libs/sndio.dll $PREFIX/plugins/
cp ./plugins/mp3/.libs/mp3.dll $PREFIX/plugins/
cp ./plugins/hotkeys/.libs/hotkeys.dll $PREFIX/plugins/
cp ./plugins/vtx/.libs/vtx.dll $PREFIX/plugins/
cp ./plugins/ffap/.libs/ffap.dll $PREFIX/plugins/
cp ./plugins/wavpack/.libs/wavpack.dll $PREFIX/plugins/
cp ./plugins/vorbis/.libs/vorbis.dll $PREFIX/plugins/
cp ./plugins/oss/.libs/oss.dll $PREFIX/plugins/
cp ./plugins/vfs_curl/.libs/vfs_curl.dll $PREFIX/plugins/
cp ./plugins/ffmpeg/.libs/ffmpeg.dll $PREFIX/plugins/
cp ./plugins/lastfm/.libs/lastfm.dll $PREFIX/plugins/
cp ./plugins/sid/.libs/sid.dll $PREFIX/plugins/
cp ./plugins/adplug/.libs/adplug.dll $PREFIX/plugins/
cp ./plugins/gtkui/.libs/ddb_gui_GTK2.dll $PREFIX/plugins/
cp ./plugins/gtkui/.libs/ddb_gui_GTK3.dll $PREFIX/plugins/
cp ./plugins/sndfile/.libs/sndfile.dll $PREFIX/plugins/
cp ./plugins/pulse/.libs/pulse.dll $PREFIX/plugins/
cp ./plugins/artwork/.libs/artwork.dll $PREFIX/plugins/
cp ./plugins/supereq/.libs/supereq.dll $PREFIX/plugins/
cp ./plugins/gme/.libs/gme.dll $PREFIX/plugins/
cp ./plugins/dumb/.libs/ddb_dumb.dll $PREFIX/plugins/
cp ./plugins/notify/.libs/notify.dll $PREFIX/plugins/
cp ./plugins/musepack/.libs/musepack.dll $PREFIX/plugins/
cp ./plugins/wildmidi/.libs/wildmidi.dll $PREFIX/plugins/
cp ./plugins/tta/.libs/tta.dll $PREFIX/plugins/
cp ./plugins/dca/.libs/dca.dll $PREFIX/plugins/
cp ./plugins/aac/.libs/aac.dll $PREFIX/plugins/
cp ./plugins/mms/.libs/mms.dll $PREFIX/plugins/
cp ./plugins/shn/.libs/ddb_shn.dll $PREFIX/plugins/
cp ./plugins/psf/.libs/psf.dll $PREFIX/plugins/
cp ./plugins/shellexec/.libs/shellexec.dll $PREFIX/plugins/
cp ./plugins/shellexecui/.libs/shellexecui_gtk2.dll $PREFIX/plugins/
cp ./plugins/shellexecui/.libs/shellexecui_gtk3.dll $PREFIX/plugins/
cp ./plugins/dsp_libsrc/.libs/dsp_libsrc.dll $PREFIX/plugins/
cp ./plugins/m3u/.libs/m3u.dll $PREFIX/plugins/
cp ./plugins/ddb_input_uade2/ddb_input_uade2.dll $PREFIX/plugins/
cp ./plugins/converter/.libs/converter.dll $PREFIX/plugins/
cp ./plugins/converter/.libs/converter_gtk2.dll $PREFIX/plugins/
cp ./plugins/converter/.libs/converter_gtk3.dll $PREFIX/plugins/
cp ./plugins/soundtouch/ddb_soundtouch.dll $PREFIX/plugins/
cp ./plugins/vfs_zip/.libs/vfs_zip.dll $PREFIX/plugins/
cp ./plugins/medialib/.libs/medialib.dll $PREFIX/plugins/
cp ./plugins/mono2stereo/.libs/ddb_mono2stereo.dll $PREFIX/plugins/
cp ./plugins/alac/.libs/alac.dll $PREFIX/plugins/
cp ./plugins/wma/.libs/wma.dll $PREFIX/plugins/
cp ./plugins/pltbrowser/.libs/pltbrowser_gtk2.dll $PREFIX/plugins/
cp ./plugins/pltbrowser/.libs/pltbrowser_gtk3.dll $PREFIX/plugins/
cp ./plugins/coreaudio/.libs/coreaudio.dll $PREFIX/plugins/
cp ./plugins/sc68/.libs/in_sc68.dll $PREFIX/plugins/
cp ./plugins/statusnotifier/.libs/statusnotifier.dll $PREFIX/plugins/
cp ./plugins/portaudio/.libs/portaudio.dll $PREFIX/plugins/
cp ./plugins/waveout/.libs/waveout.dll $PREFIX/plugins/


# libs
cp $(ldd plugins/*/.libs/*.dll .libs/deadbeef.exe | awk 'NF == 4 {print $3}; NF == 2 {print $1}' |grep -i -v "System32" | grep -i -v "WinSxS" |sort -u) $PREFIX/

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
cp -r plugins/sc68/.libs/in_sc68.dll $PREFIX/plugins/
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
strip --strip-unneeded $PREFIX/deadbeef.exe
for i in $PREFIX/plugins/.libs/*.dll ; do strip --strip-unneeded $i ; done

# MS-Windows theme (GTK2)
for i in /mingw32 /mingw64 /usr; do
    cp -r $i/share/themes/MS-Windows $PREFIX/share/themes/
    cp $i/lib/gtk-2.0/2.10.0/engines/libwimp.dll $PREFIX/lib/gtk-2.0/2.10.0/engines

done

# Adwaita icons (GTK3)
for i in /mingw32 /mingw64 /usr; do
    cp -r $i/share/icons/Adwaita $PREFIX/share/icons/
done

# set default gtk2 theme
touch $PREFIX/etc/gtk-2.0/settings.ini
echo -e "[Settings]\r\ngtk-theme-name = MS-Windows\n" > $PREFIX/etc/gtk-2.0/settings.ini
