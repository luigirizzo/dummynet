#
# $Id: ipfwslice.spec 16174 2009-12-15 13:38:15Z marta $
#
# TODO:
# restart crond
# modprobe ipfw_mod.ko (depmod ?)
#
%define url $URL: svn+ssh://onelab2/home/svn/ports-luigi/ipfw3-2012/planetlab/ipfwslice.spec $

# Marta Carbone <marta.carbone@iet.unipi.it>
# 2009 - Universita` di Pisa
# License is BSD.

%define name ipfwslice
%define version 0.9
%define taglevel 11

%define release %{kernel_version}.%{taglevel}%{?pldistro:.%{pldistro}}%{?date:.%{date}}
%define kernel_id_arch %{kernel_version}-%{kernel_release}-%{kernel_arch}
%define kernel_id %{kernel_version}-%{kernel_release}

Summary: ipfw and dummynet for Linux
Name: %{name}
Version: %{version}
Release: %{release}
License: BSD
Group: System Environment/Kernel
Source0: %{name}-%{version}.tar.bz2
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-buildroot

Vendor: unipi
Packager: PlanetLab <marta@onelab2.iet.unipi.it>
Distribution: PlanetLab %{plrelease}
URL: %(echo %{url} | cut -d ' ' -f 2)

%description
the frontend part of the ipfw planetlab package

%prep
%setup

%build
rm -rf $RPM_BUILD_ROOT

%install
install -D -m 755 planetlab/netconfig $RPM_BUILD_ROOT/sbin/netconfig
install -D -m 755 planetlab/ipfw.8.gz $RPM_BUILD_ROOT/%{_mandir}/man8/ipfw.8.gz

%clean
rm -rf $RPM_BUILD_ROOT

# here there is a list of the final installation directories
%files
%defattr(-,root,root)
/sbin/netconfig
%{_mandir}/man8/ipfw.8*

%changelog
* Mon Apr 12 2010 Thierry Parmentelat <thierry.parmentelat@sophia.inria.fr> - ipfw-0.9-11
- add ipfw initialization script to chkconfig

* Wed Mar 03 2010 Talip Baris Metin <Talip-Baris.Metin@sophia.inria.fr> - ipfw-0.9-10
- - Load module at installation - Marta

* Mon Jan 11 2010 Thierry Parmentelat <thierry.parmentelat@sophia.inria.fr> - ipfw-0.9-9
- consistent with vsys-scripts-0.95-13

* Sat Jan 09 2010 Thierry Parmentelat <thierry.parmentelat@sophia.inria.fr> - ipfw-0.9-8
- builds on 2.6.22 & 2.6.27 - for 32 and 64 bits

* Tue Dec 15 2009 Marta Carbone <marta.carbone@iet.unipi.it>
- more work on the radix code, added sysctl read/write support

* Sun Nov 29 2009 Thierry Parmentelat <thierry.parmentelat@sophia.inria.fr> - ipfw-0.9-7
- added missing qsort.c - tag 0.9-6 was broken

* Thu Nov 26 2009 Thierry Parmentelat <thierry.parmentelat@sophia.inria.fr> - ipfw-0.9-6
- root: removed goto into the main ipfw switch, enabled slice_id matching
- slice: completely move netconfig checks into the backend

* Mon Nov 09 2009 Thierry Parmentelat <thierry.parmentelat@sophia.inria.fr> - ipfw-0.9-5
- additional features on matching packets, including uid match

* Mon Sep 07 2009 Thierry Parmentelat <thierry.parmentelat@sophia.inria.fr> - ipfw-0.9-4
- on behalf of Marta Carbone, more options and features

* Thu Jul 23 2009 Thierry Parmentelat <thierry.parmentelat@sophia.inria.fr> - ipfw-0.9-3
- fixed memory usage issue

* Wed Jul 15 2009 Thierry Parmentelat <thierry.parmentelat@sophia.inria.fr> - ipfw-0.9-2
- patch for building on x86_64

* Thu Jun 25 2009 Marta Carbone <marta.carbone@iet.unipi.it>
- Initial release
