include ./config.mak

.PHONY: all lib auxil clean cleansrc

ARCH = -m32
ifeq ($(ARCH),-m64)
  TARGETDIR = win64_bin
else
  TARGETDIR = win32_bin
endif

all: lib auxil

$(TARGETDIR):
	if not exist $@ mkdir $@

lib: src/luafar.mak $(TARGETDIR)
	if exist src\$(LUAFARDLL) del /F src\$(LUAFARDLL)
	cd src && $(MAKE) -f luafar.mak ARCH=$(ARCH)
	cd src && move /Y $(LUAFARDLL) ..\$(TARGETDIR)

auxil:
	cd auxil && $(MAKE)

clean:
	-cd auxil && del farcolor.lua farkeys.lua
	-cd $(TARGETDIR) && del luafarw.dll
	$(MAKE) cleansrc

cleansrc:
	cd src && del *.o *.dll flags.cpp
