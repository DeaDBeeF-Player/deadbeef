# option to compile without XMMS plugin
%if %{?without_xmms:1}%{!?without_xmms:0}
 %define _without_xmms --without-xmms
%else
 %define _with_xmms --with-xmms
%endif
# this has been taken from http://www.hyperborea.org/software/dillo/dillo.spec
#################################################################################
# Identify which distribution we're building on.
# This will determine any changes in menu structure or release number (i.e. .mdk)
# Eventually, need to make this configurable from the rpmbuild command line.

%define freedesktop %(if [ -e /usr/share/applications ]; then echo 1; else echo 0; fi;)
%define conectiva %(if [ -e /etc/conectiva-release ]; then echo 1; else echo 0; fi;)
%define mdk  %(if [ -e /etc/mandrake-release ]; then echo 1; else echo 0; fi;)
%define suse %(if [ -e /etc/SuSE-release ]; then echo 1; else echo 0; fi;)
%define oldsuse 0
%if %{suse}
	%define oldsuse %(if [ `grep VERSION /etc/SuSE-release | sed -e "s/VERSION = //"` \\< 8.0 ]; then echo 1; else echo 0; fi;)
%endif

%define oldredhat %(if [ -e /etc/redhat-release ]; then echo 1; else echo 0; fi;) && !%{mdk} && !%{suse} && !%{conectiva} && !%{freedesktop}
%define plain !%{mdk} && !%{suse} && !%{conectiva} && !%{oldredhat} && !%{freedesktop}

Summary:    C library and frontend for decoding MPEG2/4 AAC
Name:       faad2
Version:    2.7.0
Release:    1
License:    GPL
Group:      Applications/Multimedia
Source0:    http://download.sourceforge.net/faad/%{name}-%{version}.tar.gz
#Patch:                faad2-%{version}.patch
BuildRequires: autoconf, automake, libtool, gcc-c++

%if %{?_with_xmms:1}%{!?_with_xmms:0}
BuildRequires: xmms-devel
%endif

URL:        http://www.audiocoding.com/
#################################################################################
%if %{?_with_xmms:1}%{!?_with_xmms:0}
# GTK Dependencies
%if %{mdk}
BuildRequires: libgtk+-devel >= 1.2.0
%endif
%if %{suse}
BuildRequires: gtk-devel >= 1.2.0
%endif
%if !%{suse} && !%{mdk}
BuildRequires: gtk+-devel >= 1.2.0
%endif
%endif

BuildRoot:  %{_tmppath}/%{name}-%{version}-%{release}-root
Packager:   a.kurpiers@nt.tu-darmstadt.de

%description
FAAD 2 is a LC, MAIN and LTP profile, MPEG2 and MPEG-4 AAC decoder, completely
written from scratch. FAAD 2 is licensed under the GPL.

Includes libmp4ff, a Quicktime library for UNIX in a freely redistributable,
statically linkable library.

%package devel
Summary: Development libraries the FAAD 2 AAC decoder.
Group: Development/Libraries
Requires: %{name}

%description devel
Header files and development documentation for libfaad.

%if %{?_with_xmms:1}%{!?_with_xmms:0}
%package xmms
Group: Applications/Multimedia
Summary: AAC and MP4 input plugin for xmms
Requires: %{name}, xmms


%description xmms
The AAC xmms input plugin for xmms recognizes AAC files by an
.aac extension.
This MP4 xmms plugin reads AAC files with and without ID3 tags (version 2.x).
AAC files are MPEG2 or MPEG4 files that can be found in MPEG4 audio files
(.mp4). MPEG4 files with AAC inside can be read by RealPlayer or Quicktime.
%endif

%prep
#%setup -n %{name}
%setup -n %{name}-%{version}
#%patch -p0

%build
#sh bootstrap
./configure --with-drm %{?_with_xmms} %{?_without_xmms} --prefix=/usr
make

%install
rm -rf %{buildroot}
# Hack to work around a problem with DESTDIR in libtool 1.4.x
LIBRARY_PATH="%{buildroot}/usr/lib:${LIBRARY_PATH}" make install DESTDIR=%{buildroot}
# install libmp4ff
install -m 755 common/mp4ff/libmp4ff.a %{buildroot}%{_libdir}
install common/mp4ff/mp4ff.h %{buildroot}%{_includedir}

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%clean
rm -rf %{buildroot}

%files
%defattr(-, root, root)
%doc AUTHORS COPYING ChangeLog NEWS README TODO
%{_bindir}/*
%{_libdir}/libfaad.so*

%files devel
%defattr(-, root, root)
%{_libdir}/libfaad.a
%{_libdir}/libfaad.la
%{_includedir}/faad.h
%{_includedir}/neaacdec.h
%{_includedir}/mp4ff.h
%{_libdir}/libmp4ff.a

%if %{?_with_xmms:1}%{!?_with_xmms:0}
%files xmms
%defattr(-,root,root)
%doc plugins/xmms/README
%_libdir/xmms/Input/*
%endif

%changelog
* Tue Jan 24 2006 Alexander Kurpiers <a.kurpiers@nt.tu-darmstadt.de>
- fix wrong function declaration in mp4ffint.h

* Wed Nov 15 2005 Alexander Kurpiers <a.kurpiers@nt.tu-darmstadt.de>
- fix requirements for xmms plugin
- add libmp4ff to devel package (only static library)

* Sat Aug 13 2005 Alexander Kurpiers <a.kurpiers@nt.tu-darmstadt.de>
- fix dependencies for xmms plugin. libmp4v2/id3lib no longer needed

* Sun Apr 24 2005 Alexander Kurpiers <a.kurpiers@nt.tu-darmstadt.de>
- make xmms plugin generation optional. Build with '--define "without_xmms 1"'

* Tue Nov 02 2004 Alexander Kurpiers <a.kurpiers@nt.tu-darmstadt.de>
- remove libmp4ff and libmp4v2 from RPM
- changes for new version of faad2

* Sat Apr 17 2004 Alexander Kurpiers <a.kurpiers@nt.tu-darmstadt.de>
- apply fix to make DRM support work
- use "platform-independant" dependencies (taken from dillo.spec)

* Fri Feb 06 2004 Alexander Kurpiers <a.kurpiers@nt.tu-darmstadt.de>
- remove seperate libmp4ff target

* Wed Nov 05 2003 Alexander Kurpiers <a.kurpiers@nt.tu-darmstadt.de>
- include xmms plugins/libmp4v2/libmp4ff into RPM

* Tue Aug 12 2003 Matthias Saou <matthias.saou@est.une.marmotte.net>
- Update to 2.0rc1.
- Introduced LD_LIBRARY_PATH workaround.
- Removed optional xmms plugin build, it seems mandatory now.
- Added gtk+ build dep for the xmms plugin.

* Wed May 14 2003 Matthias Saou <matthias.saou@est.une.marmotte.net>
- Added xmms plugin build.

* Wed Apr  9 2003 Matthias Saou <matthias.saou@est.une.marmotte.net>
- Spec file cleanup.
- Now exclude .la file.
- Update to latest CVS checkout to fix compile problem.


* Fri Aug 10 2002 Alexander Kurpiers <a.kurpiers@nt.tu-darmstadt.de>
- changes to compile v1.1 release

* Tue Jun 18 2002 Alexander Kurpiers <a.kurpiers@nt.tu-darmstadt.de>
- First RPM.

