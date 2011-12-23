# luaplug.mak

ifndef PATH_LUAFAR
    PATH_LUAFAR = ..
endif
ifndef TARGET
    TARGET = luaplug.dll
endif
ifdef FAR_EXPORTS
    EXPORTS = $(addprefix -DEXPORT_,$(FAR_EXPORTS)) 
endif

PATH_LUAFARSRC = $(PATH_LUAFAR)\src

include $(PATH_LUAFAR)/config.mak

ARCH= -m32
CC= gcc

OBJ       = luaplug.o
CFLAGS    = -O2 -Wall -I$(INC_FAR) -I$(INC_LUA) $(EXPORTS) \
            $(ARCH) -D_export=
MYLDFLAGS = -Wl,--kill-at -shared -s $(ARCH)

vpath %.c $(PATH_LUAFARSRC)
vpath %.h $(PATH_LUAFARSRC)

$(TARGET): $(OBJ) $(PATH_LUAFARSRC)\$(LUAFARDLL)
	$(CC) -o $@ $^ $(MYLDFLAGS)

$(PATH_LUAFARSRC)\$(LUAFARDLL):
	cd $(PATH_LUAFARSRC) && $(MAKE)

luaplug.o: luaplug.c luafar.h
# (end of Makefile)
