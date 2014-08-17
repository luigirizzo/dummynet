# $Id: Makefile 11689 2012-08-12 21:07:34Z luigi $
#
# Top level makefile for building ipfw/dummynet (kernel and userspace).
# You can run it manually or also under the Planetlab build.
# Planetlab wants also the 'install' target.
#
# To build on system with non standard Kernel sources or userland files,
# you should run this with
#
#	make KERNELPATH=/path/to/linux-2.x.y.z USRDIR=/path/to/usr
#
# We assume that $(USRDIR) contains include/ and lib/ used to build userland.
#

include Makefile.inc

DATE ?= $(shell date +%Y%m%d)
SNAPSHOT_NAME=$(DATE)-ipfw3.tgz
BINDIST=$(DATE)-dummynet-linux.tgz
WINDIST=$(DATE)-dummynet-windows.zip

DISTFILES= Makefile Makefile.inc README binary* ipfw kipfw *.h sys

.PHONY: ipfw kipfw

###########################################
#  windows x86 and x64 specific variables #
###########################################
#  DRIVE must be the hard drive letter where DDK is installed
#  DDKDIR must be the path to the DDK root directory, without drive letter
#  TARGETOS (x64 only) must be one of the following:
#  wnet   -> windows server 2003
#  wlh    -> windows vista and windows server 2008
#  win7   -> windows 7
#  future version must be added here
DRIVE ?= C:
DDKDIR ?= /WinDDK/7600.16385.1
DDK = $(DRIVE)$(DDKDIR)
TARGETOS=win7

export WIN64
export DDK
export DRIVE
export DDKDIR

_all: all

clean distclean:
	-@(cd ipfw && $(MAKE) $(@) )
	-@rm -rf kipfw-mod binary64/[A-hj-z]*

all: kipfw ipfw
	@# -- windows only
ifeq ($(OSARCH),Windows)	# copy files
ifeq ($(WIN64),)
	-@ cp ipfw/ipfw.exe kipfw-mod/$(OBJDIR)/ipfw.sys binary/
	-@ cp kipfw/*.inf binary/
else
	-@ cp binary/* kipfw/*.inf binary64/
	-@ cp ipfw/ipfw.exe kipfw-mod/objchk_win7_amd64/amd64/ipfw.sys binary64/
endif	# WIN64
endif	# Windows

win64:
	$(MAKE) WIN64=1

# kipfw-src prepares the sources for the kernel part.
# The windows files (passthru etc.) are modified version of the
# examples found in the $(DDK)/src/network/ndis/passthru/driver/
# They can be re-created using the 'ndis-glue' target
# # We need a sed trick to remove newlines from the patchfile.

ndis-glue:
	-@mkdir -p kipfw-mod
	cp $(DDK)/src/network/ndis/passthru/driver/*.[ch] kipfw-mod
	cat kipfw/win-passthru.diff | sed "s/$$(printf '\r')//g" | (cd kipfw-mod; patch )

kipfw-src:
	-@rm -rf kipfw-mod
	-@mkdir -p kipfw-mod
	-@cp -Rp kipfw/* kipfw-mod
	-@cp `find sys -name \*.c` kipfw-mod
	-@(cd kipfw-mod && $(MAKE) include_e)
ifeq ($(OSARCH),Windows)
	make ndis-glue
endif

snapshot:
	$(MAKE) distclean
	(tar cvzhf /tmp/$(SNAPSHOT_NAME) -s':^:ipfw3-2012/:' $(DISTFILES) )

bindist:
	$(MAKE) clean
	$(MAKE) all
	tar cvzf /tmp/$(BINDIST) ipfw/ipfw ipfw/ipfw.8 kipfw-mod/ipfw_mod.ko

windist:
	$(MAKE) clean
	-$(MAKE) all
	-rm /tmp/$(WINDIST)
	zip -r /tmp/$(WINDIST) binary -x \*.svn\*


ipfw:
	@(cd ipfw && $(MAKE) $(@) )

kipfw: kipfw-src
ifeq ($(WIN64),)	# linux or windows 32 bit
	@(cd kipfw-mod && $(MAKE) $(@) )
else	#--- windows 64 bit, we use build.exe and nmake
	rm -f kipfw-mod/Makefile
	mkdir kipfw-mod/tmpbuild		# check mysetenv.sh
	bash kipfw/mysetenv.sh $(DRIVE) $(DDKDIR) $(TARGETOS)
endif

openwrt_release:
	# create a temporary directory
	$(eval TMPDIR := $(shell mktemp -d -p /tmp/ ipfw3_openwrt_XXXXX))
	# create the source destination directory
	$(eval IPFWDIR := ipfw3-$(DATE))
	$(eval DSTDIR := $(TMPDIR)/$(IPFWDIR))
	mkdir $(DSTDIR)
	# copy the package, clean objects and svn info
	cp -r ./ipfw ./kipfw-mod glue.h Makefile ./configuration README $(DSTDIR)
	(cd $(DSTDIR); make -s distclean; find . -name .svn | xargs rm -rf)
	(cd $(TMPDIR); tar czf $(IPFWDIR).tar.gz $(IPFWDIR))

	# create the port files in /tmp/ipfw3-port
	$(eval PORTDIR := $(TMPDIR)/ipfw3)
	mkdir -p $(PORTDIR)/patches
	# generate the Makefile, PKG_VERSION and PKG_MD5SUM
	md5sum $(DSTDIR).tar.gz | cut -d ' ' -f 1 > $(TMPDIR)/md5sum
	cat ./OPENWRT/Makefile | \
		sed s/PKG_VERSION:=/PKG_VERSION:=$(DATE)/ | \
		sed s/PKG_MD5SUM:=/PKG_MD5SUM:=`cat $(TMPDIR)/md5sum`/ \
		> $(PORTDIR)/Makefile

	@echo ""
	@echo "The openwrt port is in $(TMPDIR)/ipfw3-port"
	@echo "The source file should be copied to the public server:"
	@echo "scp $(DSTDIR).tar.gz marta@info.iet.unipi.it:~marta/public_html/dummynet"
	@echo "after this the temporary directory $(TMPDIR) can be removed."

install:

diff:
	-@(diff -upr $(BSD_HEAD)/sbin/ipfw ipfw)
	-@(diff -upr $(BSD_HEAD)/sys sys)

