-- "Hello World" - FAR plugin in Lua --

local mTitle, mLine1, mLine2, mButton = 0,1,2,3
local F = far.Flags
local M = far.GetMsg

local MenuGuid1 = win.Uuid("491f071d-a50a-4e18-95a0-241793c758c6")

function export.GetPluginInfo()
  return {
    Flags = F.PF_EDITOR,
    PluginMenuGuids   = MenuGuid1.."",
    PluginMenuStrings = { M(mTitle) },
  }
end

function export.Open (OpenFrom, Guid, Item)
  local Text = "\n" .. M(mLine1) .. "\n" .. M(mLine2) .. "\n\n\1"
  far.Message (Text, M(mTitle), M(mButton), "w", "Contents")
end

