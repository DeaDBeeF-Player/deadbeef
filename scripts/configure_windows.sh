./configure --enable-maintainer-mode --disable-nullout --disable-oss --disable-sid --disable-ffap --disable-vtx --disable-adplug --disable-vorbis --disable-ffmpeg --disable-flac --disable-sndfile  --disable-wavpack --disable-cdda --disable-gme --disable-dumb --disable-musepack --disable-wildmidi --disable-tta --disable-dca --disable-aac --disable-mms --disable-shn --disable-ao --disable-supereq --disable-artwork --disable-lfm --disable-vfs-curl --disable-hotkeys --disable-notify --disable-shellexec \
--disable-sc68 --enable-gtk2 --enable-gtk3 --enable-flac --enable-mp3 --enable-statusnotifier --enable-aac --enable-shellexec --enable-shellexecui --enable-notify --enable-artwork --enable-supereq --enable-hotkeys --enable-pulse --enable-vorbis --enable-sndfile --enable-vfs-curl --enable-nullout \
 --disable-converter $1

 #converter had to be disabled because there is no scandir in mingw
