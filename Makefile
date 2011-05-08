include ./config.mak

.PHONY: all install clean

ARCH = -m32
ifeq ($(ARCH),-m64)
  TARGETDIR = win64_bin
else
  TARGETDIR = win32_bin
endif

all: src/luafar.mak
	cd src && $(MAKE) -f luafar.mak ARCH=$(ARCH)

install: src/$(LUAFARDLL) $(TARGETDIR)
	cd src && move /Y $(LUAFARDLL) ..\$(TARGETDIR)

$(TARGETDIR):
	if not exist $@ mkdir $@

clean: src/luafar.mak
	cd src && $(MAKE) -f luafar.mak clean
