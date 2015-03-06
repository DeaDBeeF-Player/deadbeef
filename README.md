## About

DeaDBeeF is a music player for \*nix-like systems and OS X.

More info is [here](http://deadbeef.sf.net).

## Drone.io

[![Build Status](https://drone.io/github.com/Alexey-Yakovenko/deadbeef/status.png)](https://drone.io/github.com/Alexey-Yakovenko/deadbeef/latest)

[Download latest GNU/Linux i686 and x86_64 builds](https://drone.io/github.com/Alexey-Yakovenko/deadbeef/files)

The OS X version is not production-ready yet, so I'm not making any official builds. Please use the inscructions below to make your own build, if you feel adventurous.

## Reporting bugs

Please use [the bug tracker](http://code.google.com/p/ddb/issues/list)

## Compiling

### Linux, BSD and similar (GTK/*NIX version)

See the INSTALL file

### OS X (COCOA version)

* Install XCode
* Install [Yasm](http://rudix.org/packages/yasm.html)
* Run ```xcodebuild -project osx/deadbeef.xcodeproj -target deadbeef -configuration Deployment```
* Get the output: ```osx/build/Release/deadbeef.app```
* OR open the osx/deadbeef.xcodeproj in XCode, and build/run from there

----

[![Support via Gittip](https://rawgithub.com/twolfson/gittip-badge/0.1.0/dist/gittip.png)](https://www.gittip.com/Alexey-Yakovenko/)

[More ways to support](http://deadbeef.sourceforge.net/support.html)
