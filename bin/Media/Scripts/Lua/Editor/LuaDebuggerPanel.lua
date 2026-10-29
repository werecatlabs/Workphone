class 'LuaDebuggerPanel' (BaseEditor)

function LuaDebuggerPanel:__init(window)
    self.window, self.sequence = window, 0
    self.pending, self.actions, self.rows, self.requests, self.controlButtons = {}, {}, {}, {}, {}
    self.staticButtons={}
    self.preferences = {version=1, files={}, breakOnException=false}
    self.message = ''
end
function LuaDebuggerPanel:manager() return IApplicationManager.instance():getScriptManager() end
function LuaDebuggerPanel:request(command, arguments, callback, generation)
    -- Reserve one queue slot for a run-control command, even when inspection
    -- or breakpoint configuration has filled the ordinary bounded queue.
    if #self.requests >= 511 then self.message='Debugger request queue is full.'; return end
    self.sequence = self.sequence+1
    self.requests[#self.requests+1] = {message={seq=self.sequence, type='request', command=command, arguments=arguments or {}}, callback=callback or function() end,generation=generation}
end
function LuaDebuggerPanel:button(label, action, dynamic, sameLine, shortcut)
    local button = self.ui:addElement(IUIButton.typeInfo())
    button:setLabel(label)
    self.nextId = self.nextId+1
    button:setElementId(self.nextId)
    button:setSameLine(sameLine or false)
    button:setEnabled(dynamic and self.state and self.state.ready or false,false)
    if shortcut then
        local properties=button:getProperties()
        properties:setProperty('keyboardShortcut',shortcut)
        button:setProperties(properties)
    end
    self.actions[self.nextId] = action
    self.parent:addChild(button)
    self.window:setHandleEvents(button, true)
    if dynamic then self.rows[#self.rows+1] = button end
    if not dynamic then self.staticButtons[#self.staticButtons+1]=button end
    return button
end
function LuaDebuggerPanel:field(label)
    local field = self.ui:addElement(IUILabelTextInputPair.typeInfo())
    field:setLabel(label); field:setValue(''); self.parent:addChild(field)
    return field
end
function LuaDebuggerPanel:load()
    self.ui = IApplicationManager.instance():getUI()
    self.parent = self.window:getParentWindow()
    self.parent:setLabel('Lua Debugger'); self.parent:setSize(Vector2F(850, 760))
    self.nextId = 0
    local commands, labels = {'pause','continue','stepIn','next','stepOut'}, {'Pause','Continue','Step Into','Step Over','Step Out'}
    local shortcuts={continue='F5',stepIn='F11',next='F10',stepOut='Shift+F11'}
    for i, command in ipairs(commands) do
        local action = command
        self.controlButtons[action] = self:button(labels[i], function() self:control(action) end, false, i>1,shortcuts[action])
    end
    self.file = self:field('Source file')
    self.line = self:field('Line')
    self.condition = self:field('Condition (optional)')
    self.hit = self:field('Hit count (N, >=N, >N, %N)')
    self.log = self:field('Log message ({expression}; blank stops)')
    self:button('Set breakpoint', function() self:editBreakpoint(false) end)
    self:button('Remove breakpoint', function() self:editBreakpoint(true) end, false, true)
    self.exceptionButton = self:button('Break on errors: Off', function()
        if self.state and self.state.connected then self.message='Edit exception settings in Visual Studio while attached.'; return end
        self.preferences.breakOnException = not self.preferences.breakOnException
        self:applyExceptions(); self:save()
    end)
    self.watch = self:field('Watch expression (read only)')
    self:button('Evaluate', function() self:evaluate() end)
    self:button('Show saved breakpoints', function() self.view='breakpoints'; self.dirty=true end, false, true)
    self:button('Show stack', function() self:refreshStack() end, false, true,'F8')
    self.status = self.ui:addElement(IUIText.typeInfo()); self.parent:addChild(self.status)
    self.staticId = self.nextId
    self:update()
end
function LuaDebuggerPanel:control(command)
    if not self.state or not self.state.ready then self.message='The debugger is not ready.'; return end
    if self.controlRequest or self.controlPending then self.message='A debugger control request is already pending.'; return end
    if (command=='pause')==self.state.paused then
        self.message=command=='pause' and 'Lua is already paused.' or 'Pause Lua before stepping or continuing.'
        return
    end
    -- Retire queued inspection before resuming/stepping. Existing display data
    -- stays visible until the native state changes or the command fails.
    self.generation=(self.generation or 0)+1
    for i=#self.requests,1,-1 do
        if self.requests[i].generation then table.remove(self.requests,i) end
    end
    self.sequence=self.sequence+1
    self.controlRequest={message={seq=self.sequence,type='request',command=command,arguments={threadId=1}},callback=function() end,control=true}
    self.message=''
end
function LuaDebuggerPanel:clearInspection()
    self.generation=(self.generation or 0)+1
    self.frame,self.frames,self.variables,self.scopes=nil,nil,nil,nil
    self.sourceText,self.watchResult,self.watchReference=nil,nil,nil
    for i=#self.requests,1,-1 do
        if self.requests[i].generation then table.remove(self.requests,i) end
    end
    self.dirty=true
end
function LuaDebuggerPanel:onDebuggerReady()
    if self.loadingPreferences or self.preferencesLoaded then return end
    self.loadingPreferences = true
    self:request('loadBreakpointPreferences', {}, function(body)
        self.loadingPreferences=false; self.preferencesLoaded=true; self.preferences=body
        self:restoreBreakpoints()
    end)
end
function LuaDebuggerPanel:save()
    self:request('saveBreakpointPreferences', self.preferences); self.dirty=true
end
function LuaDebuggerPanel:applyExceptions()
    if self.state and self.state.connected then return end
    self:request('setExceptionBreakpoints', {filters=self.preferences.breakOnException and {'uncaught'} or {}})
end
function LuaDebuggerPanel:applyGroup(group)
    self:request('setBreakpoints', group, function(body)
        local messages={}
        for _, point in ipairs(body.breakpoints or {}) do if point.message then messages[#messages+1]=point.message end end
        self.message = #messages>0 and table.concat(messages, '; ') or 'Breakpoints applied.'
    end)
end
function LuaDebuggerPanel:restoreBreakpoints()
    if not self.state or self.state.connected then return end
    for _, group in ipairs(self.preferences.files or {}) do self:applyGroup(group) end
    self:applyExceptions()
end
function LuaDebuggerPanel:editBreakpoint(remove)
    if not self.preferencesLoaded then self.message='Breakpoint preferences are still loading.'; return end
    if self.state and self.state.connected then self.message='Edit breakpoints in Visual Studio while attached.'; return end
    local path, line = self.file:getValue(), tonumber(self.line:getValue())
    if path=='' or not line or line<1 or line~=math.floor(line) then self.message='Enter a source file and a positive whole line number.'; return end
    local group
    for _, candidate in ipairs(self.preferences.files) do if candidate.source.path==path then group=candidate; break end end
    if not group then group={source={path=path},breakpoints={}}; self.preferences.files[#self.preferences.files+1]=group end
    for i=#group.breakpoints,1,-1 do if group.breakpoints[i].line==line then table.remove(group.breakpoints,i) end end
    if not remove then
        local point={line=line}
        if self.condition:getValue()~='' then point.condition=self.condition:getValue() end
        if self.hit:getValue()~='' then point.hitCondition=self.hit:getValue() end
        if self.log:getValue()~='' then point.logMessage=self.log:getValue() end
        group.breakpoints[#group.breakpoints+1]=point
    end
    self:applyGroup(group); self:save()
end
function LuaDebuggerPanel:refreshStack()
    self.view='stack'; self.dirty=true
    if not self.state or not self.state.paused then return end
    local epoch=self.state.epoch
    self:request('stackTrace', {threadId=1,levels=64}, function(body)
        if self.state and self.state.paused and self.state.epoch==epoch then
            self.frames=body.stackFrames or {}
            if self.frames[1] then self:selectFrame(self.frames[1]) end
            self.dirty=true
        end
    end,self.generation)
end
function LuaDebuggerPanel:selectFrame(frame)
    self.frame=frame; self.variables=nil; self.scopes=nil; self.sourceText=nil
    self.generation=(self.generation or 0)+1
    local generation=self.generation
    if frame.source then
        self.file:setValue(frame.source.path or ''); self.line:setValue(tostring(frame.line))
        if (frame.source.sourceReference or 0)>0 or (frame.source.path or '')~='' then
            self:request('source', {sourceReference=frame.source.sourceReference or 0,source=frame.source}, function(body)
                if self.generation~=generation then return end
                local lines,number={},0
                for text in ((body.content or '')..'\n'):gmatch('(.-)\n') do
                    number=number+1
                    if number>=frame.line-4 and number<=frame.line+4 then lines[#lines+1]=(number==frame.line and '> ' or '  ')..number..'  '..text end
                end
                self.sourceText=table.concat(lines,'\n')
            end,generation)
        end
    end
    self:request('scopes', {frameId=frame.id}, function(body)
        if self.generation~=generation then return end
        self.scopes=body.scopes or {}; self.dirty=true
        if self.scopes[1] then self:inspect(self.scopes[1].variablesReference) end
    end,generation)
    if self.watch:getValue()~='' then self:evaluate() end
    self.dirty=true
end
function LuaDebuggerPanel:inspect(reference)
    local generation=self.generation
    self:request('variables', {variablesReference=reference,count=128}, function(body)
        if self.generation~=generation then return end
        self.variables=body.variables or {}; self.dirty=true
    end,generation)
end
function LuaDebuggerPanel:evaluate()
    if not self.state or not self.state.paused or not self.frame then self.message='Pause and select a frame to inspect a watch.'; return end
    local generation=self.generation
    self:request('evaluate', {expression=self.watch:getValue(),frameId=self.frame.id,context='watch'}, function(body)
        if self.generation~=generation then return end
        self.watchResult=body.result; self.watchReference=body.variablesReference; self.dirty=true
    end,generation)
end
function LuaDebuggerPanel:rebuildRows()
    for _, row in ipairs(self.rows) do self.window:setHandleEvents(row,false); self.parent:removeChild(row); self.ui:removeElement(row) end
    self.rows={}
    for id in pairs(self.actions) do if id>self.staticId then self.actions[id]=nil end end
    -- Keep IDs monotonic: a click queued by the previous rendered frame must
    -- not invoke a newly rebuilt row that happens to reuse the old row's ID.
    if self.view=='breakpoints' then
        for _, group in ipairs(self.preferences.files or {}) do
            for _, point in ipairs(group.breakpoints or {}) do
                local source,saved=group.source.path,point
                self:button(source..':'..point.line..(point.logMessage and ' (log)' or ''),function()
                    self.file:setValue(source); self.line:setValue(tostring(saved.line)); self.condition:setValue(saved.condition or '')
                    self.hit:setValue(saved.hitCondition or ''); self.log:setValue(saved.logMessage or '')
                end,true)
            end
        end
        return
    end
    if not self.state or not self.state.paused then return end
    for _, frame in ipairs(self.frames or {}) do
        local selected=frame
        self:button((self.frame==frame and '> ' or '')..tostring(frame.name)..' :'..frame.line,function() self:selectFrame(selected) end,true)
    end
    for _, scope in ipairs(self.scopes or {}) do
        local reference=scope.variablesReference
        self:button(scope.name,function() self:inspect(reference) end,true,true)
    end
    if self.watchReference and self.watchReference>0 then
        local reference=self.watchReference
        self:button('Expand watch',function() self:inspect(reference) end,true)
    end
    for _, variable in ipairs(self.variables or {}) do
        local reference=variable.variablesReference or 0
        self:button(tostring(variable.name)..' = '..tostring(variable.value),function() if reference>0 then self:inspect(reference) end end,true)
    end
end
function LuaDebuggerPanel:update()
    if not self.status then return end
    local now=os.clock()
    if self.lastUpdate and now-self.lastUpdate<0.05 then return end
    self.lastUpdate=now
    if self:manager():getDebuggerPort()==0 then
        for _,button in ipairs(self.staticButtons) do button:setEnabled(false,false) end
        if self.state then
            self.state=nil; self:clearInspection(); self.pending,self.requests={},{}
            self.controlRequest,self.controlPending=nil,nil
            self.preferencesLoaded,self.loadingPreferences=false,false
            self:rebuildRows()
        end
        self.status:setText('Start the Editor with WP_LUA_DEBUGGER=1 to enable debugging.'); return
    end
    local ok,state=pcall(cjson.decode,self:manager():getDebuggerStatus())
    if not ok then self.status:setText('Debugger status unavailable'); return end
    local previous=self.state; self.state=state
    local newStop=state.paused and (not previous or not previous.paused or previous.epoch~=state.epoch)
    local resumed=not state.paused and previous and previous.paused
    -- Invalidate before dispatching replies from the previous stop. Old scope
    -- callbacks must not enqueue inspection against expired native handles.
    if newStop or resumed then self:clearInspection() end
    for _, reply in ipairs(state.responses or {}) do
        local request=self.pending[reply.request_seq]
        if request then
            self.pending[reply.request_seq]=nil
            if request.control then self.controlPending=nil end
            if not request.generation or request.generation==self.generation then
                if reply.success then request.callback(reply.body or {})
                else
                    self.message=reply.message or 'Debugger request failed.'
                    if reply.command=='loadBreakpointPreferences' then
                        self.loadingPreferences=false; self.preferencesLoaded=true
                        self.message=self.message..' Start new preferences by setting a breakpoint.'
                    end
                end
            end
        end
    end
    if state.ready and not self.preferencesLoaded then self:onDebuggerReady() end
    if self.preferencesLoaded and previous and previous.connected and not state.connected then self:restoreBreakpoints() end
    if newStop then self:refreshStack() end
    for _,button in ipairs(self.staticButtons) do button:setEnabled(state.ready==true,false) end
    for command, button in pairs(self.controlButtons) do
        button:setEnabled(state.ready and not self.controlRequest and not self.controlPending and (command=='pause' and not state.paused or command~='pause' and state.paused),false)
    end
    self.exceptionButton:setLabel('Break on errors: '..(self.preferences.breakOnException and 'On' or 'Off'))
    local text=(jit and (jit.version..' (JIT disabled)') or _VERSION)..' | '..(state.connected and 'Visual Studio connected' or 'Editor debugger')..' | '..(state.paused and 'Stopped' or 'Running')..'\n'..self.message
    if state.error and state.error~='' then text=text..'\n'..state.error end
    if state.paused and state.stop then text=text..'\n'..tostring(state.stop.reason) end
    if self.watchResult then text=text..'\nWatch: '..self.watchResult end
    if self.sourceText then text=text..'\n'..self.sourceText end
    if state.output and state.output~='' then text=text..'\nConsole:\n'..state.output end
    self.status:setText(text)
    if self.dirty then self.dirty=false; self:rebuildRows() end
    local count=0
    for _,request in pairs(self.pending) do if not request.control then count=count+1 end end
    -- Four ordinary requests may already be awaiting native responses. Run
    -- control has its own single in-flight allowance so it cannot be starved.
    if self.controlRequest then
        local request=self.controlRequest
        self.controlRequest=nil; self.controlPending=request
        self.pending[request.message.seq]=request
        self:manager():debuggerCommand(cjson.encode(request.message))
    end
    while count<4 and #self.requests>0 do
        local request=table.remove(self.requests,1)
        self.pending[request.message.seq]=request
        self:manager():debuggerCommand(cjson.encode(request.message))
        count=count+1
    end
end
function LuaDebuggerPanel:handleEvent(parameters, results)
    if not parameters or parameters:at(1)~=IEvent.handleSelection then return end
    local sender=parameters:at(3)
    local action=sender and self.actions[sender:getElementId()]
    if action then action() end
end
function LuaDebuggerPanel:show() self.lastUpdate=nil; self:update() end
function LuaDebuggerPanel:unload()
    for _, row in ipairs(self.rows) do self.window:setHandleEvents(row,false) end
    self.status=nil; self.actions,self.rows,self.pending,self.requests,self.controlButtons,self.staticButtons={},{},{},{},{},{}
    self.controlRequest,self.controlPending=nil,nil
end
