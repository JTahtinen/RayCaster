NAME=rc
CC=cl
ODIR=obj
IDIR=W:/jadel2/export/include
_DEPS=include
DEPS=$(patsubst %,$(IDIR)/%,$(_DEPS))
CFLAGS=/I$(IDIR) 
DEBUGDEFS=/DDEBUG
RELEASEDEFS=/DRELEASE /O2
_OBJ=*.obj
OBJ=$($(wildcard patsubst %,$(ODIR)/%,$(_OBJ)))


source:=$(wildcard src/*.cpp)
debugobjects:=$(patsubst src/%.cpp,obj/debug/%.obj,$(wildcard src/*.cpp))
releaseobjects:=$(patsubst src/%.cpp,obj/release/%.obj,$(wildcard src/*.cpp))

SCRDIR=src
.PHONY:
all: rc

withjadeldebug:
	(cd W:/jadel2 && make clean && make debug)
	copy W:\jadel2\export\lib\jadel.dll bin\jadel.dll
	$(CC) /c /Zi $(DEBUGDEFS) /EHsc /Iinclude /IW:/jadel2/export/include /std:c++latest ./src/*.cpp /Foobj/ 
	$(CC) /Zi $(debugobjects) W:/jadel2/export/lib/jadelmain.lib W:/jadel2/export/lib/jadel.lib /Febin/$(NAME)DEBUG.exe /link /SUBSYSTEM:WINDOWS

withjadel:
	(cd W:/jadel2 && make clean && make)
	copy W:\jadel2\export\lib\jadel.dll bin\jadel.dll
	$(CC) /c /Zi $(DEBUGDEFS) /EHsc /Iinclude $(CFLAGS) /std:c++latest ./src/*.cpp /Foobj/debug/
	$(CC) /Zi $(debugobjects) W:/jadel2/export/lib/jadelmain.lib W:/jadel2/export/lib/jadel.lib /Febin/$(NAME)DEBUG.exe /link /SUBSYSTEM:WINDOWS


rc: $(debugobjects)
	$(CC) /Zi $^ W:/jadel2/export/lib/jadelmain.lib W:/jadel2/export/lib/jadel.lib /Febin/$(NAME)DEBUG.exe /link /SUBSYSTEM:WINDOWS

release: $(releaseobjects)
	$(CC) /Zi $^ W:/jadel2/export/lib/jadelmain.lib W:/jadel2/export/lib/jadel.lib /Febin/$(NAME).exe /link /SUBSYSTEM:WINDOWS

rwithjadel:
	(cd W:/jadel2 && make clean && make)
	copy W:\jadel2\export\lib\jadel.dll bin\jadel.dll
	$(CC) /c /Zi $(RELEASEDEFS) /EHsc /Iinclude $(CFLAGS) /std:c++latest ./src/*.cpp /Foobj/release/
	$(CC) /Zi $(releaseobjects) W:/jadel2/export/lib/jadelmain.lib W:/jadel2/export/lib/jadel.lib /Febin/$(NAME).exe /link /SUBSYSTEM:WINDOWS

obj/debug/%.obj: src/%.cpp
	$(CC) /c /Zi $(DEBUGDEFS) /EHsc /Iinclude $(CFLAGS) /std:c++latest $^ /Foobj/debug/

obj/release/%.obj: src/%.cpp
	$(CC) /c /Zi $(RELEASEDEFS) /EHsc /Iinclude $(CFLAGS) /std:c++latest $^ /Foobj/release/
copyjadel:
	$(CC)  /c /Zi /EHsc /Iinclude /IW:/jadel2/export/include /std:c++latest ./src/*.cpp /Foobj/ 
	$(CC)  /Zi obj/*.obj W:/jadel2/export/lib/jadelmain.lib W:/jadel2/export/lib/jadel.lib /Febin/$(NAME).exe /link /SUBSYSTEM:WINDOWS
	copy W:\jadel2\export\lib\jadel.dll bin\jadel.dll

.PHONY:
clean:
	del obj\\debug\\*.obj obj\\release\\*.obj bin\\*.exe