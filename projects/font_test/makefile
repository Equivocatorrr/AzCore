APPNAME=AzCore_Font

AZCORE_BASE=../../base

BDIR=bin
IDIR=src
SDIR=src
ODIR=obj

CC=g++
WCC=i686-w64-mingw32-g++
WRC=i686-w64-mingw32-windres $(SDIR)/resources.rc -O coff
RCFLAGS=-I $(IDIR)
CFLAGS=-I$(IDIR) -I$(AZCORE_BASE)/src -Wall -std=c++17 -fsingle-precision-constant
LCFLAGS=-Wl,-z,origin -Wl,-rpath,'$$ORIGIN/lib'
WCFLAGS=-D_GLIBCXX_USE_NANOSLEEP -static-libgcc -static-libstdc++ -static -lpthread

WINDOWS_RESOURCES = $(AZCORE_BASE)/obj/Windows/resources.o

LIBS_L=-lpthread
LIBS_W=

_AZCORE_OBJ = memory.o log_stream.o math.o font.o
_AZCORE_OBJ_L = $(_AZCORE_OBJ)
_AZCORE_OBJ_W = $(_AZCORE_OBJ)
AZCORE_OBJ_L = $(patsubst %,$(AZCORE_BASE)/obj/Linux/Release/%,$(_AZCORE_OBJ_L))
AZCORE_OBJ_W = $(patsubst %,$(AZCORE_BASE)/obj/Windows/Release/%,$(_AZCORE_OBJ_W))
AZCORE_OBJ_LD = $(patsubst %,$(AZCORE_BASE)/obj/Linux/Debug/%,$(_AZCORE_OBJ_L))
AZCORE_OBJ_WD = $(patsubst %,$(AZCORE_BASE)/obj/Windows/Debug/%,$(_AZCORE_OBJ_W))

_OBJ = main.o
OBJ_L = $(patsubst %,$(ODIR)/Linux/Release/%,$(_OBJ))
OBJ_W = $(patsubst %,$(ODIR)/Windows/Release/%,$(_OBJ))
OBJ_LD = $(patsubst %,$(ODIR)/Linux/Debug/%,$(_OBJ))
OBJ_WD = $(patsubst %,$(ODIR)/Windows/Debug/%,$(_OBJ))

$(ODIR)/Linux/Debug/%.o: $(SDIR)/%.cpp $(DEPS) azcoredl
	@mkdir -p $(@D)
	$(CC) -c -o $@ $< -g -rdynamic $(CFLAGS)

$(ODIR)/Windows/Debug/%.o: $(SDIR)/%.cpp $(DEPS) azcoredw
	@mkdir -p $(@D)
	$(WCC) -c -o $@ $< -g $(CFLAGS)

$(ODIR)/Linux/Release/%.o: $(SDIR)/%.cpp $(DEPS) azcorel
	@mkdir -p $(@D)
	$(CC) -c -o $@ $< -g -O2 $(CFLAGS) -DNDEBUG

$(ODIR)/Windows/Release/%.o: $(SDIR)/%.cpp $(DEPS) azcorew
	@mkdir -p $(@D)
	$(WCC) -c -o $@ $< -O2 $(CFLAGS) -DNDEBUG

.PHONY: azcorel azcorew azcoredl azcoredw clean _debugl _debugw debugl debugw debug _releasel _releasew releasel releasew release all runl runw rundl rundlv rundw testl testdl testdlv

azcorel:
	$(MAKE) -C $(AZCORE_BASE) releasel

azcorew:
	$(MAKE) -C $(AZCORE_BASE) releasew

azcoredl:
	$(MAKE) -C $(AZCORE_BASE) debugl

azcoredw:
	$(MAKE) -C $(AZCORE_BASE) debugw

debugl: $(OBJ_LD) azcoredl
	@mkdir -p  $(BDIR)/Linux/Debug
	g++ -o $(BDIR)/Linux/Debug/$(APPNAME) $(OBJ_LD) $(AZCORE_OBJ_LD) -g -rdynamic $(CFLAGS) $(LCFLAGS) $(LIBS_L)

debugw: $(OBJ_WD) $(WINDOWS_RESOURCES) azcoredw
	@mkdir -p  $(BDIR)/Windows/Debug
	i686-w64-mingw32-g++ -o $(BDIR)/Windows/Debug/$(APPNAME).exe $(OBJ_WD) $(WINDOWS_RESOURCES) $(AZCORE_OBJ_WD) -g $(CFLAGS) $(WCFLAGS) $(LIBS_W)

debug: debugl debugw

releasel: $(OBJ_L) azcorel
	@mkdir -p  $(BDIR)/Linux/Release
	g++ -o $(BDIR)/Linux/Release/$(APPNAME) $(OBJ_L) $(AZCORE_OBJ_L) $(CFLAGS) $(LCFLAGS) $(LIBS_L)

releasew: $(OBJ_W) $(WINDOWS_RESOURCES) azcorew
	@mkdir -p $(BDIR)/Windows/Release
	i686-w64-mingw32-g++ -o $(BDIR)/Windows/Release/$(APPNAME).exe $(OBJ_W) $(WINDOWS_RESOURCES) $(AZCORE_OBJ_W) $(CFLAGS) $(WCFLAGS) -mwindows $(LIBS_W)

release: releasel releasew

clean:
	rm -r $(ODIR) $(BDIR)

runl:
	./bin/Linux/Release/$(APPNAME)

runw:
	./bin/Windows/Release/$(APPNAME).exe

rundl:
	./bin/Linux/Debug/$(APPNAME)

rundw:
	./bin/Windows/Debug/$(APPNAME).exe

# make INPUT_FILE="path/to/file" testl
testl: releasel
	./bin/Linux/Release/$(APPNAME) $(INPUT_FILE)

testdl: debugl
	./bin/Linux/Debug/$(APPNAME) $(INPUT_FILE)
