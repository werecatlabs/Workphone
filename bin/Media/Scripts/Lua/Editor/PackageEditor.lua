class 'PackageEditor' (BaseEditor)

PackageEditorTypes = { Profile = 1, Build = 2, Cancel = 3 }

function PackageEditor:__init(window)
    self.window = window
    self.editorWindow = nil
end

function PackageEditor:__finalize()
    self.window = nil
end

function PackageEditor:load()
    local app = IApplicationManager.instance()
    local ui = app:getUI()
    local packages = app:getPackageManager()
    self.editorWindow = ui:addElement(IUIWindow.typeInfo())
    self.editorWindow:setLabel("Resource Package")
    self.editorWindow:setSize(Vector2F(650.0, 350.0))
    self.window:getParentWindow():addChild(self.editorWindow)
    local help = ui:addElement(IUIText.typeInfo())
    help:setText("Build a saved package profile. Output must be a new directory. Profiles select target, mappings and root UUIDs.")
    self.editorWindow:addChild(help)
    self.profileEntry = ui:addElement(IUITextEntry.typeInfo())
    self.profileEntry:setLabel("Package profile (.json)")
    self.profileEntry:setElementId(PackageEditorTypes.Profile)
    local profile = packages:getResourcePackageProfile()
    if profile == nil or profile == "" then profile = ".workphone/package.json" end
    self.profileEntry:setText(profile)
    self.editorWindow:addChild(self.profileEntry)
    self.window:setHandleEvents(self.profileEntry, true)
    self.buildButton = ui:addElement(IUIButton.typeInfo())
    self.buildButton:setLabel("Build saved profile")
    self.buildButton:setElementId(PackageEditorTypes.Build)
    self.editorWindow:addChild(self.buildButton)
    self.window:setHandleEvents(self.buildButton, true)
    self.cancelButton = ui:addElement(IUIButton.typeInfo())
    self.cancelButton:setLabel("Cancel build")
    self.cancelButton:setElementId(PackageEditorTypes.Cancel)
    self.editorWindow:addChild(self.cancelButton)
    self.window:setHandleEvents(self.cancelButton, true)
    self.statusText = ui:addElement(IUIText.typeInfo())
    self.editorWindow:addChild(self.statusText)
    self.lastStatus = nil
    self:update()
end

function PackageEditor:update()
    if self.editorWindow == nil then return end
    local packages = IApplicationManager.instance():getPackageManager()
    local status = packages:getResourcePackageStatus()
    local busy = packages:isResourcePackageBusy()
    local message = status .. ": " .. packages:getResourcePackageDiagnostic()
    local log = packages:getResourcePackageLogPath()
    if log ~= nil and log ~= "" then message = message .. "\nLog: " .. log end
    if message ~= self.lastStatus then
        self.statusText:setText(message)
        self.lastStatus = message
    end
    self.profileEntry:setEnabled(not busy, false)
    self.buildButton:setEnabled(not busy, false)
    self.cancelButton:setEnabled(busy, false)
end

function PackageEditor:handleEvent(parameters, results)
    if parameters == nil then return end
    local event = parameters:at(1)
    local sender = parameters:at(3)
    if sender == nil then return end
    local packages = IApplicationManager.instance():getPackageManager()
    local id = sender:getElementId()
    if event == IEvent.handleValueChanged and id == PackageEditorTypes.Profile then
        packages:setResourcePackageProfile(self.profileEntry:getText())
    elseif event == IEvent.handleSelection then
        if id == PackageEditorTypes.Build then
            packages:setResourcePackageProfile(self.profileEntry:getText())
            packages:startResourcePackage()
        elseif id == PackageEditorTypes.Cancel then
            packages:cancelResourcePackage()
        end
    end
    self:update()
end

function PackageEditor:show()
    if self.editorWindow then self.editorWindow:setVisible(true, false) end
    self:update()
end

function PackageEditor:hide()
    if self.editorWindow then self.editorWindow:setVisible(false, false) end
end

function PackageEditor:unload()
    local app = IApplicationManager.instance()
    app:getPackageManager():cancelResourcePackage()
    if self.editorWindow then
        self.editorWindow:setVisible(false, false)
        self.editorWindow:destroyAllChildren()
        local parent = self.editorWindow:getParent()
        if parent then parent:removeChild(self.editorWindow) end
        app:getUI():removeElement(self.editorWindow)
    end
    self.editorWindow = nil
    self.profileEntry = nil
    self.buildButton = nil
    self.cancelButton = nil
    self.statusText = nil
end
