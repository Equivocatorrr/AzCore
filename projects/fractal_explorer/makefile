APPNAME=AzCore_Fractal_Explorer

AZCORE_BASE=../../base

BDIR=bin
IDIR=src
SDIR=src
ODIR=obj

CC=g++
WCC=x86_64-w64-mingw32-g++
WRC=x86_64-w64-mingw32-windres $(SDIR)/resources.rc -O coff
RCFLAGS=-I $(IDIR)
CFLAGS=-I$(IDIR) -I$(AZCORE_BASE)/src -Wall -std=c++17 -mavx2 -DAZCORE_IO_NO_XLIB
LCFLAGS=$(CFLAGS) #-Wl,-z,origin -Wl,-rpath,'$$ORIGIN/lib'
WCFLAGS=$(CFLAGS) -I$(WIN_VULKAN_SDK)/Include/ -D_GLIBCXX_USE_NANOSLEEP -static -static-libgcc -static-libstdc++ -Llibwinpthread-1.dll

# This is optional, but recommended.
WINDOWS_RESOURCES = $(ODIR)/Windows/resources.o
WINDOWS_MANIFEST = $(AZCORE_BASE)/manifest.xml
WINDOWS_ICON = $(AZCORE_BASE)/icon.ico
$(ODIR)/Windows/resources.o: $(IDIR)/resources.h $(SDIR)/resources.rc $(WINDOWS_MANIFEST) makefile $(WINDOWS_ICON)
	$(WRC) -o $@ $(RCFLAGS)
# You can use the base one from AzCore by uncommenting this
#WINDOWS_RESOURCES = $(AZCORE_BASE)/obj/Windows/resources.o

LIBS_L=-lxcb -lxcb-shm -lxcb-image -lpthread -lxkbcommon -lxkbcommon-x11 -lxcb-xkb
LIBS_W=-lgdi32 -ldinput8 -ldxguid

_DEPS =
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS)) makefile

_AZCORE_OBJ = io.o memory.o keycodes.o IO/Log.o SoftwareRendering.o math.o Image.o
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
