function export.GetGlobalInfo()
  return {
    Version       = { 3, 0, 0, 0 },
    MinFarVersion = { 3, 0, 0, 2246 },
    Guid          = win.Uuid("4ae42d83-642e-4a2d-987e-28c2ee974ec1"),
    Title         = "Hello World",
    Description   = "A demo LuaFAR plugin",
    Author        = "Shmuel Zeigerman",
  }
end
