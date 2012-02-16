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
INT_PTR WINAPI DlgProc(HANDLE hDlg, int Msg, int Param1, void *Param2)
{
  return LF_DlgProc(LS, hDlg, Msg, Param1, Param2);
}

int WINAPI MacroCallback (void* Id, FARADDKEYMACROFLAGS Flags)
{
  return LF_MacroCallback(LS, Id, Flags);
}

struct PluginStartupInfo Info;
struct FarStandardFunctions FSF;
GUID PluginId;
TPluginData PluginData = { &Info, &FSF, &PluginId, DlgProc, MacroCallback, NULL, NULL };
wchar_t PluginName[512], PluginDir[512];
int Init1_Done = 0, Init2_Done = 0; // Ensure intializations are done only once
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
  LF_InitLuaState1(L, FUNC_OPENLIBS);
  LF_InitLuaState2(L, &PluginData);
  LF_ProcessEnvVars(LS, ENV_PREFIX, PluginDir);
  return 0;
}
//---------------------------------------------------------------------------

void LUAPLUG GetGlobalInfoW(struct GlobalInfo *globalInfo)
{
  if (LS) {
    if (!Init1_Done) {
      LF_InitLuaState1(LS, FUNC_OPENLIBS);
      Init1_Done = 1;
    }
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
  if (LS && !Init2_Done) {
    Init2_Done = 1;
    Info = *aInfo;
    FSF = *aInfo->FSF;
    Info.FSF = &FSF;
    LF_InitLuaState2(LS, &PluginData);
    LF_ProcessEnvVars(LS, ENV_PREFIX, PluginDir);
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

int LUAPLUG ProcessSynchroEventW(const struct ProcessSynchroEventInfo *Info)
{
  if(LS) return LF_ProcessSynchroEvent(LS, Info);
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
void LUAPLUG ClosePanelW(const struct ClosePanelInfo *Info)
{
  if(LS) LF_ClosePanel(LS, Info);
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
void LUAPLUG ExitFARW(const struct ExitInfo *Info)
{
  if(LS) {
    LF_ExitFAR(LS, Info);
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
int LUAPLUG ConfigureW(const struct ConfigureInfo *Info)
{
  if(LS) return LF_Configure(LS, Info);
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

#ifdef EXPORT_PROCESSPANELEVENT
int LUAPLUG ProcessPanelEventW(const struct ProcessPanelEventInfo *Info)
{
  if(LS) return LF_ProcessPanelEvent(LS, Info);
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

#ifdef EXPORT_PROCESSPANELINPUT
int LUAPLUG ProcessPanelInputW(const struct ProcessPanelInputInfo *Info)
{
  if(LS) return LF_ProcessPanelInput(LS, Info);
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
int LUAPLUG ProcessEditorInputW(const struct ProcessEditorInputInfo *Info)
{
  if(LS) return LF_ProcessEditorInput(LS, Info);
  return 0;
}
#endif
//---------------------------------------------------------------------------

#ifdef EXPORT_PROCESSEDITOREVENT
int LUAPLUG ProcessEditorEventW(const struct ProcessEditorEventInfo *Info)
{
  if(LS) return LF_ProcessEditorEvent(LS, Info);
  return 0;
}
#endif
//---------------------------------------------------------------------------

#ifdef EXPORT_PROCESSVIEWEREVENT
int LUAPLUG ProcessViewerEventW(const struct ProcessViewerEventInfo *Info)
{
  if(LS) return LF_ProcessViewerEvent(LS, Info);
  return 0;
}
#endif
//---------------------------------------------------------------------------

#ifdef EXPORT_PROCESSDIALOGEVENT
int LUAPLUG ProcessDialogEventW(const struct ProcessDialogEventInfo *Info)
{
  if(LS) return LF_ProcessDialogEvent(LS, Info);
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
HANDLE LUAPLUG AnalyseW(const struct AnalyseInfo *Info)
{
  if(LS) return LF_Analyse(LS, Info);
  return 0;
}
#endif
//---------------------------------------------------------------------------

#ifdef EXPORT_CLOSEANALYSE
void LUAPLUG CloseAnalyseW(const struct CloseAnalyseInfo *Info)
{
  if(LS) LF_CloseAnalyse(LS, Info);
}
#endif
//---------------------------------------------------------------------------
