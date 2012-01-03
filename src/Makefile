# Makefile for LuaFAR

include ../config.mak

CC= gcc
CFLAGS= -O2 -W -Wall -I$(INC_FAR) -I$(INC_LUA) -DBUILD_DLL -DWINVER=0x500 \
        -DLUADLL=\"$(LUADLLNAME).dll\" $(ARCH) $(MYCFLAGS)
LDFLAGS= -lrpcrt4 -shared -s $(ARCH) $(MYLDFLAGS)

ifeq ($(ARCH),-m64)
  RESTARGET= -F pe-x86-64
else ifeq ($(ARCH),-m32)
  RESTARGET= -F pe-i386
endif

vpath %.dll $(PATH)

%.o : %.rc
	windres $< -o $@ $(RESTARGET)

OBJ = \
  bit64.o          \
  exported.o       \
  flags.o          \
  keysandcolors.o  \
  lflua.o          \
  lregex.o         \
  luafar.o         \
  reg.o            \
  service.o        \
  slnunico.o       \
  uliolib.o        \
  uloadlib.o       \
  ustring.o        \
  util.o

$(LUAFARDLL): $(OBJ) $(LUADLLNAME).dll
	$(CC) -o $@ $^ $(LDFLAGS)

flags.c: $(INC_FAR)\plugin.hpp makeflags.lua
	$(LUAEXE) makeflags.lua $< > $@

keysandcolors.c: $(INC_FAR)/farcolor.hpp makefarkeys.lua
	$(LUAEXE) makefarkeys.lua $(INC_FAR) $@

clean:
	del *.o flags.c keysandcolors.c $(LUAFARDLL)

# Dependencies
*.o        : $(INC_FAR)\plugin.hpp ustring.h
reg.o      : reg.h
service.o  : reg.h luafar.h util.h version.h
exported.o : luafar.h util.h
lregex.o   : luafar.h
util.o     : util.h
lflua.o    : luafar.h
luafar.o   : version.h

# (end of Makefile)
