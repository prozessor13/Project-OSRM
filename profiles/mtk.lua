require("profiles/lib/dem")


-- Begin of globals
barrier_whitelist = { [""] = true, ["bollard"] = true, ["entrance"] = true, ["cattle_grid"] = true, ["border_control"] = true, ["toll_booth"] = true, ["sally_port"] = true, ["gate"] = true}
access_tag_whitelist = { ["yes"] = true, ["permissive"] = true, ["designated"] = true }
access_tag_blacklist = { ["no"] = true, ["private"] = true, ["agricultural"] = true, ["forestery"] = true }
access_tag_restricted = { ["destination"] = true, ["delivery"] = true }
access_tags_hierachy = { "bicycle", "vehicle", "access" }
cycleway_tags = {["track"]=true,["lane"]=true,["opposite"]=true,["opposite_lane"]=true,["opposite_track"]=true,["share_busway"]=true,["sharrow"]=true,["shared"]=true }
service_tag_restricted = { ["parking_aisle"] = true }

default_speed = 16

-- bicycling is allowed
main_speeds = { 
  ["cycleway"] = 20,
  ["primary"] = 15,
  ["primary_link"] = 15,
  ["secondary"] = 18,
  ["secondary_link"] = 18,
  ["tertiary"] = 18,
  ["tertiary_link"] = 18,
  ["residential"] = 18,
  ["unclassified"] = 18,
  ["living_street"] = 18,
  ["road"] = 18,
  ["service"] = 18,
  ["track"] = 16,
  ["path"] = 16,
  ["footway"] = 16,
  ["pedestrian"] = 12,
  ["pier"] = 16,
  ["steps"] = 2
}

-- bicycle must be pushed (bicycling is not allowed)
pushbike_speeds = {
  ["footway"] = 5,
  ["pedestrian"] = 5,
  ["pier"] = 5,
  ["steps"] = 2
}

amenity_speeds = { 
  ["parking"] = 10,
  ["parking_entrance"] = 10
}

route_speeds = { 
  ["ferry"] = 5
}

take_minimum_of_speeds    = true
obey_oneway               = true
obey_bollards             = false
use_restrictions          = true
ignore_areas              = true -- future feature
traffic_signal_penalty    = 2
u_turn_penalty            = 20
turn_penalty              = 15
turn_bias                 = 1.4

-- End of globals

-- read networks
networks = { lcn={}, rcn={}, ncn={}, icn={} }
if (osmFileName) then
  require 'stringy'
  fname = string.gsub(osmFileName, ".osm.pbf", ".osrm.networks");
  f = io.open(fname, "r")
  if (f) then
    for line in f:lines() do
      l = stringy.split(line, ",")
      type = table.remove(l, 1)
      for i, v in ipairs(l) do
        if tonumber(v) then networks[type][tonumber(v)] = 1 end
      end
    end
  end
end

function slower(speed, k)
  k = k / 15 + 1
  if k > 3 then k = 3 end
  return speed / k
end

function faster(speed, k)
  k = k / 30 + 1
  if k > 1.5 then k = 1.5 end
  return speed * k
end

--find first tag in access hierachy which is set
function find_access_tag(source)
  for i,v in ipairs(access_tags_hierachy) do 
    local tag = source.tags:Find(v)
    if tag ~= '' then --and tag ~= "" then
      return tag
    end
  end
  return nil
end

function node_function (node)
  local barrier = node.tags:Find ("barrier")
  local access = find_access_tag(node)
  local traffic_signal = node.tags:Find("highway")
  
  -- flag node if it carries a traffic light  
  if traffic_signal == "traffic_signals" then
    node.traffic_light = true
  end
  
  -- parse access and barrier tags
  if access  and access ~= "" then
    if access_tag_blacklist[access] then
      node.bollard = true
    else
      node.bollard = false
    end
  elseif barrier and barrier ~= "" then
    if barrier_whitelist[barrier] then
      node.bollard = false
    else
      node.bollard = true
    end
  end
  
  node.altitude = altitude(node.lat/100000, node.lon/100000)

  return 1
end

function segment_function (segment, startNode, targetNode, distance)
  -- check if altigude data is available
  if startNode.altitude <= 0 or targetNode.altitude <= 0 then return end

  -- calculate grade and do nothing for relative flat segments
  local k = (targetNode.altitude - startNode.altitude) / distance * 100;
  -- io.stderr:write("alt: " .. startNode.altitude .. "," .. targetNode.altitude .. ": " .. k .. "\n")
  if k >= -1 and k <= 1 then return end

  -- increase / decrease speeds
  -- io.stderr:write("speed1: " .. segment.speed .. " -- " .. segment.speed_backward .. "\n")
  if segment.speed_backward == -1 then segment.speed_backward = segment.speed end
  if (k > 1) then
    segment.speed = slower(segment.speed, k)
    segment.speed_backward = faster(segment.speed_backward, k)
  elseif (k < 1) then
    segment.speed = faster(segment.speed, -k)
    segment.speed_backward = slower(segment.speed_backward, -k)
  end
  -- io.stderr:write("speed2: " .. segment.speed .. " -- " .. segment.speed_backward .. "\n")
end

function way_function (way, numberOfNodesInWay)
  -- A way must have two nodes or more
  if(numberOfNodesInWay < 2) then
    return 0;
  end
  
  -- First, get the properties of each way that we come across
  local highway = way.tags:Find("highway")
  local name = way.tags:Find("name")
  local ref = way.tags:Find("ref")
  local junction = way.tags:Find("junction")
  local route = way.tags:Find("route")
  local railway = way.tags:Find("railway")
  local maxspeed = parseMaxspeed(way.tags:Find ( "maxspeed") )
  local man_made = way.tags:Find("man_made")
  local barrier = way.tags:Find("barrier")
  local oneway = way.tags:Find("oneway")
  local onewayClass = way.tags:Find("oneway:bicycle")
  local cycleway = way.tags:Find("cycleway")
  local cycleway_left = way.tags:Find("cycleway:left")
  local cycleway_right = way.tags:Find("cycleway:right")
  local duration  = way.tags:Find("duration")
  local service = way.tags:Find("service")
  local area = way.tags:Find("area")
  local amenity = way.tags:Find("amenity")
  local lcn = way.tags:Find("lcn")
  local rcn = way.tags:Find("rcn")
  local ncn = way.tags:Find("ncn")
  local icn = way.tags:Find("icn")
  local access = find_access_tag(way)
  
  -- only route on things with highway tag set (not buildings, boundaries, etc)
    if (not highway or highway == '') and 
    (not route or route == '') and 
    (not railway or railway=='') and 
    (not amenity or amenity=='') then
    return 0
    end
    
  -- access
    if access_tag_blacklist[access] then
    return 0
    end

  -- name 
  if "" ~= ref then
    way.name = ref
  elseif "" ~= name then
    way.name = name
  else
    way.name = highway    -- if no name exists, use way type
  end
  
  if route_speeds[route] then
    -- ferries
    way.direction = Way.bidirectional
    way.ignore_in_grid = true
    if durationIsValid(duration) then
      way.speed = math.max( parseDuration(duration) / math.max(1, numberOfNodesInWay-1) )
      way.is_duration_set = true
    else
      way.speed = route_speeds[route]
    end
  elseif pushbike_speeds[highway] and main_speeds[highway] then
    -- pedestrian areas
    if access_tag_whitelist[access] then
      way.speed = main_speeds[highway]    -- biking 
    else
      way.speed = pushbike_speeds[highway]  -- pushing bikes
    end
  elseif amenity and amenity_speeds[amenity] then
    -- parking areas
    way.speed = amenity_speeds[amenity]
  else
    -- regular ways
    if main_speeds[highway] then 
      way.speed = main_speeds[highway]
    elseif main_speeds[man_made] then 
      way.speed = main_speeds[man_made]
    elseif access_tag_whitelist[access] then
      way.speed = default_speed
    end
  end
  
  -- maxspeed
  if take_minimum_of_speeds then
    if maxspeed and maxspeed>0 then
      way.speed = math.min(way.speed, maxspeed)
    end
  end
  
  -- direction
  way.direction = Way.bidirectional
  local impliedOneway = false
  if junction == "roundabout" or highway == "motorway_link" or highway == "motorway" then
    way.direction = Way.oneway
    impliedOneway = true
  end
  
  if onewayClass == "yes" or onewayClass == "1" or onewayClass == "true" then
    way.direction = Way.oneway
  elseif onewayClass == "no" or onewayClass == "0" or onewayClass == "false" then
    way.direction = Way.bidirectional
  elseif onewayClass == "-1" then
    way.direction = Way.opposite
  elseif oneway == "no" or oneway == "0" or oneway == "false" then
    way.direction = Way.bidirectional
  elseif cycleway and string.find(cycleway, "opposite") == 1 then
    if impliedOneway then
      way.direction = Way.opposite
    else
      way.direction = Way.bidirectional
    end
  elseif cycleway_left and cycleway_tags[cycleway_left] and cycleway_right and cycleway_tags[cycleway_right] then
    way.direction = Way.bidirectional
  elseif cycleway_left and cycleway_tags[cycleway_left] then
    if impliedOneway then
      way.direction = Way.opposite
    else
      way.direction = Way.bidirectional
    end
  elseif cycleway_right and cycleway_tags[cycleway_right] then
    if impliedOneway then
      way.direction = Way.oneway
    else
      way.direction = Way.bidirectional
    end
  elseif oneway == "-1" then
    way.direction = Way.opposite
  elseif oneway == "yes" or oneway == "1" or oneway == "true" then
    way.direction = Way.oneway
  end
  
  -- push bikes again oneways
  if way.direction == Way.oneway then
    way.direction = Way.bidirectional
    way.speed_backward = pushbike_speeds["footway"]
  end

  -- cycleways
  if cycleway and cycleway_tags[cycleway] then
    way.speed = main_speeds["cycleway"]
  elseif cycleway_left and cycleway_tags[cycleway_left] then
    way.speed = main_speeds["cycleway"]
  elseif cycleway_right and cycleway_tags[cycleway_right] then
    way.speed = main_speeds["cycleway"]
  end

  -- priorize bike relations
  speed_delta = 0
  if networks.lcn[way.id] or lcn == "yes" then speed_delta = 4 end
  if networks.rcn[way.id] or rcn == "yes" then speed_delta = 6 end
  if networks.ncn[way.id] or ncn == "yes" then speed_delta = 8 end
  if networks.icn[way.id] or icn == "yes" then speed_delta = 8 end
  way.speed = way.speed + speed_delta

  way.type = 1
  return 1
end

function turn_function (angle)
  -- compute turn penalty as angle^2, with a left/right bias
  k = turn_penalty / (90.0 * 90.0)
  if angle >= 0 then
    return angle * angle * k * turn_bias
  else
    return angle * angle * k / turn_bias
  end
end

