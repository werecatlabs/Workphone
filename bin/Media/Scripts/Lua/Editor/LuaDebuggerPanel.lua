class 'LuaDebuggerPanel' (BaseEditor)

function LuaDebuggerPanel:__init(window)
    self.window = window
    self.status = nil
end

function LuaDebuggerPanel:load()
    local ui = IApplicationManager.instance():getUI()
    local parent = self.window:getParentWindow()
    parent:setLabel("Lua Debugger")
    parent:setSize(Vector2F(650, 420))
    local commands = {"pause", "continue", "stepIn", "next", "stepOut"}
    local labels = {"Pause", "Continue", "Step Into", "Step Over", "Step Out"}
    for i, command in ipairs(commands) do
        local button = ui:addElement(IUIButton.typeInfo())
        button:setLabel(labels[i])
        button:setElementId(i)
        button:setSameLine(i > 1)
        parent:addChild(button)
        self.window:setHandleEvents(button, true)
    end
    self.status = ui:addElement(IUIText.typeInfo())
    parent:addChild(self.status)
    self:update()
end

function LuaDebuggerPanel:update()
    if not self.status then return end
    local manager = IApplicationManager.instance():getScriptManager()
    if manager:getDebuggerPort() == 0 then
        self.status:setText("Start the Editor with WP_LUA_DEBUGGER=1 to enable debugging.\nOpen .workphone/lua-debug.json in the Visual Studio Lua Debugger tool window.")
        return
    end
    local ok, state = pcall(cjson.decode, manager:getDebuggerStatus())
    if not ok then self.status:setText("Debugger status unavailable") return end
    local text = (jit and (jit.version .. " (JIT disabled for debugging)") or _VERSION) .. "\n"
    text = text .. "Local port: " .. tostring(state.port) .. "\n"
    text = text .. (state.connected and "Client attached\n" or "Waiting for Visual Studio\n")
    text = text .. (state.paused and "Stopped\n" or "Running\n")
    if state.paused and state.stop then
        text = text .. tostring(state.stop.reason) .. "\n"
        for _, frame in ipairs(state.stop.stack or {}) do
            text = text .. tostring(frame.name) .. "  " .. tostring(frame.source) .. ":" .. tostring(frame.line) .. "\n"
        end
    end
    self.status:setText(text)
end

function LuaDebuggerPanel:handleEvent(parameters, results)
    if not parameters or parameters:at(1) ~= IEvent.handleSelection then return end
    local sender = parameters:at(3)
    if not sender then return end
    local commands = {"pause", "continue", "stepIn", "next", "stepOut"}
    local command = commands[sender:getElementId()]
    if command then IApplicationManager.instance():getScriptManager():debuggerCommand(command) end
end

function LuaDebuggerPanel:show() self:update() end
function LuaDebuggerPanel:unload() self.status = nil end
