# Maintainer: Alexey Yakovenko <waker@users.sourceforge.net>

pkgname=deadbeef
pkgver=0.1.1
pkgrel=1
pkgdesc="mp3/ogg/flac/sid/mod/nsf music player based on GTK2"
arch=(i686 x86_64)
url="http://deadbeef.sourceforge.net"
license=('GPL2')
depends=('gtk2' 'libsamplerate' 'libvorbis' 'libmad' 'flac' 'alsa-lib')
makedepends=('pkgconfig')
source=(http://downloads.sourceforge.net/project/$pkgname/$pkgname-$pkgver.tar.gz)
md5sums=('3735cfe880ceb060ba53b0a67aab6a67')

build() {
    cd $srcdir/$pkgname-$pkgver
    ./configure --prefix=/usr
    make || return 1
    make prefix=$pkgdir/usr install
}
