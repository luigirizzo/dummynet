#
# TODO:
# restart crond
# modprobe ipfw_mod.ko (depmod ?)
#

# Marta Carbone <marta.carbone@iet.unipi.it>
# 2009 - Universita` di Pisa
# License is BSD.

# the 2012 release was pulled from http://info.iet.unipi.it/~marta/dummynet/ipfw3-20120610.tar.gz
# seel also          http://sourceforge.net/p/dummynet/code
# in 2013 Marta has moved to sourceforge at
# git clone git://git.code.sf.net/p/dummynet/code your read-only code
%define name ipfwslice
%define version 3
%define taglevel 1

%define release %{taglevel}%{?pldistro:.%{pldistro}}%{?date:.%{date}}

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
URL: %{SCMURL}

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
* Mon Jul 09 2012 Thierry Parmentelat <thierry.parmentelat@sophia.inria.fr> - ipfw-20120610-2
- cosmetic changes only in specfile

* Fri Jun 15 2012 Thierry Parmentelat <thierry.parmentelat@sophia.inria.fr> - ipfw-20120610-1
- integrated ipfw3 as of 20120610 from upstream

* Mon Oct 24 2011 Thierry Parmentelat <thierry.parmentelat@sophia.inria.fr> - ipfw-0.9-23
- for building against k32 on f8

* Sun Oct 02 2011 Thierry Parmentelat <thierry.parmentelat@sophia.inria.fr> - ipfw-0.9-22
- rpm version number has the kernel taglevel embedded

* Fri Jun 10 2011 Thierry Parmentelat <thierry.parmentelat@sophia.inria.fr> - ipfw-0.9-21
- build tweaks for gcc-4.6 on f15

* Sun Jan 23 2011 Thierry Parmentelat <thierry.parmentelat@sophia.inria.fr> - ipfw-0.9-20
- tweaks for compiling on k32/64 bits

* Wed Dec 08 2010 Thierry Parmentelat <thierry.parmentelat@sophia.inria.fr> - ipfw-0.9-19
- fix detection of kernel conventions

* Tue Dec 07 2010 Thierry Parmentelat <thierry.parmentelat@sophia.inria.fr> - ipfw-0.9-18
- guess conventions for either <=k27 or >=k32

* Tue Jun 15 2010 Baris Metin <Talip-Baris.Metin@sophia.inria.fr> - ipfw-0.9-17
- testing git only module-tag

* Tue Jun 15 2010 Baris Metin <Talip-Baris.Metin@sophia.inria.fr> - ipfw-0.9-16
- tagging ipfw to test module-tools on (pure) git

* Wed May 12 2010 Talip Baris Metin <Talip-Baris.Metin@sophia.inria.fr> - ipfw-0.9-15
- tagging for obsoletes

* Tue Apr 27 2010 Thierry Parmentelat <thierry.parmentelat@sophia.inria.fr> - ipfw-0.9-13
- Update to the ipfw3 version of the dummynet code.

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
