-- Run from the repository root with luavm Tests/lua/ProjectSettingsTests.lua.
-- The production window is exercised against strict UI and native-service doubles.
function class(name)
    _G[name] = {}
    return function() end
end
BaseEditor = {}
Vector2F = function(x, y) return {x=x, y=y} end
IEvent = {UI=1, handleSelection=11, handleValueChanged=12, handlePropertyChanged=13}
StringUtil = {getHash32=function(value) assert(value=="projectSettingsAction"); return 12345 end}
Parameters = function() return {} end
for _, name in ipairs({"IUIWindow", "IUITabBar", "IUICollapsingHeader", "IUIText",
    "IUITextEntry", "IUIButton", "IUILabelTogglePair", "IUILabelSliderPair", "IUIDropdown"}) do
    _G[name] = {typeInfo=function() return name end}
end

local function element()
    local result = {children={}, options={}, enabled=true}
    function result:setLabel(value) self.label=value end
    function result:getLabel() return self.label end
    function result:setText(value) self.text=value end
    function result:getText() return self.text end
    function result:setSize(value) self.size=value end
    function result:setElementId(value) self.id=value end
    function result:getElementId() return self.id end
    function result:setVisible(value) self.visible=value end
    function result:setSameLine(value) self.sameLine=value end
    function result:setValue(value) self.value=value end
    function result:getValue() return self.value end
    function result:setMinValue(value) self.min=value end
    function result:setMaxValue(value) self.max=value end
    function result:setSelectedOption(value) self.selected=value end
    function result:getSelectedOption() return self.selected end
    function result:addOption(value) table.insert(self.options, value) end
    function result:setEnabled(value, cascade)
        assert(type(value)=="boolean" and type(cascade)=="boolean")
        self.enabled=value
    end
    function result:addChild(value) table.insert(self.children, value); value.parent=self end
    function result:addTabItem() local value=element(); self:addChild(value); return value end
    function result:getParent() return self.parent end
    function result:removeChild(value)
        for i, child in ipairs(self.children) do
            if child==value then table.remove(self.children, i); return end
        end
    end
    function result:destroyAllChildren() self.children={} end
    return result
end

local function properties()
    local result={values={}, kinds={}}
    function result:hasProperty(name) return self.values[name]~=nil end
    for _, kind in ipairs({"Bool", "Int", "Float", "String"}) do
        result["setPropertyAs"..kind]=function(self, name, value)
            assert(type(value)==(kind=="String" and "string" or kind=="Bool" and "boolean" or "number"))
            self.values[name]=value; self.kinds[name]=kind
        end
        result["getPropertyAs"..kind]=function(self, name) return self.values[name] end
    end
    return result
end
local parent=element()
local ui={addElement=function() return element() end, removeElement=function(_, value) value.removed=true end}
local native={calls={}, fail=false, available=true, expectedUnavailable=true}
local window={getParentWindow=function() return parent end, setHandleEvents=function() end}
function window:handleEvent(eventType, hash, arguments, sender, request, event)
    assert(eventType==IEvent.UI and hash==12345 and sender==nil and event==nil)
    table.insert(native.calls, request)
    if not native.available then return end
    if native.fail then
        request:setPropertyAsBool("success", false)
        request:setPropertyAsString("status", "Native validation rejected the selected backend.")
        return
    end
    local action=request.values.action
    if action=="readProjectSettings" then
        assert(next(request.values, "action")==nil, "Read must not send defaults as project edits")
        request:setPropertyAsString("project.name", "Stored project")
        request:setPropertyAsString("project.company", "")
        request:setPropertyAsString("project.startupScene", "")
        request:setPropertyAsFloat("physics.gravity", -12.25)
        request:setPropertyAsBool("capability.services.cloudSave", false)
        request:setPropertyAsString("unavailable.services.cloudSave", "No cloud provider configured")
        request:setPropertyAsBool("services.cloudSave", true)
    elseif action=="importProjectSettingsJson" then
        assert(not request:hasProperty("project.name"), "Import must not overwrite file values with current controls")
        request:setPropertyAsString("project.name", "Imported project")
        request:setPropertyAsString("project.company", "")
        request:setPropertyAsBool("graphics.vsync", false)
        request:setPropertyAsBool("services.cloudSave", false)
        native.expectedUnavailable=false
    else
        assert(request.values["services.cloudSave"]==native.expectedUnavailable,
            "Saved and imported unavailable settings must round trip through the native store")
    end
    request:setPropertyAsBool("success", true)
    request:setPropertyAsString("status", action.." completed")
end
local app={getUI=function() return ui end,
    getFactoryManager=function() return {make_object=function(_, name) assert(name=="Properties"); return properties() end} end}
IApplicationManager={instance=function() return app end}
dofile("bin/Media/Scripts/Lua/Editor/ProjectSettings.lua")
local editor=setmetatable({}, {__index=ProjectSettings})
editor:__init(window)
editor:load()
assert(#native.calls==1 and native.calls[1].values.action=="readProjectSettings")
assert(editor.projectNameControl:getText()=="Stored project")
assert(editor.companyControl:getText()=="" and editor.startupSceneControl:getText()=="")
assert(editor.gravityControl:getValue()==-12.25)
assert(not editor.cloudSaveControl.enabled and editor.cloudSaveControl.label:find("(Unavailable)", 1, true))
assert(editor.settings.cloudSave==true)
assert(#editor.controlBindings==132, "All project settings must be bound")

local function event(kind, sender)
    return {at=function(_, i) if i==1 then return kind elseif i==3 then return sender end end}
end
editor.projectNameControl:setText("Edited project")
local senderProxy=setmetatable({}, {__index=editor.projectNameControl,
    __eq=function() error("UI userdata equality is unsupported; compare element IDs") end})
editor:handleEvent(event(IEvent.handlePropertyChanged, senderProxy))
assert(editor.settings.projectName=="Edited project" and #native.calls==1)
editor.vsyncControl:setValue(false)
editor:handleEvent(event(IEvent.handleSelection, editor.vsyncControl))
assert(editor.settings.vsync==false and #native.calls==1)
editor.activePlatformControl:setSelectedOption(3)
editor:handleEvent(event(IEvent.handleSelection, editor.activePlatformControl))
assert(editor.settings.activePlatform==3 and editor.rendererSummaryText.text:find("Active: Android", 1, true))
editor.gravityControl:setValue(-4.5)
editor:handleEvent(event(IEvent.handleValueChanged, editor.gravityControl))
assert(editor.settings.gravity==-4.5)
local before=#native.calls
editor:handleEvent(event(IEvent.handleValueChanged, element()))
assert(#native.calls==before)

editor:handleEvent(event(IEvent.handleSelection, editor.saveButton))
local saved=native.calls[#native.calls]
assert(saved.values.action=="saveProjectSettings" and saved.values["project.name"]=="Edited project")
assert(saved.kinds["graphics.vsync"]=="Bool" and saved.kinds["physics.gravity"]=="Float")
assert(saved.kinds["graphics.frameRateLimit"]=="Int" and saved.kinds["project.company"]=="String")
assert(editor.statusText.text=="saveProjectSettings completed")
assert(editor.settings.cloudSave==true)

for _, entry in ipairs({{editor.validateButton, "validateProjectSettings"}, {editor.reloadRendererButton, "reloadRenderer"},
    {editor.buildSelectedButton, "buildSelectedPlatform"}, {editor.exportJsonButton, "exportProjectSettingsJson"}}) do
    editor:handleEvent(event(IEvent.handleSelection, entry[1]))
    assert(native.calls[#native.calls].values.action==entry[2])
end
assert(native.calls[#native.calls].values["services.cloudSave"]==true)
editor:handleEvent(event(IEvent.handleSelection, editor.importJsonButton))
assert(editor.projectNameControl:getText()=="Imported project" and editor.settings.vsync==false)
assert(native.calls[#native.calls].values.action=="importProjectSettingsJson")
editor:handleEvent(event(IEvent.handleSelection, editor.saveButton))
assert(native.calls[#native.calls].values["services.cloudSave"]==false)

native.fail=true
editor:handleEvent(event(IEvent.handleSelection, editor.saveButton))
assert(editor.statusText.text=="Native validation rejected the selected backend.")
native.fail=false; native.available=false
editor:handleEvent(event(IEvent.handleSelection, editor.applyButton))
assert(editor.statusText.text:find("action is unavailable", 1, true))
native.available=true

before=#native.calls
editor:handleEvent(event(IEvent.handleSelection, editor.applyIosProfileButton))
assert(#native.calls==before and editor.settings.activePlatform==2 and editor.settings.aa==0)
assert(editor.settings.cloudSave==false and editor.statusText.text:find("profile staged", 1, true))
editor:handleEvent(event(IEvent.handleSelection, editor.resetButton))
assert(#native.calls==before and editor.settings.activePlatform==0 and editor.settings.cloudSave==false)
editor.cloudSaveControl:setValue(true)
editor:handleEvent(event(IEvent.handleValueChanged, editor.cloudSaveControl))
assert(editor.settings.cloudSave==false and editor.cloudSaveControl:getValue()==false)

local snapshot=properties()
editor:getProperties({at=function() return snapshot end})
assert(snapshot.kinds["graphics.frameRateLimit"]=="Int")
assert(snapshot.kinds["physics.gravity"]=="Float" and snapshot.values["services.cloudSave"]==false)
local legacy=properties(); legacy:setPropertyAsString("company", "Restored company")
editor:setProperties({at=function() return legacy end})
assert(editor.companyControl:getText()=="Restored company" and #native.calls==before)
local ownedWindow=editor.editorWindow
editor:unload()
assert(ownedWindow.removed and editor.editorWindow==nil and #editor.controlBindings==0)
native.fail=true
local unavailableEditor=setmetatable({}, {__index=ProjectSettings})
unavailableEditor:__init(window)
unavailableEditor:load()
for _, binding in ipairs(unavailableEditor.controlBindings) do assert(not binding.control.enabled) end
assert(unavailableEditor.settings.startupScene=="" and unavailableEditor.settings.sceneList=="")
unavailableEditor:unload()
print("ProjectSettings Lua contracts passed.")
