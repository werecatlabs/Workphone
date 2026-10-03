MissionRuntime = MissionRuntime or {}

local runtime = MissionRuntime
runtime._idCounter = runtime._idCounter or 0

function runtime.tryCall(object, names, ...)
	if not object then return false, nil end
	for _, name in ipairs(names or {}) do
		local found, callback = pcall(function() return object[name] end)
		if found and callback then
			local called, result = pcall(callback, object, ...)
			if called then return true, result end
		end
	end
	return false, nil
end

function runtime.read(object, getterNames, fieldNames, defaultValue)
	local called, value = runtime.tryCall(object, getterNames or {})
	if called and value ~= nil then return value end
	if object then
		for _, name in ipairs(fieldNames or {}) do
			local ok, direct = pcall(function() return object[name] end)
			if ok and direct ~= nil then return direct end
		end
	end
	return defaultValue
end

function runtime.write(object, setterNames, fieldNames, value)
	local called = runtime.tryCall(object, setterNames or {}, value)
	if called then return true end
	if object then
		for _, name in ipairs(fieldNames or {}) do
			local ok = pcall(function() object[name] = value end)
			if ok then return true end
		end
	end
	return false
end

function runtime.clamp(value, minimum, maximum, defaultValue)
	value = tonumber(value)
	if not value or value ~= value then value = defaultValue or minimum end
	if value < minimum then return minimum end
	if value > maximum then return maximum end
	return value
end

function runtime.toArray(value)
	local result = {}
	if not value then return result end
	if type(value) == "table" then
		for _, item in ipairs(value) do table.insert(result, item) end
		return result
	end
	local called, count = runtime.tryCall(value, { "size", "Size", "count", "Count", "getSize", "GetSize" })
	if not called then count = runtime.read(value, {}, { "Count", "count", "Length", "length" }, 0) end
	for index = 0, math.max(0, math.floor(tonumber(count) or 0)) - 1 do
		local found, item = runtime.tryCall(value, { "at", "At", "get", "Get", "getItem", "GetItem" }, index)
		if not found then found, item = pcall(function() return value[index] end) end
		if found and item ~= nil then table.insert(result, item) end
	end
	return result
end

function runtime.deepCopy(value, seen)
	if type(value) ~= "table" then return value end
	seen = seen or {}
	if seen[value] then return seen[value] end
	local copy = {}
	seen[value] = copy
	for key, item in pairs(value) do copy[runtime.deepCopy(key, seen)] = runtime.deepCopy(item, seen) end
	return copy
end

function runtime.makeVector3(x, y, z)
	x, y, z = tonumber(x) or 0.0, tonumber(y) or 0.0, tonumber(z) or 0.0
	local ok, value = pcall(function() return Vector3F(x, y, z) end)
	if ok and value then return value end
	ok, value = pcall(function() return Vector3(x, y, z) end)
	if ok and value then return value end
	return { x = x, y = y, z = z }
end

function runtime.vectorComponents(value)
	if not value then return 0.0, 0.0, 0.0 end
	local x = runtime.read(value, { "getX", "GetX" }, { "x", "X", 1 }, 0.0)
	local y = runtime.read(value, { "getY", "GetY" }, { "y", "Y", 2 }, 0.0)
	local z = runtime.read(value, { "getZ", "GetZ" }, { "z", "Z", 3 }, 0.0)
	return tonumber(x) or 0.0, tonumber(y) or 0.0, tonumber(z) or 0.0
end

function runtime.vectorToTable(value)
	local x, y, z = runtime.vectorComponents(value)
	return { x = x, y = y, z = z }
end

function runtime.distance(a, b)
	local ax, ay, az = runtime.vectorComponents(a)
	local bx, by, bz = runtime.vectorComponents(b)
	local dx, dy, dz = ax - bx, ay - by, az - bz
	return math.sqrt(dx * dx + dy * dy + dz * dz)
end

function runtime.normaliseHeading(value)
	value = tonumber(value) or 0.0
	value = value % 360.0
	if value < 0.0 then value = value + 360.0 end
	return value
end

function runtime.headingDelta(a, b)
	local difference = math.abs(runtime.normaliseHeading(a) - runtime.normaliseHeading(b))
	return math.min(difference, 360.0 - difference)
end

function runtime.currentTime(applicationManager)
	if not applicationManager then
		local ok, value = pcall(function() return IApplicationManager.instance() end)
		if ok then applicationManager = value end
	end
	local _, timer = runtime.tryCall(applicationManager, { "getTimer", "GetTimer" })
	local called, value = runtime.tryCall(timer,
		{ "getTimeSinceLevelLoad", "GetTimeSinceLevelLoad", "getTime", "GetTime" })
	if called and tonumber(value) then return tonumber(value) end
	local ok, fallback = pcall(function() return os.clock() end)
	return ok and fallback or 0.0
end

function runtime.generateId(prefix)
	runtime._idCounter = runtime._idCounter + 1
	local stamp = math.floor(runtime.currentTime() * 1000.0)
	return string.format("%s_%d_%d", tostring(prefix or "mission"), stamp, runtime._idCounter)
end

function runtime.asBoolean(value, defaultValue)
	if value == nil then return defaultValue == true end
	if type(value) == "boolean" then return value end
	if type(value) == "number" then return value ~= 0 end
	local text = string.lower(tostring(value))
	if text == "true" or text == "yes" or text == "1" or text == "on" then return true end
	if text == "false" or text == "no" or text == "0" or text == "off" then return false end
	return defaultValue == true
end

return MissionRuntime
