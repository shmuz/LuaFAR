INC_FAR    = s:\progr\work\system\include\far\unicode
INC_LUA    = s:\progr\work\system\include

ifeq ($(ARCH),-m64)
  LUADLLPATH = c:\exe64
else
  LUADLLPATH = c:\exe
endif

LUADLLNAME = lua5.1
LUAFARDLL  = luafarw.dll
LUAEXE     = c:\exe\lua.exe
