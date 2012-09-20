#ifndef LUAFAR_H
#define LUAFAR_H

#include <plugin.hpp>

#ifdef __cplusplus
extern "C" {
#endif

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#if defined BUILD_DLL
#  define DLLFUNC __declspec(dllexport)
#elif defined BUILD_STATIC
#  define DLLFUNC
#else
#  define DLLFUNC __declspec(dllimport)
#endif

typedef struct {
  struct PluginStartupInfo *Info;
  struct FarStandardFunctions *FSF;
  GUID *PluginId;
  FARWINDOWPROC DlgProc;
  FARMACROCALLBACK MacroCallback;
  lua_Alloc origAlloc;
  void *origUserdata;
  void (*new_action) (int i);
  void (*old_action) (int i);
} TPluginData;
TPluginData* GetPluginData(lua_State* L);

typedef struct {
  lua_State *L;
  int ref;
} FarPaneItemUserData;

DLLFUNC INT_PTR LF_DlgProc(lua_State *L, HANDLE hDlg, int Msg, int Param1, void *Param2);
DLLFUNC int     LF_MacroCallback (lua_State* L, void* Id, FARADDKEYMACROFLAGS Flags);
DLLFUNC const wchar_t *LF_Gsub (lua_State *L, const wchar_t *s, const wchar_t *p, const wchar_t *r);
DLLFUNC void    LF_InitLuaState1(lua_State *L, lua_CFunction aOpenLibs);
DLLFUNC void    LF_InitLuaState2 (lua_State *L, TPluginData *aData);
DLLFUNC int     LF_LoadFile(lua_State *L, const wchar_t* filename);
DLLFUNC int     LF_DoFile(lua_State *L, const wchar_t *fname, int argc, wchar_t* argv[]);
DLLFUNC void    LF_LuaClose(lua_State* L);
DLLFUNC lua_State* LF_LuaOpen(void);
DLLFUNC int     LF_Message(lua_State *L, const wchar_t* aMsg, const wchar_t* aTitle, const wchar_t* aButtons, const char* aFlags, const wchar_t* aHelpTopic, const GUID* aId);
DLLFUNC void    LF_ProcessEnvVars (lua_State *L, const wchar_t* aEnvPrefix, const wchar_t* PluginDir);
DLLFUNC BOOL    LF_RunDefaultScript(lua_State* L);

DLLFUNC HANDLE  LF_Analyse (lua_State* L, const struct AnalyseInfo *Info);
DLLFUNC void    LF_CloseAnalyse (lua_State* L, const struct CloseAnalyseInfo *Info);
DLLFUNC void    LF_ClosePanel (lua_State* L, const struct ClosePanelInfo *Info);
DLLFUNC int     LF_Compare (lua_State* L, const struct CompareInfo *Info);
DLLFUNC int     LF_Configure (lua_State* L, const struct ConfigureInfo *Info);
DLLFUNC int     LF_DeleteFiles (lua_State* L, const struct DeleteFilesInfo *Info);
DLLFUNC void    LF_ExitFAR (lua_State* L, const struct ExitInfo *Info);
DLLFUNC void    LF_FreeFindData (lua_State* L, const struct FreeFindDataInfo *Info);
DLLFUNC int     LF_GetFiles (lua_State* L, struct GetFilesInfo *Info);
DLLFUNC int     LF_GetFindData (lua_State* L, struct GetFindDataInfo *Info);
DLLFUNC int     LF_GetGlobalInfo (lua_State* L, struct GlobalInfo *Info, const wchar_t *PluginDir);
DLLFUNC void    LF_GetOpenPanelInfo (lua_State* L, struct OpenPanelInfo *Info);
DLLFUNC void    LF_GetPluginInfo (lua_State* L, struct PluginInfo *Info);
DLLFUNC int     LF_MakeDirectory (lua_State* L, struct MakeDirectoryInfo *Info);
DLLFUNC HANDLE  LF_Open (lua_State* L, const struct OpenInfo *Info);
DLLFUNC int     LF_ProcessDialogEvent (lua_State* L, const struct ProcessDialogEventInfo *Info);
DLLFUNC int     LF_ProcessEditorEvent (lua_State* L, const struct ProcessEditorEventInfo *Info);
DLLFUNC int     LF_ProcessEditorInput (lua_State* L, const struct ProcessEditorInputInfo *Info);
DLLFUNC int     LF_ProcessHostFile (lua_State* L, const struct ProcessHostFileInfo *Info);
DLLFUNC int     LF_ProcessPanelEvent (lua_State* L, const struct ProcessPanelEventInfo *Info);
DLLFUNC int     LF_ProcessPanelInput (lua_State* L, const struct ProcessPanelInputInfo *Info);
DLLFUNC int     LF_ProcessSynchroEvent (lua_State* L, const struct ProcessSynchroEventInfo *Info);
DLLFUNC int     LF_ProcessViewerEvent (lua_State* L, const struct ProcessViewerEventInfo *Info);
DLLFUNC int     LF_PutFiles (lua_State* L, const struct PutFilesInfo *Info);
DLLFUNC int     LF_SetDirectory (lua_State* L, const struct SetDirectoryInfo *Info);
DLLFUNC int     LF_SetFindList (lua_State* L, const struct SetFindListInfo *Info);
DLLFUNC int     LF_GetCustomData(lua_State* L, const wchar_t *FilePath, wchar_t **CustomData);
DLLFUNC void    LF_FreeCustomData(lua_State* L, wchar_t *CustomData);

#ifdef __cplusplus
}
#endif

#endif // LUAFAR_H
