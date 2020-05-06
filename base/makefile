IDIR=src
SDIR=src
ODIR=obj

WIN_VULKAN_SDK=/home/singularity/.wine/drive_c/VulkanSDK/1.1.121.2

CC=g++
WCC=x86_64-w64-mingw32-g++
WRC=x86_64-w64-mingw32-windres $(SDIR)/resources.rc -O coff
RCFLAGS=-I $(IDIR)
CFLAGS=-I$(IDIR) -Wall -std=c++17 -Wdouble-promotion -Wfloat-conversion -DAZCORE_IO_FOR_VULKAN
LCFLAGS=$(CFLAGS) #-Wl,-z,origin -Wl,-rpath,'$$ORIGIN/lib'
WCFLAGS=$(CFLAGS) -D_GLIBCXX_USE_NANOSLEEP -static-libgcc -static-libstdc++ -I$(WIN_VULKAN_SDK)/Include/

# These are all the library dependencies you need to link to in order to use the entire library
#		Linux:
#	memory.o: pthread
#	io_linux.o: xcb, xkbcommon, xkbcommon-x11, xcb-xkb
#		- GLX: X11, X11-xcb
#	vk.o: vulkan
#		Windows:
#	io_win32.o: gdi32 dinput8 dxguid
#	vk.o: vulkan-1
#LIBS_L=-lvulkan -lxcb -lpthread -lxkbcommon -lxkbcommon-x11 -lxcb-xkb
#LIBS_L_GLX= $(LIBS_L) -lX11 -lX11-xcb
#LIBS_W=-L"$(WIN_VULKAN_SDK)/Lib32/" -lvulkan-1 -lgdi32

_DEPS_BASE = keycodes.hpp io.hpp math.hpp memory.hpp vk.hpp font.hpp Font/FontTables.hpp Font/FontTables.cpp Font/font_cff_std_data.c Font/CFF.hpp Font/CFF.cpp Time.hpp Thread.hpp ThreadLinux.hpp ThreadWin32.hpp
DEPS_BASE = $(patsubst %,$(IDIR)/AzCore/%,$(_DEPS_BASE)) makefile
_DEPS_IO = ButtonState.hpp ButtonState.cpp Gamepad.hpp Gamepad.cpp Input.hpp Input.cpp LogStream.hpp LogStream.cpp RawInput.hpp RawInput.cpp Window.hpp
_DEPS_IO_PLATFORM = RawInput.cpp Window.cpp
DEPS_IO_L = $(patsubst %,$(IDIR)/AzCore/IO/%,$(_DEPS_IO)) $(patsubst %,$(IDIR)/AzCore/IO/Linux/%,$(_DEPS_IO_PLATFORM))
DEPS_IO_W = $(patsubst %,$(IDIR)/AzCore/IO/%,$(_DEPS_IO)) $(patsubst %,$(IDIR)/AzCore/IO/Win32/%,$(_DEPS_IO_PLATFORM))

_DEPS_MATH = Angle.hpp Angle.cpp basic.hpp complex_t.hpp Equations.hpp mat2_t.hpp mat3_t.hpp mat4_t.hpp mat5_t.hpp quat_t.hpp RandomNumberGenerator.hpp RandomNumberGenerator.cpp vec2_t.hpp vec3_t.hpp vec4_t.hpp vec5_t.hpp
DEPS_MATH = $(patsubst %,$(IDIR)/AzCore/Math/%,$(_DEPS_MATH))

_DEPS_MEMORY = Array.hpp ArrayList.hpp BigInt.hpp BucketArray.hpp Endian.hpp Endian.cpp List.hpp Ptr.hpp Range.hpp String.hpp String.cpp
DEPS_MEMORY = $(patsubst %,$(IDIR)/AzCore/Memory/%,$(_DEPS_MEMORY))

DEPS_L = $(DEPS_BASE) $(DEPS_IO_L) $(DEPS_MATH) $(DEPS_MEMORY)
DEPS_W = $(DEPS_BASE) $(DEPS_IO_W) $(DEPS_MATH) $(DEPS_MEMORY)

_OBJ = io.o memory.o keycodes.o IO/LogStream.o vk.o math.o Memory/BigInt.o font.o
OBJ_L = $(patsubst %,$(ODIR)/Linux/Release/%,$(_OBJ))
OBJ_W = $(patsubst %,$(ODIR)/Windows/Release/%,$(_OBJ))
OBJ_LD = $(patsubst %,$(ODIR)/Linux/Debug/%,$(_OBJ))
OBJ_WD = $(patsubst %,$(ODIR)/Windows/Debug/%,$(_OBJ))

$(ODIR)/Linux/Debug/%.o: $(SDIR)/AzCore/%.cpp $(DEPS_L)
	@mkdir -p $(@D)
	$(CC) -c -o $@ $< -g -rdynamic $(LCFLAGS)

$(ODIR)/Windows/Debug/%.o: $(SDIR)/AzCore/%.cpp $(DEPS_W)
	@mkdir -p $(@D)
	$(WCC) -c -o $@ $< -g $(WCFLAGS)

$(ODIR)/Linux/Release/%.o: $(SDIR)/AzCore/%.cpp $(DEPS_L)
	@mkdir -p $(@D)
	$(CC) -c -o $@ $< -g -O2 $(LCFLAGS) -DNDEBUG

$(ODIR)/Windows/Release/%.o: $(SDIR)/AzCore/%.cpp $(DEPS_W)
	@mkdir -p $(@D)
	$(WCC) -c -o $@ $< -O2 $(WCFLAGS) -DNDEBUG

$(ODIR)/Windows/resources.o: $(IDIR)/resources.h $(SDIR)/resources.rc manifest.xml icon.ico $(DEPS_W)
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
