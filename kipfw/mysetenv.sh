#!/bin/bash

# bash script to set a suitable environment to call MSVC's build
# to build a 64-bit version of the kernel.
#
# inspired by C:/winddk/7600.16385.1/bin/setenv.bat
# see http://www.osronline.com/ddkx/ddtools/build_ref_0kqb.htm

#############################################################
#  edit theese variables to meet your configuration         #
#  - DRIVE is the hard drive letter where DDK is installed  #
#  - DDK is the path to the DDK's root directory            #
#  - CYGDDK is the complete cygwin path to DDK              #
#############################################################
if [ $# -ne 3 ]; then
echo "invalid params" && exit 1
fi
DRIVE=$1
DDK=$2
CYGDDK=/cygdrive/c/${DDK}
TARGETOS=$3
MYDIR=`pwd`	# XXX luigi

if [ "$TARGETOS" = "wnet" ]; then
export DDK_TARGET_OS=WinNET
export _NT_TARGET_VERSION=0x502
fi

if [ "$TARGETOS" = "wlh" ]; then
export DDK_TARGET_OS=WinLH
export _NT_TARGET_VERSION=0x600
fi

if [ "$TARGETOS" = "win7" ]; then
export DDK_TARGET_OS=Win7
export _NT_TARGET_VERSION=0x601
fi


#############################################################
#  don't edit anything else below this point                #
#############################################################

D=${DRIVE}${DDK}
DB=${D}/bin
DI=${D}/inc
DL=${D}/lib


export AMD64=1
export ATL_INC_PATH=$DI				# defaults to DDKROOT/inc
export ATL_INC_ROOT=$DI				# XXX redundant ?
export ATL_LIB_PATH=${DL}/atl/*
export BASEDIR=$D				# default
export BUFFER_OVERFLOW_CHECKS=1
export BUILD_ALLOW_COMPILER_WARNINGS=1
export BUILD_ALT_DIR=chk_${TARGETOS}_AMD64
export BUILD_DEFAULT="-ei -nmake -i -nosqm"	# can go on the command line
export BUILD_DEFAULT_TARGETS="-amd64"		# can also go on the command line
export BUILD_MAKE_PROGRAM=nmake.exe		# default to nmake
export BUILD_MULTIPROCESSOR=1			# parallel make, same as -M
export BUILD_OPTIONS=" ~imca ~toastpkg"
export COFFBASE_TXT_FILE=${DB}/coffbase.txt
export CPU=AMD64
export CRT_INC_PATH=${DI}/crt			# default
export CRT_LIB_PATH=${DL}/crt/*			# not default, it seems uses lib/{wnet,win7}/*
export DDKBUILDENV=chk				# checked or free
export DDK_INC_PATH=${DI}/ddk
export DDK_LIB_DEST=${DL}/${TARGETOS}
export DDK_LIB_PATH=${DL}/${TARGETOS}/*
export DEPRECATE_DDK_FUNCTIONS=1
export DRIVER_INC_PATH=${DI}/ddk
export HALKIT_INC_PATH=${DI}/ddk
export HALKIT_LIB_PATH=${DL}/${TARGETOS}/*
export IFSKIT_INC_PATH=${DI}/ddk
export IFSKIT_LIB_DEST=${DL}/${TARGETOS}
export IFSKIT_LIB_PATH=${DL}/${TARGETOS}/*
export Include=${DI}/api
export KMDF_INC_PATH=${DI}/wdf/kmdf
export KMDF_LIB_PATH=${DL}/wdf/kmdf/*
export LANGUAGE_NEUTRAL=0
export Lib=${DL}
export LINK_LIB_IGNORE=4198
export MFC_INC_PATH=${DI}/mfc42
export MFC_LIB_PATH=${DL}/mfc/*
export MSC_OPTIMIZATION="/Od /Oi" 
export NEW_CRTS=1
export NO_BINPLACE=TRUE
export NO_BROWSER_FILE=TRUE
export NTDBGFILES=1
export NTDEBUG=ntsd
export NTDEBUGTYPE=both
# need NTMAKEENV to point to the binary dir
export NTMAKEENV=${DB}
export OAK_INC_PATH=${DI}/api

export PATH="${CYGDDK}/bin/amd64:${CYGDDK}/tools/sdv/bin:${CYGDDK}/tools/pfd/bin/bin/x86_AMD64\
:${CYGDDK}/bin/SelfSign:${CYGDDK}/bin/x86/amd64:${CYGDDK}/bin/x86\
:${CYGDDK}/tools/pfd/bin/bin/AMD64:${CYGDDK}/tools/tracing/amd64:$PATH"

export PATHEXT=".COM;.EXE;.BAT;.CMD;.VBS;.VBE;.JS;.JSE;.WSF;.WSH;.MSC"
export PROJECT_ROOT=${D}/src
export PUBLIC_ROOT=${D}
export RAZZLETOOLPATH=${DB}
export RCNOFONTMAP=1
export SDK_INC_PATH=${DI}/api
export SDK_LIB_DEST=${DL}/${TARGETOS}
export SDK_LIB_PATH=${DL}/${TARGETOS}/*
export SDV=${D}/tools/sdv
export separate_object_root=FALSE
export TEMP=tmpbuild
export TMP=tmpbuild
export UMDF_INC_PATH=${DI}/wdf/umdf
export USE_OBJECT_ROOT=1
export WDM_INC_PATH=${DI}/ddk
export WPP_CONFIG_PATH=${DB}/wppconfig
export _AMD64bit=true
export _BUILDARCH=AMD64
export _BuildType=chk
export _NTDRIVE=${DRIVE}
export _NTROOT=${DDK}
#
# --- XXX note, it spams  C:/winddk/7600.16385.1/build.dat
# -c: delete objs, -e: generare build.* logfiles, -f rescan sources, -g color errors
unset MAKEFLAGS
echo "emv ${MAKE} flags ${MAKEFLAGS}"
cd kipfw-mod && build -cefg 
echo "done"
#cp objchk_${TARGETOS}_amd64/amd64/ipfw.sys ../binary/ipfw.sys
