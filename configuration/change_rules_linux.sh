#!/bin/sh
#
# marta
# linux wrapper for the FreeBSD change rules program
# This file load the linux configuration and calls the
# original change rules program

if [ -r ./ipfw.conf ]; then
	. ./ipfw.conf
fi

. ./change_rules.sh
