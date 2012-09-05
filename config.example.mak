#To use this file, rename it to config.mak

ifndef PATH_LUAFAR
  PATH_LUAFAR = ..
endif

ifeq ($(LUAVERSION),52)
  INC_LUA = $(PATH_LUAFAR)\..\..\system\include\lua52
  LUADLLNAME = lua52
else
  INC_LUA = $(PATH_LUAFAR)\..\..\system\include\lua51
  LUADLLNAME = lua5.1
endif

INC_FAR = $(PATH_LUAFAR)\..\..\system\include\far\unicode
LUAFARDLL = luafar3.dll
LUAEXE = c:\exe32\lua.exe
MYLDFLAGS = -Lc:\exe32
