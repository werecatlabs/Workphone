include("BaseComponent.lua")

class 'RoadSection' (BaseComponent)

function RoadSection:__init()
    BaseComponent.__init(self)
    self.m_ParentRoad = nil
    self.m_RoadNodes = nil
    self.m_Sidewalks = nil
    self.m_ChildRoadsParent = nil
    self.m_RoadType = Road and Road.RoadType and Road.RoadType.None or nil
    self.m_LaneType = Road and Road.LaneType and Road.LaneType.TwoLane or nil
    self.m_IsLit = true
    self.m_OneWay = false
    self.m_IsBicycle = false
    self.m_IsFootway = false
    self.m_RoadWidth = 10.0
    self.m_SpeedLimit = 30
    self.m_Reference = ""
    self.m_SplitRoads = {}
    self.m_NumConnections = 0
end

-- Property accessors
function RoadSection:get_parentRoad() return self.m_ParentRoad end
function RoadSection:set_parentRoad(val) self.m_ParentRoad = val end
function RoadSection:get_roadType() return self.m_RoadType end
function RoadSection:set_roadType(val) self.m_RoadType = val end
function RoadSection:get_roadWidth() return self.m_RoadWidth end
function RoadSection:set_roadWidth(val) self.m_RoadWidth = val end
function RoadSection:get_laneType() return self.m_LaneType end
function RoadSection:set_laneType(val) self.m_LaneType = val end
function RoadSection:get_oneWay() return self.m_OneWay end
function RoadSection:set_oneWay(val) self.m_OneWay = val end
function RoadSection:get_isBicycle() return self.m_IsBicycle end
function RoadSection:set_isBicycle(val) self.m_IsBicycle = val end
function RoadSection:get_isFootway() return self.m_IsFootway end
function RoadSection:set_isFootway(val) self.m_IsFootway = val end
function RoadSection:get_speedLimit() return self.m_SpeedLimit end
function RoadSection:set_speedLimit(val) self.m_SpeedLimit = val end
function RoadSection:get_RoadNodes() return self.m_RoadNodes end
function RoadSection:set_RoadNodes(val) self.m_RoadNodes = val end
function RoadSection:get_Sidewalks() return self.m_Sidewalks end
function RoadSection:set_Sidewalks(val) self.m_Sidewalks = val end
function RoadSection:get_childRoadsParent() return self.m_ChildRoadsParent end
function RoadSection:set_childRoadsParent(val) self.m_ChildRoadsParent = val end
function RoadSection:get_reference() return self.m_Reference end
function RoadSection:set_reference(val) self.m_Reference = val end
function RoadSection:get_splitRoads() return self.m_SplitRoads end
function RoadSection:set_splitRoads(val) self.m_SplitRoads = val end
function RoadSection:get_numConnections() return self.m_NumConnections end
function RoadSection:set_numConnections(val) self.m_NumConnections = val end

-- Helper functions
function RoadSection:GetRoadTypeFromString(roadType)
    if Road and Road.RoadType then
        if roadType == Road.RoadType.Trunk then
            return "trunk"
        elseif roadType == Road.RoadType.Residential then
            return "residential"
        elseif roadType == Road.RoadType.Footway then
            return "footway"
        end
    end
    return ""
end

function RoadSection:GetRoadTypeFromStringStr(roadType)
    if Road and Road.RoadType then
        if roadType == "residential" then
            return Road.RoadType.Residential
        elseif roadType == "trunk" then
            return Road.RoadType.Trunk
        elseif roadType == "footway" or roadType == "steps" then
            return Road.RoadType.Footway
        end
    end
    return Road and Road.RoadType and Road.RoadType.None or nil
end

function RoadSection:GetNodes()
    local list = {}
    local parent = self.RoadNodes
    if parent and parent.GetAllChildren then
        local children = parent:GetAllChildren()
        for _, child in ipairs(children) do
            if child.GetComponent then
                local roadNode = child:GetComponent("RoadNode")
                if roadNode then
                    table.insert(list, roadNode)
                end
            end
        end
    end
    return list
end

function RoadSection:Load()
    if BaseComponent.Load then BaseComponent.Load(self) end
    local properties = self.Properties
    if properties then
        self.laneType = properties.GetPropertyValueAsInt and properties:GetPropertyValueAsInt("lanes", 2) or 2
        self.oneWay = properties.GetPropertyValueAsBool and properties:GetPropertyValueAsBool("oneway", false) or false
        local highway = properties.GetPropertyValue and properties:GetPropertyValue("highway", "none") or "none"
        self.roadType = self:GetRoadTypeFromStringStr(highway)
        self.reference = properties.GetPropertyValue and properties:GetPropertyValue("ref", "") or ""
        self.roadWidth = properties.GetPropertyValueAsFloat and properties:GetPropertyValueAsFloat("roadwidth", 10.0) or 10.0
        local nodes = self:GetNodes()
        for _, node in ipairs(nodes) do
            node.Width = self.roadWidth
        end
    end
end

function RoadSection:Save()
    local properties = self.Properties
    if properties then
        if properties.SetPropertyValue then
            properties:SetPropertyValue("lanes", self.laneType)
            properties:SetPropertyValue("oneway", self.oneWay and "yes" or "no")
            properties:SetPropertyValue("highway", self:GetRoadTypeFromString(self.roadType))
            properties:SetPropertyValue("ref", self.reference)
            properties:SetPropertyValue("roadwidth", self.roadWidth)
        end
    end
    if BaseComponent.Save then BaseComponent.Save(self) end
end

function RoadSection:GetSidewalks()
    local parent = self.Sidewalks
    if parent and parent.GetComponentsInChildren then
        return parent:GetComponentsInChildren("Sidewalk")
    end
    return {}
end

function RoadSection:GetPolygon()
    local points = {}
    local nodes = self:GetNodes()
    if #nodes == 0 then return nil end
    for i = 1, #nodes do
        local nodeA = nodes[i]
        local index = (i % #nodes) + 1
        local nodeB = nodes[index]
        local reverse = (i == #nodes)
        local right = Vector3.Cross((nodeB.transform.position - nodeA.transform.position):normalized(), Vector3.up)
        local p
        if not reverse then
            p = nodeA.transform.position - (right * nodeA.Width * 0.5)
        else
            p = nodeA.transform.position + (right * nodeA.Width * 0.5)
        end
        table.insert(points, Vector2(p.x, p.z))
    end
    for i = #nodes, 1, -1 do
        local nodeA = nodes[i]
        local index = (i % #nodes) + 1
        local nodeB = nodes[index]
        local reverse = (i == #nodes)
        local right = Vector3.Cross((nodeB.transform.position - nodeA.transform.position):normalized(), Vector3.up)
        local p
        if not reverse then
            p = nodeA.transform.position + (right * nodeA.Width * 0.5)
        else
            p = nodeA.transform.position - (right * nodeA.Width * 0.5)
        end
        table.insert(points, Vector2(p.x, p.z))
    end
    return Polygon2(points)
end

function RoadSection:CreateRenderObject()
    local scene = self.scene
    if scene and scene.proceduralRenderer and scene.proceduralRenderer.CreateRoad then
        scene.proceduralRenderer:CreateRoad(self)
    end
end

function RoadSection:CreateChildRoads()
    if self.childRoadsParent and self.childRoadsParent.DestroyAllChildren then
        self.childRoadsParent:DestroyAllChildren()
    end
    self.numConnections = self:GetNumConnections()
    self.splitRoads = self:GetRoadNodesByConnections()
    local roads = self:SplitRoadByConnection()
    for _, road in ipairs(roads) do
        road.parentRoad = self
        if road.transform and self.childRoadsParent and self.childRoadsParent.transform and road.transform.SetParent then
            road.transform:SetParent(self.childRoadsParent.transform, true)
        end
    end
end

function RoadSection:GetRoadNodesByConnections()
    local nodesByConnections = {}
    local nodes = self:GetNodes()
    local startIndex = 1
    local currentIndex = 1
    local numConnections = self:GetNumConnections() + 1
    for i = 1, numConnections do
        local roadNodes = {}
        for x = startIndex, #nodes do
            local roadNode = nodes[x]
            local nextRoadNode = nodes[(x % #nodes) + 1]
            local networkNode = roadNode.NetworkNode
            if not networkNode then goto continue end
            if networkNode.mergedNode then
                if not networkNode.mergedNode.IsConnection then
                    table.insert(roadNodes, roadNode)
                    currentIndex = currentIndex + 1
                else
                    table.insert(roadNodes, roadNode)
                    currentIndex = currentIndex + 1
                    if x > startIndex then
                        startIndex = x
                        break
                    end
                end
            elseif not networkNode.IsConnection then
                table.insert(roadNodes, roadNode)
                currentIndex = currentIndex + 1
            else
                table.insert(roadNodes, roadNode)
                currentIndex = currentIndex + 1
                if x > startIndex then
                    startIndex = x
                    break
                end
            end
            ::continue::
        end
        -- Remove duplicates
        local unique = {}
        local hash = {}
        for _,v in ipairs(roadNodes) do
            if not hash[v] then
                table.insert(unique, v)
                hash[v] = true
            end
        end
        table.insert(nodesByConnections, unique)
    end
    return nodesByConnections
end

function RoadSection:GetNumConnections()
    local numConnections = 0
    local nodes = self:GetNodes()
    for i = 1, #nodes do
        local node = nodes[i]
        local networkNode = node.NetworkNode
        if networkNode then
            if i == 1 then
                -- skip
            elseif i == #nodes then
                -- skip
            elseif networkNode.IsConnection then
                numConnections = numConnections + 1
            elseif networkNode.mergedNode and networkNode.mergedNode.IsConnection then
                numConnections = numConnections + 1
            end
        end
    end
    return numConnections
end

function RoadSection:SplitRoadByConnection()
    local roads = {}
    local status, err = pcall(function()
        local scene = self.scene
        local nodesByConnections = self:GetRoadNodesByConnections()
        local numConnections = self:GetNumConnections() + 1
        if numConnections > 0 then
            for i = 1, numConnections do
                local road = scene and scene.roadElementPrefab and GameObject.Instantiate and GameObject.Instantiate(scene.roadElementPrefab) or {}
                road.parentRoad = self
                road.scene = self.scene
                local nodes = nodesByConnections[i] or {}
                for _, node in ipairs(nodes) do
                    local clonedNode = GameObject.Instantiate and GameObject.Instantiate(node) or {}
                    if clonedNode.transform and road.RoadNodes and clonedNode.transform.SetParent then
                        clonedNode.transform:SetParent(road.RoadNodes.transform, true)
                    end
                    clonedNode.parentNode = node
                    clonedNode.roadElement = road
                end
                local clonedNodes = road.GetNodes and road:GetNodes() or {}
                for _, clonedNode in ipairs(clonedNodes) do
                    if clonedNode.DisconnectAll then clonedNode:DisconnectAll() end
                end
                for clonedNodeIdx = 1, #clonedNodes - 1 do
                    local nodeA = clonedNodes[clonedNodeIdx]
                    local nodeB = clonedNodes[clonedNodeIdx + 1]
                    if nodeA and nodeB and nodeA.Connect and nodeB.Connect then
                        nodeA:Connect(nodeB)
                        nodeB:Connect(nodeA)
                    end
                end
                if road.Build then road:Build() end
                table.insert(roads, road)
            end
        end
    end)
    if not status then
        print("Error in SplitRoadByConnection: " .. tostring(err))
    end
    return roads
end

function RoadSection:HasChildren()
    local children = self:GetChildRoads()
    return #children > 0
end

function RoadSection:GetNumNodes()
    local nodes = self:GetNodes()
    return #nodes
end

function RoadSection:Build()
    local status, err = pcall(function()
        if self.UpdateBounds then self:UpdateBounds() end
        if not self.parentRoad then
            self:CreateChildRoads()
        end
    end)
    if not status then
        print("Error in Build: " .. tostring(err))
    end
end

function RoadSection:GetChildRoads()
    local list = {}
    local parent = self.childRoadsParent
    if parent and parent.GetAllChildren then
        local children = parent:GetAllChildren()
        for _, child in ipairs(children) do
            if child.GetComponent then
                local road = child:GetComponent("RoadSection")
                if road then
                    table.insert(list, road)
                end
            end
        end
    end
    return list
end

function RoadSection:GetRoadElements()
    local list = {}
    local parent = self.gameObject
    if parent and parent.GetAllChildren then
        local children = parent:GetAllChildren()
        for _, child in ipairs(children) do
            if child.GetComponent then
                local roadElement = child:GetComponent("RoadElement")
                if roadElement then
                    table.insert(list, roadElement)
                end
            end
        end
    end
    return list
end

function RoadSection:CreateVegetationMask()
    if BaseComponent.CreateVegetationMask then BaseComponent.CreateVegetationMask(self) end
    local roadElements = self:GetRoadElements()
    for _, roadElement in ipairs(roadElements) do
        if roadElement.CreateVegetationMask then roadElement:CreateVegetationMask() end
    end
end

function RoadSection:OnSelected()
    local roadElements = self:GetRoadElements()
    for _, roadElement in ipairs(roadElements) do
        roadElement.isSelected = true
    end
end

function RoadSection:OnDeselected()
    local roadElements = self:GetRoadElements()
    for _, roadElement in ipairs(roadElements) do
        roadElement.isSelected = false
    end
end

function RoadSection:AddNode(node)
    -- node.roadElement = self
end

function RoadSection:RemoveNode(node)
    -- node.roadElement = nil
end

function RoadSection:Update()
    if self.m_ShowPolygons and self.DrawPolygon then
        self:DrawPolygon()
        local nodes = self:GetNodes()
        for i = 1, #nodes - 1 do
            local nodeA = nodes[i]
            local index = (i % #nodes) + 1
            local nodeB = nodes[index]
            if Debug and Debug.DrawLine and nodeA.transform and nodeB.transform then
                Debug.DrawLine(nodeA.transform.position, nodeB.transform.position, Color.blue, 1.0)
            end
        end
    end
end

function RoadSection:OnDrawGizmos()
    if BaseComponent.OnDrawGizmos then BaseComponent.OnDrawGizmos(self) end
end

function RoadSection:CreateDecals()
    -- stub
end
