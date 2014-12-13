Name:		vorbis-tools
Version:	1.3.0
Release:	1
Summary:	Several Ogg Vorbis Tools

Group:		Applications/Multimedia
License:	GPL
URL:		http://xiph.org/
Vendor:		Xiph.Org Foundation <team@xiph.org>
Source:         http://downloads.xiph.org/releases/vorbis/%{name}-%{version}.tar.gz
Prefix:		%{_prefix}
BuildRoot:	%{_tmppath}/%{name}-root

Requires:       libvorbis >= 1.1.1
BuildRequires:	libvorbis-devel >= 1.1.1
Requires:       flac >= 1.1.2
BuildRequires:	flac-devel >= 1.1.2
Requires:       speex >= 1.0.2
BuildRequires:	speex-devel >= 1.0.2
Requires:       libao >= 0.8.4
BuildRequires:	libao-devel >= 0.8.4
Requires:       curl >= 7.8
BuildRequires:	curl-devel >= 7.8

%description
vorbis-tools contains oggenc (an encoder) and ogg123 (a playback tool).
It also has vorbiscomment (to add comments to Vorbis files), ogginfo (to
give all useful information about an Ogg file, including streams in it),
oggdec (a simple command line decoder), and vcut (which allows you to 
cut up Vorbis files).

%prep
%setup -q -n %{name}-%{version}

%build
CFLAGS="$RPM_OPT_FLAGS" ./configure --prefix=%{_prefix} --mandir=%{_mandir}
make

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

make DESTDIR=$RPM_BUILD_ROOT install

%clean 
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc COPYING
%doc README
%doc ogg123/ogg123rc-example
%{_bindir}/oggenc
%{_bindir}/oggdec
%{_bindir}/ogg123
%{_bindir}/ogginfo
%{_bindir}/vorbiscomment
%{_bindir}/vcut
%{_datadir}/locale/*/LC_MESSAGES/*
%{_mandir}/man1/ogg123.1*
%{_mandir}/man1/oggenc.1*
%{_mandir}/man1/oggdec.1*
%{_mandir}/man1/ogginfo.1*
%{_mandir}/man1/vorbiscomment.1*
%{_mandir}/man1/vcut.1*

%changelog
* Tue Aug 31 2008 Ivo Emanuel Goncalves <justivo gmail.com>
- update for 1.2.1 release
- corrected source link
* Tue Jan 25 2008 Ivo Emanuel Goncalves <justivo gmail.com>
- update for 1.2.0 release
* Tue Oct 07 2003 Warren Dukes <shank@xiph.org>
- update for 1.0.1 release
* Fri Jul 19 2002 Michael Smith <msmith@xiph.org>
- Added oggdec and oggdec manpage.
* Sun Jul 14 2002 Thomas Vander Stichele <thomas@apestaart.org>
- updated for 1.0 release
- added vcut, vcut man and vorbiscomment man
- added LC_MESSAGES
- removed libogg and libogg-devel from requires since libvorbis pulls that in
* Fri Jul 12 2002 Michael Smith <msmith@xiph.org>
- Version number updates for 1.0 release.
* Fri May 23 2002 Thomas Vander Stichele <thomas@apestaart.org>
- Added more BuildRequires: for obvious packages
* Fri Mar 22 2002 Jack Moffitt <jack@xiph.org>
- Update curl dependency info (Closes bug #130)
* Mon Dec 31 2001 Jack Moffitt <jack@xiph.org>
- Update for rc3 release.
* Sun Oct 07 2001 Jack Moffitt <jack@xiph.org>
- Updated for configurable prefix
* Sun Aug 12 2001 Greg Maxwell <greg@linuxpower.cx>
- updated for rc2
* Sun Jun 17 2001 Jack Moffitt <jack@icecast.org>
- updated for rc1
- added ogginfo
* Mon Jan 22 2001 Jack Moffitt <jack@icecast.org>
- updated for prebeta4 builds
* Sun Oct 29 2000 Jack Moffitt <jack@icecast.org>
- initial spec file created
