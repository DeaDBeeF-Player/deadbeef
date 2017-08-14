case "$TRAVIS_OS_NAME" in
    linux)
        if [ "$BUILD_WINDOWS" = "yes" ]
        then
            sudo apt-get update 1> /dev/null 2> /dev/null || exit 1
            sudo apt-get install -qq autopoint automake autoconf intltool libc6-dev-i386 libc6-dev yasm libglib2.0-bin || exit 1
            sudo apt-get install -qq binutils-mingw-w64-x86-64 gcc-mingw-w64-x86-64  win-iconv-mingw-w64-dev libz-mingw-w64-dev
            STATICDEPS_GIT="http://github.com/kuba160/deadbeef-windows-deps"
            echo "downloading static deps..."
            git clone -q "$STATICDEPS_GIT" || exit 1
            echo "installing static deps..."
            cd deadbeef-windows-deps && sudo make install ; cd ..
            echo "building for x86_64"
            ARCH=x86_64 ./scripts/configure_windows.sh --host=x86_64-w64-mingw32 --prefix="$PWD/deadbeef-win64-build" || exit 1
            make LDFLAGS=-no-undefined
            make install
            mkdir static-deps
            echo "running make dist"
            make dist || exit 1
        else
            STATICDEPS_URL="http://sourceforge.net/projects/deadbeef/files/staticdeps/ddb-static-deps-latest.tar.bz2/download"
            mkdir static-deps
            echo "downloading static deps..."
            wget -q "$STATICDEPS_URL" -O ddb-static-deps.tar.bz2 || exit 1
            echo "unpacking static deps..."
            tar jxf ddb-static-deps.tar.bz2 -C static-deps || exit 1
            echo "installing the needed build dependencies..."
            sudo apt-get update 1> /dev/null 2> /dev/null || exit 1
            sudo apt-get install -qq autopoint automake autoconf intltool libc6-dev-i386 libc6-dev yasm libglib2.0-bin || exit 1
            echo "building for i686"
            ARCH=i686 ./scripts/static_build.sh || exit 1
            ARCH=i686 ./scripts/portable_package_static.sh || exit 1
            echo "building for x86_64"
            ARCH=x86_64 ./scripts/static_build.sh || exit 1
            ARCH=x86_64 ./scripts/portable_package_static.sh || exit 1
            echo "running make dist"
            make dist || exit 1
            fi
    ;;
    osx)
        echo brew update ...
        brew update 1> /dev/null 2> /dev/null || exit 1
        echo brew install yasm ...
        brew install yasm 1> /dev/null 2> /dev/null || exit 1
        #echo gem install xcpretty ...
        #gem install xcpretty 1> /dev/null 2> /dev/null || exit 1
        git submodule update --init || exit 1
        #xcodebuild -project osx/deadbeef.xcodeproj -target deadbeef -configuration Release | xcpretty ; test ${PIPESTATUS[0]} -eq 0 || exit 1
        xcodebuild -project osx/deadbeef.xcodeproj -target deadbeef -configuration Release | awk '{ if (length($0) < 300) print }' ; test ${PIPESTATUS[0]} -eq 0 || exit 1
        VERSION=`cat PORTABLE_VERSION | perl -ne 'chomp and print'`
        cd osx/build/Release
        zip -r deadbeef-$VERSION-osx-x86_64.zip deadbeef.app || exit 1
        cd ../../..
    ;;
esac
