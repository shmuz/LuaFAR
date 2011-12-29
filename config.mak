ifndef PATH_LUAFAR
  PATH_LUAFAR = ..
endif

INC_FAR = $(PATH_LUAFAR)\..\..\system\include\far\unicode
INC_LUA = $(PATH_LUAFAR)\..\..\system\include

ifeq ($(ARCH),-m64)
  PATH_EXE = c:\exe64
else
  PATH_EXE = c:\exe32
endif

LUADLLNAME = lua5.1
LUAFARDLL  = luafar3.dll
LUAEXE     = c:\exe32\lua.exe
