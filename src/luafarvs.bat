@rem Script to build LuaFAR under "Visual Studio .NET Command Prompt".
@rem It creates luafar3.dll and luafar3.lib.

@setlocal
@set INC_FAR=..\..\..\system\include\far\unicode
@set INC_LUA=..\..\..\system\include\lua51
@set DEFINES=/DBUILD_DLL /DWINVER=0x500 /DLUADLL=\"lua5.1.dll\" /DFAR_LUA
@set MYCOMPILE=cl /nologo /MT /O2 /W3 /c /D_CRT_SECURE_NO_DEPRECATE^
  /I%INC_FAR% /I%INC_LUA% %DEFINES%
@set MYLINK=link /nologo rpcrt4.lib advapi32.lib shell32.lib user32.lib
@set MYMT=mt /nologo
@set MYRC=rc
@set MYLUA=lua

if exist *.obj del *.obj
%MYLUA% makeflags.lua %INC_FAR%\plugin.hpp > flags.c
%MYLUA% makefarkeys.lua %INC_FAR% keysandcolors.c
%MYLUA% -erequire([[macro]])([[define.lua]],[[version.h.mcr]],[[version.h]])
%MYCOMPILE% bit64.c^
  exported.c flags.c keysandcolors.c lflua.c lregex.c reg.c service.c^
  slnudata.c slnunico.c uliolib51.c uloadlib51.c ustring.c util.c win.c
%MYRC% luafar.rc
%MYLINK% /DLL /out:luafar3.dll *.obj lua5.1_x86.lib luafar.res
if exist *.obj del *.obj
