# Maintainer: Alexey Yakovenko <waker@users.sourceforge.net>

pkgname=deadbeef
pkgver=0.3.2
pkgrel=1
pkgdesc="mp3/ogg/flac/ape/sid/mod/nsf/m4a/mpc/shn music player based on GTK2"
arch=(i686 x86_64)
url="http://deadbeef.sourceforge.net"
license=('GPL2')
makedepends=('gtk2', 'libsamplerate', 'libvorbis', 'libmad', 'flac', 'curl', 'alsa-lib', 'wavpack', 'libsndfile', 'libcdio', 'ffmpeg')
depends=('gtk2' 'libsamplerate' 'alsa-lib')
optdepends=('libvorbis: ogg vorbis playback', 'libmad: mp1/2/3 playback', 'flac: flac playback', 'curl: lastfm scrobbler, shoutcast, icecast, podcast support', 'wavpack: wv playback', 'libsndfile: wav playback', "libcdio: audio cd playback", "ffmpeg: for aac, mpc, shn, aa3, oma, ac3, etc")
makedepends=('pkgconfig')
source=(http://downloads.sourceforge.net/project/$pkgname/$pkgname-$pkgver.tar.gz)
md5sums=('85a9a63dd507dec5b3070551107aaf17')

build() {
    cd $srcdir/$pkgname-$pkgver
    ./configure --prefix=/usr
    make || return 1
    make prefix=$pkgdir/usr install
}
