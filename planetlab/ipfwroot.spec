#
# Marta Carbone <marta.carbone@iet.unipi.it>
# 2009 - Universita` di Pisa
# License is BSD.

# kernel_release, kernel_version and kernel_arch are expected to be set by the build to e.g.
# kernel_release : 24.onelab  (24 is then the planetlab taglevel)
# kernel_version : 2.6.27.57 | 2.6.32  (57 in the 27 case is the patch level)
# kernel_arch :    i686 | x86_64

# the 2012 release was pulled from http://info.iet.unipi.it/~marta/dummynet/ipfw3-20120610.tar.gz
# seel also          http://sourceforge.net/p/dummynet/code
# in 2013 Marta has moved to sourceforge at
# git clone git://git.code.sf.net/p/dummynet/code your read-only code
%define name ipfwroot
%define version 3
%define taglevel 1

# when no planetlab kernel is being built, kernel_version is defined but empty
%define _with_planetlab_kernel %{?kernel_version:1}%{!?kernel_version:0}
# we need to make sure that this rpm gets upgraded when the kernel release changes
%if %{_with_planetlab_kernel}
# with the planetlab kernel
%define pl_kernel_taglevel %( echo %{kernel_release} | cut -d. -f1 )
%define ipfw_release %{kernel_version}.%{pl_kernel_taglevel}
%else
# with the stock kernel
# this line below
#%define ipfw_release %( rpm -q --qf "%{version}" kernel-headers )
# causes recursive macro definition no matter how much you quote
%define percent %
%define braop \{
%define bracl \}
%define kernel_version %( rpm -q --qf %{percent}%{braop}version%{bracl} kernel-headers )
%define kernel_release %( rpm -q --qf %{percent}%{braop}release%{bracl} kernel-headers )
%define kernel_arch %( rpm -q --qf %{percent}%{braop}arch%{bracl} kernel-headers )
%define ipfw_release %{kernel_version}.%{kernel_release}
%endif

%define release %{ipfw_release}.%{taglevel}%{?pldistro:.%{pldistro}}%{?date:.%{date}}

# guess which convention is used; k27 and before used dash, k32 uses dot
%define kernelpath_dash /usr/src/kernels/%{kernel_version}-%{kernel_release}-%{kernel_arch}
%define kernelpath_dot /usr/src/kernels/%{kernel_version}-%{kernel_release}.%{kernel_arch}
%define kernelpath %( [ -d %{kernelpath_dot} ] && echo %{kernelpath_dot} || echo %{kernelpath_dash} )

# the k32 kernel currently builds e.g. /lib/modules/2.6.32-0.onelab.2010.12.07-i686
# the k27 and before does not have the -i686 part
%define kernel_id_old %{kernel_version}-%{kernel_release}
%define kernel_id_new %{kernel_version}-%{kernel_release}.%{kernel_arch}
%define kernel_id %( [ -d %{kernelpath_dot} ] && echo %{kernel_id_new} || echo %{kernel_id_old} )

Summary: ipfw and dummynet for Linux
Name: %{name}
Version: %{version}
Release: %{release}
License: BSD
Group: System Environment/Kernel
Source0: %{name}-%{version}.tar.bz2
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-buildroot
Requires: kernel = %{kernel_version}-%{kernel_release}
Requires: vixie-cron
Requires: vsys-scripts
Obsoletes: ipfw

Vendor: unipi
Packager: PlanetLab <marta@onelab2.iet.unipi.it>
# XXX ask 
Distribution: PlanetLab %{plrelease}
URL: %{SCMURL}

%description
ipfw is the Linux port of the FreeBSD ipfw and dummynet packages

%prep
%setup

%build
# clean the rpm build directory
rm -rf $RPM_BUILD_ROOT

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
depmod -a %{kernel_id}
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
