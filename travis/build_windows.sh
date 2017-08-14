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
