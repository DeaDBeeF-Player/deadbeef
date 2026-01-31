## About

DeaDBeeF is a multiple-platform music player for desktop operating systems.

[The Official Website](http://deadbeef.sf.net).

If you wish to chat with developers, join us on [Discord](https://discord.gg/GTVvgSqZrr).

## Download official releases (only GNU/Linux and Windows)

[Downloads Page](https://deadbeef.sourceforge.io/download.html)

## Download nightly (development) builds

<sub><sup>NOTE: The macOS version has not been officially released, and has many unresolved issues and unimplemented features</sup></sub>

[![Linux Build Status](https://github.com/DeaDBeeF-Player/deadbeef/actions/workflows/linuxbuild.yml/badge.svg)](https://github.com/DeaDBeeF-Player/deadbeef/actions/workflows/linuxbuild.yml)
[![Windows Build Status](https://github.com/DeaDBeeF-Player/deadbeef/actions/workflows/windowsbuild.yml/badge.svg)](https://github.com/DeaDBeeF-Player/deadbeef/actions/workflows/windowsbuild.yml)
[![macOS Build Status](https://github.com/DeaDBeeF-Player/deadbeef/actions/workflows/macbuild.yml/badge.svg)](https://github.com/DeaDBeeF-Player/deadbeef/actions/workflows/macbuild.yml)

[Nightly GNU/Linux Builds](https://sourceforge.net/projects/deadbeef/files/Builds/master/linux/)

[Nightly Windows Builds](https://sourceforge.net/projects/deadbeef/files/Builds/master/windows/)

[Nightly macOS Builds](https://sourceforge.net/projects/deadbeef/files/Builds/master/macOS/)


## Building DeaDBeeF from source

### Linux, BSD and similar (GTK/*NIX version)

The overall process is simple, but it's hard to figure out the dependencies.

Please consult the [README file](README) in the same folder for more details,
or visit the [wiki page](https://github.com/DeaDBeeF-Player/deadbeef/wiki/Detailed-Build-Instructions)

If you have the dependencies - follow these simple steps:

* Install Git, Clang toolchain
* Remember to get submodules: `git submodule update --init`
* Install dependencies
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

#### Prerequisites

* MSYS2: Install the 64-bit version of [msys2](https://www.msys2.org/) and ensure to run `pacman -Syu`
* Premake5: [v5.0.0-beta1](https://premake.github.io/download)
* Toolchain
```
pacman -S mingw-w64-x86_64-libzip mingw-w64-x86_64-pkg-config mingw-w64-x86_64-dlfcn mingw-w64-x86_64-clang mingw-w64-x86_64-libblocksruntime git make tar xz
```
* Dependencies:
```
pacman -S mingw-w64-x86_64-jansson mingw-w64-x86_64-gtk3 mingw-w64-x86_64-gtk2 mingw-w64-x86_64-mpg123 mingw-w64-x86_64-flac mingw-w64-x86_64-portaudio
```
* Plugin dependencies: follow the [Windows plugin status page ](https://github.com/DeaDBeeF-Player/deadbeef/wiki/Windows-plugin-status) to find out dependencies of the plugins, and install them too.

#### Compiling

* Ensure that you are in mingw64 shell (run mingw64.exe) and clone this git
  repository
* From deadbeef main directory run `premake5 --standard gmake2` using your corresponding path to `premake5.exe`
* Compile with `make config=debug_windows` (debug build) or `make config=release_windows` (stripped/release build)
* Find the resulting binaries in `bin/debug` or `bin/release`

#### Other notes

* GTK3 uses [Windows-10](https://github.com/B00merang-Project/Windows-10) theme and [Windows-10-Icons](https://github.com/B00merang-Artwork/Windows-10) by default. If they are not in msys2 tree, then they must be manually placed in the `share/icons` and `share/themes`. A different theme can be specified by editing the `etc/gtk-3.0/settings.ini` file.

## Supporting this project

[Please visit the support page](http://deadbeef.sourceforge.net/support.html)
