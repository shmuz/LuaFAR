# luafar.mak

include ../config.mak

ARCH= -m32
CC= gcc
CFLAGS= -O2 -W -Wall -I$(INC_FAR) -I$(INC_LUA) -DBUILD_DLL -DWINVER=0x500 \
        -DLUADLL=\"$(LUADLLNAME).dll\" $(ARCH)
MYLDFLAGS= -L$(PATH_EXE) -l$(LUADLLNAME) -shared $(ARCH) -s -lrpcrt4

ifeq ($(ARCH),-m64)
  RESTARGET= -F pe-x86-64
else
  RESTARGET= -F pe-i386
endif

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

$(LUAFARDLL): $(OBJ)
	$(CC) -o $@ $^ $(MYLDFLAGS)

%.o : %.rc
	windres $< -o $@ $(RESTARGET)

# Dependencies
*.o        : $(INC_FAR)\plugin.hpp
reg.o      : reg.h
service.o  : reg.h luafar.h util.h version.h ustring.h
exported.o : luafar.h util.h ustring.h
lregex.o   : luafar.h util.h ustring.h
util.o     : util.h ustring.h
lflua.o    : luafar.h util.h ustring.h
luafar.o   : version.h

flags.c: $(INC_FAR)\plugin.hpp makeflags.lua
	lua makeflags.lua $< > $@

keysandcolors.c: $(INC_FAR)/farcolor.hpp makefarkeys.lua
	lua makefarkeys.lua $(INC_FAR) $@

clean:
	del *.o flags.c keysandcolors.c luafar3.dll

# (end of Makefile)
