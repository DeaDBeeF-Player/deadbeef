#!/bin/bash
set -e
case "$TRAVIS_OS_NAME" in
    linux)
        ls -l .
#        echo "building for i686"
#        ARCH=i686 ./scripts/static_build.sh
#        ARCH=i686 ./scripts/portable_package_static.sh
        echo "Building for x86_64"
        if [[ "$1" == "--clang" ]] ; then
            CLANG_FLAG="--clang"
        fi
        ARCH=x86_64 ./scripts/static_build.sh $CLANG_FLAG
        ARCH=x86_64 ./scripts/portable_package_static.sh
        echo "Making deb package"
        ARCH=x86_64 ./tools/packages/debian.sh
        echo "Making arch package"
        ARCH=x86_64 ./tools/packages/arch.sh
        echo "Running make dist"
        make dist
    ;;
    osx)
        echo gem install xcpretty ...
        gem install xcpretty 1> /dev/null 2> /dev/null
        VERSION=`tr -d '\r' < PORTABLE_VERSION`
        /usr/libexec/PlistBuddy -c "Set CFBundleShortVersionString $VERSION" plugins/cocoaui/deadbeef-Info.plist
        rev=`git rev-parse --short HEAD`
        /usr/libexec/PlistBuddy -c "Set :CFBundleVersion $rev"  plugins/cocoaui/deadbeef-Info.plist
        xcodebuild "MACOSX_DEPLOYMENT_TARGET=10.13" test -project osx/deadbeef.xcodeproj -scheme deadbeef -configuration Release -quiet | xcpretty ; test ${PIPESTATUS[0]} -eq 0
        xcodebuild "MACOSX_DEPLOYMENT_TARGET=10.13" -project osx/deadbeef.xcodeproj -target DeaDBeeF -configuration Release -quiet | xcpretty ; test ${PIPESTATUS[0]} -eq 0
        cd osx/build/Release
        zip -r deadbeef-$VERSION-macos-universal.zip DeaDBeeF.app
        cd ../../..
    ;;
    windows)
        DISPATCH_URL="http://sourceforge.net/projects/deadbeef/files/staticdeps/ddb-xdispatch-win-latest.zip/download"
        PREMAKE_URL="https://github.com/premake/premake-core/releases/download/v5.0.0-beta2/premake-5.0.0-beta2-windows.zip"
        DEPS_URL="https://github.com/kuba160/deadbeef-windows-deps.git"
        echo "Downloading xdispatch_ddb..."
        wget -q "$DISPATCH_URL" -O ddb-xdispatch-win-latest.zip
        echo "Unpacking xdispatch_ddb..."
        $mingw64 unzip ddb-xdispatch-win-latest.zip
        echo "Downloading windows deps..."
        git clone "$DEPS_URL"
        wget "$PREMAKE_URL" -O premake.zip && unzip premake.zip
        echo "Downgrading openssh"
        wget http://repo.msys2.org/msys/x86_64/openssh-8.7p1-1-x86_64.pkg.tar.zst
        pacman --noconfirm -U openssh-8.7p1-1-x86_64.pkg.tar.zst
        echo "Building for x86_64"
        $mingw64 ./premake5 --standard gmake2
        $mingw64 make config=release_windows CC=clang CXX=clang++
        $mingw64 ./premake5 --standard --debug-console gmake2
        $mingw64 make config=debug_windows CC=clang CXX=clang++
        cp -r deadbeef-windows-deps/Windows-10 bin/debug/share/themes/Windows-10
        cp -r deadbeef-windows-deps/Windows-10 bin/release/share/themes/Windows-10
        cp -r deadbeef-windows-deps/Windows-10-Icons bin/debug/share/icons/Windows-10-Icons
        cp -r deadbeef-windows-deps/Windows-10-Icons bin/release/share/icons/Windows-10-Icons
        echo "Making zip packages"
        VERSION=`tr -d '\r' < PORTABLE_VERSION`
        mv bin/release bin/deadbeef-x86_64 && (cd bin && $msys2 zip -q -r deadbeef-$VERSION-windows-x86_64.zip deadbeef-x86_64/) && mv bin/deadbeef-x86_64 bin/release
        mv bin/debug bin/deadbeef-x86_64 && (cd bin && $msys2 zip -q -r deadbeef-$VERSION-windows-x86_64_DEBUG.zip deadbeef-x86_64/) && mv bin/deadbeef-x86_64 bin/debug
        echo "Making installer packages"
        /C/ProgramData/chocolatey/bin/ISCC.exe "//Obin" "//Qp" tools/windows-installer/deadbeef.iss
        /C/ProgramData/chocolatey/bin/ISCC.exe "//DDEBUG" "//Obin" "//Qp" tools/windows-installer/deadbeef.iss
    ;;
esac
