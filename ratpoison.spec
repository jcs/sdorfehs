%define name ratpoison
%define version 0.2.0
%define release 1

Summary: ratpoison - Simple window manager with no fat library dependencies.
Name: %{name}
Version: %{version}
Release: %{release}
Source: %{name}-%{version}.tar.gz
BuildRoot: /tmp/root-%{name}-%{version}-%{release}
URL: http://ratpoison.sourceforge.net/
Copyright: GPL
Group: User Interface/X

%description
ratpoison is a simple Window Manager with no fat library
dependencies, no fancy graphics, no window decorations,
and no flashy wank. It is largely modelled after GNU
Screen which has done wonders in virtual terminal market.

All interaction with the window manager is done through
keystrokes. ratpoison has a prefix map to minimize the
key clobbering that cripples EMACS and other quality
pieces of software.

%prep
mkdir -p $RPM_BUILD_ROOT

%setup 
rm -f config.cache
./configure	--prefix=/usr \
		--infodir=/usr/share/info \
		--mandir=/usr/share/man

%build
make 

%install
make install DESTDIR=$RPM_BUILD_ROOT

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
/usr/bin/ratpoison
/usr/share/doc/ratpoison/AUTHORS
/usr/share/doc/ratpoison/COPYING
/usr/share/doc/ratpoison/ChangeLog
/usr/share/doc/ratpoison/NEWS
/usr/share/doc/ratpoison/README
/usr/share/info/ratpoison.info*
/usr/share/man/man1/ratpoison.1*
