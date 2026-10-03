class 'RoadNetwork' (ProceduralNetwork)

function RoadNetwork:__init()
    ProceduralNetwork.__init(self)
    self.m_Roads = {}
    self.m_Nodes = {}
    self.m_MergedNetwork = {}
    self.mergeTolerance = 0.1 -- Adjust as needed
end

function RoadNetwork:Clear()
    for _, n in ipairs(self.m_Nodes) do
        if n and n.DestroyImmediate then n:DestroyImmediate() end
    end
    for _, n in ipairs(self.m_MergedNetwork) do
        if n and n.DestroyImmediate then n:DestroyImmediate() end
    end
    self.m_Roads = {}
    self.m_Nodes = {}
    self.m_MergedNetwork = {}
end

function RoadNetwork:Generate()
    -- Implement generation logic if needed
end

function RoadNetwork:AddRoad(road)
    table.insert(self.m_Roads, road)
end

function RoadNetwork:GetRoads()
    return self.m_Roads
end

function RoadNetwork:Merge(nodes)
    -- Create a merged node (assume RoadNode.new or similar constructor)
    local mergedNode = RoadNode()
    mergedNode.IsConnection = true
    mergedNode.name = "RoadNode_Merged_"
    local p = {x=0, y=0, z=0}
    for _, n in ipairs(nodes) do
        mergedNode.name = mergedNode.name .. (n.name or "")
        if n.transform and n.transform.position then
            p.x = p.x + (n.transform.position.x or 0)
            p.y = p.y + (n.transform.position.y or 0)
            p.z = p.z + (n.transform.position.z or 0)
        end
        n.mergedNode = mergedNode
    end
    for _, n in ipairs(nodes) do
        if mergedNode.AddMergedNode then mergedNode:AddMergedNode(n) end
    end
    local count = #nodes
    if count > 0 then
        mergedNode.transform = mergedNode.transform or {position={}}
        mergedNode.transform.position = {
            x = p.x / count,
            y = p.y / count,
            z = p.z / count
        }
    end
    for _, n in ipairs(nodes) do
        if n.connectedNodes then
            for _, c in ipairs(n.connectedNodes) do
                if c and mergedNode.Connect then
                    mergedNode:Connect(c)
                    if c.Connect then c:Connect(mergedNode) end
                end
            end
        end
    end
    return mergedNode
end

function RoadNetwork:GetNodes()
    local roadNodes = {}
    for _, r in ipairs(self.m_Roads) do
        if r.GetNodes then
            local nodes = r:GetNodes()
            for _, n in ipairs(nodes) do
                table.insert(roadNodes, n)
            end
        end
    end
    return roadNodes
end

function RoadNetwork:GetRoadNodes()
    local roadNodes = {}
    for _, r in ipairs(self.m_Roads) do
        if r.GetNodes then
            local nodes = r:GetNodes()
            for _, n in ipairs(nodes) do
                table.insert(roadNodes, n)
            end
        end
    end
    return roadNodes
end

function RoadNetwork:GetRoadNodesCloned()
    local roadNodesCloned = {}
    local roadNodes = self:GetRoadNodes()
    for _, n in ipairs(roadNodes) do
        if n.Clone then
            local clonedNode = RoadNode()
            clonedNode:Clone(n)
            clonedNode.transform = clonedNode.transform or {SetParent=function() end}
            if self.transform and clonedNode.transform.SetParent then
                clonedNode.transform:SetParent(self.transform, true)
            end
            n.NetworkNode = clonedNode
            table.insert(roadNodesCloned, clonedNode)
        end
    end
    return roadNodesCloned
end

function RoadNetwork:SetupGraph()
    if self.gameObject and self.gameObject.DestroyAllChildren then
        self.gameObject:DestroyAllChildren()
    end
    self.m_Nodes = {}
    self.m_MergedNetwork = {}
    local origNodes = self:GetRoadNodes()
    local nodes = self:GetRoadNodesCloned()
    for _, n in ipairs(origNodes) do
        if n.connectedNodes then
            for _, connectedNode in ipairs(n.connectedNodes) do
                if connectedNode and n.NetworkNode and connectedNode.NetworkNode and n.NetworkNode.Connect then
                    n.NetworkNode:Connect(connectedNode.NetworkNode)
                end
            end
        end
    end
    local currentMergeNodes = {}
    for _, nodeA in ipairs(nodes) do
        local skip = false
        for _, merged in ipairs(currentMergeNodes) do
            if merged == nodeA then skip = true break end
        end
        if skip then goto continue end
        local mergeNodes = {}
        for _, nodeB in ipairs(nodes) do
            if nodeA ~= nodeB then
                local alreadyMerged = false
                for _, merged in ipairs(currentMergeNodes) do
                    if merged == nodeB then alreadyMerged = true break end
                end
                if not alreadyMerged and nodeA.transform and nodeB.transform and nodeA.transform.position and nodeB.transform.position then
                    local dx = nodeA.transform.position.x - nodeB.transform.position.x
                    local dy = nodeA.transform.position.y - nodeB.transform.position.y
                    local dz = nodeA.transform.position.z - nodeB.transform.position.z
                    local dist = math.sqrt(dx*dx + dy*dy + dz*dz)
                    if dist < (self.mergeTolerance or 0.1) then
                        table.insert(mergeNodes, nodeA)
                        table.insert(mergeNodes, nodeB)
                    end
                end
            end
        end
        if #mergeNodes > 0 then
            for _, n in ipairs(mergeNodes) do table.insert(currentMergeNodes, n) end
            local newNode = self:Merge(mergeNodes)
            table.insert(self.m_Nodes, newNode)
            table.insert(self.m_MergedNetwork, newNode)
        else
            table.insert(self.m_Nodes, nodeA)
            table.insert(self.m_MergedNetwork, nodeA)
        end
        ::continue::
    end
    for _, n in ipairs(self.m_MergedNetwork) do
        if n.mergedNodes then
            for _, mergedNode in ipairs(n.mergedNodes) do
                if mergedNode.connectedNodes then
                    for _, connectedNode in ipairs(mergedNode.connectedNodes) do
                        if connectedNode and connectedNode.mergedNode and n.Connect then
                            n:Connect(connectedNode.mergedNode)
                        end
                    end
                end
            end
        end
    end
end

function RoadNetwork:SetupDisplayNodes()
    local nodes = self:GetRoadNodes()
    for _, n in ipairs(nodes) do
        if n.gameObject and n.gameObject.SetActive then
            n.gameObject:SetActive(false)
        end
    end
    for _, n in ipairs(self.m_MergedNetwork) do
        if n.gameObject and n.gameObject.SetActive then
            n.gameObject:SetActive(true)
        end
    end
end