# $Id: planetlab.mk 4533 2009-12-16 14:39:23Z luigi $
# .mk file to build a module
kernel-MODULES := linux-2.6
kernel-SPEC := kernel-2.6.spec 
kernel-BUILD-FROM-SRPM := yes
ifeq "$(HOSTARCH)" "i386"
kernel-RPMFLAGS:= --target i686
else
kernel-RPMFLAGS:= --target $(HOSTARCH)
endif
ALL += kernel

ipfwroot-MODULES := ipfwsrc
ipfwroot-SPEC := planetlab/ipfwroot.spec
ipfwroot-DEPEND-DEVEL-RPMS := kernel-devel
ipfwroot-SPECVARS = kernel_version=$(kernel.rpm-version) \
        kernel_release=$(kernel.rpm-release) \
        kernel_arch=$(kernel.rpm-arch)
ALL += ipfwroot 

ipfwslice-MODULES := ipfwsrc
ipfwslice-SPEC := planetlab/ipfwslice.spec
ipfwslice-SPECVARS = kernel_version=$(kernel.rpm-version) \
        kernel_release=$(kernel.rpm-release) \
        kernel_arch=$(kernel.rpm-arch)
ALL += ipfwslice
