class 'WaypointCircuit' (BaseComponent)

function WaypointCircuit:__init()
    self.waypointList = { items = {} }
    self.numPoints = 0
    self.points = {}
    self.distances = {}

    self.pathSmoothness = 100
    self.Length = 0

    -- private members for calculation
    self.p0n = 0
    self.p1n = 0
    self.p2n = 0
    self.p3n = 0

    self.i = 0
    self.P0 = nil -- Vector3
    self.P1 = nil -- Vector3
    self.P2 = nil -- Vector3
    self.P3 = nil -- Vector3
end

function WaypointCircuit:SetWaypoints(waypoints)
    self.waypointList.items = waypoints
    if waypoints and #waypoints > 1 then
        self:CachePositionsAndDistances()
    end
    self.numPoints = #waypoints
end

function WaypointCircuit:Waypoints()
    return self.waypointList.items
end

-- RoutePoint can be a simple table
function WaypointCircuit.CreateRoutePoint(position, direction, index)
    return {
        position = position,
        direction = direction,
        index = index
    }
end

function WaypointCircuit:CachePositionsAndDistances()
    local waypoints = self:Waypoints()
    local numWaypoints = #waypoints

    -- transfer the position of each point and distances between points to arrays for
    -- speed of lookup at runtime
    self.points = {}
    self.distances = {}

    local accumulateDistance = 0
    for i = 1, numWaypoints + 1 do
        -- C# Waypoints.Length is numWaypoints in Lua
        -- C# 'i' (0 to N) is 'i-1' in Lua loop (1 to N+1)
        -- C# (i) % N becomes ((i-1) % N)
        -- C# (i+1) % N becomes (i % N)
        local t1 = waypoints[((i - 1) % numWaypoints) + 1]
        local t2 = waypoints[(i % numWaypoints) + 1]

        if t1 and t2 then
            local p1 = t1.position
            local p2 = t2.position
            self.points[i] = waypoints[((i-1) % numWaypoints) + 1].position
            self.distances[i] = accumulateDistance
            accumulateDistance = accumulateDistance + (p1 - p2):Magnitude()
        end
    end
    
    self.Length = self.distances[#self.distances]
end

function WaypointCircuit:GetRoutePoint(dist)
    -- position and direction
    local p1 = self:GetRoutePosition(dist)
    local p2 = self:GetRoutePosition(dist + 0.1)
    local delta = p2 - p1
    return WaypointCircuit.CreateRoutePoint(p1, delta:Normalized(), -1)
end

local function Repeat(t, length)
    return t - math.floor(t / length) * length
end

local function InverseLerp(a, b, value)
    if a ~= b then
        return (value - a) / (b - a)
    else
        return 0.0
    end
end

function WaypointCircuit:GetRoutePosition(dist)
    local point = 1

    if self.Length == 0 then
        self.Length = self.distances[#self.distances]
    end

    dist = Repeat(dist, self.Length)

    while self.distances[point] < dist do
        point = point + 1
    end

    -- get nearest two points, ensuring points wrap-around start & end of circuit
    -- In C# points are 0-based, in Lua they are 1-based.
    -- The C# code does a lot of % numPoints, which is tricky with 0 vs 1-based and array sizes.
    -- In C#, p1n = ((point - 1) + numPoints) % numPoints; p2n = point;
    -- With Lua 1-based index, point is already what we need.
    self.p1n = point - 1
    self.p2n = point

    -- found point numbers, now find interpolation value between the two middle points
    self.i = InverseLerp(self.distances[self.p1n], self.distances[self.p2n], dist)

    -- get indices for the surrounding 2 points, because
    -- four points are required by the catmull-rom function
    local numPoints = self.numPoints
    self.p0n = self.p1n - 1
    self.p3n = self.p2n + 1
    
    -- The C# code uses % numPoints, which for an array of size N+1
    -- that has a duplicate first point at the end, should wrap around N points.
    self.P0 = self.points[ ( (self.p0n - 1) % numPoints) + 1 ]
    self.P1 = self.points[ ( (self.p1n - 1) % numPoints) + 1 ]
    self.P2 = self.points[ ( (self.p2n - 1) % numPoints) + 1 ]
    self.P3 = self.points[ ( (self.p3n - 1) % numPoints) + 1 ]

    return self:CatmullRom(self.P0, self.P1, self.P2, self.P3, self.i)
end

function WaypointCircuit:CatmullRom(p0, p1, p2, p3, i)
    -- comments are no use here... it's the catmull-rom equation.
    -- Un-magic this, lord vector!
    local i2 = i * i
    local i3 = i * i * i
    return 0.5 * ( (2 * p1) + (-p0 + p2) * i + (2 * p0 - 5 * p1 + 4 * p2 - p3) * i2 + (-p0 + 3 * p1 - 3 * p2 + p3) * i3 )
end

function WaypointCircuit:GetClosestPointIndex(pos)
    local index = -1
    local dist = 1e10
    for i=1, #self.points do
        local curDist = (self.points[i] - pos):Magnitude()
        if curDist < dist then
            dist = curDist
            index = i
        end
    end
    return index
end

function WaypointCircuit:GetRoutePointByIndex(index)
    -- position and direction
    local p1 = self.points[((index-1) % (#self.points - 1)) + 1]
    local p2 = self.points[((index) % (#self.points - 1)) + 1]

    local delta = p2 - p1
    return WaypointCircuit.CreateRoutePoint(p1, delta:Normalized(), index)
end

function WaypointCircuit:GetRoutePointByPosition(pos, index)
    if not index then
        index = self:GetClosestPointIndex(pos)
    end
    
    -- position and direction
    local p1 = self.points[((index) % (#self.points - 1)) + 1]
    local p2 = self.points[((index + 1) % (#self.points - 1)) + 1]

    local delta = p2 - p1
    return WaypointCircuit.CreateRoutePoint(p1, delta:Normalized(), index)
end