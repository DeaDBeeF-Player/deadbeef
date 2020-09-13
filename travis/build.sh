case "$TRAVIS_OS_NAME" in
    linux)
        ls -l .
        if [ ! -d static-deps ]; then
            rm -rf static-deps
            STATICDEPS_URL="http://sourceforge.net/projects/deadbeef/files/staticdeps/ddb-static-deps-latest.tar.bz2/download"
            mkdir static-deps
            echo "downloading static deps..."
            wget -q "$STATICDEPS_URL" -O ddb-static-deps.tar.bz2 || exit 1
            echo "unpacking static deps..."
            tar jxf ddb-static-deps.tar.bz2 -C static-deps || exit 1
        fi
#        echo "building for i686"
#        ARCH=i686 ./scripts/static_build.sh || exit 1
#        ARCH=i686 ./scripts/portable_package_static.sh || exit 1
        echo "building for x86_64"
        if [[ "$1" == "--clang" ]] ; then
            CLANG_FLAG="--clang"
        fi
        ARCH=x86_64 ./scripts/static_build.sh $CLANG_FLAG || exit 1
        ARCH=x86_64 ./scripts/portable_package_static.sh || exit 1
        echo "making deb package"
        ARCH=x86_64 ./tools/packages/debian.sh || exit 1
        echo "making arch package"
        ARCH=x86_64 ./tools/packages/arch.sh || exit 1
        echo "running make dist"
        make dist || exit 1
    ;;
    osx)
        echo gem install xcpretty ...
        gem install xcpretty 1> /dev/null 2> /dev/null || exit 1
        rev=`git rev-parse --short HEAD`
        /usr/libexec/PlistBuddy -c "Set :CFBundleVersion $rev"  plugins/cocoaui/deadbeef-Info.plist
        xcodebuild -project osx/deadbeef.xcodeproj -target DeaDBeeF -configuration Release -quiet | xcpretty ; test ${PIPESTATUS[0]} -eq 0 || exit 1
        xcodebuild test -project osx/deadbeef.xcodeproj -scheme deadbeef -configuration Release -quiet | xcpretty ; test ${PIPESTATUS[0]} -eq 0 || exit 1
        VERSION=`cat PORTABLE_VERSION | perl -ne 'chomp and print'`
        cd osx/build/Release
        zip -r deadbeef-$VERSION-osx-x86_64.zip DeaDBeeF.app || exit 1
        cd ../../..
    ;;
    windows)
        git clone https://github.com/kuba160/deadbeef-windows-deps.git
        wget https://github.com/premake/premake-core/releases/download/v5.0.0-alpha15/premake-5.0.0-alpha15-windows.zip && unzip premake-5.0.0-alpha15-windows.zip
        $mingw64 ./premake5 --os=linux --file=premake5-win.lua --standard gmake
        $mingw64 make config=release_windows || exit 1
        $mingw64 make config=debug_windows || exit 1
        cp -r deadbeef-windows-deps/Windows-10 bin/debug/share/themes/Windows-10
        cp -r deadbeef-windows-deps/Windows-10 bin/release/share/themes/Windows-10
        cp -r deadbeef-windows-deps/Windows-10-Icons bin/debug/share/icons/Windows-10-Icons
        cp -r deadbeef-windows-deps/Windows-10-Icons bin/release/share/icons/Windows-10-Icons
        echo "making zip packages"
        VERSION=`cat PORTABLE_VERSION | perl -ne 'chomp and print'`
        mv bin/release bin/deadbeef-x86_64 && (cd bin && $msys2 zip -q -r deadbeef-$VERSION-windows-x86_64.zip deadbeef-x86_64/) && mv bin/deadbeef-x86_64 bin/release
        mv bin/debug bin/deadbeef-x86_64 && (cd bin && $msys2 zip -q -r deadbeef-$VERSION-windows-x86_64_DEBUG.zip deadbeef-x86_64/) && mv bin/deadbeef-x86_64 bin/debug
        echo "making installer packages"
        /C/ProgramData/chocolatey/bin/ISCC.exe "//Obin" "//Qp" tools/windows-installer/deadbeef.iss
        /C/ProgramData/chocolatey/bin/ISCC.exe "//DDEBUG" "//Obin" "//Qp" tools/windows-installer/deadbeef.iss
    ;;
esac
