//---------------------------------------------------------------------------

#include <windows.h>
#include <ctype.h>
#include <math.h>
#include "luafar.h"
#include "reg.h"
#include "util.h"
#include "ustring.h"
#include "version.h"

#ifndef LUADLL
#define LUADLL "lua5.1.dll"
#endif

typedef struct PluginStartupInfo PSInfo;
typedef unsigned __int64 UINT64;

extern int push64(lua_State *L, UINT64 v);
extern UINT64 check64(lua_State *L, int pos, int *success);

extern int luaopen_bit64 (lua_State *L);
extern int luaopen_regex (lua_State*);
extern int luaopen_uio (lua_State *L);
extern int luaopen_unicode (lua_State *L);
extern int luaopen_upackage (lua_State *L);

extern int  luaB_loadfileW (lua_State *L);
extern int  pcall_msg (lua_State* L, int narg, int nret);
extern void push_flags_table (lua_State *L);
extern void SetFarColors (lua_State *L);

#define CAST(tp,expr) (tp)(expr)
#define DIM(buff) (sizeof(buff)/sizeof(buff[0]))
#define OptHandle(L,i) ((HANDLE)luaL_optinteger (L,i,(INT_PTR)INVALID_HANDLE_VALUE))

const char FarFileFilterType[] = "FarFileFilter";
const char FarTimerType[]      = "FarTimer";
const char FarDialogType[]     = "FarDialog";
const char SettingsType[]      = "FarSettings";
const char SettingsHandles[]   = "FarSettingsHandles";
const char PluginHandleType[]  = "FarPluginHandle";

const char FAR_VIRTUALKEYS[]   = "far.virtualkeys";
const char FAR_FLAGSTABLE[]    = "far.Flags";

const char* VirtualKeyStrings[256] = {
  // 0x00
  NULL, "LBUTTON", "RBUTTON", "CANCEL",
  "MBUTTON", "XBUTTON1", "XBUTTON2", NULL,
  "BACK", "TAB", NULL, NULL,
  "CLEAR", "RETURN", NULL, NULL,
  // 0x10
  "SHIFT", "CONTROL", "MENU", "PAUSE",
  "CAPITAL", "KANA", NULL, "JUNJA",
  "FINAL", "HANJA", NULL, "ESCAPE",
  NULL, "NONCONVERT", "ACCEPT", "MODECHANGE",
  // 0x20
  "SPACE", "PRIOR", "NEXT", "END",
  "HOME", "LEFT", "UP", "RIGHT",
  "DOWN", "SELECT", "PRINT", "EXECUTE",
  "SNAPSHOT", "INSERT", "DELETE", "HELP",
  // 0x30
  "0", "1", "2", "3",
  "4", "5", "6", "7",
  "8", "9", NULL, NULL,
  NULL, NULL, NULL, NULL,
  // 0x40
  NULL, "A", "B", "C",
  "D", "E", "F", "G",
  "H", "I", "J", "K",
  "L", "M", "N", "O",
  // 0x50
  "P", "Q", "R", "S",
  "T", "U", "V", "W",
  "X", "Y", "Z", "LWIN",
  "RWIN", "APPS", NULL, "SLEEP",
  // 0x60
  "NUMPAD0", "NUMPAD1", "NUMPAD2", "NUMPAD3",
  "NUMPAD4", "NUMPAD5", "NUMPAD6", "NUMPAD7",
  "NUMPAD8", "NUMPAD9", "MULTIPLY", "ADD",
  "SEPARATOR", "SUBTRACT", "DECIMAL", "DIVIDE",
  // 0x70
  "F1", "F2", "F3", "F4",
  "F5", "F6", "F7", "F8",
  "F9", "F10", "F11", "F12",
  "F13", "F14", "F15", "F16",
  // 0x80
  "F17", "F18", "F19", "F20",
  "F21", "F22", "F23", "F24",
  NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL,
  // 0x90
  "NUMLOCK", "SCROLL", "OEM_NEC_EQUAL", "OEM_FJ_MASSHOU",
  "OEM_FJ_TOUROKU", "OEM_FJ_LOYA", "OEM_FJ_ROYA", NULL,
  NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL,
  // 0xA0
  "LSHIFT", "RSHIFT", "LCONTROL", "RCONTROL",
  "LMENU", "RMENU", "BROWSER_BACK", "BROWSER_FORWARD",
  "BROWSER_REFRESH", "BROWSER_STOP", "BROWSER_SEARCH", "BROWSER_FAVORITES",
  "BROWSER_HOME", "VOLUME_MUTE", "VOLUME_DOWN", "VOLUME_UP",
  // 0xB0
  "MEDIA_NEXT_TRACK", "MEDIA_PREV_TRACK", "MEDIA_STOP", "MEDIA_PLAY_PAUSE",
  "LAUNCH_MAIL", "LAUNCH_MEDIA_SELECT", "LAUNCH_APP1", "LAUNCH_APP2",
  NULL, NULL, "OEM_1", "OEM_PLUS",
  "OEM_COMMA", "OEM_MINUS", "OEM_PERIOD", "OEM_2",
  // 0xC0
  "OEM_3", NULL, NULL, NULL,
  NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL,
  // 0xD0
  NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, "OEM_4",
  "OEM_5", "OEM_6", "OEM_7", "OEM_8",
  // 0xE0
  NULL, NULL, "OEM_102", NULL,
  NULL, "PROCESSKEY", NULL, "PACKET",
  NULL, "OEM_RESET", "OEM_JUMP", "OEM_PA1",
  "OEM_PA2", "OEM_PA3", "OEM_WSCTRL", NULL,
  // 0xF0
  NULL, NULL, NULL, NULL,
  NULL, NULL, "ATTN", "CRSEL",
  "EXSEL", "EREOF", "PLAY", "ZOOM",
  "NONAME", "PA1", "OEM_CLEAR", NULL,
};

static UINT64 get_env_flag (lua_State *L, int pos, int *success)
{
  UINT64 ret = 0;
  int type = lua_type (L, pos);
  if (success)
    *success = TRUE;
  //---------------------------------------------------------------------------
  if (type == LUA_TNONE || type == LUA_TNIL) {
  }
  else if (type == LUA_TNUMBER)
    ret = (unsigned int)lua_tointeger (L, pos);
  else if (type == LUA_TSTRING) {
    int tmp;
    const char* s = lua_tostring(L, pos);
    //-------------------------------------------------------------------------
    if (*s == '0') {
      ret = check64(L, pos, &tmp);
      if (success) *success = tmp;
    }
    //-------------------------------------------------------------------------
    else {
      lua_getfield(L, LUA_REGISTRYINDEX, FAR_FLAGSTABLE);
      lua_getfield (L, -1, s);
      type = lua_type(L, -1);
      if (type == LUA_TNUMBER)
        ret = (unsigned int)lua_tointeger (L, -1);
      else if (type == LUA_TSTRING) {
        ret = check64(L, -1, &tmp);
        if (success) *success = tmp;
      }
      else if (success)
        *success = FALSE;
      lua_pop (L, 2);
    }
    //-------------------------------------------------------------------------
  }
  else if (success)
    *success = FALSE;
  return ret;
}

static UINT64 check_env_flag (lua_State *L, int pos)
{
  int success = FALSE;
  UINT64 ret = get_env_flag (L, pos, &success);
  if (!success)
    luaL_argerror(L, pos, "invalid flag");
  return ret;
}

UINT64 GetFlagCombination (lua_State *L, int pos, int *success)
{
  UINT64 ret = 0;
  if (lua_type (L, pos) == LUA_TTABLE) {
    if (success)
      *success = TRUE;
    pos = abs_index (L, pos);
    lua_pushnil(L);
    while (lua_next(L, pos)) {
      if (lua_type(L,-2)==LUA_TSTRING && lua_toboolean(L,-1)) {
        UINT64 flag = get_env_flag (L, -2, success);
        if (success == NULL || *success)
          ret |= flag;
        else
          { lua_pop(L,2); return ret; }
      }
      lua_pop(L, 1);
    }
  }
  else
    ret = get_env_flag (L, pos, success);
  return ret;
}

static UINT64 CheckFlags(lua_State* L, int pos)
{
  int success = FALSE;
  UINT64 Flags = GetFlagCombination (L, pos, &success);
  if (!success)
    luaL_error(L, "invalid flag combination");
  return Flags;
}

static UINT64 OptFlags(lua_State* L, int pos, UINT64 dflt)
{
  return lua_isnoneornil(L, pos) ? dflt : CheckFlags(L, pos);
}

static UINT64 CheckFlagsFromTable (lua_State *L, int pos, const char* key)
{
  UINT64 f;
  lua_getfield(L, pos, key);
  f = CheckFlags(L, -1);
  lua_pop(L, 1);
  return f;
}

UINT64 GetFlagsFromTable (lua_State *L, int pos, const char* key)
{
  UINT64 f;
  lua_getfield(L, pos, key);
  f = GetFlagCombination(L, -1, NULL);
  lua_pop(L, 1);
  return f;
}

void PutFlagsToTable (lua_State *L, const char* key, UINT64 flags)
{
  push64(L, flags);
  lua_setfield(L, -2, key);
}

void PutFlagsToArray (lua_State *L, int index, UINT64 flags)
{
  push64(L, flags);
  lua_rawseti(L, -2, index);
}

TPluginData* GetPluginData (lua_State* L)
{
  TPluginData *pd;
  (void) lua_getallocf(L, (void**)&pd);
  return pd;
}

static void PushPluginHandle (lua_State *L, HANDLE Handle)
{
  HANDLE *p = (HANDLE*)lua_newuserdata(L, sizeof(HANDLE));
  *p = Handle;
  luaL_getmetatable(L, PluginHandleType);
  lua_setmetatable(L, -2);
}

static int far_GetFileOwner (lua_State *L)
{
  wchar_t Owner[512];
  const wchar_t *Computer = opt_utf8_string (L, 1, NULL);
  const wchar_t *Name = check_utf8_string (L, 2, NULL);
  if (GetPluginData(L)->FSF->GetFileOwner (Computer, Name, Owner, DIM(Owner)))
    push_utf8_string(L, Owner, -1);
  else
    lua_pushnil(L);
  return 1;
}

static int far_GetNumberOfLinks (lua_State *L)
{
  const wchar_t *Name = check_utf8_string (L, 1, NULL);
  int num = GetPluginData(L)->FSF->GetNumberOfLinks (Name);
  return lua_pushinteger (L, num), 1;
}

static int far_LuafarVersion (lua_State *L)
{
  if (lua_toboolean(L, 1)) {
    lua_pushinteger(L, VER_MAJOR);
    lua_pushinteger(L, VER_MINOR);
    lua_pushinteger(L, VER_MICRO);
    return 3;
  }
  lua_pushliteral(L, VER_STRING);
  return 1;
}

static void GetMouseEvent(lua_State *L, MOUSE_EVENT_RECORD* rec)
{
  rec->dwMousePosition.X = GetOptIntFromTable(L, "MousePositionX", 0);
  rec->dwMousePosition.Y = GetOptIntFromTable(L, "MousePositionY", 0);
  rec->dwButtonState = GetOptIntFromTable(L, "ButtonState", 0);
  rec->dwControlKeyState = GetOptIntFromTable(L, "ControlKeyState", 0);
  rec->dwEventFlags = GetOptIntFromTable(L, "EventFlags", 0);
}

void PutMouseEvent(lua_State *L, const MOUSE_EVENT_RECORD* rec, BOOL table_exist)
{
  if (!table_exist)
    lua_createtable(L, 0, 5);
  PutNumToTable(L, "MousePositionX", rec->dwMousePosition.X);
  PutNumToTable(L, "MousePositionY", rec->dwMousePosition.Y);
  PutNumToTable(L, "ButtonState", rec->dwButtonState);
  PutNumToTable(L, "ControlKeyState", rec->dwControlKeyState);
  PutNumToTable(L, "EventFlags", rec->dwEventFlags);
}

// convert a string from utf-8 to wide char and put it into a table,
// to prevent stack overflow and garbage collection
const wchar_t* StoreTempString(lua_State *L, int store_stack_pos, int* index)
{
  const wchar_t *s = check_utf8_string(L,-1,NULL);
  lua_rawseti(L, store_stack_pos, ++(*index));
  return s;
}

static void PushEditorSetPosition(lua_State *L, const struct EditorSetPosition *esp)
{
  lua_createtable(L, 0, 6);
  PutIntToTable(L, "CurLine",       esp->CurLine);
  PutIntToTable(L, "CurPos",        esp->CurPos);
  PutIntToTable(L, "CurTabPos",     esp->CurTabPos);
  PutIntToTable(L, "TopScreenLine", esp->TopScreenLine);
  PutIntToTable(L, "LeftPos",       esp->LeftPos);
  PutIntToTable(L, "Overtype",      esp->Overtype);
}

static void FillEditorSetPosition(lua_State *L, struct EditorSetPosition *esp)
{
  esp->CurLine   = GetOptIntFromTable(L, "CurLine", -1);
  esp->CurPos    = GetOptIntFromTable(L, "CurPos", -1);
  esp->CurTabPos = GetOptIntFromTable(L, "CurTabPos", -1);
  esp->TopScreenLine = GetOptIntFromTable(L, "TopScreenLine", -1);
  esp->LeftPos   = GetOptIntFromTable(L, "LeftPos", -1);
  esp->Overtype  = GetOptIntFromTable(L, "Overtype", -1);
}

// table is on stack top
static void PutFarFindData(lua_State *L, const struct PluginPanelItem *PanelItem)
{
  PutAttrToTable     (L,                       PanelItem->FileAttributes);
  PutNumToTable      (L, "FileSize",           (double)PanelItem->FileSize);
  PutFileTimeToTable (L, "LastWriteTime",      PanelItem->LastWriteTime);
  PutFileTimeToTable (L, "LastAccessTime",     PanelItem->LastAccessTime);
  PutFileTimeToTable (L, "CreationTime",       PanelItem->CreationTime);
  PutWStrToTable     (L, "FileName",           PanelItem->FileName, -1);
  PutWStrToTable     (L, "AlternateFileName",  PanelItem->AlternateFileName, -1);
  PutNumToTable      (L, "AllocationSize",     (double)PanelItem->AllocationSize);
}

// on entry : the table's on the stack top
// on exit  : 2 strings added to the stack top (don't pop them!)
static void GetFarFindData(lua_State *L, struct PluginPanelItem *PanelItem)
{
  //memset(PanelItem, 0, sizeof(*PanelItem)); //TODO

  PanelItem->FileAttributes = GetAttrFromTable(L);
  PanelItem->FileSize = GetFileSizeFromTable(L, "FileSize");
  PanelItem->AllocationSize = GetFileSizeFromTable(L, "AllocationSize");
  PanelItem->LastWriteTime  = GetFileTimeFromTable(L, "LastWriteTime");
  PanelItem->LastAccessTime = GetFileTimeFromTable(L, "LastAccessTime");
  PanelItem->CreationTime   = GetFileTimeFromTable(L, "CreationTime");

  lua_getfield(L, -1, "FileName"); // +1
  PanelItem->FileName = opt_utf8_string(L, -1, L""); // +1

  lua_getfield(L, -2, "AlternateFileName"); // +2
  PanelItem->AlternateFileName = opt_utf8_string(L, -1, L""); // +2
}
//---------------------------------------------------------------------------

static void PushWinFindData (lua_State *L, const WIN32_FIND_DATAW *FData)
{
  lua_createtable(L, 0, 7);
  PutAttrToTable(L,                          FData->dwFileAttributes);
  PutNumToTable(L, "FileSize", FData->nFileSizeLow + 65536.*65536.*FData->nFileSizeHigh);
  PutFileTimeToTable(L, "LastWriteTime",     FData->ftLastWriteTime);
  PutFileTimeToTable(L, "LastAccessTime",    FData->ftLastAccessTime);
  PutFileTimeToTable(L, "CreationTime",      FData->ftCreationTime);
  PutWStrToTable(L, "FileName",              FData->cFileName, -1);
  PutWStrToTable(L, "AlternateFileName",     FData->cAlternateFileName, -1);
}

void PushPanelItem(lua_State *L, const struct PluginPanelItem *PanelItem)
{
  lua_createtable(L, 0, 11); // "PanelItem"
  //-----------------------------------------------------------------------
  PutFarFindData(L, PanelItem);
  PutFlagsToTable(L, "Flags", PanelItem->Flags);
  PutNumToTable(L, "NumberOfLinks", PanelItem->NumberOfLinks);
  if (PanelItem->Description)
    PutWStrToTable(L, "Description",  PanelItem->Description, -1);
  if (PanelItem->Owner)
    PutWStrToTable(L, "Owner",  PanelItem->Owner, -1);
  //-----------------------------------------------------------------------
  /* not clear why custom columns are defined on per-file basis */
  if (PanelItem->CustomColumnNumber > 0) {
    size_t j;
    lua_createtable (L, PanelItem->CustomColumnNumber, 0);
    for(j=0; j < PanelItem->CustomColumnNumber; j++)
      PutWStrToArray(L, j+1, PanelItem->CustomColumnData[j], -1);
    lua_setfield(L, -2, "CustomColumnData");
  }
  //-----------------------------------------------------------------------
  PutNumToTable(L, "UserData", PanelItem->UserData);
  //-----------------------------------------------------------------------
  /* skip PanelItem->Reserved for now */
  //-----------------------------------------------------------------------
}
//---------------------------------------------------------------------------

void PushPanelItems(lua_State *L, const struct PluginPanelItem *PanelItems, int ItemsNumber)
{
  int i;
  lua_createtable(L, ItemsNumber, 0); // "PanelItems"
  for(i=0; i < ItemsNumber; i++) {
    PushPanelItem (L, PanelItems + i);
    lua_rawseti(L, -2, i+1);
  }
}
//---------------------------------------------------------------------------

static BOOL PushFarUserSubkey (lua_State* L, BOOL addOneLevel, wchar_t* trg)
{
  wchar_t user[256];
  wchar_t src[DIM(user)+64] = L"Software\\Far Manager";
  DWORD res = GetEnvironmentVariableW (L"FARUSER", user, DIM(user));
  if (res) {
    if (res >= DIM(user))
      return FALSE;
    wcscat(src, L"\\Users\\");
    wcscat(src, user);
  }
  if (addOneLevel)
    wcscat(src, L"\\Plugins");
  if (trg)
    wcscpy(trg, src);
  push_utf8_string(L, src, -1);
  return TRUE;
}

static int far_PluginStartupInfo(lua_State *L)
{
  TPluginData *pd = GetPluginData(L);
  lua_createtable(L, 0, 3);
  PutWStrToTable(L, "ModuleName", pd->Info->ModuleName, -1);
  PutLStrToTable(L, "PluginGuid", pd->PluginId, sizeof(GUID));
  if (PushFarUserSubkey(L, TRUE, NULL))
    lua_setfield(L, -2, "RootKey");
  return 1;
}

static int far_GetCurrentDirectory (lua_State *L)
{
  struct FarStandardFunctions *FSF = GetPluginData(L)->FSF;
  int size = FSF->GetCurrentDirectory(0, NULL);
  wchar_t* buf = (wchar_t*)lua_newuserdata(L, size * sizeof(wchar_t));
  FSF->GetCurrentDirectory(size, buf);
  push_utf8_string(L, buf, -1);
  return 1;
}

static int push_editor_filename(lua_State *L, int EditorId)
{
  wchar_t* fname;
  PSInfo *Info = GetPluginData(L)->Info;
  int size = Info->EditorControl(EditorId, ECTL_GETFILENAME, 0, 0);
  if (!size) return 0;

  fname = (wchar_t*)lua_newuserdata(L, size * sizeof(wchar_t));
  if (Info->EditorControl(EditorId, ECTL_GETFILENAME, 0, fname)) {
    push_utf8_string(L, fname, -1);
    lua_remove(L, -2);
    return 1;
  }
  lua_pop(L,1);
  return 0;
}

static int editor_GetFileName(lua_State *L) {
  int EditorId = luaL_optinteger(L, 1, -1);
  if (!push_editor_filename(L, EditorId)) lua_pushnil(L);
  return 1;
}

static int editor_GetInfo(lua_State *L)
{
  int EditorId = luaL_optinteger(L, 1, -1);
  PSInfo *Info = GetPluginData(L)->Info;
  struct EditorInfo ei;
  if (!Info->EditorControl(EditorId, ECTL_GETINFO, 0, &ei))
    return lua_pushnil(L), 1;
  lua_createtable(L, 0, 18);
  PutNumToTable(L, "EditorID", ei.EditorID);

  if (push_editor_filename(L, EditorId))
    lua_setfield(L, -2, "FileName");

  PutNumToTable(L, "WindowSizeX", ei.WindowSizeX);
  PutNumToTable(L, "WindowSizeY", ei.WindowSizeY);
  PutNumToTable(L, "TotalLines", ei.TotalLines);
  PutNumToTable(L, "CurLine", ei.CurLine);
  PutNumToTable(L, "CurPos", ei.CurPos);
  PutNumToTable(L, "CurTabPos", ei.CurTabPos);
  PutNumToTable(L, "TopScreenLine", ei.TopScreenLine);
  PutNumToTable(L, "LeftPos", ei.LeftPos);
  PutBoolToTable(L, "Overtype", ei.Overtype);
  PutNumToTable(L, "BlockType", ei.BlockType);
  PutNumToTable(L, "BlockStartLine", ei.BlockStartLine);
  PutNumToTable(L, "Options", ei.Options);
  PutNumToTable(L, "TabSize", ei.TabSize);
  PutNumToTable(L, "BookMarkCount", ei.BookMarkCount);
  PutNumToTable(L, "CurState", ei.CurState);
  PutNumToTable(L, "CodePage", ei.CodePage);
  return 1;
}

/* t-rex:
 * Для тех кому плохо доходит описываю:
 * Редактор в фаре это двух связный список, указатель на текущюю строку
 * изменяется только при ECTL_SETPOSITION, при использовании любой другой
 * ECTL_* для которой нужно задавать номер строки если этот номер не -1
 * (т.е. текущаая строка) то фар должен найти эту строку в списке (а это
 * занимает дофига времени), поэтому если надо делать несколько ECTL_*
 * (тем более когда они делаются на последовательность строк
 * i,i+1,i+2,...) то перед каждым ECTL_* надо делать ECTL_SETPOSITION а
 * сами ECTL_* вызывать с -1.
 */
static BOOL FastGetString (int EditorId, int string_num,
  struct EditorGetString *egs, PSInfo *Info)
{
  struct EditorSetPosition esp;
  esp.CurLine   = string_num;
  esp.CurPos    = -1;
  esp.CurTabPos = -1;
  esp.TopScreenLine = -1;
  esp.LeftPos   = -1;
  esp.Overtype  = -1;
  if(!Info->EditorControl(EditorId, ECTL_SETPOSITION, 0, &esp))
    return FALSE;

  egs->StringNumber = string_num;
  return Info->EditorControl(EditorId, ECTL_GETSTRING, 0, egs);
}

// LineInfo = EditorGetString (EditorId, line_num, [mode])
//   line_num:  number of line in the Editor, 0-based; a number
//   mode:      0 = normal;
//              1 = much faster, but changes current position;
//              2 = the fastest: as 1 but returns StringText only;
//   LineInfo:  a table
static int _EditorGetString(lua_State *L, int is_wide)
{
  int EditorId = luaL_optinteger(L, 1, -1);
  PSInfo *Info = GetPluginData(L)->Info;
  int line_num = luaL_optinteger(L, 2, -1);
  int mode     = luaL_optinteger(L, 3, 0);
  BOOL res;
  struct EditorGetString egs;

  if (mode == 0) {
    egs.StringNumber = line_num;
    res = Info->EditorControl(EditorId, ECTL_GETSTRING, 0, &egs);
  }
  else
    res = FastGetString(EditorId, line_num, &egs, Info);

  if (res) {
    if (mode == 2) {
      if (is_wide) {
        push_utf16_string (L, egs.StringText, egs.StringLength);
        push_utf16_string (L, egs.StringEOL, -1);
      }
      else {
        push_utf8_string (L, egs.StringText, egs.StringLength);
        push_utf8_string (L, egs.StringEOL, -1);
      }
      return 2;
    }
    else {
      lua_createtable(L, 0, 6);
      PutNumToTable (L, "StringNumber", egs.StringNumber);
      PutNumToTable (L, "StringLength", egs.StringLength);
      PutNumToTable (L, "SelStart",     egs.SelStart);
      PutNumToTable (L, "SelEnd",       egs.SelEnd);
      if (is_wide) {
        push_utf16_string(L, egs.StringText, egs.StringLength);
        lua_setfield(L, -2, "StringText");
        push_utf16_string(L, egs.StringEOL, -1);
        lua_setfield(L, -2, "StringEOL");
      }
      else {
        PutWStrToTable (L, "StringText",  egs.StringText, egs.StringLength);
        PutWStrToTable (L, "StringEOL",   egs.StringEOL, -1);
      }
    }
    return 1;
  }
  return 0;
}

static int editor_GetString  (lua_State *L) { return _EditorGetString(L, 0); }
static int editor_GetStringW (lua_State *L) { return _EditorGetString(L, 1); }

static int _EditorSetString(lua_State *L, int is_wide)
{
  PSInfo *Info = GetPluginData(L)->Info;
  struct EditorSetString ess;
  int EditorId = luaL_optinteger(L, 1, -1);
  ess.StringNumber = luaL_optinteger(L, 2, -1);
  if (is_wide) {
    ess.StringText = check_utf16_string(L, 3, &ess.StringLength);
    ess.StringEOL = opt_utf16_string(L, 4, NULL);
    if (ess.StringEOL) {
      lua_pushvalue(L, 4);
      lua_pushliteral(L, "\0\0");
      lua_concat(L, 2);
      ess.StringEOL = (wchar_t*) lua_tostring(L, -1);
    }
  }
  else {
    ess.StringText = check_utf8_string(L, 3, &ess.StringLength);
    ess.StringEOL = opt_utf8_string(L, 4, NULL);
  }
  lua_pushboolean(L, Info->EditorControl(EditorId, ECTL_SETSTRING, 0, &ess));
  return 1;
}

static int editor_SetString  (lua_State *L) { return _EditorSetString(L, 0); }
static int editor_SetStringW (lua_State *L) { return _EditorSetString(L, 1); }

static int editor_InsertString(lua_State *L)
{
  PSInfo *Info = GetPluginData(L)->Info;
  int EditorId = luaL_optinteger(L, 1, -1);
  int indent = lua_toboolean(L, 2);
  lua_pushboolean(L, Info->EditorControl(EditorId, ECTL_INSERTSTRING, 0, &indent));
  return 1;
}

static int editor_DeleteString(lua_State *L)
{
  PSInfo *Info = GetPluginData(L)->Info;
  int EditorId = luaL_optinteger(L, 1, -1);
  lua_pushboolean(L, Info->EditorControl(EditorId, ECTL_DELETESTRING, 0, 0));
  return 1;
}

static int _EditorInsertText(lua_State *L, int is_wide)
{
  int EditorId = luaL_optinteger(L, 1, -1);
  PSInfo *Info = GetPluginData(L)->Info;
  const wchar_t* text = is_wide ? check_utf16_string(L,2,NULL) : check_utf8_string(L,2,NULL);
  lua_pushboolean(L, Info->EditorControl(EditorId, ECTL_INSERTTEXT, 0, (void*)text));
  return 1;
}

static int editor_InsertText  (lua_State *L) { return _EditorInsertText(L, 0); }
static int editor_InsertTextW (lua_State *L) { return _EditorInsertText(L, 1); }

static int editor_DeleteChar(lua_State *L)
{
  int EditorId = luaL_optinteger(L, 1, -1);
  PSInfo *Info = GetPluginData(L)->Info;
  lua_pushboolean(L, Info->EditorControl(EditorId, ECTL_DELETECHAR, 0, 0));
  return 1;
}

static int editor_DeleteBlock(lua_State *L)
{
  int EditorId = luaL_optinteger(L, 1, -1);
  PSInfo *Info = GetPluginData(L)->Info;
  lua_pushboolean(L, Info->EditorControl(EditorId, ECTL_DELETEBLOCK, 0, 0));
  return 1;
}

static int editor_UndoRedo(lua_State *L)
{
  int EditorId = luaL_optinteger(L, 1, -1);
  struct EditorUndoRedo eur;
  memset(&eur, 0, sizeof(eur));
  eur.Command = check_env_flag(L, 2);
  lua_pushboolean (L, GetPluginData(L)->Info->EditorControl(EditorId, ECTL_UNDOREDO, 0, &eur));
  return 1;
}

static void FillKeyBarTitles (lua_State *L, int src_pos, struct KeyBarTitles *kbt)
{
  int store=0, store_pos, size, i;
  lua_newtable(L);
  store_pos = lua_gettop(L);
  //-------------------------------------------------------------------------
  memset(kbt, 0, sizeof(*kbt));
  kbt->CountLabels = lua_objlen(L, src_pos);
  size = kbt->CountLabels * sizeof(struct KeyBarLabel);
  kbt->Labels = (struct KeyBarLabel*)lua_newuserdata(L, size);
  memset(kbt->Labels, 0, size);
  for (i=0; i < (int)kbt->CountLabels; i++) {
    lua_rawgeti(L, src_pos, i+1);
    if (!lua_istable(L, -1)) {
      kbt->CountLabels = i;
      lua_pop(L, 1);
      break;
    }
    kbt->Labels[i].Key.VirtualKeyCode = GetOptIntFromTable(L, "VirtualKeyCode", 0);
    kbt->Labels[i].Key.ControlKeyState = CAST(DWORD,CheckFlagsFromTable(L, -1, "ControlKeyState"));
    //-----------------------------------------------------------------------
    lua_getfield(L, -1, "Text");
    kbt->Labels[i].Text = StoreTempString(L, store_pos, &store);
    //-----------------------------------------------------------------------
    lua_getfield(L, -1, "LongText");
    kbt->Labels[i].LongText = StoreTempString(L, store_pos, &store);
    //-----------------------------------------------------------------------
    lua_pop(L, 1);
  }
}

static int SetKeyBar(lua_State *L, BOOL editor)
{
  int result;
  void* param;
  struct KeyBarTitles kbt;
  int Id = luaL_optinteger(L, 1, -1);
  PSInfo *Info = GetPluginData(L)->Info;

  enum { REDRAW=-1, RESTORE=0 }; // corresponds to FAR API
  BOOL argfail = FALSE;
  if (lua_isstring(L,2)) {
    const char* p = lua_tostring(L,2);
    if (0 == strcmp("redraw", p)) param = (void*)REDRAW;
    else if (0 == strcmp("restore", p)) param = (void*)RESTORE;
    else argfail = TRUE;
  }
  else if (lua_istable(L,2)) {
    param = &kbt;
    FillKeyBarTitles(L, 2, &kbt);
  }
  else
    argfail = TRUE;
  if (argfail)
    return luaL_argerror(L, 2, "must be 'redraw', 'restore', or table");

  result = editor ? Info->EditorControl(Id, ECTL_SETKEYBAR, 0, param) :
                    Info->ViewerControl(Id, VCTL_SETKEYBAR, 0, param);
  lua_pushboolean(L, result);
  return 1;
}

static int editor_SetKeyBar(lua_State *L)
{
  return SetKeyBar(L, TRUE);
}

static int viewer_SetKeyBar(lua_State *L)
{
  return SetKeyBar(L, FALSE);
}

static int editor_SetParam(lua_State *L)
{
  wchar_t buf[256];
  int tp, result;
  int EditorId = luaL_optinteger(L, 1, -1);
  PSInfo *Info = GetPluginData(L)->Info;
  struct EditorSetParameter esp;
  memset(&esp, 0, sizeof(esp));
  esp.Type = check_env_flag(L,2);
  //-----------------------------------------------------
  tp = lua_type(L,3);
  if (tp == LUA_TNUMBER)
    esp.Param.iParam = lua_tointeger(L,3);
  else if (tp == LUA_TBOOLEAN)
    esp.Param.iParam = lua_toboolean(L,3);
  else if (tp == LUA_TSTRING)
    esp.Param.wszParam = (wchar_t*)check_utf8_string(L,3,NULL);
  //-----------------------------------------------------
  if(esp.Type == ESPT_GETWORDDIV) {
    esp.Param.wszParam = buf;
    esp.Size = DIM(buf);
  }
  //-----------------------------------------------------
  esp.Flags = GetFlagCombination(L, 4, NULL);
  //-----------------------------------------------------
  result = Info->EditorControl(EditorId, ECTL_SETPARAM, 0, &esp);
  lua_pushboolean(L, result);
  if(result && esp.Type == ESPT_GETWORDDIV) {
    push_utf8_string(L,buf,-1); return 2;
  }
  return 1;
}

static int editor_SetPosition(lua_State *L)
{
  int EditorId = luaL_optinteger(L, 1, -1);
  PSInfo *Info = GetPluginData(L)->Info;
  struct EditorSetPosition esp;
  if (lua_istable(L, 2)) {
    lua_settop(L, 2);
    FillEditorSetPosition(L, &esp);
  }
  else {
    esp.CurLine   = luaL_optinteger(L, 2, -1);
    esp.CurPos    = luaL_optinteger(L, 3, -1);
    esp.CurTabPos = luaL_optinteger(L, 4, -1);
    esp.TopScreenLine = luaL_optinteger(L, 5, -1);
    esp.LeftPos   = luaL_optinteger(L, 6, -1);
    esp.Overtype  = luaL_optinteger(L, 7, -1);
  }
  if (Info->EditorControl(EditorId, ECTL_SETPOSITION, 0, &esp) != 0)
    return lua_pushboolean(L, 1), 1;
  return 0;
}

static int editor_Redraw(lua_State *L)
{
  int EditorId = luaL_optinteger(L, 1, -1);
  PSInfo *Info = GetPluginData(L)->Info;
  if (Info->EditorControl(EditorId, ECTL_REDRAW, 0, 0))
    return lua_pushboolean(L, 1), 1;
  return 0;
}

static int editor_ExpandTabs(lua_State *L)
{
  int EditorId = luaL_optinteger(L, 1, -1);
  PSInfo *Info = GetPluginData(L)->Info;
  int line_num = luaL_optinteger(L, 2, -1);
  if (Info->EditorControl(EditorId, ECTL_EXPANDTABS, 0, &line_num))
    return lua_pushboolean(L, 1), 1;
  return 0;
}

static int PushBookmarks(lua_State *L, int count, int command, int EditorId)
{
  struct EditorBookMarks ebm;
  int i;
  if (count <= 0)
    return 0;

  ebm.Line = (int*)lua_newuserdata(L, 4 * count * sizeof(int));
  ebm.Cursor     = ebm.Line + count;
  ebm.ScreenLine = ebm.Cursor + count;
  ebm.LeftPos    = ebm.ScreenLine + count;
  if (!GetPluginData(L)->Info->EditorControl(EditorId, command, 0, &ebm))
    return 0;

  lua_createtable(L, count, 0);
  for (i=0; i < count; i++) {
    lua_pushinteger(L, i+1);
    lua_createtable(L, 0, 4);
    PutIntToTable (L, "Line", ebm.Line[i]);
    PutIntToTable (L, "Cursor", ebm.Cursor[i]);
    PutIntToTable (L, "ScreenLine", ebm.ScreenLine[i]);
    PutIntToTable (L, "LeftPos", ebm.LeftPos[i]);
    lua_rawset(L, -3);
  }
  return 1;
}

static int editor_GetBookmarks(lua_State *L)
{
  struct EditorInfo ei;
  int EditorId = luaL_optinteger(L, 1, -1);
  PSInfo *Info = GetPluginData(L)->Info;
  if (!Info->EditorControl(EditorId, ECTL_GETINFO, 0, &ei))
    return 0;
  return PushBookmarks(L, ei.BookMarkCount, ECTL_GETBOOKMARKS, EditorId);
}

static int editor_GetSessionBookmarks(lua_State *L)
{
  int EditorId = luaL_optinteger(L, 1, -1);
  PSInfo *Info = GetPluginData(L)->Info;
  int count = Info->EditorControl(EditorId, ECTL_GETSESSIONBOOKMARKS, 0, 0);
  return PushBookmarks(L, count, ECTL_GETSESSIONBOOKMARKS, EditorId);
}

static int editor_AddSessionBookmark(lua_State *L)
{
  int EditorId = luaL_optinteger(L, 1, -1);
  PSInfo *Info = GetPluginData(L)->Info;
  lua_pushboolean(L, Info->EditorControl(EditorId, ECTL_ADDSESSIONBOOKMARK, 0, 0));
  return 1;
}

static int editor_ClearSessionBookmarks(lua_State *L)
{
  int EditorId = luaL_optinteger(L, 1, -1);
  PSInfo *Info = GetPluginData(L)->Info;
  lua_pushinteger(L, Info->EditorControl(EditorId, ECTL_CLEARSESSIONBOOKMARKS, 0, 0));
  return 1;
}

static int editor_DeleteSessionBookmark(lua_State *L)
{
  int EditorId = luaL_optinteger(L, 1, -1);
  INT_PTR num = luaL_optinteger(L, 2, -1);
  PSInfo *Info = GetPluginData(L)->Info;
  lua_pushboolean(L, Info->EditorControl(EditorId, ECTL_DELETESESSIONBOOKMARK, 0, (void*)num));
  return 1;
}

static int editor_NextSessionBookmark(lua_State *L)
{
  int EditorId = luaL_optinteger(L, 1, -1);
  PSInfo *Info = GetPluginData(L)->Info;
  lua_pushboolean(L, Info->EditorControl(EditorId, ECTL_NEXTSESSIONBOOKMARK, 0, 0));
  return 1;
}

static int editor_PrevSessionBookmark(lua_State *L)
{
  int EditorId = luaL_optinteger(L, 1, -1);
  PSInfo *Info = GetPluginData(L)->Info;
  lua_pushboolean(L, Info->EditorControl(EditorId, ECTL_PREVSESSIONBOOKMARK, 0, 0));
  return 1;
}

static int editor_SetTitle(lua_State *L)
{
  int EditorId = luaL_optinteger(L, 1, -1);
  PSInfo *Info = GetPluginData(L)->Info;
  const wchar_t* text = opt_utf8_string(L, 2, NULL);
  if (Info->EditorControl(EditorId, ECTL_SETTITLE, 0, (void*)text))
    return lua_pushboolean(L, 1), 1;
  return 0;
}

static int editor_Quit(lua_State *L)
{
  int EditorId = luaL_optinteger(L, 1, -1);
  PSInfo *Info = GetPluginData(L)->Info;
  if (Info->EditorControl(EditorId, ECTL_QUIT, 0, 0))
    return lua_pushboolean(L, 1), 1;
  return 0;
}

static int FillEditorSelect(lua_State *L, int pos_table, struct EditorSelect *es)
{
  int success;
  lua_getfield(L, pos_table, "BlockType");
  es->BlockType = CAST(int, get_env_flag(L, -1, &success));
  if (!success) {
    lua_pop(L,1);
    return 0;
  }
  lua_pushvalue(L, pos_table);
  es->BlockStartLine = GetOptIntFromTable(L, "BlockStartLine", -1);
  es->BlockStartPos  = GetOptIntFromTable(L, "BlockStartPos", -1);
  es->BlockWidth     = GetOptIntFromTable(L, "BlockWidth", -1);
  es->BlockHeight    = GetOptIntFromTable(L, "BlockHeight", -1);
  lua_pop(L,2);
  return 1;
}

static int editor_Select(lua_State *L)
{
  int EditorId = luaL_optinteger(L, 1, -1);
  PSInfo *Info = GetPluginData(L)->Info;
  struct EditorSelect es;
  if (lua_istable(L, 2)) {
    if (!FillEditorSelect(L, 2, &es))
      return 0;
  }
  else {
    int success;
    es.BlockType = CAST(int, get_env_flag(L, 2, &success));
    if (!success)
      return 0;
    es.BlockStartLine = luaL_optinteger(L, 3, -1);
    es.BlockStartPos  = luaL_optinteger(L, 4, -1);
    es.BlockWidth     = luaL_optinteger(L, 5, -1);
    es.BlockHeight    = luaL_optinteger(L, 6, -1);
  }
  if (Info->EditorControl(EditorId, ECTL_SELECT, 0, &es))
    return lua_pushboolean(L, 1), 1;
  return 0;
}

// This function is that long because FAR API does not supply needed
// information directly.
static int editor_GetSelection(lua_State *L)
{
  int BlockStartPos, h, from, to;
  int EditorId = luaL_optinteger(L, 1, -1);
  PSInfo *Info = GetPluginData(L)->Info;
  struct EditorInfo EI;
  struct EditorGetString egs;
  struct EditorSetPosition esp;

  Info->EditorControl(EditorId, ECTL_GETINFO, 0, &EI);
  if (EI.BlockType == BTYPE_NONE || !FastGetString(EditorId, EI.BlockStartLine, &egs, Info))
    return lua_pushnil(L), 1;

  lua_createtable (L, 0, 5);
  PutIntToTable (L, "BlockType", EI.BlockType);
  PutIntToTable (L, "StartLine", EI.BlockStartLine);

  BlockStartPos = egs.SelStart;
  PutIntToTable (L, "StartPos", BlockStartPos);

  // binary search for a non-block line
  h = 100; // arbitrary small number
  from = EI.BlockStartLine;
  for (to = from+h; to < EI.TotalLines; to = from + (h*=2)) {
    if(!FastGetString(EditorId, to, &egs, Info))
      return lua_pushnil(L), 1;
    if (egs.SelStart < 0 || egs.SelEnd == 0)
      break;
  }
  if (to >= EI.TotalLines)
    to = EI.TotalLines - 1;

  // binary search for the last block line
  while (from != to) {
    int curr = (from + to + 1) / 2;
    if(!FastGetString(EditorId, curr, &egs, Info))
      return lua_pushnil(L), 1;
    if (egs.SelStart < 0 || egs.SelEnd == 0) {
      if (curr == to)
        break;
      to = curr;      // curr was not selected
    }
    else {
      from = curr;    // curr was selected
    }
  }

  if(!FastGetString(EditorId, from, &egs, Info))
    return lua_pushnil(L), 1;

  PutIntToTable (L, "EndLine", from);
  PutIntToTable (L, "EndPos", egs.SelEnd);

  // restore current position, since FastGetString() changed it
  esp.CurLine       = EI.CurLine;
  esp.CurPos        = EI.CurPos;
  esp.CurTabPos     = EI.CurTabPos;
  esp.TopScreenLine = EI.TopScreenLine;
  esp.LeftPos       = EI.LeftPos;
  esp.Overtype      = EI.Overtype;
  Info->EditorControl(EditorId, ECTL_SETPOSITION, 0, &esp);
  return 1;
}

static int _EditorTabConvert(lua_State *L, int Operation)
{
  int EditorId = luaL_optinteger(L, 1, -1);
  PSInfo *Info = GetPluginData(L)->Info;
  struct EditorConvertPos ecp;
  ecp.StringNumber = luaL_optinteger(L, 2, -1);
  ecp.SrcPos = luaL_checkinteger(L, 3);
  if (Info->EditorControl(EditorId, Operation, 0, &ecp))
    return lua_pushinteger(L, ecp.DestPos), 1;
  return 0;
}

static int editor_TabToReal(lua_State *L)
{
  return _EditorTabConvert(L, ECTL_TABTOREAL);
}

static int editor_RealToTab(lua_State *L)
{
  return _EditorTabConvert(L, ECTL_REALTOTAB);
}

static int editor_TurnOffMarkingBlock(lua_State *L)
{
  int EditorId = luaL_optinteger(L, 1, -1);
  PSInfo *Info = GetPluginData(L)->Info;
  Info->EditorControl(EditorId, ECTL_TURNOFFMARKINGBLOCK, 0, 0);
  return 0;
}

static void GetFarColorFromTable (lua_State *L, int pos, struct FarColor* Color)
{
  lua_pushvalue(L, pos);
  Color->Flags  = CheckFlagsFromTable(L, -1, "Flags");
  Color->ForegroundColor = GetOptIntFromTable(L, "ForegroundColor", 0);
  Color->BackgroundColor = GetOptIntFromTable(L, "BackgroundColor", 0);
  lua_pop(L, 1);
}

static void PushFarColor (lua_State *L, const struct FarColor* Color)
{
  lua_createtable(L, 0, 3);
  PutFlagsToTable(L, "Flags", Color->Flags);
  PutIntToTable(L, "ForegroundColor", Color->ForegroundColor);
  PutIntToTable(L, "BackgroundColor", Color->BackgroundColor);
}

static int editor_AddColor(lua_State *L)
{
  TPluginData *pd = GetPluginData(L);
  int EditorId;
  struct EditorColor ec;
  memset(&ec, 0, sizeof(ec));
  ec.StructSize = sizeof(ec);
  ec.ColorItem = 0;

  EditorId        = luaL_optinteger(L, 1, -1);
  ec.StringNumber = luaL_optinteger(L, 2, -1);
  ec.StartPos     = luaL_checkinteger(L, 3);
  ec.EndPos       = luaL_checkinteger(L, 4);
  ec.Flags        = CheckFlags(L, 5);
  luaL_checktype(L, 6, LUA_TTABLE);
  GetFarColorFromTable(L, 6, &ec.Color);
  ec.Owner        = *pd->PluginId;
  ec.Priority     = CAST(unsigned, luaL_optnumber(L, 7, EDITOR_COLOR_NORMAL_PRIORITY));
  lua_pushboolean(L, pd->Info->EditorControl(EditorId, ECTL_ADDCOLOR, 0, &ec));
  return 1;
}

static int editor_DelColor(lua_State *L)
{
  TPluginData *pd = GetPluginData(L);
  int EditorId;
  struct EditorDeleteColor edc;
  edc.StructSize = sizeof(edc);
  edc.Owner = *pd->PluginId;

  EditorId         = luaL_optinteger(L, 1, -1);
  edc.StringNumber = luaL_optinteger(L, 2, -1);
  edc.StartPos     = luaL_checkinteger(L, 3);
  lua_pushboolean(L, pd->Info->EditorControl(EditorId, ECTL_DELCOLOR, 0, &edc));
  return 1;
}

static int editor_GetColor(lua_State *L)
{
  PSInfo *Info = GetPluginData(L)->Info;
  int EditorId;
  struct EditorColor ec;
  memset(&ec, 0, sizeof(ec));
  ec.StructSize = sizeof(ec);
  EditorId        = luaL_optinteger(L, 1, -1);
  ec.StringNumber = luaL_optinteger(L, 2, -1);
  ec.StartPos     = luaL_checkinteger(L, 3);
  ec.EndPos       = luaL_checkinteger(L, 4);
  ec.ColorItem    = luaL_checkinteger(L, 5);
  ec.Flags        = CheckFlags(L, 6);
  if (Info->EditorControl(EditorId, ECTL_GETCOLOR, 0, &ec))
    PushFarColor(L, &ec.Color);
  else
    lua_pushnil(L);
  return 1;
}

static int editor_SaveFile(lua_State *L)
{
  PSInfo *Info = GetPluginData(L)->Info;
  struct EditorSaveFile esf;
  int EditorId = luaL_optinteger(L, 1, -1);
  esf.FileName = opt_utf8_string(L, 2, L"");
  esf.FileEOL = opt_utf8_string(L, 3, NULL);
  if (Info->EditorControl(EditorId, ECTL_SAVEFILE, 0, &esf))
    return lua_pushboolean(L, 1), 1;
  return 0;
}

int pushInputRecord(lua_State *L, const INPUT_RECORD* ir)
{
  lua_newtable(L);
  PutIntToTable(L, "EventType", ir->EventType);
  switch(ir->EventType) {
    case KEY_EVENT:
    case FARMACRO_KEY_EVENT:
      PutBoolToTable(L,"KeyDown", ir->Event.KeyEvent.bKeyDown);
      PutNumToTable(L, "RepeatCount", ir->Event.KeyEvent.wRepeatCount);
      PutNumToTable(L, "VirtualKeyCode", ir->Event.KeyEvent.wVirtualKeyCode);
      PutNumToTable(L, "VirtualScanCode", ir->Event.KeyEvent.wVirtualScanCode);
      PutWStrToTable(L, "UnicodeChar", &ir->Event.KeyEvent.uChar.UnicodeChar, 1);
      PutNumToTable(L, "ControlKeyState", ir->Event.KeyEvent.dwControlKeyState);
      break;

    case MOUSE_EVENT:
      PutMouseEvent(L, &ir->Event.MouseEvent, TRUE);
      break;

    case WINDOW_BUFFER_SIZE_EVENT:
      PutNumToTable(L, "SizeX", ir->Event.WindowBufferSizeEvent.dwSize.X);
      PutNumToTable(L, "SizeY", ir->Event.WindowBufferSizeEvent.dwSize.Y);
      break;

    case MENU_EVENT:
      PutNumToTable(L, "CommandId", ir->Event.MenuEvent.dwCommandId);
      break;

    case FOCUS_EVENT:
      PutBoolToTable(L,"SetFocus", ir->Event.FocusEvent.bSetFocus);
      break;

    default:
      return 0;
  }
  return 1;
}

static int editor_ReadInput(lua_State *L)
{
  int EditorId = luaL_optinteger(L, 1, -1);
  PSInfo *Info = GetPluginData(L)->Info;
  INPUT_RECORD ir;
  if (!Info->EditorControl(EditorId, ECTL_READINPUT, 0, &ir))
    return 0;
  return pushInputRecord(L, &ir);
}

static void FillInputRecord(lua_State *L, int pos, INPUT_RECORD *ir)
{
  pos = abs_index(L, pos);
  luaL_checktype(L, pos, LUA_TTABLE);
  memset(ir, 0, sizeof(INPUT_RECORD));

  // determine event type
  lua_getfield(L, pos, "EventType");
  ir->EventType = CAST(WORD, check_env_flag(L, -1));
  lua_pop(L, 1);

  lua_pushvalue(L, pos);
  switch(ir->EventType) {
    case KEY_EVENT:
      ir->Event.KeyEvent.bKeyDown = GetOptBoolFromTable(L, "KeyDown", FALSE);
      ir->Event.KeyEvent.wRepeatCount = GetOptIntFromTable(L, "RepeatCount", 1);
      ir->Event.KeyEvent.wVirtualKeyCode = GetOptIntFromTable(L, "VirtualKeyCode", 0);
      ir->Event.KeyEvent.wVirtualScanCode = GetOptIntFromTable(L, "VirtualScanCode", 0);
      lua_getfield(L, -1, "UnicodeChar");
      ir->Event.KeyEvent.uChar.UnicodeChar = *opt_utf8_string (L, -1, L"");
      lua_pop(L, 1);
      ir->Event.KeyEvent.dwControlKeyState = GetOptIntFromTable(L, "ControlKeyState", 0);
      break;

    case MOUSE_EVENT:
      GetMouseEvent(L, &ir->Event.MouseEvent);
      break;

    case WINDOW_BUFFER_SIZE_EVENT:
      ir->Event.WindowBufferSizeEvent.dwSize.X = GetOptIntFromTable(L, "SizeX", 0);
      ir->Event.WindowBufferSizeEvent.dwSize.Y = GetOptIntFromTable(L, "SizeY", 0);
      break;

    case MENU_EVENT:
      ir->Event.MenuEvent.dwCommandId = GetOptIntFromTable(L, "CommandId", 0);
      break;

    case FOCUS_EVENT:
      ir->Event.FocusEvent.bSetFocus = GetOptBoolFromTable(L, "SetFocus", FALSE);
      break;
  }
  lua_pop(L, 1);
}

static void OptInputRecord (lua_State* L, TPluginData *pd, int pos, INPUT_RECORD* ir)
{
  if (lua_istable(L, pos))
    FillInputRecord(L, pos, ir);
  else if (lua_type(L, pos) == LUA_TSTRING) {
    wchar_t* name = check_utf8_string(L, pos, NULL);
    if (!pd->FSF->FarNameToInputRecord(name, ir))
      luaL_argerror(L, pos, "invalid key");
  }
  else {
    memset(ir, 0, sizeof(INPUT_RECORD));
    ir->EventType = KEY_EVENT;
  }
}

static int editor_ProcessInput(lua_State *L)
{
  INPUT_RECORD ir;
  int EditorId = luaL_optinteger(L, 1, -1);
  luaL_checktype(L, 2, LUA_TTABLE);
  FillInputRecord(L, 2, &ir);
  lua_pushboolean(L, GetPluginData(L)->Info->EditorControl(EditorId, ECTL_PROCESSINPUT, 0, &ir));
  return 1;
}

// Item, Position = Menu (Properties, Items [, Breakkeys])
// Parameters:
//   Properties -- a table
//   Items      -- an array of tables
//   BreakKeys  -- an array of strings with special syntax
// Return value:
//   Item:
//     a table  -- the table of selected item (or of breakkey) is returned
//     a nil    -- menu canceled by the user
//   Position:
//     a number -- position of selected menu item
//     a nil    -- menu canceled by the user
static int far_Menu(lua_State *L)
{
  TPluginData *pd = GetPluginData(L);
  int X = -1, Y = -1, MaxHeight = 0;
  UINT64 Flags = FMENU_WRAPMODE | FMENU_AUTOHIGHLIGHT;
  const wchar_t *Title = L"Menu", *Bottom = NULL, *HelpTopic = NULL;
  int SelectIndex = 0, store = 0, i, ItemsNumber;
  int BreakCode, *pBreakCode, NumBreakCodes;
  const GUID* MenuGuid = NULL;
  struct FarMenuItem *Items, *pItem;
  struct FarKey *pBreakKeys;
  int ret;

  lua_settop (L, 3);    // cut unneeded parameters; make stack predictable
  luaL_checktype(L, 1, LUA_TTABLE);
  luaL_checktype(L, 2, LUA_TTABLE);
  if (!lua_isnil(L,3) && !lua_istable(L,3))
    return luaL_argerror(L, 3, "must be table or nil");

  lua_newtable(L); // temporary store; at stack position 4

  // Properties
  lua_pushvalue (L,1);                //+1
  X = GetOptIntFromTable(L, "X", -1);
  Y = GetOptIntFromTable(L, "Y", -1);
  MaxHeight = GetOptIntFromTable(L, "MaxHeight", 0);
  lua_getfield(L, 1, "Flags");        //+2
  if (!lua_isnil(L, -1)) Flags = CheckFlags(L, -1);
  lua_getfield(L, 1, "Title");        //+3
  if(lua_isstring(L,-1))    Title = StoreTempString(L, 4, &store);
  lua_getfield(L, 1, "Bottom");       //+3
  if(lua_isstring(L,-1))    Bottom = StoreTempString(L, 4, &store);
  lua_getfield(L, 1, "HelpTopic");    //+3
  if(lua_isstring(L,-1))    HelpTopic = StoreTempString(L, 4, &store);
  lua_getfield(L, 1, "SelectIndex");  //+3
  SelectIndex = lua_tointeger(L,-1);
  lua_getfield(L, 1, "Id");           //+4
  if (lua_type(L,-1)==LUA_TSTRING && lua_objlen(L,-1)==sizeof(GUID))
    MenuGuid = (const GUID*)lua_tostring(L, -1);
  lua_pop(L, 4);

  // Items
  ItemsNumber = lua_objlen(L, 2);
  Items = (struct FarMenuItem*)lua_newuserdata(L, ItemsNumber*sizeof(struct FarMenuItem));
  memset(Items, 0, ItemsNumber*sizeof(struct FarMenuItem));
  pItem = Items;
  for(i=0; i < ItemsNumber; i++,pItem++,lua_pop(L,1)) {
    static const char key[] = "text";
    lua_pushinteger(L, i+1);
    lua_gettable(L, 2);
    if (!lua_istable(L, -1))
      return luaLF_SlotError (L, i+1, "table");
    //-------------------------------------------------------------------------
    lua_getfield(L, -1, key);
    if (lua_isstring(L,-1))  pItem->Text = StoreTempString(L, 4, &store);
    else if(!lua_isnil(L,-1)) return luaLF_FieldError (L, key, "string");
    if (!pItem->Text)
      lua_pop(L, 1);
    //-------------------------------------------------------------------------
    lua_getfield(L,-1,"checked");
    if (lua_type(L,-1) == LUA_TSTRING) {
      const wchar_t* s = utf8_to_utf16(L,-1,NULL);
      if (s) pItem->Flags |= s[0];
    }
    else if (lua_toboolean(L,-1)) pItem->Flags |= MIF_CHECKED;
    lua_pop(L,1);
    //-------------------------------------------------------------------------
    if (GetBoolFromTable(L, "separator")) pItem->Flags |= MIF_SEPARATOR;
    if (GetBoolFromTable(L, "disable"))   pItem->Flags |= MIF_DISABLE;
    if (GetBoolFromTable(L, "grayed"))    pItem->Flags |= MIF_GRAYED;
    if (GetBoolFromTable(L, "hidden"))    pItem->Flags |= MIF_HIDDEN;
    //-------------------------------------------------------------------------
    lua_getfield(L, -1, "AccelKey");
    if (lua_istable(L, -1)) {
      pItem->AccelKey.VirtualKeyCode = GetOptIntFromTable(L, "VirtualKeyCode", 0);
      pItem->AccelKey.ControlKeyState = GetOptIntFromTable(L, "ControlKeyState", 0);
    }
    lua_pop(L, 1);
  }
  if (SelectIndex > 0 && SelectIndex <= ItemsNumber)
    Items[SelectIndex-1].Flags |= MIF_SELECTED;

  // Break Keys
  pBreakKeys = NULL;
  pBreakCode = NULL;
  NumBreakCodes = lua_istable(L,3) ? lua_objlen(L,3) : 0;
  if (NumBreakCodes) {
    char buf[32];
    int ind;
    struct FarKey* BreakKeys = (struct FarKey*)lua_newuserdata(L, (1+NumBreakCodes)*sizeof(struct FarKey));
    // get virtualkeys table from the registry; push it on top
    lua_pushstring(L, FAR_VIRTUALKEYS);
    lua_rawget(L, LUA_REGISTRYINDEX);
    // push breakkeys table on top
    lua_pushvalue(L, 3);              // vk=-2; bk=-1;
    for(ind=0; ind < NumBreakCodes; ind++) {
      DWORD mod = 0;
      const char* s;
      char* vk;  // virtual key

      // get next break key (optional modifier plus virtual key)
      lua_pushinteger(L,ind+1);       // vk=-3; bk=-2;
      lua_gettable(L,-2);             // vk=-3; bk=-2; bki=-1;
      if(!lua_istable(L,-1)) { lua_pop(L,1); continue; }
      lua_getfield(L, -1, "BreakKey");// vk=-4; bk=-3;bki=-2;bknm=-1;
      if(!lua_isstring(L,-1)) { lua_pop(L,2); continue; }
      // separate modifier and virtual key strings
      s = lua_tostring(L,-1);
      if(strlen(s) >= sizeof(buf)) { lua_pop(L,2); continue; }
      strcpy(buf, s);
      vk = strchr(buf, '+');  // virtual key
      if (vk) {
        *vk++ = '\0';
        if(strchr(buf,'C')) mod |= LEFT_CTRL_PRESSED;
        if(strchr(buf,'A')) mod |= LEFT_ALT_PRESSED;
        if(strchr(buf,'S')) mod |= SHIFT_PRESSED;
        // replace on stack: break key name with virtual key name
        lua_pop(L, 1);
        lua_pushstring(L, vk);        // vk=-4; bk=-3;bki=-2;vknm=-1;
      }
      // get virtual key and break key values
      lua_rawget(L,-4);               // vk=-4; bk=-3;
      BreakKeys[ind].VirtualKeyCode = lua_tointeger(L,-1);
      BreakKeys[ind].ControlKeyState = mod;
      lua_pop(L,2);                   // vk=-2; bk=-1;
    }
    BreakKeys[ind].VirtualKeyCode = 0; // required by FAR API //TODO / SUSPICIOUS
    pBreakKeys = BreakKeys;
    pBreakCode = &BreakCode;
  }

  ret = pd->Info->Menu(pd->PluginId, MenuGuid, X, Y, MaxHeight, Flags, Title,
    Bottom, HelpTopic, pBreakKeys, pBreakCode, Items, ItemsNumber);

  if (NumBreakCodes && (BreakCode != -1)) {
    lua_pushinteger(L, BreakCode+1);
    lua_gettable(L, 3);
  }
  else if (ret == -1)
    return 0;
  else {
    lua_pushinteger(L, ret+1);
    lua_gettable(L, 2);
  }
  lua_pushinteger(L, ret+1);
  return 2;
}

// Return:   -1 if escape pressed, else - button number chosen (0 based).
int LF_Message(lua_State *L,
  const wchar_t* aMsg,      // if multiline, then lines must be separated by '\n'
  const wchar_t* aTitle,
  const wchar_t* aButtons,  // if multiple, then captions must be separated by ';'
  const char*    aFlags,
  const wchar_t* aHelpTopic,
  const GUID*    aMessageGuid)
{
  const wchar_t **items, **pItems;
  wchar_t** allocLines;
  int nAlloc;
  wchar_t *lastSpace, *lastDelim, *MsgCopy, *start, *pos;

  TPluginData *pd = GetPluginData(L);

  CONSOLE_SCREEN_BUFFER_INFO csbi;
  HANDLE hnd = GetStdHandle(STD_OUTPUT_HANDLE);
  int ret = GetConsoleScreenBufferInfo(hnd, &csbi);
  const int max_len   = ret ? csbi.srWindow.Right - csbi.srWindow.Left+1-14 : 66;
  const int max_lines = ret ? csbi.srWindow.Bottom - csbi.srWindow.Top+1-5 : 20;
  int num_lines = 0, num_buttons = 0;
  UINT64 Flags = 0;

  // Buttons
  wchar_t *BtnCopy = NULL, *ptr = NULL;
  if (*aButtons == L';') {
    const wchar_t* p = aButtons + 1;
    if      (!_wcsicmp(p, L"Ok"))               Flags = FMSG_MB_OK;
    else if (!_wcsicmp(p, L"OkCancel"))         Flags = FMSG_MB_OKCANCEL;
    else if (!_wcsicmp(p, L"AbortRetryIgnore")) Flags = FMSG_MB_ABORTRETRYIGNORE;
    else if (!_wcsicmp(p, L"YesNo"))            Flags = FMSG_MB_YESNO;
    else if (!_wcsicmp(p, L"YesNoCancel"))      Flags = FMSG_MB_YESNOCANCEL;
    else if (!_wcsicmp(p, L"RetryCancel"))      Flags = FMSG_MB_RETRYCANCEL;
  }
  else {
    // Buttons: 1-st pass, determining number of buttons
    BtnCopy = _wcsdup(aButtons);
    ptr = BtnCopy;
    while (*ptr && (num_buttons < 64)) {
      while (*ptr == L';')
        ptr++; // skip semicolons
      if (*ptr) {
        ++num_buttons;
        ptr = wcschr(ptr, L';');
        if (!ptr) break;
      }
    }
  }

  items = (const wchar_t**) malloc((1+max_lines+num_buttons) * sizeof(wchar_t*));
  allocLines = (wchar_t**) malloc(max_lines * sizeof(wchar_t*)); // array of pointers to allocated lines
  nAlloc = 0;                                                    // number of allocated lines
  pItems = items;

  // Title
  *pItems++ = aTitle;

  // Message lines
  lastSpace = lastDelim = NULL;
  MsgCopy = _wcsdup(aMsg);
  start = pos = MsgCopy;
  while (num_lines < max_lines) {
    if (*pos == 0) {                       // end of the entire message
      *pItems++ = start;
      ++num_lines;
      break;
    }
    else if (*pos == L'\n') {              // end of a message line
      *pItems++ = start;
      *pos = L'\0';
      ++num_lines;
      start = ++pos;
      lastSpace = lastDelim = NULL;
    }
    else if (pos-start < max_len) {         // characters inside the line
      if (*pos == L' ' || *pos == L'\t') lastSpace = pos;
      else if (!isalnum(*pos) && *pos != L'_') lastDelim = pos;
      pos++;
    }
    else {                                 // the 1-st character beyond the line
      int len;
      wchar_t **q;
      pos = lastSpace ? lastSpace+1 : lastDelim ? lastDelim+1 : pos;
      len = pos - start;
      q = &allocLines[nAlloc++]; // line allocation is needed
      *pItems++ = *q = (wchar_t*) malloc((len+1)*sizeof(wchar_t));
      wcsncpy(*q, start, len);
      (*q)[len] = L'\0';
      ++num_lines;
      start = pos;
      lastSpace = lastDelim = NULL;
    }
  }

  if (*aButtons != L';') {
    // Buttons: 2-nd pass.
    int i;
    ptr = BtnCopy;
    for (i=0; i < num_buttons; i++) {
      while (*ptr == L';')
        ++ptr;
      if (*ptr) {
        *pItems++ = ptr;
        ptr = wcschr(ptr, L';');
        if (ptr)
          *ptr++ = 0;
        else
          break;
      }
      else break;
    }
  }

  // Flags
  if (aFlags) {
    if(strchr(aFlags, 'w')) Flags |= FMSG_WARNING;
    if(strchr(aFlags, 'e')) Flags |= FMSG_ERRORTYPE;
    if(strchr(aFlags, 'k')) Flags |= FMSG_KEEPBACKGROUND;
    if(strchr(aFlags, 'l')) Flags |= FMSG_LEFTALIGN;
  }

  // Id
  if (aMessageGuid == NULL) aMessageGuid = pd->PluginId;

  ret = pd->Info->Message(pd->PluginId, aMessageGuid, Flags, aHelpTopic, items,
                          1+num_lines+num_buttons, num_buttons);
  free(BtnCopy);
  while(nAlloc) free(allocLines[--nAlloc]);
  free (allocLines);
  free(MsgCopy);
  free(CAST(void*, items));
  return ret;
}

void LF_Error(lua_State *L, const wchar_t* aMsg)
{
  PSInfo *Info = GetPluginData(L)->Info;
  if (Info == NULL)
    return;
  if (!aMsg) aMsg = L"<non-string error message>";
  lua_pushlstring(L, (const char*)Info->ModuleName, wcslen(Info->ModuleName)*sizeof(wchar_t));
  lua_pushlstring(L, (const char*)L":\n", 4);
  lua_pushlstring(L, (const char*)aMsg, wcslen(aMsg)*sizeof(wchar_t));
  lua_pushlstring(L, "\0\0", 2);
  lua_concat(L, 4);
  LF_Message(L, (const wchar_t*)lua_tostring(L,-1), L"Error", L"OK", "w", NULL, NULL);
  lua_pop(L, 1);
}

// 1-st param: message text (if multiline, then lines must be separated by '\n')
// 2-nd param: message title (if absent or nil, then "Message" is used)
// 3-rd param: buttons (if multiple, then captions must be separated by ';';
//             if absent or nil, then one button "OK" is used).
// 4-th param: flags
// 5-th param: help topic
// 6-th param: Id
// Return: -1 if escape pressed, else - button number chosen (0 based).
static int far_Message(lua_State *L)
{
  const wchar_t *Msg, *Title, *Buttons, *HelpTopic;
  const char *Flags;
  const GUID *Id;

  luaL_checkany(L,1);
  lua_settop(L,5);
  Msg = NULL;
  if (lua_isstring(L, 1))
    Msg = check_utf8_string(L, 1, NULL);
  else {
    lua_getglobal(L, "tostring");
    if (lua_isfunction(L,-1)) {
      lua_pushvalue(L,1);
      lua_call(L,1,1);
      Msg = check_utf8_string(L,-1,NULL);
    }
    if (Msg == NULL) luaL_argerror(L, 1, "cannot convert to string");
    lua_replace(L,1);
  }
  Title   = opt_utf8_string(L, 2, L"Message");
  Buttons = opt_utf8_string(L, 3, L";OK");
  Flags   = luaL_optstring(L, 4, "");
  HelpTopic = opt_utf8_string(L, 5, NULL);
  Id = (lua_type(L,6)==LUA_TSTRING && lua_objlen(L,6)==sizeof(GUID)) ?
    (const GUID*)lua_tostring(L,6) : NULL;
  lua_pushinteger(L, LF_Message(L, Msg, Title, Buttons, Flags, HelpTopic, Id));
  return 1;
}

static int panel_CheckPanelsExist(lua_State *L)
{
  PSInfo *Info = GetPluginData(L)->Info;
  HANDLE handle = OptHandle (L, 1);
  if (Info->PanelControl(handle, FCTL_CHECKPANELSEXIST, 0, 0))
    return lua_pushboolean(L, 1), 1;
  return 0;
}

static int panel_ClosePanel(lua_State *L)
{
  PSInfo *Info = GetPluginData(L)->Info;
  HANDLE handle = OptHandle (L, 1);
  const wchar_t *dir = opt_utf8_string(L, 2, L".");
  if (Info->PanelControl(handle, FCTL_CLOSEPANEL, 0, (void*)dir))
    return lua_pushboolean(L, 1), 1;
  return 0;
}

static int panel_GetPanelInfo(lua_State *L)
{
  PSInfo *Info = GetPluginData(L)->Info;
  HANDLE handle;
  struct PanelInfo pi;
  pi.StructSize = sizeof(pi);
  handle = OptHandle (L, 1);
  if (handle == INVALID_HANDLE_VALUE) {
    handle = (luaL_checkinteger(L,2) % 2) ? PANEL_ACTIVE:PANEL_PASSIVE;
  }
  if (!Info->PanelControl(handle, FCTL_GETPANELINFO, 0, &pi))
    return lua_pushnil(L), 1;

  lua_createtable(L, 0, 13);
  //-------------------------------------------------------------------------
  PutLStrToTable(L, "OwnerGuid", &pi.OwnerGuid, sizeof(GUID));
  PushPluginHandle(L, pi.PluginHandle);
  lua_setfield(L, -2, "PluginHandle");
  //-------------------------------------------------------------------------
  PutIntToTable(L, "PanelType", pi.PanelType);
  //-------------------------------------------------------------------------
  lua_createtable(L, 0, 4); // "PanelRect"
  PutIntToTable(L, "left", pi.PanelRect.left);
  PutIntToTable(L, "top", pi.PanelRect.top);
  PutIntToTable(L, "right", pi.PanelRect.right);
  PutIntToTable(L, "bottom", pi.PanelRect.bottom);
  lua_setfield(L, -2, "PanelRect");
  //-------------------------------------------------------------------------
  PutIntToTable(L, "ItemsNumber", pi.ItemsNumber);
  PutIntToTable(L, "SelectedItemsNumber", pi.SelectedItemsNumber);
  //-------------------------------------------------------------------------
  PutIntToTable(L, "CurrentItem", pi.CurrentItem + 1);
  PutIntToTable(L, "TopPanelItem", pi.TopPanelItem + 1);
  PutIntToTable(L, "ViewMode", pi.ViewMode);
  PutIntToTable(L, "SortMode", pi.SortMode);
  PutFlagsToTable(L, "Flags", pi.Flags);
  //-------------------------------------------------------------------------
  return 1;
}

static int get_panel_item(lua_State *L, int command)
{
  int index;
  struct FarGetPluginPanelItem fgppi = {0,0};
  PSInfo *Info = GetPluginData(L)->Info;
  HANDLE handle = OptHandle (L, 1);
  if (handle == INVALID_HANDLE_VALUE) {
    handle = (luaL_checkinteger(L,2) % 2) ? PANEL_ACTIVE:PANEL_PASSIVE;
  }
  index = luaL_optinteger(L,3,1) - 1;
  if (index < 0) index = 0;
  fgppi.Size = Info->PanelControl(handle, command, index, &fgppi);
  if (fgppi.Size) {
    fgppi.Item = (struct PluginPanelItem*)lua_newuserdata(L, fgppi.Size);
    if (Info->PanelControl(handle, command, index, &fgppi)) {
      PushPanelItem(L, fgppi.Item);
      return 1;
    }
  }
  return lua_pushnil(L), 1;
}

static int panel_GetPanelItem(lua_State *L) {
  return get_panel_item(L, FCTL_GETPANELITEM);
}

static int panel_GetSelectedPanelItem(lua_State *L) {
  return get_panel_item(L, FCTL_GETSELECTEDPANELITEM);
}

static int panel_GetCurrentPanelItem(lua_State *L) {
  return get_panel_item(L, FCTL_GETCURRENTPANELITEM);
}

static int get_string_info(lua_State *L, int command)
{
  int size;
  PSInfo *Info = GetPluginData(L)->Info;
  HANDLE handle = OptHandle (L, 1);
  if (handle == INVALID_HANDLE_VALUE) {
    handle = (luaL_checkinteger(L,2) % 2) ? PANEL_ACTIVE:PANEL_PASSIVE;
  }
  size = Info->PanelControl(handle, command, 0, 0);
  if (size) {
    wchar_t *buf = (wchar_t*)lua_newuserdata(L, size * sizeof(wchar_t));
    if (Info->PanelControl(handle, command, size, buf)) {
      push_utf8_string(L, buf, -1);
      return 1;
    }
  }
  return lua_pushnil(L), 1;
}

static int panel_GetPanelFormat(lua_State *L) {
  return get_string_info(L, FCTL_GETPANELFORMAT);
}

static int panel_GetPanelHostFile(lua_State *L) {
  return get_string_info(L, FCTL_GETPANELHOSTFILE);
}

static int panel_GetColumnTypes(lua_State *L) {
  return get_string_info(L, FCTL_GETCOLUMNTYPES);
}

static int panel_GetColumnWidths(lua_State *L) {
  return get_string_info(L, FCTL_GETCOLUMNWIDTHS);
}

static int panel_GetPanelPrefix(lua_State *L) {
  return get_string_info(L, FCTL_GETPANELPREFIX);
}

static int panel_RedrawPanel(lua_State *L)
{
  PSInfo *Info = GetPluginData(L)->Info;
  void *param2 = NULL;
  struct PanelRedrawInfo pri;
  HANDLE handle = OptHandle (L, 1);
  if (handle == INVALID_HANDLE_VALUE) {
    handle = (luaL_checkinteger(L,2) % 2) ? PANEL_ACTIVE:PANEL_PASSIVE;
  }
  if (lua_istable(L, 3)) {
    param2 = &pri;
    lua_getfield(L, 3, "CurrentItem");
    pri.CurrentItem = lua_tointeger(L, -1) - 1;
    lua_getfield(L, 3, "TopPanelItem");
    pri.TopPanelItem = lua_tointeger(L, -1) - 1;
  }
  lua_pushboolean(L, Info->PanelControl(handle, FCTL_REDRAWPANEL, 0, param2));
  return 1;
}

static int SetPanelBooleanProperty(lua_State *L, int command)
{
  int param1;
  PSInfo *Info = GetPluginData(L)->Info;
  HANDLE handle = OptHandle (L, 1);
  if (handle == INVALID_HANDLE_VALUE) {
    handle = (luaL_checkinteger(L,2) % 2) ? PANEL_ACTIVE:PANEL_PASSIVE;
  }
  param1 = lua_toboolean(L,3);
  lua_pushboolean(L, Info->PanelControl(handle, command, param1, 0));
  return 1;
}

static int SetPanelIntegerProperty(lua_State *L, int command)
{
  int param1;
  PSInfo *Info = GetPluginData(L)->Info;
  HANDLE handle = OptHandle (L, 1);
  if (handle == INVALID_HANDLE_VALUE) {
    handle = (luaL_checkinteger(L,2) % 2) ? PANEL_ACTIVE:PANEL_PASSIVE;
  }
  param1 = CAST(int, check_env_flag(L,3));
  lua_pushboolean(L, Info->PanelControl(handle, command, param1, 0));
  return 1;
}

static int panel_SetCaseSensitiveSort(lua_State *L) {
  return SetPanelBooleanProperty(L, FCTL_SETCASESENSITIVESORT);
}

static int panel_SetNumericSort(lua_State *L) {
  return SetPanelBooleanProperty(L, FCTL_SETNUMERICSORT);
}

static int panel_SetSortOrder(lua_State *L) {
  return SetPanelBooleanProperty(L, FCTL_SETSORTORDER);
}

static int panel_UpdatePanel(lua_State *L) {
  return SetPanelBooleanProperty(L, FCTL_UPDATEPANEL);
}

static int panel_SetSortMode(lua_State *L) {
  return SetPanelIntegerProperty(L, FCTL_SETSORTMODE);
}

static int panel_SetViewMode(lua_State *L) {
  return SetPanelIntegerProperty(L, FCTL_SETVIEWMODE);
}

static int panel_GetPanelDirectory(lua_State *L)
{
  int size;
  TPluginData *pd = GetPluginData(L);
  HANDLE handle = OptHandle (L, 1);
  if (handle == INVALID_HANDLE_VALUE) {
    handle = (luaL_checkinteger(L,2) % 2) ? PANEL_ACTIVE:PANEL_PASSIVE;
  }
  size = pd->Info->PanelControl(handle, FCTL_GETPANELDIRECTORY, 0, NULL);
  if (size) {
    struct FarPanelDirectory *fpd = (struct FarPanelDirectory*)lua_newuserdata(L, size);
    memset(fpd, 0, size);
    fpd->StructSize = sizeof(*fpd);
    if (pd->Info->PanelControl(handle, FCTL_GETPANELDIRECTORY, size, fpd)) {
      lua_createtable(L, 0, 4);
      PutWStrToTable(L, "Name",  fpd->Name, -1);
      PutWStrToTable(L, "Param", fpd->Param, -1);
      PutWStrToTable(L, "File",  fpd->File, -1);
      PutLStrToTable(L, "PluginId", &fpd->PluginId, sizeof(fpd->PluginId));
      return 1;
    }
  }
  return lua_pushnil(L), 1;
}

static int panel_SetPanelDirectory(lua_State *L)
{
  TPluginData *pd = GetPluginData(L);
  struct FarPanelDirectory fpd;
  HANDLE handle = OptHandle (L, 1);
  if (handle == INVALID_HANDLE_VALUE) {
    handle = (luaL_checkinteger(L,2) % 2) ? PANEL_ACTIVE:PANEL_PASSIVE;
  }
  memset(&fpd, 0, sizeof(fpd)); // also sets fpd.PluginId = FarId
  fpd.StructSize = sizeof(fpd);
  if (lua_istable(L, 3)) {
    size_t len;
    const GUID* id;
    lua_getfield(L, 3, "PluginId");
    id = (const GUID*)lua_tolstring(L, -1, &len);
    if (id && len == sizeof(GUID)) fpd.PluginId = *id;
    lua_getfield(L, 3, "Name");  if (lua_isstring(L, -1)) fpd.Name = check_utf8_string(L, -1, NULL);
    lua_getfield(L, 3, "Param"); if (lua_isstring(L, -1)) fpd.Param = check_utf8_string(L, -1, NULL);
    lua_getfield(L, 3, "File");  if (lua_isstring(L, -1)) fpd.File = check_utf8_string(L, -1, NULL);
  }
  else if (lua_isstring(L, 3))
    fpd.Name = check_utf8_string(L, 3, NULL);
  else
    luaL_argerror(L, 3, "table or string");
  lua_pushboolean(L, pd->Info->PanelControl(handle, FCTL_SETPANELDIRECTORY, 0, &fpd));
  return 1;
}

static int panel_GetCmdLine(lua_State *L)
{
  PSInfo *Info = GetPluginData(L)->Info;
  HANDLE handle = OptHandle (L, 1);
  int size = Info->PanelControl(handle, FCTL_GETCMDLINE, 0, 0);
  wchar_t *buf = (wchar_t*) malloc(size*sizeof(wchar_t));
  Info->PanelControl(handle, FCTL_GETCMDLINE, size, buf);
  push_utf8_string(L, buf, -1);
  free(buf);
  return 1;
}

static int panel_SetCmdLine(lua_State *L)
{
  PSInfo *Info = GetPluginData(L)->Info;
  HANDLE handle = OptHandle (L, 1);
  wchar_t* str = check_utf8_string(L, 2, NULL);
  lua_pushboolean(L, Info->PanelControl(handle, FCTL_SETCMDLINE, 0, str));
  return 1;
}

static int panel_GetCmdLinePos(lua_State *L)
{
  PSInfo *Info = GetPluginData(L)->Info;
  HANDLE handle = OptHandle (L, 1);
  int pos;
  Info->PanelControl(handle, FCTL_GETCMDLINEPOS, 0, &pos) ?
    lua_pushinteger(L, pos+1) : lua_pushnil(L);
  return 1;
}

static int panel_SetCmdLinePos(lua_State *L)
{
  PSInfo *Info = GetPluginData(L)->Info;
  HANDLE handle = OptHandle (L, 1);
  int pos = luaL_checkinteger(L, 2) - 1;
  int ret = Info->PanelControl(handle, FCTL_SETCMDLINEPOS, pos, 0);
  return lua_pushboolean(L, ret), 1;
}

static int panel_InsertCmdLine(lua_State *L)
{
  PSInfo *Info = GetPluginData(L)->Info;
  HANDLE handle = OptHandle (L, 1);
  wchar_t* str = check_utf8_string(L, 2, NULL);
  lua_pushboolean(L, Info->PanelControl(handle, FCTL_INSERTCMDLINE, 0, str));
  return 1;
}

static int panel_GetCmdLineSelection(lua_State *L)
{
  struct CmdLineSelect cms;
  PSInfo *Info = GetPluginData(L)->Info;
  HANDLE handle = OptHandle (L, 1);
  if (Info->PanelControl(handle, FCTL_GETCMDLINESELECTION, 0, &cms)) {
    if (cms.SelStart < 0) cms.SelStart = 0;
    if (cms.SelEnd < 0) cms.SelEnd = 0;
    lua_pushinteger(L, cms.SelStart + 1);
    lua_pushinteger(L, cms.SelEnd);
    return 2;
  }
  return lua_pushnil(L), 1;
}

static int panel_SetCmdLineSelection(lua_State *L)
{
  struct CmdLineSelect cms;
  PSInfo *Info = GetPluginData(L)->Info;
  HANDLE handle = OptHandle (L, 1);
  cms.SelStart = luaL_checkinteger(L, 2) - 1;
  cms.SelEnd = luaL_checkinteger(L, 3);
  if (cms.SelStart < -1) cms.SelStart = -1;
  if (cms.SelEnd < -1) cms.SelEnd = -1;
  lua_pushboolean(L, Info->PanelControl(handle, FCTL_SETCMDLINESELECTION, 0, &cms));
  return 1;
}

// CtrlSetSelection   (handle, whatpanel, items, selection)
// CtrlClearSelection (handle, whatpanel, items)
//   handle:       handle
//   whatpanel:    1=active_panel, 0=inactive_panel
//   items:        either number of an item, or a list of item numbers
//   selection:    boolean
static int ChangePanelSelection(lua_State *L, BOOL op_set)
{
  PSInfo *Info = GetPluginData(L)->Info;
  int itemindex = -1, numItems, command;
  INT_PTR state;
  struct PanelInfo pi;
  HANDLE handle = OptHandle (L, 1);
  if (handle == INVALID_HANDLE_VALUE) {
    handle = (luaL_checkinteger(L,2) % 2) ? PANEL_ACTIVE:PANEL_PASSIVE;
  }
  if (lua_isnumber(L,3)) {
    itemindex = lua_tointeger(L,3) - 1;
    if (itemindex < 0) return luaL_argerror(L, 3, "non-positive index");
  }
  else if (!lua_istable(L,3))
    return luaL_typerror(L, 3, "number or table");
  state = op_set ? lua_toboolean(L,4) : 0;

  // get panel info
  pi.StructSize = sizeof(pi);
  if (!Info->PanelControl(handle, FCTL_GETPANELINFO, 0, &pi) ||
     (pi.PanelType != PTYPE_FILEPANEL))
    return 0;
  //---------------------------------------------------------------------------
  numItems = op_set ? pi.ItemsNumber : pi.SelectedItemsNumber;
  command  = op_set ? FCTL_SETSELECTION : FCTL_CLEARSELECTION;
  Info->PanelControl(handle, FCTL_BEGINSELECTION, 0, 0);
  if (itemindex >= 0 && itemindex < numItems)
    Info->PanelControl(handle, command, itemindex, (void*)state);
  else {
    int i, len = lua_objlen(L,3);
    for (i=1; i<=len; i++) {
      lua_pushinteger(L, i);
      lua_gettable(L,3);
      if (lua_isnumber(L,-1)) {
        itemindex = lua_tointeger(L,-1) - 1;
        if (itemindex >= 0 && itemindex < numItems)
          Info->PanelControl(handle, command, itemindex, (void*)state);
      }
      lua_pop(L,1);
    }
  }
  Info->PanelControl(handle, FCTL_ENDSELECTION, 0, 0);
  //---------------------------------------------------------------------------
  return lua_pushboolean(L,1), 1;
}

static int panel_SetSelection(lua_State *L) {
  return ChangePanelSelection(L, TRUE);
}

static int panel_ClearSelection(lua_State *L) {
  return ChangePanelSelection(L, FALSE);
}

// CtrlSetUserScreen (handle)
//   handle:       FALSE=INVALID_HANDLE_VALUE, TRUE=lua_State*
static int panel_SetUserScreen(lua_State *L)
{
  PSInfo *Info = GetPluginData(L)->Info;
  HANDLE handle = OptHandle (L, 1);
  int ret = Info->PanelControl(handle, FCTL_SETUSERSCREEN, 0, 0);
  if(ret)
    return lua_pushboolean(L, 1), 1;
  return 0;
}

// CtrlGetUserScreen (handle)
//   handle:       FALSE=INVALID_HANDLE_VALUE, TRUE=lua_State*
static int panel_GetUserScreen(lua_State *L)
{
  PSInfo *Info = GetPluginData(L)->Info;
  HANDLE handle = OptHandle (L, 1);
  int ret = Info->PanelControl(handle, FCTL_GETUSERSCREEN, 0, 0);
  if(ret)
    return lua_pushboolean(L, 1), 1;
  return 0;
}

static int panel_IsActivePanel(lua_State *L)
{
  PSInfo *Info = GetPluginData(L)->Info;
  HANDLE handle = OptHandle (L, 1);
  return lua_pushboolean(L, Info->PanelControl(handle, FCTL_ISACTIVEPANEL, 0, 0)), 1;
}

// GetDirList (Dir)
//   Dir:     Name of the directory to scan (full pathname).
static int far_GetDirList (lua_State *L)
{
  PSInfo *Info = GetPluginData(L)->Info;
  const wchar_t *Dir = check_utf8_string (L, 1, NULL);
  struct PluginPanelItem *PanelItems;
  size_t ItemsNumber;
  int ret = Info->GetDirList (Dir, &PanelItems, &ItemsNumber);
  if(ret) {
    size_t i;
    lua_createtable(L, ItemsNumber, 0); // "PanelItems"
    for(i=0; i < ItemsNumber; i++) {
      lua_createtable(L, 0, 8);
      PutFarFindData (L, PanelItems + i);
      lua_rawseti(L, -2, i+1);
    }
    Info->FreeDirList (PanelItems, ItemsNumber);
    return 1;
  }
  return 0;
}

// GetPluginDirList (hPanel, Dir) //TODO: update manual
//   hPanel:          Current plugin instance handle.
//   Dir:             Name of the directory to scan (full pathname).
static int far_GetPluginDirList (lua_State *L)
{
  TPluginData *pd = GetPluginData(L);
  HANDLE handle = OptHandle (L, 1);
  const wchar_t *Dir = check_utf8_string (L, 2, NULL);
  struct PluginPanelItem *PanelItems;
  size_t ItemsNumber;
  int ret = pd->Info->GetPluginDirList (pd->PluginId, handle, Dir, &PanelItems, &ItemsNumber);
  if(ret) {
    PushPanelItems (L, PanelItems, ItemsNumber);
    pd->Info->FreePluginDirList (PanelItems, ItemsNumber);
    return 1;
  }
  return 0;
}

// RestoreScreen (handle)
//   handle:    handle of saved screen.
static int far_RestoreScreen (lua_State *L)
{
  if (lua_type(L,1) == LUA_TLIGHTUSERDATA) {
    PSInfo *Info = GetPluginData(L)->Info;
    Info->RestoreScreen ((HANDLE)lua_touserdata (L, 1));
    return lua_pushboolean(L, 1), 1;
  }
  return 0;
}

// handle = SaveScreen (X1,Y1,X2,Y2)
//   handle:    handle of saved screen, [lightuserdata]
static int far_SaveScreen (lua_State *L)
{
  PSInfo *Info = GetPluginData(L)->Info;
  int X1 = luaL_optinteger(L,1,0);
  int Y1 = luaL_optinteger(L,2,0);
  int X2 = luaL_optinteger(L,3,-1);
  int Y2 = luaL_optinteger(L,4,-1);
  void* handle = Info->SaveScreen(X1,Y1,X2,Y2);
  if (handle) {
    return lua_pushlightuserdata(L, handle), 1;
  }
  return 0;
}

static UINT64 GetDialogItemType(lua_State* L, int key, int item)
{
  int success;
  UINT64 iType;
  lua_pushinteger(L, key);
  lua_gettable(L, -2);
  iType = get_env_flag(L, -1, &success);
  if (!success) {
    const char* sType = lua_tostring(L, -1);
    return luaL_error(L, "%s - unsupported type in dialog item %d", sType, item);
  }
  lua_pop(L, 1);
  return iType;
}

// the table is on lua stack top
static UINT64 GetItemFlags(lua_State* L, int flag_index, int item_index)
{
  UINT64 flags;
  int success;
  lua_pushinteger(L, flag_index);
  lua_gettable(L, -2);
  flags = GetFlagCombination (L, -1, &success);
  if (!success)
    return luaL_error(L, "unsupported flag in dialog item %d", item_index);
  lua_pop(L, 1);
  return flags;
}

// list table is on Lua stack top
struct FarList* CreateList (lua_State *L, int historyindex)
{
  int i;
  struct FarList* list;
  int n = lua_objlen(L,-1);
  char* ptr = (char*)lua_newuserdata(L,
    sizeof(struct FarList) + n*sizeof(struct FarListItem)); // +2
  size_t len = lua_objlen(L, historyindex);
  lua_rawseti (L, historyindex, ++len); // +1; put into "histories" table to avoid being gc'ed
  list = (struct FarList*) ptr;
  list->ItemsNumber = n;
  list->Items = (struct FarListItem*)(ptr + sizeof(struct FarList));
  for (i=0; i<n; i++) {
    struct FarListItem *p = list->Items + i;
    lua_pushinteger(L, i+1); // +2
    lua_gettable(L,-2);      // +2
    if (lua_type(L,-1) != LUA_TTABLE)
      luaL_error (L, "value at index %d is not a table", i+1);
    p->Text = NULL;
    lua_getfield(L, -1, "Text"); // +3
    if (lua_isstring(L,-1)) {
      lua_pushvalue(L,-1);       // +4
      p->Text = check_utf8_string(L,-1,NULL); // +4
      lua_rawseti(L, historyindex, ++len);  // +3
    }
    lua_pop(L, 1);                 // +2
    p->Flags = CheckFlagsFromTable(L, -1, "Flags");
    lua_pop(L, 1);                 // +1
  }
  return list;
}

//	enum FARDIALOGITEMTYPES Type;            1
//	int X1,Y1,X2,Y2;                         2,3,4,5
//	union
//	{
//		DWORD_PTR Reserved;                    6
//		int Selected;                          6
//		struct FarList *ListItems;             6
//		FAR_CHAR_INFO *VBuf;                   6
//	}
//#ifndef __cplusplus
//	Param
//#endif
//	;
//	const wchar_t *History;                  7
//	const wchar_t *Mask;                     8
//	FARDIALOGITEMFLAGS Flags;                9
//	const wchar_t *Data;                     10
//	size_t MaxLength;                        11  // terminate 0 not included (if == 0 string size is unlimited)
//	LONG_PTR UserData;                       12


// item table is on Lua stack top
static void SetFarDialogItem(lua_State *L, struct FarDialogItem* Item, int itemindex,
  int historyindex)
{
  size_t len;
  memset(Item, 0, sizeof(struct FarDialogItem));
  Item->Type  = GetDialogItemType (L, 1, itemindex+1);
  Item->X1    = GetIntFromArray   (L, 2);
  Item->Y1    = GetIntFromArray   (L, 3);
  Item->X2    = GetIntFromArray   (L, 4);
  Item->Y2    = GetIntFromArray   (L, 5);
  Item->Flags = GetItemFlags      (L, 9, itemindex+1);

  if (Item->Type==DI_LISTBOX || Item->Type==DI_COMBOBOX) {
    int SelectIndex;
    lua_rawgeti(L, -1, 6);             // +1
    if (lua_type(L,-1) != LUA_TTABLE)
      luaLF_SlotError (L, 6, "table");
    Item->Param.ListItems = CreateList(L, historyindex);
    SelectIndex = GetOptIntFromTable(L, "SelectIndex", -1);
    if (SelectIndex > 0 && SelectIndex <= (int)lua_objlen(L,-1))
      Item->Param.ListItems->Items[SelectIndex-1].Flags |= LIF_SELECTED;
    lua_pop(L,1);                      // 0
  }
  else
    Item->Param.Selected = GetIntFromArray(L, 6);
  //---------------------------------------------------------------------------
  if (Item->Flags & DIF_HISTORY) {
    lua_rawgeti(L, -1, 7);                          // +1
    Item->History = opt_utf8_string (L, -1, NULL);  // +1
    len = lua_objlen(L, historyindex);
    lua_rawseti (L, historyindex, len+1); // +0; put into "histories" table to avoid being gc'ed
  }
  //---------------------------------------------------------------------------
  lua_rawgeti(L, -1, 8);                       // +1
  Item->Mask = opt_utf8_string (L, -1, NULL);  // +1
  len = lua_objlen(L, historyindex);
  lua_rawseti (L, historyindex, len+1); // +0; put into "histories" table to avoid being gc'ed
  //---------------------------------------------------------------------------
  Item->MaxLength = GetOptIntFromArray(L, 11, 0);
  lua_pushinteger(L, 10); // +1
  lua_gettable(L, -2);    // +1
  if (lua_isstring(L, -1)) {
    Item->Data = check_utf8_string (L, -1, NULL); // +1
    len = lua_objlen(L, historyindex);
    lua_rawseti (L, historyindex, len+1); // +0; put into "histories" table to avoid being gc'ed
  }
  else
    lua_pop(L, 1);
  //---------------------------------------------------------------------------
  lua_rawgeti(L, -1, 12);
  Item->UserData = lua_tointeger(L, -1);
  lua_pop(L, 1);
}

static void PushDlgItem (lua_State *L, const struct FarDialogItem* pItem, BOOL table_exist)
{
  if (! table_exist) {
    lua_createtable(L, 12, 0);
    if (pItem->Type == DI_LISTBOX || pItem->Type == DI_COMBOBOX) {
      lua_createtable(L, 0, 1);
      lua_rawseti(L, -2, 6);
    }
  }
  PutIntToArray  (L, 1, pItem->Type);
  PutIntToArray  (L, 2, pItem->X1);
  PutIntToArray  (L, 3, pItem->Y1);
  PutIntToArray  (L, 4, pItem->X2);
  PutIntToArray  (L, 5, pItem->Y2);
  PutIntToArray  (L, 6, pItem->Param.Selected);

  PutFlagsToArray(L, 9, pItem->Flags);
  lua_pushinteger(L, 10);
  push_utf8_string(L, pItem->Data, -1);
  lua_settable(L, -3);
  PutIntToArray  (L, 11, pItem->MaxLength);
  lua_pushinteger(L, 12);
  lua_pushinteger(L, pItem->UserData);
  lua_rawset(L, -3);
}

static void PushDlgItemNum (lua_State *L, HANDLE hDlg, int numitem, int pos_table,
  PSInfo *Info)
{
  struct FarGetDialogItem fgdi = {0,0};
  fgdi.Size = Info->SendDlgMessage(hDlg, DM_GETDLGITEM, numitem, &fgdi);
  if (fgdi.Size > 0) {
    BOOL table_exist;
    fgdi.Item = (struct FarDialogItem*) lua_newuserdata(L, fgdi.Size);
    Info->SendDlgMessage(hDlg, DM_GETDLGITEM, numitem, &fgdi);

    table_exist = lua_istable(L, pos_table);
    if (table_exist)
      lua_pushvalue(L, pos_table);
    PushDlgItem(L, fgdi.Item, table_exist);
    lua_remove(L, -2);
  }
  else
    lua_pushnil(L);
}

static int SetDlgItem (lua_State *L, HANDLE hDlg, int numitem, int pos_table,
  PSInfo *Info)
{
  struct FarDialogItem DialogItem;
  lua_newtable(L);
  lua_replace(L,1);
  luaL_checktype(L, pos_table, LUA_TTABLE);
  lua_pushvalue(L, pos_table);
  SetFarDialogItem(L, &DialogItem, numitem, 1);
  if (Info->SendDlgMessage(hDlg, DM_SETDLGITEM, numitem, &DialogItem))
    lua_pushboolean(L,1);
  else
    lua_pushboolean(L,0);
  return 1;
}

TDialogData* NewDialogData(lua_State* L, PSInfo *Info, HANDLE hDlg,
                           BOOL isOwned)
{
  TDialogData *dd = (TDialogData*) lua_newuserdata(L, sizeof(TDialogData));
  dd->L        = L;
  dd->Info     = Info;
  dd->hDlg     = hDlg;
  dd->isOwned  = isOwned;
  dd->wasError = FALSE;
  luaL_getmetatable(L, FarDialogType);
  lua_setmetatable(L, -2);
  if (isOwned) {
    lua_newtable(L);
    lua_setfenv(L, -2);
  }
  return dd;
}

TDialogData* CheckDialog(lua_State* L, int pos)
{
  return (TDialogData*)luaL_checkudata(L, pos, FarDialogType);
}

TDialogData* CheckValidDialog(lua_State* L, int pos)
{
  TDialogData* dd = CheckDialog(L, pos);
  luaL_argcheck(L, dd->hDlg != INVALID_HANDLE_VALUE, pos, "closed dialog");
  return dd;
}

HANDLE CheckDialogHandle (lua_State* L, int pos)
{
  return CheckValidDialog(L, pos)->hDlg;
}

static int far_SendDlgMessage (lua_State *L)
{
  PSInfo *Info = GetPluginData(L)->Info;
  INT_PTR res;
  int Msg, Param1, res_incr=0;
  void* Param2 = NULL;
  wchar_t buf[512];
  //---------------------------------------------------------------------------
  COORD                      coord;
  struct DialogInfo          dlg_info;
  struct EditorSelect        es;
  struct EditorSetPosition   esp;
  struct FarDialogItemData   fdid;
  struct FarListDelete       fld;
  struct FarListFind         flf;
  struct FarListGetItem      flgi;
  struct FarListInfo         fli;
  struct FarListInsert       flins;
  struct FarListPos          flp;
  struct FarListTitles       flt;
  struct FarListUpdate       flu;
  struct FarListItemData     flid;
  SMALL_RECT                 small_rect;
  //---------------------------------------------------------------------------
  HANDLE hDlg = CheckDialogHandle(L, 1);
  Msg = CAST(int, check_env_flag (L, 2));
  Param1 = luaL_optinteger(L, 3, 0);
  lua_settop(L, 4);

  switch(Msg) {
    default:
      luaL_argerror(L, 2, "operation not implemented");
      break;

    case DM_CLOSE:
    case DM_EDITUNCHANGEDFLAG:
    case DM_ENABLE:
    case DM_ENABLEREDRAW:
    case DM_GETCHECK:
    case DM_GETCOMBOBOXEVENT:
    case DM_GETCURSORSIZE:
    case DM_GETDLGDATA:
    case DM_GETDROPDOWNOPENED:
    case DM_GETFOCUS:
    case DM_GETITEMDATA:
    case DM_GETTEXTLENGTH:
    case DM_LISTGETDATASIZE:
    case DM_LISTSORT:
    case DM_REDRAW:               // alias: DM_SETREDRAW
    case DM_SET3STATE:
    case DM_SETCURSORSIZE:
    case DM_SETDLGDATA:
    case DM_SETDROPDOWNOPENED:
    case DM_SETFOCUS:
    case DM_SETITEMDATA:
    case DM_SETMAXTEXTLENGTH:     // alias: DM_SETTEXTLENGTH
    case DM_SETMOUSEEVENTNOTIFY:
    case DM_SHOWDIALOG:
    case DM_SHOWITEM:
    case DM_USER:
      Param2 = (void*)(LONG_PTR)luaL_optlong(L, 4, 0);
      break;

    case DM_LISTADDSTR: res_incr=1;
    case DM_ADDHISTORY:
    case DM_SETHISTORY:
    case DM_SETTEXTPTR:
      Param2 = (void*)opt_utf8_string(L, 4, NULL);
      break;

    case DM_SETCHECK:
      Param2 = (void*)(INT_PTR)get_env_flag (L, 4, NULL);
      break;

    case DM_GETCURSORPOS:
      if (Info->SendDlgMessage (hDlg, Msg, Param1, &coord)) {
        lua_createtable(L,0,2);
        PutNumToTable(L, "X", coord.X);
        PutNumToTable(L, "Y", coord.Y);
        return 1;
      }
      return lua_pushnil(L), 1;

    case DM_GETDIALOGINFO:
      dlg_info.StructSize = sizeof(dlg_info);
      if (Info->SendDlgMessage (hDlg, Msg, Param1, &dlg_info)) {
        lua_createtable(L,0,3);
        PutNumToTable(L, "StructSize", dlg_info.StructSize);
        PutLStrToTable(L, "Id", (const char*)&dlg_info.Id, sizeof(dlg_info.Id));
        PutLStrToTable(L, "Owner", (const char*)&dlg_info.Owner, sizeof(dlg_info.Owner));
        return 1;
      }
      return lua_pushnil(L), 1;

    case DM_GETDLGRECT:
    case DM_GETITEMPOSITION:
      if (Info->SendDlgMessage (hDlg, Msg, Param1, &small_rect)) {
        lua_createtable(L,0,4);
        PutNumToTable(L, "Left", small_rect.Left);
        PutNumToTable(L, "Top", small_rect.Top);
        PutNumToTable(L, "Right", small_rect.Right);
        PutNumToTable(L, "Bottom", small_rect.Bottom);
        return 1;
      }
      return lua_pushnil(L), 1;

    case DM_GETEDITPOSITION:
      if (Info->SendDlgMessage (hDlg, Msg, Param1, &esp))
        return PushEditorSetPosition(L, &esp), 1;
      return lua_pushnil(L), 1;

    case DM_GETSELECTION:
      if (Info->SendDlgMessage (hDlg, Msg, Param1, &es)) {
        lua_createtable(L,0,5);
        PutNumToTable(L, "BlockType", es.BlockType);
        PutNumToTable(L, "BlockStartLine", es.BlockStartLine);
        PutNumToTable(L, "BlockStartPos", es.BlockStartPos);
        PutNumToTable(L, "BlockWidth", es.BlockWidth);
        PutNumToTable(L, "BlockHeight", es.BlockHeight);
        return 1;
      }
      return lua_pushnil(L), 1;

    case DM_SETSELECTION:
      luaL_checktype(L, 4, LUA_TTABLE);
      if (FillEditorSelect(L, 4, &es)) {
        Param2 = &es;
        break;
      }
      return lua_pushinteger(L,0), 1;

    case DM_GETTEXT:
      fdid.PtrData = buf;
      fdid.PtrLength = sizeof(buf)/sizeof(buf[0]) - 1;
      Info->SendDlgMessage (hDlg, Msg, Param1, &fdid);
      push_utf8_string(L, fdid.PtrData, -1);
      return 1;

    case DM_GETCONSTTEXTPTR:
      push_utf8_string(L, (wchar_t*)Info->SendDlgMessage (hDlg, Msg, Param1, 0), -1);
      return 1;

    case DM_SETTEXT:
      fdid.PtrData = (wchar_t*)check_utf8_string(L, 4, NULL);
      fdid.PtrLength = 0; // wcslen(fdid.PtrData);
      Param2 = &fdid;
      break;

    case DM_KEY: { //TODO
      DWORD *arr;
      int i;
      luaL_checktype(L, 4, LUA_TTABLE);
      res = lua_objlen(L, 4);
      arr = (DWORD*)lua_newuserdata(L, res * sizeof(DWORD));
      for(i=0; i<res; i++) {
        lua_pushinteger(L,i+1);
        lua_gettable(L,4);
        arr[i] = lua_tointeger(L,-1);
        lua_pop(L,1);
      }
      res = Info->SendDlgMessage (hDlg, Msg, res, arr);
      return lua_pushinteger(L, res), 1;
    }

    case DM_LISTADD:
    case DM_LISTSET: {
      luaL_checktype(L, 4, LUA_TTABLE);
      lua_createtable(L,1,0); // "history table"
      lua_replace(L,1);
      lua_settop(L,4);
      Param2 = CreateList(L, 1);
      break;
    }

    case DM_LISTDELETE:
      luaL_checktype(L, 4, LUA_TTABLE);
      fld.StartIndex = GetOptIntFromTable(L, "StartIndex", 1) - 1;
      fld.Count = GetOptIntFromTable(L, "Count", 1);
      Param2 = &fld;
      break;

    case DM_LISTFINDSTRING:
      luaL_checktype(L, 4, LUA_TTABLE);
      flf.StartIndex = GetOptIntFromTable(L, "StartIndex", 1) - 1;
      lua_getfield(L, 4, "Pattern");
      flf.Pattern = check_utf8_string(L, -1, NULL);
      lua_getfield(L, 4, "Flags");
      flf.Flags = get_env_flag(L, -1, NULL);
      res = Info->SendDlgMessage (hDlg, Msg, Param1, &flf);
      res < 0 ? lua_pushnil(L) : lua_pushinteger (L, res+1);
      return 1;

    case DM_LISTGETCURPOS:
      Info->SendDlgMessage (hDlg, Msg, Param1, &flp);
      lua_createtable(L,0,2);
      PutIntToTable(L, "SelectPos", flp.SelectPos+1);
      PutIntToTable(L, "TopPos", flp.TopPos+1);
      return 1;

    case DM_LISTGETITEM:
      flgi.ItemIndex = luaL_checkinteger(L, 4) - 1;
      res = Info->SendDlgMessage (hDlg, Msg, Param1, &flgi);
      if (res) {
        lua_createtable(L,0,2);
        PutFlagsToTable(L, "Flags", flgi.Item.Flags);
        PutWStrToTable(L, "Text", flgi.Item.Text, -1);
        return 1;
      }
      return lua_pushnil(L), 1;

    case DM_LISTGETTITLES:
      flt.Title = buf;
      flt.Bottom = buf + sizeof(buf)/2;
      flt.TitleSize = sizeof(buf)/2;
      flt.BottomSize = sizeof(buf)/2;
      res = Info->SendDlgMessage (hDlg, Msg, Param1, &flt);
      if (res) {
        lua_createtable(L,0,2);
        PutWStrToTable(L, "Title", flt.Title, -1);
        PutWStrToTable(L, "Bottom", flt.Bottom, -1);
        return 1;
      }
      return lua_pushnil(L), 1;

    case DM_LISTSETTITLES:
      luaL_checktype(L, 4, LUA_TTABLE);
      lua_getfield(L, 4, "Title");
      flt.Title = lua_isstring(L,-1) ? check_utf8_string(L,-1,NULL) : NULL;
      lua_getfield(L, 4, "Bottom");
      flt.Bottom = lua_isstring(L,-1) ? check_utf8_string(L,-1,NULL) : NULL;
      Param2 = &flt;
      break;

    case DM_LISTINFO:
      res = Info->SendDlgMessage (hDlg, Msg, Param1, &fli);
      if (res) {
        lua_createtable(L,0,6);
        PutFlagsToTable(L, "Flags", fli.Flags);
        PutIntToTable(L, "ItemsNumber", fli.ItemsNumber);
        PutIntToTable(L, "SelectPos", fli.SelectPos+1);
        PutIntToTable(L, "TopPos", fli.TopPos+1);
        PutIntToTable(L, "MaxHeight", fli.MaxHeight);
        PutIntToTable(L, "MaxLength", fli.MaxLength);
        return 1;
      }
      return lua_pushnil(L), 1;

    case DM_LISTINSERT:
      luaL_checktype(L, 4, LUA_TTABLE);
      flins.Index = GetOptIntFromTable(L, "Index", 1) - 1;
      lua_getfield(L, 4, "Text");
      flins.Item.Text = lua_isstring(L,-1) ? check_utf8_string(L,-1,NULL) : NULL;
      flins.Item.Flags = CheckFlagsFromTable(L, 4, "Flags");
      res = Info->SendDlgMessage (hDlg, Msg, Param1, &flins);
      res < 0 ? lua_pushnil(L) : lua_pushinteger (L, res);
      return 1;

    case DM_LISTUPDATE:
      luaL_checktype(L, 4, LUA_TTABLE);
      flu.Index = GetOptIntFromTable(L, "Index", 1) - 1;
      lua_getfield(L, 4, "Text");
      flu.Item.Text = lua_isstring(L,-1) ? check_utf8_string(L,-1,NULL) : NULL;
      flu.Item.Flags = CheckFlagsFromTable(L, 4, "Flags");
      lua_pushboolean(L, Info->SendDlgMessage (hDlg, Msg, Param1, &flu));
      return 1;

    case DM_LISTSETCURPOS:
      luaL_checktype(L, 4, LUA_TTABLE);
      flp.SelectPos = GetOptIntFromTable(L, "SelectPos", 1) - 1;
      flp.TopPos = GetOptIntFromTable(L, "TopPos", 1) - 1;
      Param2 = &flp;
      break;

    case DM_LISTSETDATA:
    {
      int ref;
      memset(&flid, 0, sizeof(flid));
      luaL_checktype(L, 4, LUA_TTABLE);
      flid.Index = GetOptIntFromTable(L, "Index", 1) - 1;
      lua_getfenv(L, 1);
      lua_getfield(L, 4, "Data");
      ref = luaL_ref(L, -2);
      flid.Data = &ref;
      flid.DataSize = sizeof(int);
      Param2 = &flid;
      break;
    }

    case DM_LISTGETDATA:
      res = Info->SendDlgMessage (hDlg, Msg, Param1, (void*)(luaL_checkinteger(L, 4)-1));
      if (res) {
        lua_getfenv(L, 1);
        lua_rawgeti(L, -1, *(int*)res);
      }
      else lua_pushnil(L);
      return 1;

    case DM_GETDLGITEM:
      return PushDlgItemNum(L, hDlg, Param1, 4, Info), 1;

    case DM_SETDLGITEM:
      return SetDlgItem(L, hDlg, Param1, 4, Info);

    case DM_MOVEDIALOG:
    case DM_RESIZEDIALOG:
    case DM_SETCURSORPOS: {
      COORD *c;
      luaL_checktype(L, 4, LUA_TTABLE);
      coord.X = GetOptIntFromTable(L, "X", 0);
      coord.Y = GetOptIntFromTable(L, "Y", 0);
      Param2 = &coord;
      if (Msg == DM_SETCURSORPOS)
        break;
      c = (COORD*) Info->SendDlgMessage (hDlg, Msg, Param1, Param2);
      lua_createtable(L, 0, 2);
      PutIntToTable(L, "X", c->X);
      PutIntToTable(L, "Y", c->Y);
      return 1;
    }

    case DM_SETITEMPOSITION:
      luaL_checktype(L, 4, LUA_TTABLE);
      small_rect.Left = GetOptIntFromTable(L, "Left", 0);
      small_rect.Top = GetOptIntFromTable(L, "Top", 0);
      small_rect.Right = GetOptIntFromTable(L, "Right", 0);
      small_rect.Bottom = GetOptIntFromTable(L, "Bottom", 0);
      Param2 = &small_rect;
      break;

    case DM_SETCOMBOBOXEVENT:
      Param2 = (void*)(INT_PTR)CheckFlags(L, 4);
      break;

    case DM_SETEDITPOSITION:
      luaL_checktype(L, 4, LUA_TTABLE);
      lua_settop(L, 4);
      FillEditorSetPosition(L, &esp);
      Param2 = &esp;
      break;

    //~ case DM_GETTEXTPTR:
  }
  res = Info->SendDlgMessage (hDlg, Msg, Param1, Param2);
  lua_pushinteger (L, res + res_incr);
  return 1;
}

INT_PTR LF_DlgProc (lua_State *L, HANDLE hDlg, int Msg, int Param1, void *Param2)
{
  INT_PTR ret;
  TPluginData *pd = GetPluginData(L);
  PSInfo *Info = pd->Info;
  TDialogData *dd = (TDialogData*) Info->SendDlgMessage(hDlg,DM_GETDLGDATA,0,0);
  if (dd->wasError)
    return Info->DefDlgProc(hDlg, Msg, Param1, Param2);

  lua_pushlightuserdata (L, dd);       //+1   retrieve the table
  lua_rawget (L, LUA_REGISTRYINDEX);   //+1
  lua_rawgeti(L, -1, 2);               //+2   retrieve the procedure
  lua_rawgeti(L, -2, 3);               //+3   retrieve the handle
  lua_pushinteger (L, Msg);            //+4
  lua_pushinteger (L, Param1);         //+5

  if (Msg == DN_CTLCOLORDLGLIST || Msg == DN_CTLCOLORDLGITEM) {
    size_t i;
    struct FarDialogItemColors* fdic = (struct FarDialogItemColors*) Param2;
    lua_createtable(L, fdic->ColorsCount, 1);
    PutFlagsToTable(L, "Flags", fdic->Flags);
    for (i=0; i < fdic->ColorsCount; i++) {
      PushFarColor(L, &fdic->Colors[i]);
      lua_rawseti(L, -2, i+1);
    }
  }

  else if (Msg == DN_CTLCOLORDIALOG)
    PushFarColor(L, (struct FarColor*) Param2);

  else if (Msg == DN_DRAWDLGITEM)
    PushDlgItem (L, (struct FarDialogItem*)Param2, FALSE);

  else if (Msg == DN_EDITCHANGE)
    PushDlgItem (L, (struct FarDialogItem*)Param2, FALSE);

  else if (Msg == DN_HELP)
    push_utf8_string (L, Param2 ? (wchar_t*)Param2 : L"", -1);

  else if (Msg == DN_INITDIALOG)
    lua_pushnil(L);

  else if (Msg == DN_INPUT) // TODO was: (Msg == DN_MOUSEEVENT)
    pushInputRecord(L, (const INPUT_RECORD*)Param2);

  else if (Msg == DN_CONTROLINPUT)  // TODO
    pushInputRecord(L, (const INPUT_RECORD*)Param2);

  else if (Msg == DN_LISTCHANGE || Msg == DN_LISTHOTKEY)
    lua_pushinteger (L, (INT_PTR)Param2+1); // make list positions 1-based

  else if (Msg == DN_RESIZECONSOLE) {
    COORD* coord = (COORD*)Param2;
    lua_createtable(L, 0, 2);
    PutIntToTable(L, "X", coord->X);
    PutIntToTable(L, "Y", coord->Y);
  }

  else if (Msg == DN_GETVALUE) {
    struct FarGetValue *fgv = (struct FarGetValue*) Param2;
    lua_newtable(L);
    PutIntToTable(L, "GetType", fgv->Type);
    PutIntToTable(L, "ValType", fgv->Value.Type);
    if (fgv->Value.Type == FMVT_INTEGER)
      PutFlagsToTable(L, "Value", fgv->Value.Value.Integer);
    else if (fgv->Value.Type == FMVT_STRING)
      PutWStrToTable(L, "Value", fgv->Value.Value.String, -1);
    else if (fgv->Value.Type == FMVT_DOUBLE)
      PutNumToTable(L, "Value", fgv->Value.Value.Double);
  }

  else
    lua_pushinteger (L, (INT_PTR)Param2); //+6

  //---------------------------------------------------------------------------
  ret = pcall_msg (L, 4, 1); //+2
  if (ret) {
    lua_pop(L, 1);
    dd->wasError = TRUE;
    Info->SendDlgMessage(hDlg, DM_CLOSE, -1, 0);
    return Info->DefDlgProc(hDlg, Msg, Param1, Param2);
  }
  //---------------------------------------------------------------------------

  if (lua_isnil(L, -1))
    ret = Info->DefDlgProc(hDlg, Msg, Param1, Param2);

  else if (Msg == DN_CTLCOLORDLGLIST || Msg == DN_CTLCOLORDLGITEM) {
    if ((ret = lua_istable(L,-1)) != 0) {
      struct FarDialogItemColors* fdic = (struct FarDialogItemColors*) Param2;
      size_t i;
      size_t len = lua_objlen(L, -1);
      if (len > fdic->ColorsCount) len = fdic->ColorsCount;
      for (i = 0; i < len; i++) {
        lua_rawgeti(L, -1, i+1);
        if (lua_istable(L, -1))
          GetFarColorFromTable(L, -1, &fdic->Colors[i]);
        lua_pop(L, 1);
      }
    }
  }

  else if (Msg == DN_CTLCOLORDIALOG) {
    if ((ret = lua_istable(L,-1)) != 0)
      GetFarColorFromTable(L, -1, (struct FarColor*)Param2);
  }

  else if (Msg == DN_HELP) {
    if ((ret = (INT_PTR)utf8_to_utf16(L, -1, NULL)) != 0) {
      lua_pushvalue(L, -1);                // keep stack balanced
      lua_setfield(L, -3, "helpstring");   // protect from garbage collector
    }
  }

  else if (Msg == DN_GETVALUE) {
    if ((ret = lua_istable(L,-1)) != 0) {
      struct FarGetValue *fgv = (struct FarGetValue*) Param2;
      fgv->Value.Type = GetOptIntFromTable(L, "ValType", FMVT_UNKNOWN);
      lua_getfield(L, -1, "Value");
      if (fgv->Value.Type == FMVT_INTEGER)
        fgv->Value.Value.Integer = get_env_flag (L, -1, NULL);
      else if (fgv->Value.Type == FMVT_STRING) {
        if ((fgv->Value.Value.String = utf8_to_utf16(L, -1, NULL)) != 0) {
          lua_pushvalue(L, -1);                   // keep stack balanced
          lua_setfield(L, -4, "getvaluestring");  // protect from garbage collector
        }
        else ret = 0;
      }
      else if (fgv->Value.Type == FMVT_DOUBLE)
        fgv->Value.Value.Double = lua_tonumber(L, -1);
      else
        ret = 0;
      lua_pop(L, 1);
    }
  }

  else if (lua_isnumber(L, -1))
    ret = lua_tointeger (L, -1);
  else
    ret = lua_toboolean(L, -1);

  lua_pop (L, 2);
  return ret;
}

static int far_DialogInit(lua_State *L)
{
  int ItemsNumber, i;
  struct FarDialogItem *Items;
  UINT64 Flags;
  TDialogData *dd;
  FARAPIDEFDLGPROC Proc;
  void *Param;

  TPluginData *pd = GetPluginData(L);

  GUID Id = *(GUID*)luaL_checkstring(L, 1);
  int X1 = luaL_checkinteger(L, 2);
  int Y1 = luaL_checkinteger(L, 3);
  int X2 = luaL_checkinteger(L, 4);
  int Y2 = luaL_checkinteger(L, 5);
  const wchar_t *HelpTopic = opt_utf8_string(L, 6, NULL);
  luaL_checktype(L, 7, LUA_TTABLE);

  lua_newtable (L); // create a "histories" table, to prevent history strings
                    // from being garbage collected too early
  lua_replace (L, 1);

  ItemsNumber = lua_objlen(L, 7);
  Items = (struct FarDialogItem*)lua_newuserdata (L, ItemsNumber * sizeof(struct FarDialogItem));
  lua_replace (L, 2);
  for(i=0; i < ItemsNumber; i++) {
    int type;
    lua_pushinteger(L, i+1);
    lua_gettable(L, 7);
    type = lua_type(L, -1);
    if (type == LUA_TTABLE) {
      SetFarDialogItem(L, Items+i, i, 1);
    }
    lua_pop(L, 1);
    if(type == LUA_TNIL)
      break;
    if(type != LUA_TTABLE)
      return luaL_error(L, "Items[%d] is not a table", i+1);
  }

  // 8-th parameter (flags)
  Flags = CheckFlags(L, 8);

  dd = NewDialogData(L, pd->Info, INVALID_HANDLE_VALUE, TRUE);

  // 9-th parameter (DlgProc function)
  Proc = NULL;
  Param = NULL;
  if (lua_isfunction(L, 9)) {
    Proc = pd->DlgProc;
    Param = dd;
  }

  dd->hDlg = pd->Info->DialogInit(pd->PluginId, &Id, X1, Y1, X2, Y2, HelpTopic,
                                   Items, ItemsNumber, 0, Flags, Proc, Param);

  if (dd->hDlg != INVALID_HANDLE_VALUE) {
    // Put some values into the registry
    lua_pushlightuserdata(L, dd); // important: index it with dd
    lua_createtable(L, 3, 0);
    lua_pushvalue (L, 1);     // store the "histories" table
    lua_rawseti(L, -2, 1);
    if (lua_isfunction(L, 9)) {
      lua_pushvalue (L, 9);   // store the procedure
      lua_rawseti(L, -2, 2);
      lua_pushvalue (L, -3);  // store the handle
      lua_rawseti(L, -2, 3);
    }
    lua_rawset (L, LUA_REGISTRYINDEX);
  }
  else
    lua_pushnil(L);
  return 1;
}

static void free_dialog (TDialogData* dd)
{
  lua_State* L = dd->L;
  if (dd->isOwned && dd->hDlg != INVALID_HANDLE_VALUE) {
    dd->Info->DialogFree(dd->hDlg);
    dd->hDlg = INVALID_HANDLE_VALUE;
    lua_pushlightuserdata(L, dd);
    lua_pushnil (L);
    lua_rawset (L, LUA_REGISTRYINDEX);
  }
}

static int far_DialogRun (lua_State *L)
{
  TDialogData* dd = CheckValidDialog(L, 1);
  int result = dd->Info->DialogRun(dd->hDlg);
  if (dd->wasError) {
    free_dialog(dd);
    luaL_error(L, "error occured in dialog procedure");
  }
  lua_pushinteger(L, result);
  return 1;
}

static int far_DialogFree (lua_State *L)
{
  free_dialog(CheckDialog(L, 1));
  return 0;
}

static int dialog_tostring (lua_State *L)
{
  TDialogData* dd = CheckDialog(L, 1);
  if (dd->hDlg != INVALID_HANDLE_VALUE)
    lua_pushfstring(L, "%s (%p)", FarDialogType, dd->hDlg);
  else
    lua_pushfstring(L, "%s (closed)", FarDialogType);
  return 1;
}

static int far_DefDlgProc(lua_State *L)
{
  int Msg, Param1;
  INT_PTR Param2;
  HANDLE hDlg;

  luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
  hDlg = lua_touserdata(L, 1);
  Msg = CAST(int, get_env_flag(L, 2, NULL));
  Param1 = luaL_checkinteger(L, 3);
  Param2 = luaL_checkinteger(L, 4);

  lua_pushinteger(L, GetPluginData(L)->Info->DefDlgProc(hDlg, Msg, Param1, (void*)Param2));
  return 1;
}

static int far_GetDlgItem(lua_State *L)
{
  PSInfo *Info = GetPluginData(L)->Info;
  HANDLE hDlg = CheckDialogHandle(L,1);
  int numitem = luaL_checkinteger(L,2);
  PushDlgItemNum(L, hDlg, numitem, 3, Info);
  return 1;
}

static int far_SetDlgItem(lua_State *L)
{
  PSInfo *Info = GetPluginData(L)->Info;
  HANDLE hDlg = CheckDialogHandle(L,1);
  int numitem = luaL_checkinteger(L,2);
  return SetDlgItem(L, hDlg, numitem, 3, Info);
}

static int editor_Editor(lua_State *L)
{
  PSInfo *Info = GetPluginData(L)->Info;
  const wchar_t* FileName = check_utf8_string(L, 1, NULL);
  const wchar_t* Title    = opt_utf8_string(L, 2, FileName);
  int X1 = luaL_optinteger(L, 3, 0);
  int Y1 = luaL_optinteger(L, 4, 0);
  int X2 = luaL_optinteger(L, 5, -1);
  int Y2 = luaL_optinteger(L, 6, -1);
  UINT64 Flags = CheckFlags(L,7);
  int StartLine = luaL_optinteger(L, 8, -1);
  int StartChar = luaL_optinteger(L, 9, -1);
  int CodePage  = luaL_optinteger(L, 10, CP_AUTODETECT);
  int ret = Info->Editor(FileName, Title, X1, Y1, X2, Y2, Flags,
                         StartLine, StartChar, CodePage);
  lua_pushinteger(L, ret);
  return 1;
}

static int viewer_Viewer(lua_State *L)
{
  PSInfo *Info = GetPluginData(L)->Info;
  const wchar_t* FileName = check_utf8_string(L, 1, NULL);
  const wchar_t* Title    = opt_utf8_string(L, 2, FileName);
  int X1 = luaL_optinteger(L, 3, 0);
  int Y1 = luaL_optinteger(L, 4, 0);
  int X2 = luaL_optinteger(L, 5, -1);
  int Y2 = luaL_optinteger(L, 6, -1);
  UINT64 Flags = CheckFlags(L, 7);
  int CodePage = luaL_optinteger(L, 8, CP_AUTODETECT);
  int ret = Info->Viewer(FileName, Title, X1, Y1, X2, Y2, Flags, CodePage);
  lua_pushboolean(L, ret);
  return 1;
}

static int viewer_GetInfo(lua_State *L)
{
  int ViewerId = luaL_optinteger(L, 1, -1);
  PSInfo *Info = GetPluginData(L)->Info;
  struct ViewerInfo vi;
  vi.StructSize = sizeof(vi);
  if (!Info->ViewerControl(ViewerId, VCTL_GETINFO, 0, &vi))
    return 0;
  lua_createtable(L, 0, 10);
  PutNumToTable(L,  "ViewerID",    vi.ViewerID);
  PutWStrToTable(L, "FileName",    vi.FileName, -1);
  PutNumToTable(L,  "FileSize",    CAST(double, vi.FileSize));
  PutNumToTable(L,  "FilePos",     CAST(double, vi.FilePos));
  PutNumToTable(L,  "WindowSizeX", vi.WindowSizeX);
  PutNumToTable(L,  "WindowSizeY", vi.WindowSizeY);
  PutNumToTable(L,  "Options",     CAST(double, vi.Options));
  PutNumToTable(L,  "TabSize",     vi.TabSize);
  PutNumToTable(L,  "LeftPos",     CAST(double, vi.LeftPos));
  lua_createtable(L, 0, 4);
  PutNumToTable (L, "CodePage",    vi.CurMode.CodePage);
  PutBoolToTable(L, "Wrap",        vi.CurMode.Wrap);
  PutNumToTable (L, "WordWrap",    vi.CurMode.WordWrap);
  PutBoolToTable(L, "Hex",         vi.CurMode.Hex);
  lua_setfield(L, -2, "CurMode");
  return 1;
}

static int viewer_Quit(lua_State *L)
{
  int ViewerId = luaL_optinteger(L, 1, -1);
  PSInfo *Info = GetPluginData(L)->Info;
  Info->ViewerControl(ViewerId, VCTL_QUIT, 0, 0);
  return 0;
}

static int viewer_Redraw(lua_State *L)
{
  int ViewerId = luaL_optinteger(L, 1, -1);
  PSInfo *Info = GetPluginData(L)->Info;
  Info->ViewerControl(ViewerId, VCTL_REDRAW, 0, 0);
  return 0;
}

static int viewer_Select(lua_State *L)
{
  PSInfo *Info = GetPluginData(L)->Info;
  struct ViewerSelect vs;
  int ViewerId = luaL_optinteger(L, 1, -1);
  vs.BlockStartPos = (long long int)luaL_checknumber(L,2);
  vs.BlockLen = luaL_checkinteger(L,3);
  lua_pushboolean(L, Info->ViewerControl(ViewerId, VCTL_SELECT, 0, &vs));
  return 1;
}

static int viewer_SetPosition(lua_State *L)
{
  int ViewerId = luaL_optinteger(L, 1, -1);
  PSInfo *Info = GetPluginData(L)->Info;
  struct ViewerSetPosition vsp;
  if (lua_istable(L, 2)) {
    lua_settop(L, 2);
    vsp.StartPos = (__int64)GetOptNumFromTable(L, "StartPos", 0);
    vsp.LeftPos = (__int64)GetOptNumFromTable(L, "LeftPos", 0);
    vsp.Flags = CheckFlagsFromTable(L, -1, "Flags");
  }
  else {
    vsp.StartPos = (__int64)luaL_optnumber(L,2,0);
    vsp.LeftPos = (__int64)luaL_optnumber(L,3,0);
    vsp.Flags = CheckFlags(L,4);
  }
  if (Info->ViewerControl(ViewerId, VCTL_SETPOSITION, 0, &vsp))
    lua_pushnumber(L, (double)vsp.StartPos);
  else
    lua_pushnil(L);
  return 1;
}

static int viewer_SetMode(lua_State *L)
{
  int success, ViewerId;
  struct ViewerSetMode vsm;
  memset(&vsm, 0, sizeof(struct ViewerSetMode));
  ViewerId = luaL_optinteger(L, 1, -1);
  luaL_checktype(L, 2, LUA_TTABLE);

  lua_getfield(L, 2, "Type");
  vsm.Type = get_env_flag(L, -1, &success);
  if (!success)
    return lua_pushboolean(L,0), 1;

  lua_getfield(L, 2, "iParam");
  if (lua_isnumber(L, -1))
    vsm.Param.iParam = lua_tointeger(L, -1);
  else
    return lua_pushboolean(L,0), 1;

  lua_getfield(L, 2, "Flags");
  vsm.Flags = get_env_flag(L, -1, &success);
  if (!success)
    return lua_pushboolean(L,0), 1;

  lua_pushboolean(L, GetPluginData(L)->Info->ViewerControl(ViewerId, VCTL_SETMODE, 0, &vsm));
  return 1;
}

static int far_ShowHelp(lua_State *L)
{
  const wchar_t *ModuleName = check_utf8_string (L,1,NULL);
  const wchar_t *HelpTopic = opt_utf8_string (L,2,NULL);
  UINT64 Flags = CheckFlags(L,3);
  PSInfo *Info = GetPluginData(L)->Info;
  BOOL ret = Info->ShowHelp (ModuleName, HelpTopic, Flags);
  return lua_pushboolean(L, ret), 1;
}

// DestText = far.InputBox(Title,Prompt,HistoryName,SrcText,DestLength,HelpTopic,Flags)
// all arguments are optional
static int far_InputBox(lua_State *L)
{
  TPluginData *pd = GetPluginData(L);
  const GUID *Id = (lua_type(L,1)==LUA_TSTRING && lua_objlen(L,1)==sizeof(GUID)) ?
    (const GUID*)lua_tostring(L, 1) : pd->PluginId;
  const wchar_t *Title       = opt_utf8_string (L, 2, L"Input Box");
  const wchar_t *Prompt      = opt_utf8_string (L, 3, L"Enter the text:");
  const wchar_t *HistoryName = opt_utf8_string (L, 4, NULL);
  const wchar_t *SrcText     = opt_utf8_string (L, 5, L"");
  int DestLength             = luaL_optinteger (L, 6, 1024);
  const wchar_t *HelpTopic   = opt_utf8_string (L, 7, NULL);
  UINT64 Flags = OptFlags(L, 8, FIB_ENABLEEMPTY|FIB_BUTTONS|FIB_NOAMPERSAND);
  wchar_t *DestText;
  int res;

  if (DestLength < 0) DestLength = 0;
  DestText = (wchar_t*) malloc(sizeof(wchar_t)*(DestLength+1));
  res = pd->Info->InputBox(pd->PluginId, Id, Title, Prompt, HistoryName, SrcText,
                           DestText, DestLength+1, HelpTopic, Flags);
  if (res) push_utf8_string (L, DestText, -1);
  else lua_pushnil(L);

  free(DestText);
  return 1;
}

static int far_GetMsg(lua_State *L)
{
  int MsgId = luaL_checkinteger(L, 1);
  if (MsgId >= 0) {
    TPluginData *pd = GetPluginData(L);
    const wchar_t* str = pd->Info->GetMsg(pd->PluginId, MsgId);
    if (str) push_utf8_string(L, str, -1);
    else     lua_pushnil(L);
  }
  else
    lua_pushnil(L); // (MsgId < 0) crashes FAR
  return 1;
}

static int far_Text(lua_State *L)
{
  PSInfo *Info = GetPluginData(L)->Info;
  const wchar_t *Str;
  struct FarColor fc = { (FCF_FG_4BIT | FCF_BG_4BIT), 0x0F, 0x00, NULL };
  int X = luaL_checkinteger(L, 1);
  int Y = luaL_checkinteger(L, 2);
  if (lua_istable(L, 3))
    GetFarColorFromTable(L, 3, &fc);
  else if (lua_isnumber(L, 3)) {
    int Color = lua_tointeger(L, 3);
    fc.ForegroundColor = Color & 0x0F;
    fc.BackgroundColor = (Color>>4) & 0x0F;
  }
  Str = opt_utf8_string(L, 4, NULL);
  Info->Text(X, Y, &fc, Str);
  return 0;
}

// os.getenv does not always work correctly, hence the following.
static int win_GetEnv (lua_State *L)
{
  const wchar_t* name = check_utf8_string(L, 1, NULL);
  wchar_t buf[256], *p;
  DWORD res, size;
  res = GetEnvironmentVariableW (name, buf, DIM(buf));
  if (res == 0) return 0;
  if (res < DIM(buf)) return push_utf8_string(L, buf, -1), 1;

  size = res + 1;
  p = (wchar_t*)lua_newuserdata(L, size * sizeof(wchar_t));
  res = GetEnvironmentVariableW (name, p, size);
  if (res > 0 && res < size)
    return push_utf8_string(L, p, -1), 1;
  return 0;
}

static int win_SetEnv (lua_State *L)
{
  const wchar_t* name = check_utf8_string(L, 1, NULL);
  const wchar_t* value = opt_utf8_string(L, 2, NULL);
  BOOL res = SetEnvironmentVariableW (name, value);
  return lua_pushboolean (L, res), 1;
}

static HKEY CheckHKey (lua_State *L, int pos)
{
  const char *str = luaL_checkstring(L, pos);
  if (!strcmp(str, "HKLM")) return HKEY_LOCAL_MACHINE;
  if (!strcmp(str, "HKCC")) return HKEY_CURRENT_CONFIG;
  if (!strcmp(str, "HKCR")) return HKEY_CLASSES_ROOT;
  if (!strcmp(str, "HKCU")) return HKEY_CURRENT_USER;
  if (!strcmp(str, "HKU"))  return HKEY_USERS;
  luaL_argerror(L, pos, "must be 'HKLM', 'HKCC', 'HKCR', 'HKCU' or 'HKU'");
  return 0;
}

// SetRegKey (Root, Key, ValueName, DataType, ValueData)
//   Root:       root, [string], one of "HKLM", "HKCC", "HKCR", "HKCU", "HKU"
//   Key:        registry key, [string]
//   ValueName:  registry value name, [string]
//   DataType:   "string","expandstring","multistring","dword" or "binary", [string]
//   ValueData:  registry value data, [string | number | lstring]
// Returns:
//   nothing.
static int win_SetRegKey(lua_State *L)
{
  HKEY hRoot           = CheckHKey(L, 1);
  wchar_t* Key         = (wchar_t*)check_utf8_string(L, 2, NULL);
  wchar_t* ValueName   = (wchar_t*)check_utf8_string(L, 3, NULL);
  const char* DataType = luaL_checkstring(L, 4);
  size_t len;

  if (!strcmp ("string", DataType)) {
    SetRegKeyStr(hRoot, Key, ValueName,
              (wchar_t*)check_utf8_string(L, 5, NULL));
  }
  else if (!strcmp ("dword", DataType)) {
    SetRegKeyDword(hRoot, Key, ValueName, luaL_checkinteger(L, 5));
  }
  else if (!strcmp ("binary", DataType)) {
    BYTE *data = (BYTE*)luaL_checklstring(L, 5, &len);
    SetRegKeyArr(hRoot, Key, ValueName, data, len);
  }
  else if (!strcmp ("expandstring", DataType)) {
    const wchar_t* data = check_utf8_string(L, 5, NULL);
    HKEY hKey = CreateRegKey(hRoot, Key);
    RegSetValueExW(hKey, ValueName, 0, REG_EXPAND_SZ, (BYTE*)data, 1+wcslen(data));
    RegCloseKey(hKey);
  }
  else if (!strcmp ("multistring", DataType)) {
    const char* data = luaL_checklstring(L, 5, &len);
    HKEY hKey = CreateRegKey(hRoot, Key);
    RegSetValueExW(hKey, ValueName, 0, REG_MULTI_SZ, (BYTE*)data, len);
    RegCloseKey(hKey);
  }
  else
    luaL_argerror (L, 5, "unsupported value type");
  return 0;
}

// ValueData, DataType = GetRegKey (Root, Key, ValueName)
//   Root:       [string], one of "HKLM", "HKCC", "HKCR", "HKCU", "HKU"
//   Key:        registry key, [string]
//   ValueName:  registry value name, [string]
//   ValueData:  registry value data, [string | number | lstring]
//   DataType:   "string", "expandstring", "multistring", "dword"
//                    or "binary", [string]
static int win_GetRegKey(lua_State *L)
{
  HKEY hKey;
  DWORD datatype, datasize;
  char *data;
  LONG ret;

  HKEY hRoot = CheckHKey(L, 1);
  wchar_t* Key = (wchar_t*)check_utf8_string(L, 2, NULL);
  const wchar_t* ValueName = check_utf8_string(L, 3, NULL);

  hKey = OpenRegKey(hRoot, Key);
  if (hKey == NULL) {
    lua_pushnil(L);
    lua_pushstring(L, "OpenRegKey failed.");
    return 2;
  }

  RegQueryValueExW(hKey, ValueName, NULL, &datatype, NULL, &datasize);

  data = (char*) malloc(datasize);
  ret = RegQueryValueExW(hKey, ValueName, NULL, &datatype, (BYTE*)data, &datasize);
  RegCloseKey(hKey);

  if (ret != ERROR_SUCCESS) {
    lua_pushnil(L);
    lua_pushstring(L, "RegQueryValueEx failed.");
  }
  else {
    switch (datatype) {
      case REG_BINARY:
        lua_pushlstring (L, data, datasize);
        lua_pushstring (L, "binary");
        break;
      case REG_DWORD:
        lua_pushinteger (L, *(int*)data);
        lua_pushstring (L, "dword");
        break;
      case REG_SZ:
        push_utf8_string (L, (wchar_t*)data, -1);
        lua_pushstring (L, "string");
        break;
      case REG_EXPAND_SZ:
        push_utf8_string (L, (wchar_t*)data, -1);
        lua_pushstring (L, "expandstring");
        break;
      case REG_MULTI_SZ:
        push_utf8_string (L, (wchar_t*)data, datasize/sizeof(wchar_t));
        lua_pushstring (L, "multistring");
        break;
      default:
        lua_pushnil(L);
        lua_pushstring(L, "unsupported value type");
        break;
    }
  }
  free(data);
  return 2;
}

// Result = DeleteRegKey (Root, Key)
//   Root:      [string], one of "HKLM", "HKCC", "HKCR", "HKCU", "HKU"
//   Key:       registry key, [string]
//   Result:    TRUE if success, FALSE if failure, [boolean]
static int win_DeleteRegKey(lua_State *L)
{
  HKEY hRoot = CheckHKey(L, 1);
  const wchar_t* Key = check_utf8_string(L, 2, NULL);
  long res = RegDeleteKeyW (hRoot, Key);
  lua_pushboolean (L, res==ERROR_SUCCESS);
  return 1;
}

// Based on "CheckForEsc" function, by Ivan Sintyurin (spinoza@mail.ru)
WORD ExtractKey()
{
  INPUT_RECORD rec;
  DWORD ReadCount;
  HANDLE hConInp=GetStdHandle(STD_INPUT_HANDLE);
  while (PeekConsoleInput(hConInp,&rec,1,&ReadCount), ReadCount) {
    ReadConsoleInput(hConInp,&rec,1,&ReadCount);
    if (rec.EventType==KEY_EVENT && rec.Event.KeyEvent.bKeyDown)
      return rec.Event.KeyEvent.wVirtualKeyCode;
  }
  return 0;
}

// result = ExtractKey()
// -- general purpose function; not FAR dependent
static int win_ExtractKey(lua_State *L)
{
  WORD vKey = ExtractKey() & 0xff;
  if (vKey && VirtualKeyStrings[vKey])
    lua_pushstring(L, VirtualKeyStrings[vKey]);
  else
    lua_pushnil(L);
  return 1;
}

static int far_CopyToClipboard (lua_State *L)
{
  const wchar_t *str = check_utf8_string(L,1,NULL);
  int r = GetPluginData(L)->FSF->CopyToClipboard(str);
  return lua_pushboolean(L, r), 1;
}

static int far_PasteFromClipboard (lua_State *L)
{
  struct FarStandardFunctions *FSF = GetPluginData(L)->FSF;
  wchar_t* str = FSF->PasteFromClipboard();
  if (str) {
    push_utf8_string(L, str, -1);
    FSF->DeleteBuffer(str);
  }
  else lua_pushnil(L);
  return 1;
}

static int far_InputRecordToName (lua_State *L)
{
  wchar_t buf[256];
  INPUT_RECORD ir;
  size_t result;
  FillInputRecord(L, 1, &ir);
  result = GetPluginData(L)->FSF->FarInputRecordToName(&ir, buf, DIM(buf)-1);
  if (result > 0) {
    if (lua_toboolean(L, 2)) {
      static const char C[]="RCtrl", A[]="RAlt", S[]="Shift";
      const char *p;
      push_utf8_string(L, buf, -1);
      p = lua_tostring(L, -1);
      if (!strncmp(p, C+1, 4))       { lua_pushstring(L, C+1);  p += 4; }
      else if (!strncmp(p, C, 5))    { lua_pushstring(L, C); p += 5; }
      else lua_pushboolean(L, 0);

      if (!strncmp(p, A+1, 3))       { lua_pushstring(L, A+1);  p += 3; }
      else if (!strncmp(p, A, 4))    { lua_pushstring(L, A); p += 4; }
      else lua_pushboolean(L, 0);

      if (!strncmp(p, S, 5))         { lua_pushstring(L, S); p += 5; }
      else lua_pushboolean(L, 0);

      *p ? lua_pushstring(L, p) : lua_pushboolean(L, 0);
      return 4;
    }
    else
      push_utf8_string(L, buf, -1);
  }
  else
    lua_pushnil(L);
  return 1;
}

static int far_NameToInputRecord (lua_State *L)
{
  INPUT_RECORD ir;
  const wchar_t* str = check_utf8_string(L, 1, NULL);
  if (GetPluginData(L)->FSF->FarNameToInputRecord(str, &ir))
    pushInputRecord(L, &ir);
  else
    lua_pushnil(L);
  return 1;
}

static int far_LStricmp (lua_State *L)
{
  const wchar_t* s1 = check_utf8_string(L, 1, NULL);
  const wchar_t* s2 = check_utf8_string(L, 2, NULL);
  lua_pushinteger(L, GetPluginData(L)->FSF->LStricmp(s1, s2));
  return 1;
}

static int far_LStrnicmp (lua_State *L)
{
  const wchar_t* s1 = check_utf8_string(L, 1, NULL);
  const wchar_t* s2 = check_utf8_string(L, 2, NULL);
  int num = luaL_checkinteger(L, 3);
  if (num < 0) num = 0;
  lua_pushinteger(L, GetPluginData(L)->FSF->LStrnicmp(s1, s2, num));
  return 1;
}

// Result = far.ProcessName (Op, Mask, Name, Flags, Size)
//   @Op: PN_CMPNAME, PN_CMPNAMELIST, PN_GENERATENAME, PN_CHECKMASK
//   @Mask: string
//   @Name: string
//   @Flags: PN_SKIPPATH, PN_SHOWERRORMESSAGE
//   @Size: integer 0...0xFFFF
//   @Result: boolean
static int far_ProcessName (lua_State *L)
{
  UINT64 Op = CheckFlags(L,1);
  const wchar_t* Mask = check_utf8_string(L,2,NULL);
  const wchar_t* Name = (Op==PN_CHECKMASK) ? L"" : check_utf8_string(L,3,NULL);
	UINT64 Flags = CheckFlags(L,4);
  if (Op == PN_CMPNAME || Op == PN_CMPNAMELIST || Op == PN_CHECKMASK) {
    int result = GetPluginData(L)->FSF->ProcessName(Mask, (wchar_t*)Name, 0, Op|Flags);
    lua_pushboolean(L, result);
  }
  else if (Op == PN_GENERATENAME) {
    UINT64 Size = luaL_optinteger(L,5,0) & 0xFFFF;
    int result;
    wchar_t* buf;
    size_t len = wcslen(Mask), len2 = wcslen(Name), bufsize;
    if (len < len2) len = len2;
    bufsize = len < 1024 ? 1024 : len+1;
    buf = (wchar_t*)lua_newuserdata(L, bufsize * sizeof(wchar_t));
    wcsncpy(buf, Mask, bufsize-1);
    buf[bufsize-1] = 0;
    result = GetPluginData(L)->FSF->ProcessName(Name, buf, bufsize, Op|Flags|Size);
    result ? push_utf8_string(L, buf, -1) : lua_pushboolean(L, 0);
  }
  else
    lua_pushboolean(L, 0);
  return 1;
}

static int far_GetReparsePointInfo (lua_State *L)
{
  wchar_t* Dest;
  struct FarStandardFunctions *FSF = GetPluginData(L)->FSF;
  const wchar_t* Src = check_utf8_string(L, 1, NULL);
  int size = FSF->GetReparsePointInfo(Src, NULL, 0);
  if (size <= 0)
    return lua_pushnil(L), 1;
  Dest = (wchar_t*)lua_newuserdata(L, size * sizeof(wchar_t));
  FSF->GetReparsePointInfo(Src, Dest, size);
  return push_utf8_string(L, Dest, -1), 1;
}

static int far_LIsAlpha (lua_State *L)
{
  const wchar_t* str = check_utf8_string(L, 1, NULL);
  return lua_pushboolean(L, GetPluginData(L)->FSF->LIsAlpha(*str)), 1;
}

static int far_LIsAlphanum (lua_State *L)
{
  const wchar_t* str = check_utf8_string(L, 1, NULL);
  return lua_pushboolean(L, GetPluginData(L)->FSF->LIsAlphanum(*str)), 1;
}

static int far_LIsLower (lua_State *L)
{
  const wchar_t* str = check_utf8_string(L, 1, NULL);
  return lua_pushboolean(L, GetPluginData(L)->FSF->LIsLower(*str)), 1;
}

static int far_LIsUpper (lua_State *L)
{
  const wchar_t* str = check_utf8_string(L, 1, NULL);
  return lua_pushboolean(L, GetPluginData(L)->FSF->LIsUpper(*str)), 1;
}

static int convert_buf (lua_State *L, int command)
{
  struct FarStandardFunctions *FSF = GetPluginData(L)->FSF;
  int len;
  wchar_t* dest = check_utf8_string(L, 1, &len);
  if (command=='l')
    FSF->LLowerBuf(dest,len);
  else
    FSF->LUpperBuf(dest,len);
  push_utf8_string(L, dest, len);
  return 1;
}

static int far_LLowerBuf (lua_State *L) {
  return convert_buf(L, 'l');
}

static int far_LUpperBuf (lua_State *L) {
  return convert_buf(L, 'u');
}

static int far_MkTemp (lua_State *L)
{
  const wchar_t* prefix = opt_utf8_string(L, 1, NULL);
  const int dim = 4096;
  wchar_t* dest = (wchar_t*)lua_newuserdata(L, dim * sizeof(wchar_t));
  if (GetPluginData(L)->FSF->MkTemp(dest, dim, prefix))
    push_utf8_string(L, dest, -1);
  else
    lua_pushnil(L);
  return 1;
}

static int far_MkLink (lua_State *L)
{
  const wchar_t* src = check_utf8_string(L, 1, NULL);
  const wchar_t* dst = check_utf8_string(L, 2, NULL);
  UINT64 type = CheckFlags(L, 3);
  UINT64 flags = CheckFlags(L, 4);
  return lua_pushboolean(L, GetPluginData(L)->FSF->MkLink(src, dst, type, flags)), 1;
}

static int far_GetPathRoot (lua_State *L)
{
  const wchar_t* Path = check_utf8_string(L, 1, NULL);
  wchar_t* Root = (wchar_t*)lua_newuserdata(L, 4096 * sizeof(wchar_t));
  *Root = L'\0';
  GetPluginData(L)->FSF->GetPathRoot(Path, Root, 4096);
  return push_utf8_string(L, Root, -1), 1;
}

static int truncstring (lua_State *L, int op)
{
  struct FarStandardFunctions *FSF = GetPluginData(L)->FSF;
  const wchar_t *Src = check_utf8_string(L, 1, NULL), *ptr;
  wchar_t *Trg;
  int MaxLen = luaL_checkinteger(L, 2);
  int SrcLen = wcslen(Src);
  if (MaxLen < 0) MaxLen = 0;
  else if (MaxLen > SrcLen) MaxLen = SrcLen;
  Trg = (wchar_t*)lua_newuserdata(L, (1 + SrcLen) * sizeof(wchar_t));
  wcscpy(Trg, Src);
  ptr = (op == 'p') ? FSF->TruncPathStr(Trg, MaxLen) : FSF->TruncStr(Trg, MaxLen);
  return push_utf8_string(L, ptr, -1), 1;
}

static int far_TruncPathStr (lua_State *L)
{
  return truncstring(L, 'p');
}

static int far_TruncStr (lua_State *L)
{
  return truncstring(L, 's');
}

static int WINAPI FrsUserFunc (const struct PluginPanelItem *FData, const wchar_t *FullName,
  void *Param)
{
  int err, proceed;
  lua_State *L = (lua_State*) Param;
  lua_pushvalue(L, 3); // push the Lua function
  lua_createtable(L, 0, 8);
  PutFarFindData(L, FData);
  push_utf8_string(L, FullName, -1);
  err = lua_pcall(L, 2, 1, 0);
  proceed = !(err || lua_toboolean(L, -1));
  if (err)
    LF_Error(L, check_utf8_string(L, -1, NULL));
  lua_pop(L, 1);
  return proceed;
}

static int far_RecursiveSearch (lua_State *L)
{
  UINT64 Flags;
  const wchar_t *InitDir = check_utf8_string(L, 1, NULL);
  const wchar_t *Mask = check_utf8_string(L, 2, NULL);
  luaL_checktype(L, 3, LUA_TFUNCTION);
  Flags = CheckFlags(L, 4);
  GetPluginData(L)->FSF->FarRecursiveSearch(InitDir, Mask, FrsUserFunc, Flags, L);
  return 0;
}

static int far_ConvertPath (lua_State *L)
{
  struct FarStandardFunctions *FSF = GetPluginData(L)->FSF;
  const wchar_t *Src = check_utf8_string(L, 1, NULL);
  enum CONVERTPATHMODES Mode = lua_isnoneornil(L,2) ?
    CPM_FULL : (enum CONVERTPATHMODES)check_env_flag(L,2);
  size_t Size = FSF->ConvertPath(Mode, Src, NULL, 0);
  wchar_t* Target = (wchar_t*)lua_newuserdata(L, Size*sizeof(wchar_t));
  FSF->ConvertPath(Mode, Src, Target, Size);
  push_utf8_string(L, Target, -1);
  return 1;
}

static int win_GetFileInfo (lua_State *L)
{
  WIN32_FIND_DATAW fd;
  const wchar_t *fname = check_utf8_string(L, 1, NULL);
  HANDLE h = FindFirstFileW(fname, &fd);
  if (h == INVALID_HANDLE_VALUE)
    lua_pushnil(L);
  else {
    PushWinFindData(L, &fd);
    FindClose(h);
  }
  return 1;
}

static int far_AdvControl (lua_State *L)
{
  TPluginData *pd = GetPluginData(L);
  GUID* PluginId = pd->PluginId;
  PSInfo *Info = pd->Info;
  int Command = CAST(int, check_env_flag (L, 1));
  int Param1 = 0;
  void *Param2 = NULL;
  wchar_t buf[300];
  struct ActlEjectMedia em;
  struct FarColor fc;
  struct FarSetColors fsc;
  struct ProgressValue pv;
  struct WindowInfo wi;
  struct WindowType wt;
  SMALL_RECT sr;
  COORD coord;
  INPUT_RECORD ir;

  lua_settop(L,3);  /* for proper calling GetOptIntFromTable and the like */
  switch (Command) {
    default:
      return luaL_argerror(L, 1, "command not supported");

    case ACTL_COMMIT:
    case ACTL_GETFARHWND:
    case ACTL_GETCONFIRMATIONS:
    case ACTL_GETDESCSETTINGS:
    case ACTL_GETDIALOGSETTINGS:
    case ACTL_GETINTERFACESETTINGS:
    case ACTL_GETPANELSETTINGS:
    case ACTL_GETPLUGINMAXREADDATA:
    case ACTL_GETSYSTEMSETTINGS:
    case ACTL_GETWINDOWCOUNT:
    case ACTL_PROGRESSNOTIFY:
    case ACTL_QUIT:
    case ACTL_REDRAWALL:
      break;

    case ACTL_SETCURRENTWINDOW:
      Param1 = luaL_checkinteger(L, 2);
      break;

    case ACTL_WAITKEY:
      if (!lua_isnoneornil(L, 3)) {
        OptInputRecord(L, pd, 3, &ir);
        Param2 = &ir;
      }
      break;

    case ACTL_GETCOLOR:
      Param1 = luaL_checkinteger(L, 2);
      if (Info->AdvControl(PluginId, Command, Param1, &fc))
        PushFarColor(L, &fc);
      else
        lua_pushnil(L);
      return 1;

    case ACTL_SYNCHRO: {
      int p = luaL_checkinteger(L, 2);
      Param2 = malloc(sizeof(TSynchroData));
      InitSynchroData((TSynchroData*)Param2, 0, 0, 0, p);
      break;
    }

    case ACTL_SETPROGRESSSTATE:
      Param1 = (INT_PTR) check_env_flag(L, 2);
      break;

    case ACTL_SETPROGRESSVALUE:
      luaL_checktype(L, 3, LUA_TTABLE);
      pv.Completed = (UINT64)GetOptNumFromTable(L, "Completed", 0.0);
      pv.Total = (UINT64)GetOptNumFromTable(L, "Total", 100.0);
      Param2 = &pv;
      break;

    case ACTL_GETSYSWORDDIV:
      Info->AdvControl(PluginId, Command, DIM(buf), buf);
      return push_utf8_string(L,buf,-1), 1;

    case ACTL_EJECTMEDIA:
      luaL_checktype(L, 3, LUA_TTABLE);
      lua_getfield(L, 3, "Letter");
      em.Letter = lua_isstring(L,-1) ? lua_tostring(L,-1)[0] : '\0';
      em.Flags = CheckFlagsFromTable(L, 3, "Flags");
      Param2 = &em;
      break;

    case ACTL_GETARRAYCOLOR: {
      int size = Info->AdvControl(PluginId, Command, 0, NULL), i;
      int len = size / sizeof(struct FarColor);
      struct FarColor *arr = (struct FarColor*) lua_newuserdata(L, size);
      Info->AdvControl(PluginId, Command, 0, arr);
      lua_createtable(L, len, 0);
      for (i=0; i < len; i++) {
        PushFarColor(L, &arr[i]);
        lua_rawseti(L, -2, i+1);
      }
      return 1;
    }

    case ACTL_GETFARMANAGERVERSION: {
      struct VersionInfo vi;
      Info->AdvControl(PluginId, Command, 0, &vi);
      if (lua_toboolean(L, 2)) {
        lua_pushinteger(L, vi.Major);
        lua_pushinteger(L, vi.Minor);
        lua_pushinteger(L, vi.Revision);
        lua_pushinteger(L, vi.Build);
        lua_pushinteger(L, vi.Stage);
        return 5;
      }
      lua_pushfstring(L, "%d.%d.%d.%d.%d", vi.Major, vi.Minor, vi.Revision, vi.Build, vi.Stage);
      return 1;
    }

    case ACTL_GETWINDOWINFO:
    {
      int r;
      memset(&wi, 0, sizeof(wi));
      wi.StructSize = sizeof(wi);
      wi.Pos = luaL_optinteger(L, 2, -1);

      r = Info->AdvControl(PluginId, Command, 0, &wi);
      if (!r)
        return lua_pushinteger(L,0), 1;
      wi.TypeName = (wchar_t*)
        lua_newuserdata(L, (wi.TypeNameSize + wi.NameSize) * sizeof(wchar_t));
      wi.Name = wi.TypeName + wi.TypeNameSize;

      r = Info->AdvControl(PluginId, Command, 0, &wi);
      if (!r)
        return lua_pushinteger(L,0), 1;
      lua_createtable(L,0,6);
      lua_pushlightuserdata(L, CAST(void*, wi.Id));
      lua_setfield(L, -2, "Id");
      PutIntToTable(L, "Pos", wi.Pos);
      PutIntToTable(L, "Type", wi.Type);
      PutFlagsToTable(L, "Flags", wi.Flags);
      PutWStrToTable(L, "TypeName", wi.TypeName, -1);
      PutWStrToTable(L, "Name", wi.Name, -1);
      return 1;
    }

    case ACTL_SETARRAYCOLOR:
    {
      size_t size, i;
      luaL_checktype(L, 3, LUA_TTABLE);
      fsc.StartIndex = GetOptIntFromTable(L, "StartIndex", 0);
      lua_getfield(L, 3, "Flags");
      fsc.Flags = GetFlagCombination(L, -1, NULL);
      fsc.ColorsCount = lua_objlen(L, 3);
      size = fsc.ColorsCount * sizeof(struct FarColor);
      fsc.Colors = (struct FarColor*) lua_newuserdata(L, size);
      memset(fsc.Colors, 0, size);
      for (i=0; i < fsc.ColorsCount; i++) {
        lua_rawgeti(L, 3, i+1);
        if (lua_istable(L, -1))
          GetFarColorFromTable(L, -1, &fsc.Colors[i]);
        lua_pop(L,1);
      }
      Param2 = &fsc;
      break;
    }

    case ACTL_GETFARRECT:
      if (!Info->AdvControl(PluginId, Command, 0, &sr))
        return 0;
      lua_createtable(L, 0, 4);
      PutIntToTable(L, "Left",   sr.Left);
      PutIntToTable(L, "Top",    sr.Top);
      PutIntToTable(L, "Right",  sr.Right);
      PutIntToTable(L, "Bottom", sr.Bottom);
      return 1;

    case ACTL_GETCURSORPOS:
      if (!Info->AdvControl(PluginId, Command, 0, &coord))
        return 0;
      lua_createtable(L, 0, 2);
      PutIntToTable(L, "X", coord.X);
      PutIntToTable(L, "Y", coord.Y);
      return 1;

    case ACTL_SETCURSORPOS:
      luaL_checktype(L, 3, LUA_TTABLE);
      lua_getfield(L, 3, "X");
      coord.X = lua_tointeger(L, -1);
      lua_getfield(L, 3, "Y");
      coord.Y = lua_tointeger(L, -1);
      Param2 = &coord;
      break;

    case ACTL_GETWINDOWTYPE:
      wt.StructSize = sizeof(wt);
      if (Info->AdvControl(PluginId, Command, 0, &wt)) {
        lua_createtable(L, 0, 1);
        lua_pushinteger(L, wt.Type);
        lua_setfield(L, -2, "Type");
      }
      else lua_pushnil(L);
      return 1;
  }
  lua_pushinteger(L, Info->AdvControl(PluginId, Command, Param1, Param2));
  return 1;
}

static int far_MacroLoadAll (lua_State* L)
{
  TPluginData *pd = GetPluginData(L);
  lua_pushboolean(L, pd->Info->MacroControl(pd->PluginId, MCTL_LOADALL, 0, 0));
  return 1;
}

static int far_MacroSaveAll (lua_State* L)
{
  TPluginData *pd = GetPluginData(L);
  lua_pushboolean(L, pd->Info->MacroControl(pd->PluginId, MCTL_SAVEALL, 0, 0));
  return 1;
}

static int far_MacroGetState (lua_State* L)
{
  TPluginData *pd = GetPluginData(L);
  lua_pushinteger(L, pd->Info->MacroControl(pd->PluginId, MCTL_GETSTATE, 0, 0));
  return 1;
}

static int far_MacroGetArea (lua_State* L)
{
  TPluginData *pd = GetPluginData(L);
  lua_pushinteger(L, pd->Info->MacroControl(pd->PluginId, MCTL_GETAREA, 0, 0));
  return 1;
}

static int MacroSendString (lua_State* L, int Param1)
{
  TPluginData *pd = GetPluginData(L);
  struct MacroSendMacroText smt;
  smt.StructSize = sizeof(smt);
  smt.SequenceText = check_utf8_string(L, 1, NULL);
  smt.Flags = CheckFlags(L, 2);
  OptInputRecord(L, pd, 3, &smt.AKey);
  lua_pushboolean(L, pd->Info->MacroControl(pd->PluginId, MCTL_SENDSTRING, Param1, &smt));
  return 1;
}

static int far_MacroPost (lua_State* L)
{
  return MacroSendString(L, MSSC_POST);
}

static int far_MacroCheck (lua_State* L)
{
  return MacroSendString(L, MSSC_CHECK);
}

static int far_MacroGetLastError (lua_State* L)
{
  TPluginData *pd = GetPluginData(L);
  int size = pd->Info->MacroControl(pd->PluginId, MCTL_GETLASTERROR, 0, NULL);
  if (size) {
    struct MacroParseResult *mpr = (struct MacroParseResult*)lua_newuserdata(L, size);
    mpr->StructSize = sizeof(*mpr);
    pd->Info->MacroControl(pd->PluginId, MCTL_GETLASTERROR, size, mpr);
    lua_createtable(L, 0, 4);
    PutIntToTable(L, "ErrCode", mpr->ErrCode);
    PutIntToTable(L, "ErrPosX", mpr->ErrPos.X);
    PutIntToTable(L, "ErrPosY", mpr->ErrPos.Y);
    PutWStrToTable(L, "ErrSrc", mpr->ErrSrc, -1);
  }
  else
    lua_pushboolean(L, 0);
  return 1;
}

int LF_MacroCallback (lua_State* L, void* Id, FARADDKEYMACROFLAGS Flags)
{
  int result = FALSE;
  int funcref = (INT_PTR) Id;
  lua_rawgeti(L, LUA_REGISTRYINDEX, funcref);
  if (lua_type(L,-1) == LUA_TFUNCTION) {
    lua_pushinteger(L, funcref);
    push64(L, Flags);
    if (lua_pcall(L, 2, 1, 0) == 0)
      result = lua_toboolean(L, -1);
  }
  lua_pop(L, 1);
  return result;
}

static int far_MacroAdd (lua_State* L)
{
  TPluginData *pd = GetPluginData(L);

  INT_PTR ref;
  struct MacroAddMacro data;
  memset(&data, 0, sizeof(data));
  data.StructSize = sizeof(data);

  data.Area = OptFlags(L, 1, MACROAREA_COMMON);
  data.Flags = OptFlags(L, 2, 0);
  OptInputRecord(L, pd, 3, &data.AKey);
  data.SequenceText = check_utf8_string(L, 4, NULL);
  data.Description = opt_utf8_string(L, 5, L"");
  if (lua_isnoneornil(L, 6))
    lua_pushboolean(L, 1);
  else {
    luaL_checktype(L, 6, LUA_TFUNCTION);
    data.Callback = pd->MacroCallback;
    lua_pushvalue(L, 6);
  }
  ref = luaL_ref(L, LUA_REGISTRYINDEX);
  data.Id = (void*) ref;

  if (pd->Info->MacroControl(pd->PluginId, MCTL_ADDMACRO, 0, &data))
    lua_pushinteger(L, ref);
  else {
    luaL_unref(L, LUA_REGISTRYINDEX, ref);
    lua_pushnil(L);
  }
  return 1;
}

static int far_MacroDelete (lua_State* L)
{
  TPluginData *pd = GetPluginData(L);
  INT_PTR ref = luaL_checkinteger(L, 1);
  int result = pd->Info->MacroControl(pd->PluginId, MCTL_DELMACRO, 0, (void*)ref);
  if (result)
    luaL_unref(L, LUA_REGISTRYINDEX, ref);
  lua_pushboolean(L, result);
  return 1;
}

static int far_CPluginStartupInfo(lua_State *L)
{
  return lua_pushlightuserdata(L, (void*)GetPluginData(L)->Info), 1;
}

static void pushSystemTime (lua_State *L, const SYSTEMTIME *st)
{
  lua_createtable(L, 0, 8);
  PutIntToTable(L, "wYear", st->wYear);
  PutIntToTable(L, "wMonth", st->wMonth);
  PutIntToTable(L, "wDayOfWeek", st->wDayOfWeek);
  PutIntToTable(L, "wDay", st->wDay);
  PutIntToTable(L, "wHour", st->wHour);
  PutIntToTable(L, "wMinute", st->wMinute);
  PutIntToTable(L, "wSecond", st->wSecond);
  PutIntToTable(L, "wMilliseconds", st->wMilliseconds);
}

static void pushFileTime (lua_State *L, const FILETIME *ft)
{
  long long llFileTime = ft->dwLowDateTime + 0x100000000LL * ft->dwHighDateTime;
  llFileTime /= 10000;
  lua_pushnumber(L, (double)llFileTime);
}

static int win_GetSystemTimeAsFileTime (lua_State *L)
{
  FILETIME ft;
  GetSystemTimeAsFileTime(&ft);
  pushFileTime(L, &ft);
  return 1;
}

static int win_FileTimeToSystemTime (lua_State *L)
{
  FILETIME ft;
  SYSTEMTIME st;
  long long llFileTime = 10000 * (long long) luaL_checknumber(L, 1);
  ft.dwLowDateTime = llFileTime & 0xFFFFFFFF;
  ft.dwHighDateTime = llFileTime >> 32;
  if (! FileTimeToSystemTime(&ft, &st))
    return SysErrorReturn(L);
  pushSystemTime(L, &st);
  return 1;
}

static int win_SystemTimeToFileTime (lua_State *L)
{
  FILETIME ft;
  SYSTEMTIME st;
  memset(&st, 0, sizeof(st));
  luaL_checktype(L, 1, LUA_TTABLE);
  lua_settop(L, 1);
  st.wYear         = GetOptIntFromTable(L, "wYear", 0);
  st.wMonth        = GetOptIntFromTable(L, "wMonth", 0);
  st.wDayOfWeek    = GetOptIntFromTable(L, "wDayOfWeek", 0);
  st.wDay          = GetOptIntFromTable(L, "wDay", 0);
  st.wHour         = GetOptIntFromTable(L, "wHour", 0);
  st.wMinute       = GetOptIntFromTable(L, "wMinute", 0);
  st.wSecond       = GetOptIntFromTable(L, "wSecond", 0);
  st.wMilliseconds = GetOptIntFromTable(L, "wMilliseconds", 0);
  if (! SystemTimeToFileTime(&st, &ft))
    return SysErrorReturn(L);
  pushFileTime(L, &ft);
  return 1;
}

static int win_FileTimeToLocalFileTime (lua_State *L)
{
  FILETIME ft, local_ft;
  long long llFileTime = 10000 * (long long) luaL_checknumber(L, 1);
  ft.dwLowDateTime = llFileTime & 0xFFFFFFFF;
  ft.dwHighDateTime = llFileTime >> 32;
  if (FileTimeToLocalFileTime(&ft, &local_ft))
    pushFileTime(L, &local_ft);
  else
    return SysErrorReturn(L);
  return 1;
}

static int win_CompareString (lua_State *L)
{
  DWORD dwFlags = 0;
  int len1, len2, result;
  const wchar_t *ws1  = check_utf8_string(L, 1, &len1);
  const wchar_t *ws2  = check_utf8_string(L, 2, &len2);
  const char *sLocale = luaL_optstring(L, 3, "");
  const char *sFlags  = luaL_optstring(L, 4, "");

  LCID Locale = LOCALE_USER_DEFAULT;
  if      (!strcmp(sLocale, "s")) Locale = LOCALE_SYSTEM_DEFAULT;
  else if (!strcmp(sLocale, "n")) Locale = LOCALE_NEUTRAL;

  if (strchr(sFlags, 'c')) dwFlags |= NORM_IGNORECASE;
  if (strchr(sFlags, 'k')) dwFlags |= NORM_IGNOREKANATYPE;
  if (strchr(sFlags, 'n')) dwFlags |= NORM_IGNORENONSPACE;
  if (strchr(sFlags, 's')) dwFlags |= NORM_IGNORESYMBOLS;
  if (strchr(sFlags, 'w')) dwFlags |= NORM_IGNOREWIDTH;
  if (strchr(sFlags, 'S')) dwFlags |= SORT_STRINGSORT;

  result = CompareStringW(Locale, dwFlags, ws1, len1, ws2, len2) - 2;
  (result == -2) ? lua_pushnil(L) : lua_pushinteger(L, result);
  return 1;
}

static int win_wcscmp (lua_State *L)
{
  const wchar_t *ws1  = check_utf8_string(L, 1, NULL);
  const wchar_t *ws2  = check_utf8_string(L, 2, NULL);
  int insens = lua_toboolean(L, 3);
  lua_pushinteger(L, (insens ? _wcsicmp : wcscmp)(ws1, ws2));
  return 1;
}

static int far_MakeMenuItems (lua_State *L)
{
  int argn = lua_gettop(L);
  lua_createtable(L, argn, 0);
  if (argn > 0) {
    int item = 1, i;
    wchar_t delim[] = { 9474, L'\0' };
    wchar_t fmt1[64], fmt2[64], wbuf[64];
    int maxno = (int)floor(log10(argn)) + 1;
    wsprintfW(fmt1, L"%%%dd%ls ", maxno, delim);
    wsprintfW(fmt2, L"%%%dls%ls ", maxno, delim);
    for (i=1; i<=argn; i++) {
      int len, j;
      wchar_t *str, *start;
      lua_getglobal(L, "tostring");
      if (i == 1 && lua_type(L,-1) != LUA_TFUNCTION)
        luaL_error(L, "global `tostring' is not function");
      lua_pushvalue(L, i);
      if (0 != lua_pcall(L, 1, 1, 0))
        luaL_error(L, lua_tostring(L, -1));
      lua_replace(L, i);
      str = check_utf8_string(L, i, &len);
      start = str;
      for (j=0; j<len; j++)
        if (str[j] == 0) str[j] = L' ';
      do {
        wchar_t* nl = wcschr(start, L'\n');
        if (nl) *nl = L'\0';
        start == str ? wsprintfW(wbuf, fmt1, i) : wsprintfW(wbuf, fmt2, L"");
        lua_newtable(L);
        push_utf8_string(L, wbuf, -1);
        push_utf8_string(L, start, nl ? (nl++) - start : len - (start-str));
        lua_concat(L, 2);
        lua_setfield(L, -2, "text");
        lua_rawseti(L, argn+1, item++);
        start = nl;
      } while (start);
    }
  }
  return 1;
}

static int far_Show (lua_State *L)
{
  const char* f =
  "local items,n=...\n"
  "local bottom=n==0 and 'No arguments' or n==1 and '1 argument' or n..' arguments'\n"
  "far.Menu({Title='',Bottom=bottom,Flags='FMENU_SHOWAMPERSAND'},\n"
    "items,{{BreakKey='RETURN'},{BreakKey='SPACE'}})";

  int argn = lua_gettop(L);
  far_MakeMenuItems(L);

  if (luaL_loadstring(L, f) != 0)
    luaL_error(L, lua_tostring(L, -1));
  lua_pushvalue(L, -2);
  lua_pushinteger(L, argn);
  if (lua_pcall(L, 2, 0, 0) != 0)
    luaL_error(L, lua_tostring(L, -1));
  return 0;
}

static void NewVirtualKeyTable(lua_State* L, BOOL twoways)
{
  int i;
  lua_createtable(L, twoways ? 256:0, 200);
  for (i=0; i<256; i++) {
    const char* str = VirtualKeyStrings[i];
    if (str) {
      lua_pushinteger(L, i);
      lua_setfield(L, -2, str);
    }
    if (twoways) {
      lua_pushstring(L, str ? str : "");
      lua_rawseti(L, -2, i);
    }
  }
}

static int win_GetVirtualKeys (lua_State *L)
{
  NewVirtualKeyTable(L, TRUE);
  return 1;
}

HANDLE* CheckFileFilter(lua_State* L, int pos)
{
  return (HANDLE*)luaL_checkudata(L, pos, FarFileFilterType);
}

HANDLE CheckValidFileFilter(lua_State* L, int pos)
{
  HANDLE h = *CheckFileFilter(L, pos);
  luaL_argcheck(L,h != INVALID_HANDLE_VALUE,pos,"attempt to access invalid file filter");
  return h;
}

static int far_CreateFileFilter (lua_State *L)
{
  PSInfo *Info = GetPluginData(L)->Info;
  HANDLE hHandle = (luaL_checkinteger(L,1) % 2) ? PANEL_ACTIVE:PANEL_PASSIVE;
  int filterType = CAST(int, check_env_flag(L,2));
  HANDLE* pOutHandle = (HANDLE*)lua_newuserdata(L, sizeof(HANDLE));
  if (Info->FileFilterControl(hHandle, FFCTL_CREATEFILEFILTER, filterType,
    pOutHandle))
  {
    luaL_getmetatable(L, FarFileFilterType);
    lua_setmetatable(L, -2);
  }
  else
    lua_pushnil(L);
  return 1;
}

static int filefilter_Free (lua_State *L)
{
  HANDLE *h = CheckFileFilter(L, 1);
  if (*h != INVALID_HANDLE_VALUE) {
    PSInfo *Info = GetPluginData(L)->Info;
    lua_pushboolean(L, Info->FileFilterControl(*h, FFCTL_FREEFILEFILTER, 0, 0));
    *h = INVALID_HANDLE_VALUE;
  }
  else
    lua_pushboolean(L,0);
  return 1;
}

static int filefilter_gc (lua_State *L)
{
  filefilter_Free(L);
  return 0;
}

static int filefilter_tostring (lua_State *L)
{
  HANDLE *h = CheckFileFilter(L, 1);
  if (*h != INVALID_HANDLE_VALUE)
    lua_pushfstring(L, "%s (%p)", FarFileFilterType, h);
  else
    lua_pushfstring(L, "%s (closed)", FarFileFilterType);
  return 1;
}

static int filefilter_OpenMenu (lua_State *L)
{
  HANDLE h = CheckValidFileFilter(L, 1);
  PSInfo *Info = GetPluginData(L)->Info;
  lua_pushboolean(L, Info->FileFilterControl(h, FFCTL_OPENFILTERSMENU, 0, 0));
  return 1;
}

static int filefilter_Starting (lua_State *L)
{
  HANDLE h = CheckValidFileFilter(L, 1);
  PSInfo *Info = GetPluginData(L)->Info;
  lua_pushboolean(L, Info->FileFilterControl(h, FFCTL_STARTINGTOFILTER, 0, 0));
  return 1;
}

static int filefilter_IsFileInFilter (lua_State *L)
{
  struct PluginPanelItem ffd;
  PSInfo *Info = GetPluginData(L)->Info;
  HANDLE h = CheckValidFileFilter(L, 1);
  luaL_checktype(L, 2, LUA_TTABLE);
  lua_settop(L, 2);         // +2
  GetFarFindData(L, &ffd);  // +4
  lua_pushboolean(L, Info->FileFilterControl(h, FFCTL_ISFILEINFILTER, 0, &ffd));
  return 1;
}

static int plugin_load (lua_State *L, enum FAR_PLUGINS_CONTROL_COMMANDS command)
{
  PSInfo *Info = GetPluginData(L)->Info;
  int param1 = CAST(int, check_env_flag(L, 1));
  void *param2 = check_utf8_string(L, 2, NULL);
  INT_PTR result = Info->PluginsControl(INVALID_HANDLE_VALUE, command, param1, param2);
  if (result) PushPluginHandle(L, CAST(HANDLE, result));
  else lua_pushnil(L);
  return 1;
}

static int far_LoadPlugin (lua_State *L) { return plugin_load(L, PCTL_LOADPLUGIN); }
static int far_ForcedLoadPlugin (lua_State *L) { return plugin_load(L, PCTL_FORCEDLOADPLUGIN); }

static int far_UnloadPlugin (lua_State *L)
{
  PSInfo *Info = GetPluginData(L)->Info;
  void* Handle = *(void**)luaL_checkudata(L, 1, PluginHandleType);
  lua_pushboolean(L, Info->PluginsControl(Handle, PCTL_UNLOADPLUGIN, 0, 0));
  return 1;
}

static int far_FindPlugin (lua_State *L)
{
  PSInfo *Info = GetPluginData(L)->Info;
  int param1 = CAST(int, check_env_flag(L, 1));
  void *param2 = NULL;
  if (param1 == PFM_MODULENAME)
    param2 = check_utf8_string(L, 2, NULL);
  else if (param1 == PFM_GUID) {
    size_t len;
    param2 = CAST(void*, luaL_checklstring(L, 2, &len));
    if (len < sizeof(GUID)) param2 = NULL;
  }
  if (param2) {
    INT_PTR handle = Info->PluginsControl(NULL, PCTL_FINDPLUGIN, param1, param2);
    if (handle) {
      PushPluginHandle(L, CAST(HANDLE, handle));
      return 1;
    }
  }
  lua_pushnil(L);
  return 1;
}

static void PutPluginMenuItemToTable (lua_State *L, const char* field, const struct PluginMenuItem *mi)
{
  lua_createtable(L, 0, 3);
  {
    size_t i;
    PutIntToTable(L, "Count", mi->Count);
    lua_createtable(L, mi->Count, 0); // Guids
    lua_createtable(L, mi->Count, 0); // Strings
    for (i=0; i < mi->Count; i++) {
      lua_pushlstring(L, CAST(const char*, mi->Guids + i), sizeof(GUID));
      lua_rawseti(L, -3, i+1);
      push_utf8_string(L, mi->Strings[i], -1);
      lua_rawseti(L, -2, i+1);
    }
    lua_setfield(L, -3, "Strings");
    lua_setfield(L, -2, "Guids");
  }
  lua_setfield(L, -2, field);
}

static void PutVersionInfoToTable (lua_State *L, const char* field, const struct VersionInfo *vi)
{
  lua_createtable(L, 5, 0);
  PutIntToArray(L, 1, vi->Major);
  PutIntToArray(L, 2, vi->Minor);
  PutIntToArray(L, 3, vi->Revision);
  PutIntToArray(L, 4, vi->Build);
  PutIntToArray(L, 5, vi->Stage);
  lua_setfield(L, -2, field);
}

static int far_GetPluginInformation (lua_State *L)
{
  struct FarGetPluginInformation *pi;
  PSInfo *Info = GetPluginData(L)->Info;
  HANDLE Handle = *(HANDLE*)luaL_checkudata(L, 1, PluginHandleType);
  size_t size = Info->PluginsControl(Handle, PCTL_GETPLUGININFORMATION, 0, 0);
  if (size == 0) return lua_pushnil(L), 1;

  pi = (struct FarGetPluginInformation *)lua_newuserdata(L, size);
  pi->StructSize = sizeof(*pi);
  if(!Info->PluginsControl(Handle, PCTL_GETPLUGININFORMATION, size, pi))
    return lua_pushnil(L), 1;

  lua_createtable(L, 0, 4);
  {
    PutWStrToTable(L, "ModuleName", pi->ModuleName, -1);
    PutFlagsToTable(L, "Flags", pi->Flags);
    lua_createtable(L, 0, 6); // PInfo
    {
      PutIntToTable(L, "StructSize", pi->PInfo.StructSize);
      PutFlagsToTable(L, "Flags", pi->PInfo.Flags);
      PutPluginMenuItemToTable(L, "DiskMenu", &pi->PInfo.DiskMenu);
      PutPluginMenuItemToTable(L, "PluginMenu", &pi->PInfo.PluginMenu);
      PutPluginMenuItemToTable(L, "PluginConfig", &pi->PInfo.PluginConfig);
      if (pi->PInfo.CommandPrefix)
        PutWStrToTable(L, "CommandPrefix", pi->PInfo.CommandPrefix, -1);
      lua_setfield(L, -2, "PInfo");
    }
    lua_createtable(L, 0, 7); // GInfo
    {
      PutIntToTable(L, "StructSize", pi->GInfo.StructSize);
      PutVersionInfoToTable(L, "MinFarVersion", &pi->GInfo.MinFarVersion);
      PutVersionInfoToTable(L, "Version", &pi->GInfo.Version);
      PutLStrToTable(L, "Guid", (const char*)&pi->GInfo.Guid, sizeof(GUID));
      PutWStrToTable(L, "Title", pi->GInfo.Title, -1);
      PutWStrToTable(L, "Description", pi->GInfo.Description, -1);
      PutWStrToTable(L, "Author", pi->GInfo.Author, -1);
      lua_setfield(L, -2, "GInfo");
    }
  }
  return 1;
}

static int far_GetPlugins (lua_State *L)
{
  PSInfo *Info = GetPluginData(L)->Info;
  int count = Info->PluginsControl(INVALID_HANDLE_VALUE, PCTL_GETPLUGINS, 0, 0);
  lua_createtable(L, count, 0);
  if (count > 0) {
    int i;
    HANDLE *handles = lua_newuserdata(L, count*sizeof(HANDLE));
    count = Info->PluginsControl(INVALID_HANDLE_VALUE, PCTL_GETPLUGINS, count, handles);
    for (i=0; i<count; i++) {
      PushPluginHandle(L, handles[i]);
      lua_rawseti(L, -3, i+1);
    }
    lua_pop(L, 1);
  }
  return 1;
}

static int far_XLat (lua_State *L)
{
  int size;
  wchar_t *Line = check_utf8_string(L, 1, &size), *str;
  int StartPos = luaL_optinteger(L, 2, 1) - 1;
  int EndPos = luaL_optinteger(L, 3, size);
  UINT64 Flags = CheckFlags(L, 4);

  StartPos < 0 ? StartPos = 0 : StartPos > size ? StartPos = size : 0;
  EndPos < StartPos ? EndPos = StartPos : EndPos > size ? EndPos = size : 0;

  str = GetPluginData(L)->FSF->XLat(Line, StartPos, EndPos, Flags);
  str ? (void)push_utf8_string(L, str, -1) : lua_pushnil(L);
  return 1;
}

static int far_FormatFileSize (lua_State *L)
{
  wchar_t buf[256];
  UINT64 Size = CAST(UINT64, luaL_checknumber(L, 1));
  int Width = luaL_checkinteger(L, 2);
  UINT64 Flags = CheckFlags(L, 3);
  if (Flags & FFFS_MINSIZEINDEX)
    Flags |= (luaL_optinteger(L, 4, 0) & FFFS_MINSIZEINDEX_MASK);
  GetPluginData(L)->FSF->FormatFileSize(Size, Width, Flags, buf, DIM(buf));
  push_utf8_string(L, buf, -1);
  return 1;
}

DWORD WINAPI TimerThreadFunc (LPVOID data)
{
  FILETIME tCurrent;
  LARGE_INTEGER lStart, lCurrent;
  TSynchroData *sd;
  TTimerData* td = (TTimerData*)data;

  int interval_copy = td->interval;
  td->interval_changed = 0;

  memcpy(&lStart, &td->tStart, sizeof(lStart));
  for ( ; !td->needClose; lStart.QuadPart += interval_copy * 10000ll) {
    while (!td->needClose) {
      long long llElapsed;
      int iElapsed;
      GetSystemTimeAsFileTime(&tCurrent);
      memcpy(&lCurrent, &tCurrent, sizeof(lCurrent));
      if (td->interval_changed) {
        interval_copy = td->interval;
        lStart.QuadPart = lCurrent.QuadPart;
        td->interval_changed = 0;
      }
      llElapsed = lCurrent.QuadPart - lStart.QuadPart;
      iElapsed = CAST(int, llElapsed / 10000);
      if (iElapsed + 5 < interval_copy) {
        Sleep(10);
      }
      else break;
    }
    if (!td->needClose && td->enabled) {
      sd = (TSynchroData*) malloc(sizeof(TSynchroData));
      InitSynchroData(sd, LUAFAR_TIMER_CALL, td->objRef, td->funcRef, 0);
      td->Info->AdvControl(td->PluginGuid, ACTL_SYNCHRO, 0, sd);
    }
  }
  sd = (TSynchroData*) malloc(sizeof(TSynchroData));
  InitSynchroData(sd, LUAFAR_TIMER_UNREF, td->objRef, td->funcRef, 0);
  td->Info->AdvControl(td->PluginGuid, ACTL_SYNCHRO, 0, sd);
  return 0;
}

static int far_Timer (lua_State *L)
{
  TPluginData *pd;
  TTimerData *td;
  FILETIME ft;
  GetSystemTimeAsFileTime(&ft); // do this immediately, for greater accuracy

  pd = GetPluginData(L);
  td = (TTimerData*)lua_newuserdata(L, sizeof(TTimerData));
  td->Info = pd->Info;
  td->PluginGuid = pd->PluginId;
  td->interval = luaL_checkinteger(L, 1);
  if (td->interval < 0) td->interval = 0;

  lua_pushvalue(L, -1);
  td->objRef = luaL_ref(L, LUA_REGISTRYINDEX);

  luaL_checktype(L, 2, LUA_TFUNCTION);
  lua_pushvalue(L, 2);
  td->funcRef = luaL_ref(L, LUA_REGISTRYINDEX);

  td->tStart = ft;
  td->interval_changed = 1;
  td->needClose = 0;
  td->enabled = 1;

  td->hThread = CreateThread(NULL, 0, TimerThreadFunc, td, 0, &td->threadId);
  if (td->hThread != NULL) {
    luaL_getmetatable(L, FarTimerType);
    lua_setmetatable(L, -2);
    return 1;
  }
  luaL_unref(L, LUA_REGISTRYINDEX, td->objRef);
  luaL_unref(L, LUA_REGISTRYINDEX, td->funcRef);
  return lua_pushnil(L), 1;
}

TTimerData* CheckTimer(lua_State* L, int pos)
{
  return (TTimerData*)luaL_checkudata(L, pos, FarTimerType);
}

TTimerData* CheckValidTimer(lua_State* L, int pos)
{
  TTimerData* td = CheckTimer(L, pos);
  luaL_argcheck(L, td->needClose == 0, pos, "attempt to access closed timer");
  return td;
}

static int timer_Close (lua_State *L)
{
  TTimerData* td = CheckTimer(L, 1);
  td->needClose = 1;
  return 0;
}

static int timer_tostring (lua_State *L)
{
  TTimerData* td = CheckTimer(L, 1);
  if (td->needClose == 0)
    lua_pushfstring(L, "%s (%p)", FarTimerType, td);
  else
    lua_pushfstring(L, "%s (closed)", FarTimerType);
  return 1;
}

static int timer_index (lua_State *L)
{
  TTimerData* td = CheckTimer(L, 1);
  const char* method = luaL_checkstring(L, 2);
  if      (!strcmp(method, "Close"))       lua_pushcfunction(L, timer_Close);
  else if (!strcmp(method, "Enabled"))     lua_pushboolean(L, td->enabled);
  else if (!strcmp(method, "Interval")) {
    while (td->interval_changed)
      SwitchToThread();
    lua_pushinteger(L, td->interval);
  }
  else if (!strcmp(method, "OnTimer"))     lua_rawgeti(L, LUA_REGISTRYINDEX, td->funcRef);
  else if (!strcmp(method, "Closed"))      lua_pushboolean(L, td->needClose);
  else                                     luaL_error(L, "attempt to call non-existent method");
  return 1;
}

static int timer_newindex (lua_State *L)
{
  TTimerData* td = CheckValidTimer(L, 1);
  const char* method = luaL_checkstring(L, 2);
  if (!strcmp(method, "Enabled")) {
    luaL_checkany(L, 3);
    td->enabled = lua_toboolean(L, 3);
  }
  else if (!strcmp(method, "Interval")) {
    int interval = luaL_checkinteger(L, 3);
    if (interval < 0) interval = 0;
    while (td->interval_changed)
      SwitchToThread();
    td->interval = interval;
    td->interval_changed = 1;
  }
  else if (!strcmp(method, "OnTimer")) {
    luaL_checktype(L, 3, LUA_TFUNCTION);
    lua_pushvalue(L, 3);
    lua_rawseti(L, LUA_REGISTRYINDEX, td->funcRef);
  }
  else luaL_error(L, "attempt to call non-existent method");
  return 0;
}

static int win_GetConsoleScreenBufferInfo (lua_State* L)
{
  CONSOLE_SCREEN_BUFFER_INFO info;
  HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
  if (!GetConsoleScreenBufferInfo(h, &info))
    return lua_pushnil(L), 1;
  lua_createtable(L, 0, 11);
  PutIntToTable(L, "SizeX",              info.dwSize.X);
  PutIntToTable(L, "SizeY",              info.dwSize.Y);
  PutIntToTable(L, "CursorPositionX",    info.dwCursorPosition.X);
  PutIntToTable(L, "CursorPositionY",    info.dwCursorPosition.Y);
  PutIntToTable(L, "Attributes",         info.wAttributes);
  PutIntToTable(L, "WindowLeft",         info.srWindow.Left);
  PutIntToTable(L, "WindowTop",          info.srWindow.Top);
  PutIntToTable(L, "WindowRight",        info.srWindow.Right);
  PutIntToTable(L, "WindowBottom",       info.srWindow.Bottom);
  PutIntToTable(L, "MaximumWindowSizeX", info.dwMaximumWindowSize.X);
  PutIntToTable(L, "MaximumWindowSizeY", info.dwMaximumWindowSize.Y);
  return 1;
}

static int win_CopyFile (lua_State *L)
{
  const wchar_t* src = check_utf8_string(L, 1, NULL);
  const wchar_t* trg = check_utf8_string(L, 2, NULL);

  BOOL fail_if_exists = FALSE; // default = overwrite the target
  if(lua_gettop(L) > 2)
    fail_if_exists = lua_toboolean(L,3);

  if (CopyFileW(src, trg, fail_if_exists))
    return lua_pushboolean(L, 1), 1;
  return SysErrorReturn(L);
}

static int win_MoveFile (lua_State *L)
{
  const wchar_t* src = check_utf8_string(L, 1, NULL);
  const wchar_t* trg = check_utf8_string(L, 2, NULL);
  const char* sFlags = luaL_optstring(L, 3, NULL);
  int flags = 0;
  if (sFlags) {
    if      (strchr(sFlags, 'c')) flags |= MOVEFILE_COPY_ALLOWED;
    else if (strchr(sFlags, 'd')) flags |= MOVEFILE_DELAY_UNTIL_REBOOT;
    else if (strchr(sFlags, 'r')) flags |= MOVEFILE_REPLACE_EXISTING;
    else if (strchr(sFlags, 'w')) flags |= MOVEFILE_WRITE_THROUGH;
  }
  if (MoveFileExW(src, trg, flags))
    return lua_pushboolean(L, 1), 1;
  return SysErrorReturn(L);
}

static int win_DeleteFile (lua_State *L)
{
  if (DeleteFileW(check_utf8_string(L, 1, NULL)))
    return lua_pushboolean(L, 1), 1;
  return SysErrorReturn(L);
}

BOOL dir_exist(const wchar_t* path)
{
  DWORD attr = GetFileAttributesW(path);
  return (attr != 0xFFFFFFFF) && (attr & FILE_ATTRIBUTE_DIRECTORY);
}

BOOL mkdir (const wchar_t* path)
{
  BOOL result = FALSE;
  const wchar_t* src = path;
  wchar_t *p = _wcsdup(path), *trg = p, *q;
  while (*src) {
    if (*src == L'\\' || *src == L'/') {
      *trg++ = L'\\';
      do src++; while (*src == L'\\' || *src == L'/');
    }
    else *trg++ = *src++;
  }
  if (trg > p && trg[-1] == '\\') trg--;
  *trg = 0;

  for (q=p; *q; *q++=L'\\') {
    q = wcschr(q, L'\\');
    if (q != NULL)  *q = 0;
    if (q != p && !dir_exist(p) && !CreateDirectoryW(p, NULL)) break;
    if (q == NULL) { result=TRUE; break; }
  }
  free(p);
  return result;
}

static int win_CreateDir (lua_State *L)
{
  const wchar_t* path = check_utf8_string(L, 1, NULL);
  BOOL tolerant = lua_toboolean(L, 2);
  if (dir_exist(path)) {
    if (tolerant) return lua_pushboolean(L,1), 1;
    return lua_pushnil(L), lua_pushliteral(L, "directory already exists"), 2;
  }
  if (mkdir(path))
    return lua_pushboolean(L, 1), 1;
  return SysErrorReturn(L);
}

static int win_RemoveDir (lua_State *L)
{
  if (RemoveDirectoryW(check_utf8_string(L, 1, NULL)))
    return lua_pushboolean(L, 1), 1;
  return SysErrorReturn(L);
}

static int win_ShellExecute (lua_State *L)
{
  HWND hwnd = lua_isuserdata(L, 1) ? lua_touserdata(L, 1) : NULL;
  const wchar_t* lpOperation = opt_utf8_string(L, 2, NULL);
  const wchar_t* lpFile = check_utf8_string(L, 3, NULL);
  const wchar_t* lpParameters = opt_utf8_string(L, 4, NULL);
  const wchar_t* lpDirectory = opt_utf8_string(L, 5, NULL);
  INT nShowCmd = luaL_optinteger(L, 6, SW_SHOWNORMAL);

  HINSTANCE hinst = ShellExecuteW(
    hwnd,           // handle to parent window
    lpOperation,    // pointer to string that specifies operation to perform
    lpFile,         // pointer to filename or folder name string
    lpParameters,   // pointer to string that specifies executable-file parameters
    lpDirectory,    // pointer to string that specifies default directory
    nShowCmd        // whether file is shown when opened
  );
  lua_pushinteger (L, (INT_PTR)hinst);
  return 1;
}

typedef struct {
  HANDLE Handle;
  BOOL IsFarSettings;
} FarSettingsUdata;


static int far_CreateSettings (lua_State *L)
{
  static const char FarGuid[sizeof(GUID)] = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
  size_t len = 0;
  const char* strId;
  const GUID* ParamId;
  FarSettingsUdata *udata;
  struct FarSettingsCreate fsc;
  BOOL IsFarSettings = 0;
  TPluginData *pd = GetPluginData(L);
  int location;

  strId = luaL_optlstring(L, 1, NULL, &len);
  if (strId == NULL)
    ParamId = pd->PluginId;
  else {
    if (len == 3 && strcmp(strId, "far") == 0)
      IsFarSettings = 1;
    else if (len == sizeof(GUID))
      IsFarSettings = !memcmp(strId, FarGuid, len);
    else {
      lua_pushnil(L);
      return 1;
    }
    ParamId = CAST(const GUID*, (IsFarSettings ? FarGuid : strId));
  }
  location = OptFlags(L, 2, PSL_ROAMING);

  fsc.StructSize = sizeof(fsc);
  fsc.Guid = *ParamId;
  if (!pd->Info->SettingsControl(INVALID_HANDLE_VALUE, SCTL_CREATE, location, &fsc)) {
    lua_pushnil(L);
    return 1;
  }

  lua_getfield(L, LUA_REGISTRYINDEX, SettingsHandles);

  udata = (FarSettingsUdata*)lua_newuserdata(L, sizeof(FarSettingsUdata));
  udata->Handle = fsc.Handle;
  udata->IsFarSettings = IsFarSettings;
  luaL_getmetatable(L, SettingsType);
  lua_setmetatable(L, -2);

  lua_pushvalue(L, -1);
  lua_pushinteger(L, 1);
  lua_rawset(L, -4);
  return 1;
}

static FarSettingsUdata* GetSettingsUdata (lua_State *L, int pos)
{
  return luaL_checkudata(L, pos, SettingsType);
}

static FarSettingsUdata* CheckSettings (lua_State *L, int pos)
{
  FarSettingsUdata* udata = GetSettingsUdata(L, pos);
  if (udata->Handle == INVALID_HANDLE_VALUE) {
    const char* s = lua_pushfstring(L, "attempt to access a closed %s", SettingsType);
    luaL_argerror(L, pos, s);
  }
  return udata;
}

static int Settings_set (lua_State *L)
{
  struct FarSettingsItem fsi;
  FarSettingsUdata* udata = CheckSettings(L, 1);
  fsi.Root = luaL_checkinteger(L, 2);
  fsi.Name = opt_utf8_string(L, 3, NULL);
  fsi.Type = (enum FARSETTINGSTYPES) check_env_flag(L, 4);

  if (fsi.Type == FST_QWORD)
    fsi.Value.Number = GetFlagCombination(L, 5, NULL);
  else if (fsi.Type == FST_STRING)
    fsi.Value.String = check_utf8_string(L, 5, NULL);
  else if (fsi.Type == FST_DATA)
    fsi.Value.Data.Data = luaL_checklstring(L, 5, &fsi.Value.Data.Size);
  else
    return lua_pushboolean(L,0), 1;

  lua_pushboolean(L, GetPluginData(L)->Info->SettingsControl(udata->Handle, SCTL_SET, 0, &fsi));
  return 1;
}

static int Settings_get (lua_State *L)
{
  struct FarSettingsItem fsi;
  FarSettingsUdata* udata = CheckSettings(L, 1);
  fsi.Root = luaL_checkinteger(L, 2);
  fsi.Name = check_utf8_string(L, 3, NULL);
  fsi.Type = (enum FARSETTINGSTYPES) check_env_flag(L, 4);
  if (GetPluginData(L)->Info->SettingsControl(udata->Handle, SCTL_GET, 0, &fsi)) {
    if (fsi.Type == FST_QWORD)
      push64(L, fsi.Value.Number);
    else if (fsi.Type == FST_STRING)
      push_utf8_string(L, fsi.Value.String, -1);
    else if (fsi.Type == FST_DATA)
      lua_pushlstring(L, fsi.Value.Data.Data, fsi.Value.Data.Size);
    else
      lua_pushnil(L);
  }
  else
    lua_pushnil(L);
  return 1;
}

static int Settings_delete (lua_State *L)
{
  struct FarSettingsValue fsv;
  FarSettingsUdata* udata = CheckSettings(L, 1);
  fsv.Root = luaL_checkinteger(L, 2);
  fsv.Value = opt_utf8_string(L, 3, NULL);
  lua_pushboolean(L, GetPluginData(L)->Info->SettingsControl(udata->Handle, SCTL_DELETE, 0, &fsv));
  return 1;
}

static int Settings_createsubkey (lua_State *L)
{
  PSInfo *Info = GetPluginData(L)->Info;
  const wchar_t *description;
  struct FarSettingsValue fsv;
  int subkey;
  FarSettingsUdata* udata = CheckSettings(L, 1);
  fsv.Root = luaL_checkinteger(L, 2);
  fsv.Value = check_utf8_string(L, 3, NULL);
  description = opt_utf8_string(L, 4, NULL);

  subkey = Info->SettingsControl(udata->Handle, SCTL_CREATESUBKEY, 0, &fsv);
  if (subkey != 0) {
    if (description != NULL) {
      struct FarSettingsItem fsi;
      fsi.Root = subkey;
      fsi.Name = NULL;
      fsi.Type = FST_STRING;
      fsi.Value.String = description;
      Info->SettingsControl(udata->Handle, SCTL_SET, 0, &fsi);
    }
    lua_pushinteger(L, subkey);
  }
  else
    lua_pushnil(L);
  return 1;
}

static int Settings_opensubkey (lua_State *L)
{
  int subkey;
  struct FarSettingsValue fsv;
  FarSettingsUdata* udata = CheckSettings(L, 1);
  fsv.Root = luaL_checkinteger(L, 2);
  fsv.Value = check_utf8_string(L, 3, NULL);

  subkey = GetPluginData(L)->Info->SettingsControl(udata->Handle, SCTL_OPENSUBKEY, 0, &fsv);
  if (subkey != 0)
    lua_pushinteger(L, subkey);
  else
    lua_pushnil(L);
  return 1;
}

static int Settings_enum (lua_State *L)
{
  struct FarSettingsEnum fse;
  int i, from = 1, to = -1;
  FarSettingsUdata* udata = CheckSettings(L, 1);
  fse.Root = luaL_checkinteger(L, 2);
  if (!lua_isnoneornil(L, 3))  from = luaL_checkinteger(L, 3);
  if (!lua_isnoneornil(L, 4))  to = luaL_checkinteger(L, 4);
  if (GetPluginData(L)->Info->SettingsControl(udata->Handle, SCTL_ENUM, 0, &fse)) {
    if (from < 1 && (from += fse.Count + 1) < 1) from = 1;
    --from;
    if (to < 0 && (to += fse.Count + 1) < 0) to = 0;
    if (to > (int)fse.Count) to = fse.Count;
    lua_createtable(L, fse.Count, 1);
    PutIntToTable(L, "Count", fse.Count);
    for (i = from; i < to; i++) {
      if (udata->IsFarSettings) {
        const struct FarSettingsHistory *fsh = fse.Value.Histories + i;
        lua_createtable(L, 0, 6);
        if (fsh->Name) PutWStrToTable(L, "Name", fsh->Name, -1);
        if (fsh->Param) PutWStrToTable(L, "Param", fsh->Param, -1);
        PutLStrToTable(L, "PluginId", &fsh->PluginId, sizeof(GUID));
        if (fsh->File) PutWStrToTable(L, "File", fsh->File, -1);
        pushFileTime(L, &fsh->Time);
        lua_setfield(L, -2, "Time");
        PutBoolToTable(L, "Lock", fsh->Lock);
      }
      else {
        lua_createtable(L, 0, 2);
        PutWStrToTable(L, "Name", fse.Value.Items[i].Name, -1);
        PutIntToTable(L, "Type", fse.Value.Items[i].Type);
      }
      lua_rawseti(L, -2, i-from+1);
    }
  }
  else
    lua_pushnil(L);
  return 1;
}

static int Settings_free (lua_State *L)
{
  FarSettingsUdata* udata = GetSettingsUdata(L, 1);
  if (udata->Handle != INVALID_HANDLE_VALUE) {
    PSInfo *Info = GetPluginData(L)->Info;
    Info->SettingsControl(udata->Handle, SCTL_FREE, 0, 0);
    udata->Handle = INVALID_HANDLE_VALUE;

    lua_getfield(L, LUA_REGISTRYINDEX, SettingsHandles);
    lua_pushvalue(L, 1);
    lua_pushnil(L);
    lua_rawset(L, -3);
  }
  return 0;
}

int far_FreeSettings (lua_State *L)
{
  lua_getfield(L, LUA_REGISTRYINDEX, SettingsHandles);
  lua_pushnil(L);
  while (lua_next(L, -2)) {
    lua_pushcfunction(L, Settings_free);
    lua_pushvalue(L, -3);
    lua_call(L, 1, 0);
    lua_pop(L, 1);
  }
  lua_pop(L, 1); // mandatory, since this function is called directly from pcall_msg
  return 0;
}

static int Settings_tostring (lua_State *L)
{
  FarSettingsUdata* udata = GetSettingsUdata(L, 1);
  if (udata->Handle != INVALID_HANDLE_VALUE)
    lua_pushfstring(L, "%s (%p)", SettingsType, udata->Handle);
  else
    lua_pushfstring(L, "%s (closed)", SettingsType);
  return 1;
}

static int far_ColorDialog (lua_State *L)
{
  UINT64 Flags;
  struct FarColor Color;
  TPluginData *pd = GetPluginData(L);
  int istable = lua_istable(L, 1);

  if (istable)
    GetFarColorFromTable(L, 1, &Color);
  else {
    int code = luaL_optinteger(L, 1, 0x0F);
    Color.ForegroundColor = code & 0x0F;
    Color.BackgroundColor = (code & 0xF0) >> 4;
    Color.Flags = (FCF_FG_4BIT | FCF_BG_4BIT);
  }
  Flags = CheckFlags(L, 2);

  if (pd->Info->ColorDialog(pd->PluginId, Flags, &Color)) {
    if (istable) PushFarColor(L, &Color);
    else lua_pushinteger(L, Color.ForegroundColor | (Color.BackgroundColor << 4));
  }
  else
    lua_pushnil(L);
  return 1;
}

const luaL_Reg timer_methods[] = {
  {"__gc",                timer_Close},
  {"__tostring",          timer_tostring},
  {"__index",             timer_index},
  {"__newindex",          timer_newindex},
  {NULL, NULL},
};

const luaL_Reg filefilter_methods[] = {
  {"__gc",                filefilter_gc},
  {"__tostring",          filefilter_tostring},
  {"FreeFileFilter",      filefilter_Free},
  {"OpenFiltersMenu",     filefilter_OpenMenu},
  {"StartingToFilter",    filefilter_Starting},
  {"IsFileInFilter",      filefilter_IsFileInFilter},
  {NULL, NULL},
};

const luaL_Reg dialog_methods[] = {
  {"__gc",                far_DialogFree},
  {"__tostring",          dialog_tostring},
  {NULL, NULL},
};

const luaL_Reg Settings_methods[] = {
  {"__gc",                Settings_free},
  {"__tostring",          Settings_tostring},
  {"Delete",              Settings_delete},
  {"Enum",                Settings_enum},
  {"Free",                Settings_free},
  {"Get",                 Settings_get},
  {"Set",                 Settings_set},
  {"CreateSubkey",        Settings_createsubkey},
  {"OpenSubkey",          Settings_opensubkey},
  {NULL, NULL},
};

const luaL_Reg editor_funcs[] = {
  {"AddColor",            editor_AddColor},
  {"AddSessionBookmark",  editor_AddSessionBookmark},
  {"ClearSessionBookmarks", editor_ClearSessionBookmarks},
  {"DelColor",            editor_DelColor},
  {"DeleteBlock",         editor_DeleteBlock},
  {"DeleteChar",          editor_DeleteChar},
  {"DeleteSessionBookmark", editor_DeleteSessionBookmark},
  {"DeleteString",        editor_DeleteString},
  {"Editor",              editor_Editor},
  {"ExpandTabs",          editor_ExpandTabs},
  {"GetBookmarks",        editor_GetBookmarks},
  {"GetColor",            editor_GetColor},
  {"GetFileName",         editor_GetFileName},
  {"GetInfo",             editor_GetInfo},
  {"GetSelection",        editor_GetSelection},
  {"GetSessionBookmarks", editor_GetSessionBookmarks},
  {"GetString",           editor_GetString},
  {"GetStringW",          editor_GetStringW},
  {"InsertString",        editor_InsertString},
  {"InsertText",          editor_InsertText},
  {"InsertTextW",         editor_InsertTextW},
  {"NextSessionBookmark", editor_NextSessionBookmark},
  {"PrevSessionBookmark", editor_PrevSessionBookmark},
  {"ProcessInput",        editor_ProcessInput},
  {"Quit",                editor_Quit},
  {"ReadInput",           editor_ReadInput},
  {"RealToTab",           editor_RealToTab},
  {"Redraw",              editor_Redraw},
  {"SaveFile",            editor_SaveFile},
  {"Select",              editor_Select},
  {"SetKeyBar",           editor_SetKeyBar},
  {"SetParam",            editor_SetParam},
  {"SetPosition",         editor_SetPosition},
  {"SetString",           editor_SetString},
  {"SetStringW",          editor_SetStringW},
  {"SetTitle",            editor_SetTitle},
  {"TabToReal",           editor_TabToReal},
  {"TurnOffMarkingBlock", editor_TurnOffMarkingBlock},
  {"UndoRedo",            editor_UndoRedo},
  {NULL, NULL},
};

const luaL_Reg viewer_funcs[] = {
  {"GetInfo",             viewer_GetInfo},
  {"Quit",                viewer_Quit},
  {"Redraw",              viewer_Redraw},
  {"Select",              viewer_Select},
  {"SetKeyBar",           viewer_SetKeyBar},
  {"SetMode",             viewer_SetMode},
  {"SetPosition",         viewer_SetPosition},
  {"Viewer",              viewer_Viewer},
  {NULL, NULL},
};

const luaL_Reg panel_funcs[] = {
//{"Control",             far_Control}, // done as multiple functions
  {"CheckPanelsExist",    panel_CheckPanelsExist},
  {"ClearSelection",      panel_ClearSelection},
  {"ClosePanel",          panel_ClosePanel},
  {"GetCmdLine",          panel_GetCmdLine},
  {"GetCmdLinePos",       panel_GetCmdLinePos},
  {"GetCmdLineSelection", panel_GetCmdLineSelection},
  {"GetColumnTypes",      panel_GetColumnTypes},
  {"GetColumnWidths",     panel_GetColumnWidths},
  {"GetCurrentPanelItem", panel_GetCurrentPanelItem},
  {"GetPanelDirectory",   panel_GetPanelDirectory},
  {"GetPanelFormat",      panel_GetPanelFormat},
  {"GetPanelHostFile",    panel_GetPanelHostFile},
  {"GetPanelInfo",        panel_GetPanelInfo},
  {"GetPanelItem",        panel_GetPanelItem},
  {"GetPanelPrefix",      panel_GetPanelPrefix},
  {"GetSelectedPanelItem", panel_GetSelectedPanelItem},
  {"GetUserScreen",       panel_GetUserScreen},
  {"InsertCmdLine",       panel_InsertCmdLine},
  {"IsActivePanel",       panel_IsActivePanel},
  {"RedrawPanel",         panel_RedrawPanel},
  {"SetCaseSensitiveSort", panel_SetCaseSensitiveSort},
  {"SetCmdLine",          panel_SetCmdLine},
  {"SetCmdLinePos",       panel_SetCmdLinePos},
  {"SetCmdLineSelection", panel_SetCmdLineSelection},
  {"SetNumericSort",      panel_SetNumericSort},
  {"SetPanelDirectory",   panel_SetPanelDirectory},
  {"SetSelection",        panel_SetSelection},
  {"SetSortMode",         panel_SetSortMode},
  {"SetSortOrder",        panel_SetSortOrder},
  {"SetUserScreen",       panel_SetUserScreen},
  {"SetViewMode",         panel_SetViewMode},
  {"UpdatePanel",         panel_UpdatePanel},
  {NULL, NULL},
};

const luaL_Reg far_funcs[] = {
  {"PluginStartupInfo",   far_PluginStartupInfo},

  {"DialogInit",          far_DialogInit},
  {"DialogRun",           far_DialogRun},
  {"DialogFree",          far_DialogFree},
  {"SendDlgMessage",      far_SendDlgMessage},
  {"GetDlgItem",          far_GetDlgItem},
  {"SetDlgItem",          far_SetDlgItem},
  {"GetDirList",          far_GetDirList},
  {"GetMsg",              far_GetMsg},
  {"GetPluginDirList",    far_GetPluginDirList},
  {"Menu",                far_Menu},
  {"Message",             far_Message},
  {"RestoreScreen",       far_RestoreScreen},
  {"SaveScreen",          far_SaveScreen},
  {"Text",                far_Text},
  {"ShowHelp",            far_ShowHelp},
  {"InputBox",            far_InputBox},
  {"AdvControl",          far_AdvControl},
  {"MacroLoadAll",        far_MacroLoadAll},
  {"MacroSaveAll",        far_MacroSaveAll},
  {"MacroGetState",       far_MacroGetState},
  {"MacroGetArea",        far_MacroGetArea},
  {"MacroPost",           far_MacroPost},
  {"MacroCheck",          far_MacroCheck},
  {"MacroAdd",            far_MacroAdd},
  {"MacroDelete",         far_MacroDelete},
  {"MacroGetLastError",   far_MacroGetLastError},
  {"DefDlgProc",          far_DefDlgProc},
  {"CreateFileFilter",    far_CreateFileFilter},
  {"LoadPlugin",          far_LoadPlugin},
  {"UnloadPlugin",        far_UnloadPlugin},
  {"ForcedLoadPlugin",    far_ForcedLoadPlugin},
  {"FindPlugin",          far_FindPlugin},
  {"GetPluginInformation",far_GetPluginInformation},
  {"GetPlugins",          far_GetPlugins},
  {"CreateSettings",      far_CreateSettings},
  {"FreeSettings",        far_FreeSettings},
  {"ColorDialog",         far_ColorDialog},

  /* FUNCTIONS ADDED FOR VARIOUS REASONS */
  {"CopyToClipboard",     far_CopyToClipboard},
  {"PasteFromClipboard",  far_PasteFromClipboard},
  {"InputRecordToName",   far_InputRecordToName},
  {"NameToInputRecord",   far_NameToInputRecord},
  {"LStricmp",            far_LStricmp},
  {"LStrnicmp",           far_LStrnicmp},
  {"ProcessName",         far_ProcessName},
  {"GetPathRoot",         far_GetPathRoot},
  {"GetReparsePointInfo", far_GetReparsePointInfo},
  {"LIsAlpha",            far_LIsAlpha},
  {"LIsAlphanum",         far_LIsAlphanum},
  {"LIsLower",            far_LIsLower},
  {"LIsUpper",            far_LIsUpper},
  {"LLowerBuf",           far_LLowerBuf},
  {"LUpperBuf",           far_LUpperBuf},
  {"MkTemp",              far_MkTemp},
  {"MkLink",              far_MkLink},
  {"TruncPathStr",        far_TruncPathStr},
  {"TruncStr",            far_TruncStr},
  {"RecursiveSearch",     far_RecursiveSearch},
  {"ConvertPath",         far_ConvertPath},
  {"XLat",                far_XLat},
  {"FormatFileSize",      far_FormatFileSize},

  {"CPluginStartupInfo",  far_CPluginStartupInfo},
  {"GetCurrentDirectory", far_GetCurrentDirectory},
  {"GetFileOwner",        far_GetFileOwner},
  {"GetNumberOfLinks",    far_GetNumberOfLinks},
  {"LuafarVersion",       far_LuafarVersion},
  {"MakeMenuItems",       far_MakeMenuItems},
  {"Show",                far_Show},
  {"Timer",               far_Timer},

  {NULL, NULL}
};

const luaL_Reg win_funcs[] = {
  {"CompareString",       win_CompareString},
  {"CopyFile",            win_CopyFile},
  {"CreateDir",           win_CreateDir},
  {"DeleteFile",          win_DeleteFile},
  {"DeleteRegKey",        win_DeleteRegKey},
  {"ExtractKey",          win_ExtractKey},
  {"FileTimeToLocalFileTime", win_FileTimeToLocalFileTime},
  {"FileTimeToSystemTime",win_FileTimeToSystemTime},
  {"GetConsoleScreenBufferInfo", win_GetConsoleScreenBufferInfo},
  {"GetEnv",              win_GetEnv},
  {"GetFileInfo",         win_GetFileInfo},
  {"GetRegKey",           win_GetRegKey},
  {"GetSystemTimeAsFileTime", win_GetSystemTimeAsFileTime},
  {"GetVirtualKeys",      win_GetVirtualKeys},
  {"MoveFile",            win_MoveFile},
  {"RemoveDir",           win_RemoveDir},
  {"RenameFile",          win_MoveFile}, // alias
  {"SetEnv",              win_SetEnv},
  {"SetRegKey",           win_SetRegKey},
  {"ShellExecute",        win_ShellExecute},
  {"SystemTimeToFileTime",win_SystemTimeToFileTime},
  {"wcscmp",              win_wcscmp},

  {"EnumSystemCodePages", ustring_EnumSystemCodePages},
  {"GetACP",              ustring_GetACP},
  {"GetCPInfo",           ustring_GetCPInfo},
  {"GetDriveType",        ustring_GetDriveType},
  {"GetFileAttr",         ustring_GetFileAttr},
  {"GetLogicalDriveStrings",ustring_GetLogicalDriveStrings},
  {"GetOEMCP",            ustring_GetOEMCP},
  {"GlobalMemoryStatus",  ustring_GlobalMemoryStatus},
  {"MultiByteToWideChar", ustring_MultiByteToWideChar },
  {"OemToUtf8",           ustring_OemToUtf8},
  {"SHGetFolderPath",     ustring_SHGetFolderPath},
  {"SearchPath",          ustring_SearchPath},
  {"SetFileAttr",         ustring_SetFileAttr},
  {"Sleep",               ustring_Sleep},
  {"Utf16ToUtf8",         ustring_Utf16ToUtf8},
  {"Utf8ToOem",           ustring_Utf8ToOem},
  {"Utf8ToUtf16",         ustring_Utf8ToUtf16},
  {"Uuid",                ustring_Uuid},
  {"WideCharToMultiByte", ustring_WideCharToMultiByte},
  {"subW",                ustring_sub},
  {"lenW",                ustring_len},

  {NULL, NULL}
};

const char far_Dialog[] =
"function far.Dialog (Id,X1,Y1,X2,Y2,HelpTopic,Items,Flags,DlgProc)\n\
  local hDlg = far.DialogInit(Id,X1,Y1,X2,Y2,HelpTopic,Items,Flags,DlgProc)\n\
  if hDlg == nil then return nil end\n\
\n\
  local ret = far.DialogRun(hDlg)\n\
  for i, item in ipairs(Items) do\n\
    local newitem = far.GetDlgItem(hDlg, i-1)\n\
    if type(item[6]) == 'table' then\n\
      local pos = far.SendDlgMessage(hDlg, 'DM_LISTGETCURPOS', i-1, 0)\n\
      item[6].SelectIndex = pos.SelectPos\n\
    else\n\
      item[6] = newitem[6]\n\
    end\n\
    item[10] = newitem[10]\n\
  end\n\
\n\
  far.DialogFree(hDlg)\n\
  return ret\n\
end";

static int luaopen_far (lua_State *L)
{
  NewVirtualKeyTable(L, FALSE);
  lua_setfield(L, LUA_REGISTRYINDEX, FAR_VIRTUALKEYS);

  luaL_register(L, "far", far_funcs);
  push_flags_table(L);
  lua_pushvalue(L, -1);
  lua_setfield(L, -3, "Flags");
  lua_setfield(L, LUA_REGISTRYINDEX, FAR_FLAGSTABLE);
  SetFarColors(L);

  luaL_register(L, "editor", editor_funcs);
  luaL_register(L, "viewer", viewer_funcs);
  luaL_register(L, "panel",  panel_funcs);

  luaL_newmetatable(L, FarFileFilterType);
  lua_pushvalue(L,-1);
  lua_setfield(L, -2, "__index");
  luaL_register(L, NULL, filefilter_methods);

  luaL_newmetatable(L, FarTimerType);
  luaL_register(L, NULL, timer_methods);

  luaL_newmetatable(L, FarDialogType);
  lua_pushvalue(L,-1);
  lua_setfield(L, -2, "__index");
  luaL_register(L, NULL, dialog_methods);

  luaL_newmetatable(L, SettingsType);
  lua_pushvalue(L,-1);
  lua_setfield(L, -2, "__index");
  luaL_register(L, NULL, Settings_methods);

  lua_newtable(L);
  lua_setfield(L, LUA_REGISTRYINDEX, SettingsHandles);

  (void) luaL_dostring(L, far_Dialog);

  luaL_newmetatable(L, PluginHandleType);

  return 0;
}

// Run default script
BOOL LF_RunDefaultScript(lua_State* L)
{
  int pos = lua_gettop (L);
  int status = 1, embedded, i;
  wchar_t *defscript;
  FILE *fp = NULL;
  const char *name = "<boot";
  PSInfo *Info = GetPluginData(L)->Info;
  const wchar_t delims[] = L".-";

  // First: try to load the default script embedded into the plugin.
  // For speed, prevent calling require() on non-embedded plugins.
  lua_getglobal(L, "package");
  lua_getfield(L, -1, "preload");
  lua_getfield(L, -1, name);
  embedded = !lua_isnil(L, -1);
  lua_pop(L, 3);
  if (embedded) {
    lua_getglobal(L, "require");
    lua_pushstring(L, name);
    status = lua_pcall(L,1,1,0) || pcall_msg(L,0,0);
    lua_settop (L, pos);
    return !status;
  }

  // Second: try to load the default script from a disk file
  defscript = (wchar_t*)lua_newuserdata (L, sizeof(wchar_t) * (wcslen(Info->ModuleName) + 5));
  wcscpy(defscript, Info->ModuleName);

  for (i=0; delims[i]; i++) {
    wchar_t *end = wcsrchr(defscript, delims[i]);
    if (end) {
      wcscpy(end, L".lua");
      if ((fp = _wfopen(defscript, L"r")) != NULL)
        break;
    }
  }
  if (fp) {
    fclose(fp);
    status = LF_LoadFile(L, defscript);
    if (status == 0)
      status = pcall_msg(L,0,0);
    else
      LF_Error(L, utf8_to_utf16 (L, -1, NULL));
  }
  else
    LF_Error(L, L"Default script not found");

  lua_settop (L, pos);
  return (status == 0);
}

void LF_ProcessEnvVars (lua_State *L, const wchar_t* aEnvPrefix, const wchar_t* PluginDir)
{
  wchar_t bufName[256];
  const int VALSIZE = 16384;
  wchar_t* bufVal = NULL;

  if (aEnvPrefix && wcslen(aEnvPrefix) <=  DIM(bufName) - 8)
    bufVal = (wchar_t*)lua_newuserdata(L, VALSIZE*sizeof(wchar_t));

  if (bufVal) {
    int size;
    wcscpy(bufName, aEnvPrefix);
    wcscat(bufName, L"_PATH");
    size = GetEnvironmentVariableW (bufName, bufVal, VALSIZE);
    if (size && size < VALSIZE) {
      lua_getglobal(L, "package");
      push_utf8_string(L, bufVal, -1);
      lua_setfield(L, -2, "path");
      lua_pop(L,1);
    }
  }

  // prepend <plugin directory>\?.lua; to package.path
  lua_getglobal(L, "package");        //+1
  push_utf8_string(L, PluginDir, -1); //+2
  lua_pushliteral(L, "?.lua;");       //+3
  lua_getfield(L, -3, "path");        //+4
  lua_concat(L, 3);                   //+2
  lua_setfield(L, -2, "path");        //+1
  lua_pop(L, 1);

  if (bufVal) {
    int size;
    wcscpy(bufName, aEnvPrefix);
    wcscat(bufName, L"_CPATH");
    size = GetEnvironmentVariableW (bufName, bufVal, VALSIZE);
    if (size && size < VALSIZE) {
      lua_getglobal(L, "package");
      push_utf8_string(L, bufVal, -1);
      lua_setfield(L, -2, "cpath");
      lua_pop(L,1);
    }

    wcscpy(bufName, aEnvPrefix);
    wcscat(bufName, L"_INIT");
    size = GetEnvironmentVariableW (bufName, bufVal, VALSIZE);
    if (size && size < VALSIZE) {
      int status;
      if (*bufVal==L'@') {
        status = LF_LoadFile(L, bufVal+1) || lua_pcall(L,0,0,0);
      }
      else {
        push_utf8_string(L, bufVal, -1);
        status = luaL_loadstring(L, lua_tostring(L,-1)) || lua_pcall(L,0,0,0);
        lua_remove(L, status ? -2 : -1);
      }
      if (status) {
        LF_Error (L, check_utf8_string(L, -1, NULL));
        lua_pop(L,1);
      }
    }
    lua_pop(L, 1); // bufVal
  }
}

static const luaL_Reg lualibs[] = {
  {"", luaopen_base},
  {LUA_LOADLIBNAME, luaopen_upackage},  // changed
  {LUA_TABLIBNAME, luaopen_table},
  {LUA_IOLIBNAME, luaopen_uio},         // changed
  {LUA_OSLIBNAME, luaopen_os},
  {LUA_STRLIBNAME, luaopen_string},
  {LUA_MATHLIBNAME, luaopen_math},
  {LUA_DBLIBNAME, luaopen_debug},
  {NULL, NULL}
};

static void luaL_openlibs2 (lua_State *L) {
  HMODULE hLib;
  const luaL_Reg *lib = lualibs;
  for (; lib->func; lib++) {
    lua_pushcfunction(L, lib->func);
    lua_pushstring(L, lib->name);
    lua_call(L, 1, 0);
  }

  // Try to load LuaJIT 2.0 libraries. This is done dynamically to ensure that
  // LuaFAR works with either Lua 5.1 or LuaJIT 2.0
  hLib = GetModuleHandleA(LUADLL);
  if (hLib) {
    static const char* names[] = { "luaopen_bit", "luaopen_ffi", "luaopen_jit", NULL };
    const char** pName;
    for (pName=names; *pName; pName++) {
      lua_CFunction func = (lua_CFunction) GetProcAddress(hLib, *pName);
      if (func) {
        lua_pushcfunction(L, func);
        lua_pushstring(L, *pName + 8); // skip "luaopen_" prefix
        lua_call(L, 1, 0);
      }
    }
  }
}

void LF_InitLuaState1 (lua_State *L, lua_CFunction aOpenLibs)
{
  // open Lua libraries
  luaL_openlibs2(L);
  if (aOpenLibs) aOpenLibs(L);

  lua_createtable(L, 0, 10);
  lua_setglobal(L, "export");

  lua_pushcfunction(L, luaB_loadfileW);
  lua_setglobal(L, "loadfile");

  luaL_register(L, "win", win_funcs);
  lua_pop(L, 1);

  lua_pushcfunction(L, luaopen_bit64);
  lua_call(L, 0, 0);

  lua_pushcfunction(L, luaopen_unicode);
  lua_call(L, 0, 0);
}

static void* CustomAllocator (void *ud, void *ptr, size_t osize, size_t nsize)
{
  return ((TPluginData*)ud)->origAlloc(((TPluginData*)ud)->origUserdata, ptr, osize, nsize);
}

void LF_InitLuaState2 (lua_State *L, TPluginData *aInfo)
{
  aInfo->origAlloc = lua_getallocf(L, &aInfo->origUserdata);
  lua_setallocf(L, CustomAllocator, aInfo);

  // open "far" library
  lua_pushcfunction(L, luaopen_far);
  lua_call(L, 0, 0);

  // open "regex" library
  lua_pushcfunction(L, luaopen_regex);
  lua_pushliteral(L, "regex");
  lua_call(L, 1, 0);
}

lua_State* LF_LuaOpen()
{
  return luaL_newstate();
}

void LF_LuaClose (lua_State* L)
{
  lua_close(L);
}

