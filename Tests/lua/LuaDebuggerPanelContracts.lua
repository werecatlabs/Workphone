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
    function w:setSameLine() end
    function w:setSize() end
    function w:addChild() end
    function w:removeChild() end
    widgets[#widgets+1]=w; return w
end
local ui={addElement=function() return widget() end,removeElement=function() removed=removed+1 end}
local state={port=4555,paused=false,connected=false,ready=true,epoch=1,responses={},output='fixture output'}
local outgoing,history={},{}
local manager={getDebuggerPort=function() return 4555 end,getDebuggerStatus=function() return state end,
    debuggerCommand=function(_,request) outgoing[#outgoing+1]=request end}
local app={getUI=function() return ui end,getScriptManager=function() return manager end}
IApplicationManager={instance=function() return app end}
local parent=widget()
local window={getParentWindow=function() return parent end,setHandleEvents=function() end}
dofile('bin/Media/Scripts/Lua/Editor/LuaDebuggerPanel.lua')
local panel=setmetatable({},{__index=LuaDebuggerPanel}); panel:__init(window); panel:load()
local saved={version=1,files={},breakOnException=false}
local function tick()
    local batch=outgoing; outgoing={}
    for _, request in ipairs(batch) do
        assert(type(request)=='table','Unexpected command')
        history[#history+1]=request
        local body={}
        if request.command=='loadBreakpointPreferences' then body=saved
        elseif request.command=='saveBreakpointPreferences' then saved=request.arguments
        elseif request.command=='stackTrace' then body={stackFrames={{id=7,name='fixture',line=3,source={path='fixture.lua',sourceReference=1}}}}
        elseif request.command=='source' then body={content='one\ntwo\nthree\nfour'}
        elseif request.command=='scopes' then body={scopes={{name='Locals',variablesReference=8}}}
        elseif request.command=='variables' then body={variables={{name='child',value='table',variablesReference=9}}}
        elseif request.command=='evaluate' then body={result='42',variablesReference=0} end
        state.responses[#state.responses+1]={request_seq=request.seq,success=true,body=body}
        if #state.responses>8 then table.remove(state.responses,1) end
    end
    panel.lastUpdate=nil; panel:update()
end
local function ticks(n) for i=1,n do tick() end end
local function click(label)
    for i=#widgets,1,-1 do
        local w=widgets[i]
        if w.label==label and panel.actions[w.id] then
            panel:handleEvent({at=function(_,index) if index==1 then return IEvent.handleSelection elseif index==3 then return w end end})
            return
        end
    end
    error('Missing control '..label)
end
ticks(6); assert(panel.preferencesLoaded)
panel.file:setValue('fixture.lua'); panel.line:setValue('3'); panel.condition:setValue('count > 2')
panel.hit:setValue('>=4'); panel.log:setValue('value={count}')
click('Set breakpoint'); ticks(5)
assert(#saved.files==1 and saved.files[1].breakpoints[1].line==3)
assert(saved.files[1].breakpoints[1].condition=='count > 2' and saved.files[1].breakpoints[1].logMessage=='value={count}')
click('Remove breakpoint'); ticks(5); assert(#saved.files[1].breakpoints==0)
click('Break on errors: Off'); ticks(5); assert(saved.breakOnException)
state.paused=true; state.epoch=2; state.stop={reason='breakpoint'}; ticks(10)
assert(panel.frame.id==7 and panel.variables[1].name=='child')
assert(panel.sourceText:find('> 3  three',1,true))
panel.watch:setValue('6 * 7'); click('Evaluate'); ticks(5); assert(panel.watchResult=='42')
click('child = table'); ticks(5)
state.paused=false; ticks(3); assert(not panel.frame and not panel.watchResult and removed>0)
state.connected=true; ticks(2); panel.line:setValue('5'); click('Set breakpoint'); assert(panel.message:find('Visual Studio',1,true))
-- Many saved source groups must flow through a bounded number of outstanding requests.
state.connected=false; saved.files={}
for i=1,40 do saved.files[i]={source={path='file'..i..'.lua'},breakpoints={{line=1}}} end
panel:restoreBreakpoints(); ticks(50); assert(#panel.requests==0 and next(panel.pending)==nil)
assert(panel.status.text:find('fixture output',1,true))
panel:unload(); assert(not panel.status and next(panel.actions)==nil)
print('Lua Editor debugger panel contracts: PASS')
