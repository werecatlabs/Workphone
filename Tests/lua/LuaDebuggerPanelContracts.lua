-- UI contract fixture. Native host tests separately exercise the real service.
function class(name) _G[name]={}; return function() end end
BaseEditor={}
Vector2F=function(x,y) return {x=x,y=y} end
IEvent={handleSelection=1}
for _, name in ipairs({'IUIButton','IUIText','IUILabelTextInputPair'}) do _G[name]={typeInfo=function() return name end} end
local function copy(value)
    if type(value)~='table' then return value end
    local result={}; for key,item in pairs(value) do result[key]=copy(item) end; return result
end
cjson={encode=copy,decode=copy}
local widgets,removed={},0
local function widget()
    local w={}
    function w:setLabel(v) self.label=v end
    function w:setText(v) self.text=v end
    function w:setValue(v) self.value=v end
    function w:getValue() return self.value end
    function w:setElementId(v) self.id=v end
    function w:getElementId() return self.id end
    function w:setSameLine(value) self.sameLine=value end
    function w:setEnabled(value) self.enabled=value end
    function w:getProperties()
        return {setProperty=function(properties,name,value) properties[name]=value end}
    end
    function w:setProperties(properties) self.properties=properties end
    function w:setSize() end
    function w:addChild() end
    function w:removeChild() end
    widgets[#widgets+1]=w; return w
end
local ui={addElement=function() return widget() end,removeElement=function() removed=removed+1 end}
local state={port=4555,paused=false,connected=false,ready=true,epoch=1,responses={},output='fixture output'}
local outgoing,history={},{}
local rejectCommand
local debuggerPort=0
local manager={getDebuggerPort=function() return debuggerPort end,getDebuggerStatus=function() return state end,
    debuggerCommand=function(_,request) outgoing[#outgoing+1]=request end}
local app={getUI=function() return ui end,getScriptManager=function() return manager end}
IApplicationManager={instance=function() return app end}
local parent=widget()
local window={getParentWindow=function() return parent end,setHandleEvents=function() end}
dofile('bin/Media/Scripts/Lua/Editor/LuaDebuggerPanel.lua')
local panel=setmetatable({},{__index=LuaDebuggerPanel}); panel:__init(window); panel:load()
assert(parent.properties.initialPlacement=='floating','Debugger must request usable first-use floating placement')
assert(not panel.controlButtons.pause.sameLine and panel.controlButtons.continue.sameLine and
       not panel.controlButtons.stepIn.sameLine and panel.controlButtons.next.sameLine and panel.controlButtons.stepOut.sameLine,
       'Debugger run controls and stepping controls must use separate compact rows')
for _,button in ipairs(panel.staticButtons) do assert(button.enabled==false,'Debugger-disabled controls were enabled at startup') end
assert(not panel.preferencesLoaded and panel.status.text:find('WP_LUA_DEBUGGER=1',1,true),'Disabled debugger did not explain availability')
debuggerPort=4555
local saved={version=1,files={},breakOnException=false}
local function tick()
    local batch=outgoing; outgoing={}
    for _, request in ipairs(batch) do
        assert(type(request)=='table','Unexpected command')
        history[#history+1]=request
        local body={}
        if request.command=='loadBreakpointPreferences' then body=saved
        elseif request.command=='saveBreakpointPreferences' then saved=request.arguments
        elseif request.command=='stackTrace' then body={stackFrames={{id=7,name='fixture',line=3,source={path='fixture.lua',sourceReference=0}}}}
        elseif request.command=='source' then
            assert(request.arguments.source.path=='fixture.lua' and request.arguments.sourceReference==0,'Physical source path was lost')
            body={content='one\ntwo\nthree\nfour'}
        elseif request.command=='scopes' then body={scopes={{name='Locals',variablesReference=8}}}
        elseif request.command=='variables' then body={variables={{name='child',value='table',variablesReference=9}}}
        elseif request.command=='evaluate' then body={result='42',variablesReference=0} end
        state.responses[#state.responses+1]={request_seq=request.seq,success=request.command~=rejectCommand,body=body,message='Fixture control failed'}
        if #state.responses>8 then table.remove(state.responses,1) end
    end
    panel.lastUpdate=nil; panel:update()
end
local function ticks(n) for i=1,n do tick() end end
local function click(label)
    for i=#widgets,1,-1 do
        local w=widgets[i]
        if w.label==label and panel.actions[w.id] then
            assert(w.enabled~=false,'Disabled control '..label)
            panel:handleEvent({at=function(_,index) if index==1 then return IEvent.handleSelection elseif index==3 then return w end end})
            return
        end
    end
    error('Missing control '..label)
end
ticks(6); assert(panel.preferencesLoaded)
for command,shortcut in pairs({continue='F5',stepIn='F11',next='F10',stepOut='Shift+F11'}) do
    assert(panel.controlButtons[command].properties.keyboardShortcut==shortcut,'Debugger button keyboard metadata is missing')
end
local stackShortcut=false
for _,w in ipairs(widgets) do if w.label=='Show stack' then stackShortcut=w.properties.keyboardShortcut=='F8' end end
assert(stackShortcut,'Show stack must expose its native button accelerator')
assert(panel.controlButtons.pause.enabled and not panel.controlButtons.next.enabled,'Running debugger controls are incorrect')
panel:control('next'); assert(#panel.requests==0 and panel.message:find('Pause Lua',1,true))
click('Pause'); ticks(3); assert(history[#history].command=='pause','Pause button did not issue a debugger request')
panel.file:setValue('fixture.lua'); panel.line:setValue('3'); panel.condition:setValue('count > 2')
panel.hit:setValue('>=4'); panel.log:setValue('value={count}')
click('Set breakpoint'); ticks(5)
assert(#saved.files==1 and saved.files[1].breakpoints[1].line==3)
assert(saved.files[1].breakpoints[1].condition=='count > 2' and saved.files[1].breakpoints[1].logMessage=='value={count}')
click('Remove breakpoint'); ticks(5); assert(#saved.files[1].breakpoints==0)
click('Break on errors: Off'); ticks(5); assert(saved.breakOnException)
state.paused=true; state.epoch=2; state.stop={reason='breakpoint'}; ticks(10)
assert(panel.frame.id==7 and panel.variables[1].name=='child')
assert(not panel.controlButtons.pause.enabled and panel.controlButtons.next.enabled,'Stopped debugger controls are incorrect')
assert(panel.sourceText:find('> 3  three',1,true))
panel.watch:setValue('6 * 7'); click('Evaluate'); ticks(5); assert(panel.watchResult=='42')
click('child = table'); ticks(5)
-- A queued click from the rendered frame must not target a replacement row.
local oldRow
for _, w in ipairs(widgets) do if w.label=='child = table' and panel.actions[w.id] then oldRow=w end end
panel.dirty=true; ticks(1)
assert(not panel.actions[oldRow.id],'Dynamic row IDs were reused')
local before=#panel.requests
panel:handleEvent({at=function(_,index) if index==1 then return IEvent.handleSelection elseif index==3 then return oldRow end end})
assert(#panel.requests==before,'An obsolete row click invoked a new action')
for _, control in ipairs({{'Step Into','stepIn'},{'Step Over','next'},{'Step Out','stepOut'},{'Continue','continue'}}) do
    click(control[1]); ticks(3); assert(history[#history].command==control[2],control[1]..' did not reach the debugger queue')
end
rejectCommand='next'; click('Step Over'); ticks(3)
assert(panel.message=='Fixture control failed','Debugger control failures were hidden')
rejectCommand=nil
-- A late old-stop scopes reply must not enqueue inspection of expired handles.
panel:selectFrame(panel.frame)
local scopesRequest,evaluateRequest
for _, request in ipairs(panel.requests) do
    if request.message.command=='scopes' then scopesRequest=request.message end
    if request.message.command=='evaluate' then evaluateRequest=request.message end
end
panel.lastUpdate=nil; panel:update() -- flush requests into the native pending map
state.epoch=3
local previousMessage=panel.message
state.responses={{request_seq=scopesRequest.seq,success=true,body={scopes={{name='Expired',variablesReference=999}}}},
    {request_seq=evaluateRequest.seq,success=false,message='Expired old watch'}}
panel.lastUpdate=nil; panel:update()
assert(not panel.frame and not panel.watchResult,'Old inspection survived a new stop')
assert(panel.message==previousMessage,'An old inspection failure replaced the current stop message')
for _, request in ipairs(outgoing) do assert(request.command~='variables' or request.arguments.variablesReference~=999,'Old scopes reply inspected an expired handle') end
ticks(10); assert(panel.frame and panel.watchResult=='42','Inspection did not recover at the new stop')
-- Run controls must bypass a full ordinary queue and four unanswered requests.
for i=1,4 do panel:request('setBreakpoints',{source={path='pending'..i..'.lua'},breakpoints={}}) end
panel.lastUpdate=nil; panel:update()
local stalled=outgoing; outgoing={}
assert(#stalled==4,'Saturation fixture must have four actual outstanding requests')
for i=1,510 do panel:request('setBreakpoints',{source={path='queued'..i..'.lua'},breakpoints={}}) end
panel:inspect(8)
assert(#panel.requests==511,'Ordinary queue did not reach its bounded capacity')
panel:request('variables',{variablesReference=9},nil,panel.generation)
assert(#panel.requests==511 and panel.message:find('queue is full',1,true),'Ordinary requests exceeded the queue bound')
click('Continue')
assert(#panel.requests==510,'Control did not retire queued inspection while preserving breakpoint configuration')
panel:control('continue')
assert(panel.message:find('already pending',1,true),'Duplicate queued control was accepted')
panel.lastUpdate=nil; panel:update()
assert(#outgoing==1 and outgoing[1].command=='continue','Continue was starved behind saturated inspection/configuration requests')
assert(not panel.controlButtons.continue.enabled,'In-flight controls must be visibly unavailable')
local continuation=outgoing[1]; outgoing={}
panel:control('next')
assert(not panel.controlRequest,'A second in-flight run control was accepted')
state.responses={{request_seq=continuation.seq,success=true,body={}}}
panel.lastUpdate=nil; panel:update()
click('Step Over'); panel.lastUpdate=nil; panel:update()
assert(#outgoing==1 and outgoing[1].command=='next','Step Over did not retain reserved transport capacity after Continue')
local stepping=outgoing[1]; outgoing={}
state.responses={{request_seq=stepping.seq,success=false,message='Saturated control failed'}}
panel.lastUpdate=nil; panel:update()
assert(panel.message=='Saturated control failed' and panel.controlButtons.next.enabled,'Failed priority control did not report failure and restore controls')
state.responses={}
for _,request in ipairs(stalled) do state.responses[#state.responses+1]={request_seq=request.seq,success=true,body={}} end
panel.lastUpdate=nil; panel:update(); ticks(140)
assert(#panel.requests==0 and next(panel.pending)==nil,'Ordinary configuration did not recover after priority controls completed')
state.paused=false; ticks(3); assert(not panel.frame and not panel.watchResult and removed>0)
state.connected=true; ticks(2); panel.line:setValue('5'); click('Set breakpoint'); assert(panel.message:find('Visual Studio',1,true))
-- Many saved source groups must flow through a bounded number of outstanding requests.
state.connected=false; saved.files={}
for i=1,40 do saved.files[i]={source={path='file'..i..'.lua'},breakpoints={{line=1}}} end
panel:restoreBreakpoints(); ticks(50); assert(#panel.requests==0 and next(panel.pending)==nil)
assert(panel.status.text:find('fixture output',1,true))
debuggerPort=0; ticks(1)
for _,button in ipairs(panel.staticButtons) do assert(button.enabled==false,'Controls stayed enabled after debugger shutdown') end
panel:unload(); assert(not panel.status and next(panel.actions)==nil)
print('Lua Editor debugger panel contracts: PASS')
