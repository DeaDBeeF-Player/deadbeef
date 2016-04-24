VERSION=`cat PORTABLE_VERSION | perl -ne 'chomp and print'`
BUILD=`cat PORTABLE_BUILD | perl -ne 'chomp and print'`
scp package_out/i686/arch/deadbeef-static-${VERSION}-${BUILD}-i686.pkg.tar.xz waker,deadbeef@frs.sourceforge.net:/home/frs/project/d/de/deadbeef/archlinux/
scp package_out/x86_64/arch/deadbeef-static-${VERSION}-${BUILD}-x86_64.pkg.tar.xz waker,deadbeef@frs.sourceforge.net:/home/frs/project/d/de/deadbeef/archlinux/
scp package_out/i686/debian/deadbeef-static_${VERSION}-${BUILD}_i386.deb waker,deadbeef@frs.sourceforge.net:/home/frs/project/d/de/deadbeef/debian/
scp package_out/x86_64/debian/deadbeef-static_${VERSION}-${BUILD}_amd64.deb waker,deadbeef@frs.sourceforge.net:/home/frs/project/d/de/deadbeef/debian/
