//---------------------------------------------------------------------------
#include <windows.h>
#include "lua.h"
#include "luafar.h"

#define LUAPLUG WINAPI __declspec(dllexport)

#ifdef FUNC_OPENLIBS
extern int FUNC_OPENLIBS (lua_State*);
#else
#define FUNC_OPENLIBS NULL
#endif

#ifndef ENV_PREFIX
# ifdef _WIN64
#  define ENV_PREFIX L"LUAFAR64"
# else
#  define ENV_PREFIX L"LUAFAR"
# endif
#endif

lua_State* LS;
INT_PTR WINAPI DlgProc(HANDLE hDlg, int Msg, int Param1, INT_PTR Param2)
{
  return LF_DlgProc(LS, hDlg, Msg, Param1, Param2);
}

struct PluginStartupInfo Info;
struct FarStandardFunctions FSF;
GUID PluginId;
TPluginData PluginData = { &Info, &FSF, &PluginId, DlgProc };
wchar_t PluginName[512], PluginDir[512];
//---------------------------------------------------------------------------

BOOL WINAPI DllMain (HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
  (void) lpReserved;
  if (DLL_PROCESS_ATTACH == dwReason && hDll) {
    if ((LS = LF_LuaOpen()) != NULL) {
      GetModuleFileNameW((HINSTANCE)hDll, PluginName, sizeof(PluginName)/sizeof(PluginName[0]));
      wcscpy(PluginDir, PluginName);
      wcsrchr(PluginDir, L'\\')[1] = 0;
    }
  }
  else if (DLL_PROCESS_DETACH == dwReason) {
    if (LS) {
      LF_LuaClose(LS);
      LS = NULL;
    }
  }
  return TRUE;
}

// This function must have __cdecl calling convention, it is not `LUAPLUG'.
int __declspec(dllexport) luaopen_luaplug (lua_State *L)
{
  LF_InitLuaState1(L, PluginDir, FUNC_OPENLIBS, ENV_PREFIX);
  LF_InitLuaState2(L, &PluginData);
  return 0;
}
//---------------------------------------------------------------------------

void LUAPLUG GetGlobalInfoW(struct GlobalInfo *globalInfo)
{
  if (LS) {
    LF_InitLuaState1(LS, PluginDir, FUNC_OPENLIBS, ENV_PREFIX);
    if (LF_GetGlobalInfo(LS, globalInfo, PluginDir))
      PluginId = globalInfo->Guid;
    else {
      LF_LuaClose(LS);
      LS = NULL;
    }
  }
}
//---------------------------------------------------------------------------

void LUAPLUG SetStartupInfoW(const struct PluginStartupInfo *aInfo)
{
  if (LS) {
    Info = *aInfo;
    FSF = *aInfo->FSF;
    Info.FSF = &FSF;
    LF_InitLuaState2(LS, &PluginData);
    if (LF_RunDefaultScript(LS) == FALSE) {
      LF_LuaClose(LS);
      LS = NULL;
    }
  }
}
//---------------------------------------------------------------------------

void LUAPLUG GetPluginInfoW(struct PluginInfo *Info)
{
  if(LS) LF_GetPluginInfo (LS, Info);
}
//---------------------------------------------------------------------------

int LUAPLUG ProcessSynchroEventW(int Event, void *Param)
{
  if(LS) return LF_ProcessSynchroEvent(LS, Event, Param);
  return 0;
}
//---------------------------------------------------------------------------

// This is exported in order not to crash when run from under Far 2.0.xxxx
// Minimal Far version = 3.0.0
int LUAPLUG GetMinFarVersionW()
{
  return (3<<8) | 0 | (0<<16);
}
//---------------------------------------------------------------------------

#ifdef EXPORT_OPEN
HANDLE LUAPLUG OpenW(const struct OpenInfo *Info)
{
  if(LS) return LF_Open(LS, Info);
  return INVALID_HANDLE_VALUE;
}
#endif
//---------------------------------------------------------------------------

#ifdef EXPORT_GETFINDDATA
int LUAPLUG GetFindDataW(struct GetFindDataInfo *Info)
{
  if(LS) return LF_GetFindData(LS, Info);
  return FALSE;
}
#endif
//---------------------------------------------------------------------------

#ifdef EXPORT_FREEFINDDATA
void LUAPLUG FreeFindDataW(const struct FreeFindDataInfo *Info)
{
  if(LS) LF_FreeFindData(LS, Info);
}
#endif
//---------------------------------------------------------------------------

#ifdef EXPORT_CLOSEPANEL
void LUAPLUG ClosePanelW(HANDLE hPanel)
{
  if(LS) LF_ClosePanel(LS, hPanel);
}
#endif
//---------------------------------------------------------------------------

#ifdef EXPORT_GETFILES
int LUAPLUG GetFilesW(struct GetFilesInfo *Info)
{
  if(LS)
    return LF_GetFiles(LS, Info);
  return 0;
}
#endif
//---------------------------------------------------------------------------

#ifdef EXPORT_GETOPENPANELINFO
void LUAPLUG GetOpenPanelInfoW(struct OpenPanelInfo *Info)
{
  if(LS) LF_GetOpenPanelInfo(LS, Info);
}
#endif
//---------------------------------------------------------------------------

#ifdef EXPORT_EXITFAR
void LUAPLUG ExitFARW()
{
  if(LS) {
    LF_ExitFAR(LS);
    LF_LuaClose(LS); LS = NULL;
  }
}
#endif
//---------------------------------------------------------------------------

#ifdef EXPORT_COMPARE
int LUAPLUG CompareW(const struct CompareInfo *Info)
{
  if(LS) return LF_Compare(LS, Info);
  return 0;
}
#endif
//---------------------------------------------------------------------------

#ifdef EXPORT_CONFIGURE
int LUAPLUG ConfigureW(const GUID* Guid)
{
  if(LS) return LF_Configure(LS, Guid);
  return FALSE;
}
#endif
//---------------------------------------------------------------------------

#ifdef EXPORT_DELETEFILES
int LUAPLUG DeleteFilesW(const struct DeleteFilesInfo *Info)
{
  if(LS) return LF_DeleteFiles(LS, Info);
  return FALSE;
}
#endif
//---------------------------------------------------------------------------

#ifdef EXPORT_FREEVIRTUALFINDDATA
void LUAPLUG FreeVirtualFindDataW(const struct FreeFindDataInfo *Info)
{
  if(LS) LF_FreeVirtualFindData(LS, Info);
}
#endif
//---------------------------------------------------------------------------

#ifdef EXPORT_GETVIRTUALFINDDATA
int LUAPLUG GetVirtualFindDataW(struct GetVirtualFindDataInfo *Info)
{
  if(LS) return LF_GetVirtualFindData(LS, Info);
  return FALSE;
}
#endif
//---------------------------------------------------------------------------

#ifdef EXPORT_MAKEDIRECTORY
int LUAPLUG MakeDirectoryW(struct MakeDirectoryInfo *Info)
{
  if(LS) return LF_MakeDirectory(LS, Info);
  return 0;
}
#endif
//---------------------------------------------------------------------------

#ifdef EXPORT_PROCESSEVENT
int LUAPLUG ProcessEventW(HANDLE hPanel, int Event, void *Param)
{
  if(LS) return LF_ProcessEvent(LS, hPanel, Event, Param);
  return FALSE;
}
#endif
//---------------------------------------------------------------------------

#ifdef EXPORT_PROCESSHOSTFILE
int LUAPLUG ProcessHostFileW(const struct ProcessHostFileInfo *Info)
{
  if(LS) return LF_ProcessHostFile(LS, Info);
  return FALSE;
}
#endif
//---------------------------------------------------------------------------

#ifdef EXPORT_PROCESSKEY
int LUAPLUG ProcessKeyW(HANDLE hPanel, const INPUT_RECORD *Rec)
{
  if(LS) return LF_ProcessKey(LS, hPanel, Rec);
  return FALSE;
}
#endif
//---------------------------------------------------------------------------

#ifdef EXPORT_PUTFILES
int LUAPLUG PutFilesW(const struct PutFilesInfo *Info)
{
  if(LS) return LF_PutFiles(LS, Info);
  return 0;
}
#endif
//---------------------------------------------------------------------------

#ifdef EXPORT_SETDIRECTORY
int LUAPLUG SetDirectoryW(const struct SetDirectoryInfo *Info)
{
  if(LS) return LF_SetDirectory(LS, Info);
  return FALSE;
}
#endif
//---------------------------------------------------------------------------

#ifdef EXPORT_SETFINDLIST
int LUAPLUG SetFindListW(const struct SetFindListInfo *Info)
{
  if(LS) return LF_SetFindList(LS, Info);
  return FALSE;
}
#endif
//---------------------------------------------------------------------------

#ifdef EXPORT_PROCESSEDITORINPUT
int LUAPLUG ProcessEditorInputW(const INPUT_RECORD *Rec)
{
  if(LS) return LF_ProcessEditorInput(LS, Rec);
  return 0;
}
#endif
//---------------------------------------------------------------------------

#ifdef EXPORT_PROCESSEDITOREVENT
int LUAPLUG ProcessEditorEventW(int Event, void *Param)
{
  if(LS) return LF_ProcessEditorEvent(LS, Event, Param);
  return 0;
}
#endif
//---------------------------------------------------------------------------

#ifdef EXPORT_PROCESSVIEWEREVENT
int LUAPLUG ProcessViewerEventW(int Event, void *Param)
{
  if(LS) return LF_ProcessViewerEvent(LS, Event, Param);
  return 0;
}
#endif
//---------------------------------------------------------------------------

#ifdef EXPORT_PROCESSDIALOGEVENT
int LUAPLUG ProcessDialogEventW(int Event, void *Param)
{
  if(LS) return LF_ProcessDialogEvent(LS, Event, Param);
  return 0;
}
#endif
//---------------------------------------------------------------------------

#ifdef EXPORT_GETCUSTOMDATA
int LUAPLUG GetCustomDataW(const wchar_t *FilePath, wchar_t **CustomData)
{
  if(LS) return LF_GetCustomData(LS, FilePath, CustomData);
  return 0;
}

void LUAPLUG FreeCustomDataW(wchar_t *CustomData)
{
  if(LS) LF_FreeCustomData(LS, CustomData);
}
#endif
//---------------------------------------------------------------------------

#ifdef EXPORT_ANALYSE
int LUAPLUG AnalyseW(const struct AnalyseInfo *Info)
{
  if(LS) return LF_Analyse(LS, Info);
  return 0;
}
#endif
//---------------------------------------------------------------------------
