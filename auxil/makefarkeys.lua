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

local cfile_top = [[
#include <windows.h>
#include <stdio.h>
#include "@@@"

int main() {
]]

local luafile_top = [[
-- This is a generated file.
-- Its input is "@@@".

return {
]]

local function makefarkeys (fname)
  assert(fname, "input file not specified")
  local fp = assert(io.open(fname))
  local src = fp:read("*all")
  fp:close()
  cfile_top = cfile_top:gsub("@@@", fname)
  local tb = { cfile_top }
  for _,v in ipairs(extract_enums(src)) do
    if v:match("^%s*#") then table.insert(tb, v.."\n")
    else table.insert(tb, ('  printf("  %s = 0x%%X,\\n", (unsigned int)%s);\n'):format(v,v))
    end
  end
  table.insert(tb, "  return 0;\n}\n")
  local src = table.concat(tb)

  luafile_top = luafile_top:gsub("@@@", (fname:match("[^\\/]+$")))
  io.write(luafile_top)
  io.flush()
  local fp = assert(io.open("tmp.c", "w"))
  fp:write(src)
  fp:close()
  assert(0 == os.execute("gcc -o tmp.exe tmp.c"))
  assert(0 == os.execute("tmp.exe"))
  io.write("}\n")
end

return makefarkeys
