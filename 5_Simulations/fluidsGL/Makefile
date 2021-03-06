################################################################################
#
# Copyright 1993-2013 NVIDIA Corporation.  All rights reserved.
#
# NOTICE TO USER:
#
# This source code is subject to NVIDIA ownership rights under U.S. and
# international Copyright laws.
#
# NVIDIA MAKES NO REPRESENTATION ABOUT THE SUITABILITY OF THIS SOURCE
# CODE FOR ANY PURPOSE.  IT IS PROVIDED "AS IS" WITHOUT EXPRESS OR
# IMPLIED WARRANTY OF ANY KIND.  NVIDIA DISCLAIMS ALL WARRANTIES WITH
# REGARD TO THIS SOURCE CODE, INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE.
# IN NO EVENT SHALL NVIDIA BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL,
# OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
# OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
# OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE
# OR PERFORMANCE OF THIS SOURCE CODE.
#
# U.S. Government End Users.  This source code is a "commercial item" as
# that term is defined at 48 C.F.R. 2.101 (OCT 1995), consisting  of
# "commercial computer software" and "commercial computer software
# documentation" as such terms are used in 48 C.F.R. 12.212 (SEPT 1995)
# and is provided to the U.S. Government only as a commercial end item.
# Consistent with 48 C.F.R.12.212 and 48 C.F.R. 227.7202-1 through
# 227.7202-4 (JUNE 1995), all U.S. Government End Users acquire the
# source code with only those rights set forth herein.
#
################################################################################
#
# Makefile project only supported on Mac OS X and Linux Platforms)
#
################################################################################

# Compile for system with primary NVIDIA GPU or for Optimus
# (primary Intel GPU, secondary NVIDIA GPU), without bumblebee
OPTIMUS ?= 1

# Broadcast signal to the specified socket for output visualization
BROADCAST ?= 1

# Location of the CUDA Toolkit
CUDA_PATH ?= "/usr/local/cuda"

OSUPPER = $(shell uname -s 2>/dev/null | tr "[:lower:]" "[:upper:]")
OSLOWER = $(shell uname -s 2>/dev/null | tr "[:upper:]" "[:lower:]")

OS_SIZE = $(shell uname -m | sed -e "s/i.86/32/" -e "s/x86_64/64/" -e "s/armv7l/32/")
OS_ARCH = $(shell uname -m | sed -e "s/i386/i686/")

DARWIN = $(strip $(findstring DARWIN, $(OSUPPER)))
ifneq ($(DARWIN),)
	XCODE_GE_5 = $(shell expr `xcodebuild -version | grep -i xcode | awk '{print $$2}' | cut -d'.' -f1` \>= 5)
endif

# Take command line flags that override any of these settings
ifeq ($(i386),1)
	OS_SIZE = 32
	OS_ARCH = i686
endif
ifeq ($(x86_64),1)
	OS_SIZE = 64
	OS_ARCH = x86_64
endif
ifeq ($(ARMv7),1)
	OS_SIZE = 32
	OS_ARCH = armv7l
endif

# Common binaries
ifneq ($(DARWIN),)
ifeq ($(XCODE_GE_5),1)
  GCC ?= clang
else
  GCC ?= g++
endif
else
  GCC ?= g++
endif
NVCC := $(CUDA_PATH)/bin/nvcc -ccbin $(GCC)

# internal flags
NVCCFLAGS   := -m${OS_SIZE}
ifeq ($(OPTIMUS),1)
NVCCFLAGS   += -DOPTIMUS
endif
ifeq ($(BROADCAST),1)
NVCCFLAGS   += -DBROADCAST
endif
CCFLAGS     :=
LDFLAGS     :=

# Extra user flags
EXTRA_NVCCFLAGS   ?=
EXTRA_LDFLAGS     ?=
EXTRA_CCFLAGS     ?=

# OS-specific build flags
ifneq ($(DARWIN),)
  LDFLAGS += -rpath $(CUDA_PATH)/lib
  CCFLAGS += -arch $(OS_ARCH)
else
  ifeq ($(OS_ARCH),armv7l)
    ifeq ($(abi),gnueabi)
      CCFLAGS += -mfloat-abi=softfp
    else
      # default to gnueabihf
      override abi := gnueabihf
      LDFLAGS += --dynamic-linker=/lib/ld-linux-armhf.so.3
      CCFLAGS += -mfloat-abi=hard
    endif
  endif
endif

ifeq ($(ARMv7),1)
NVCCFLAGS += -target-cpu-arch ARM
ifneq ($(TARGET_FS),)
CCFLAGS += --sysroot=$(TARGET_FS)
LDFLAGS += --sysroot=$(TARGET_FS)
LDFLAGS += -rpath-link=$(TARGET_FS)/lib
LDFLAGS += -rpath-link=$(TARGET_FS)/usr/lib
LDFLAGS += -rpath-link=$(TARGET_FS)/usr/lib/arm-linux-$(abi)
endif
endif

# Debug build flags
ifeq ($(dbg),1)
      NVCCFLAGS += -g -O0
      TARGET := debug
else
      NVCCFLAGS += -g -O3 -Xcompiler -mtune=native
      TARGET := release
endif

ALL_CCFLAGS :=
ALL_CCFLAGS += $(NVCCFLAGS)
ALL_CCFLAGS += $(EXTRA_NVCCFLAGS)
ALL_CCFLAGS += $(addprefix -Xcompiler ,$(CCFLAGS))
ALL_CCFLAGS += $(addprefix -Xcompiler ,$(EXTRA_CCFLAGS))

ALL_LDFLAGS :=
ALL_LDFLAGS += $(ALL_CCFLAGS)
ALL_LDFLAGS += $(addprefix -Xlinker ,$(LDFLAGS))
ALL_LDFLAGS += $(addprefix -Xlinker ,$(EXTRA_LDFLAGS))

# Common includes and paths for CUDA
INCLUDES  := -I../../common/inc
LIBRARIES := -lpthread

################################################################################

EXEC   ?=

# Makefile include to help find GL Libraries
include ./findgllib.mk

# OpenGL specific libraries
ifneq ($(DARWIN),)
 # Mac OSX specific libraries and paths to include
 LIBRARIES += -L/System/Library/Frameworks/OpenGL.framework/Libraries
 LIBRARIES += -lGL -lGLU ../../common/lib/darwin/libGLEW.a
 ALL_LDFLAGS += -Xlinker -framework -Xlinker GLUT
else
 LIBRARIES += -L../../common/lib/$(OSLOWER)/$(OS_ARCH) $(GLLINK)
 LIBRARIES += -lGL -lGLU -lX11 -lXi -lXmu -lglut -lGLEW
endif

# CUDA code generation flags
GENCODE_SM20    := -gencode arch=compute_20,code=sm_20
GENCODE_SM30    := -gencode arch=compute_30,code=sm_30
GENCODE_SM32    := -gencode arch=compute_32,code=sm_32
GENCODE_SM35    := -gencode arch=compute_35,code=sm_35
GENCODE_SM50    := -gencode arch=compute_50,code=sm_50
GENCODE_SMXX    := -gencode arch=compute_50,code=compute_50
GENCODE_FLAGS   ?= $(GENCODE_SM10) $(GENCODE_SM20) $(GENCODE_SM30) $(GENCODE_SM32) $(GENCODE_SM35) $(GENCODE_SM50) $(GENCODE_SMXX)

LIBRARIES += -lcufft

################################################################################

# Target rules
all: build

build: fluidsGL fluidsGL_client

fluidsGL.o:fluidsGL.cpp UdpBroadcastServer.h
	$(EXEC) $(NVCC) $(INCLUDES) $(ALL_CCFLAGS) $(GENCODE_FLAGS) -o $@ -c $<

fluidsGL_kernels.o:fluidsGL_kernels.cu
	$(EXEC) $(NVCC) $(INCLUDES) $(ALL_CCFLAGS) $(GENCODE_FLAGS) -o $@ -c $<

UdpBroadcastServer.o: UdpBroadcastServer.cpp UdpBroadcastServer.h
	$(EXEC) $(NVCC) $(INCLUDES) $(ALL_CCFLAGS) $(GENCODE_FLAGS) -o $@ -c $<	

mkaddr.o: mkaddr.c
	$(EXEC) $(NVCC) $(INCLUDES) $(ALL_CCFLAGS) $(GENCODE_FLAGS) -o $@ -c $<	

fluidsGL: fluidsGL.o fluidsGL_kernels.o UdpBroadcastServer.o mkaddr.o
	$(EXEC) $(NVCC) $(ALL_LDFLAGS) $(GENCODE_FLAGS) -o $@ $+ $(LIBRARIES)
	$(EXEC) mkdir -p ../../bin/$(OS_ARCH)/$(OSLOWER)/$(TARGET)$(if $(abi),/$(abi))
	$(EXEC) cp $@ ../../bin/$(OS_ARCH)/$(OSLOWER)/$(TARGET)$(if $(abi),/$(abi))

fluidsGL_client.o: clients/linux/fluidsGL_client.cpp clients/linux/UdpBroadcastClient.h
	$(EXEC) $(NVCC) $(INCLUDES) $(ALL_CCFLAGS) $(GENCODE_FLAGS) -I. -o $@ -c $<

UdpBroadcastClient.o: clients/linux/UdpBroadcastClient.cpp clients/linux/UdpBroadcastClient.h UdpBroadcastServer.h
	$(EXEC) $(NVCC) $(INCLUDES) $(ALL_CCFLAGS) $(GENCODE_FLAGS) -I. -o $@ -c $<	

fluidsGL_client: fluidsGL_client.o UdpBroadcastClient.o mkaddr.o
	$(EXEC) $(NVCC) $(ALL_LDFLAGS) $(GENCODE_FLAGS) -o $@ $+ $(LIBRARIES)
	$(EXEC) mkdir -p ../../bin/$(OS_ARCH)/$(OSLOWER)/$(TARGET)$(if $(abi),/$(abi))
	$(EXEC) cp $@ ../../bin/$(OS_ARCH)/$(OSLOWER)/$(TARGET)$(if $(abi),/$(abi))

run: build
	$(EXEC) ./fluidsGL

clean:
	rm -f fluidsGL fluidsGL_client *.o
	rm -rf ../../bin/$(OS_ARCH)/$(OSLOWER)/$(TARGET)$(if $(abi),/$(abi))/fluidsGL
	rm -rf ../../bin/$(OS_ARCH)/$(OSLOWER)/$(TARGET)$(if $(abi),/$(abi))/fluidsGL_client

clobber: clean
