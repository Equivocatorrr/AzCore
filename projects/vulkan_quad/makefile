APPNAME=AzCore_Quad

AZCORE_BASE=../../base

BDIR=bin
IDIR=src
SDIR=src
ODIR=obj

CC=g++
WCC=x86_64-w64-mingw32-g++
WRC=x86_64-w64-mingw32-windres $(SDIR)/resources.rc -O coff
RCFLAGS=-I $(IDIR)
CFLAGS=-I$(IDIR) -I$(AZCORE_BASE)/src -Wall -std=c++17 -fsingle-precision-constant -DIO_FOR_VULKAN
LCFLAGS=-Wl,-z,origin -Wl,-rpath,'$$ORIGIN/lib'
WCFLAGS=-D_GLIBCXX_USE_NANOSLEEP -static-libgcc -static-libstdc++ -static -lpthread

# This is optional, but recommended.
WINDOWS_RESOURCES = $(ODIR)/Windows/resources.o
WINDOWS_MANIFEST = $(AZCORE_BASE)/manifest.xml
WINDOWS_ICON = $(AZCORE_BASE)/icon.ico
$(ODIR)/Windows/resources.o: $(IDIR)/resources.h $(SDIR)/resources.rc $(WINDOWS_MANIFEST) makefile $(WINDOWS_ICON)
	$(WRC) -o $@ $(RCFLAGS)
# You can use the base one from AzCore by uncommenting this
#WINDOWS_RESOURCES = $(AZCORE_BASE)/obj/Windows/resources.o

WIN_VULKAN_SDK=/home/singularity/.wine/drive_c/VulkanSDK/1.1.82.0

LIBS_L=-lvulkan -lxcb -lpthread -lxkbcommon -lxkbcommon-x11 -lxcb-xkb
LIBS_L_GLX= $(LIBS_L) -lX11 -lX11-xcb
LIBS_W=-L"$(WIN_VULKAN_SDK)/Lib/" -lvulkan-1 -lgdi32 -ldinput8 -ldxguid

_DEPS =
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS)) makefile

_AZCORE_OBJ = io.o memory.o keycodes.o log_stream.o vk.o math.o
_AZCORE_OBJ_L = $(_AZCORE_OBJ) io_linux.o
_AZCORE_OBJ_W = $(_AZCORE_OBJ) io_win32.o
AZCORE_OBJ_L = $(patsubst %,$(AZCORE_BASE)/obj/Linux/Release/%,$(_AZCORE_OBJ_L))
AZCORE_OBJ_W = $(patsubst %,$(AZCORE_BASE)/obj/Windows/Release/%,$(_AZCORE_OBJ_W))
AZCORE_OBJ_LD = $(patsubst %,$(AZCORE_BASE)/obj/Linux/Debug/%,$(_AZCORE_OBJ_L))
AZCORE_OBJ_WD = $(patsubst %,$(AZCORE_BASE)/obj/Windows/Debug/%,$(_AZCORE_OBJ_W))

_OBJ = main.o
OBJ_L = $(patsubst %,$(ODIR)/Linux/Release/%,$(_OBJ))
OBJ_W = $(patsubst %,$(ODIR)/Windows/Release/%,$(_OBJ))
OBJ_LD = $(patsubst %,$(ODIR)/Linux/Debug/%,$(_OBJ))
OBJ_WD = $(patsubst %,$(ODIR)/Windows/Debug/%,$(_OBJ))

_SHADERS = test.vert.spv test.frag.spv
SHADERS = $(patsubst %,data/shaders/%,$(_SHADERS))

include ../base_project_makefile
