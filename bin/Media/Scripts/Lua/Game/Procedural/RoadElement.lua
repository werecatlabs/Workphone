class 'RoadElement' (ProceduralObject)

function RoadElement:__init()
    ProceduralObject.__init(self)
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
function RoadElement:get_parentRoad() return self.m_ParentRoad end
function RoadElement:set_parentRoad(val) self.m_ParentRoad = val end
function RoadElement:get_roadType() return self.m_RoadType end
function RoadElement:set_roadType(val) self.m_RoadType = val end
function RoadElement:get_roadWidth() return self.m_RoadWidth end
function RoadElement:set_roadWidth(val) self.m_RoadWidth = val end
function RoadElement:get_laneType() return self.m_LaneType end
function RoadElement:set_laneType(val) self.m_LaneType = val end
function RoadElement:get_oneWay() return self.m_OneWay end
function RoadElement:set_oneWay(val) self.m_OneWay = val end
function RoadElement:get_isBicycle() return self.m_IsBicycle end
function RoadElement:set_isBicycle(val) self.m_IsBicycle = val end
function RoadElement:get_isFootway() return self.m_IsFootway end
function RoadElement:set_isFootway(val) self.m_IsFootway = val end
function RoadElement:get_speedLimit() return self.m_SpeedLimit end
function RoadElement:set_speedLimit(val) self.m_SpeedLimit = val end
function RoadElement:get_RoadNodes() return self.m_RoadNodes end
function RoadElement:set_RoadNodes(val) self.m_RoadNodes = val end
function RoadElement:get_Sidewalks() return self.m_Sidewalks end
function RoadElement:set_Sidewalks(val) self.m_Sidewalks = val end
function RoadElement:get_childRoadsParent() return self.m_ChildRoadsParent end
function RoadElement:set_childRoadsParent(val) self.m_ChildRoadsParent = val end
function RoadElement:get_reference() return self.m_Reference end
function RoadElement:set_reference(val) self.m_Reference = val end
function RoadElement:get_splitRoads() return self.m_SplitRoads end
function RoadElement:set_splitRoads(val) self.m_SplitRoads = val end
function RoadElement:get_numConnections() return self.m_NumConnections end
function RoadElement:set_numConnections(val) self.m_NumConnections = val end

-- Helper functions
function RoadElement:GetRoadTypeFromString(roadType)
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

function RoadElement:GetRoadTypeFromStringStr(roadType)
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

function RoadElement:GetNodes()
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

function RoadElement:Load()
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

function RoadElement:Save()
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

function RoadElement:GetSidewalks()
    local parent = self.Sidewalks
    if parent and parent.GetComponentsInChildren then
        return parent:GetComponentsInChildren("Sidewalk")
    end
    return {}
end