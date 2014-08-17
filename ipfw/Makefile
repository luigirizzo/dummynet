#
# $Id: Makefile 11688 2012-08-12 20:58:26Z luigi $
#
# GNUMakefile to build the userland part of ipfw on Linux and Windows
#
# Do not set with = or := so we can inherit from the caller

include ../Makefile.inc

all: $(TARGET)

#TCC=c:/path/to/tcc

# common flags
EXTRA_CFLAGS += -O1
EXTRA_CFLAGS += -Wall
EXTRA_CFLAGS += -include ../glue.h
EXTRA_CFLAGS += -I ./include_e -I ./include

TARGET := ipfw
ifneq ($(VER),openwrt)
ifeq ($(OSARCH),Linux)
    EXTRA_CFLAGS += -D__BSD_VISIBLE
    EXTRA_CFLAGS += -Werror
    # Required by GCC 4.6
    EXTRA_CFLAGS += -Wno-unused-but-set-variable
endif
ifeq ($(OSARCH),FreeBSD)
    EXTRA_CFLAGS += -D__BSD_VISIBLE
    EXTRA_CFLAGS += -Werror
endif
ifeq ($(OSARCH),Darwin)
    EXTRA_CFLAGS += -D__BSD_VISIBLE
    EXTRA_CFLAGS += -Werror
endif

ifeq ($(OSARCH),Windows)
# we only support Cygwin and tcc as compilers.
ifeq ($(WIN64),1)
    EXTRA_CFLAGS += -D_X64EMU
endif

ifeq ($(TCC),)	# cygwin
    EXTRA_CFLAGS += -I/cygdrive/c/$(DDKDIR)/inc/ddk
    EXTRA_CFLAGS += -I .
    EXTRA_CFLAGS += -pipe -Wall
else		#-- build with tcc
    # TCC points to the root of tcc tree
    CC=$(TCC)/tcc.exe
    EXTRA_CFLAGS += -DTCC -I..
    EXTRA_CFLAGS += -I$(TCC)/include/winapi -I$(TCC)/include
    EXTRA_CFLAGS += -nostdinc

    EFILES_. += err.h grp.h netdb.h pwd.h sysexits.h
    EFILES_arpa += inet.h
    EFILES_net += if.h
    EFILES_netinet += in.h in_systm.h ip.h ip_icmp.h
    EFILES_sys += cdefs.h wait.h ioctl.h socket.h

endif
    # EXTRA_CFLAGS += -D_WIN32 # see who defines it
    EXTRA_CFLAGS += -Dsetsockopt=wnd_setsockopt
    EXTRA_CFLAGS += -Dgetsockopt=wnd_getsockopt
    EXTRA_CFLAGS += -DEMULATE_SYSCTL
    EFILES_net += ethernet.h route.h
    EFILES_netinet += ether.h icmp6.h
    EFILES_sys += sysctl.h
    TARGET = ipfw.exe
ipfw: $(TARGET)
endif # windows
endif # !openwrt

CFLAGS += $(EXTRA_CFLAGS)
# Location of OS headers and libraries. After our stuff.
USRDIR?= /usr
ifeq ($(TCC),)
    CFLAGS += -I$(USRDIR)/include
    LDFLAGS += -L$(USRDIR)/lib
else
    LDFLAGS += -L. -L$(TCC)/lib -lws2_32
endif

OBJS = ipfw2.o dummynet.o main.o ipv6.o qsort_r.o
OBJS += expand_number.o humanize_number.o glue.o

# we don't use ALTQ
CFLAGS += -DNO_ALTQ
#OBJS += altq.o

all: $(TARGET)
	-@echo "Done build for $(OSARCH) VER $(VER)"

$(TARGET): $(OBJS)
	$(MSG) "   LD  $@"
	$(HIDE)$(CC) $(LDFLAGS) -o $@ $^

$(OBJS) : ipfw2.h ../glue.h include_e

# support to create empty dirs and files in include_e/
# EDIRS is the list of directories, EFILES is the list of files.
EFILES_sys  += sockio.h
EFILES_.    += libutil.h
EFILES_netinet += __emtpy.h

M ?= $(shell pwd)

# first make a list of directories from variable names
EDIRS= $(subst EFILES_,,$(filter EFILES_%,$(.VARIABLES)))
# then prepend the directory name to individual files.
#       $(empty) serves to interpret the following space literally,
#       and the ":  = " substitution packs spaces into one.
EFILES = $(foreach i,$(EDIRS),$(subst $(empty) , $(i)/, $(EFILES_$(i):  = )))

include_e:
	$(MSG) "building include_e in $M"
	-@rm -rf $(M)/include_e opt_*
	-@mkdir -p $(M)/include_e
	-@(cd $(M)/include_e; mkdir -p $(EDIRS); touch $(EFILES) )
	-@(cd $(M)/include_e/netinet; \
		for i in ip_fw.h ip_dummynet.h tcp.h; do \
		cp ../../../sys/netinet/$$i .; done; )

clean distclean:
	-@rm -rf $(OBJS) $(TARGET) include_e

diff:
	-@(diff -upr $(BSD_HEAD)/sbin/ipfw .)

