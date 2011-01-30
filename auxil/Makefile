include ../config.mak

all: farcolor.lua farkeys.lua

farcolor.lua: $(INC_FAR)/farcolor.hpp
	lua -erequire('makefarkeys')([[$<]]) > $@
	del tmp.c tmp.exe

farkeys.lua: $(INC_FAR)/farkeys.hpp
	lua -erequire('makefarkeys')([[$<]]) > $@
	del tmp.c tmp.exe

clean:
	del tmp.c tmp.exe farcolor.lua farkeys.lua

.PHONY: all clean
