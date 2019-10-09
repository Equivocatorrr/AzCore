IDIR=src
SDIR=src
ODIR=obj

CC=g++
WCC=x86_64-w64-mingw32-g++
WRC=x86_64-w64-mingw32-windres $(SDIR)/resources.rc -O coff
RCFLAGS=-I $(IDIR)
CFLAGS=-I$(IDIR) -Wall -std=c++17 -fsingle-precision-constant -DIO_FOR_VULKAN
LCFLAGS=-Wl,-z,origin -Wl,-rpath,'$$ORIGIN/lib'
WCFLAGS=-D_GLIBCXX_USE_NANOSLEEP -static-libgcc -static-libstdc++ -static -lpthread

WIN_VULKAN_SDK=/home/singularity/.wine/drive_c/VulkanSDK/1.1.82.0

# These are all the library dependencies you need to link to in order to use the entire library
#		Linux:
#	memory.o: pthread
#	io_linux.o: xcb, xkbcommon, xkbcommon-x11, xcb-xkb
#		- GLX: X11, X11-xcb
#	vk.o: vulkan
#		Windows:
#	io_win32.o: gdi32 dinput8 dxguid
#	vk.o: vulkan-1
#LIBS_L=-lvulkan -lxcb -levdev -lpthread -lxkbcommon -lxkbcommon-x11 -lxcb-xkb
#LIBS_L_GLX= $(LIBS_L) -lX11 -lX11-xcb
#LIBS_W=-L"$(WIN_VULKAN_SDK)/Lib32/" -lvulkan-1 -lgdi32

_DEPS = keycodes.hpp io.hpp math.hpp memory.hpp log_stream.hpp vk.hpp bigint.hpp font.hpp font_tables.hpp font_tables.cpp
DEPS = $(patsubst %,$(IDIR)/AzCore/%,$(_DEPS)) makefile

_OBJ = io.o memory.o keycodes.o log_stream.o vk.o math.o bigint.o font.o
_OBJ_L = $(_OBJ) io_linux.o
_OBJ_W = $(_OBJ) io_win32.o
OBJ_L = $(patsubst %,$(ODIR)/Linux/Release/%,$(_OBJ_L))
OBJ_W = $(patsubst %,$(ODIR)/Windows/Release/%,$(_OBJ_W))
OBJ_LD = $(patsubst %,$(ODIR)/Linux/Debug/%,$(_OBJ_L))
OBJ_WD = $(patsubst %,$(ODIR)/Windows/Debug/%,$(_OBJ_W))

$(ODIR)/Linux/Debug/%.o: $(SDIR)/AzCore/%.cpp $(DEPS)
	@mkdir -p $(@D)
	$(CC) -c -o $@ $< -g -rdynamic $(CFLAGS)

$(ODIR)/Windows/Debug/%.o: $(SDIR)/AzCore/%.cpp $(DEPS)
	@mkdir -p $(@D)
	$(WCC) -c -o $@ $< -g $(CFLAGS)

$(ODIR)/Linux/Release/%.o: $(SDIR)/AzCore/%.cpp $(DEPS)
	@mkdir -p $(@D)
	$(CC) -c -o $@ $< -g -O2 $(CFLAGS) -DNDEBUG

$(ODIR)/Windows/Release/%.o: $(SDIR)/AzCore/%.cpp $(DEPS)
	@mkdir -p $(@D)
	$(WCC) -c -o $@ $< -O2 $(CFLAGS) -DNDEBUG

$(ODIR)/Windows/resources.o: $(IDIR)/resources.h $(SDIR)/resources.rc manifest.xml icon.ico $(DEPS)
	@mkdir -p $(@D)
	$(WRC) -o $@ $(RCFLAGS)

.PHONY: debugl debugw debug releasel releasew release all clean staticlibl staticlibdl staticlibw staticlibdw staticlibs

debugl: $(OBJ_LD)
	touch .touchdl

debugw: $(OBJ_WD) $(ODIR)/Windows/resources.o
	touch .touchdw

debug: debugl debugw

releasel: $(OBJ_L)
	touch .touchl

releasew: $(OBJ_W) $(ODIR)/Windows/resources.o
	touch .touchw

release: releasel releasew

staticlibl: $(OBJ_L)
	ar rcs lib/libAzCoreLinux.a $(OBJ_L)

staticlibdl: $(OBJ_LD)
	ar rcs lib/libAzCoreLinuxDebug.a $(OBJ_LD)

staticlibw: $(OBJ_W)
	ar rcs lib/libAzCoreWin32.a $(OBJ_W)

staticlibdw: $(OBJ_WD)
	ar rcs lib/libAzCoreWin32Debug.a $(OBJ_WD)

staticlibs: staticlibl staticlibdl staticlibw staticlibdw

all: debug release

clean:
	rm -r $(ODIR)
