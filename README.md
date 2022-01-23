## About

DeaDBeeF is a multiple-platform music player for desktop operating systems.

[The Official Website](http://deadbeef.sf.net).

If you wish to chat with developers, join us on [Slack](https://deadbeef-slack.herokuapp.com), or [Discord](https://discord.gg/GTVvgSqZrr).

## Download official releases (only GNU/Linux and Windows)

[Downloads Page](https://deadbeef.sourceforge.io/download.html)

## Download nightly (development) builds

<sub><sup>NOTE: The macOS version has not been officially released, and has many unresolved issues and unimplemented features</sup></sub>

[![Linux Build Status](https://github.com/DeaDBeeF-Player/deadbeef/workflows/Build%20for%20Linux/badge.svg)](https://github.com/DeaDBeeF-Player/deadbeef/actions?query=workflow%3A%22Build+for+Linux%22)
[![Windows Build Status](https://github.com/DeaDBeeF-Player/deadbeef/workflows/Build%20for%20Windows/badge.svg)](https://github.com/DeaDBeeF-Player/deadbeef/actions?query=workflow%3A%22Build+for+Windows%22)
[![macOS Build Status](https://github.com/DeaDBeeF-Player/deadbeef/workflows/Build%20for%20macOS/badge.svg)](https://github.com/DeaDBeeF-Player/deadbeef/actions?query=workflow%3A%22Build+for+macOS%22)

[Nightly GNU/Linux Builds](https://sourceforge.net/projects/deadbeef/files/travis/linux/master/)

[Nightly Windows Builds](https://sourceforge.net/projects/deadbeef/files/travis/windows/master/)

[Nightly macOS Builds](https://sourceforge.net/projects/deadbeef/files/travis/macOS/master/)


## Building DeaDBeeF from source

### Linux, BSD and similar (GTK/*NIX version)

* See the README file for detailed instructions, dependencies, etc.
* Install git, Clang toolchain
* Remember to get submodules: `git submodule update --init`
* Install dependencies, as listed in the README file
* Run `./autogen.sh` to bootstrap
* Run `CC=clang CXX=clang++ ./configure`, followed with `make` and `sudo make install`.
* For more information about the build process, read the generated INSTALL file and the output of `./configure --help`.

### macOS

* Install Xcode. The latest one is the best, but older versions will usually keep working for a year or two.
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
	```pacman -S mingw-w64-x86_64-libzip mingw-w64-x86_64-pkg-config mingw-w64-x86_64-dlfcn mingw-w64-x86_64-clang mingw-w64-x86_64-libblocksruntime git make tar xz```
* Get a basic set of libraries for most important plugins:
	```pacman -S mingw-w64-x86_64-jansson mingw-w64-x86_64-gtk3 mingw-w64-x86_64-gtk2 mingw-w64-x86_64-mpg123 mingw-w64-x86_64-flac mingw-w64-x86_64-portaudio```
* Check [Windows plugin status](https://github.com/DeaDBeeF-Player/deadbeef/wiki/Windows-plugin-status) for other plugins dependencies and its functionality
* Ensure that you are in mingw64 shell (run mingw64.exe) and clone this repo
* From deadbeef main directory run ```premake5 --standard gmake2``` using your corresponding path to ```premake5.exe```
* Compile with ```make config=debug_windows``` (debug version) or ```make config=release_windows``` (strip/normal version)
* Binaries will be placed in ```bin/debug``` or ```bin/release```
* GTK3 uses [Windows-10](https://github.com/B00merang-Project/Windows-10) theme and [Windows-10-Icons](https://github.com/B00merang-Artwork/Windows-10) by default. If they are not in msys2 tree, then they must be placed manually in ```share/icons``` and ```share/themes```. Eventually you get different theme and set it in ```etc/gtk-3.0/settings.ini``` file.

----

[Support development of this project](http://deadbeef.sourceforge.net/support.html)
