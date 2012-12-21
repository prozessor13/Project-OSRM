-- FIXME! cache data
require("math")
require("pack")

local _f = {}
local _size = 3600

local function fname(lat, lon)
  return "dem/" .. math.floor(lat) .. "_" .. math.floor(lon) .. ".bin"
end

function altitude(lat, lon)
  local fname = fname(lat, lon)
  if not _f[fname] then
    local f = io.open(fname ,"rb")
    if not f then
      return 0
    else
      _f[fname] = f
    end
  end

  local x = math.floor((lon - math.floor(lon)) * _size)
  local y = math.floor((lat - math.floor(lat)) * _size)
  _f[fname]:seek("set", (x * _size + y) * 2)
  local p, altitude = string.unpack(_f[fname]:read(2), "H<")
  return altitude
end
