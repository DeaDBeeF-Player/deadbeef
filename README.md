## About

DeaDBeeF is a music player for \*nix-like systems and OS X.

More info is [here](http://deadbeef.sf.net).

[Join us on slack](https://deadbeef-slack.herokuapp.com)

## Download development builds

[![Build Status](https://travis-ci.org/Alexey-Yakovenko/deadbeef.svg?branch=master)](https://travis-ci.org/Alexey-Yakovenko/deadbeef)

[Download the latest GNU/Linux builds](https://sourceforge.net/projects/deadbeef/files/travis/linux/)

Whilst OSX/Cocoa version can be used, it is unfinished and is under heavy development. Don't put your expectations too high yet.

[Download the latest OSX build](https://sourceforge.net/projects/deadbeef/files/travis/osx/)

## Compiling

The following instructions assume that commands are executed in deadbeef repository folder.

### Linux, BSD and similar (GTK/*NIX version)

* Install autoconf, automake, libtool, intltool, autopoint
* Run ```./autogen.sh``` to bootstrap
* Read the generated INSTALL file and ```./configure --help``` for instructions
* See the README file for more information

### OS X (COCOA version)

* Fetch the dependencies: ```git submodule update --init```
* Install XCode
* Install [Yasm](http://rudix.org/packages/yasm.html)
* Run ```xcodebuild -project osx/deadbeef.xcodeproj -target deadbeef -configuration Release```
* Get the output: ```osx/build/Release/deadbeef.app```
* OR open the osx/deadbeef.xcodeproj in XCode, and build/run from there

### Windows (msys2)

* Install msys2
* Get needed dependencies: 
	```pacman -S mingw-w64-x86_64-libzip pkgconfig mingw-w64-x86_64-dlfcn mingw-w64-x86_64-libtool mingw-w64-x86_64-gcc git automake autogen autoconf gettext gettext-devel libtool make m4 tar xz intltool```
* Get some packages for plugins:
	```pacman -S mingw-w64-x86_64-gtk3 mingw-w64-x86_64-gtk2 mingw-w64-x86_64-libmad mingw-w64-x86_64-flac```
* Install jansson or get compiled lib from https://github.com/kuba160/deadbeef-w64-deps
   If you're compiling, make sure you are using mingw shell (run mingwXX.exe), if you don't want to compile it self:
   ```git clone https://github.com/kuba160/deadbeef-w64-deps && cd deadbeef-w64-deps && make jansson-install```
* Optionally, install other dependencies for plugins which you want to compile.
* Switch to mingw shell (run mingwXX.exe)
* Run ```./autogen.sh``` to bootstrap
* Run `./scripts/configure_windows.sh`
* `make`
* Run `./scripts/windows_install.sh".`
* Your build will be located in `build` folder.

### Windows (Debian/Ubuntu mingw32 cross-compile)

* Steps below assume compiling 64-bit version
* Install checkinstall, gcc-mingw-w64-x86_64 and mingw-w64-x86_64-dev packages `sudo apt install checkinstall gcc-mingw-w64-x86_64 mingw-w64-x86_64-dev`
* Install precompiled libs from https://github.com/kuba160/deadbeef-w64-deps
```git clone https://github.com/kuba160/deadbeef-w64-deps && cd deadbeef-w64-deps && make jansson-checkinstall``` Use `jansson-install` if you don't have `checkinstall`

  OR
* Install jansson manually. Additionally you will need to get dll files needed for stand-alone build because Debian/Ubuntu does not include them. The program will however compile fine.
* Optionally, install other dependencies for plugins which you want to compile.
* Run ```./autogen.sh``` to bootstrap
* Run `PKG_CONFIG_LIBDIR=/usr/x86_64-w64-mingw32/lib/pkgconfig ./scripts/configure_windows.sh --host=x86_64-w64-mingw32`
* `make`
* Run `./scripts/windows_install.sh".`
* Your build will be located in `build` folder.
----

[![Support via Gratipay](https://cdn.rawgit.com/gratipay/gratipay-badge/2.3.0/dist/gratipay.png)](https://gratipay.com/deadbeef/)

[More ways to support](http://deadbeef.sourceforge.net/support.html)
