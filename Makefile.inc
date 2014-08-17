# $Id$
# GNU makefile header for ipfw/kipfw building
BSD_HEAD ?= ~/FreeBSD/head
OSARCH := $(shell uname)
OSARCH := $(findstring $(OSARCH),FreeBSD Linux Darwin)
ifeq ($(OSARCH),)
    OSARCH := Windows
endif
OBJDIR=mia

KSRC ?= /lib/modules/$(shell uname -r)/build
ifneq ($V,1) # no echo
    MSG=@echo
    HIDE=@
else
    MSG=@\#
    HIDE=
endif

.c.o:
	$(MSG) "   CC $<"
	$(HIDE) $(CC) $(CFLAGS) -c $< -o $@

