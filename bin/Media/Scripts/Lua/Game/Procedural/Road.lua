class 'Road' (ProceduralObject)

function Road:__init()
	ProceduralObject.__init(self)
end

function Road:Build()
	self:UpdateBounds()
	local sections = self:GetRoadSections()
	for _, section in ipairs(sections) do
		if section.Build then section:Build() end
	end
end

function Road:GetRoadSections()
	local list = {}
	local parent = self.gameObject
	if parent and parent.GetAllChildren then
		local children = parent:GetAllChildren()
		for _, child in ipairs(children) do
			if child.GetComponent then
				local roadSection = child:GetComponent("RoadSection")
				if roadSection then
					table.insert(list, roadSection)
				end
			end
		end
	end
	return list
end

function Road:GetNodes()
	local nodes = {}
	local sections = self:GetRoadSections()
	for _, section in ipairs(sections) do
		if section.GetNodes then
			local sectionNodes = section:GetNodes()
			for _, node in ipairs(sectionNodes) do
				table.insert(nodes, node)
			end
		end
	end
	return nodes
end

function Road:CreateVegetationMask()
	if ProceduralObject.CreateVegetationMask then ProceduralObject.CreateVegetationMask(self) end
	local roadSections = self:GetRoadSections()
	for _, roadSection in ipairs(roadSections) do
		if roadSection.CreateVegetationMask then roadSection:CreateVegetationMask() end
	end
end

function Road:OnSelected()
	local roadSections = self:GetRoadSections()
	for _, roadSection in ipairs(roadSections) do
		roadSection.isSelected = true
		if roadSection.OnSelected then roadSection:OnSelected() end
	end
end

function Road:OnDeselected()
	local roadSections = self:GetRoadSections()
	for _, roadSection in ipairs(roadSections) do
		roadSection.isSelected = false
		if roadSection.OnDeselected then roadSection:OnDeselected() end
	end
end
