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

OBJ1 = \
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
  ustring.o        \
  util.o           \
  win.o

ifeq ($(LUAVERSION),52)
  OBJ = $(OBJ1) compat52.o uliolib52.o uloadlib52.o
else
  OBJ = $(OBJ1) uliolib51.o uloadlib51.o
endif

$(LUAFARDLL): $(OBJ) $(LUADLLNAME).dll
	$(CC) -o $@ $^ $(LDFLAGS)

flags.c: $(INC_FAR)\plugin.hpp makeflags.lua
	$(LUAEXE) makeflags.lua $< > $@

keysandcolors.c: $(INC_FAR)/farcolor.hpp makefarkeys.lua
	$(LUAEXE) makefarkeys.lua $(INC_FAR) $@

version.h : % : %.mcr define.lua
	$(LUAEXE) -erequire([[macro]])([[define.lua]],[[$<]],[[$@]])

clean:
	del *.o flags.c keysandcolors.c version.h $(LUAFARDLL)

# Dependencies
*.o        : $(INC_FAR)\plugin.hpp ustring.h compat52.h
compat52.o : compat52.h
exported.o : luafar.h util.h
lflua.o    : luafar.h
lregex.o   : luafar.h
luafar.o   : version.h
reg.o      : reg.h
service.o  : luafar.h util.h version.h
util.o     : util.h
win.o      : util.h reg.h

# (end of Makefile)
