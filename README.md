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
* Install autoconf, automake, libtool, intltool, autopoint, libjansson-dev
* Run ```./autogen.sh``` to bootstrap
* Read the generated INSTALL file and ```./configure --help``` for instructions
* See the README file for more information

### OS X (COCOA version)

* Install XCode, and run `sudo xcode-select --install`; This would also get you git etc
* Clone the deadbeef repo, and fetch the dependencies: ```git submodule update --init```
* Install [Yasm](https://yasm.tortall.net/Download.html) -- unpack the source, then run `./configure && make -j8 && sudo make install`
* Run ```xcodebuild -project osx/deadbeef.xcodeproj -target deadbeef -configuration Release```
* Get the output: ```osx/build/Release/deadbeef.app```
* OR open the osx/deadbeef.xcodeproj in XCode, and build/run from there

----

[Support this project development](http://deadbeef.sourceforge.net/support.html)
