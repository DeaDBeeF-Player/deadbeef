## About

DeaDBeeF is a music player for \*nix-like systems and OS X.

More info is [here](http://deadbeef.sf.net).

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
* Run ```xcodebuild -project osx/deadbeef.xcodeproj -target deadbeef -configuration Deployment```
* Get the output: ```osx/build/Release/deadbeef.app```
* OR open the osx/deadbeef.xcodeproj in XCode, and build/run from there

----

[![Support via Gittip](https://rawgithub.com/twolfson/gittip-badge/0.1.0/dist/gittip.png)](https://www.gittip.com/Alexey-Yakovenko/)

[More ways to support](http://deadbeef.sourceforge.net/support.html)
