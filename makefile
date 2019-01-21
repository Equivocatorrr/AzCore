APPNAME=Test

IDIR=src
CC=g++
WCC=i686-w64-mingw32-g++
WRC=i686-w64-mingw32-windres $(SDIR)/resources.rc -O coff
RCFLAGS=-I $(IDIR)
CFLAGS=-I$(IDIR) -Wall -std=c++17 -fsingle-precision-constant
LCFLAGS=-Wl,-z,origin -Wl,-rpath,'$$ORIGIN/lib'
WCFLAGS=-D_GLIBCXX_USE_NANOSLEEP -static-libgcc -static-libstdc++ -static -lpthread

BDIR=bin
SDIR=src
ODIR=obj

HOME=/home/singularity

LIBS_L=-lvulkan -lxcb -lxcb-errors -lX11 -lX11-xcb -levdev -lpthread -lxkbcommon -lxkbcommon-x11 -lxcb-xkb
LIBS_W=-L"$(HOME)/.wine/drive_c/VulkanSDK/1.1.82.0/Lib32/" -lvulkan-1 -lgdi32

_DEPS = ../makefile keycode/keycode.hpp keycode/keytable.hpp keycode/keytable_common.cpp keycode/keytable_evdev.cpp keycode/keytable_win.cpp keycode/keytable_mac.cpp io.hpp math.hpp memory.hpp
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = main.o io.o memory.o
_OBJ_L = $(_OBJ) io_linux.o
_OBJ_W = $(_OBJ) io_windows.o
OBJ_L = $(patsubst %,$(ODIR)/Linux/Release/%,$(_OBJ_L))
OBJ_W = $(patsubst %,$(ODIR)/Windows/Release/%,$(_OBJ_W))
OBJ_LD = $(patsubst %,$(ODIR)/Linux/Debug/%,$(_OBJ_L))
OBJ_WD = $(patsubst %,$(ODIR)/Windows/Debug/%,$(_OBJ_W))

_SHADERS =
SHADERS = $(patsubst %,data/shaders/%,$(_SHADERS))


$(ODIR)/Linux/Debug/%.o: $(SDIR)/%.cpp $(DEPS)
	$(CC) -c -o $@ $< -g -rdynamic $(CFLAGS)

$(ODIR)/Windows/Debug/%.o: $(SDIR)/%.cpp $(DEPS)
	$(WCC) -c -o $@ $< -g $(CFLAGS)

$(ODIR)/Linux/Release/%.o: $(SDIR)/%.cpp $(DEPS)
	$(CC) -c -o $@ $< -g -O2 $(CFLAGS) -DNDEBUG

$(ODIR)/Windows/Release/%.o: $(SDIR)/%.cpp $(DEPS)
	$(WCC) -c -o $@ $< -O2 $(CFLAGS) -DNDEBUG

$(ODIR)/Windows/resources.o: $(IDIR)/resources.h $(SDIR)/resources.rc manifest.xml
	$(WRC) -o $@ $(RCFLAGS)

_debugl: $(OBJ_LD)
	g++ -o $(BDIR)/Linux/Debug/$(APPNAME) $^ -g -rdynamic $(CFLAGS) $(LCFLAGS) $(LIBS_L)

_debugw: $(OBJ_WD) $(ODIR)/Windows/resources.o
	i686-w64-mingw32-g++ -o $(BDIR)/Windows/Debug/$(APPNAME).exe $^ -g $(CFLAGS) $(WCFLAGS) $(LIBS_W)

debugl: _debugl shaders

debugw: _debugw shaders

debug: _debugl _debugw shaders

_releasel: $(OBJ_L)
	g++ -o $(BDIR)/Linux/Release/$(APPNAME) $^ $(CFLAGS) $(LCFLAGS) $(LIBS_L)

_releasew: $(OBJ_W) $(ODIR)/Windows/resources.o
	i686-w64-mingw32-g++ -o $(BDIR)/Windows/Release/$(APPNAME).exe $^ $(CFLAGS) $(WCFLAGS) $(LIBS_W)

releasel: _releasel shaders

releasew: _releasew shaders

release: _releasel _releasew shaders

packl: _debugl _releasel shaders
	scripts/PackLinux.sh $(APPNAME)

packw: _debugw _releasew shaders
	scripts/PackWindows.sh $(APPNAME)


.PHONY: clean shaders runl runw rundl rundw testl testdl

clean:
	rm -f $(ODIR)/Linux/Debug/*.o $(ODIR)/Windows/Debug/*.o $(ODIR)/Linux/Release/*.o $(ODIR)/Windows/Release/*.o *~ core $(INCDIR)/*~

data/shaders/%.vert.spv: src/shaders/%.vert
	./scripts/CompileShaders.sh $^ vert $@

data/shaders/%.frag.spv: src/shaders/%.frag
	./scripts/CompileShaders.sh $^ frag $@

shaders: $(SHADERS)

runl:
	./bin/Linux/Release/$(APPNAME)

primusrunl:
	primusrun bin/Linux/Release/$(APPNAME)

runw:
	./bin/Windows/Release/$(APPNAME).exe

rundl:
	./bin/Linux/Debug/$(APPNAME)

primusrundl:
	primusrun bin/Linux/Debug/$(APPNAME)

rundw:
	./bin/Windows/Debug/$(APPNAME).exe

testl: releasel
	./bin/Linux/Release/$(APPNAME)

testdl: debugl
	./bin/Linux/Debug/$(APPNAME)
