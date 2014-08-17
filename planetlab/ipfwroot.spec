#
# $Id: ipfwroot.spec 16174 2009-12-15 13:38:15Z marta $
#
# TODO:
# restart crond
#
%define url $URL: svn+ssh://onelab2/home/svn/ports-luigi/ipfw3-2012/planetlab/ipfwroot.spec $

# Marta Carbone <marta.carbone@iet.unipi.it>
# 2009 - Universita` di Pisa
# License is BSD.

# kernel_release, kernel_version and kernel_arch are expected to be set by the build to e.g.
# kernel_release : vs2.3.0.29.1.planetlab
# kernel_version : 2.6.22.14

%define name ipfwroot
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
Requires: vixie-cron
Requires: vsys-scripts

Vendor: unipi
Packager: PlanetLab <marta@onelab2.iet.unipi.it>
# XXX ask 
Distribution: PlanetLab %{plrelease}
URL: %(echo %{url} | cut -d ' ' -f 2)

%description
ipfw is the Linux port of the FreeBSD ipfw and dummynet packages

%prep
%setup

%build
# clean the rpm build directory
rm -rf $RPM_BUILD_ROOT

# with the new build, we use the kernel-devel rpm for building
%define kernelpath /usr/src/kernels/%{kernel_id_arch}

%__make KERNELPATH=%kernelpath clean
%__make KERNELPATH=%kernelpath IPFW_PLANETLAB=1

%install
install -D -m 755 dummynet2/ipfw_mod.ko $RPM_BUILD_ROOT/lib/modules/%{kernel_id}/net/netfilter/ipfw_mod.ko
install -D -m 755 ipfw/ipfw $RPM_BUILD_ROOT/sbin/ipfw
install -D -m 644 planetlab/ipfw.cron $RPM_BUILD_ROOT/%{_sysconfdir}/cron.d/ipfw.cron
install -D -m 755 planetlab/ipfw $RPM_BUILD_ROOT/etc/rc.d/init.d/ipfw

%clean
rm -rf $RPM_BUILD_ROOT

%post
### this script is also triggered while the node image is being created at build-time
# some parts of the script do not make sense in this context
# this is why the build exports PL_BOOTCD=1 in such cases
depmod -a
/sbin/chkconfig --add ipfw
# start the service if not building
[ -z "$PL_BOOTCD" ] && service ipfw start

%postun
# stop the service if not building
[ -z "$PL_BOOTCD" ] && service ipfw stop

# here there is a list of the final installation directories
%files
%defattr(-,root,root)
%dir /lib/modules/%{kernel_id}
/lib/modules/%{kernel_id}/net/netfilter/ipfw_mod.ko
/sbin/ipfw
%{_sysconfdir}/cron.d/ipfw.cron
/etc/rc.d/init.d/ipfw

%changelog
* Mon Apr 12 2010 Thierry Parmentelat <thierry.parmentelat@sophia.inria.fr> - ipfw-0.9-11
- add ipfw initialization script to chkconfig

* Wed Mar 03 2010 Talip Baris Metin <Talip-Baris.Metin@sophia.inria.fr> - ipfw-0.9-10
- - Load module at installation - Marta

* Mon Jan 11 2010 Thierry Parmentelat <thierry.parmentelat@sophia.inria.fr> - ipfw-0.9-9
- consistent with vsys-scripts-0.95-13

* Mon Jan 11 2010 Marta Carbone <marta.carbone@iet.unipi.it>
- Integrated the ipfw rules cleanup into the backend

* Sat Jan 09 2010 Thierry Parmentelat <thierry.parmentelat@sophia.inria.fr> - ipfw-0.9-8
- builds on 2.6.22 & 2.6.27 - for 32 and 64 bits

* Wed Jan 06 2010 Marta Carbone <marta.carbone@iet.unipi.it>
- move to dummynet2, added support for table lookup
- added the vsys-script dependencies and the ipfw initialization

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
- post installation removed for deployment, moved manpages to the slice package

* Fri Apr 17 2009 Marta Carbone <marta.carbone@iet.unipi.it>
- Initial release
