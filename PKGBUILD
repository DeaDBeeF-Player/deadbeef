# Maintainer: Alexey Yakovenko <waker@users.sourceforge.net>

pkgname=deadbeef
pkgver=0.2.2
pkgrel=2
pkgdesc="mp3/ogg/flac/ape/sid/mod/nsf music player based on GTK2"
arch=(i686 x86_64)
url="http://deadbeef.sourceforge.net"
license=('GPL2')
makedepends=('gtk2', 'libsamplerate', 'libvorbis', 'libmad', 'flac', 'curl', 'alsa-lib')
depends=('gtk2' 'libsamplerate' 'alsa-lib')
optdepends=('libvorbis: ogg vorbis playback', 'libmad: mp1/2/3 playback', 'flac: flac playback', 'curl: lastfm scrobbler')
makedepends=('pkgconfig')
source=(http://downloads.sourceforge.net/project/$pkgname/$pkgname-$pkgver.tar.gz)
md5sums=('62f745ed00ed835530b9ffc9b7e89df2')

build() {
    cd $srcdir/$pkgname-$pkgver
    ./configure --prefix=/usr
    make || return 1
    make prefix=$pkgdir/usr install
}
