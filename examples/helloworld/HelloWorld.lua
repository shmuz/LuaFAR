-- "Hello World" - FAR plugin in Lua --

local mTitle, mLine1, mLine2, mButton = 0,1,2,3
local F = far.GetFlags()
local M = far.GetMsg
----------------------------------------------------------------------------

function far.GetPluginInfo()
  return {
    Flags = F.PF_EDITOR,
    PluginMenuStrings = { M(mTitle) },
  }
end


function far.OpenPlugin (OpenFrom, Item)
  local Text = "\n" .. M(mLine1) .. "\n" .. M(mLine2) .. "\n\n\1"
  far.Message (Text, M(mTitle), M(mButton), "w", "Contents")
end

