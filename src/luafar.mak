# luafar.mak

include ../config.mak

ARCH= -m32
CC= gcc
CFLAGS= -O2 -Wall -I$(INC_FAR) -I$(INC_LUA) -DBUILD_DLL $(ARCH) -D_export=
MYLDFLAGS= -L$(LUADLLPATH) -l$(LUADLLNAME) -shared $(ARCH) -s

ifeq ($(ARCH),-m64)
  RESTARGET= -F pe-x86-64
else
  RESTARGET= -F pe-i386
endif

OBJ = \
  bit.o         \
  exported.o    \
  flags.o       \
  lflua.o       \
  lregex.o      \
  luafar.o      \
  reg.o         \
  service.o     \
  slnunico.o    \
  uliolib.o     \
  uloadlib.o    \
  ustring.o     \
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

flags.c: $(INC_FAR)\plugin.hpp
	lua -erequire('makeflags')([[$<]]) > $@

# (end of Makefile)
