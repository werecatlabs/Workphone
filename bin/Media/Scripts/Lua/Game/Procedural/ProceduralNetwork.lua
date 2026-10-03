include("ProceduralObject.lua")

class 'ProceduralNetwork' (ProceduralObject)

function ProceduralNetwork:__init()
    ProceduralObject.__init(self)
    self.m_City = nil
    self.m_Objects = {}
    self.m_ObjectNodes = {}
    self.m_NetworkNodes = {}
    self.m_PlanarGraph = nil -- PlanarGraph stub
    self.m_Primitives = nil
    self.m_MergeTolerance = 1.0
end

function ProceduralNetwork:get_mergeTolerance() return self.m_MergeTolerance end
function ProceduralNetwork:set_mergeTolerance(val) self.m_MergeTolerance = val end

function ProceduralNetwork:get_city() return self.m_City end
function ProceduralNetwork:set_city(val) self.m_City = val end

function ProceduralNetwork:GetObjectNodes()
    return self.m_ObjectNodes
end

function ProceduralNetwork:GetNetworkNodes()
    return self.m_NetworkNodes
end

function ProceduralNetwork:GetNodes()
    local nodes = {}
    for _, obj in ipairs(self.m_Objects) do
        local objNodes = obj.GetNodes and obj:GetNodes() or {}
        for _, n in ipairs(objNodes) do
            table.insert(nodes, n)
        end
    end
    return nodes
end

function ProceduralNetwork:GetNodesCloned()
    local roadNodesCloned = {}
    local roadNodes = self:GetNodes()
    for _, n in ipairs(roadNodes) do
        local clonedNode = GameObject.Instantiate and GameObject.Instantiate(n) or {}
        if clonedNode.transform and n.transform then
            clonedNode.transform:SetParent(self.transform, true)
            n.NetworkNode = clonedNode
            clonedNode.transform.position = n.transform.position
        end
        table.insert(roadNodesCloned, clonedNode)
    end
    return roadNodesCloned
end

function ProceduralNetwork:AddObject(proceduralObject)
    for _, obj in ipairs(self.m_Objects) do
        if obj == proceduralObject then return end
    end
    table.insert(self.m_Objects, proceduralObject)
end

function ProceduralNetwork:RemoveObject(proceduralObject)
    for i, obj in ipairs(self.m_Objects) do
        if obj == proceduralObject then
            table.remove(self.m_Objects, i)
            break
        end
    end
end

function ProceduralNetwork:ClearObjects()
    self.m_Objects = {}
end

function ProceduralNetwork:MergeNodes(nodes)
    -- Stub: implement merging logic if needed
    return nil
end

function ProceduralNetwork:SetupGraph()
    -- Try/catch not idiomatic in Lua, so just proceed
    if self.gameObject and self.gameObject.DestroyAllChildren then
        self.gameObject:DestroyAllChildren()
    end
    self.m_ObjectNodes = {}
    self.m_NetworkNodes = {}
    local origNodes = self:GetNodes()
    local nodes = self:GetNodesCloned()
    for i, n in ipairs(origNodes) do
        local connectedNodes = n.connectedNodes or (n.get_connectedNodes and n:get_connectedNodes()) or {}
        for _, connectedNode in ipairs(connectedNodes) do
            if connectedNode and n.NetworkNode and connectedNode.NetworkNode and n.NetworkNode.Connect then
                n.NetworkNode:Connect(connectedNode.NetworkNode)
            end
        end
    end
    local currentMergeNodes = {}
    for _, nodeA in ipairs(nodes) do
        local skip = false
        for _, n in ipairs(currentMergeNodes) do if n == nodeA then skip = true break end end
        if skip then goto continue end
        local mergeNodes = {}
        for _, nodeB in ipairs(nodes) do
            if nodeA ~= nodeB then
                local skipB = false
                for _, n in ipairs(currentMergeNodes) do if n == nodeB then skipB = true break end end
                if not skipB and nodeA.transform and nodeB.transform and (nodeA.transform.position - nodeB.transform.position):magnitude() < self.m_MergeTolerance then
                    table.insert(mergeNodes, nodeA)
                    table.insert(mergeNodes, nodeB)
                end
            end
        end
        if #mergeNodes > 0 then
            for _, n in ipairs(mergeNodes) do table.insert(currentMergeNodes, n) end
            local newNode = self:MergeNodes(mergeNodes)
            if newNode then
                table.insert(self.m_ObjectNodes, newNode)
                table.insert(self.m_NetworkNodes, newNode)
            end
        else
            table.insert(self.m_ObjectNodes, nodeA)
            table.insert(self.m_NetworkNodes, nodeA)
        end
        ::continue::
    end
    for _, n in ipairs(self.m_NetworkNodes) do
        local mergedNodes = n.mergedNodes or (n.get_mergedNodes and n:get_mergedNodes()) or {}
        for _, mergedNode in ipairs(mergedNodes) do
            local connections = mergedNode.connectedNodes or (mergedNode.get_connectedNodes and mergedNode:get_connectedNodes()) or {}
            for _, connectedNode in ipairs(connections) do
                if connectedNode and connectedNode.mergedNode and n.Connect then
                    n:Connect(connectedNode.mergedNode)
                end
            end
        end
    end
end

function ProceduralNetwork:CreateGroups()
    -- PlanarGraph logic is not implemented in Lua, so this is a stub
end