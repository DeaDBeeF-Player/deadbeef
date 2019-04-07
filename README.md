## About

DeaDBeeF is a music player for \*nix-like systems and OS X.

More info is [here](http://deadbeef.sf.net).

[Join us on slack](https://deadbeef-slack.herokuapp.com)

## Download development builds

[![Build Status](https://travis-ci.org/DeaDBeeF-Player/deadbeef.svg?branch=master)](https://travis-ci.org/DeaDBeeF-Player/deadbeef)

[Download the latest GNU/Linux builds](https://sourceforge.net/projects/deadbeef/files/travis/linux/)

Whilst OSX/Cocoa version can be used, it is unfinished and is under heavy development. Don't put your expectations too high yet.

[Download the latest OSX build](https://sourceforge.net/projects/deadbeef/files/travis/osx/)

## Compiling

### Linux, BSD and similar (GTK/*NIX version)

* Install git, GCC toolchain, then clone the repo
* Install autoconf, automake, libtool, intltool, autopoint
* Run ```./autogen.sh``` to bootstrap
* Read the generated INSTALL file and ```./configure --help``` for instructions
* See the README file for more information

### OS X (COCOA version)

* Install XCode, and run `sudo xcode-select --install`; This would also get you git etc
* Clone the deadbeef repo, and fetch the dependencies: ```git submodule update --init```
* Install [Yasm](https://yasm.tortall.net/Download.html) -- unpack the source, then run `./configure && make -j8 && sudo make install`
* Run ```xcodebuild -project osx/deadbeef.xcodeproj -target deadbeef -configuration Release```
* Get the output: ```osx/build/Release/DeaDBeeF.app```
* OR open the osx/deadbeef.xcodeproj in XCode, and build/run from there

### Windows

* Install 64-bit version of [msys2](https://www.msys2.org/)
* [premake5](https://premake.github.io/download.html) is also needed
* Get needed dependencies: 
	```pacman -S mingw-w64-x86_64-libzip mingw-w64-x86_64-pkg-config pkgconfig mingw-w64-x86_64-dlfcn mingw-w64-x86_64-libtool mingw-w64-x86_64-gcc git automake autogen autoconf gettext gettext-devel libtool make m4 tar xz intltool```
* Get a basic set of libraries for necessary plugins:
	```pacman -S mingw-w64-x86_64-jansson mingw-w64-x86_64-gtk3 mingw-w64-x86_64-gtk2 mingw-w64-x86_64-mpg123 mingw-w64-x86_64-flac mingw-w64-x86_64-portaudio```
* Some packages for other plugins (not all may be working through):
	```mingw-w64-x86_64-libsamplerate mingw-w64-x86_64-curl mingw-w64-x86_64-faad2 mingw-w64-x86_64-ffmpeg mingw-w64-x86_64-opusfile-0.8-1```
* Ensure that you are in mingw64 shell (run mingw64.exe)
* Run ```./autogen.sh``` to bootstrap
* From deadbeef main directory run ```premake5.exe --file=premake5-win.lua --os=linux gmake --standard``` using your corresponding path to ```premake5.exe```
* Compile with ```make config=debug_windows``` (debug version) or ```make config=release_windows``` (strip/normal version)
* Binaries will be placed in ```bin/debug``` or ```bin/release```
* GTK3 uses [Windows-10](https://github.com/B00merang-Project/Windows-10) theme and [Windows-10-Icons](https://github.com/B00merang-Artwork/Windows-10) by default. If they are not in msys2 tree, then they must be placed manually in ```share/icons``` and ```share/themes```. Eventually you get different theme and set it in ```etc/gtk-3.0/settings.ini``` file.

----

[Support this project development](http://deadbeef.sourceforge.net/support.html)
