-- started: 2008-12-15 by Shmuel Zeigerman

local function extract_enums (src)
  local collector = {}
  for enum in src:gmatch("%senum%s*[%w_]*%s*(%b{})") do
    for line in enum:gmatch("[^\n]+") do
      if line:match("^%s*#") then table.insert(collector, line)
      else
        local var = line:match("^%s*([%a_][%w_]*)")
        if var then table.insert(collector, var) end
      end
    end
  end
  return collector
end

local sTmpFile = [[
#include <windows.h>
#include <stdio.h>
#include <farkeys.hpp>
#include <farcolor.hpp>

int main() {
]]

local sOutFile = [[
// This is a generated file.
#include <lua.h>
#include <lauxlib.h>

static const char keys[] = "return { $keys }";
static const char colors[] = "return { $colors }";

// output table is on stack top
void SetFarKeysAndColors (lua_State *L)
{
  luaL_loadstring(L, keys);
  lua_call(L, 0, 1);
  lua_setfield(L, -2, "Keys");
  luaL_loadstring(L, colors);
  lua_call(L, 0, 1);
  lua_setfield(L, -2, "Colors");
}
]]

local function get_insert (in_dir, src)
  local tb = { sTmpFile, }
  for _,v in ipairs(extract_enums(src)) do
    if v:match("^%s*#") then table.insert(tb, v.."\n")
    else table.insert(tb, ('  printf("%s=%%u,", (unsigned int)%s);\n'):format(v,v))
    end
  end
  table.insert(tb, "  return 0;\n}\n")

  local fp = assert(io.open("tmp.c", "w"))
  fp:write(table.concat(tb))
  fp:close()
  assert(0 == os.execute(("gcc -o tmp.exe -DWINVER=0x500 -I%s tmp.c"):format(in_dir)))

  fp = assert(io.popen("tmp.exe"))
  local str = fp:read("*all")
  fp:close()

  os.remove "tmp.exe"
  os.remove "tmp.c"
  return str
end

local function makefarkeys (in_dir, out_file, notall)
  local fp = assert(io.open(in_dir.."\\farkeys.hpp"))
  local src = fp:read("*all")
  fp:close()
  if notall then
    src = assert(src:match("\n%s*enum%s*BaseDefKeyboard.-\n%s*}%s*;"))
  end
  local out = sOutFile:gsub("$keys", get_insert(in_dir, src))

  fp = assert(io.open(in_dir.."\\farcolor.hpp"))
  src = fp:read("*all")
  fp:close()
  out = out:gsub("$colors", get_insert(in_dir, src))

  fp = io.open(out_file, "w")
  fp:write(out)
  fp:close()
end

local in_dir, out_file, notall = ...
assert(in_dir, "input directory not specified")
assert(out_file, "output file not specified")
makefarkeys(in_dir, out_file, notall=="-notall")
