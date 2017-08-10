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

----

[![Support via Gratipay](https://cdn.rawgit.com/gratipay/gratipay-badge/2.3.0/dist/gratipay.png)](https://gratipay.com/deadbeef/)

[More ways to support](http://deadbeef.sourceforge.net/support.html)
