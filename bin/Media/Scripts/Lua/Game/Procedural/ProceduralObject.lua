class 'ProceduralObject' (BaseComponent)

function ProceduralObject:__init()
    BaseComponent.__init(self)
    self.m_Id = ""
    self.m_Scene = nil
    self.m_RenderObject = nil
    self.m_Cells = {}
    self.m_ProceduralGroup = nil
    self.m_Properties = nil
    self.m_Polygon = nil
    self.m_Bounds = nil
    self.m_BoundingSphere = nil
    self.m_MinCellIndex = Vector2(0, 0)
    self.m_MaxCellIndex = Vector2(0, 0)
    self.m_ShowBounds = false
    self.m_ShowPolygons = false
    self.m_ShowDetailBounds = false
    self.m_Color = Color.blue
    self.m_IsSelected = false
end

-- Property accessors
function ProceduralObject:get_isSelected() return self.m_IsSelected end
function ProceduralObject:set_isSelected(val) self.m_IsSelected = val end

function ProceduralObject:get_scene() return self.m_Scene end
function ProceduralObject:set_scene(val) self.m_Scene = val end

function ProceduralObject:get_proceduralGroup() return self.m_ProceduralGroup end
function ProceduralObject:set_proceduralGroup(val) self.m_ProceduralGroup = val end

function ProceduralObject:get_renderObject() return self.m_RenderObject end
function ProceduralObject:set_renderObject(val) self.m_RenderObject = val end

function ProceduralObject:get_Cells() return self.m_Cells end
function ProceduralObject:set_Cells(val) self.m_Cells = val end

function ProceduralObject:get_MinCellIndex() return self.m_MinCellIndex end
function ProceduralObject:set_MinCellIndex(val) self.m_MinCellIndex = val end

function ProceduralObject:get_MaxCellIndex() return self.m_MaxCellIndex end
function ProceduralObject:set_MaxCellIndex(val) self.m_MaxCellIndex = val end

function ProceduralObject:get_Properties() return self.m_Properties end
function ProceduralObject:set_Properties(val) self.m_Properties = val end

function ProceduralObject:get_Polygon() return self.m_Polygon end
function ProceduralObject:set_Polygon(val) self.m_Polygon = val end

function ProceduralObject:get_Bounds() return self.m_Bounds end
function ProceduralObject:set_Bounds(val) self.m_Bounds = val end

function ProceduralObject:get_BoundingSphere() return self.m_BoundingSphere end
function ProceduralObject:set_BoundingSphere(val) self.m_BoundingSphere = val end

function ProceduralObject:get_Color() return self.m_Color end
function ProceduralObject:set_Color(val) self.m_Color = val end

function ProceduralObject:get_ShowPolygons() return self.m_ShowPolygons end
function ProceduralObject:set_ShowPolygons(val) self.m_ShowPolygons = val end

-- Methods
function ProceduralObject:Build()
    self:UpdateBounds()
end

function ProceduralObject:GetNodes()
    return {}
end

function ProceduralObject:GetArea()
    if self.m_Polygon and self.m_Polygon.getArea then
        return self.m_Polygon:getArea()
    end
    return 0
end

function ProceduralObject:GetAreaWithNodes(nodes)
    local polygon = Polygon2()
    for _, node in ipairs(nodes) do
        local p = node.transform.position
        polygon:AddPoint(Vector2(p.x, p.z))
    end
    return polygon:getArea()
end

function ProceduralObject:GetLength(nodes)
    local length = 0
    for i = 1, #nodes - 1 do
        local nodeA = nodes[i]
        local index = (i % #nodes) + 1
        local nodeB = nodes[index]
        length = length + (nodeB.transform.position - nodeA.transform.position):magnitude()
    end
    return length
end

function ProceduralObject:GetCenter()
    return self.m_Bounds and self.m_Bounds.center or nil
end

function ProceduralObject:GetMin()
    return self.m_Bounds and self.m_Bounds.min or nil
end

function ProceduralObject:GetMax()
    return self.m_Bounds and self.m_Bounds.max or nil
end

function ProceduralObject:SetId(id)
    self.m_Id = id
end

function ProceduralObject:GetId()
    return self.m_Id
end

function ProceduralObject:Load()
    -- Stub: implement database logic if needed
end

function ProceduralObject:Save()
    -- Stub: implement database logic if needed
end

function ProceduralObject:DrawPolygon()
    local polygon = self:GetPolygon3()
    if not polygon or not polygon.Points then return end
    local points = polygon.Points
    for i = 1, #points do
        local p1 = points[i]
        local index = (i % #points) + 1
        local p2 = points[index]
        if Debug and Debug.DrawLine then
            Debug.DrawLine(p1, p2, Color.red, 1.0)
        end
    end
end

function ProceduralObject:DrawPolygonColor(color)
    local polygon = self:GetPolygon3()
    if not polygon or not polygon.Points then return end
    local points = polygon.Points
    for i = 1, #points do
        local p1 = points[i]
        local index = (i % #points) + 1
        local p2 = points[index]
        if Debug and Debug.DrawLine then
            Debug.DrawLine(p1, p2, color or Color.red, 1.0)
        end
    end
end

function ProceduralObject:CreateVegetationMask()
    -- Stub: implement vegetation mask logic if needed
end

function ProceduralObject:DrawTerrainDetailBounds()
    -- Stub: implement terrain detail bounds logic if needed
end

function ProceduralObject:OnDrawGizmos()
    if self.m_ShowDetailBounds then
        self:DrawTerrainDetailBounds()
    end
    if self.m_ShowBounds and self.m_Bounds then
        -- Stub: Gizmos.DrawWireCube(self.m_Bounds.center, self.m_Bounds.extents)
    end
    if self.m_IsSelected then
        -- Stub: draw polygon in yellow
        self:DrawPolygonColor(Color.yellow)
    end
end

function ProceduralObject:UpdateBounds()
    self.m_Polygon = self:GetPolygon()
    self.m_Bounds = self:GetBounds()
    if self.m_Bounds and self.m_Bounds.center and self.m_Bounds.size then
        self.m_BoundingSphere = BoundingSphere(self.m_Bounds.center, self.m_Bounds.size:magnitude())
    end
end

function ProceduralObject:GetBounds()
    local bounds = Bounds()
    local polygon = self:GetPolygon3()
    if not polygon or not polygon.Points then return bounds end
    bounds.center = polygon:GetCenter()
    bounds.extents = Vector3(0, 0, 0)
    for _, p in ipairs(polygon.Points) do
        bounds:Encapsulate(p)
    end
    local extents = bounds.extents * 2.0
    extents.y = 3.0
    bounds.extents = extents
    return bounds
end

function ProceduralObject:GetPolygon()
    return Polygon2()
end

function ProceduralObject:GetPolygon3()
    local points = {}
    local polygon2 = self:get_Polygon()
    if polygon2 and polygon2.GetPoints then
        local points2d = polygon2:GetPoints()
        for _, point2d in ipairs(points2d) do
            table.insert(points, Vector3(point2d.x, 0.0, point2d.y))
        end
    end
    return Polygon3(points)
end

function ProceduralObject:Intersects(bounds)
    if self:get_Bounds() and self:get_Bounds().Intersects then
        return self:get_Bounds():Intersects(bounds)
    end
    return false
end

function ProceduralObject:IntersectsObject(obj)
    if self:get_Bounds() and obj:get_Bounds() and self:get_Bounds().Intersects then
        if self:get_Bounds():Intersects(obj:get_Bounds()) then
            local polygon = self:get_Polygon()
            local objPolygon = obj:get_Polygon()
            if polygon and polygon.Intersects then
                return polygon:Intersects(objPolygon)
            end
        end
    end
    return false
end

function ProceduralObject:IsPointInside(p)
    local polygon = self:get_Polygon()
    if not polygon or not polygon.IsPointInside then return false end
    if p.x and p.y then
        return polygon:IsPointInside(p)
    elseif p.x and p.z then
        return polygon:IsPointInside(Vector2(p.x, p.z))
    end
    return false
end

function ProceduralObject:CreateRenderObject()
    -- Stub: implement render object creation if needed
end

function ProceduralObject:UpdateRender()
    -- Stub: implement render update logic if needed
end

function ProceduralObject:SetupVertexColors()
    -- Stub: implement vertex color setup if needed
end

function ProceduralObject:OnSelected()
    -- Optional: implement selection logic
end

function ProceduralObject:OnDeselected()
    -- Optional: implement deselection logic
end

