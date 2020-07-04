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

* Install git, GCC toolchain
* Install autoconf, automake, libtool, intltool, autopoint
* Remember to get submodules: ```git submodule update --init```
* Run ```./autogen.sh``` to bootstrap
* Read the generated INSTALL file and ```./configure --help``` for instructions
* See the README file for more information

### macOS (COCOA version)

* Install Xcode, 10.0 or higher.
* Run `sudo xcode-select --install` - This will configure git and command line build tools
* Clone the deadbeef git repository
* Remember to get submodules: ```git submodule update --init```

#### Command line

* Run ```xcodebuild -project osx/deadbeef.xcodeproj -target DeaDBeeF -configuration Release```
* The output will be located here: ```osx/build/Release/DeaDBeeF.app```

#### Xcode UI

* Open the `osx/deadbeef.xcodeproj` in Xcode, and build/run from there

### Windows

* Install 64-bit version of [msys2](https://www.msys2.org/) and ensure it has updated repositories (`pacman -Syu`)
* [premake5](https://premake.github.io/download.html) is also needed
* Get needed dependencies: 
	```pacman -S mingw-w64-x86_64-libzip mingw-w64-x86_64-pkg-config mingw-w64-x86_64-dlfcn mingw-w64-x86_64-gcc git make tar xz```
* Get a basic set of libraries for most important plugins:
	```pacman -S mingw-w64-x86_64-jansson mingw-w64-x86_64-gtk3 mingw-w64-x86_64-gtk2 mingw-w64-x86_64-mpg123 mingw-w64-x86_64-flac mingw-w64-x86_64-portaudio```
* Check [Windows plugin status](https://github.com/DeaDBeeF-Player/deadbeef/wiki/Windows-plugin-status) for other plugins dependencies and its functionality
* Ensure that you are in mingw64 shell (run mingw64.exe) and clone this repo
* From deadbeef main directory run ```premake5.exe --file=premake5-win.lua --os=linux gmake --standard``` using your corresponding path to ```premake5.exe```
* Compile with ```make config=debug_windows``` (debug version) or ```make config=release_windows``` (strip/normal version)
* If you compiled with multiple jobs (`-j`) run ```make resources_windows``` to make sure all libraries are copied
* Binaries will be placed in ```bin/debug``` or ```bin/release```
* GTK3 uses [Windows-10](https://github.com/B00merang-Project/Windows-10) theme and [Windows-10-Icons](https://github.com/B00merang-Artwork/Windows-10) by default. If they are not in msys2 tree, then they must be placed manually in ```share/icons``` and ```share/themes```. Eventually you get different theme and set it in ```etc/gtk-3.0/settings.ini``` file.

----

[Support this project development](http://deadbeef.sourceforge.net/support.html)
