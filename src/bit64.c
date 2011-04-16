#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#define MAX32 0xffffffffULL
typedef unsigned __int64 UINT64;

int push64(lua_State *L, UINT64 v)
{
  if (v <= MAX32)
    lua_pushnumber(L, (double)v);
  else {
    char buf[2+16] = "0x0";
    int i, j = 0;
    for (i=60; i >= 0; i -= 4) {
      int u = (v >> i) & 0xF;
      if (j == 0 && u != 0) j = 2;
      if (j) buf[j++] = (u<10) ? u+'0' : u-10+'A';
    }
    lua_pushlstring(L, buf, j == 0 ? 3 : j);
  }
  return 1;
}

UINT64 check64(lua_State *L, int pos, int* success)
{
  int tp = lua_type(L, pos);
  if (success) *success = 1;
  if (tp == LUA_TNUMBER) {
    double d = lua_tonumber(L, pos);
    if (d <= MAX32) return (UINT64)d;
  }
  else if (tp == LUA_TSTRING) {
    size_t len;
    const char* s = lua_tolstring(L, pos, &len);
    if (len >= 3 && len <= 18 && s[0]=='0' && (s[1]=='x' || s[1]=='X')) {
      size_t i;
      len -= 2; s += 2;
      UINT64 v = 0;
      for (i=0; i<len; i++) {
        int a;
        if (s[i] >= '0' && s[i] <= '9')      a = s[i] - '0';
        else if (s[i] >= 'A' && s[i] <= 'F') a = s[i] + 10 - 'A';
        else if (s[i] >= 'a' && s[i] <= 'f') a = s[i] + 10 - 'a';
        else break;
        v = (v << 4) | a;
      }
      if (i == len) return v;
    }
  }
  if (success) *success=0;
  else luaL_argerror(L, pos, "bad flag64");
  return 0;
}

static int band(lua_State *L)
{
  int i;
  UINT64 v = 0xffffffffffffffffULL;
  int n = lua_gettop(L);
  for (i=0; i<n; i++)
    v &= check64(L, i+1, NULL);
  return push64(L, v);
}

static int bor(lua_State *L)
{
  int i;
  UINT64 v = 0;
  int n = lua_gettop(L);
  for (i=0; i<n; i++)
    v |= check64(L, i+1, NULL);
  return push64(L, v);
}

static int bxor(lua_State *L)
{
  int i;
  UINT64 v = 0;
  int n = lua_gettop(L);
  for (i=0; i<n; i++)
    v ^= check64(L, i+1, NULL);
  return push64(L, v);
}

static int bnot(lua_State *L)
{
  luaL_checkany(L, 1);
  return push64(L, ~check64(L, 1, NULL));
}

static int lshift(lua_State *L)
{
  UINT64 v = check64(L, 1, NULL);
  unsigned int n = luaL_checkinteger(L, 2);
  if (n > 64) n = 64;
  return push64(L, v << n);
}

static int rshift(lua_State *L)
{
  UINT64 v = check64(L, 1, NULL);
  unsigned int n = luaL_checkinteger(L, 2);
  if (n > 64) n = 64;
  return push64(L, v >> n);
}

static int arshift(lua_State *L)
{
  UINT64 v = check64(L, 1, NULL);
  unsigned int n = luaL_checkinteger(L, 2);
  if (n > 64) n = 64;
  UINT64 res = (v >> n);
  if (v & 0x8000000000000000ULL)
    res |= (0xffffffffffffffffULL << (64-n));
  return push64(L, res);
}

static const luaL_reg funcs[] = {
  { "bnot",    bnot    },
  { "band",    band    },
  { "bor",     bor     },
  { "bxor",    bxor    },
  { "lshift",  lshift  },
  { "rshift",  rshift  },
  { "arshift", arshift },
  { NULL,      NULL    },
};

LUALIB_API int luaopen_bit64(lua_State *L)
{
  luaL_register(L, "bit64", funcs);
  return 1;
}
