class 'PackageEditor' (BaseEditor)

PackageEditorTypes = { Profile=1, Build=2, Cancel=3, Load=4, Save=5,
    Source=6, Catalog=7, Cooked=8, Output=9, Target=10, Roots=11, Mappings=12,
    Search=13, TypeFilter=14, Find=15, Preview=16, RootsPrevious=17, RootsNext=18,
    PreviewPrevious=19, PreviewNext=20, BuildProfile=21, Toolchain=22, BuildSettings=23, RootFirst=100 }

local function trim(value) return (value:gsub("^%s+", ""):gsub("%s+$", "")) end
local function tokens(value)
    local result = {}
    for token in value:gmatch("[^;\r\n]+") do
        local cleaned = trim(token)
        if cleaned ~= "" then table.insert(result, cleaned) end
    end
    return result
end

function PackageEditor:__init(window)
    self.window = window
    self.editorWindow = nil
end

function PackageEditor:__finalize() self.window = nil end

function PackageEditor:resetDraft(root)
    self:clearPreview()
    self.rootRows = {}; self.rootCursors = {}; self.rootPage = 0; self.rootNext = nil
    if self.rootButtons then
        for _, control in ipairs(self.rootButtons) do control:setVisible(false, false) end
    end
    self.projectRoot = root
    self.profileDirty = true
    self.fields.sourceRoot:setText(root)
    self.fields.catalog:setText(root ~= "" and root .. "/asset.db" or "")
    self.fields.cookedRoot:setText(root ~= "" and root .. "/.workphone/cooked-package" or "")
    self.fields.output:setText("")
    self.fields.target:setText("pc")
    self.fields.buildProfile:setText("")
    self.fields.toolchain:setText("")
    self.fields.buildSettings:setText("{}")
    self.fields.roots:setText("")
    self.fields.mappings:setText("script=lua")
end

function PackageEditor:load()
    local app = IApplicationManager.instance()
    local ui = app:getUI()
    local packages = app:getPackageManager()
    self.editorWindow = ui:addElement(IUIWindow.typeInfo())
    self.editorWindow:setLabel("Resource Package")
    self.editorWindow:setSize(Vector2F(800.0, 650.0))
    self.window:getParentWindow():addChild(self.editorWindow)
    local help = ui:addElement(IUIText.typeInfo())
    help:setText("Load or save settings to a new profile filename. Output must be outside project/cooked storage. Build identity includes current cooker modules; identify any external tools in the toolchain label. Format options remain in asset recipes.")
    self.editorWindow:addChild(help)
    local function entry(label, id)
        local control = ui:addElement(IUITextEntry.typeInfo())
        control:setLabel(label)
        control:setElementId(id)
        self.editorWindow:addChild(control)
        self.window:setHandleEvents(control, true)
        return control
    end
    local function button(label, id)
        local control = ui:addElement(IUIButton.typeInfo())
        control:setLabel(label)
        control:setElementId(id)
        self.editorWindow:addChild(control)
        self.window:setHandleEvents(control, true)
        return control
    end
    self.profileEntry = entry("Profile file (.json)", PackageEditorTypes.Profile)
    local profile = packages:getResourcePackageProfile()
    self.profileEntry:setText(profile ~= nil and profile ~= "" and profile or "package.json")
    self.loadButton = button("Load profile", PackageEditorTypes.Load)
    self.fields = {
        sourceRoot=entry("Active project source root", PackageEditorTypes.Source),
        catalog=entry("Catalog database", PackageEditorTypes.Catalog),
        cookedRoot=entry("Cooked storage", PackageEditorTypes.Cooked),
        output=entry("New output directory", PackageEditorTypes.Output),
        target=entry("Target", PackageEditorTypes.Target),
        roots=entry("Root UUIDs (semicolon separated)", PackageEditorTypes.Roots),
        mappings=entry("Type mappings (catalog-type=extension; ...)", PackageEditorTypes.Mappings),
        buildProfile=entry("Build profile (blank for legacy/default)", PackageEditorTypes.BuildProfile),
        toolchain=entry("External toolchain label", PackageEditorTypes.Toolchain),
        buildSettings=entry("Build identity settings (JSON string object)", PackageEditorTypes.BuildSettings)
    }
    self.saveButton = button("Save new profile", PackageEditorTypes.Save)
    self.rootSearch = entry("Find root (literal path or UUID)", PackageEditorTypes.Search)
    self.rootType = entry("Catalog type filter (optional, exact)", PackageEditorTypes.TypeFilter)
    self.findButton = button("Find project catalog roots", PackageEditorTypes.Find)
    self.rootsPrevious = button("Previous root page", PackageEditorTypes.RootsPrevious)
    self.rootsNext = button("Next root page", PackageEditorTypes.RootsNext)
    self.rootButtons = {}
    for i=1,25 do
        self.rootButtons[i] = button("", PackageEditorTypes.RootFirst+i)
        self.rootButtons[i]:setVisible(false, false)
    end
    self.buildButton = button("Build saved profile", PackageEditorTypes.Build)
    self.previewButton = button("Preview saved closure (updates cooking cache)", PackageEditorTypes.Preview)
    self.previewPrevious = button("Previous closure page", PackageEditorTypes.PreviewPrevious)
    self.previewNext = button("Next closure page", PackageEditorTypes.PreviewNext)
    self.previewText = ui:addElement(IUIText.typeInfo())
    self.editorWindow:addChild(self.previewText)
    self.cancelButton = button("Cancel build", PackageEditorTypes.Cancel)
    self.statusText = ui:addElement(IUIText.typeInfo())
    self.editorWindow:addChild(self.statusText)
    self.profileMessage = "Load a profile or save new settings before building."
    self.lastStatus = nil
    self:resetDraft(app:getProjectPath() or "")
    if profile ~= nil and profile ~= "" and not packages:isResourcePackageBusy() then self:loadProfile() end
    self:update()
end

function PackageEditor:clearPreview()
    self.previewPending = false
    self.previewPage = 0; self.previewPath = nil; self.previewHasPrevious = false; self.previewHasMore = false
    if self.previewText then self.previewText:setText("") end
end

function PackageEditor:readPreview(page)
    self.previewPending = false
    page = page or 0
    local result = IApplicationManager.instance():getPackageManager():readResourcePackagePreviewPage(page)
    if not result.ok or (page > 0 and result.path ~= self.previewPath) then
        self:clearPreview()
        self.previewText:setText("Preview unavailable: " .. (result.error or "Preview changed; run preview again."))
        return
    end
    self.previewPath = result.path; self.previewPage = result.page
    self.previewHasPrevious = result.hasPrevious; self.previewHasMore = result.hasMore
    local lines = {"Admitted cooked closure snapshot: " .. tostring(result.total) .. " resources, " ..
        result.totalPayloadBytes .. " payload bytes, target " .. result.target ..
        ". Build revalidates this snapshot. Package output was not touched."}
    for _, row in ipairs(result.entries) do
        table.insert(lines, row.resourceId .. " (" .. row.uuid .. ", " .. row.payloadBytes .. " bytes): " ..
            (row.root and "selected root" or "installed by " .. row.includedBy))
    end
    table.insert(lines, "Closure page " .. tostring(result.page+1) .. ": resources " ..
        tostring(result.page*25+1) .. "-" .. tostring(result.page*25+#result.entries) .. " of " .. tostring(result.total))
    table.insert(lines, "Report: " .. result.path)
    self.previewText:setText(table.concat(lines, "\n"))
end

function PackageEditor:findRoots(cursor, page)
    page = page or 1
    local result = IApplicationManager.instance():getPackageManager():queryResourcePackageRootsPage(
        self.rootSearch:getText() or "", self.rootType:getText() or "", cursor or nil)
    self.rootRows = result.ok and result.entries or {}
    if result.ok then
        self.rootCursors[page] = cursor or false; self.rootPage = page; self.rootNext = result.next
    else self.rootCursors = {}; self.rootPage = 0; self.rootNext = nil end
    for i, control in ipairs(self.rootButtons) do
        local row = self.rootRows[i]
        control:setVisible(row ~= nil, false)
        if row then control:setLabel("Add " .. row.path .. " [" .. row.type .. "] " .. row.uuid) end
    end
    if not result.ok then self.profileMessage = "Search failed: " .. (result.error or "Unknown error")
    else self.profileMessage = "Root page " .. tostring(page) .. ": " .. tostring(#self.rootRows) ..
        " matches. Click an asset to add its UUID." end
end

function PackageEditor:addRoot(index)
    local row = self.rootRows[index]
    if not row then return end
    local result = IApplicationManager.instance():getPackageManager():selectResourcePackageRoot(row)
    if not result.ok then
        self.profileMessage = "Selection failed: " .. (result.error or "Unknown error")
        return
    end
    local mapped = false
    for _, mapping in ipairs(tokens(self.fields.mappings:getText())) do
        local catalogType = mapping:match("^([^=]+)=([^=]+)$")
        if catalogType and trim(catalogType) == row.type then mapped = true end
    end
    if not mapped then self.profileMessage = "Add a type mapping for " .. row.type .. " before selecting this root."; return end
    local roots = tokens(self.fields.roots:getText())
    for _, uuid in ipairs(roots) do
        if uuid:lower() == result.uuid:lower() then self.profileMessage = "Root already selected."; return end
    end
    if #roots >= 512 then self.profileMessage = "A profile supports at most 512 roots."; return end
    table.insert(roots, result.uuid)
    self.fields.roots:setText(table.concat(roots, "; "))
    self.profileDirty = true
    self:clearPreview()
    self.profileMessage = "Added " .. row.path .. ". Save to a new profile filename before building."
end

function PackageEditor:loadProfile()
    self:clearPreview()
    local packages = IApplicationManager.instance():getPackageManager()
    local result = packages:readResourcePackageProfile(self.profileEntry:getText())
    if not result.ok then
        self.profileMessage = "Load failed: " .. (result.error or "Unknown error")
        return false
    end
    local profile = result.profile
    self.fields.buildProfile:setText(profile.buildProfile or "")
    self.fields.toolchain:setText(profile.toolchain or "")
    self.fields.buildSettings:setText(profile.buildSettings or "{}")
    for _, name in ipairs({"sourceRoot", "catalog", "cookedRoot", "output", "target"}) do
        self.fields[name]:setText(profile[name])
    end
    local mappings = {}
    for _, mapping in ipairs(profile.mappings) do
        table.insert(mappings, mapping.catalogType .. "=" .. mapping.extension)
    end
    self.fields.roots:setText(table.concat(profile.roots, "; "))
    self.fields.mappings:setText(table.concat(mappings, "; "))
    self.profileEntry:setText(result.path)
    self.profileDirty = false
    self.profileMessage = "Loaded saved profile."
    return true
end

function PackageEditor:saveProfile()
    self:clearPreview()
    local profile = {roots=tokens(self.fields.roots:getText()), mappings={}}
    for _, name in ipairs({"buildProfile", "toolchain", "buildSettings"}) do
        profile[name] = self.fields[name]:getText()
    end
    for _, name in ipairs({"sourceRoot", "catalog", "cookedRoot", "output", "target"}) do
        profile[name] = self.fields[name]:getText()
    end
    for _, value in ipairs(tokens(self.fields.mappings:getText())) do
        local catalogType, extension = value:match("^([^=]+)=([^=]+)$")
        if catalogType == nil then
            self.profileMessage = "Each type mapping must use catalog-type=extension."
            return false
        end
        table.insert(profile.mappings, {catalogType=trim(catalogType), extension=trim(extension)})
    end
    local result = IApplicationManager.instance():getPackageManager():saveResourcePackageProfile(self.profileEntry:getText(), profile)
    if not result.ok then
        self.profileMessage = "Save failed: " .. (result.error or "Unknown error") .. ". Use a new profile filename."
        return false
    end
    self.profileEntry:setText(result.path)
    self.profileDirty = false
    self.profileMessage = "Saved profile. Build will use these settings."
    return true
end

function PackageEditor:update()
    if self.editorWindow == nil then return end
    local app = IApplicationManager.instance()
    local packages = app:getPackageManager()
    local root = app:getProjectPath() or ""
    if root ~= self.projectRoot then
        self:resetDraft(root)
        self.profileEntry:setText("package.json")
        packages:setResourcePackageProfile("package.json")
        self.profileMessage = "Project changed. Load or save a profile for this project."
    end
    local status = packages:getResourcePackageStatus()
    local busy = packages:isResourcePackageBusy()
    if self.previewPending and not busy then
        if status == "succeeded" then self:readPreview()
        else
            self.previewPending = false
            self.previewText:setText("Preview " .. status .. ": " .. packages:getResourcePackageDiagnostic())
        end
    end
    local message = self.profileMessage .. "\n" .. status .. ": " .. packages:getResourcePackageDiagnostic()
    local log = packages:getResourcePackageLogPath()
    if log ~= nil and log ~= "" then message = message .. "\nLog: " .. log end
    if message ~= self.lastStatus then self.statusText:setText(message); self.lastStatus=message end
    self.profileEntry:setEnabled(not busy, false)
    self.loadButton:setEnabled(not busy, false)
    self.saveButton:setEnabled(not busy, false)
    self.buildButton:setEnabled(not busy and not self.profileDirty, false)
    self.previewButton:setEnabled(not busy and not self.profileDirty, false)
    self.cancelButton:setEnabled(busy, false)
    self.rootSearch:setEnabled(not busy, false)
    self.rootType:setEnabled(not busy, false)
    self.findButton:setEnabled(not busy, false)
    self.rootsPrevious:setEnabled(not busy and self.rootPage > 1, false)
    self.rootsNext:setEnabled(not busy and self.rootNext ~= nil, false)
    self.previewPrevious:setEnabled(not busy and not self.profileDirty and self.previewHasPrevious, false)
    self.previewNext:setEnabled(not busy and not self.profileDirty and self.previewHasMore, false)
    for _, control in ipairs(self.rootButtons) do control:setEnabled(not busy, false) end
    for name, control in pairs(self.fields) do control:setEnabled(not busy and name ~= "sourceRoot", false) end
end

function PackageEditor:handleEvent(parameters, results)
    if parameters == nil then return end
    local event, sender = parameters:at(1), parameters:at(3)
    if sender == nil then return end
    local packages = IApplicationManager.instance():getPackageManager()
    local id = sender:getElementId()
    if packages:isResourcePackageBusy() and id ~= PackageEditorTypes.Cancel then return end
    if event == IEvent.handleValueChanged then
        if id == PackageEditorTypes.Search or id == PackageEditorTypes.TypeFilter then
            self.rootRows = {}; self.rootCursors = {}; self.rootPage = 0; self.rootNext = nil
            for _, control in ipairs(self.rootButtons) do control:setVisible(false, false) end
        else
            self.profileDirty = true
            self:clearPreview()
            self.profileMessage = "Unsaved settings. Load a saved profile or save to a new filename."
        end
    elseif event == IEvent.handleSelection then
        if id == PackageEditorTypes.Load then self:loadProfile()
        elseif id == PackageEditorTypes.Save then self:saveProfile()
        elseif id == PackageEditorTypes.Find then self.rootCursors = {}; self:findRoots()
        elseif id == PackageEditorTypes.RootsNext then
            if self.rootNext then self:findRoots(self.rootNext, self.rootPage+1) end
        elseif id == PackageEditorTypes.RootsPrevious then
            if self.rootPage > 1 then self:findRoots(self.rootCursors[self.rootPage-1], self.rootPage-1) end
        elseif id == PackageEditorTypes.PreviewNext then
            if not self.profileDirty and self.previewHasMore then self:readPreview(self.previewPage+1) end
        elseif id == PackageEditorTypes.PreviewPrevious then
            if not self.profileDirty and self.previewHasPrevious then self:readPreview(self.previewPage-1) end
        elseif id == PackageEditorTypes.Preview then
            if not self.profileDirty then
                self:clearPreview()
                packages:setResourcePackageProfile(self.profileEntry:getText())
                self.previewPending = packages:startResourcePackagePreview()
                if self.previewPending then self.previewText:setText("Preview queued; cooking and validating install dependencies.") end
                if not self.previewPending then self.profileMessage = "Preview could not be queued: " .. packages:getResourcePackageDiagnostic() end
            end
        elseif id > PackageEditorTypes.RootFirst and id <= PackageEditorTypes.RootFirst+25 then
            self:addRoot(id-PackageEditorTypes.RootFirst)
        elseif id == PackageEditorTypes.Build then
            if not self.profileDirty then
                self:clearPreview()
                packages:setResourcePackageProfile(self.profileEntry:getText())
                packages:startResourcePackage()
            end
        elseif id == PackageEditorTypes.Cancel then packages:cancelResourcePackage() end
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
    self.editorWindow=nil; self.profileEntry=nil; self.fields=nil
    self.loadButton=nil; self.saveButton=nil; self.buildButton=nil
    self.cancelButton=nil; self.statusText=nil
    self.rootButtons=nil; self.rootRows=nil; self.rootSearch=nil; self.rootType=nil; self.findButton=nil
    self.previewButton=nil; self.previewText=nil; self.previewPending=false
    self.rootsPrevious=nil; self.rootsNext=nil; self.previewPrevious=nil; self.previewNext=nil
    self.rootCursors=nil; self.rootNext=nil
end
