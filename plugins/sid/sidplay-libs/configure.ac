AC_INIT(Makefile.am)

#Variables
resid=resid-0.13
pwd=`pwd`
builders='${libdir}'/sidplay/builders
ac_configure_args="$ac_configure_args --disable-library-checks"

# Setup paths
test ! -r "resid" && ln -sf "$resid" "resid"
test ! -r "unix" && ln -sf libsidplay/unix unix

AC_CONFIG_AUX_DIR(unix)
AM_INIT_AUTOMAKE(sidplay-libs,2.1.0)

if ! test -r "builders/Makefile.am"; then
echo "Generating builder Makefile.am..."
cat >>builders/Makefile.am <<EOF
SUBDIRS = resid-builder
EOF
automake -a
fi

cd $pwd/libsidplay
if ! eval ./configure $ac_configure_args \
--with-sidbuilders=$builders;
then
exit
fi

cd "$pwd/resid"
if ! eval ./configure $ac_configure_args \
--disable-resid-install;
then
exit
fi

cd $pwd/builders/resid-builder
if ! eval ./configure $ac_configure_args \
--with-sidplay2-includes=$pwd/libsidplay/include \
--with-sidplay2-library=$pwd/libsidplay/src \
--with-resid-includes=$pwd/resid \
--with-resid-library=$pwd/resid \
--libdir=$builders;
then
exit
fi

cd $pwd/libsidutils
if ! eval ./configure $ac_configure_args \
--with-sidplay2-includes=$pwd/libsidplay/include \
--with-sidplay2-library=$pwd/libsidplay/src;
then
exit
fi

cd $pwd

AC_OUTPUT(
Makefile \
builders/Makefile \
)
