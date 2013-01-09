require("math")
require("pack")

local _f = {}
local _size = 3600

local function _fh(lat, lon)
  local fname = "dem/" .. math.floor(lat) .. "_" .. math.floor(lon) .. ".bin"
  if _f[fname] == -1 then return 0 end
  if not _f[fname] then
    local f = io.open(fname ,"rb")
    if not f then
      print("missing DEM file: " .. fname)
      _f[fname] = -1
    else
      _f[fname] = f
    end
  end
  return _f[fname]
end

local function _altitude(lat, lon, x, y)
  if x >= _size then lon = lon + 1; x = x - _size end
  if y >= _size then lat = lat + 1; y = y - _size end
  local fh = _fh(lat, lon)
  fh:seek("set", ((_size - y - 1) * _size + x) * 2)
  local p, altitude = string.unpack(fh:read(2), "H<")
  return altitude
end

function altitude(lat, lon)
  -- bilinear interpolation
  local x, y = (lon - math.floor(lon)) * _size, (lat - math.floor(lat)) * _size
  local ix, iy = math.floor(x), math.floor(y)
  local p11 = _altitude(lat, lon, ix, iy)
  local p21 = _altitude(lat, lon, ix + 1, iy)
  local p12 = _altitude(lat, lon, ix, iy + 1)
  local p22 = _altitude(lat, lon, ix + 1, iy + 1)
  x = x - ix; y = y - iy
  return (p11 * (1 - x) * (1 - y) + p21 * x * (1 - y) + p12 * (1 - x) * y + p22 * x * y)
end
