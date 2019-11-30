APPNAME=AzCore_Persistence

AZCORE_BASE=../../base

BDIR=bin
IDIR=src
SDIR=src
ODIR=obj

CC=g++
WCC=x86_64-w64-mingw32-g++
WRC=x86_64-w64-mingw32-windres $(SDIR)/resources.rc -O coff
RCFLAGS=-I $(IDIR)
CFLAGS=-I$(IDIR) -I$(AZCORE_BASE)/src -Wall -std=c++17 -fsingle-precision-constant
LCFLAGS=$(CFLAGS) #-Wl,-z,origin -Wl,-rpath,'$$ORIGIN/lib'
WCFLAGS=$(CFLAGS) -I$(WIN_VULKAN_SDK)/Include/ -D_GLIBCXX_USE_NANOSLEEP -static -static-libgcc -static-libstdc++ -Llibwinpthread-1.dll

WINDOWS_RESOURCES = $(AZCORE_BASE)/obj/Windows/resources.o

LIBS_L=-lpthread
LIBS_W=

_AZCORE_OBJ = memory.o IO/LogStream.o math.o Memory/BigInt.o
AZCORE_OBJ_L = $(patsubst %,$(AZCORE_BASE)/obj/Linux/Release/%,$(_AZCORE_OBJ))
AZCORE_OBJ_W = $(patsubst %,$(AZCORE_BASE)/obj/Windows/Release/%,$(_AZCORE_OBJ))
AZCORE_OBJ_LD = $(patsubst %,$(AZCORE_BASE)/obj/Linux/Debug/%,$(_AZCORE_OBJ))
AZCORE_OBJ_WD = $(patsubst %,$(AZCORE_BASE)/obj/Windows/Debug/%,$(_AZCORE_OBJ))

_OBJ = main.o
OBJ_L = $(patsubst %,$(ODIR)/Linux/Release/%,$(_OBJ))
OBJ_W = $(patsubst %,$(ODIR)/Windows/Release/%,$(_OBJ))
OBJ_LD = $(patsubst %,$(ODIR)/Linux/Debug/%,$(_OBJ))
OBJ_WD = $(patsubst %,$(ODIR)/Windows/Debug/%,$(_OBJ))

include ../base_project_makefile
