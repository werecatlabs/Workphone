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
    function result:setEnabled(value) self.enabled=value end
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
local app = {getUI=function() return ui end, getPackageManager=function() return packages end}
IApplicationManager = {instance=function() return app end}
dofile("bin/Media/Scripts/Lua/Editor/PackageEditor.lua")
local editor = setmetatable({}, {__index=PackageEditor})
editor:__init(window)
editor:load()
assert(editor.profileEntry:getText()==".workphone/package.json")
assert(editor.buildButton.enabled and not editor.cancelButton.enabled)
local function event(kind, sender)
    return {at=function(_, i) if i==1 then return kind elseif i==3 then return sender end end}
end
editor.profileEntry:setText("shipping.json")
editor:handleEvent(event(IEvent.handleValueChanged, editor.profileEntry))
assert(packages.profile=="shipping.json")
editor:handleEvent(event(IEvent.handleSelection, editor.buildButton))
assert(packages.starts==1 and not editor.buildButton.enabled and not editor.profileEntry.enabled)
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
editor:unload()
print("PASS: PackageEditor profile selection, async controls, diagnostics, retry and teardown")
