IDIR=src
SDIR=src
ODIR=obj

CC=g++
WCC=i686-w64-mingw32-g++
WRC=i686-w64-mingw32-windres $(SDIR)/resources.rc -O coff
RCFLAGS=-I $(IDIR)
CFLAGS=-I$(IDIR) -Wall -std=c++17 -fsingle-precision-constant -DIO_FOR_VULKAN
LCFLAGS=-Wl,-z,origin -Wl,-rpath,'$$ORIGIN/lib'
WCFLAGS=-D_GLIBCXX_USE_NANOSLEEP -static-libgcc -static-libstdc++ -static -lpthread

WIN_VULKAN_SDK=/home/singularity/.wine/drive_c/VulkanSDK/1.1.82.0

# These are all the libraries you need to link to in order to use the entire library
#LIBS_L=-lvulkan -lxcb -levdev -lpthread -lxkbcommon -lxkbcommon-x11 -lxcb-xkb
#LIBS_L_GLX= $(LIBS_L) -lX11 -lX11-xcb
#LIBS_W=-L"$(WIN_VULKAN_SDK)/Lib32/" -lvulkan-1 -lgdi32

_DEPS = keycodes.hpp io.hpp math.hpp memory.hpp log_stream.hpp vk.hpp bigint.hpp
DEPS = $(patsubst %,$(IDIR)/AzCore/%,$(_DEPS)) makefile

_OBJ = io.o memory.o keycodes.o log_stream.o vk.o math.o bigint.o
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

$(ODIR)/Windows/resources.o: $(IDIR)/resources.h $(SDIR)/resources.rc manifest.xml icon.ico
	@mkdir -p $(@D)
	$(WRC) -o $@ $(RCFLAGS)

.PHONY: debugl debugw debug releasel releasew release all clean

debugl: $(OBJ_LD)

debugw: $(OBJ_WD) $(ODIR)/Windows/resources.o

debug: debugl debugw

releasel: $(OBJ_L)

releasew: $(OBJ_W) $(ODIR)/Windows/resources.o

release: releasel releasew

all: debug release

clean:
	rm -r $(ODIR)
