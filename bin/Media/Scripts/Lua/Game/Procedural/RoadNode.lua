include("ProceduralObject.lua")

class 'RoadNode' (ProceduralNode)

function RoadNode:__init()
    ProceduralNode.__init(self)
    self.m_RoadConnection = nil
    self.m_Road = nil
    self.m_roadNodeId = -1
    self.m_Width = 10.0
    self.m_Offset = 5.0
    self.m_SidewalkNodes = {}
end

-- Property accessors
function RoadNode:get_roadElement() return self.m_Road end
function RoadNode:set_roadElement(val) self.m_Road = val end

function RoadNode:get_roadConnection() return self.m_RoadConnection end
function RoadNode:set_roadConnection(val) self.m_RoadConnection = val end

function RoadNode:get_Width() return self.m_Width end
function RoadNode:set_Width(val) self.m_Width = val end

function RoadNode:get_Offset() return self.m_Offset end
function RoadNode:set_Offset(val) self.m_Offset = val end

function RoadNode:Awake()
    self.m_UniqueId = "RoadNode_" .. ProceduralNode.m_UniqueIdExt
    ProceduralNode.m_UniqueIdExt = ProceduralNode.m_UniqueIdExt + 1
end

function RoadNode:Update()
    if self.transform and self.transform.hasChanged then
        if self.m_MergedNodes then
            for _, n in ipairs(self.m_MergedNodes) do
                if n and n.transform then
                    n.transform.position = self.transform.position
                end
            end
        end
        self.transform.hasChanged = false
    end
end

function RoadNode:GetConnectedNodeByRoad(road)
    if not self.m_ConnectedNodes then return nil end
    for _, node in ipairs(self.m_ConnectedNodes) do
        if node and node.get_roadElement and node:get_roadElement() == road then
            return node
        end
    end
    return nil
end

function RoadNode:Clone(node)
    self.m_ConnectedNodes = node.m_ConnectedNodes
end

function RoadNode:AddSidewalkNode(node)
    table.insert(self.m_SidewalkNodes, node)
end

function RoadNode:RemoveSidewalkNode(node)
    for i, n in ipairs(self.m_SidewalkNodes) do
        if n == node then
            table.remove(self.m_SidewalkNodes, i)
            break
        end
    end
end

function RoadNode:GetSidewalkNodes()
    return self.m_SidewalkNodes
end