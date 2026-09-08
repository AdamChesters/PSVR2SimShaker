-- PSVR2SimShaker independent telemetry exporter. GPL-3.0.
-- DCS signals/Hornet draw arguments reference TelemFFB (Valmantas Paliksa,
-- Micah Frisby and contributors); see THIRD_PARTY_NOTICES.md.
-- Install through the application. Existing exporters are chained, never replaced.
if _G.PSVR2SimShakerExport then return end
local state = { last = -1, failed = false }
-- Hornet damage draw arguments adapted from TelemFFB's FA-18 mapping (GPL-3.0).
-- Duplicates in the upstream sum are omitted. This is an indicator, not a hit event.
local damageArgs = {65,135,136,137,146,148,149,150,152,153,154,156,157,158,160,166,183,213,214,215,216,217,220,222,223,224,225,226,227,230,232,233,235,241,242,244,245,247,248,265,266,267,298,299}
_G.PSVR2SimShakerExport = state
local lfs = require('lfs')
local root = lfs.writedir() .. 'Scripts/PSVR2SimShaker/'
local loadBridge, bridgeError = package.loadlib(root .. 'PSVR2SimShakerDcsBridge.dll', 'luaopen_PSVR2SimShakerDcsBridge')
local bridge
if loadBridge then local ok, value = pcall(loadBridge); if ok then bridge = value end end
local function report(message)
    if log and log.write then log.write('PSVR2SimShaker', log.INFO, tostring(message)) end
end
if type(bridge) ~= 'table' or type(bridge.publish) ~= 'function' then
    report('Bridge unavailable: ' .. tostring(bridgeError)); return
end
local function read(fn, ...)
    if type(fn) ~= 'function' then return nil end
    local ok, value = pcall(fn, ...)
    if ok then return value end
end
local function finite(n) return type(n) == 'number' and n == n and n ~= math.huge and n ~= -math.huge end
local function quote(s)
    return '"' .. tostring(s):gsub('[%z\1-\31\\"]', function(c)
        if c == '"' then return '\\"' elseif c == '\\' then return '\\\\' end
        return string.format('\\u%04x', string.byte(c))
    end) .. '"'
end
local function encode(values)
    local keys = {}; for key in pairs(values) do keys[#keys+1] = key end; table.sort(keys)
    local items = {}
    for _,key in ipairs(keys) do
        local value = values[key]
        if finite(value) then items[#items+1] = quote(key) .. ':' .. string.format('%.8g', value):gsub(',', '.') end
    end
    return '{' .. table.concat(items, ',') .. '}'
end
local function publish()
    local t = read(LoGetModelTime)
    if not finite(t) then return end
    if t >= state.last and t - state.last < 1/50 then return end
    if t < state.last then bridge.reset() end
    state.last = t
    local aircraft = read(LoGetSelfData)
    if type(aircraft) ~= 'table' or type(aircraft.Name) ~= 'string' then
        bridge.publish('{"version":1,"state":"no_aircraft","aircraft":"","values":{}}', t); return
    end
    local v = {}
    local function put(k,n) if finite(n) then v[k] = n end end
    put('shake', read(LoGetShakeAmplitude))
    put('aoa_deg', read(LoGetAngleOfAttack))
    put('ias_mps', read(LoGetIndicatedAirSpeed))
    put('vertical_mps', read(LoGetVerticalVelocity))
    local acc = read(LoGetAccelerationUnits)
    if type(acc) == 'table' then put('accel_x_g',acc.x); put('accel_y_g',acc.y); put('accel_z_g',acc.z) end
    local vel = read(LoGetVectorVelocity)
    if type(vel) == 'table' and finite(vel.x) and finite(vel.z) then put('ground_mps',math.sqrt(vel.x*vel.x + vel.z*vel.z)) end
    local engine = read(LoGetEngineInfo)
    if type(engine) == 'table' and type(engine.RPM) == 'table' then put('rpm_left_pct',engine.RPM.left); put('rpm_right_pct',engine.RPM.right) end
    local payload = read(LoGetPayloadInfo)
    if type(payload) == 'table' then
        if type(payload.Cannon) == 'table' then put('cannon_rounds',payload.Cannon.shells) end
        if type(payload.Stations) == 'table' then
            local count, complete = 0, true
            for _, station in pairs(payload.Stations) do
                if type(station) ~= 'table' or not finite(station.count) or station.count < 0 then complete = false; break end
                count = count + station.count
            end
            if complete then put('stores_count',count) end
        end
    end
    local cm = read(LoGetSnares)
    if type(cm) == 'table' then put('flares',cm.flare); put('chaff',cm.chaff) end
    if aircraft.Name:find('FA%-18') then
        local function arg(i) return read(LoGetAircraftDrawArgumentValue,i) end
        local l,n,r = arg(6),arg(1),arg(4)
        if finite(l) and finite(n) and finite(r) then put('on_ground', (l+n+r > 0.001) and 1 or 0) end
        put('gear',arg(3)); put('flaps',arg(9)); put('airbrake',arg(21))
        put('ab_left',arg(28)); put('ab_right',arg(29))
        put('launch_bar',arg(85)); put('tail_hook',arg(25))
        local damage, complete = 0, true
        for _,i in ipairs(damageArgs) do
            local value = arg(i)
            if not finite(value) then complete = false; break end
            damage = damage + value
        end
        if complete then put('damage_total',damage) end
    end
    local packet = '{"version":1,"state":"flying","aircraft":' .. quote(aircraft.Name) .. ',"values":' .. encode(v) .. '}'
    if #packet <= 32768 then bridge.publish(packet,t) end
end
local function protect(fn)
    if state.failed then return end
    local ok, err = pcall(fn)
    if not ok then state.failed = true; pcall(bridge.close); report('Export disabled after error: ' .. tostring(err)) end
end
local previousStart, previousFrame, previousStop = LuaExportStart, LuaExportAfterNextFrame, LuaExportStop
LuaExportStart = function()
    if previousStart then local ok,err = pcall(previousStart); if not ok then report('Earlier start hook: ' .. tostring(err)) end end
    state.last = -1; state.failed = false; protect(function() bridge.reset() end)
end
LuaExportAfterNextFrame = function()
    if previousFrame then local ok,err = pcall(previousFrame); if not ok and not state.previousFailed then report('Earlier frame hook: ' .. tostring(err)); state.previousFailed = true end end
    protect(publish)
end
LuaExportStop = function()
    protect(function() bridge.publish('{"version":1,"state":"stopped","aircraft":"","values":{}}',state.last); bridge.close() end)
    if previousStop then local ok,err = pcall(previousStop); if not ok then report('Earlier stop hook: ' .. tostring(err)) end end
end
report('Independent telemetry exporter loaded')
