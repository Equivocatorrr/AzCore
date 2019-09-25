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

LIBS_L=-lvulkan -lxcb -levdev -lpthread -lxkbcommon -lxkbcommon-x11 -lxcb-xkb
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

$(ODIR)/Linux/Debug/%.o: $(SDIR)/%.cpp $(DEPS) $(AZCORE_BASE)/.touchdl
	@mkdir -p $(@D)
	$(CC) -c -o $@ $< -g -rdynamic $(CFLAGS)

$(ODIR)/Windows/Debug/%.o: $(SDIR)/%.cpp $(DEPS) $(AZCORE_BASE)/.touchdw
	@mkdir -p $(@D)
	$(WCC) -c -o $@ $< -g $(CFLAGS)

$(ODIR)/Linux/Release/%.o: $(SDIR)/%.cpp $(DEPS) $(AZCORE_BASE)/.touchl
	@mkdir -p $(@D)
	$(CC) -c -o $@ $< -g -O2 $(CFLAGS) -DNDEBUG

$(ODIR)/Windows/Release/%.o: $(SDIR)/%.cpp $(DEPS) $(AZCORE_BASE)/.touchw
	@mkdir -p $(@D)
	$(WCC) -c -o $@ $< -O2 $(CFLAGS) -DNDEBUG

.PHONY: clean _debugl _debugw debugl debugw debug _releasel _releasew releasel releasew release all shaders runl runw rundl rundlv rundw testl testdl testdlv

$(AZCORE_BASE)/.touchl: $(AZCORE_BASE)/src $(AZCORE_BASE)/makefile
	$(MAKE) -C $(AZCORE_BASE) releasel

$(AZCORE_BASE)/.touchw: $(AZCORE_BASE)/src $(AZCORE_BASE)/makefile
	$(MAKE) -C $(AZCORE_BASE) releasew

$(AZCORE_BASE)/.touchdl: $(AZCORE_BASE)/src $(AZCORE_BASE)/makefile
	$(MAKE) -C $(AZCORE_BASE) debugl

$(AZCORE_BASE)/.touchdw: $(AZCORE_BASE)/src $(AZCORE_BASE)/makefile
	$(MAKE) -C $(AZCORE_BASE) debugw

_debugl: $(OBJ_LD) $(AZCORE_BASE)/.touchdl
	@mkdir -p  $(BDIR)/Linux/Debug
	$(CC) -o $(BDIR)/Linux/Debug/$(APPNAME) $(OBJ_LD) $(AZCORE_OBJ_LD) -g -rdynamic $(CFLAGS) $(LCFLAGS) $(LIBS_L)

_debugw: $(OBJ_WD) $(WINDOWS_RESOURCES) $(AZCORE_BASE)/.touchdw
	@mkdir -p $(BDIR)/Windows/Debug
	$(WCC) -o $(BDIR)/Windows/Debug/$(APPNAME).exe $(OBJ_WD) $(WINDOWS_RESOURCES) $(AZCORE_OBJ_WD) -g $(CFLAGS) $(WCFLAGS) $(LIBS_W)

debugl: _debugl shaders

debugw: _debugw shaders

debug: _debugl _debugw shaders

_releasel: $(OBJ_L) $(AZCORE_BASE)/.touchl
	@mkdir -p $(BDIR)/Linux/Release
	$(CC) -o $(BDIR)/Linux/Release/$(APPNAME) $(OBJ_L) $(AZCORE_OBJ_L) $(CFLAGS) $(LCFLAGS) $(LIBS_L)

_releasew: $(OBJ_W) $(WINDOWS_RESOURCES) $(AZCORE_BASE)/.touchw
	@mkdir -p $(BDIR)/Windows/Release
	$(WCC) -o $(BDIR)/Windows/Release/$(APPNAME).exe $(OBJ_W) $(WINDOWS_RESOURCES) $(AZCORE_OBJ_W) $(CFLAGS) $(WCFLAGS) -mwindows $(LIBS_W)

releasel: $(AZCORE_BASE)/.touchl _releasel shaders

releasew: $(AZCORE_BASE)/.touchw _releasew shaders

release: _releasel _releasew shaders

packl: _debugl _releasel shaders
	scripts/PackLinux.sh $(APPNAME)

packw: _debugw _releasew shaders
	scripts/PackWindows.sh $(APPNAME)

clean:
	rm -r $(ODIR) $(BDIR) Exports

data/shaders/%.vert.spv: src/shaders/%.vert
	@mkdir -p $(@D)
	./scripts/CompileShaders.sh $^ vert $@

data/shaders/%.frag.spv: src/shaders/%.frag
	@mkdir -p $(@D)
	./scripts/CompileShaders.sh $^ frag $@

shaders: $(SHADERS)

runl:
	./bin/Linux/Release/$(APPNAME)

runw:
	./bin/Windows/Release/$(APPNAME).exe

rundl:
	./bin/Linux/Debug/$(APPNAME)

rundlv:
	./bin/Linux/Debug/$(APPNAME) --enable-layers --core-validation

rundw:
	./bin/Windows/Debug/$(APPNAME).exe

testl: releasel
	./bin/Linux/Release/$(APPNAME)

testdl: debugl
	./bin/Linux/Debug/$(APPNAME)

testdlv: debugl
	./bin/Linux/Debug/$(APPNAME) --enable-layers --core-validation

all: debug release
