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
Parameters = function()
    return {push_back=function(self, value) table.insert(self, value) end,
        getAsStringArray=function(self) return self end}
end
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
    function result:setOptions(value) self.options=value end
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
local native={calls={}, fail=false, available=true}
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
        request:setPropertyAsInt("graphics.shadowQuality", 4)
        request:setPropertyAsInt("graphics.customWidth", 1173)
        request:setPropertyAsInt("graphics.customHeight", 641)
        request:setPropertyAsBool("inherited.graphics.renderPipeline", true)
        request:setPropertyAsBool("capability.graphics.antiAliasing.5", false)
        request:setPropertyAsString("unavailable.graphics.antiAliasing.5", "This device does not support MSAA 8x.")
        request:setPropertyAsString("availability.graphics.presentMode", "Mailbox requires a renderer restart.")
        request:setPropertyAsBool("capability.services.cloudSave", false)
        request:setPropertyAsString("unavailable.services.cloudSave", "No cloud provider configured")
        request:setPropertyAsBool("services.cloudSave", true)
    elseif action=="importProjectSettingsJson" then
        assert(not request:hasProperty("project.name"), "Import must not overwrite file values with current controls")
        request:setPropertyAsString("project.name", "Imported project")
        request:setPropertyAsString("project.company", "")
        request:setPropertyAsBool("graphics.vsync", false)
        request:setPropertyAsBool("graphics.hdr", true)
        request:setPropertyAsInt("editor.theme", 1)
        request:setPropertyAsBool("services.cloudSave", false)
        for _, key in ipairs({"project.name", "graphics.vsync", "graphics.hdr", "services.cloudSave"}) do
            request:setPropertyAsBool("included." .. key, true)
        end
    elseif native.onCommit then
        native.onCommit(request)
        native.onCommit=nil
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
assert(#editor.broadphaseControl.options==3, "Stock Editor exposes the three CPU broadphases")
assert(table.concat(editor.shadowQualityControl.options, ",")=="Off,Low,Medium,High,Ultra")
assert(editor.settings.shadowQuality==4 and editor.shadowQualityControl:getSelectedOption()==4,
    "Reading Ultra shadows must preserve its schema value in the dropdown")
assert(not editor.cloudSaveControl.enabled and editor.cloudSaveControl.label:find("(Unavailable)", 1, true))
assert(editor.settings.cloudSave==true)
assert(#editor.controlBindings==132, "All project settings must be bound")

local function event(kind, sender)
    return {at=function(_, i) if i==1 then return kind elseif i==3 then return sender end end}
end
local function assertNoSettings(request)
    for key in pairs(request.values) do
        assert(not key:find(".", 1, true), "An unedited draft must omit project key " .. key)
    end
end
editor:handleEvent(event(IEvent.handleSelection, editor.saveButton))
assertNoSettings(native.calls[#native.calls])
editor.editorThemeControl:setSelectedOption(2)
editor:handleEvent(event(IEvent.handleSelection, editor.editorThemeControl))
editor.editorThemeControl:setSelectedOption(1)
editor:handleEvent(event(IEvent.handleSelection, editor.editorThemeControl))
editor:handleEvent(event(IEvent.handleSelection, editor.saveButton))
assertNoSettings(native.calls[#native.calls])
assert(editor.renderPipelineControl.label:find("(Inherited)", 1, true))
editor.renderPipelineControl:setSelectedOption(0)
editor:handleEvent(event(IEvent.handleSelection, editor.renderPipelineControl))
editor.renderPipelineControl:setSelectedOption(1)
editor:handleEvent(event(IEvent.handleSelection, editor.renderPipelineControl))
editor:handleEvent(event(IEvent.handleSelection, editor.saveButton))
assertNoSettings(native.calls[#native.calls])
editor:handleEvent(event(IEvent.handleSelection, editor.renderPipelineControl))
editor:handleEvent(event(IEvent.handleSelection, editor.saveButton))
assert(native.calls[#native.calls].values["graphics.renderPipeline"]==1,
    "Explicitly choosing an inherited Graphics entry must stage that value")
assert(not editor.renderPipelineControl.label:find("(Inherited)", 1, true))

assert(not editor.customWidthControl.enabled and not editor.customHeightControl.enabled)
assert(editor.aaControl.options[6]=="MSAA 8x (Unavailable)")
editor.aaControl:setSelectedOption(5)
editor:handleEvent(event(IEvent.handleSelection, editor.aaControl))
assert(editor.aaControl:getSelectedOption()==2 and editor.graphicsStatusText.text=="This device does not support MSAA 8x.")
editor.resolutionControl:setSelectedOption(5)
editor:handleEvent(event(IEvent.handleSelection, editor.resolutionControl))
assert(editor.customWidthControl.enabled and editor.customHeightControl.enabled)
native.onCommit=function(request)
    assert(request.values["graphics.resolution"]==5 and request.values["graphics.customWidth"]==1173 and
        request.values["graphics.customHeight"]==641, "Selecting Custom must stage both displayed dimensions")
    assert(request.values["graphics.antiAliasing"]==nil, "An unavailable Graphics choice must not be committed")
    request:setPropertyAsBool("settingsSnapshot", true)
    request:setPropertyAsString("availability.graphics.presentMode", "Mailbox requires a renderer restart.")
    request:setPropertyAsBool("graphics.hdr", false)
end
editor:handleEvent(event(IEvent.handleSelection, editor.applyButton))
assert(editor.aaControl.options[6]=="MSAA 8x" and not editor.hdrControl:getValue(),
    "A refreshed profile must replace the previous capabilities and effective values")
editor:handleEvent(event(IEvent.handleSelection, editor.saveButton))
assertNoSettings(native.calls[#native.calls])

-- Legacy window readback displays Custom even when the project has no resolution override.
native.onCommit=function(request)
    request:setPropertyAsBool("settingsSnapshot", true)
    request:setPropertyAsBool("inherited.graphics.resolution", true)
    request:setPropertyAsString("availability.graphics.presentMode", "Mailbox requires a renderer restart.")
end
editor:handleEvent(event(IEvent.handleSelection, editor.saveButton))
editor.customWidthControl:setValue(1234)
editor:handleEvent(event(IEvent.handleValueChanged, editor.customWidthControl))
editor:handleEvent(event(IEvent.handleSelection, editor.applyButton))
local customOverride=native.calls[#native.calls].values
assert(customOverride["graphics.resolution"]==5 and customOverride["graphics.customWidth"]==1234 and
    customOverride["graphics.customHeight"]==641, "Editing an inherited Custom size must enable explicit window sizing")

editor.presentModeControl:setSelectedOption(2)
editor:handleEvent(event(IEvent.handleSelection, editor.presentModeControl))
assert(not editor.vsyncControl.enabled and editor.graphicsStatusText.text:find("requires a renderer restart", 1, true))
editor.vsyncControl:setValue(false)
editor:handleEvent(event(IEvent.handleValueChanged, editor.vsyncControl))
assert(editor.vsyncControl:getValue() and editor.settings.vsync)
editor.presentModeControl:setSelectedOption(0)
editor:handleEvent(event(IEvent.handleSelection, editor.presentModeControl))
assert(editor.vsyncControl.enabled)
editor:handleEvent(event(IEvent.handleSelection, editor.saveButton))
assertNoSettings(native.calls[#native.calls])

local initialCalls=#native.calls
editor.projectNameControl:setText("Edited project")
local senderProxy=setmetatable({}, {__index=editor.projectNameControl,
    __eq=function() error("UI userdata equality is unsupported; compare element IDs") end})
editor:handleEvent(event(IEvent.handlePropertyChanged, senderProxy))
assert(editor.settings.projectName=="Edited project" and #native.calls==initialCalls)
editor.vsyncControl:setValue(false)
editor:handleEvent(event(IEvent.handleSelection, editor.vsyncControl))
assert(editor.settings.vsync==false and #native.calls==initialCalls)
editor.activePlatformControl:setSelectedOption(3)
editor:handleEvent(event(IEvent.handleSelection, editor.activePlatformControl))
assert(editor.settings.activePlatform==3 and editor.rendererSummaryText.text:find("Active: Android", 1, true))
editor.gravityControl:setValue(-4.5)
editor:handleEvent(event(IEvent.handleValueChanged, editor.gravityControl))
assert(editor.settings.gravity==-4.5)
assert(editor.settings.shadowQuality==4,
    "Editing unrelated controls must not downgrade stored Ultra shadows")
local before=#native.calls
editor:handleEvent(event(IEvent.handleValueChanged, element()))
assert(#native.calls==before)

editor:handleEvent(event(IEvent.handleSelection, editor.saveButton))
local saved=native.calls[#native.calls]
assert(saved.values.action=="saveProjectSettings" and saved.values["project.name"]=="Edited project")
assert(saved.kinds["graphics.vsync"]=="Bool" and saved.kinds["physics.gravity"]=="Float")
assert(saved.kinds["platform.active"]=="Int" and saved.kinds["project.company"]==nil)
assert(saved.values["graphics.shadowQuality"]==nil and saved.values["editor.theme"]==nil and
    saved.values["graphics.hdr"]==nil and saved.values["graphics.resolution"]==nil,
    "Save must omit unrelated stored and default display settings")
assert(saved.values["services.cloudSave"]==nil,
    "Save must leave unavailable settings alone unless explicitly imported")
assert(editor.settings.shadowQuality==4, "Save must keep the displayed stored shadow quality")
assert(editor.statusText.text=="saveProjectSettings completed")
assert(editor.settings.cloudSave==true)

editor:handleEvent(event(IEvent.handleSelection, editor.applyButton))
assertNoSettings(native.calls[#native.calls])

for _, entry in ipairs({{editor.validateButton, "validateProjectSettings"}, {editor.reloadRendererButton, "reloadRenderer"},
    {editor.buildSelectedButton, "buildSelectedPlatform"}, {editor.exportJsonButton, "exportProjectSettingsJson"}}) do
    editor:handleEvent(event(IEvent.handleSelection, entry[1]))
    local request=native.calls[#native.calls]
    assert(request.values.action==entry[2])
    if entry[2]~="buildSelectedPlatform" then
        assert(request.values["graphics.shadowQuality"]==4 and request.values["services.cloudSave"]==true)
    else
        assertNoSettings(request)
    end
end
editor:handleEvent(event(IEvent.handleSelection, editor.importJsonButton))
assert(editor.projectNameControl:getText()=="Imported project" and editor.settings.vsync==false)
assert(native.calls[#native.calls].values.action=="importProjectSettingsJson")
editor:handleEvent(event(IEvent.handleSelection, editor.saveButton))
local imported=native.calls[#native.calls].values
assert(imported["project.name"]=="Imported project" and imported["graphics.vsync"]==false and
    imported["graphics.hdr"]==true and imported["services.cloudSave"]==false,
    "Explicit imported values must be staged even when they match the displayed defaults")
assert(imported["editor.theme"]==nil and imported["project.company"]==nil,
    "The merged import response must not stage values absent from the file")

editor.gravityControl:setValue(-6.25)
native.fail=true
editor:handleEvent(event(IEvent.handleSelection, editor.saveButton))
assert(editor.statusText.text=="Native validation rejected the selected backend.")
assert(native.calls[#native.calls].values["physics.gravity"]==-6.25)
native.fail=false; native.available=false
editor:handleEvent(event(IEvent.handleSelection, editor.applyButton))
assert(editor.statusText.text:find("action is unavailable", 1, true))
native.available=true
editor:handleEvent(event(IEvent.handleSelection, editor.saveButton))
assert(native.calls[#native.calls].values["physics.gravity"]==-6.25,
    "Failed actions must preserve unsaved control values for retry")

before=#native.calls
editor:handleEvent(event(IEvent.handleSelection, editor.applyIosProfileButton))
assert(#native.calls==before and editor.settings.activePlatform==2 and editor.settings.aa==0)
assert(editor.settings.cloudSave==false and editor.statusText.text:find("profile staged", 1, true))
local platformProfiles = {
    {button=editor.applyPcProfileButton, shadows=3, platform="pc", compression=1},
    {button=editor.applyMacProfileButton, shadows=3, platform="mac", compression=1},
    {button=editor.applyIosProfileButton, shadows=2, platform="ios", compression=0},
    {button=editor.applyAndroidProfileButton, shadows=1, platform="android", compression=0},
}
for _, profile in ipairs(platformProfiles) do
    local callsBeforeProfile=#native.calls
    editor:handleEvent(event(IEvent.handleSelection, profile.button))
    assert(#native.calls==callsBeforeProfile, "Platform profiles must stage changes without applying them")
    assert(editor.settings.shadowQuality==profile.shadows and
        editor.shadowQualityControl:getSelectedOption()==profile.shadows,
        "A staged platform profile must display its exact shadow quality")
    editor.companyControl:setText("Profile company")
    editor:handleEvent(event(IEvent.handlePropertyChanged, editor.companyControl))
    for _, button in ipairs({editor.validateButton, editor.applyButton}) do
        editor:handleEvent(event(IEvent.handleSelection, button))
        local values=native.calls[#native.calls].values
        assert(values["graphics.shadowQuality"]==profile.shadows,
            "Unrelated edits must preserve the profile's shadow quality")
        assert(values["assets.textureCompression."..profile.platform]==profile.compression,
            "Platform profiles must use texture formats supported by the bundled encoders")
        if button==editor.applyButton then
            assert(values["editor.theme"]==nil and values["graphics.hdr"]==nil,
                "Profiles must stage only the fields they set")
        end
    end
    editor:handleEvent(event(IEvent.handleSelection, editor.saveButton))
    assertNoSettings(native.calls[#native.calls])
end
before=#native.calls
editor:handleEvent(event(IEvent.handleSelection, editor.resetButton))
assert(#native.calls==before and editor.settings.activePlatform==0 and editor.settings.cloudSave==false)
editor.cloudSaveControl:setValue(true)
editor:handleEvent(event(IEvent.handleValueChanged, editor.cloudSaveControl))
assert(editor.settings.cloudSave==false and editor.cloudSaveControl:getValue()==false)
editor:handleEvent(event(IEvent.handleSelection, editor.saveButton))
local reset=native.calls[#native.calls].values
assert(reset["editor.theme"]==1 and reset["graphics.hdr"]==true and reset["services.cloudSave"]==nil,
    "Reset must explicitly stage all available defaults while preserving unavailable values")
before=#native.calls

local snapshot=properties()
editor:getProperties({at=function() return snapshot end})
assert(snapshot.kinds["graphics.frameRateLimit"]=="Int")
assert(snapshot.kinds["physics.gravity"]=="Float" and snapshot.values["services.cloudSave"]==false)
local legacy=properties(); legacy:setPropertyAsString("company", "Restored company")
editor:setProperties({at=function() return legacy end})
assert(editor.companyControl:getText()=="Restored company" and #native.calls==before)
legacy:setPropertyAsInt("physics.broadphase", 3)
editor:setProperties({at=function() return legacy end})
assert(editor.broadphaseControl:getSelectedOption()==3 and
    editor.broadphaseControl.options[4]=="GPU Provider (Unavailable)")
local savedGpu=properties()
editor:getProperties({at=function() return savedGpu end})
assert(savedGpu.values["physics.broadphase"]==3, "An absent provider must not silently change a stored choice")
editor.broadphaseControl:setSelectedOption(2)
editor:handleEvent(event(IEvent.handleSelection, editor.broadphaseControl))
assert(editor.settings.broadphase==2 and #editor.broadphaseControl.options==3)
local plugin=properties()
plugin:setPropertyAsBool("capability.physics.broadphase.gpu", true)
editor:setProperties({at=function() return plugin end})
assert(editor.broadphaseControl.options[4]=="GPU Provider")
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
