if not StartMenu then include("StartMenu.lua") end
class 'MenuManager' (BaseComponent)
local actions = {"Resume", "Start", "Workshop", "Settings", "Exit"}
function MenuManager:__init(component)
    BaseComponent.__init(self, component)
    self.view = StartMenu(component)
    self.view.interactionDelay, self.view.debounceTime = 0, 0.2
    self.items, self.selected, self.visible = {}, 1, false
end
-- Reuse StartMenu's layout, buttons, listeners and generated-root ownership.
function MenuManager:show(title, subtitle, items, status)
    local view = self.view
    view.title, view.subtitle, view.versionText = title, subtitle or "", "WORKPHONE RACING"
    local _, breaks = view.subtitle:gsub("\n", "")
    view.subtitleHeight = math.max(48, (breaks + 1) * 24)
    view.buttonCentreY = math.min(110, 15 + math.max(0, view.subtitleHeight - 80))
    view.showResume, view.showWorkshop, view.showSettings, view.showExit = false, false, false, false
    view.callbacks, self.items = {}, {}
    for _, action in ipairs(actions) do
        local item = items[action]
        if item then
            view[action:lower() .. "Label"] = item[1]
            view["show" .. action] = true
            view:setCallback(action, item[2])
            self.items[#self.items + 1] = {action = action, label = item[1]}
        end
    end
    assert(items.Start, "StartMenu needs a primary action")
    self.selected, self.visible = 1, true
    assert(view:generate(), "Cannot create menu: " .. tostring(view.lastError))
    view:show()
    self.status = status or "Up/Down: choose   Enter: select   Esc: back"
    self:refreshSelection()
end
function MenuManager:refreshSelection()
    self.view:setStatus(self.status .. "\nSelected: " .. self.items[self.selected].label, false)
    for index, item in ipairs(self.items) do
        local button = self.view.buttons[item.action]
        if button then button:setNormalColour(index == self.selected and ColourF(0.18, 0.45, 0.73, 1)
            or ColourF(0.10, 0.19, 0.30, 1)) end
    end
end
function MenuManager:move(direction)
    self.selected = (self.selected - 1 + direction) % #self.items + 1
    self:refreshSelection()
end
function MenuManager:activate() self.view:dispatchAction(self.items[self.selected].action) end
function MenuManager:hide() self.visible = false; self.view:hide() end
function MenuManager:destroy(manager)
    if self.view.generatedRoot then manager:destroyActor(self.view.generatedRoot, true) end
    self.view.generatedRoot, self.visible = nil, false
end
