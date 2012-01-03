include ./config.mak

.PHONY: all install clean

ARCH = -m32
ifeq ($(ARCH),-m64)
  TARGETDIR = win64_bin
else
  TARGETDIR = win32_bin
endif

all: src/Makefile
	cd src && $(MAKE) ARCH=$(ARCH)

install: src/$(LUAFARDLL) $(TARGETDIR)
	cd src && copy /Y $(LUAFARDLL) ..\$(TARGETDIR)

$(TARGETDIR):
	if not exist $@ mkdir $@

clean: src/Makefile
	cd src && $(MAKE) clean
