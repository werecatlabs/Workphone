-- Exercise the production Lua window with strict UI and package-manager doubles.
function class(name)
    _G[name] = {}
    return function() end
end
BaseEditor = {}
Vector2F = function(x, y) return {x=x, y=y} end
IEvent = {handleSelection=1, handleValueChanged=2}
for _, name in ipairs({"IUIWindow", "IUIText", "IUITextEntry", "IUIButton"}) do
    _G[name] = {typeInfo=function() return name end}
end
local function element()
    local result = {children={}, enabled=true}
    function result:setLabel(value) self.label=value end
    function result:setText(value) self.text=value end
    function result:getText() return self.text end
    function result:setSize(value) self.size=value end
    function result:setElementId(value) self.id=value end
    function result:getElementId() return self.id end
    function result:setVisible(value) self.visible=value end
    function result:setEnabled(value, cascade)
        assert(type(value)=="boolean" and type(cascade)=="boolean", "Native setEnabled requires two booleans")
        self.enabled=value
    end
    function result:addChild(value) table.insert(self.children, value); value.parent=self end
    function result:getParent() return self.parent end
    function result:removeChild(value)
        for i, child in ipairs(self.children) do
            if child==value then table.remove(self.children, i); return end
        end
    end
    function result:destroyAllChildren() self.children={} end
    return result
end
local parent = element()
local window = {getParentWindow=function() return parent end, setHandleEvents=function() end}
local ui = {addElement=function() return element() end, removeElement=function(_, value) value.removed=true end}
local packages = {profile="", status="idle", busy=false, starts=0, cancels=0, diagnostic="Select a profile", log=""}
function packages:getResourcePackageProfile() return self.profile end
function packages:setResourcePackageProfile(value) self.profile=value end
function packages:getResourcePackageStatus() return self.status end
function packages:isResourcePackageBusy() return self.busy end
function packages:getResourcePackageDiagnostic() return self.diagnostic end
function packages:getResourcePackageLogPath() return self.log end
function packages:startResourcePackage()
    assert(not self.busy)
    self.starts=self.starts+1; self.busy=true; self.status="queued"
    return true
end
function packages:cancelResourcePackage() self.cancels=self.cancels+1; self.status="cancelling" end
function packages:startResourcePackagePreview()
    assert(not self.busy)
    self.previews=(self.previews or 0)+1; self.busy=true; self.status="queued"
    return true
end
function packages:readResourcePackagePreviewPage(page)
    assert(not self.busy and self.status=="succeeded")
    self.previewReads=(self.previewReads or 0)+1
    assert(page==0 or page==1)
    local entries = {}
    for i=page*25+1,math.min(page*25+25,27) do
        table.insert(entries, {resourceId="data://item"..i..".lua", uuid="uuid"..i, root=i==1,
            payloadBytes="100", includedBy="data://main.lua"})
    end
    return {ok=true, total=27, totalPayloadBytes="2700", target="pc",
        path=self.replacedPreview and "other-preview.json" or "preview.json", page=page,
        hasPrevious=page>0, hasMore=page==0, entries=entries}
end
local project = "/project"
local saved = {}
function packages:readResourcePackageProfile(path)
    if self.busy then return {ok=false, error="Build is running"} end
    if not saved[path] then return {ok=false, error="Missing profile"} end
    self.profile=path
    return {ok=true, path=path, profile=saved[path]}
end
function packages:saveResourcePackageProfile(path, profile)
    if self.busy then return {ok=false, error="Build is running"} end
    if saved[path] then return {ok=false, error="Profile already exists"} end
    if #profile.roots==0 then return {ok=false, error="Roots required"} end
    saved[path]=profile; self.profile=path
    return {ok=true, path=path}
end
local pickerRow = {uuid="00000000-0000-4000-8000-000000000003", path="main.lua", type="script"}
local pickerCursor = {page=2}
function packages:queryResourcePackageRootsPage(search, typeFilter, cursor)
    assert(not self.busy)
    self.lastSearch=search; self.lastFilter=typeFilter
    if cursor then
        assert(cursor==pickerCursor)
        if self.stalePage then return {ok=false, error="Catalog changed"} end
        return {ok=true, entries={pickerRow}, hasMore=false}
    end
    return {ok=true, entries={pickerRow}, hasMore=true, next=pickerCursor}
end
function packages:selectResourcePackageRoot(row)
    assert(not self.busy and row==pickerRow)
    if self.stale then return {ok=false, error="Catalog changed"} end
    return {ok=true, uuid=row.uuid}
end
local app = {getUI=function() return ui end, getPackageManager=function() return packages end,
    getProjectPath=function() return project end}
IApplicationManager = {instance=function() return app end}
dofile("bin/Media/Scripts/Lua/Editor/PackageEditor.lua")
local editor = setmetatable({}, {__index=PackageEditor})
editor:__init(window)
editor:load()
assert(editor.profileEntry:getText()=="package.json")
assert(not editor.buildButton.enabled and not editor.cancelButton.enabled)
assert(not editor.fields.sourceRoot.enabled and editor.fields.catalog:getText()=="/project/asset.db")
local function event(kind, sender)
    return {at=function(_, i) if i==1 then return kind elseif i==3 then return sender end end}
end
editor.rootSearch:setText("main")
editor.rootType:setText("script")
editor:handleEvent(event(IEvent.handleSelection, editor.findButton))
assert(packages.lastSearch=="main" and packages.lastFilter=="script")
assert(editor.rootButtons[1].visible and not editor.rootButtons[2].visible)
assert(editor.statusText.text:find("Root page 1", 1, true))
assert(editor.rootsNext.enabled and not editor.rootsPrevious.enabled)
editor:handleEvent(event(IEvent.handleSelection, editor.rootsNext))
assert(editor.rootPage==2 and editor.rootsPrevious.enabled and not editor.rootsNext.enabled)
editor:handleEvent(event(IEvent.handleSelection, editor.rootsPrevious))
assert(editor.rootPage==1 and editor.rootsNext.enabled)
packages.stalePage=true
editor:handleEvent(event(IEvent.handleSelection, editor.rootsNext))
assert(editor.rootPage==0 and #editor.rootRows==0 and not editor.rootsPrevious.enabled and not editor.rootsNext.enabled)
packages.stalePage=false
editor:handleEvent(event(IEvent.handleSelection, editor.findButton))
packages.stale=true
editor:handleEvent(event(IEvent.handleSelection, editor.rootButtons[1]))
assert(editor.fields.roots:getText()=="" and editor.profileMessage:find("Catalog changed", 1, true))
packages.stale=false
editor.fields.mappings:setText("Texture=texres")
editor:handleEvent(event(IEvent.handleSelection, editor.rootButtons[1]))
assert(editor.fields.roots:getText()=="" and editor.profileMessage:find("type mapping", 1, true))
editor.fields.mappings:setText("script=lua")
editor:handleEvent(event(IEvent.handleSelection, editor.rootButtons[1]))
assert(editor.fields.roots:getText()==pickerRow.uuid and editor.profileDirty)
editor:handleEvent(event(IEvent.handleSelection, editor.rootButtons[1]))
assert(editor.fields.roots:getText()==pickerRow.uuid and editor.profileMessage=="Root already selected.")
editor:handleEvent(event(IEvent.handleValueChanged, editor.rootSearch))
assert(#editor.rootRows==0 and not editor.rootButtons[1].visible)
editor.profileEntry:setText("shipping.json")
editor:handleEvent(event(IEvent.handleValueChanged, editor.profileEntry))
editor.fields.target:setText("ui-target")
editor.fields.output:setText("/packages/shipping")
editor.fields.roots:setText("00000000-0000-4000-8000-000000000001; 00000000-0000-4000-8000-000000000002")
editor.fields.mappings:setText("script=lua; Texture=texres")
editor.fields.buildProfile:setText("desktop")
editor.fields.toolchain:setText("tools-sha256-contract")
editor.fields.buildSettings:setText('{"quality":"high","empty":""}')
editor:handleEvent(event(IEvent.handleSelection, editor.saveButton))
assert(packages.profile=="shipping.json" and not editor.profileDirty)
assert(saved["shipping.json"].target=="ui-target" and #saved["shipping.json"].roots==2)
assert(saved["shipping.json"].mappings[2].catalogType=="Texture")
assert(saved["shipping.json"].buildProfile=="desktop" and saved["shipping.json"].toolchain=="tools-sha256-contract")
editor.fields.buildProfile:setText("unsaved-profile")
editor:handleEvent(event(IEvent.handleValueChanged, editor.fields.buildProfile))
assert(editor.profileDirty and not editor.buildButton.enabled)
editor:handleEvent(event(IEvent.handleSelection, editor.loadButton))
assert(not editor.profileDirty and editor.fields.buildProfile:getText()=="desktop")
assert(editor.buildButton.enabled)
assert(editor.previewButton.enabled)
editor:handleEvent(event(IEvent.handleSelection, editor.previewButton))
assert(editor.previewPending and packages.previews==1 and not editor.buildButton.enabled)
assert(not editor.previewButton.enabled and editor.cancelButton.enabled)
assert(not editor.fields.buildProfile.enabled and not editor.fields.toolchain.enabled)
editor:handleEvent(event(IEvent.handleSelection, editor.buildButton))
assert(packages.starts==0)
packages.busy=false; packages.status="succeeded"
editor:update()
assert(not editor.previewPending and packages.previewReads==1)
assert(editor.previewText.text:find("installed by data://main.lua", 1, true))
assert(editor.previewText.text:find("resources 1-25 of 27", 1, true))
editor:update(); assert(packages.previewReads==1) -- parse each completed preview once
assert(editor.previewNext.enabled and not editor.previewPrevious.enabled)
editor:handleEvent(event(IEvent.handleSelection, editor.previewNext))
assert(editor.previewText.text:find("resources 26-27 of 27", 1, true))
assert(editor.previewPrevious.enabled and not editor.previewNext.enabled)
editor:handleEvent(event(IEvent.handleSelection, editor.previewPrevious))
assert(editor.previewPage==0 and editor.previewNext.enabled and packages.previewReads==3)
packages.replacedPreview=true
editor:handleEvent(event(IEvent.handleSelection, editor.previewNext))
assert(not editor.previewPrevious.enabled and not editor.previewNext.enabled and editor.previewPath==nil)
assert(editor.previewText.text:find("Preview unavailable", 1, true))
packages.replacedPreview=false
editor.fields.target:setText("edited-target")
editor:handleEvent(event(IEvent.handleValueChanged, editor.fields.target))
assert(editor.previewText.text=="" and not editor.previewButton.enabled)
editor:handleEvent(event(IEvent.handleSelection, editor.previewButton))
assert(packages.previews==1)
editor:handleEvent(event(IEvent.handleSelection, editor.buildButton))
assert(packages.starts==0 and not editor.buildButton.enabled)
editor:handleEvent(event(IEvent.handleSelection, editor.saveButton))
assert(editor.profileDirty and saved["shipping.json"].target=="ui-target")
editor:handleEvent(event(IEvent.handleSelection, editor.loadButton))
assert(not editor.profileDirty and editor.fields.target:getText()=="ui-target")
editor:handleEvent(event(IEvent.handleSelection, editor.previewButton))
packages.busy=false; packages.status="failed"; packages.diagnostic="Excluded dependency"
editor:update()
assert(editor.previewText.text:find("Excluded dependency", 1, true) and packages.previewReads==4)
editor:handleEvent(event(IEvent.handleSelection, editor.buildButton))
assert(packages.starts==1 and not editor.buildButton.enabled and not editor.profileEntry.enabled)
assert(not editor.findButton.enabled and not editor.rootSearch.enabled)
editor:handleEvent(event(IEvent.handleSelection, editor.findButton)) -- queued UI events cannot search during a job
assert(editor.cancelButton.enabled and editor.statusText.text:find("queued", 1, true))
editor:handleEvent(event(IEvent.handleSelection, editor.cancelButton))
assert(packages.cancels==1)
packages.busy=false; packages.status="failed"; packages.diagnostic="Policy rejected dependency"; packages.log="job.log"
editor:update()
assert(editor.buildButton.enabled and not editor.cancelButton.enabled)
assert(editor.statusText.text:find("Policy rejected", 1, true) and editor.statusText.text:find("job.log", 1, true))
editor:hide(); assert(not editor.editorWindow.visible)
editor:show(); assert(editor.editorWindow.visible)
local removed = editor.editorWindow
editor:unload()
assert(packages.cancels==2 and removed.removed and #parent.children==0)
editor:update() -- no closed UI writes
editor:load(); assert(editor.profileEntry:getText()=="shipping.json")
assert(not editor.profileDirty and editor.fields.target:getText()=="ui-target")
project="/other-project"
editor:update()
assert(editor.profileDirty and editor.fields.sourceRoot:getText()==project and not editor.buildButton.enabled)
assert(editor.fields.buildProfile:getText()=="" and editor.fields.toolchain:getText()=="" and editor.fields.buildSettings:getText()=="{}")
assert(#editor.rootRows==0 and not editor.rootButtons[1].visible)
editor:unload()
print("PASS: PackageEditor profile authoring, save/load, dirty build guard, project switch, async controls and teardown")
