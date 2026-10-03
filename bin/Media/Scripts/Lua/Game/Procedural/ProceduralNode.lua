include("ProceduralObject.lua")

class 'ProceduralNode' (ProceduralObject)

function ProceduralNode:__init()
    ProceduralObject.__init(self)
    self.m_CidyNode = nil
    self.m_RenderNode = nil
    self.m_ParentNode = nil
    self.m_MergedNode = nil
    self.m_NetworkNode = nil
    self.m_ConnectedObjects = {}
    self.m_ConnectedNodes = {}
    self.m_MergedNodes = {}
    self.m_UniqueId = ""
    self.m_GraphId = -1
    self.m_isConnection = false
    -- Static variable handled on class table
    ProceduralNode.m_UniqueIdExt = ProceduralNode.m_UniqueIdExt or 0
end

-- Property accessors
function ProceduralNode:get_CidyNode() return self.m_CidyNode end
function ProceduralNode:set_CidyNode(val) self.m_CidyNode = val end

function ProceduralNode:get_RenderNode() return self.m_RenderNode end
function ProceduralNode:set_RenderNode(val) self.m_RenderNode = val end

function ProceduralNode:get_parentNode() return self.m_ParentNode end
function ProceduralNode:set_parentNode(val) self.m_ParentNode = val end

function ProceduralNode:get_mergedNode() return self.m_MergedNode end
function ProceduralNode:set_mergedNode(val) self.m_MergedNode = val end

function ProceduralNode:get_NetworkNode() return self.m_NetworkNode end
function ProceduralNode:set_NetworkNode(val) self.m_NetworkNode = val end

function ProceduralNode:get_connectedNodes() return self.m_ConnectedNodes end
function ProceduralNode:set_connectedNodes(val) self.m_ConnectedNodes = val end

function ProceduralNode:get_mergedNodes() return self.m_MergedNodes end
function ProceduralNode:set_mergedNodes(val) self.m_MergedNodes = val end

function ProceduralNode.get_UniqueIdExt() return ProceduralNode.m_UniqueIdExt end
function ProceduralNode.set_UniqueIdExt(val) ProceduralNode.m_UniqueIdExt = val end

function ProceduralNode:get_UniqueId() return self.m_UniqueId end
function ProceduralNode:set_UniqueId(val) self.m_UniqueId = val end

function ProceduralNode:get_graphId() return self.m_GraphId end
function ProceduralNode:set_graphId(val) self.m_GraphId = val end

function ProceduralNode:get_IsConnection() return self.m_isConnection end
function ProceduralNode:set_IsConnection(val) self.m_isConnection = val end

-- Methods
function ProceduralNode:Awake()
    self.m_UniqueId = "ProceduralNode_" .. ProceduralNode.m_UniqueIdExt
    ProceduralNode.m_UniqueIdExt = ProceduralNode.m_UniqueIdExt + 1
end

function ProceduralNode:IsConnected(node)
    for _, n in ipairs(self.m_ConnectedNodes) do
        if n == node then return true end
    end
    return false
end

function ProceduralNode:Connect(node)
    if not self:IsConnected(node) then
        table.insert(self.m_ConnectedNodes, node)
    end
end

function ProceduralNode:Disconnect(node)
    for i, n in ipairs(self.m_ConnectedNodes) do
        if n == node then
            table.remove(self.m_ConnectedNodes, i)
            break
        end
    end
end

function ProceduralNode:DisconnectAll()
    if self.m_MergedNode then
        for i, n in ipairs(self.m_MergedNode.m_MergedNodes) do
            if n == self then
                table.remove(self.m_MergedNode.m_MergedNodes, i)
                break
            end
        end
    end
    for _, n in ipairs(self.m_ConnectedNodes) do
        if n then
            n:Disconnect(self)
        end
    end
    self.m_ConnectedNodes = {}
end

function ProceduralNode:AddMergedNode(node)
    table.insert(self.m_MergedNodes, node)
end

function ProceduralNode:RemoveMergedNode(node)
    for i, n in ipairs(self.m_MergedNodes) do
        if n == node then
            table.remove(self.m_MergedNodes, i)
            break
        end
    end
end

function ProceduralNode:AddConnectedObject(obj)
    table.insert(self.m_ConnectedObjects, obj)
end

function ProceduralNode:RemoveConnectedObject(obj)
    for i, o in ipairs(self.m_ConnectedObjects) do
        if o == obj then
            table.remove(self.m_ConnectedObjects, i)
            break
        end
    end
end

