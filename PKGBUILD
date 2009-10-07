# Maintainer: Alexey Yakovenko <waker@users.sourceforge.net>

pkgname=deadbeef
pkgver=0.2.3.2
pkgrel=1
pkgdesc="mp3/ogg/flac/ape/sid/mod/nsf music player based on GTK2"
arch=(i686 x86_64)
url="http://deadbeef.sourceforge.net"
license=('GPL2')
makedepends=('gtk2', 'libsamplerate', 'libvorbis', 'libmad', 'flac', 'curl', 'alsa-lib', 'wavpack', 'libsndfile')
depends=('gtk2' 'libsamplerate' 'alsa-lib')
optdepends=('libvorbis: ogg vorbis playback', 'libmad: mp1/2/3 playback', 'flac: flac playback', 'curl: lastfm scrobbler', 'wavpack: wv playback', 'libsndfile: wav playback')
makedepends=('pkgconfig')
source=(http://downloads.sourceforge.net/project/$pkgname/$pkgname-$pkgver.tar.gz)
md5sums=('b0401f725c3dbc5b7eefe2b7218a564f')

build() {
    cd $srcdir/$pkgname-$pkgver
    ./configure --prefix=/usr
    make || return 1
    make prefix=$pkgdir/usr install
}
