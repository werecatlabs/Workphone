class 'ResourceSelect' 

function ResourceSelect:__init(editorWindow, window, elementId, label)
	--print("ResourceSelect constructor called");
	
	self.editorWindow = editorWindow;
	self.window = window;
	self.parentWindow = nil;
	self.text = nil;
	self.image = nil;
	self.button = nil;
	self.label = nil;
	self.material = nil;
	self.terrain = nil;
	self.index = elementId;	
	self.deleteHash = StringUtil.getHashS32("Delete"..label);
		
	local applicationManager = IApplicationManager.instance();
	local resourceDatabase = applicationManager:getResourceDatabase();
	local ui = applicationManager:getUI();	
	
	local defaultTexture = resourceDatabase:loadMediaTexture("checker.png");
	
	local windowTypeInfo = IUIWindow.typeInfo();
	local buttonTypeInfo = IUIButton.typeInfo();
	local imageTypeInfo = IUIImage.typeInfo();
	local textTypeInfo = IUIText.typeInfo();

	self.parentWindow = ui:addElement(windowTypeInfo);
	self.parentWindow:setSize(Vector2F(100.0, 100.0));
	self.window:addChild(self.parentWindow);

	self.text = ui:addElement(textTypeInfo); 
	self.text:setText(label);
	self.parentWindow:addChild(self.text);	

	self.image = ui:addElement(imageTypeInfo); 
	self.image:setElementId(elementId);
	
	if defaultTexture then
		self.image:setTexture(defaultTexture);
	end
	self.image:setSize(Vector2F(50.0, 50.0));
	self.parentWindow:addChild(self.image);
	
	self.editorWindow:setDroppable(self.image, true);

	self.button = ui:addElement(buttonTypeInfo);
	self.button:setElementId(self.deleteHash);
	self.button:setLabel("Delete");
	self.parentWindow:addChild(self.button);

	if self.editorWindow then
		self.editorWindow:setHandleEvents(self.button, true);
	end
end

function ResourceSelect:__finalize()
	--print("ResourceSelect __finalize called");

	-- The owning editor may destroy its UI tree before ResourceSelect is
	-- finalized.  In that case parentWindow is still a truthy userdata value,
	-- but looking up a method on it raises "attempt to index a userdata value".
	-- Detach our references first so finalization is idempotent, then make the
	-- best-effort UI cleanup inside pcall for the still-live case.
	local parentWindow = self.parentWindow;
	self.parentWindow = nil;
	self.editorWindow = nil;
	self.window = nil;
	self.text = nil;
	self.image = nil;
	self.button = nil;
	self.label = nil;
	self.material = nil;
	self.terrain = nil;

	if parentWindow then
		pcall(function()
			parentWindow:setVisible(false, false);
			parentWindow:destroyAllChildren();
		end);
	end
end

function ResourceSelect:setLabel(label)
	self.text:setText(label);
end

function ResourceSelect:setSameLine(sameLine)
	self.parentWindow:setSameLine(sameLine);
end

function ResourceSelect:setTerrain(terrain)
    self.terrain = terrain;
end

function ResourceSelect:setTextureIndex(index)
	self.index = index;
end

function ResourceSelect:setMaterial(material)
	local applicationManager = IApplicationManager.instance();
    local resourceDatabase = applicationManager:getResourceDatabase();

    local defaultTextureName = "checker.png";

	self.material = material;
	
	if self.material then
		local index = self.index;
		--print("index "..index);
		
		local texture = self.material:getTexture(index);
		if texture then
			if self.image then
				self.image:setTexture(texture);
			end
		else	
			local defaultTexture = resourceDatabase:loadMediaTexture(defaultTextureName);
		
			if self.image and defaultTexture then
				self.image:setTexture(defaultTexture);
			end
		end
	else
        local defaultTexture = resourceDatabase:loadMediaTexture(defaultTextureName);

        if self.image and defaultTexture then
            self.image:setTexture(defaultTexture);
        end
	end	
end

function ResourceSelect:setTexture(texture)
    if self.material == nil and self.terrain == nil then
        return false;
    end

    -- Clearing a slot keeps the established checker fallback. Never pass nil
    -- through the native SmartPtr<ITexture> bindings when media is unavailable.
    local selected = texture;
    if selected == nil then
        local resources = IApplicationManager.instance():getResourceDatabase();
        selected = resources:loadMediaTexture("checker.png");
    end
    if selected == nil then
        return false;
    end

    if self.material then
        self.material:setTexture(selected, self.index);
        self.material:save();
    end
    if self.terrain then
        self.terrain:setHeightMap(selected);
    end
    if self.image then
        self.image:setTexture(selected);
    end
    return true;
end

function ResourceSelect:setIndex(index)
	self.index = index;
end

function ResourceSelect:handleEvent(parameters, results)
	--print("ResourceSelect handleEvent called");

	if (parameters == nil) then 
		print("ResourceSelect parameters nil");
		return;
	end 
	
	local sender = nil;
	if parameters.at then
		local okSender, valueSender = pcall(function() return parameters:at(3); end);
		if okSender then
			sender = valueSender;
		end
	else
		sender = parameters.sender or parameters.element;
	end

	if sender == nil then
		return;
	end

	local okId, elementId = pcall(function() return sender:getElementId(); end);
	if not okId or elementId == nil then
		return;
	end

	--print("ResourceSelect "..self.deleteHash.." "..elementId);

	if (elementId == self.deleteHash) then
		print("ResourceSelect deleteHash");
		self:setTexture(nil);
	end
end
