#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include "compat52.h"

#define MAX53 0x1FFFFFFFFFFFFFLL
typedef __int64 INT64;
typedef unsigned __int64 UINT64;

const char metatable_name[] = "64 bit integer";

static int push_new_userdata(lua_State *L, INT64 v)
{
  *(INT64*)lua_newuserdata(L, sizeof(INT64)) = v;
  luaL_getmetatable(L, metatable_name);
  lua_setmetatable(L, -2);
  return 1;
}

int bit64_push(lua_State *L, INT64 v)
{
  if ((v >= 0 && v <= MAX53) || (v < 0 && -v <= MAX53))
    lua_pushnumber(L, (double)v);
  else
    push_new_userdata(L, v);
  return 1;
}

int bit64_getvalue (lua_State *L, int pos, INT64 *target)
{
  if (lua_type(L,pos)==LUA_TUSERDATA) {
    int equal;
    lua_getmetatable(L, pos);
    luaL_getmetatable(L, metatable_name);
    equal = lua_rawequal(L,-1,-2);
    lua_pop(L,2);
    if (equal && target) {
      *target = *(INT64*)lua_touserdata(L,pos);
    }
    return equal;
  }
  return 0;
}

INT64 check64(lua_State *L, int pos, int* success)
{
  int tp = lua_type(L, pos);
  if (success) *success = 1;
  if (tp == LUA_TNUMBER) {
    double d = lua_tonumber(L, pos);
    if ((d >= 0 && d <= MAX53) || (d < 0 && -d <= MAX53))
      return d;
  }
  else {
    INT64 ret;
    if (bit64_getvalue(L, pos, &ret))
      return ret;
  }
  if (success) *success=0;
  else luaL_argerror(L, pos, "bad int64");
  return 0;
}

static int band(lua_State *L)
{
  int i;
  UINT64 v = 0xffffffffffffffffULL;
  int n = lua_gettop(L);
  for (i=0; i<n; i++)
    v &= check64(L, i+1, NULL);
  return bit64_push(L, v);
}

static int bor(lua_State *L)
{
  int i;
  UINT64 v = 0;
  int n = lua_gettop(L);
  for (i=0; i<n; i++)
    v |= check64(L, i+1, NULL);
  return bit64_push(L, v);
}

static int bxor(lua_State *L)
{
  int i;
  UINT64 v = 0;
  int n = lua_gettop(L);
  for (i=0; i<n; i++)
    v ^= check64(L, i+1, NULL);
  return bit64_push(L, v);
}

static int bnot(lua_State *L)
{
  luaL_checkany(L, 1);
  return bit64_push(L, ~check64(L, 1, NULL));
}

static int lshift(lua_State *L)
{
  UINT64 v = check64(L, 1, NULL);
  unsigned int n = luaL_checkinteger(L, 2);
  if (n > 64) n = 64;
  return bit64_push(L, v << n);
}

static int rshift(lua_State *L)
{
  UINT64 v = check64(L, 1, NULL);
  unsigned int n = luaL_checkinteger(L, 2);
  if (n > 64) n = 64;
  return bit64_push(L, v >> n);
}

static int arshift(lua_State *L)
{
  UINT64 v = check64(L, 1, NULL), res;
  unsigned int n = luaL_checkinteger(L, 2);
  if (n > 64) n = 64;
  res = (v >> n);
  if (v & 0x8000000000000000ULL)
    res |= (0xffffffffffffffffULL << (64-n));
  return bit64_push(L, res);
}

static int f_new (lua_State *L)
{
  int type = lua_type(L, 1);
  if (type == LUA_TSTRING) {
    size_t len;
    const char* s = lua_tolstring(L, 1, &len);
    if (len >= 3 && len <= 18 && s[0]=='0' && (s[1]=='x' || s[1]=='X')) {
      size_t i;
      UINT64 v = 0;
      len -= 2; s += 2;
      for (i=0; i<len; i++) {
        int a;
        if (s[i] >= '0' && s[i] <= '9')      a = s[i] - '0';
        else if (s[i] >= 'A' && s[i] <= 'F') a = s[i] + 10 - 'A';
        else if (s[i] >= 'a' && s[i] <= 'f') a = s[i] + 10 - 'a';
        else break;
        v = (v << 4) | a;
      }
      if (i == len)
        return push_new_userdata(L, v);
    }
  }
  else if (type == LUA_TNUMBER) {
    int success = 0;
    INT64 v = check64(L, 1, &success);
    if (success)
      return push_new_userdata(L, v);
  }
  else {
    INT64 v;
    if (bit64_getvalue (L, 1, &v))
      return push_new_userdata(L, v);
  }
  lua_pushnil(L);
  return 1;
}

static int f_tostring (lua_State *L)
{
  UINT64 v = check64(L,1,NULL);
  char buf[2+16] = "0x0";
  int i, j = 0;
  for (i=60; i >= 0; i -= 4) {
    int u = (v >> i) & 0xF;
    if (j == 0 && u != 0) j = 2;
    if (j) buf[j++] = (u<10) ? u+'0' : u-10+'A';
  }
  lua_pushlstring(L, buf, j == 0 ? 3 : j);
  return 1;
}

static int f_isint64 (lua_State *L)
{
  lua_pushboolean(L, bit64_getvalue(L,1,NULL));
  return 1;
}

static int f_add (lua_State *L)
{
  int i;
  INT64 v = 0;
  int n = lua_gettop(L);
  for (i=0; i<n; i++)
    v += check64(L, i+1, NULL);
  return bit64_push(L, v);
}

static int f_sub (lua_State *L)
{
  INT64 a1 = check64(L,1,NULL);
  INT64 a2 = check64(L,2,NULL);
  return bit64_push(L, a1-a2);
}

static int f_mul (lua_State *L)
{
  int i;
  INT64 v = 1;
  int n = lua_gettop(L);
  for (i=0; i<n; i++)
    v *= check64(L, i+1, NULL);
  return bit64_push(L, v);
}

static int f_div (lua_State *L)
{
  INT64 a1 = check64(L,1,NULL);
  INT64 a2 = check64(L,2,NULL);
  if (a2 != 0)
    bit64_push(L, a1/a2);
  else {
    if (a1 > 0)      bit64_push(L, 0x7FFFFFFFFFFFFFFFll);
    else if (a1 < 0) bit64_push(L, 0x8000000000000000ll);
    else             bit64_push(L, 1);
  }
  return 1;
}

static int f_mod (lua_State *L)
{
  INT64 a1 = check64(L,1,NULL);
  INT64 a2 = check64(L,2,NULL);
  if (a2==0) a2=1;
  return bit64_push(L, a1%a2);
}

static int f_unm (lua_State *L)
{
  INT64 a1 = check64(L,1,NULL);
  return bit64_push(L, -a1);
}

static int f_eq (lua_State *L)
{
  INT64 a1 = check64(L,1,NULL);
  INT64 a2 = check64(L,2,NULL);
  lua_pushboolean(L, a1==a2);
  return 1;
}

static int f_lt (lua_State *L)
{
  INT64 a1 = check64(L,1,NULL);
  INT64 a2 = check64(L,2,NULL);
  lua_pushboolean(L, a1<a2);
  return 1;
}

static int f_le (lua_State *L)
{
  INT64 a1 = check64(L,1,NULL);
  INT64 a2 = check64(L,2,NULL);
  lua_pushboolean(L, a1<=a2);
  return 1;
}

static const luaL_Reg funcs[] = {
  { "bnot",       bnot    },
  { "band",       band    },
  { "bor",        bor     },
  { "bxor",       bxor    },
  { "lshift",     lshift  },
  { "rshift",     rshift  },
  { "arshift",    arshift },

  { "new",        f_new   },
  { "add",        f_add   },
  { "sub",        f_sub   },
  { "mul",        f_mul   },
  { "div",        f_div   },
  { "mod",        f_mod   },
  { "isint64",    f_isint64 },

  { NULL,         NULL    },
};

static const luaL_Reg metamethods[] = {
  { "__add",      f_add   },
  { "__sub",      f_sub   },
  { "__mul",      f_mul   },
  { "__div",      f_div   },
  { "__mod",      f_mod   },
  { "__unm",      f_unm   },
  { "__eq",       f_eq    },
  { "__lt",       f_lt    },
  { "__le",       f_le    },
  { "__tostring", f_tostring },
  { NULL,         NULL    },
};

LUALIB_API int luaopen_bit64(lua_State *L)
{
  luaL_newmetatable(L, metatable_name);
  luaL_register(L, NULL, metamethods);
  luaL_register(L, "bit64", funcs);
  return 1;
}
